/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-support"


/** 
 * The following is an internal definition that allows to store a
 * search path along with the domain where the search for a file is
 * being requested.
 */
typedef struct _SearchPathNode SearchPathNode;
struct _SearchPathNode {
	/** 
	 * The domain where the lookup was requested (or will be
	 * requested by the application).
	 */
	char * domain;
	/** 
	 * The path that will be used associated to the domain, to
	 * perform lookups.
	 */
	char * path;
};

/** 
 * @internal Function that just creates a new search path node.
 * 
 * @param domain The domain to lookup.
 *
 * @param path The particular path to use for the lookup.
 * 
 * @return A newly allocated reference to the SearchPathNode created.
 */
SearchPathNode * __search_path_node_new (char * domain,
					 char * path)
{

	SearchPathNode * result;

	result         = axl_new (SearchPathNode, 1);
	result->domain = domain;
	result->path   = path;

	return result;
	
} /* end __search_path_node_new */

/** 
 * @internal Function used to destroy each path node.
 * 
 * @param node the node to be destroy once released the axlList.
 */
void __search_path_node_destroy (SearchPathNode * node)
{
	if (node == NULL)
		return;

	/* free the domain and the path */
	axl_free (node->domain);
	axl_free (node->path);
	axl_free (node);
	
	return;
}

/** 
 * @brief Inits the vortex support module using the context provided
 * (\ref VortexCtx). Function that checks and creates the search path
 * list.
 */
void vortex_support_init (VortexCtx * ctx)
{
	/* do not operate if a null value is received */
	v_return_if_fail (ctx);

	/* init search path (if it wasn't) */
	if (ctx->support_search_path == NULL) {
		ctx->support_search_path = axl_list_new (axl_list_always_return_1,
							 (axlDestroyFunc) __search_path_node_destroy);
	}

	/* init hte mutex */
	vortex_mutex_create (&ctx->search_path_mutex);

	/* init inet_ntoa mutex */
	vortex_mutex_create (&ctx->inet_ntoa_mutex);

	return;
}

/** 
 * @brief Allows to cleanup the vortex support module state from the
 * provided \ref VortexCtx object.
 * 
 * @param ctx The vortex context to clean up (only module specific
 * part).
 */
void vortex_support_cleanup (VortexCtx * ctx)
{

	/* do not operate if a null value is received */
	v_return_if_fail (ctx);

	/* clear search path */
	axl_list_free (ctx->support_search_path);
	ctx->support_search_path = NULL;

	/* destroy mutex */
	vortex_mutex_destroy (&ctx->search_path_mutex);

	/* terminate inet_ntoa mutex */
	vortex_mutex_destroy (&ctx->inet_ntoa_mutex);

	return;
}

/**
 * \defgroup vortex_support Vortex Support: Support function used across the library
 */

/**
 * \addtogroup vortex_support
 * @{
 */

typedef void (*DeallocFunction) (axlPointer data);

/** 
 * @brief Allows to perform several memory dispose operations at the
 * same time. 
 * 
 * This function is mainly by Vortex Library but it could be useful
 * for Vortex Library API consumers. Here is an example:
 * \code
 * // perform a massive dealloc operation
 * vortex_support_free (3,  // three memory disposal operations
 *                      // the first one
 *                      profile, axl_free, 
 *                      // the second one
 *                      channel, vortex_channel_unref
 *                      // the last one.
 *                      serverName, axl_free);
 * \endcode
 * 
 * As you may observed, you have to provide the reference to be
 * deallocated an a destroy function which accept the reference to be
 * unreferenced. 
 * 
 * Any mistake produce calling to this function without providing the
 * right parameters will cause unexpected behaviors and lot of fun
 * debugging memory misfunctions.
 * 
 * @param params The number of disposal operations to be performed.
 */
void   vortex_support_free (int params, ...)
{
	DeallocFunction        dealloc;
	axlPointer             data;
	int                    iterator;
	va_list                args;

	/* open the variable argument stack */
	va_start (args, params);

	/* init and loop over the iterator */
	iterator = 0;
	while (iterator < params) {

		/* get a reference to the deallocator and its resource
		   to deallocate */
		data    = va_arg (args, axlPointer);
		dealloc = va_arg (args, DeallocFunction);

		/* dealloc */
		if ((dealloc != NULL) && (data != NULL))
			dealloc (data);
		
		/* update the iterator status */
		iterator++;
	}

	/* end variable argument invocation */
	va_end (args);
	return;
}

/** 
 * @brief Allows to add new search path to be used by \ref vortex_support_find_data_file.
 *
 * This function allows to configure how Vortex Library lookups
 * files. This is important to locate dtd file definitions, SSL
 * certificates and so on. 
 *
 * You can also use this API to install and configure application
 * level files that will be used by your profile implementation.
 * 
 * Once a path is added, \ref vortex_support_find_data_file function
 * will lookup on the provided paths.
 *
 * \code 
 * // add all search paths (maybe from an user interface)
 * vortex_support_add_search_path (ctx, "/my/path/to/local/data");
 * vortex_support_add_search_path (ctx, "/my/alternative/path");
 *
 * // where ctx is an initialized VortexCtx object
 *
 * ...
 *
 * // write you file location code in an abstract manner so it could be
 * // used on every platform easily.
 * char  * my_data_def = vortex_support_find_data_file (ctx, "my_data.def");
 *
 * // NOTE that the file to lookup doesn't have any references to
 * // local paths
 * \endcode
 *
 * The function will add the search path using "default" as domain. If
 * you want to configure a more especific domain use \ref
 * vortex_support_add_domain_search_path.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param path A new path to be added. The function will perform a copy for the given path
 */
void     vortex_support_add_search_path (VortexCtx   * ctx, 
					 const char  * path)
{

	/* call to domain implementation with default vaule */
	vortex_support_add_domain_search_path (ctx, "default",  path);
	return;
}

/** 
 * @brief Allows to check if the provided path is alredy foudn in the
 * provided domain.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param domain The domain where the path will be checked to be
 * already added or not. This value cannot be NULL. Use "default" for
 * default path.
 *
 * @param path The path to be check to be already added or not. This
 * value cannot be NULL.
 *
 * @return axl_true in the case the search path is already added under
 * the domain provided, otherwise axl_false is returned. Keep in mind
 * the function will also return axl_false in the case path or domain
 * reference provided is NULL.
 */
axl_bool vortex_support_check_search_path          (VortexCtx  * ctx,
						    const char * domain,
						    const char * path)
{
	SearchPathNode * node;
	int              iterator;

	v_return_val_if_fail (ctx,    axl_false);
	v_return_val_if_fail (path,   axl_false);
	v_return_val_if_fail (domain, axl_false);
	
	vortex_mutex_lock (&ctx->search_path_mutex);

	/* check we do not add something already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->support_search_path)) {
		/* get node */
		node = axl_list_get_nth (ctx->support_search_path, iterator);
		
		if (axl_cmp (node->domain, domain) && axl_cmp (node->path, path)) {
			/* item already added, skip it */
			vortex_mutex_unlock (&ctx->search_path_mutex);	
			return axl_true;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end if */

	/* unlock */
	vortex_mutex_unlock (&ctx->search_path_mutex);	

	/* path do not exists */
	return axl_false;
}

/** 
 * @brief Adds a new path to be used while looking for files without
 * making a local copy.
 *
 * This function works like \ref vortex_support_add_search_path
 * without making a local copy. This function is useful when it is
 * needed to add an already allocated lookup path. 
 *
 * String provided as a path must be allocated using either axl_new or
 * axl_stream_* family of functions. Alternatively, you can use any
 * allocation function that uses calloc/malloc to allocate the memory
 * used by the string provided.
 *
 * If a static path is needed to be added \ref vortex_support_add_search_path should be used.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param path A new path to be added. Provided path reference mustn't be deallocated.
 */
void     vortex_support_add_search_path_ref (VortexCtx * ctx,
					     char      * path)
{

	/* check if the path already exists */
	if (vortex_support_check_search_path (ctx, "default", path)) {
		axl_free (path); /* free path since we own the ref but it won't be used */
		return; /* do not added it, already added */
	}

	/* call to default implementation with default domain */
	vortex_support_add_domain_search_path_ref (ctx, axl_strdup ("default"), path);

	return;
}

/** 
 * @brief Allows to define a new search path, providing the domain
 * that will apply.
 *
 * This function at its associated \ref
 * vortex_support_add_domain_search_path_ref, allow to configure the
 * search function \ref vortex_support_domain_find_data_file with new
 * search path included inside a domain.
 *
 * This function works like \ref vortex_support_add_search_path but
 * providing the default domain "default".
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param domain The domain where the lookup will be performed.
 * @param path The path to lookup for files once called \ref
 * vortex_support_domain_find_data_file.
 */
void     vortex_support_add_domain_search_path     (VortexCtx  * ctx,
						    const char * domain, 
						    const char * path)
{

	/* check if the path already exists */
	if (vortex_support_check_search_path (ctx, domain, path))
		return; /* do not added it, already added */

	/* call to default implementation */
	vortex_support_add_domain_search_path_ref (ctx, 
						   axl_strdup (domain),
						   axl_strdup (path));
	return;	
}

/** 
 * @brief Allows to define a new search path, providing the domain
 * that will apply, providing the values already allocated.
 * 
 * @param ctx The context where the operation will be performed.
 * @param domain The domain that will be assocaited to the path.
 * @param path The path to use to lookup for files once called \ref
 * vortex_support_domain_find_data_file.
 */
void     vortex_support_add_domain_search_path_ref (VortexCtx * ctx,
						    char      * domain, 
						    char      * path)
{
	SearchPathNode * node;
	int              iterator;


	/* check arguments received */
	if (path == NULL || ctx == NULL) {
		axl_free (domain);
		return;
	} /* end if */

	/* check if the path already exists */
	if (vortex_support_check_search_path (ctx, "default", path)) {
		/* release values even if we got them */
		axl_free (path);
		axl_free (domain);
		return; /* do not added it, already added */
	}
	
	vortex_mutex_lock (&ctx->search_path_mutex);

	/* check we do not add something already added */
	iterator = 0;
	while (iterator < axl_list_length (ctx->support_search_path)) {
		/* get node */
		node = axl_list_get_nth (ctx->support_search_path, iterator);
		
		if (axl_cmp (node->domain, domain) && axl_cmp (node->path, path)) {
			/* release values even if we got them */
			axl_free (path);
			axl_free (domain);

			/* item already added, skip it */
			vortex_mutex_unlock (&ctx->search_path_mutex);	
			return;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end if */

	/* create the search path node */
	node = __search_path_node_new (domain, path);

	/* create the search path list */
	axl_list_add (ctx->support_search_path, node);

	vortex_mutex_unlock (&ctx->search_path_mutex);	

	return;
}

/** 
 * @brief Allows to lookup for a file into the known vortex data file
 * locations.
 * 
 * While locating data files inside Vortex Library context a set of
 * function are used to allows Vortex Library API consumers to produce
 * easily application level code that is platform/directory-structure
 * independent as possible. 
 * 
 * Because profile definitions needs DTD files, certificates, xml
 * documents, etc, a common problem to face is how to locate this
 * files not only into the development environment but also into the
 * production one.
 * 
 * Vortex Library allows to configure known file locations using \ref
 * vortex_support_add_search_path and \ref vortex_support_add_search_path_ref. 
 *
 * Once configured search paths, application level calls to this
 * function to locate files using basenames such as
 * <b>myCertificate.cert</b>, <b>channel.dtd</b>, etc. This avoid full
 * paths file names that are a problem while moving around the source
 * code not only across different platforms but also on the same.
 *
 * See \ref vortex_support_add_search_path for more information about
 * known data files location.
 *
 * This function could have a problem while looking for sensitive
 * files. If a path is not properly configured, an attacker is able to
 * place a manipulated copy for the file inside a valid path, but not
 * the one expected. In such case you must perform domain constrained
 * searches using \ref vortex_support_domain_find_data_file and its
 * associated function to install search path with a domain constraing
 * them:
 *
 *   - \ref vortex_support_add_domain_search_path
 *   - \ref vortex_support_add_domain_search_path_ref
 *
 * @param ctx The context where the operation will be performed.
 * @param name the base file name to lookup.
 * 
 * @return NULL or the file path. Value returned should be unrefered using axl_free when no longer needed.
 */
char   * vortex_support_find_data_file (VortexCtx   * ctx, 
					const char  * name)
{
	return vortex_support_domain_find_data_file (ctx, "default", name);
}

/** 
 * @brief Perform a file lookup, providing a domain at the file to
 * lookup, using current search path configuration for the domain.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param domain The domain where the search operation will be
 * performed.
 *
 * @param name The name to lookup.
 * 
 * @return A newly allocated full path reference to the item located,
 * inside the domain provided or NULL if it fails.
 */
char   * vortex_support_domain_find_data_file      (VortexCtx  * ctx,
						    const char * domain, 
						    const char * name)
{
	axlListCursor  * cursor;
	SearchPathNode * node;
	char           * file_name;

	v_return_val_if_fail (domain, NULL);
	v_return_val_if_fail (name,   NULL);
	v_return_val_if_fail (ctx,    NULL);

	vortex_mutex_lock (&ctx->search_path_mutex);	

	/* foreach path installed */
	cursor   = axl_list_cursor_new (ctx->support_search_path);
	while (axl_list_cursor_has_item (cursor)) {

		/* get the item */
		node = axl_list_cursor_get (cursor);

		/* check the domain */
		if (axl_cmp (node->domain, domain)) {
			
			/* build a file path, trying to build ./ or .\ file
			 * path if path "." is used */
			if (axl_cmp (node->path, "."))
				file_name = axl_strdup (name);
			else
				file_name = axl_strdup_printf ("%s%s%s", node->path, VORTEX_FILE_SEPARATOR, name);

			/* check the file to be found */
			if (vortex_support_file_test (file_name, FILE_EXISTS | FILE_IS_REGULAR)) {
				/* free the cursor */
				axl_list_cursor_free (cursor);

				vortex_log (VORTEX_LEVEL_DEBUG, "file found at: %s, %d, %p", file_name, 
					    axl_list_length (ctx->support_search_path), ctx->support_search_path);
				vortex_mutex_unlock (&ctx->search_path_mutex);	
				return file_name;
			} /* end if */
			
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to find file %s on '%s', %d, %p",  
				    name, file_name, axl_list_length (ctx->support_search_path), ctx->support_search_path);
			axl_free (file_name);

		} /* end for */

		/* get the next item */
		axl_list_cursor_next (cursor);
		
	} /* end while */

	/* free the cursor */
	axl_list_cursor_free (cursor);

	vortex_log (VORTEX_LEVEL_DEBUG, "unable to find %s inside domain: '%s', %d, %p", name, domain,
		    axl_list_length (ctx->support_search_path), ctx->support_search_path);


	vortex_mutex_unlock (&ctx->search_path_mutex);
	return NULL;	
}

/**
 * @brief Allows to get the integer value stored in the provided
 * environment varible.
 *
 * The function tries to get the content from the environment
 * variable, and return the integer content that it is
 * representing. The function asumes the environment variable provides
 * has a numeric value. 
 * 
 * @return The variable numeric value. If the variable is not defined,
 * then 0 will be returned.
 */
int      vortex_support_getenv_int                 (const char * env_name)
{
#if defined (AXL_OS_UNIX)
	/* get the variable value */
	char * variable = getenv (env_name);

	if (variable == NULL)
		return 0;
	
	/* just return the content translated */
	return (strtol (variable, NULL, 10));
#elif defined(AXL_OS_WIN32)
	char  variable[1024];
	int   size_returned = 0;
	int   value         = 0;

	/* get the content of the variable */
	memset (variable, 0, sizeof (char) * 1024);
	size_returned = GetEnvironmentVariableA (env_name, variable, 1023);

	if (size_returned > 1023) {
		return 0;
	}
	
	/* return the content translated */
	value = (strtol (variable, NULL, 10));

	return value;
#endif	
}

/** 
 * @brief Allows to get the string variable found at the provided
 * env_name.
 *
 * The function tries to get the content from the environment
 * variable, and return the string content that it is
 * representing. 
 * 
 * @return The variable value or NULL if it fails. The caller must
 * dealloc the string returned when no longer needed by calling to
 * axl_free.
 */
char *   vortex_support_getenv                 (const char * env_name)
{
#if defined (AXL_OS_UNIX)
	/* get the variable value */
	char * variable = getenv (env_name);

	if (variable == NULL)
		return NULL;
	
	/* just return the content translated */
	return axl_strdup (variable);

#elif defined(AXL_OS_WIN32)

	char  variable[1024];
	int   size_returned = 0;

	/* get the content of the variable */
	memset (variable, 0, sizeof (char) * 1024);
	size_returned = GetEnvironmentVariableA (env_name, variable, 1023);

	if (size_returned > 1023) {
		return 0;
	}
	
	/* return the content translated */
	return axl_strdup (variable);
#endif	
}

/**
 * @brief Allows to configure the environment value identified by
 * env_name, with the value provided env_value.
 *
 * @param env_name The environment variable to configure.
 *
 * @param env_value The environment value to configure. The value
 * provide must be not NULL. To unset an environment variable use \ref vortex_support_unsetenv
 *
 * @return axl_true if the operation was successfully completed, otherwise
 * axl_false is returned.
 */
axl_bool     vortex_support_setenv                     (const char * env_name, 
							const char * env_value)
{
	/* check values received */
	if (env_name == NULL || env_value == NULL)
		return axl_false;
	
#if defined (AXL_OS_WIN32)
	/* use windows implementation */
	return SetEnvironmentVariableA (env_name, env_value);
#elif defined(AXL_OS_UNIX)
	/* use the unix implementation */
	return setenv (env_name, env_value, 1) == 0;
#endif
}

/**
 * @brief Allows to unset the provided environment variable.
 *
 * @param env_name The environment variable to unset its value.
 *
 * @return axl_true if the operation was successfully completed, otherwise
 * axl_false is returned.
 */
axl_bool      vortex_support_unsetenv                   (const char * env_name)
{
	/* check values received */
	if (env_name == NULL)
		return axl_false;

#if defined (AXL_OS_WIN32)
	/* use windows implementation */
	return SetEnvironmentVariableA (env_name, NULL);
#elif defined(AXL_OS_UNIX)
	/* use the unix implementation */
	setenv (env_name, "", 1);
		
	/* always axl_true */
	return axl_true;
#endif
}

/** 
 * @brief Allows to create a path to a filename, by providing its
 * tokens, ending them with NULL.
 * 
 * @param name The first token to be provided, followed by the rest to
 * tokens that conforms the path, ended by a NULL terminator.
 * 
 * @return A newly allocated string that must be deallocated using
 * axl_free.
 */
char   * vortex_support_build_filename      (const char * name, ...)
{
	va_list   args;
	char    * result;
	char    * aux    = NULL;
	char    * token;

	/* do not produce a result if a null is received */
	if (name == NULL)
		return NULL;

	/* initialize the args value */
	va_start (args, name);

	/* get the token */
	result = axl_strdup (name);
	token  = va_arg (args, char *);
	
	while (token != NULL) {
		aux    = result;
		result = axl_strdup_printf ("%s%s%s", result, VORTEX_FILE_SEPARATOR, token);
		axl_free (aux);

		/* get next token */
		token = va_arg (args, char *);
	} /* end while */

	/* end args values */
	va_end (args);

	return result;
}

/** 
 * @brief Tries to translate the double provided on the string
 * received, doing the best effort (meaning that the locale will be
 * skiped).
 * 
 * @param param The string that is considered to contain a double
 * value.
 *
 * @param string_aux A reference to a pointer that signal the place
 * were an error was found. Optional argument.
 * 
 * @return The double value representing the string received or 0.0 if
 * it fails.
 */
double   vortex_support_strtod                     (const char * param, char ** string_aux)
{
	double     double_value;
	axl_bool   second_try = axl_false;
	char     * alt_string = NULL;

	/* provide a local reference */
	if (string_aux == NULL)
		string_aux = &alt_string;

	/* try to get the double value */
 try_again:
	double_value = strtod (param, string_aux);
	if (string_aux != NULL && strlen (*string_aux) == 0) {
		return double_value;
	}

	if ((! second_try) && (string_aux != NULL && strlen (*string_aux) > 0)) {

		/* check if the discord value that makes POSIX
		 * designers mind to be not possible to translate the
		 * double value */
		if ((*string_aux) [0] == '.') {
			(*string_aux) [0] = ',';
			second_try     = axl_true;
		} else if ((*string_aux) [0] == ',') {
			(*string_aux) [0] = '.';
			second_try     = axl_true;
		}

		/* check if we can try again */
		if (second_try)
			goto try_again;
	}

	/* unable to find the double value, maybe it is not a double
	 * value */
	return 0.0;
}

/** 
 * @brief Allows to convert the provided integer value into its string
 * representation leaving the result on the provided buffer.
 *
 * @param value The value to convert.
 * @param buffer Pointer to the buffer that will hold the result.
 * @param buffer_size The size of the buffer that will hold the result.
 *
 * Note the function does not place a NUL char at the end of the number
 * written.
 * 
 * @return The function returns bytes written into the buffer or -1 if
 * the buffer can't hold the content.
 */ 
int      vortex_support_itoa                       (unsigned int    value,
						    char          * buffer,
						    int             buffer_size)
{
	static char digits[] = "0123456789";
	char        inverse[10];
	int         iterator  = 0;
	int         iterator2 = 0;

	if (buffer_size <= 0)
		return -1;

	/* do the conversion */
	while (iterator < 10) {
		/* copy content */
		inverse[iterator] = digits[value % 10];

		/* reduce the value */
		value = value / 10;

		if (value == 0)
			break;
		iterator++;
	} /* end while */

	/* now reserve the content */
	while (iterator2 < buffer_size) {
		buffer[iterator2] = inverse[iterator];
		iterator2++;
		iterator--;

		if (iterator == -1)
			break;
			
	} /* end while */
    
	/* check result */
	if (iterator != -1) 
		return -1;

	/* return size created */
	return iterator2;
}

/** 
 * @brief Performs a timeval substract leaving the result in
 * (result). Subtract the `struct timeval' values a and b, storing the
 * result in result.  
 *
 * @param a First parameter to substract
 *
 * @param b Second parameter to substract
 *
 * @param result Result variable. Do no used a or b to place the
 * result.
 *
 * @return 1 if the difference is negative, otherwise 0 (operations
 * implemented is a - b).
 */ 
int     vortex_timeval_substract                  (struct timeval * a, 
						   struct timeval * b,
						   struct timeval * result)
{
	/* Perform the carry for the later subtraction by updating
	 * y. */
	if (a->tv_usec < b->tv_usec) {
		int nsec = (b->tv_usec - a->tv_usec) / 1000000 + 1;
		b->tv_usec -= 1000000 * nsec;
		b->tv_sec += nsec;
	}

	if (a->tv_usec - b->tv_usec > 1000000) {
		int nsec = (a->tv_usec - b->tv_usec) / 1000000;
		b->tv_usec += 1000000 * nsec;
		b->tv_sec -= nsec;
	}
	
	/* get the result */
	result->tv_sec = a->tv_sec - b->tv_sec;
	result->tv_usec = a->tv_usec - b->tv_usec;
     
       /* return 1 if result is negative. */
       return a->tv_sec < b->tv_sec;	
}

/**
 * @brief Thread safe implementation for inet_ntoa.
 *
 * @param ctx The vortex context where the operation will be
 * performed.  @param sin Socket information where to get inet_ntoa
 * result.
 *
 * @return A newly allocated string that must be deallocated with
 * axl_free that contains the host information. The function return
 * NULL if it fails.
 */
char   * vortex_support_inet_ntoa                  (VortexCtx           * ctx, 
						    struct sockaddr_in  * sin)
{
	char * result;

	v_return_val_if_fail (ctx && sin, NULL);

	/* lock during operation */
	vortex_mutex_lock (&ctx->inet_ntoa_mutex);

	/* allocate the string */
	result = axl_strdup (inet_ntoa (sin->sin_addr));

	/* unlock */
	vortex_mutex_unlock (&ctx->inet_ntoa_mutex);

	/* return the string */
	return result;
}

/** 
 * @brief Creates a portable pipe (by creating a socket connected
 * pair).
 *
 * @param descf The pointer to the buffer where descriptors will be
 * stored.
 *
 * @param ctx The context where the operation will take place.
 *
 * @return The function returns -1 in the case of failure or 0 if pipe
 * was properly created.
 */
int      vortex_support_pipe                       (VortexCtx * ctx, int descf[2])
{
	struct sockaddr_in      saddr;
	struct sockaddr_in      sin;

	VORTEX_SOCKET           listener_fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                    unit      = axl_true; */
	int                     sin_size  = sizeof (sin);
#else    	
	int                     unit      = 1; 
	socklen_t               sin_size  = sizeof (sin);
#endif	  
	int                     bind_res;
	int                     result;

	/* create listener socket */
	if ((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create listener socket: %d (errno=%d:%s)", listener_fd, errno, vortex_errno_get_error (errno));
		return -1;
        } /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt (listener_fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = 0;
	saddr.sin_addr.s_addr     = htonl (INADDR_LOOPBACK);

	/* call to bind */
	bind_res = bind (listener_fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
	if (bind_res == VORTEX_SOCKET_ERROR) {
		if (errno == EADDRNOTAVAIL) {
			/* generic error */
			vortex_log (VORTEX_LEVEL_CRITICAL, "vortex_support_pipe () failing: LOOPBACK interface is not working. Errno reported EADDRNOTAVAIL. Check your system configuration to have loopback device working. Error reported errno=%d (%s). Closing socket: %d",
				    errno, vortex_errno_get_error (errno), listener_fd);
		} else {
			/* generic error */
			vortex_log (VORTEX_LEVEL_CRITICAL, "vortex_support_pipe () failing: unable to bind address (port already in use or insufficient permissions). Error reported errno=%d (%s). Closing socket: %d",
				    errno, vortex_errno_get_error (errno), listener_fd);
		} /* end if */
		vortex_close_socket (listener_fd);
		return -1;
	}
	
	if (listen (listener_fd, 1) == VORTEX_SOCKET_ERROR) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex_support_pipe () failing: an error have occur while executing listen");
		vortex_close_socket (listener_fd);
		return -1;
        } /* end if */

	/* notify listener */
	if (getsockname (listener_fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex_support_pipe () failing: an error have happen while executing getsockname");
		vortex_close_socket (listener_fd);
		return -1;
	} /* end if */

	vortex_log  (VORTEX_LEVEL_DEBUG, "created listener running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), listener_fd);

	/* on now connect: read side */
	descf[0]      = socket (AF_INET, SOCK_STREAM, 0);
	if (descf[0] == VORTEX_INVALID_SOCKET) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex_support_pipe () failing: Unable to create socket required for pipe");
		vortex_close_socket (listener_fd);
		return -1;
	} /* end if */

	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (descf[0], axl_true);

	/* set non blocking connection */
	vortex_connection_set_sock_block (descf[0], axl_false);  

        memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr     = htonl(INADDR_LOOPBACK);
        saddr.sin_family          = AF_INET;
        saddr.sin_port            = sin.sin_port;

	/* connect in non blocking manner */
	result = connect (descf[0], (struct sockaddr *)&saddr, sizeof (saddr));
	if (result < 0 && errno != VORTEX_EINPROGRESS) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "connect () returned %d, errno=%d:%s", 
			    result, errno, vortex_errno_get_last_error ());
		vortex_close_socket (listener_fd);
		return -1;
	}

	/* accept connection */
	vortex_log  (VORTEX_LEVEL_DEBUG, "calling to accept () socket");
	descf[1] = vortex_listener_accept (listener_fd);

	if (descf[1] <= 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Unable to accept connection, failed to create pipe");
		vortex_close_socket (listener_fd);
		return -1;
	}
	/* set pipe read end from result returned by thread */
	vortex_log (VORTEX_LEVEL_DEBUG, "Created pipe [%d, %d]", descf[0], descf[1]);

	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (descf[1], axl_true);

	/* close listener */
	vortex_close_socket (listener_fd);

	/* report and return fd */
	return 0;
}

/** 
 * @brief Allows to perform a set of test for the provided path.
 * 
 * @param path The path that will be checked.
 *
 * @param test The set of test to be performed. Separate each test
 * with "|" to perform several test at the same time.
 * 
 * @return axl_true if all test returns axl_true. Otherwise axl_false is returned.
 */
axl_bool    vortex_support_file_test (const char * path, VortexFileTest test)
{
	axl_bool    result = axl_false;
	struct stat file_info;

	/* perform common checks */
	axl_return_val_if_fail (path, axl_false);

	/* call to get status */
	result = (stat (path, &file_info) == 0);
	if (! result) {
		/* check that it is requesting for not file exists */
		if (errno == ENOENT && (test & FILE_EXISTS) == FILE_EXISTS)
			return axl_false;
		return axl_false;
	} /* end if */

	/* check for file exists */
	if ((test & FILE_EXISTS) == FILE_EXISTS) {
		/* check result */
		if (result == axl_false)
			return axl_false;
		
		/* reached this point the file exists */
		result = axl_true;
	}

	/* check if the file is a link */
	if ((test & FILE_IS_LINK) == FILE_IS_LINK) {
		if (! S_ISLNK (file_info.st_mode))
			return axl_false;

		/* reached this point the file is link */
		result = axl_true;
	}

	/* check if the file is a regular */
	if ((test & FILE_IS_REGULAR) == FILE_IS_REGULAR) {
		if (! S_ISREG (file_info.st_mode))
			return axl_false;

		/* reached this point the file is link */
		result = axl_true;
	}

	/* check if the file is a directory */
	if ((test & FILE_IS_DIR) == FILE_IS_DIR) {
		if (! S_ISDIR (file_info.st_mode)) {
			return axl_false;
		}

		/* reached this point the file is link */
		result = axl_true;
	}

	/* return current result */
	return result;
}

/* @} */

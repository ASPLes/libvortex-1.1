/**
 *  LibExploreArguments: a library to parse command line options
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
 * 
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are wellcome to
 *  develop propietary applications using this library withtout any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
 *
 *  Contact us at:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 *
 */
#include <exarg.h>

#if defined(__GNUC__)
#  ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#  endif
#endif

/* local includes */
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define LOG_DOMAIN "exarg"

typedef struct _ExArgNodeOption ExArgNodeOption;

/** 
 * @internal Definition for the void *.
 */
typedef void * epointer;

/** 
 * @brief First argument option supported (this is the head of the
 * list of arguments supported).
 */
ExArgNodeOption * argument_options          = NULL;
ExArgument      * params                    = NULL;
const char      * exarg_exec_name           = NULL;
char            * __exarg_usage_header      = NULL;
char            * __exarg_help_header       = NULL;
char            * __exarg_post_usage_header = NULL;
char            * __exarg_post_help_header  = NULL;
int               __exarg_disable_free_args = 0;

typedef enum { 
	/** 
	 * @internal Parse mode not defined.
	 */
	PARSE_MODE_NOT_DEF,
	/** 
	 * @internal Parse mode unix defined.
	 */
	PARSE_MODE_UNIX,
	/** 
	 * @internal Parse module windows defined.
	 */
	PARSE_MODE_WINDOWS
} ExArgParseMode;

/** 
 * @internal This allows to know the parse mode used by exarg. Parse
 * mode allows to differenciate arguments that are file path under
 * unix by options under windows. This option is automatic, being the
 * first option parse to configure the parse mode. This means that the
 * user can't mix options provided using the windows and unix mode at
 * the same time.
 */
ExArgParseMode   parse_mode = PARSE_MODE_NOT_DEF;

/** 
 * @internal Function used by the exarg library to drop a message.
 * 
 * @param format The message to print using a printf format.
 */
void exarg_msg (char * format, ...)
{
	va_list args;

	/* print the message */
	va_start (args, format);
	vprintf (format, args);
	va_end (args);

	/* flush */
	fflush (stdout);

	/* exit from the library */
	exit (-1);

	return;
}

/** 
 * @brief Allows to check current parse mode, return ok if the parse
 * mode isn't defined or the current parse mode is the one provided.
 * 
 * @param mode The parse mode to check.
 * 
 * @return 1 if the parse mode is ok or 0 if current parse mode is
 * incompatible.
 */
#define CHECK_PARSE_MODE(mode) ((parse_mode == PARSE_MODE_NOT_DEF) || (parse_mode == mode))

/** 
 * @brief Allows to update current parse mode.
 * 
 * @param mode The parse mode to configure.
 * 
 */
#define UPDATE_PARSE_MODE(mode)\
if (parse_mode == PARSE_MODE_NOT_DEF)\
   parse_mode = mode;

/**
 * \mainpage LibExArg (Explore Arguments Library)  Manual
 * \section Introduction
 *
 *  LibExArg library is a small, easy to use with a consistent API
 *  which allows you to parse command line argument for your program.
 *
 *  It doesn't require any external dependency for its function, it
 *  really portable and at this point is running on <b>GNU/Linux</b>
 *  platforms and <b>Microsoft Windows</b> platforms.
 *
 *  For details on how to starting to use LibExarg, you can find the
 *  \ref exarg "module documentation for LibExArg here". You can find
 *  \ref exarg_example "here an example about using LibExArg".
 *
 * \section licence Library License
 *
 *  The library is released under the terms of the GNU LGPL (The GNU
 *  Lesser General Public License) licence. You can get a copy of the
 *  license http://www.gnu.org/licenses/licenses.html#LGPL.
 *
 *  This license allows you to write applications using LibExArg which
 *  are using some GPL compatible license or commercial applications.
 *
 */

/**
 * \page exarg_example A simple example on using LibExarg
 *
 * \section Introduction
 *
 *  You may find interesting the following example which makes use of
 *  LibExArg. This example install 3 arguments to be accepted as valid
 *  command line options and the write down to the terminal the result
 *  of those command used by the user.
 * 
 *  Once the arguments are installed, the user can execute 
 *
 *
 *  \include "test-exarg.c"
 * 
 */

/** \defgroup exarg ExArg module: functions to parse command line argument in your program
 *
 */

/** 
 * @brief Argument dependency (a list of nodes that each argument has
 * to define for its dependencies and its mutual exclusions).
 */
typedef struct _ExArgDependency ExArgDependency;

struct _ExArgDependency {
	ExArgNodeOption * node;
	ExArgDependency  * next;
};


/**
 * \addtogroup exarg
 * @{ 
 */

struct _ExArgNodeOption {
	/** 
	 * @brief Holds the argument name (long value recognized by
	 * --long-option and /long-option).
	 */
	const char      * arg_name;
	/** 
	 * @brief Holds the optional short argument option.
	 */
	const char      * arg_short_name;
	/** 
	 * @brief Argument type.
	 */
	ExArgType         type;
	/** 
	 * @brief Argument description.
	 */
	const char      * description;
	/** 
	 * @brief If the argument is \ref EXARG_STRING, this holds the
	 * string value associated if defined.
	 */
	char            * string_value;
	/** 
	 * @brief If the argument is \ref EXARG_INT, this holds the
	 * int value associated if defined.
	 */
	int               int_value;
	/** 
	 * @brief Boolean value that allows to know if the argument
	 * option was defined
	 */
	int               is_defined;

	/** 
	 * @brief Allows to configure if the provided argument is
	 * optional.
	 */
	int               is_optional;

	/** 
	 * @brief Pointer to the next argument option stored.
	 */
	ExArgNodeOption * next;

	/* a reference to the first dependecy argument */
	ExArgDependency * depends;

	ExArgDependency * excludes;
};

struct _ExArgument {
	/** 
	 * @brief The string value that is encapsulated by the
	 * argument.
	 */
	char       * string;

	/** 
	 * @brief A reference to the next argument in the list.
	 */
	ExArgument * next;
};


/** 
 * @internal Definition to allocate memory.
 * 
 * @param type The type to allocate
 * @param count The amount of memory to allocate.
 * 
 * @return Returns a newly allocated memory.
 */
#define exarg_new(type, count) (type *) calloc (count, sizeof (type))

/** 
 * @internal Definition to dealloc an object.
 * 
 * @param ref The reference to dealloc.
 */
void exarg_free (epointer ref)
{
	if (ref == NULL)
		return;
	free (ref);
	
	return;
}

/** 
 * @internal Allows to get the basename for the file path provided.
 * 
 * @param file_path The file path that is requested to return the
 * basename value.
 * 
 * @return A reference to the basename (the return must not be
 * dealloc).
 */
const char * exarg_basename (char * file_path) 
{
	int iterator = strlen (file_path);

	while ((iterator != 0) && 
	       (file_path [iterator - 1]  != '/') && 
	       (file_path [iterator - 1]  != '\\')) {
		/* decrease iterator */
		iterator--;
	}

	/* return the base name */
	return file_path + iterator;
}

/** 
 * @internal Allows to split the provided string using the provided
 * separators.
 * 
 * @param chunk The chunk to split.
 * @param separator_num The number of separators.
 * 
 * @return A newly allocated set of strings.
 */
char     ** exarg_split           (const char * chunk, int separator_num, ...)
{
	va_list      args;
	char      ** separators;
	char      ** result;
	int          iterator;
	int          index;
	int          previous_index;
	int          count      = 0;
	int          length     = 0;

	/* check received values */
	if (chunk == NULL)
		return NULL;
	if (separator_num < 1)
		return NULL;

	separators = exarg_new (char *, separator_num + 1);
	iterator   = 0;
	va_start (args, separator_num);

	/* get all separators to be used */
	while (iterator < separator_num) {
		separators[iterator] = va_arg (args, char *);
		iterator++;
	}
	
	va_end (args);

	/* now, count the number of strings that we will get by
	 * separating the string into several pieces */
	index    = 0;
	while (*(chunk + index) != 0) {

		/* reset the iterator */
		iterator = 0;
		while (iterator < separator_num) { 

			/* compare the current index with the current
			 * separator */
			length = strlen (separators[iterator]);
			if (! memcmp (chunk + index, separators[iterator], length)) {

				/* update items found */
				count++;

				/* update index to skip the item found */
				index += length - 1; /* make the last index to be captured the the -1 */

				/* break the loop */
				break;
			}
			iterator++;
		}

		/* update the index to the next item */
		index++;
	}
	
	/* create the result that will hold items separated */
	result = exarg_new (char *, count + 2);

	/* now copy items found */
	count          = 0;
	index          = 0;

	/* remember previous_index */
	previous_index = index;
	while (*(chunk + index) != 0) {

		/* reset the iterator */
		iterator = 0;
		while (iterator < separator_num) { 

			/* compare the current index with the current
			 * separator */
			length = strlen (separators[iterator]);
			if (! memcmp (chunk + index, separators[iterator], length)) {

				/* copy the chunk found */
				result[count] = exarg_new (char, index - previous_index + 1);
				memcpy (result[count], chunk + previous_index, index - previous_index);

				/* update items found */
				count++;

				/* update index to skip the item found */
				if (*(chunk + index + length) == 0) {
					/* in the case no more elements to read will be found */
					/* put an empty space at the end */
					result [count]    = exarg_new (char, 1);
					
					exarg_free (separators);
					return result;
				}

				/* remember previous_index */
				index += length; 
				previous_index = index;
				index--; /* make the last index to be captured the the -1 */
				break;
			}
			iterator++;
		}

		/* update the index to the next item */
		index++;
	}

	/* check for a last chunk */
	if (index != previous_index) {
		/* copy the chunk found */
		result[count] = exarg_new (char, index - previous_index + 1);
		memcpy (result[count], chunk + previous_index, index - previous_index);
	}

	
	/* release memory */
	exarg_free (separators);
	
	return result;
}

/** 
 * @internal Proto-type declaration to avoid gcc complaining.
 */
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

/** 
 * @internal Allows to calculate the amount of memory required to
 * store the string that will representing the construction provided
 * by the printf-like format received and its arguments.
 * 
 * @param format The printf-like format to be printed.
 *
 * @param args The set of arguments that the printf applies to.
 *
 * <i><b>NOTE:</b> not all printf specification is supported. Generally, the
 * following is supported: %s, %d, %f, %g, %ld, %lg and all
 * combinations that provides precision, number of items inside the
 * integer part, etc: %6.2f, %+2d, etc. An especial case not supported
 * is %lld, %llu and %llg.</i>
 *
 * @return Return the number of bytes that must be allocated to hold
 * the string (including the string terminator \0). If the format is
 * not correct or it is not properly formated according to the value
 * found at the argument set, the function will return -1.
 */
int exarg_vprintf_len (const char * format, va_list args)
{
	/** IMPLEMENTATION NOTE: in the case this code is update,
	 * update axl_stream_vprintf_len **/

# if defined (OS_WIN32) && ! defined (__GNUC__)
#   if HAVE_VSCPRINTF
	if (format == NULL)
		return 0;
	return _vscprintf (format, args) + 1;
#   else
	char buffer[8192];
	if (format == NULL)
		return 0;
	return _vsnprintf (buffer, 8191, format, args) + 1;
#   endif
#else
	/* gnu gcc case */
	if (format == NULL)
		return 0;
	return vsnprintf (NULL, 0, format, args) + 1;

#endif
}



/** 
 * @internal Allows to produce an string representing the message hold by
 * chunk with the parameters provided.
 * 
 * @param chunk The message chunk to print.
 * @param args The arguments for the chunk.
 * 
 * @return A newly allocated string.
 */
char  * exarg_strdup_printfv    (char * chunk, va_list args)
{
	/** IMPLEMENTATION NOTE: place update axl_stream_printf_buffer
	 * code in the case this code is updated **/

#ifndef HAVE_VASPRINTF
	int       size;
#endif
	char    * result   = NULL;
	int       new_size = -1;

	if (chunk == NULL)
		return NULL;

#ifdef HAVE_VASPRINTF
	/* do the operation using the GNU extension */
	new_size = vasprintf (&result, chunk, args);
#else
	/* get the amount of memory to be allocated */
	size = exarg_vprintf_len (chunk, args);

	/* check result */
	if (size == -1) {
		__axl_log (LOG_DOMAIN, AXL_LEVEL_CRITICAL, "unable to calculate the amount of memory for the strdup_printf operation");
		return NULL;
	} /* end if */

	/* allocate memory */
	result   = axl_new (char, size + 2);
	
	/* copy current size */
#if defined(OS_WIN32) && ! defined (__GNUC__)
	new_size = _vsnprintf_s (result, size + 1, size, chunk, args);
#else
	new_size = vsnprintf (result, size + 1, chunk, args);
#endif
#endif
	/* return the result */
	return result;
}

/** 
 * @internal Implementation that allows to produce dinamically
 * allocated strings using printf-like format.
 * 
 * @param chunk The chunk representing the format.
 * 
 * @return An dynamically allocated string.
 */
char      * exarg_strdup_printf   (char * chunk, ...)
{
	char    * result   = NULL;
	va_list   args;

	/* return a null value if null chunk is received */
	if (chunk == NULL)
		return NULL;

	/* open std args */
	va_start (args, chunk);

	/* get the string */
	result = exarg_strdup_printfv (chunk, args);
	
	/* close std args */
	va_end (args);
	
	return result;
}

/** 
 * @internal Allows to copy the given chunk, supposing that is a
 * properly format C string that ends with a '\\0' value.
 *
 * This function allows to perform a copy for the given string. If a
 * copy limited by a size is required, use \ref axl_stream_strdup_n.
 * 
 * @param chunk The chunk to copy
 * 
 * @return A newly allocated string or NULL if fails.
 */
char      * exarg_strdup          (char * chunk)
{
	char * result;
	int    length;

	/* do not copy if null reference is received */
	if (chunk == NULL)
		return NULL;

	length = strlen (chunk);
	result = exarg_new (char, length + 1);
	
	memcpy (result, chunk, length);

	return result;
}

/** 
 * @internal Internal function that allows to dealloc the provided array of chunks.
 * 
 * @param chunks An array containing pointers to chunks.
 */
void        exarg_freev           (char ** chunks)
{
	int iterator = 0;

	/* return if the received reference is null */
	if (chunks == NULL)
		return;
	 
	/* release memory used by all elements inside the chunk */
	while (chunks[iterator] != 0) {
		exarg_free (chunks[iterator]);
		iterator++;
	}
	
	/* now release the chunk inside */
	exarg_free (chunks);
	
	/* nothing more to do */
	return;
}


/** 
 * @internal Adds the argument to the current argument list.
 * 
 * @param argument The argument to add.
 */
void __exarg_add_argument (char * argument)
{
	ExArgument * arg;
	ExArgument * arg2;

	/* creates the node */
	arg         = exarg_new (ExArgument, 1);
	arg->string = argument;

	if (params == NULL) {
		/* basic case, only one argument */
		params = arg;
		
		return;
	} /* end if */

	/* complex case, lookup for the last node */
	arg2 = params;
	while (arg2->next != NULL)
		arg2 = arg2->next;

	/* set the argument on the last position */
	arg2->next = arg;

	return;
}

/**
 * @internal
 * @brief Perfom a look up inside the argument option installed.
 *
 * This function tries to return the ExArgNodeOption using the given
 * value as index key. 
 *
 * The given value can represent the long key option or the short
 * form.
 * 
 * @param arg_name the key to use on lookup process
 *
 * @return the function retuns the argument option if found or NULL if
 * not.
 */
ExArgNodeOption * exarg_lookup_node (const char * arg_name)
{
	ExArgNodeOption * result      = NULL;
	int               arg_name_len;

	/* check for null value */
	if (arg_name == NULL)
		return NULL;

	/* get length */
	arg_name_len = strlen (arg_name);

	/* look up the item on the current argument option */
	result = argument_options;
	while (result != NULL) {

		/* check value with long argument form */
		if (arg_name_len == strlen (result->arg_name) &&
		    !memcmp (arg_name, result->arg_name, strlen (arg_name))) {

			/* return found node */
			return result;
		}
		
		/* check value with short argument form */
		if ((result->arg_short_name != NULL) && 
		    (arg_name_len == strlen (result->arg_short_name)) &&
		    !memcmp (arg_name, result->arg_short_name, strlen (arg_name))) {

			/* return node found */
			return result;
		}
		
		/* update to the next item */
		result = result->next;

	} /* end while */

	/* return that we didn't found an item */
	return result;
	
} /* end exarg_lookup_node */




int exarg_is_argument (char * argument) 
{

	int iterator = 1;

	/* support standard unix format definition */
	if (CHECK_PARSE_MODE (PARSE_MODE_UNIX) && !memcmp (argument, "--", 2)) {

		/* update parse mode */
		UPDATE_PARSE_MODE (PARSE_MODE_UNIX);
		return 1;
	}

	/* support sort argument format definition */
	if (CHECK_PARSE_MODE (PARSE_MODE_UNIX) && (strlen (argument) == 2) && (argument[0] == '-')) {

		/* update parse mode */
		UPDATE_PARSE_MODE (PARSE_MODE_UNIX);

		return 1;
	}
	
	/* support windows argument format */
	if (CHECK_PARSE_MODE (PARSE_MODE_WINDOWS) && (strlen (argument) > 2) && (argument[0] == '/')) {

		/* now check if the argument doesn't have more bars */
		while (argument[iterator] != 0) {
			/* check value */
			if (argument[iterator] == '/')
				return 0;

			/* update iterator */
			iterator++;
			
		} /* end while */

		/* update parse mode */
		UPDATE_PARSE_MODE (PARSE_MODE_WINDOWS);
		
		return 1;
	}

	return 0;
}

char * exarg_get_argument (char * argument)
{
	/* support getting the argument value for unix standard
	 * argument */
	if (!memcmp (argument, "--", 2))
		return &(argument[2]);
	
	/* support getting the argument value for short def */
	if ((strlen (argument) == 2) && (argument[0] == '-'))
		return &(argument[1]);

	/* support getting the argument value for windows def */
	if ((strlen (argument) > 2) && (argument[0] == '/'))
		return &(argument[1]);

	return NULL;
}

int exarg_check_help_argument (char * argument)
{
	if (!memcmp (argument, "help", 4) || !memcmp (argument, "?", 1))
		return 1;
	return 0;
}

void     exarg_wrap_and_print (const char * text, int size_to_wrap, const char * fill_text)
{
	char    ** stringv      = NULL;
	int        i            = 0;
	int        sum          = 0;
	char     * working_line = NULL;
	char     * aux_string   = NULL;
	int        first_line   = 1;

	/* do not print anything if a null value is received */
	if (text == NULL)
		return;

	if (strlen (text) <= size_to_wrap) {
		printf ("%s\n", text);
		return;
	}

	stringv = exarg_split (text, 1, " ");
	for (i = 0; stringv[i]; i++) {
		if (working_line) {
			aux_string   = working_line;
			working_line = exarg_strdup_printf ("%s %s", working_line, stringv[i]);
			exarg_free (aux_string);
		}else
			working_line = exarg_strdup (stringv[i]);
		sum = sum + strlen (stringv[i]) + 1;

		if (sum >= size_to_wrap) {
			sum = 0;
			if (first_line) {
				first_line = 0;
				printf ("%s\n", working_line);
			}else
				printf ("%s%s\n", fill_text, working_line);
			exarg_free (working_line);
			working_line = NULL;
		}

	}
	if (sum) {
		printf ("%s%s\n", fill_text, working_line);
		exarg_free (working_line);
	}
	exarg_freev (stringv);
	
	
	return;
}

void __exarg_show_usage_foreach (epointer key, epointer value, epointer user_data)
{
	char            ** string_aux = (char **) user_data;
	char             * str        = (* string_aux);
	char             * aux        = NULL;
	ExArgNodeOption  * node       = (ExArgNodeOption  *) value;

	switch (node->type) {
	case EXARG_NONE:
		if (node->arg_short_name)
			(*string_aux) = exarg_strdup_printf (" [-%s|--%s]", node->arg_short_name, node->arg_name);
		else
			(*string_aux) = exarg_strdup_printf (" [--%s]", node->arg_name);
		break;
	case EXARG_INT:
		if (node->arg_short_name)
			(*string_aux) = exarg_strdup_printf (" [-%s <number>|--%s <number>]", node->arg_short_name, node->arg_name);
		else
			(*string_aux) = exarg_strdup_printf (" [--%s <number>]", node->arg_name);
		break;
	case EXARG_STRING:
		if (node->arg_short_name)
			(*string_aux) = exarg_strdup_printf (" [-%s <string>|--%s <string>]", node->arg_short_name, node->arg_name);
		else
			(*string_aux) = exarg_strdup_printf (" [--%s <string>]", node->arg_name);
		break;
	} /* end switch */

	if (str != NULL) {
		/* get a reference to previous content */
		aux           = (* string_aux);
		(*string_aux) = exarg_strdup_printf ("%s%s", str, (*string_aux));
		
		/* dealloc both pieces that joined are the result */
		exarg_free (str);
		exarg_free (aux);
	}
	return;
}

void exarg_show_usage (int show_header)
{
	char            * string_aux = NULL;
	ExArgNodeOption * node;
	
	if (show_header && (__exarg_usage_header && (* __exarg_usage_header)))
		printf (__exarg_usage_header);

	printf ("Usage: %s ", exarg_exec_name);
	
	/* get the first node */
	node = argument_options;
	while (node != NULL) {
		/* call to the foreach function */
		__exarg_show_usage_foreach ((epointer) node->arg_name, node, &string_aux);
		
		/* update to the next */
		node = node->next;
		
	} /* end while */

	exarg_wrap_and_print (string_aux, 55, "               ");
	printf ("\n");

	/* free string created */
	exarg_free (string_aux);

	if (show_header && (__exarg_post_usage_header && (* __exarg_post_usage_header)))
		printf (__exarg_post_usage_header);
	
	return;
}

char * __exarg_build_dep_string (ExArgNodeOption * node, int exclude)
{
	char            * result;
	char            * aux;
	ExArgDependency * dep;

	/* check node dependency */
	if (!exclude && ! node->depends)
		return NULL;
	if (exclude && ! node->excludes)
		return NULL;

	if (exclude) 
		result = exarg_strdup ("[excludes: ");
	else
		result = exarg_strdup ("[depends on: ");
	
	/* foreach each dependency configured */
	if (exclude)
		dep    = node->excludes;
	else
		dep    = node->depends;
	while (dep != NULL) {

		aux    = result;
		if (dep->next != NULL)
			result = exarg_strdup_printf ("%s --%s", result, dep->node->arg_name);
		else
			result = exarg_strdup_printf ("%s --%s]", result, dep->node->arg_name);
		exarg_free (aux);
		
		/* get next dependency */
		dep = dep->next;

	} /* end while */

	/* return string created */
	return result;
}

void __exarg_show_help_foreach (epointer key, epointer value, epointer user_data)
{
	ExArgNodeOption * node = (ExArgNodeOption  *) value;
	char            * dep;
	int               chars = 35;
	int               iterator = 0;
	
	/* print argument help */
	if (node->arg_short_name) {
		printf ("  -%s, --%-18s       ", 
			node->arg_short_name, node->arg_name);
		exarg_wrap_and_print (node->description, 40, "                                 ");

	}else {
		printf ("  --%-22s       ", node->arg_name);
		exarg_wrap_and_print (node->description, 40, "                                 ");
	}

	/* print argument dependency */
	dep = __exarg_build_dep_string (node, 0);
	if (dep != NULL) {
		/* write spaces */
		while (iterator < chars) {
			printf (" ");
			iterator++;
		}
		exarg_wrap_and_print (dep, 40, "                                 ");
	}
	exarg_free (dep);

	/* print argument exclusion */
	iterator = 0;
	dep      = __exarg_build_dep_string (node, 1);
	if (dep != NULL) {
		/* write spaces */
		while (iterator < chars) {
			printf (" ");
			iterator++;
		}
		exarg_wrap_and_print (dep, 40, "                                 ");
	}
	exarg_free (dep);
	return;
}

void   exarg_show_help () 
{
	ExArgNodeOption * node;

	if (__exarg_help_header && (* __exarg_help_header))
		printf (__exarg_help_header);

	exarg_show_usage (0);

	printf ("\nCommand options:\n");

	/* get first node */
	node = argument_options;
	while (node != NULL) {
		/* call to show the node information */
		__exarg_show_help_foreach ((epointer) node->arg_name, node, NULL);

		/* update to the next node */
		node = node->next;
	} /* end while */

	printf ("\nHelp options:\n");
	printf ("  -?, --help                    Show this help message.\n");
	printf ("  --usage                       Display brief usage message.\n");

	if (__exarg_post_help_header && (* __exarg_post_help_header))
		printf (__exarg_post_help_header);

	return;
}

void exarg_check_argument_value (char ** argv, int iterator) 
{

	if (argv[iterator] == NULL) {
		printf ("error: not defined value for argument: %s, exiting..",
			argv[iterator - 1]);
		fflush (stdout);
		exit (-1);
	}

	if (exarg_is_argument (argv[iterator])) {
		printf ("error: not defined value for argument: %s, instead found another argument: %s, exiting..",
			argv[iterator - 1],
			argv[iterator]);
		fflush (stdout);
		exit (-1);
	}

	return;
}


void __exarg_parse_check_non_optional ()
{
	ExArgNodeOption * node = argument_options;

	/* for each node */
	while (node != NULL) {
		/* check the node */
		if (! node->is_optional && ! node->is_defined) {
			exarg_msg ("error: argument '%s' is not defined, but it is required for a proper function..\n",
				   node->arg_name);
		} /* end if */

		/* get the next */
		node = node->next;

	} /* end while */

	return;
}

void __exarg_parse_check_depends ()
{
	/* first argument */
	ExArgNodeOption * node = argument_options;
	ExArgDependency * dep;

	/* for each argument defined */
	while (node != NULL) {

		/* check if the argument was defined, and hence
		 * activates its dependencies */
		if (! node->is_defined) {
			node = node->next;
			continue;
		}

		/* check depends */
		dep = node->depends;
		while (dep != NULL) {

			/* check that the dependency is defined */
			if (! dep->node->is_defined) {
				exarg_msg ("You must define argument (--%s) if used (--%s). Try %s --help.", 
					   dep->node->arg_name, node->arg_name, exarg_exec_name);
				return;
			} /* end if */

			/* get next dependency */
			dep = dep->next;
			
		} /* end while */

		/* check depends */
		dep = node->excludes;
		while (dep != NULL) {
			
			/* check that the dependency is defined */
			if (dep->node->is_defined) {
				printf ("You can't define argument (--%s) if used (--%s). Try %s --help.\n", 
					node->arg_name, dep->node->arg_name, exarg_exec_name);
				fflush (stdout);
				return;
			} /* end if */

			/* get next dependency */
			dep = dep->next;
			
		} /* end while */

		/* get next node */
		node = node->next;
 
	} /* end whle */

	/* nothing more to check */
	return;
}


/** 
 * \brief Makes exarg to start parsing argument options.
 *
 * Makes LibExArg to start command line parsing by using arguments
 * installed. 
 * 
 * Once this functions is called it is posible to call the set of
 * function which returns data obtained: \ref exarg_is_defined,
 * \ref exarg_get_int, \ref exarg_get_string and \ref exarg_get_params.
 *
 * The advantage is that LibExArg allows you to get access to command
 * line data from any point of your program.
 * 
 * @param argc the argc value the current application have received.
 * @param argv the argv value the current application have received.
 */
void       exarg_parse         (int         argc,
				char     ** argv)
{
	int              iterator = 1;
	ExArgNodeOption * node;
	char           * argument;

	/* check how many argument have been defined */
	if (iterator == argc) {
		/* once terminated, check if the non optional options
		 * were defined */
		
		__exarg_parse_check_non_optional ();
		return;
	}

	exarg_exec_name = exarg_basename (argv[0]);
	
	/* iterate over all arguments */
	while (iterator < argc) {

		if (!exarg_is_argument (argv[iterator])) {
			/* check free argument configuration */
			if (__exarg_disable_free_args) {
				exarg_msg ("error: provided a free argument='%s', not listed in the accept command line option", argv[iterator]);

			} /* end if */

			/* save and increase iterator */
			__exarg_add_argument (argv[iterator]);
			iterator++;
			continue;
		}
		
		/* get clean argument */
		argument = exarg_get_argument (argv[iterator]);

		/* check for help argument */
		if (exarg_check_help_argument (argument)){
			exarg_show_help ();
			exarg_end ();
			exit (0);
		}
		
		/* check for usage argument */
		if (!memcmp (argument, "usage", 5)) {
			exarg_show_usage (1);
			exarg_end ();
			exit (0);
		}

		/* get node argument */
		node = exarg_lookup_node (argument);
		if (node == NULL) {
			exarg_msg ("%s error: argument not found: %s, try: %s --help\n",
				   exarg_exec_name, argv[iterator], exarg_exec_name);
		}
		
		/* check if node is defined */
		if (node->is_defined) {
			exarg_msg ("%s error: argument %s already defined, exiting..\n",
				   exarg_exec_name, argv[iterator]);
		}

		
		/* set this argument to be defined */
		node->is_defined = 1;
		switch (node->type) {
		case EXARG_NONE:
			/* nothing to do */ 
				break;
		case EXARG_INT:
			/* save int value */
			iterator++;

			exarg_check_argument_value (argv, iterator);

			node->int_value    = atoi (argv[iterator]);
			break;
		case EXARG_STRING:
			iterator++;
			
			exarg_check_argument_value (argv, iterator);

			node->string_value = argv[iterator];

			break;
		}
		
		/* update iterator */
		iterator++;
	}

	/* once terminated, check if the non optional options were
	 * defined */
	__exarg_parse_check_non_optional ();

	/* check argument dependency and argument mutual exclusion. */
	__exarg_parse_check_depends ();

	return;
}

void __exarg_end_free_dep (ExArgDependency * dep)
{
	ExArgDependency * next;

	/* foreach dependency established */
	while (dep != NULL) {

		/* free the dep node */
		next = dep->next;
		exarg_free (dep);
		dep  = next;
	} /* end while */

	return;
}

/**
 * \brief Ends exarg library execution.
 * 
 * Terminates the exarg function. This function will free all
 * resources allocated so the library cannot be used any more for the
 * current execution.
 *
 * This function is reatrant. Several threads can actually call this
 * function. It will take care about making only one thread to
 * actually free resources.
 **/
void       exarg_end           ()
{
	ExArgNodeOption * node;
	ExArgNodeOption * node2;
	
	ExArgument      * params2;
	

	if (argument_options != NULL) {
		/* get a reference to the table, and dealloc it */
		node             = argument_options;
		argument_options = NULL;
		
		while (node != NULL) {
			/* get a reference to the next */
			node2 = node->next;

			/* free node arguments dependencies and mutual
			 * exclusions */
			__exarg_end_free_dep (node->depends);
			__exarg_end_free_dep (node->excludes);

			/* free current node */
			exarg_free (node);
			
			/* update the node ref */
			node = node2;
		} /* end while */

	} /* end if */

	if (params != NULL) {
		while (params != NULL) {
			/* get a reference to the next node */
			params2 = params->next;

			/* free params node */
			exarg_free (params);

			/* configure the next argument to dealloc */
			params = params2;
		} /* end while */
	} /* end if */

	return;
}

/**
 * \brief Disable autohelp.
 * 
 * Allows to disable auto help and --help and -? option
 * recognition. This is useful if you don't want your program to
 * accept --help and --usage options.
 **/
void       exarg_disable_help ()
{
	return;
}

/**
 * \brief Adds user defined header to automatic usage command generated.
 * 
 * Once a program is linked to libexarg is able to produce a usage
 * help by accepting the --usage command line option. But that usage
 * help is showed as is. This function allows you to define a header
 * to be showed preceding the usage info. You can use this, for
 * example, to introduce your copyright.
 *
 * This function doesn't make a copy of the given string so you
 * must not free the header provided or use an static string.
 **/
void       exarg_add_usage_header (char * header)
{
	__exarg_usage_header = header;
}

/**
 * \brief Adds user defined header to automatic help command generated.
 * 
 * Once a program is linked to libexarg is able to produce a help by
 * accepting the --help or -? command line option. But that help is
 * showed as is. This function allows you to define a header to be
 * showed preceding the help info. You can use this, for example, to
 * introduce your copyright.
 *
 * This function doesn't make a copy of the given string so you
 * must not free the header provided or use an static string.
 **/
void       exarg_add_help_header  (char * header)
{
	__exarg_help_header = header;
}

/**
 * \brief Adds user defined post header for usage command.
 * 
 * This function works pretty much like \ref exarg_add_usage_header but
 * adding the header provided to be appended at the end of the usage
 * automatically generated.
 **/
void       exarg_post_usage_header (char * post_header)
{
	__exarg_post_usage_header = post_header;
}

/**
 * \brief Adds user defined post header for help command.
 * 
 * This function works pretty much like \ref exarg_add_help_header but
 * adding the header provided to be appended at the end of the help
 * automatically generated.
 **/
void       exarg_post_help_header (char * post_header)
{
	__exarg_post_help_header = post_header;
}

/**
 * \internal
 * Internal ExArg function. This function allows \ref exarg_install_arg to
 * check if there are already installed an short argument as the one
 * been installed.
 **/
void __exarg_check_short_arg (epointer key, epointer value, epointer user_data)
{
	ExArgNodeOption * node         = (ExArgNodeOption  *) value;
	char           * arg_to_check = (char *) user_data;

	if (node->arg_short_name != NULL) {
		if (!memcmp (node->arg_short_name, arg_to_check, strlen (arg_to_check))) {
			exarg_msg ("error: found that short arg installed is already found: %s\n", arg_to_check);
		} /* end if */
	} /* end if */

	return;
}

/**
 * \brief Installs a new command line to be accepted.
 * 
 * This functions allows you to install new arguments to be
 * accepted. Every argument passed in to the program which is not
 * installed throught this function will no be accepted and will
 * generate a non recognized command line error.
 *
 * Let's see an example on how to use exarg to install the --version
 * argument. You should call exarg as follows:
 * 
 * \code
 *       exarg_install_arg ("version", "v", EXARG_NONE, 
 *                          "show program version");
 *
 * \endcode
 * 
 * Previous line have installed a argument called "version" which will
 * be invoked by the user as --version. The short alias
 * in this case is "v" which as we have see will allow user to invoke
 * your program as -v.
 *
 * Because you could install arguments which may conflict, this
 * function will abort the program execution on that case. This will
 * ensure you, as programer, to have a binary compiled with no
 * problems due to exarg.
 *
 * Later on, you can use \ref exarg_is_defined to check the status of
 * installed arguments. Once you install an argument the user may
 * invoke it and you could check that status using previous
 * function. Because the "version" argument type is \ref EXARG_NONE you can
 * only use \ref exarg_is_defined. But, for other argument types as
 * \ref EXARG_STRING, you can also use \ref exarg_get_string function to get the
 * value defined by user. Let's see an example.
 *
 * \code
 *       exarg_install_arg ("load-library", "l", EXARG_STRING,
 *                          "loads the library defined as argument");
 * \endcode
 * 
 * This previous line will allow user to invoke your program as 
 * --load-library path/to/lib. Then you can use \ref exarg_get_string to get
 * the path/to/lib value by using:
 * 
 * \code
 *       // check user have defined this argument
 *       if (exarg_is_defined ("load-library")) {
 *
 *             arg_value = exarg_get_string ("load-library");
 *
 *             printf ("The library defined by user was: %s\n", 
 *                      arg_value);
 *       }
 * 
 * \endcode
 *
 * You must install all argument before calling \ref exarg_parse. That function
 * will parse argument options by using installed arguments. Check the info
 * about that function.
 * 
 * ExArg is not thread-safe which doesn't means anything wrong but you
 * must call all exarg function from the same thread to get the right
 * results. 
 *
 * This function will not do a copy from arg_name, arg_short_name or
 * description. This means you should not free that values while
 * using exarg. To end exarg using check \ref exarg_end. It is recomended to use
 * static values and shows on previous examples.
 **/
void       exarg_install_arg  (const char     * arg_name, 
			       const char     * arg_short_name, 
			       ExArgType   type,
			       const char     * description)
{
	ExArgNodeOption * node;
	ExArgNodeOption * node2;

	/* init hash table */
	if (argument_options != NULL) {
		/* check if there are an argument with the same name */
		if (exarg_lookup_node (arg_name)) {
			exarg_end ();
			exarg_msg ("error: argument being installed is already defined: %s..",
				   arg_name);
		}
		
		/* check if there are an shor argument with the same
		 * name */
		if (arg_short_name != NULL) {
			node = argument_options;
			
			/* while there are options to process */
			while (node != NULL) {
				/* call to check */
				__exarg_check_short_arg ((epointer) node->arg_name, node, (epointer) arg_short_name);

				/* update to the next */
				node = node->next;
			} /* end while */
		} /* end if */
	} /* end if */
		
	/* create node option */
	node                 = exarg_new (ExArgNodeOption, 1);
	node->arg_name       = arg_name;
	node->arg_short_name = arg_short_name;
	node->type           = type;
	node->description    = description;
	node->is_optional    = 1;

	/* lookup for the last position */
	if (argument_options == NULL)
		argument_options = node;
	else {
		/* lookup for the last */
		node2 = argument_options;
		while (node2->next != NULL)
			node2 = node2->next;

		/* set it */
		node2->next = node;
	} /* end if */

	return;
}

/**
 * \brief Installs several command lines to be accepted.
 * 
 * This function does the same that \ref exarg_install_arg but making 
 * group of argument to be installed in one step.
 *
 * Think about installing two argument. To install them we have to do
 * the following:
 *
 * \code
 *    exarg_install_arg ("version", "v", 
 *                       EXARG_NONE, "show argument version");
 *    exarg_install_arg ("load-library", "l", 
 *                       EXARG_STRING, "load my library");
 * \endcode
 *
 * Because some people doesn't like to perform several calls to the
 * same funcion the previous code can be done by using one step as:
 *
 * \code
 *    exarg_install_argv (2, "version", "v", EXARG_NONE, 
 *                        "show argument version", 
 *                        "load-library", "l",
 *                        EXARG_STRING, "load my library");
 * \endcode
 *
 * When you install argument this way you have to specify how many
 * argument is going to be installed. In this case that number is
 * 2. This allows exarg to know how many arguments needs to search
 * inside the argv. If something is wrong while specifying the number
 * or argument or the argument information itself you'll get a
 * segmentation fault or something similar.
 *
 * While calling to \ref exarg_install_arg the short_name argument can be
 * optional. This is *NOT* applied to this function. If you don't want
 * to define a short argument name you must use NULL as value.
 * 
 * @param num_arg Must be at least 1 or error will happen and library will abort
 * application execution.
 **/
void       exarg_install_argv (int num_arg, ...)
{
	va_list      args;
	int         iterator;
	char      * arg_name;
	char      * arg_short_name;
	ExArgType    type;
	char      * description;

	if (num_arg <= 0) {
		exarg_end ();
		exarg_msg ("error: calling to exarg_install_argv with num_arg equal or less to 0..");
	}

	va_start (args, num_arg);

	for (iterator = 0; iterator < num_arg; iterator++) {

		/* get the argument info */
		arg_name       = va_arg (args, char *);
		arg_short_name = va_arg (args, char *);
		type           = va_arg (args, ExArgType);
		description    = va_arg (args, char *);
		
		exarg_install_arg (arg_name, arg_short_name, type, description);
	}
	va_end (args);
	return;
}

/** 
 * @brief Allows to define a parameter dependency between the to
 * arguments defined. The kind of dependency has direction. This means
 * that the first argument will depend on the second argument, forcing
 * the user to define the second argument if the first one is defined.
 *
 * You can call to this function several times making the first
 * argument to depend on any number of argument defined. You must not
 * call to establish a dependency to the argument itself.
 * 
 * @param arg_name The argument that will be configured with an
 * argument dependency.
 *
 * @param arg_dependency The argument that will receive the
 * dependency.
 *
 *
 */
void         exarg_add_dependency   (const char * arg_name, 
				     const char * arg_dependency)
{
	ExArgNodeOption * node;
	ExArgNodeOption * node2;
	ExArgDependency * dep;
	/* check arguments received */
	if (arg_name == NULL)
		return;
	if (arg_dependency == NULL)
		return;

	/* check that both arguments aren't equal */
	if (strlen (arg_name) == strlen (arg_dependency) &&
	    !memcmp (arg_name, arg_dependency, strlen (arg_name))) {
		exarg_msg ("error: defined argument dependency with an argument itself.");
		return;
	}
	
	/* locates the argument node */
	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		exarg_msg ("error: you did especify an argument that doesn't exists (%s)", arg_name);
		return;
	}

	/* locates dependecy argument node */
	node2 = exarg_lookup_node (arg_dependency);
	if (node2 == NULL) {
		exarg_msg ("error: you did especify an argument that doesn't exists (%s)", arg_dependency);
		return;
	}

	/* make the first argument to depend on the second */
	dep = node->depends;
	
	/* create the new dependency node */
	node->depends       = exarg_new (ExArgDependency, 1);
	node->depends->node = node2;
	node->depends->next = dep;

	return;
}

/** 
 * @brief Allows to configure arguments that are mutually
 * excluyents. This function will take the first arguments to be
 * muatually excluyen with the second one without direction as it
 * happens with \ref exarg_add_dependency function.
 *
 * Once defined both arguments provided can't be defined at the same
 * time at the command line options.
 * 
 * @param arg_name The first argument to make mutual exclusion with
 * the second argument.
 *
 * @param arg_excluded Second argument to make mutual exclusion with
 * the first argument.
 */
void         exarg_add_exclusion     (const char * arg_name, 
				      const char * arg_excluded)
{
	ExArgNodeOption * node;
	ExArgNodeOption * node2;
	ExArgDependency * dep;

	/* check arguments received */
	if (arg_name == NULL)
		return;
	
	if (arg_excluded == NULL)
		return;

	/* check that both arguments aren't equal */
	if (strlen (arg_name) == strlen (arg_excluded) &&
	    !memcmp (arg_name, arg_excluded, strlen (arg_name))) {
		exarg_msg ("error: defined argument dependency with an argument itself.");
		return;
	}
	
	/* locates the argument node */
	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		exarg_msg ("error: you did especify an argument that doesn't exists (%s)", arg_name);
		return;
	}

	/* locates dependecy argument node */
	node2 = exarg_lookup_node (arg_excluded);
	if (node2 == NULL) {
		exarg_msg ("error: you did especify an argument that doesn't exists (%s)", arg_excluded);
		return;
	}

	/* make the first argument to depend on the second */
	dep = node->excludes;
	
	/* create the new dependency node */
	node->excludes       = exarg_new (ExArgDependency, 1);
	node->excludes->node = node2;
	node->excludes->next = dep;

	return;
}


/** 
 * @brief Makes an argument installed to be obligatory (not optional
 * at the user command line input).
 *
 * Once called \ref exarg_install_arg, you can use this function to
 * make the program to make the option obligatory.
 * 
 * @param arg_name The argument to make it obligatory.
 */
void       exarg_set_obligatory   (char * arg_name)
{
	ExArgNodeOption * node;

	/* perform some environment checks */
	if (arg_name == NULL)
		return;

	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		return;
	}

	/* make it to be defined */
	node->is_optional = 0;

	return;
}


/** 
 * @brief Allows to configure exarg library to accept or not free
 * arguments. 
 * 
 * Free arguments are optional parameters provided to the application,
 * such files, which aren't associated to a particular option.
 *
 * If your command line application do not uses free arguments, you
 * can use this function to enable exarg library to show an error
 * message to the user:
 *
 * \code
 * // disable free arguments
 * exarg_accept_free_args (0);
 * \endcode
 * 
 * @param accept 1 to accept free arguments. It is the default value,
 * so it is not required to enable it. 0 to disable free arguments.
 */
void         exarg_accept_free_args (int accept)
{
	/* configure free arguments */
	__exarg_disable_free_args = (accept == 0);

	return;
}

/** 
 * @brief Allows to simulate user defined command line options alread
 * installed by \ref exarg_install_arg, without requiring the user to
 * set those values.
 *
 * This function allows to install values received or to just define
 * the argument to be supported by the program, without requiring the
 * user to provide such option. This is a convenient to make some
 * options to be default, writting your application relying on
 * arguments defined by command line.
 *
 * The function won't define the argument if not installed
 * previously. If the argument isn't found, the function takes no
 * action.
 *
 * @param arg_name The argument o define as provided by the user.
 * @param value The value to be associated to the argument. Some
 * arguments doesn't require this paremeters (\ref EXARG_NONE), so,
 * you can provide NULL to this parameter.
 */
void       exarg_define           (char * arg_name,
				   char * value)
{
	ExArgNodeOption * node;

	/* perform some environment checks */
	if (arg_name == NULL)
		return;

	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		return;
	}
	/* make it to be defined */
	node->is_defined = 1;

	/* check argument value */
	if (value != NULL) {
		switch (node->type) {
		case EXARG_NONE:
			/* nothing to set */
			break;
		case EXARG_INT:
			/* integer value */
			node->int_value = strtol (value, NULL, 10);
			break;
		case EXARG_STRING:
			/* string value */
			node->string_value = value;
			break;
		}
	} /* end if */

	/* nothing to do over here */
	return;
}

/**
 * \brief Allows to check if a user have defined a command.
 * 
 * Once an argument is installed the user may or may not define it. To
 * define it simply means to use this argument as command line
 * option. This function allows to check if an argument was used for
 * the current command line option.
 *
 * This function is expecting to receive the argument name to lookup
 * not the short name. User may be using the short name to invoke the
 * argument but the lookup is allways done by using the argument name.
 * 
 * \return TRUE if argument was defined or FALSE if not.
 **/
int   exarg_is_defined   (char     * arg_name)
{
	ExArgNodeOption * node;

	/* perform some environment checks */
	if (arg_name == NULL)
		return 0;
	
	if (argument_options == NULL)
		return 0;

	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		return 0;
	}
	return node->is_defined;
}

/** 
 * @brief Allows to check several values to be defined at the same time.
 *
 * This allows to check if several command line options have been
 * defined at the same time. 
 * Example:
 * \code
 *   if (exarg_is_definedv ("param1", "param2", NULL)) {
 *         //param1 and param2 have been defined by the user
 *   }
 * \endcode
 *
 * This is a short way for actually doing:
 * \code
 *  if (exarg_is_defined ("param1") && exarg_is_defined ("param2")) {
 *      // param1 and para2 have been defined by the user
 *  }
 * \endcode
 * 
 * Do not forget to pass a NULL value for the last item.
 * 
 * @return TRUE if all values are defined, otherwise FALSE.
 */
int   exarg_is_definedv      (char * first_value, ...)
{
	int       result = 1;
	char    * string_value;
	va_list    args;

	if (first_value == NULL)
		return 0;

	/* check first value */
	if (!exarg_is_defined (first_value))
		return 0;

	/* open stdargs */
	va_start        (args, first_value);
	string_value = va_arg (args, char *);

	/* check for last NULL value */
	while ((string_value != NULL) && (* string_value)) {
		/* check next value */
		result       = result && exarg_is_defined (string_value);

		/* get next value */
		string_value = va_arg (args, char *);
	}

	/* return current value */
	va_end (args);
	return result;
}

/**
 * \brief Allows to get defined string for a given command line.
 * 
 * Returns the value associated with the argument name. The returned
 * value must not be deallocated. If it is needed a string copy use
 * \ref exarg_get_string_alloc.
 * 
 * \return The value associated with the param or NULL if fail.
 **/
char    * exarg_get_string   (char     * arg_name)
{
	ExArgNodeOption * node;

	if (arg_name == NULL)
		return NULL;
	
	if (argument_options == NULL)
		return NULL;

	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		exarg_msg ("error: calling to get string value for: %s, but this argument isn't defined..",
			   arg_name);
	}
	if (node->type != EXARG_STRING) {
		exarg_msg ("error: calling to get string value for: %s, but the argument wasn't defined as EXARG_STRING..",
			   arg_name);
	}
	
	return node->string_value;
}

/**
 * \brief Allows to get defined string for a given command line allocating the result.
 * 
 * Returns the value associated with the argument name. The returned
 * value must be <b>deallocated</b> using g_free. You can also use
 * \ref exarg_get_string to get a value that doesn't need to be
 * unrefered.
 * 
 * \return The value associated with the param or NULL if fail.
 **/
char    * exarg_get_string_alloc (char * arg_name)
{
	char * string_value;

	string_value = exarg_get_string (arg_name);
	if (string_value == NULL)
		return NULL;
	return exarg_strdup (string_value);
}

/**
 * \brief Allows to get int value for a given command line argument.
 * 
 * Returns the value associated with the argument name. The returned
 * may be 0 which means the user may not defined that argument or may
 * defined it by using the 0 as value. To avoid confusing cases you
 * should use \ref exarg_is_defined to know if the user have used that
 * argument and then call this function.
 * 
 * \return the value defined for this argument. If the argument
 * is null or the exarg is not initialized the function will return -1.
 **/
int       exarg_get_int      (char     * arg_name)
{
	ExArgNodeOption * node;

	if (arg_name == NULL)
		return -1;
	if (argument_options == NULL)
		return -1;

	node = exarg_lookup_node (arg_name);
	if (node == NULL) {
		exarg_msg ("error: calling to get int value for: %s, but this argument isn't defined..",
			   arg_name);
	}

	if (node->type != EXARG_INT) {
		exarg_msg ("error: calling to get int value for: %s, but the argument wasn't defined as EXARG_INT..",
			   arg_name);
	}
	
	return node->int_value;
}

/**
 * \brief Returns free params defined at command line.
 * 
 * Every value which is not an argument is considered to be a
 * parameter. All of them are stored and retrieved by using this
 * function and the following functions:
 *
 *  - \ref exarg_param_get 
 *  - \ref exarg_param_next
 *
 * An example about using this function could be the next. A program
 * have defined several arguments using the function \ref
 * exarg_install_arg but that program also needs to receive several
 * file path. 
 * 
 * Once the \ref exarg_parse have detected all argument, all
 * parameters are stored, and this function provides access to the
 * first argument *found. Then a call to \ref exarg_param_get is
 * required to get the *argument value and a call to \ref
 * exarg_param_next to get a *reference to the next argument.
 * 
 *
 * A program which may receive the argument --save-all but also several
 * files:
 *
 * \code
 *    a_program --save-all file1 file2 file3
 * \endcode
 *
 * Will need to call this function to get the list: file1 file2 and
 * file3. 
 * 
 * \return The first param provided to the program. The returned value
 * must not be deallocated.
 */
ExArgument   * exarg_get_params   ()
{
	return params;
}

/** 
 * @brief Allows to get the string value that is represeting the argument received.
 * 
 * @param arg The argument that was received by the program.
 * 
 * @return An string reference that must not be deallocated.
 */
const char * exarg_param_get        (ExArgument * arg)
{
	/* check for null value received */
	if (arg == NULL)
		return NULL;
	
	/* return the string value inside */
	return arg->string;
}

/** 
 * @brief Allows to get the next parameters defined at the command
 * line option that is following the argument provided.
 * 
 * @param arg The argument which is previous to the argument to be
 * returned.
 * 
 * @return An argument reference to the next or NULL if there are no
 * more arguments.
 */
ExArgument * exarg_param_next       (ExArgument * arg)
{
	/* check for null value received */
	if (arg == NULL)
		return NULL;
	
	/* return the next argument */
	return arg->next;
}


/** 
 * @brief Allows to get the number of parameters that were defined by
 * the user. 
 *
 * Check also the following function \ref exarg_get_params_num.
 * 
 * @return The number of parameters starting from 0.
 */
int       exarg_get_params_num   ()
{
	ExArgument * arg    = NULL;
	int          result = 0;

	if (params == NULL)
		return 0;

	/* count the number of params */
	arg = params;
	while (arg != NULL) {
		/* update the count */
		result++;

		/* update to the next */
		arg = arg->next;

	} /* end while */

	/* return current count */
	return result;
}

/* @} */


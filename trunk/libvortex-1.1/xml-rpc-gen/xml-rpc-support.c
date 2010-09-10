/* Hey emacs, show me this like 'c': -*- c -*-

 * xml-rpc-gen: a protocol compiler for the XDL definition language
 * Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include <xml-rpc.h>

axlList      * __xml_rpc_support_search_path    = NULL;
FILE         * opened_file                      = NULL;
char         * opened_file_name                 = NULL;
char         * original_user_file_to_merge      = NULL;
int            xml_rpc_support_num_tabs         = 0;

/** 
 * @internal
 *
 * Allows to create a directory providing a path that is build using a
 * printf-like format, and creating all directories through the path
 * provided.
 * 
 * @param format A printf-like format especifying the directory to be created.
 * 
 * @return A newly allocated string returning the directory
 * created. The result must be unrefered using axl_free.
 */
char  * xml_rpc_support_create_dir      (char  * format, ...)
{
	char    * result_dir       = NULL;
	char   ** dir_elements     = NULL;
	char    * actual_dir_name  = NULL;
	char    * string_ptr       = NULL;
	int       iterator         = 0;
	int       build_iterator   = 0;
	va_list   args;
	
	/* open the std args */
	va_start (args, format);
	
	/* create the result dir */
	result_dir = axl_strdup_printfv (format, args);

	/* close the std args */
	va_end (args);

	/* for each element inside the path, create the directory */
	dir_elements    = axl_stream_split (result_dir, 1, "/");
	iterator        = 0;


	/* for each piece inside the blocks found for the directory
	 * path */
	while (dir_elements[iterator] != NULL) {
		
		for (build_iterator = iterator; build_iterator <= iterator; build_iterator++) {
			string_ptr = actual_dir_name;
			if (actual_dir_name) {
				/* build a new directory path from
				 * previous pieces */
				actual_dir_name = axl_strdup_printf ("%s/%s", actual_dir_name, 
								     dir_elements[build_iterator]);
				axl_free (string_ptr);
			}else {
				/* actual directory name, the first
				 * one piece created */
				actual_dir_name = axl_strdup_printf ("%s", dir_elements[0]);
			}
		}
		
		iterator++;
		
		/* check for empty values */
		if (strlen (actual_dir_name) == 0)
			continue;
		
		if (!vortex_support_file_test (actual_dir_name, FILE_IS_DIR)) {
#if defined(AXL_OS_UNIX)
			if (mkdir (actual_dir_name, 0770) < 0) {
#elif defined(AXL_OS_WIN32)
			if (_mkdir (actual_dir_name) < 0) {
#endif
				xml_rpc_support_error ("unable to create '%s' directory\n", axl_true, actual_dir_name);
				return NULL;
			}
		}
	}

	/* free path elements */
	axl_stream_freev (dir_elements);

	/* return current string */
	return result_dir;
}

/** 
 * @internal
 *
 * Allows to open a file, that is located at the path provided for the
 * printf-like format.
 * 
 * @param format A path that identifies the file to be opened.
 */
void    xml_rpc_support_open_file       (char  * format, ...)
{
	va_list   args;
	va_start (args, format);
	original_user_file_to_merge = axl_strdup_printfv (format, args);
	va_end (args);

	/* because we are using a backslash representation to open
	 * files we need to do some foo to be run-time portable with
	 * windows platforms.  original_user_file_to_merge =
	 * af_gen_support_get_native_file_name
	 * (original_user_file_to_merge); */
	opened_file_name = axl_strdup_printf ("%s.xml-rpc-gen-version", original_user_file_to_merge);

	if (opened_file) {
		fclose (opened_file);
		opened_file = NULL;
	}

	opened_file = fopen (opened_file_name, "w");
	if (!opened_file) {
		xml_rpc_support_error ("unable to create %s file\n", axl_true, opened_file_name);
		return;
	} /* end if */
	
	/* reset automatic tab indentation */
	xml_rpc_support_num_tabs = 0;

	return;
}

/** 
 * @internal
 *
 * Generates an error message and then exist according to the values provided.
 * 
 * @param message The message to report.
 *
 * @param must_exit A variable that controls if the function must
 * perform a call to the exit or not.
 */
void    xml_rpc_support_error           (char  * message, axl_bool      must_exit, ...)
{
	char     * result;
	va_list    args;

	/* open the stdarg to get the message to print, and close
	 * it */
	va_start (args, must_exit);
	result = axl_strdup_printfv (message, args);
	va_end (args);

	printf ("error: %s\n", result);

	/* release memory used */
	axl_free (result);

	/* check if a exit is required */
	if (must_exit)
		exit (-1);

	return;
}

/** 
 * @internal
 *
 * Moves a file from the detination provided to the destination provided.
 * 
 * @param from The source destination.
 * @param to The file destination.
 */
void   xml_rpc_support_move_file (char  * from, char  * to)
{

#if defined(AXL_OS_WIN32)
	/* first remove the file to be overwrited. On unix platforms
	 * this is not necesary because move (rename) already
	 * overwrite destination file. On windows platform is not
	 * posible to rename a file with a file on the destination. */
	if (vortex_support_file_test (to, FILE_IS_REGULAR) && (unlink (to) != 0)) {
		xml_rpc_support_error ("unable to remove destination file: %s to be overwrited by: %s at rename operation",
				       axl_false, to, from);
		return;
	}
#endif

	/* now move the file */
	if (rename (from, to) != 0) {
		xml_rpc_support_error ("unable to move file: %s to %s", axl_false, from, to);
	}
	return;
}

/** 
 * @brief Allows to know if two files are equal.
 * 
 * @param file1 First file to check.
 * @param file2 Second file to check.
 * 
 * @return axl_true if both files are equal (false if not).
 */
axl_bool  xml_rpc_support_are_equal (char * file1 , char * file2)
{
	int fd1;
	int fd2;
	char buffer1[1];
	char buffer2[1];
	int  read1;
	int  read2;

	/* check that both files exists to ensure that the function
	 * returns axl_true for files that exists. */
	if (! vortex_support_file_test (file1, FILE_EXISTS))
		return axl_false;

	if (! vortex_support_file_test (file2, FILE_EXISTS))
		return axl_false;

	/* open both files */
	fd1 = open (file1, O_RDONLY);
	if (fd1 < 0) {
		return axl_false;
	}

	fd2 = open (file2, O_RDONLY);
	if (fd2 < 0) {
		close (fd1);
		return axl_false;
	}
	
	read1 = read (fd1, buffer1, 1);
	read2 = read (fd2, buffer2, 1);

	/* while both contents are equal */
	while ((read1 == read2) && buffer1[0] == buffer2[0] && read1 != 0 ) {
		
		/* read the next */
		read1 = read (fd1, buffer1, 1);
		read2 = read (fd2, buffer2, 1);
		
	} /* end while */

	close (fd1);
	close (fd2);

	
	
	return (read1 == read2) && (read1 == 0);
}



char * next_line (FILE * file)
{
	long   current;
	int    desp = 0;
	int    size;
	char   value;
	char * result;
	char   temp[1024];

	/* check end of file */
	if (feof (file) != 0) {
		printf ("end of file reached..\n");
		return NULL;
	}

	/* check file error */
	if (ferror (file) != 0) {
		printf ("error found....\n");
		return NULL;
	}

	/* get current file position */
	if (file != stdin) {
		current = ftell (file);
		
		/* get the desp */
		while (fread (&value, 1, 1, file) == 1 && value != '\n')
			desp++;
		
		if (ferror (file) != 0) {
			printf ("error found..\n");
			return NULL;
		}

		/* reconfigure descriptor */
		if (fseek (file, current, SEEK_SET) != 0) {
#if defined(AXL_OS_UNIX)
			printf ("failed to reconfigure current file stream: %ld, error=%s..\n", current, strerror (errno));
#elif defined(AXL_OS_WIN32)
			printf ("failed to reconfigure current file stream: %ld\n", current);
#endif
			return NULL;
		}
		
		/* allocate enough memory */
		result = axl_new (char, desp + 1);
		
		/* get the desp */
		if (fread (result, 1, desp, file) != desp) {
			printf ("failed to read data..\n");
			axl_free (result);
			return NULL;
		} /* end if */


		if (feof (file) == 0) {
			/* consume the latest \n */
			size = fread (&value, 1, 1, file);
		} /* end if */
	} else {
		memset (temp, 0, 1024);
		
		/* get the desp */
		while (desp < 1024 && fread (temp + desp, 1, 1, file) == 1 && temp[desp] != '\n')
			desp++;

		/* allocate enough memory */
		result = axl_new (char, desp + 1);
		
		/* copy content */
		memcpy (result, temp, desp);
		
	} /* end if */
	
	return result;
}


/** 
 * @internal
 *
 * Close the current opened file.
 */
void    xml_rpc_support_close_file      (void)
{
	char    * path  = original_user_file_to_merge;
	char    * reply = NULL;

	/* close the file opened */
	fclose (opened_file);
	opened_file = NULL;

	/* check force option */
	if (exarg_is_defined ("force"))
		goto write_file;

	if (vortex_support_file_test (path, FILE_EXISTS)) {
		/* check if both files differs */
		if (xml_rpc_support_are_equal (original_user_file_to_merge, opened_file_name)) {
			ok_msg ("skiping file not modified: %s", original_user_file_to_merge);
			unlink (opened_file_name);
			goto finish_close_file;
		}

		/* ask the user to overwrite this file */
	get_option:
		ask_msg ("\n");
		ask_msg ("file already exists, and differs: %s\n", path);
		ask_msg ("Do you want me to: Write (w), Skip (s): ");
		reply = next_line (stdin);
		if (axl_cmp (reply, "w")) {
			goto write_file;
		} else if (axl_cmp (reply, "s")) {
			ok_msg ("skiping creating: %s", path);
		} else {
			/* option not supported */
			error_msg ("option not supported, try again: %s", reply);
			axl_free (reply);
			goto get_option;
		} /* end if */
		
		/* flag ok result */
		axl_free (reply);
	} else {
		/* write the file case */
	write_file:
		ok_msg ("creating file:             %s", path);

		/* move the file to the final destination */
		xml_rpc_support_move_file (opened_file_name, original_user_file_to_merge);
	} /* end if */
	
 finish_close_file:

	/* free original file to merge: currently not supported */
	axl_free (original_user_file_to_merge);
	original_user_file_to_merge = NULL;

	/* free file name value */
	axl_free (opened_file_name);
	opened_file_name = NULL;
	
	return;
}	


/** 
 * @internal
 *
 * Push current indent increasing the tab size.
 */
void    xml_rpc_support_push_indent     (void)
{
	xml_rpc_support_num_tabs++;
}

/** 
 * @internal
 * 
 * Pop current indent decreasing the tab size.
 */
void    xml_rpc_support_pop_indent      (void)
{
	xml_rpc_support_num_tabs--;
}

/** 
 * @internal
 *
 * Writes the provided content, using a printf-like format, to the
 * current opened file.
 * 
 * @param format 
 */
void xml_rpc_support_write (const char  * format, ...) {
	
	va_list     args;
	int       i;

	/* write tabular according to current indent configuration */
	if (xml_rpc_support_num_tabs > 0) {
		for (i = 0; i < xml_rpc_support_num_tabs; i++) {
			fprintf (opened_file, "\t");
		}
	}

	/* opend the std arg content to write it to the file */
	va_start (args, format);
	vfprintf (opened_file, format, args);
	va_end (args);
	
	/* done */
	return;
}

#define xml_rpc_support_mwrite xml_rpc_support_multiple_write

/** 
 * @internal
 *
 * Allow to write several lines at the same time using one call.
 *
 * Here is an example:
 * \code
 *    xml_rpc_support_multiple_write ("The first line..",
 *                                    "Second line...",
 *                                    "Third line...",
 *                                    NULL);
 * \endcode
 * 
 * Rembember to end every call to this function with a NULL value. 
 * 
 * @param first_line The first line of a series of comma separeted
 * strings ended with a NULL value.
 */
void    xml_rpc_support_multiple_write  (const char  * first_line, ...)
{
	va_list     args;
	char      * next_line;

	/* perform some checks */
	v_return_if_fail (first_line);

	/* opend the std args */
	va_start (args, first_line);

	/* write the first line */
	xml_rpc_support_write (first_line);

	/* and each line inside the parameters received */
	while ((next_line = va_arg (args, char  *)))
		xml_rpc_support_write (next_line);

	/* close std args */
	va_end (args);

	return;
}

#define xml_rpc_support_write_sl  xml_rpc_support_sl_write

/** 
 * @internal
 *
 * Allows to write content to the opened file as defined for \ref
 * af_gen_support_write but without placing tabular content.
 * 
 * @param format A printf-like format especifying the content to write
 * to the file.
 */
void    xml_rpc_support_sl_write        (const char  * format, ...)
{
	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	vfprintf (opened_file, format, args);
	va_end (args);

	/* return */
	return;

}

/** 
 * @internal Drop a log to the console, with the line heading.
 *
 * This function should be used only for dropping messages or logs to
 * the console, not for reporting errors.
 * 
 * @param format A printf-like format containing a log to drop.
 */
void    xml_rpc_report (const char  * format, ...)
{
	char  * result;

	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	result = axl_strdup_printfv (format, args);
	va_end (args);

	/* drop the log */
	fprintf (stdout, "[ ok ] %s\n", result);

	/* flush */
	fflush (stdout);

	/* release the memory hold */
	axl_free (result);

	/* return */
	return;
}

/** 
 * @internal Drop an error log to the console, with the line heading.
 *
 * This function should be used only for dropping messages or logs to
 * the console, not for reporting errors.
 * 
 * @param format A printf-like format containing a log to drop.
 */
void    xml_rpc_report_error (const char  * format, ...)
{
	char  * result;

	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	result = axl_strdup_printfv (format, args);
	va_end (args);

	/* drop the log */
	fprintf (stderr, "[ EE ] %s\n", result);

	/* flush */
	fflush (stderr);

	/* release the memory hold */
	axl_free (result);

	/* return */
	return;
}

/** 
 * @internal Perform a question to the console, with the line heading.
 *
 * This function should be used only for dropping messages or logs to
 * the console, not for reporting errors.
 * 
 * @param format A printf-like format containing a log to drop.
 */
void    xml_rpc_report_ask (const char  * format, ...)
{
	char  * result;

	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	result = axl_strdup_printfv (format, args);
	va_end (args);

	/* drop the log */
	fprintf (stderr, "[ ?? ] %s", result);

	/* flush */
	fflush (stderr);

	/* release the memory hold */
	axl_free (result);

	/* return */
	return;
}


/** 
 * @internal
 * 
 * Makes the provided file, located at the path formed by the
 * printf-like function, to be executable.
 * 
 * @param format The file to be executable, especified by the a
 * printf-like path.
 */
void    xml_rpc_support_make_executable (char  * format, ...)
{
	char  * result;
	
	va_list   args;

	/* write content defined by the parameters */
	va_start (args, format);
	result = axl_strdup_printfv (format, args);
	va_end (args);

	/* make a chmod operation */
	if (chmod (result, 0770) < 0) {
		xml_rpc_support_error ("error: unable to make executable the file: '%s'\n", axl_false, result);
	} /* end if */
	
	/* release the memory hold */
	axl_free (result);

	/* return */
	return;
	
}

/* init the list if it weren't */
void __xml_rpc_support_search_path_init (void)
{
	if (__xml_rpc_support_search_path == NULL) {
		/* create the list */
		__xml_rpc_support_search_path = axl_list_new (axl_list_equal_string, axl_free);
	} /* end if */

	return;
}

/** 
 * @internal
 * 
 * Allows to configure a new search path to locate files.
 * 
 * @param path The new path to be used as a base dir to locate files.
 */
void     xml_rpc_support_add_search_path     (char  * path)
{
	v_return_if_fail (path);

	/* init the list if it weren't */
	__xml_rpc_support_search_path_init ();
	
	/* add search path */
	axl_list_append (__xml_rpc_support_search_path, axl_strdup (path));

	return;
}

/** 
 * @internal
 * 
 * Allows to configure a new search path to locate files. However, the
 * parameter received is reference that is already allocated. The
 * function will not perform a copy.
 * 
 * @param path The new path to be used as a base dir to locate files.
 */
void     xml_rpc_support_add_search_path_ref (char  * path)
{

	/* init the list if it weren't */
	__xml_rpc_support_search_path_init ();

	/* add search path */
	axl_list_append (__xml_rpc_support_search_path, path);

	return;
}

/** 
 * @internal
 *
 * Performs an internal lookup for the file requested, using the
 * configured path locations.
 * 
 * @param name The file name to search.
 * 
 * @return The full path for the file located or NULL if it fails.
 */
char   * xml_rpc_support_find_data_file      (char  * name)
{
	axlListCursor * cursor;
	char          * file_name;

	/* do not perform any operation if the search path list isn't
	 * defined */
	if (__xml_rpc_support_search_path == NULL)
		return NULL;

	/* create the cursor */
	cursor = axl_list_cursor_new (__xml_rpc_support_search_path);

	/* foreach path installed */
	while (axl_list_cursor_has_item (cursor)) {
		/* get the file */
		file_name = axl_list_cursor_get (cursor);

		if (axl_cmp (file_name, "."))
			file_name = axl_strdup (name);
		else
			file_name = vortex_support_build_filename (file_name, name, NULL );

		/* check the file to be found */
		if (vortex_support_file_test (file_name, FILE_EXISTS | FILE_IS_REGULAR)) {
			/* free the cursor */
			axl_list_cursor_free (cursor);
			
			return file_name;
		} /* end if */

		axl_free (file_name);
		axl_list_cursor_next (cursor);
		
	} /* end while */

	/* free the cursor */
	axl_list_cursor_free (cursor);

	return NULL;
}

/** 
 * @internal
 * 
 * Internal implementation that support xml_rpc_support_to_lower and
 * xml_rpc_support_to_upper.
 */
char  * __xml_rpc_support_common_name (char  * name, int      to_upper)
{
	char  * result;
	int     iterator;

	axl_return_val_if_fail (name, NULL);
	
	/* get a lower copy */
	if (to_upper)
		result   = axl_stream_to_upper_copy (name);
	else
		result   = axl_stream_to_lower_copy (name);

	iterator = 0;

	/* check every character */
	while (result [iterator] != 0) {
		/* change the value */
		if (! isalnum (result [iterator]))
			result [iterator] = '_';

		/* update iterator */
		iterator++;
	}
	
	/* return result */
	return result;
}

/** 
 * @brief Allows to get the lower representation, not only including
 * alphabetic values but other values (like '-', which are translated
 * into _).
 * 
 * @param name The name to get its lower version.
 * 
 * @return A newly allocated string or NULL if it fails.
 */
char   * xml_rpc_support_to_lower            (char  * name)
{
	/* makes a to lower operation */
	return __xml_rpc_support_common_name (name, axl_false);
}

/** 
 * @brief Allows to get the upper representation, not only including
 * alphabetic values but other values (like '-', which are translated
 * into _).
 * 
 * @param name The name to get its lower version.
 * 
 * @return A newly allocated string or NULL if it fails.
 */
char   * xml_rpc_support_to_upper            (char  * name)
{
	/* makes a to upper operation */
	return __xml_rpc_support_common_name (name, axl_true);
}

/** 
 * @brief Provides the same functionality like \ref xml_rpc_file_test,
 * but allowing to provide the file path as a printf like argument.
 * 
 * @param format The path to be checked.
 * @param test The test to be performed. 
 * 
 * @return axl_true if all test returns axl_true. Otherwise axl_false is returned.
 */
axl_bool  xml_rpc_file_test_v (const char * format, VortexFileTest test, ...)
{
	va_list        args;
	char         * path;
	axl_bool       result;

	/* open arguments */
	va_start (args, test);

	/* get the path */
	path = axl_strdup_printfv (format, args);

	/* close args */
	va_end (args);

	/* do the test */
	result = vortex_support_file_test (path, test);
	axl_free (path);

	/* return the test */
	return result;
}

/** 
 * @internal
 *
 * Gets the service type prefix for a provided service, that is
 * represented by the <params> node.
 * 
 * @param params The <params> node that contains all <param> values
 * inside.
 * 
 * @return A newly allocated string, that represents the service type
 * prefix.
 */
char   * xml_rpc_support_get_function_type_prefix (axlNode * params)
{
	axlNode * aux2;
	axlNode * aux3;	

	/* service type unit */
	char    * type;
	char    * type_ref;
	
	/* service type prefix result */
	char    * result = NULL;

	/* now we have int aux a reference to the <params> node, get
	 * its first child */
	if (axl_node_have_childs (params)) {
		/* the params node have childs, which means the
		 * service have parameters */
		aux2 = axl_node_get_child_nth (params, 0);
		do {
			/* get a reference to the type node */
			aux3 = axl_node_get_child_nth (aux2, 1);
			
			/* get the type */
			type = axl_node_get_content_trim (aux3, NULL);
			type_ref = xml_rpc_support_to_lower (type);

			/* save status */
			result = axl_strdup_printf ("%s_%s", 
						    (result != NULL) ? result : "",
						    type_ref);

			axl_free (type_ref);

		}while ((aux2 = axl_node_get_next (aux2)) != NULL);
	}
	
	/* return string allocated */
	return (result != NULL) ? result : axl_strdup ("");
}

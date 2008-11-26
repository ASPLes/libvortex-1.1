/*  LibExploreArguments: a library to parse command line options
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
#ifndef __EXARG_H__
#define __EXARG_H__

/**
 * \addtogroup exarg
 * @{
 */

/**
 * @brief Enum type which defines how a argument must be interpreted.
 * 
 * This enumeration is used to figure out what type of argument is been installed.
 * You can also see the documentation \ref exarg_install_arg to get more info about
 * this enumeration.
 */
typedef enum { 
	/**
	 * @brief Defines an argument with no option. 
	 *
	 * \ref EXARG_NONE arguments are those with are switches for
	 * the program and receives not additional arguments to
	 * work. Examples of this types of arguments are usually
	 * <b>--help</b> and <b>--verson</b> options.
	 */
	EXARG_NONE, 
	/**
	 * @brief Defines an argument with is expected to receive an
	 * additional integer value.
	 *
	 * \ref EXARG_INT arguments are those used to instruct some
	 * value to the system. An example of this type of argument
	 * can be: <b>--column-size 17</b>
	 *
	 * Because you are using an argument with expects to receive
	 * an interger, LibExArg will check this for you.
	 * 
	 */

	EXARG_INT, 
	/**
	 * @brief Defines an argument with is expected to receive an
	 * additional string value.
	 *
	 * \ref EXARG_STRING arguments are those used to instruct some
	 * value to the system. An example of this type of argument
	 * can be: <b>--column-name "Test"</b>
	 *
	 * Because you are using an argument with expects to receive
	 * an string, LibExArg will check this for you.
	 *
	 * 
	 */
	EXARG_STRING
} ExArgType;

/** 
 * @brief Free argument definition. 
 *
 * This type represents an argument that is provided to the program
 * without options (not part of data associated to a option and not an
 * option itself. Once you get the first argument using \ref
 * exarg_get_params, you can get the data inside using:
 * 
 *  - \ref exarg_param_get (to get the string value)
 *
 * To get next parameters you can use:
 * 
 *  - \ref exarg_param_next (will return the next param to the provided param).
 * 
 */
typedef struct _ExArgument ExArgument;

void         exarg_parse        (int         argc,
			       char     ** argv);

void         exarg_end          ();

void         exarg_disable_help ();

void         exarg_add_usage_header  (char * header);

void         exarg_add_help_header   (char * header);

void         exarg_post_usage_header (char * post_header);

void         exarg_post_help_header  (char * post_header);

void         exarg_install_arg  (const char     * arg_name, 
				 const char     * arg_short_name, 
				 ExArgType   type,
				 const char     * description);

void         exarg_install_argv     (int num_arg, ...);

void         exarg_add_dependency   (const char * arg_name, 
				     const char * arg_dependency);

void         exarg_add_exclusion    (const char * arg_name, 
				     const char * arg_excluded);

void         exarg_set_obligatory   (char * arg_name);

void         exarg_accept_free_args (int accept);

void         exarg_define           (char * arg_name,
				   char * value);

int          exarg_is_defined       (char     * arg_name);

int          exarg_is_definedv      (char * first_value, ...);

char       * exarg_get_string       (char * arg_name);

char       * exarg_get_string_alloc (char * arg_name);

int          exarg_get_int          (char * arg_name);

ExArgument * exarg_get_params       ();

const char * exarg_param_get        (ExArgument * arg);

ExArgument * exarg_param_next       (ExArgument * arg);

int          exarg_get_params_num   ();

#endif

/* @} */

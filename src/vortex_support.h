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
#ifndef __VORTEX_SUPPORT_H__
#define __VORTEX_SUPPORT_H__

/**
 * \addtogroup vortex_support
 * @{
 */

#include <vortex.h>

void     vortex_support_free                       (int  params, ...);

axl_bool vortex_support_check_search_path          (VortexCtx  * ctx,
						    const char * domain,
						    const char * path);

void     vortex_support_add_search_path            (VortexCtx  * ctx,
						    const char * path);

void     vortex_support_add_search_path_ref        (VortexCtx * ctx, 
						    char      * path);

void     vortex_support_add_domain_search_path     (VortexCtx  * ctx,
						    const char * domain, 
						    const char * path);

void     vortex_support_add_domain_search_path_ref (VortexCtx  * ctx,
						    char       * domain, 
						    char       * path);

char   * vortex_support_find_data_file             (VortexCtx  * ctx,
						    const char * name);

char   * vortex_support_domain_find_data_file      (VortexCtx  * ctx,
						    const char * domain, 
						    const char * name);

void     vortex_support_init                       (VortexCtx * ctx);

void     vortex_support_cleanup                    (VortexCtx * ctx);

int      vortex_support_getenv_int                 (const char * env_name);

char *   vortex_support_getenv                     (const char * env_name);

axl_bool vortex_support_setenv                     (const char * env_name, 
						    const char * env_value);

axl_bool vortex_support_unsetenv                   (const char * env_name);

/** 
 * @brief Available tests to be performed while using \ref
 * vortex_support_file_test
 */
typedef enum {
	/** 
	 * @brief Check if the path exist.
	 */
	FILE_EXISTS     = 1 << 0,
	/** 
	 * @brief Check if the path provided is a symlink.
	 */
	FILE_IS_LINK    = 1 << 1,
	/** 
	 * @brief Check if the path provided is a directory.
	 */
	FILE_IS_DIR     = 1 << 2,
	/** 
	 * @brief Check if the path provided is a regular file.
	 */
	FILE_IS_REGULAR = 1 << 3
} VortexFileTest;

axl_bool vortex_support_file_test                  (const char * path,   
						    VortexFileTest test);

char   * vortex_support_build_filename             (const char  * name, ...);

double   vortex_support_strtod                     (const char  * param,
						    char       ** string_aux);

int      vortex_support_itoa                       (unsigned int   value,
						    char         * buffer,
						    int            buffer_size);

int      vortex_timeval_substract                  (struct timeval * a, 
						    struct timeval * b,
						    struct timeval * result);

char   * vortex_support_inet_ntoa                  (VortexCtx          * ctx, 
 						    struct sockaddr_in * sin);

int      vortex_support_pipe                       (VortexCtx * ctx, int descf[2]);

#define copy_if_not_null(arg) (arg != NULL) ? axl_strdup (arg) : NULL;

/* @} */

#endif

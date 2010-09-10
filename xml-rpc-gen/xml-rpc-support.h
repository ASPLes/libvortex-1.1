/* Hey emacs, show me this like 'c': -*- c -*-
 *
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

#ifndef __XML_RPC_SUPPORT_H__
#define __XML_RPC_SUPPORT_H__

/* directory handling */
char    * xml_rpc_support_create_dir      (char  * format, 
					   ...);

/* file handling */
void      xml_rpc_support_open_file       (char  * format, 
					   ...);

void      xml_rpc_support_close_file      (void);

axl_bool  xml_rpc_support_are_equal       (char * file1 , 
					   char * file2);

void      xml_rpc_support_error           (char      * message, 
					   axl_bool    must_exit, 
					   ...);

void      xml_rpc_support_move_file       (char  * from, 
					   char  * to);

/* indent handling */
void      xml_rpc_support_push_indent     (void);

#define push_indent xml_rpc_support_push_indent

void      xml_rpc_support_pop_indent      (void);

#define pop_indent xml_rpc_support_pop_indent

/* writing content into files handling */
#define write xml_rpc_support_write

void      xml_rpc_support_write           (const char  * format, ...);

#define xml_rpc_support_mwrite xml_rpc_support_multiple_write

void      xml_rpc_support_multiple_write  (const char  * first_line, ...);

#define xml_rpc_support_write_sl  xml_rpc_support_sl_write

void      xml_rpc_support_sl_write        (const char  * format, ...);

#define ok_msg xml_rpc_report
void      xml_rpc_report       (const char  * format, ...);

#define error_msg xml_rpc_report_error
void      xml_rpc_report_error (const char  * format, ...);

#define ask_msg xml_rpc_report_ask
void      xml_rpc_report_ask (const char  * format, ...);

void      xml_rpc_support_make_executable (char  * format, ...);

/* file location functions */
void      xml_rpc_support_add_search_path     (char  * path);

void      xml_rpc_support_add_search_path_ref (char  * path);

char    * xml_rpc_support_find_data_file      (char  * name);

/* name handling */
char    * xml_rpc_support_to_lower            (char  * name);

char    * xml_rpc_support_to_upper            (char  * name);

/** 
 * @brief Simple alias definition to \ref xml_rpc_file_test.
 */
#define file_test xml_rpc_file_test

/** 
 * @brief Simple alias definition to \ref xml_rpc_file_test_v.
 */
#define file_test_v xml_rpc_file_test_v

axl_bool xml_rpc_file_test_v (const char * format, VortexFileTest test, ...);

char   * xml_rpc_support_get_function_type_prefix (axlNode * params);

#endif

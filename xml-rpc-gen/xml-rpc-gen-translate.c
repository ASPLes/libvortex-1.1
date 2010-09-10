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

#include <xml-rpc.h>

#define XML_RPC_GEN_CONSUME_SPACES(stream, error) \
axl_stream_consume_white_spaces (stream); xml_rpc_gen_translate_consume_comments (stream, error)

/* enforced resources */
axlHash * resources = NULL;

void xml_rpc_gen_translate_consume_comments (axlStream * stream, axlError ** error)
{
	axl_bool  found_item;
	int       size;
	
	/* know, try to read comments a process instructions.  Do this
	 * until both fails. Do this until one of them find
	 * something. */
	do {
		/* flag the loop to end, and only end if both,
		 * comments matching and PI matching fails. */
		found_item = axl_false;
		
		/* get rid from spaces */
		AXL_CONSUME_SPACES (stream);

		/* check for comments */
		if (axl_stream_inspect (stream, "/*", 2) > 0) {
			
			if (! axl_stream_get_until_ref (stream, NULL, NULL, axl_true, &size, 1, "*/")) {
				axl_error_new (-1, "detected an opened comment but not found the comment ending",
					       stream, error);
				return;
			} 
			
			/* flag that we have found a comment */
			found_item = axl_true;
		}

		/* get rid from spaces */
		AXL_CONSUME_SPACES(stream);

		if (axl_stream_inspect (stream, "//", 2) > 0) {
			if (! axl_stream_get_until_ref (stream, NULL, NULL, axl_true, &size, 1, "\n")) {
				axl_error_new (-1, "detected an opened comment but not found the comment ending",
					       stream, error);
				return;
			}

			/* flag that we have found a comment */
			found_item = axl_true;
		}
	
		/* get rid from spaces */
		AXL_CONSUME_SPACES(stream);

		/* check to break-the-loop */
	}while (found_item);

	return;
}

axlNode * __xml_rpc_gen_translate_struct (axlDoc * doc, axl_bool  * result, axlStream * stream, axlError ** error) 
{
	char    * name;
	char    * type;
	int       chunk_matched;

	axlNode * _struct;
	axlNode * root;
	axlNode * node;
	axlNode * nodeAux;


	/* consume spaces and comments */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	/* seems we have an struct definition, get the struct
	 * name */
	name = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, " ");
	if (name == NULL) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find the struct name but it wasn't found", 
			       stream, error);
		axl_stream_free (stream);
		return NULL;
	}
	
	/* create the struct node */
	_struct = axl_node_create ("struct");
	root    = axl_doc_get_root (doc);
	axl_node_set_child (root, _struct);
	
	/* add the struct name */
	node   = axl_node_create ("name");
	axl_stream_nullify (stream, LAST_CHUNK);
	axl_node_set_content_ref (node, name, -1);
	axl_node_set_child (_struct, node);
	
	/* now get the next open bracket */
	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);
	
	/* now get the services types, first, the open parent */
	if (! (axl_stream_inspect (stream, "{", 1) > 0)) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find an open bracket {, while reading a struct definition", 
			       stream, error);
		axl_stream_free (stream);
		return NULL;
	}
	
	/* parse all members */
	while (! (axl_stream_inspect (stream, "}", 1) > 0)) {
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);
		
		/* get the member type */
		type = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, " ");
		axl_stream_nullify (stream, LAST_CHUNK);
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);
		
		/* get the member name */
		name = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 2, " ", ";");
		axl_stream_nullify (stream, LAST_CHUNK);
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);
		
		if (chunk_matched == 0) {
			/* seems the user have placed a white
			 * space between the name and the
			 * semicolon */
			if (! (axl_stream_inspect (stream, ";", 1) > 0)) {
				(* result) = axl_false;
				axl_error_new (-2, "Expected to find a semi-colon (;), while reading a member name definition.", 
					       stream, error);
				axl_stream_free (stream);
				return NULL;
			}
		}
		
		/* add the member to the struct */
		node = axl_node_create ("member");
		axl_node_set_child (_struct, node);
		
		/* add the name to the member */
		nodeAux = axl_node_create ("name");
		axl_node_set_content_ref (nodeAux, name, -1);
		axl_node_set_child (node, nodeAux);
		
		/* add the type to the nameber  */
		nodeAux = axl_node_create ("type");
		axl_node_set_content_ref (nodeAux, type, -1);
		axl_node_set_child (node, nodeAux);
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);
	}
	
	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);
	
	/* struct properly parsed */
	(* result) = axl_true;
	return _struct;
}

/** 
 * @internal Parse IDL definition file and translate it into an
 * in-memory XDL definition.
 */
axlNode * __xml_rpc_gen_translate_array (axlDoc * doc, axl_bool  * result, axlStream * stream, axlError ** error)
{
	axlNode * array;
	axlNode * root;
	axlNode * node;

	char    * array_name;
	char    * node_type;

	int       chunk_matched;

	/* consume spaces and comments */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	/* get the array name */
	array_name = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, " ");
	axl_stream_nullify (stream, LAST_CHUNK);
	if (array_name == NULL) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find the array name declaration but it wasn't found", 
			       stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	/* consume spaces and comments */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	/* get of keyword */
	if (! (axl_stream_inspect (stream, "of", 2) > 0)) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find array \"of\" keyword, but it wasn't found. It separates the array declaration from the type holded inside it.", 
			       stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	/* consume spaces and comments */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);
	
	/* now get the node type inside the array */
	node_type = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 2, " ", ";");
	axl_stream_nullify (stream, LAST_CHUNK);
	if (node_type == NULL) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find the array node type declaration (the type inside the array) but it wasn't found", 
			       stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	/* get last semicolon */
	if (chunk_matched == 0) {
		/* chunk matched was white space (in any form) */
		if (! (axl_stream_inspect (stream, ";", 1) > 0)) {
			(* result) = axl_false;
			axl_error_new (-2, "Expected to find a semicolon at the end of an array type declaration.", 
				       stream, error);
			axl_stream_free (stream);
			return NULL;
		}
	}

	/* now create the xml structure, representing the array */
	array = axl_node_create ("array");
	root  = axl_doc_get_root (doc);
	axl_node_set_child (root, array);
	
	/* add the array name */
	node   = axl_node_create ("name");
	axl_node_set_content_ref (node, array_name, -1);
	axl_node_set_child (array, node);

	/* add the array type */
	node   = axl_node_create ("type");
	axl_node_set_content_ref (node, node_type, -1);
	axl_node_set_child (array, node);

	/* add the size declaration (for now, always 0) */
	node   = axl_node_create ("size");
	axl_node_set_content (node, "0", 1);
	axl_node_set_child (array, node);
	
	/* return the array */
	(* result) = axl_true;
	return array;
}

axl_bool  __xml_rpc_gen_translate_service_check_method_name_decl (axlNode   * service, 
								  char      * service_name,
								  axlHash   * attributes,
								  axlStream * stream, 
								  axl_bool  * result,
								  axlError ** error)
{
	char    * name;
	axlNode * node;

	/* check if attribute hash have the method name declaration */
	if (axl_hash_exists (attributes, "method_name")) {

		/* method name is defined check its value */
		name = axl_hash_get (attributes, "method_name");
		if (name == NULL) {
			(* result) = axl_false;
			axl_error_new (-2, "[method_name] attribute was defined for the service but not its value, this is not allowed.", 
				       stream, error);
			axl_stream_free (stream);
			return axl_false;
		}
		
		/* set method name child */
		node = axl_node_create ("method_name");
		axl_node_set_content (node, name, -1);
		axl_node_set_child (service, node);
		
		xml_rpc_report ("configuring alternative method name='%s' for service='%s'..",
				name, service_name);
	}
	
	return axl_true;
}


char * xml_rpc_gen_translate_copy_and_escape (const char * content, 
					      int          content_size, 
					      int          additional_size)
{
	int    iterator  = 0;
	int    iterator2 = 0;
	char * result;
	axl_return_val_if_fail (content, axl_false);

	/* allocate the memory to be returned */
	result = axl_new (char, content_size + additional_size + 1);

	/* iterate over all content defined */
	while (iterator2 < content_size) {
		/* check for &apos; */
		if (content [iterator2] == '%') {
			memcpy (result + iterator, "%%", 2);
			iterator += 2;
			iterator2++;
			continue;
		}

		/* copy value received because it is not an escape
		 * sequence */
		memcpy (result + iterator, content + iterator2, 1);

		/* update the iterator */
		iterator++;
		iterator2++;
	}

	/* return results */
	return result;
}

axl_bool       xml_rpc_gen_translate_invalid_chars        (const char * content,
							   int          content_size,
							   int        * added_size)
{
	int      iterator = 0;
	int      result   = axl_false;
	axl_return_val_if_fail (content, axl_false);

	/* reset additional size value */
	if (added_size != NULL)
		*added_size = 0;

	/* calculate the content size */
	if (content_size == -1)
		content_size = strlen (content);

	__axl_log (LOG_DOMAIN, AXL_LEVEL_DEBUG, "checking valid sequence: content size=%d", content_size);

	/* iterate over all content defined */
	while (iterator < content_size) {
		/* check for &apos; */
		if (content [iterator] == '%') {
			__axl_log (LOG_DOMAIN, AXL_LEVEL_DEBUG, "found invalid sequence='%%'");
			result = axl_true;
			if (added_size != NULL)
				(*added_size) += 1;
		}

		/* update the iterator */
		iterator++;
	}

	/* return results */
	return result;
}

char * __get_all_user_content (axlStream * stream)
{

	char       * user_code;
	char       * string_aux;
	char       * user_code_aux;
	char       * user_code_complete = NULL;
	int          chunk_matched;
	int          additional_size;
	int          parents_opened = 0;

	while (axl_true) {

		/* get the user code for the service */
		user_code = axl_stream_get_until (stream, NULL, &chunk_matched, 
						  axl_true, 3, "};", "}", "{");
		axl_stream_nullify (stream, LAST_CHUNK);
		
		/* check here for user declarations using the
		 * % char, to replace it with a double char
		 * decl. */
		if (xml_rpc_gen_translate_invalid_chars (user_code, strlen (user_code), &additional_size)) {
			/* found, seems the user have placed a
			 * % sign, translate it */
			string_aux = user_code;
			user_code  = xml_rpc_gen_translate_copy_and_escape (user_code, 
									    strlen (user_code),
									    additional_size);
			axl_free (string_aux);
		} /* end if */
		
		/* store service content */
		user_code_aux      = user_code_complete;
		if (parents_opened == 0 && (chunk_matched == 0 || chunk_matched == 1)) {
			user_code_complete = axl_stream_strdup_printf ("%s%s",
								       (user_code_aux != NULL) ? user_code_aux : "",
								       user_code);
		} else {
			user_code_complete = axl_stream_strdup_printf ("%s%s%s",
								       (user_code_aux != NULL) ? user_code_aux : "",
								       user_code,
								       (chunk_matched == 2) ? "{" : "}");
		} /* end if */
		
		/* free previous code */
		axl_free (user_code_aux);
		
		/* release prevous content */
		axl_free (user_code);
		
		/* seems that a closing parent have been
		 * received */
		if (chunk_matched == 0 || 
		    chunk_matched == 1) {
			if (parents_opened == 0)
				break;
			
			/* decreate current parent opened */
			parents_opened--;
		}
		
		/* detected an open parent, store it to
		 * properly detect service code */
		if (chunk_matched == 2) {
			parents_opened++;
		}
	} /* end while */

	/* return code readed */
	return user_code_complete;
}


char * __get_all_user_content_from_file (char * fileName)
{
	FILE * file;
	char   buffer[4096];
	int    read_content;
	char * result = axl_strdup ("");
	char * string_aux;

	file = fopen (fileName, "r");
	if (file == NULL) {
		error_msg ("Failed to open file: %s", fileName);
		return NULL;
	} /* end if */
	
	do {
		/* read the content */
		read_content = fread (buffer, 1, 4095, file);
		
		/* nullify the content */
		if (read_content >= 0)
			buffer[read_content] = 0;

		/* append content found */
		string_aux = result;
		result = axl_strdup_printf ("%s%s", result, buffer);
		axl_free (string_aux);

	} while (feof (file) == 0);

	/* close the file */
	fclose (file);

	/* return content read */
	return result;
}

/** 
 * @internal Translate a service description into the XDL
 * representation.
 */
axlNode * __xml_rpc_gen_translate_service (char      * return_type, 
					   axlDoc    * doc, 
					   axl_bool  * result, 
					   axlStream * stream, 
					   axlHash   * attributes, 
					   axlError ** error)
{

	char       * type;
	char       * name;
	char       * service_name;
	char       * user_code_complete;

	axlNode * root;
	axlNode * service;
	axlNode * params = NULL;
	axlNode * code;
	axlNode * content;

	axlNode * node;
	axlNode * nodeAux;

	int       chunk_matched;
	int       parents_opened;
	char    * fileName;


	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* now get the service name */
	service_name = axl_stream_get_until (stream, NULL, NULL, axl_true, 1, " ");
	axl_stream_nullify (stream, LAST_CHUNK);

	/* create the service node */
	service = axl_node_create ("service");
	root    = axl_doc_get_root (doc);

	axl_node_set_child (root, service);

	/* configure service name */
	node    = axl_node_create ("name");
	axl_node_set_content_ref (node, service_name, -1);
	axl_node_set_child (service, node);

	/* configure service return type */
	node    = axl_node_create ("returns");
	axl_node_set_content_ref (node, return_type, -1);
	axl_node_set_child (service, node);

	/* configure resource if defined */
	if (axl_hash_exists (attributes, "resource")) {
		node = axl_node_create ("resource");
		axl_node_set_content (node, axl_hash_get (attributes, "resource"), -1);
		axl_node_set_child (service, node);
	} /* end if */

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* now get the services types, first, the open parent */
	if (! (axl_stream_inspect (stream, "(", 1) > 0)) {
		(* result) = axl_false;
		axl_error_new (-2, "Expected to find an open parent (, while reading service definition", 
			      stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	/* create the params node and add it to the service */
	params = axl_node_create ("params");
	axl_node_set_child (service, params);

	/* get service parameter definition */
	while (axl_true) {
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);

		/* check if the service parameter definition have
		 * ended */
		if (axl_stream_inspect (stream, ")", 1) > 0)
			break;

		/* get the parameter type */
		type = axl_stream_get_until (stream, NULL, NULL, axl_true, 1, " ");
		axl_stream_nullify (stream, LAST_CHUNK);
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES(stream, error);

		/* get the parameter name */
		name = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 3, " ", ",", ")");
		axl_stream_nullify (stream, LAST_CHUNK);

		/* create the param node */
		node = axl_node_create ("param");
		axl_node_set_child (params, node);
		
		/* create the param name */
		nodeAux = axl_node_create ("name");
		axl_node_set_content_ref (nodeAux, name, -1);
		axl_node_set_child (node, nodeAux);
		
		/* create the param type */
		nodeAux = axl_node_create ("type");
		axl_node_set_content_ref (nodeAux, type, -1);
		axl_node_set_child (node, nodeAux);

		/* check matched chunk */
		if (chunk_matched == 2) {
			/* there are no more parameters */
			break;
		}

		if (chunk_matched == 0) {
			/* we need to consume the comma */

			/* consume spaces */
			XML_RPC_GEN_CONSUME_SPACES(stream, error);

			/* check for comma, and, if found, consume
			 * it */
			if (axl_stream_peek (stream, ",", 1) > 0)
				axl_stream_accept (stream);
		} /* end if */
	} /* end while */

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* now try to parse the service content if defined */
	if (axl_stream_inspect (stream, ";", 1) > 0) {
		/* the service doesn't have user code defined, but
		 * before returning, check if we have a method name
		 * declaration  */
		/* check for method_name declaration */
		if (! __xml_rpc_gen_translate_service_check_method_name_decl (service, service_name, attributes, stream, result, error))
			return NULL;

		/* if ok, fill the value with axl_true */
		(* result) = axl_true;
		return service;
	}

	/* if reached this place, it means that the user have defined
	 * usr code for the service */
	if (axl_stream_inspect (stream, "{", 1) > 0) {
		/* parse the service code definition */
		parents_opened = 0;

		/* configure node relations */
		code           = axl_node_create ("code");
		content        = axl_node_create ("content");

		/* configure node relations */
		axl_node_set_child (service, code);
		axl_node_set_child (code, content);

		/* get the user content */
		user_code_complete = __get_all_user_content (stream);

		/* configure the content */
		axl_node_set_cdata_content (content, user_code_complete, -1);
		axl_free (user_code_complete);

		/* check for method_name declaration */
		if (! __xml_rpc_gen_translate_service_check_method_name_decl (service, service_name, attributes, stream, result, error))
			return NULL;
		
	} /* end if */

	xml_rpc_report ("service declaration %s found..", service_name);

	/* check for additional attributes */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	if (axl_stream_inspect (stream, "options", 7) > 0) {
		xml_rpc_report ("  found additional options for %s", service_name);
		/* check for additional attributes */
		XML_RPC_GEN_CONSUME_SPACES (stream, error);

		if (! (axl_stream_inspect (stream, "{", 1) > 0)) {
			error_msg ("expected to find { after options declaration");
			return NULL;
		}

		/* create the options node */
		code           = axl_node_create ("options");
		axl_node_set_child (service, code);

		while (axl_true) {

			/* check for additional attributes */
			XML_RPC_GEN_CONSUME_SPACES (stream, error);

			/* check if we have to stop */
			if (axl_stream_inspect (stream, "};", 2) > 0)
				break;

			if (axl_stream_inspect (stream, "}", 1) > 0)
				break;

			if (axl_stream_inspect (stream, "include", 7) > 0) {
				/* include on body declaration */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);

				if (! (axl_stream_inspect (stream, "on", 2) > 0)) {
					error_msg ("expected to find 'on' after 'include' declaration");
					return NULL;
				} /* end if */

				/* include on body declaration */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);

				/* check which type of include */
				if (axl_stream_inspect (stream, "body", 4) > 0) {
					ok_msg ("  found include on body declaration for %s", service_name);

					/* include on body declaration */
					XML_RPC_GEN_CONSUME_SPACES (stream, error);
					
					/* configure node relations */
					content        = axl_node_create ("body");
					axl_node_set_child (code, content);
					
					/* get the user code as inline of the xml file */
					user_code_complete = NULL;
					if (axl_stream_inspect (stream, "{", 1) > 0) {

						/* get the user content */
						user_code_complete = __get_all_user_content (stream);
						axl_node_set_attribute (content, "from", "idl-inline");

					} else if ((axl_stream_inspect (stream, "\"", 1) > 0) || 
						   (axl_stream_inspect (stream, "'", 1) > 0)) {

						/* get the file */
						fileName = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 2, "\"", "'");
						ok_msg ("  found include on body file: %s", fileName);
						axl_node_set_attribute (content, "from", fileName);
						axl_stream_nullify (stream, LAST_CHUNK);

						/* include on body declaration */
						XML_RPC_GEN_CONSUME_SPACES (stream, error);

						if (! (axl_stream_inspect (stream, ";", 1) > 0)) {
							error_msg ("Expected to find a ';' sign after file name declaration");
							return NULL;
						}

						/* include on body declaration */
						XML_RPC_GEN_CONSUME_SPACES (stream, error);

						/* open the file and include the content */
						user_code_complete = __get_all_user_content_from_file (fileName);
						axl_free (fileName);
						
					} else {
						error_msg ("Expected to find inline declaration ({) or file reference..");
						return NULL;
					}


					/* configure the content */
					axl_node_set_cdata_content (content, user_code_complete, -1);
					axl_free (user_code_complete);
					
				} else if (axl_stream_inspect (stream, "header", 6) > 0) {
					ok_msg ("  found include on header declaration for %s", service_name);

					/* include on body declaration */
					XML_RPC_GEN_CONSUME_SPACES (stream, error);

					/* configure node relations */
					content        = axl_node_create ("header");
					axl_node_set_child (code, content);

					/* get the user code as inline of the xml file */
					user_code_complete = NULL;
					if (axl_stream_inspect (stream, "{", 1) > 0) {

						/* get the user content */
						user_code_complete = __get_all_user_content (stream);

					} else if ((axl_stream_inspect (stream, "\"", 1) > 0) || 
						   (axl_stream_inspect (stream, "'", 1) > 0)) {

						/* get the file */
						fileName = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 2, "\"", "'");
						ok_msg ("  found include on body file: %s", fileName);
						axl_stream_nullify (stream, LAST_CHUNK);

						/* include on body declaration */
						XML_RPC_GEN_CONSUME_SPACES (stream, error);

						if (! (axl_stream_inspect (stream, ";", 1) > 0)) {
							error_msg ("Expected to find a ';' sign after file name declaration");
							return NULL;
						}

						/* include on body declaration */
						XML_RPC_GEN_CONSUME_SPACES (stream, error);

						/* open the file and include the content */
						user_code_complete = __get_all_user_content_from_file (fileName);
						axl_free (fileName);
						
					} else {
						error_msg ("Expected to find inline declaration ({) or file reference..");
						return NULL;
					}

					/* configure the content */
					axl_node_set_cdata_content (content, user_code_complete, -1);
					axl_free (user_code_complete);
				}
			} /* end if */
		} /* end while */
	} /* end if */
	
	(* result) = axl_true;
	return service;
	
} /* end service translation */

/** 
 * @internal Function that parses an "allowed resources" declaration
 * which allows to enforce resource declaration for services found
 * inside the IDL file.
 */
axl_bool  __xml_rpc_gen_parse_allowed_resources_decl (axlStream  * stream, 
						      axlError  ** error)
{
	char * attr_value = NULL;
	int    chunk_matched;

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	/* check and consume "resources" */
	if (! (axl_stream_inspect (stream, "resources", 9) > 0)) {
		axl_error_new (-1, "Expected to find 'resources' declaration after 'allowed', but it wasn't found", stream, error);
		return axl_false;
	}
	
	/* now parse strings */
	while (axl_true) {
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES (stream, error);

		/* terminate parsing */
		if (axl_stream_inspect (stream, ";", 1) > 0)
			break;

		/* get the type of value either "..." or '...' */
		if (axl_stream_inspect (stream, "\"", 1) > 0) {
			/* we got the "..." declaration */
			attr_value = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, "\"");
		}else if (axl_stream_inspect (stream, "\"", 1) > 0) {
			/* we got the '...' declaration */
			attr_value = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, "'");
		}
		
		/* nullify the stream reference to the result */
		axl_stream_nullify (stream, LAST_CHUNK);
		
		/* if attr value is NULL we got a error */
		if (attr_value == NULL) {
			axl_error_new (-2, "Expected to find the resource declaration but it wasn't found", 
				       stream, error);
			return axl_false;
		}
		
		/* register the value: attribute declaration with
		   value */
		if (resources == NULL)
			resources = axl_hash_new (axl_hash_string, axl_hash_equal_string);
		
		/* check if the resource already exists */
		if (axl_hash_exists (resources, attr_value)) {
			xml_rpc_report ("?! found enforced resource already declared: %s", attr_value);
			axl_free (attr_value);
		} else {
			
			/* add the resource */
			axl_hash_insert_full (resources, attr_value, axl_free, attr_value, NULL);
			xml_rpc_report ("found enforced resource: %s", attr_value);
		}
		
		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES (stream, error);
		
		/* continue */
		if (axl_stream_inspect (stream, ",", 1) > 0)
			continue;
		    
		if (axl_stream_inspect (stream, ";", 1) > 0)
			break;
	} /* end while */

	/* finish */
	return axl_true;
}
	
axlNode * xml_rpc_gen_translate_parse_interface_item (axlDoc     * doc, 
						      axlStream  * stream, 
						      axl_bool   * result, 
						      axlError  ** error)
{
	/* [attribute declaration] */
	char      * attr_decl  = NULL;
	char      * attr_value = NULL;
	axlHash   * attributes = NULL;

	char      * return_type;
	int         chunk_matched;

	axlNode   * res;

	/* label used to restart parsing services */
 restart_parsing:

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES (stream, error);

	/* check for attribute declaration */
	if (axl_stream_inspect (stream, "[", 1) > 0) {
		/* create the attribute list */
		attributes = axl_hash_new (axl_hash_string, axl_hash_equal_string);

		/* for all attribute declaration found inside the
		 * [...]; do */
		while (axl_true) {
			/* get attribute declaration */
			attr_decl = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 5, " ", "=", ",", "];", "]");
			axl_stream_nullify (stream, LAST_CHUNK);
		
			/* check chunk matched */
			if (chunk_matched == 0) {
				/* consume spaces */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);
				
				/* flag chunk matched 1 */
				if (axl_stream_inspect (stream, "=", 1) > 0)
					chunk_matched = 1;

				/* flag chunk matched 2 */
				if (axl_stream_inspect (stream, ",", 1) > 0)
					chunk_matched = 2;

				/* flag chunk matched 3 */
				if ((axl_stream_inspect (stream, "];", 2) > 0) || (axl_stream_inspect (stream, "]", 1) > 0))
					chunk_matched = 3;
			}

			/* in both cases, the attribute declaration is
			 * limited to the value register it */
			if (chunk_matched == 2 || chunk_matched == 3 || chunk_matched == 4) {
				xml_rpc_report ("registered empty attribute '%s'..", attr_decl);
				/* attribute declaration without
				 * value */
				axl_hash_insert_full (attributes, attr_decl, axl_free, NULL, NULL);
			}

			/* the attribute declaration have value */
			if (chunk_matched == 1) {

				/* consume spaces */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);

				/* get the type of value either "..." or
				 * '...' */
				if (axl_stream_inspect (stream, "\"", 1) > 0) {
					/* we got the "..." declaration */
					attr_value = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, "\"");
				}else if (axl_stream_inspect (stream, "\"", 1) > 0) {
					/* we got the '...' declaration */
					attr_value = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 1, "'");
				}

				/* nullify the stream reference to the
				 * result */
				axl_stream_nullify (stream, LAST_CHUNK);
				
				/* if attr value is NULL we got a
				 * error */
				if (attr_value == NULL) {
					(* result ) = axl_false;
					axl_error_new (-2, "Expected to find the attribute value declaration but it wasn't found", 
						       stream, error);
					return NULL;
				}

				/* check here recognized attributes */
				if (axl_cmp (attr_decl, "resource")) {
					/* found resource declaration,
					 * check if we have enforced
					 * resource declaration */
					if (resources != NULL && ! axl_hash_exists (resources, attr_value)) {
						error_msg ("found resource declaration '%s' not found in the enforced list", 
							   attr_value);
						axl_error_new (-2, "found resource declaration not found in the enforce list", 
							       stream, error);
						return NULL;
					} /* end if */
				} /* end if */
				
				/* register the value: attribute
				   declaration with value */
				axl_hash_insert_full (attributes, attr_decl, axl_free, attr_value, axl_free);

				xml_rpc_report ("registered valued attribute %s='%s'..", attr_decl, attr_value);

				/* consume spaces */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);

				if ((axl_stream_inspect (stream, "];", 2) > 0) || (axl_stream_inspect (stream, "]", 1) > 0))
					chunk_matched = 3;
				else if (axl_stream_inspect (stream, ",", 1) > 0)
					chunk_matched = 2;
				else if (axl_stream_inspect (stream, "=", 1) > 0) {
					(* result ) = axl_false;
					axl_error_new (-2, "Found and unexpected '=' simbol at attribute declaration", 
						       stream, error);
					return NULL;
				}
					
			} /* end if */
			
			/* check for attribute declaration
			 * termination */
			if (chunk_matched == 3) {
				/* no more attributes to be
				 * declarted */
				break;
			}
			
			if (chunk_matched == 2) {
				/* more attribute declarations
				   consume spaces */
				XML_RPC_GEN_CONSUME_SPACES (stream, error);				
			}

		} /* end while */

		/* consume spaces */
		XML_RPC_GEN_CONSUME_SPACES (stream, error);
	}

	/* get the service return type */
	return_type = axl_stream_get_until (stream, NULL, &chunk_matched, axl_true, 2, " ", "}");

	/* check that there is no more services defined */
	if (chunk_matched == 1) {
		(* result) = axl_true;
		return NULL;
	}

	/* nullify here to make return_type to be owned by this
	 * scope */
	axl_stream_nullify (stream, LAST_CHUNK);

	/* check for struct definition */
	if (axl_cmp (return_type, "struct")) {
		/* parse an struct */
		return __xml_rpc_gen_translate_struct (doc, result, stream, error);
	}

	if (axl_cmp (return_type, "array")) {
		/* parse an array */
		return __xml_rpc_gen_translate_array (doc, result, stream, error);
	}

	if (axl_cmp (return_type, "allowed")) {
		/* found allowed resources declaration */
		if (! __xml_rpc_gen_parse_allowed_resources_decl (stream, error)) {
			(* result) = axl_false;
			return NULL;
		}

		/* restart parsing services */
		goto restart_parsing;
	}

	/* parse and translate the service */
	res = __xml_rpc_gen_translate_service (return_type, doc, result, stream, attributes, error);
	
	/* free the hash */
	axl_hash_free (attributes);

	return res;
}

axl_bool  xml_rpc_gen_translate_parse_services (axlDoc     * doc, 
						axlStream  * stream, 
						axlError  ** error)
{
	axlNode   * service;
	axlNode   * root;
	axl_bool    result;

	/* get the root node */
	root = axl_doc_get_root (doc);

	/* parse the service */
	while (axl_true) {
		/* parse one service */
		service = xml_rpc_gen_translate_parse_interface_item (doc, stream, &result, error);

		/* check parse result */
		if (! result) 
			return axl_false;

		/* check if there are no more services to parse */
		if (service == NULL) 
			break;
	}

	return axl_true;
}

axlDoc * xml_rpc_gen_translate_idl_to_xdl (const char  * selected, axlError ** error)
{
	axlStream * stream;
	axlDoc    * doc;
	axlNode   * node;
	axlNode   * nodeAux;

	char      * string_aux;
	

	/* create the stream and try to parse */
	stream     = axl_stream_new (NULL, -1, selected, -1, error);
	string_aux = NULL;
	v_return_val_if_fail (stream, NULL);

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* we have already read the stream, now parse the document
	 * header */
	if (! axl_stream_inspect (stream, "xml-rpc", 7) > 0) {
		axl_error_new (-2, "Expected to find xml-rpc keyword at the begin of the interface definition", 
			      stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	xml_rpc_report ("detected IDL format definition..");

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* we have already read the stream, now parse the document
	 * header */
	if (! axl_stream_inspect (stream, "interface", 9) > 0) {
		axl_error_new (-2, "Expected to find interface keyword at the begin of the interface definition", 
			      stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);

	/* now get the name of the interface */
	string_aux = axl_stream_get_until (stream, NULL, NULL, axl_true, 1, " ");
	axl_stream_nullify (stream, LAST_CHUNK);
	if (string_aux == NULL) {
		axl_error_new (-2, "Expected to find the interface name", stream, error);
		axl_stream_free (stream);
		return NULL;
	}

	xml_rpc_report ("detected xml-rpc definition: '%s'..", string_aux);
	
	/* create the document */
	doc  = axl_doc_create ("1.0", NULL, axl_true);
	node = axl_node_create ("xml-rpc-interface");
	axl_doc_set_root (doc, node);
	
	/* set the xml-rpc interface name */
	nodeAux = axl_node_create ("name");
	axl_node_set_content_ref (nodeAux, string_aux, -1);
	axl_node_set_child (node, nodeAux);

	/* link the document to the stream */
	axl_stream_link (stream, doc, (axlDestroyFunc) axl_doc_free);

	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);
	
	/* get the open brace */
	if (! axl_stream_inspect (stream, "{", 1) > 0) {
		axl_error_new (-2, "Expected to find an open brace, enclosing services definition", stream, error);
		axl_stream_free (stream);
		return NULL;	
	}

	/* parse services: if something goes wrong, just return NULL
	 * because the function already deallocates the document and
	 * the stream */
	if (! xml_rpc_gen_translate_parse_services (doc, stream, error))
		return NULL;
	
	/* consume spaces */
	XML_RPC_GEN_CONSUME_SPACES(stream, error);
	
	/* get the open brace */
	if (! axl_stream_inspect (stream, "}", 1) > 0) {
		axl_error_new (-2, "Expected to find a close brace, closing services definition", stream, error);
		axl_stream_free (stream);
		return NULL;	
	}
	
	/* unlink all references */
	axl_stream_unlink (stream);
	axl_stream_free (stream);
	axl_hash_free (resources);

	/* return the document */
	return doc;
}

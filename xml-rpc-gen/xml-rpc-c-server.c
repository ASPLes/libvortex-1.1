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
#include <xml-rpc-c-server.h>

/* write initial header comments */
void xml_rpc_c_server_write_header (char  * comp_name)
{
	xml_rpc_support_multiple_write ("/**\n",
					" * C server skel to implement services exported by the XML-RPC\n",
					NULL);
	xml_rpc_support_write (" * component: %s.\n", comp_name);
	xml_rpc_support_multiple_write (" *\n",
					" * This file was generated by xml-rpc-gen tool, from Vortex Library\n",
					" * project. \n",
					" *\n",
					" * Vortex Library homepage:           http://vortex.aspl.es\n",
					" * Axl Library homepage:              http://xml.aspl.es\n",
					" * Advanced Software Production Line: http://www.aspl.es\n",
					" */\n", NULL);
	return;
}

/** 
 * @internal
 *
 * Writes all includes that enables the access to functions XML-RPC
 * exported.
 * 
 * @param doc The axlDoc that holds the interface.
 *
 * @param comp_name The component name.
 */
void xml_rpc_write_c_server_include_services (axlDoc * doc, 
					      char   * comp_name)
{
	axlNode * service;
	axlNode * name;
	axlNode * params;

	char    * content;
	char    * to_lower;
	
	/* get a reference to the first <service> node */
	service = axl_doc_get (doc, "/xml-rpc-interface/service");
	
	/* for all services found */
	while (service != NULL) {

		if (! (NODE_CMP_NAME (service, "service"))) {
			/* get next service */
			service = axl_node_get_next (service);
			continue;
		} /* end if */

		/* get the service name and write the include */
		name    = axl_node_get_child_nth (service, 0);
		content = axl_node_get_content_trim (name, NULL);

		/* write the initial header */
		to_lower = axl_strdup (comp_name);
		to_lower = xml_rpc_support_to_lower (to_lower);
		xml_rpc_support_write ("#include <%s_%s", to_lower, content);
		axl_free (to_lower);

		/* get the <params> node */
		params = axl_node_get_child_nth (service, 2);
		if (NODE_CMP_NAME (params, "resource"))
			params = axl_node_get_next (params);

		/* write type prefixes */
		xml_rpc_support_write_function_type_prefix (params);
		
		/* write include trail */
		xml_rpc_support_write (".h>\n");

		/* all parameters written, get next node  */
		service = axl_node_get_next (service);

		/* get the next service */
	}

	/* write the last separator */
	xml_rpc_support_write ("\n\n");

	return;
}

/* write service dispatch */
void __xml_rpc_c_server_service_dispach (axlNode * service)
{
	char    * service_name;

	/* alternative method name support */
	axlNode * aux2;
	char    * method_name;
	axlDoc  * doc = axl_node_get_doc (service);
	
	axlNode * name;
	axlNode * params;
	axlNode * param;

	/* service type */
	axlNode * type;
	char    * service_type;

	/* check if the service was already handled */
	if (axl_node_annotate_get_int (service, "service-handled", axl_false))
		return;

	/* get the service name */
	name         = axl_node_get_child_called (service, "name");
	service_name = axl_node_get_content_trim (name, NULL);
	
	/* get the params node */
	params = axl_node_get_child_called (service, "params");
	
	/* check there is a method call defined (alternative
	 * method call name) */
	aux2 = axl_node_get_child_called (service, "method_name");
	if (aux2 == NULL) {
		/* write initial service dispath code */
		xml_rpc_support_write ("if (method_call_is (method_call, \"%s\", %d, ", 
				       service_name, axl_node_get_child_num (params));
	} else {
		/* we have method_name defined */
		method_name = axl_node_get_content_trim (aux2, NULL);
		xml_rpc_support_write ("if (method_call_is (method_call, \"%s\", %d, ", 
				       method_name, axl_node_get_child_num (params));
	}
	
	/* now write the service type especification */
	if (axl_node_have_childs (params)) {
		param = axl_node_get_child_nth (params, 0);
		do {
			/* get the type inside the param */
			type         = axl_node_get_child_nth (param, 1);
			service_type = axl_node_get_content_trim (type, NULL);

			if (axl_cmp (service_type, "int"))
				xml_rpc_support_sl_write ("XML_RPC_INT_VALUE, ");
			else if (axl_cmp (service_type, "bool"))
				xml_rpc_support_sl_write ("XML_RPC_BOOLEAN_VALUE, ");
			else if (axl_cmp (service_type, "string"))
				xml_rpc_support_sl_write ("XML_RPC_STRING_VALUE, ");
			else if (axl_cmp (service_type, "base64"))
				xml_rpc_support_sl_write ("XML_RPC_BASE64_VALUE, ");
			else if (axl_cmp (service_type, "double"))
				xml_rpc_support_sl_write ("XML_RPC_DOUBLE_VALUE, ");
			else if (xml_rpc_c_stub_type_is_array (doc, service_type)) {
				xml_rpc_support_sl_write ("XML_RPC_ARRAY_VALUE, ");
			} else if (xml_rpc_c_stub_type_is_struct (doc, service_type)) { 
				xml_rpc_support_sl_write ("XML_RPC_STRUCT_VALUE, ");
			} 
			
			/* get the next param */
		}while ((param = axl_node_get_next (param)) != NULL);
	} /* if */
	
	/* write support ending */
	xml_rpc_support_sl_write (" -1))\n");
	
	/* push again the indent */
	xml_rpc_support_push_indent ();
	
	/* write service invocator */
	xml_rpc_support_write ("return __%s_%d", service_name, axl_node_get_child_num (params));
	
	/* write type prefixes */
	xml_rpc_support_write_function_type_prefix (params);
	
	/* write invocation termination */
	xml_rpc_support_sl_write (" (method_call, channel);\n\n");
	
	/* pop the indent */
	xml_rpc_support_pop_indent ();

	return;
}

/** 
 * @internal
 * 
 * Writes the default service dispatch function, that recognizes the
 * service and dispath it to the appropiate handler.
 *
 * @param doc The \ref axlDoc reference representing the interface.
 *
 * @param comp_name The XML-RPC server component name.
 */
void xml_rpc_write_c_server_default_service_dispath (axlDoc   * doc, 
						     char     * comp_name,
						     axl_bool   also_body)
{
	/* service location */
	axlNode * service;
	axlNode * node;

	axlHash       * resources;
	axlHashCursor * iter;

	/* write depedency to the xml-rpc implementation */
	xml_rpc_support_write ("/* include xml-rpc library */\n");
	xml_rpc_support_write ("#include <vortex_xml_rpc.h>\n");

	/* write initial header */
	xml_rpc_support_write ("XmlRpcMethodResponse *  service_dispatch (VortexChannel * channel, XmlRpcMethodCall * method_call, axlPointer user_data)%s\n\n",
			       also_body ? "\n{" : ";");

	/* check if the body source code must be generated */
	if (!also_body)
		return;

	/* push the indent */
	xml_rpc_support_push_indent ();
	
	xml_rpc_support_write ("/* check for a match for a service */\n");

	/* check if some service have a resource declaration */
	service   = axl_doc_get (doc, "/xml-rpc-interface/service");
	resources = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	while (service != NULL) {
		
		/* get the resource node */
		node = axl_node_get_child_called (service, "resource");

		if (node != NULL) {
			/* check if the resource was already handled */
			if (! axl_hash_exists (resources, (axlPointer) axl_node_get_content (node, NULL))) {
				/* the resource do not exists, store it */
				axl_hash_insert (resources, 
						 (axlPointer) axl_node_get_content (node, NULL), 
						 (axlPointer) axl_node_get_content (node, NULL));
			} /* end if */
		} /* end if */

		/* get next service */
		service = axl_node_get_next (service);
	} /* end if */

	/* now, for all resources found, create a particular
	 * representation to group all services under the same
	 * resource */
	if (axl_hash_items (resources) > 0) {
		/* create the hash cursor */
		iter = axl_hash_cursor_new (resources);
		while (axl_hash_cursor_has_item (iter)) {
			
			write ("/* handle all services under the resource=%s */\n", (char*) axl_hash_cursor_get_key (iter));
			write ("if (axl_cmp (vortex_xml_rpc_channel_get_resource (channel), \"%s\")) {\n\n", (char*) axl_hash_cursor_get_key (iter));
			push_indent ();

			/* now write all services that have the same resource */
			service   = axl_doc_get (doc, "/xml-rpc-interface/service");
			while (service != NULL) {

				/* get the resource node */
				node = axl_node_get_child_called (service, "resource");
				
				if (node != NULL) {
					/* check if the resource value is the one we are handling */
					if (axl_cmp (axl_node_get_content (node, NULL),
						     (char*) axl_hash_cursor_get_key (iter))) {
						
						/* good, found a service with the same resource, handle it  */
						__xml_rpc_c_server_service_dispach (service);

						/* now flag the service to not be handle in the non-resource section */
						axl_node_annotate_int (service, "service-handled", 1);
					} /* end if */

				} /* end if */

				/* get next service */
				service = axl_node_get_next (service);
				
			} /* end while */

			write ("/* no more services under resource=%s */\n", (char*) axl_hash_cursor_get_key (iter));
			write ("return NULL;\n\n");
			
			pop_indent ();
			write ("} /* end if */\n\n");

			/* get the next resource  */
			axl_hash_cursor_next (iter);

		} /* end while */

		/* free the cursor */
		axl_hash_cursor_free (iter);
		
	} /* end if */

	/* free resources hash */

	axl_hash_free (resources);
	
	/* get a reference to the first service node. */
	service = axl_doc_get (doc, "/xml-rpc-interface/service");
	while (service != NULL) {

		/* check service node */
		if (! (NODE_CMP_NAME (service, "service"))) {
			/* get next service */
			service = axl_node_get_next (service);
			continue;
		} /* end if */

		/* write service dispatch */
		__xml_rpc_c_server_service_dispach (service);

		/* all parameters written, get next node  */
		service = axl_node_get_next_called (service, "service");
	} /* end if */

	/* write the unrecognized method */
	xml_rpc_support_write ("/* return that the method to be invoked, is not supported or recognized by this module */\n");
	xml_rpc_support_write ("return NULL;\n");
	
	/* restore the indent before returning */
	xml_rpc_support_pop_indent ();

	xml_rpc_support_write ("}\n\n");
	
	return;
}


/** 
 * @internal
 *
 * Writes the main function, that starts the xml-rpc listener server.
 * 
 */
void xml_rpc_write_c_server_main_function (void)
{
	/* write the main header */
	xml_rpc_support_write ("int  main (int  argc, char  ** argv)\n{\n\n");
	
	/* push the current indent */
	xml_rpc_support_push_indent ();
	
	/* write several lines */
	xml_rpc_support_multiple_write ("/* init vortex library */\n",
					"vortex_init ();\n\n",
					"/* enable XML-RPC profile */\n",
					"vortex_xml_rpc_accept_negotiation (\n",
					NULL);
	
	/* push again the indent */
	xml_rpc_support_push_indent ();
	
	xml_rpc_support_multiple_write ("/* no resource validation function */\n",
					"NULL,\n",
					"/* no user space data for the validation resource\n",
					"* function. */\n",
					"NULL,\n",
					"service_dispatch,\n",
					"/* no user space data for the dispatch function. */\n",
					"NULL);\n\n",
					NULL);

	/* restore the previous indent */
	xml_rpc_support_pop_indent ();

	xml_rpc_support_multiple_write ("/* parse configuration and start the listener */\n",
					"if (! vortex_xml_rpc_listener_parse_conf_and_start_listeners ())\n",
					NULL);

	/* push the indent for the return */
	xml_rpc_support_push_indent ();

	xml_rpc_support_write ("return -1;\n\n");

	/* pop de indent for the return */
	xml_rpc_support_pop_indent ();
					
	xml_rpc_support_multiple_write ("/* wait for listeners (until vortex_exit is called) */\n",
					"vortex_listener_wait ();\n\n",
					"/* end vortex function */\n",
					"vortex_exit ();\n\n",
					"return 0;\n",
					NULL);

	/* write the last brace */
	xml_rpc_support_pop_indent ();

	xml_rpc_support_write ("}\n");

	return;
}

/** 
 * @internal
 * 
 * Writes the services_dispatch.[hc] file.
 * 
 */
void xml_rpc_c_server_create_services_dispatch_file (axlDoc * doc, 
						     char   * out_dir,
						     char   * comp_name)
{
	/* open the service_dispatch.h file */
	xml_rpc_support_open_file ("%s/service_dispatch.h", out_dir);

	/* write initial header comments */
	xml_rpc_c_server_write_header (comp_name);

	xml_rpc_support_write ("#ifndef __SERVICE_DISPATCH_H__\n");
	xml_rpc_support_write ("#define __SERVICE_DISPATCH_H__\n\n");

	xml_rpc_write_c_server_default_service_dispath (doc, comp_name, axl_false);

	xml_rpc_support_write ("#endif\n");

	xml_rpc_support_close_file ();

	/* open the service_dispatch.c file */
	xml_rpc_support_open_file ("%s/service_dispatch.c", out_dir);

	/* write initial header comments */
	xml_rpc_c_server_write_header (comp_name);

	xml_rpc_write_c_server_include_services (doc, comp_name);
	
	/* now write the default service dispath */
	xml_rpc_write_c_server_default_service_dispath (doc, comp_name, axl_true);

	xml_rpc_support_close_file ();

	return;
}

/** 
 * @internal
 *
 * Creates the main.c file for the provided component name.
 * 
 * @param doc The xml document that represents the interface.
 *
 * @param out_dir The directory where the main.c file will be
 * generated.
 *
 * @param comp_name The component name that the XML-RPC have.
 */
void xml_rpc_c_server_create_main_c (axlDoc * doc, 
				     char   * out_dir, 
				     char   * comp_name)
{
	/* drop a log */
	xml_rpc_report ("generating server stub at: %s..", out_dir);

	/* open the main.c file */
	xml_rpc_support_open_file ("%s/main.c", out_dir);

	/* write initial header comments */
	xml_rpc_c_server_write_header (comp_name);

	/* write all header includes that correspond to services */
	xml_rpc_support_write ("/* include base library */\n");
	xml_rpc_support_write ("#include <vortex.h>\n");
	xml_rpc_support_write ("/* include xml-rpc library */\n");
	xml_rpc_support_write ("#include <vortex_xml_rpc.h>\n");
	xml_rpc_support_write ("#include <service_dispatch.h>\n\n");

	/* write main function */
	xml_rpc_write_c_server_main_function ();

	/* close the file */
	xml_rpc_support_close_file ();

	return;
}

/** 
 * @internal
 * 
 * Writes to the opened file the service header, the service that will
 * execute the service code to be invoked.
 */
void xml_rpc_c_server_write_service_header (char    * service_name, 
					    int       param_count, 
					    char    * return_type, 
					    char    * type_prefix,
					    axlNode * aux,
					    axl_bool  is_header)
{
	axlDoc * doc = axl_node_get_doc (aux);

	/* write service prefix */
	if (axl_cmp (return_type, "int"))
		xml_rpc_support_write ("int ");

	else if (axl_cmp (return_type, "bool"))
		xml_rpc_support_write ("int ");

	else if (axl_cmp (return_type, "double"))
		xml_rpc_support_write ("double ");

	else if (axl_cmp (return_type, "string"))
		xml_rpc_support_write ("char * ");

	else if (axl_cmp (return_type, "base64"))
		xml_rpc_support_write ("char * ");

	else if (xml_rpc_c_stub_type_is_array (doc, return_type) ||
	    xml_rpc_c_stub_type_is_struct (doc, return_type))
		xml_rpc_support_write ("%s * ", return_type);
	    
	/* write the service header implementation */
	xml_rpc_support_write ("%s_%d%s (", service_name, param_count, type_prefix);
	
	/* write service parameters */
	xml_rpc_support_write_function_parameters (doc, aux);
	
	if (axl_node_have_childs (aux))
		xml_rpc_support_sl_write (", ");
	
	xml_rpc_support_sl_write ("char ** fault_error, int * fault_code, VortexChannel * channel)");

	if (is_header)
		xml_rpc_support_sl_write (";");

	return;
}

/** 
 * @internal
 *
 * Declares all struct, array, string, base64 variables that will be
 * required while unmarshalling.
 * 
 * @param params The root node that contains the parameters.
 */
axl_bool      xml_rpc_c_server_write_temporal_parameter_getting (char  * comp_name, axlNode * params)
{
	axlNode * aux;
	axlNode * aux2;
	axlNode * aux3;	
	axlDoc  * doc;

	char    * type;
	char    * type_lower;
	char    * comp_name_lower = NULL;
	char    * name;

	int       iterator;
	axl_bool  first_time = axl_true;

	/* now write the service parameter spec */
	iterator = 0;
	if (axl_node_have_childs (params)) {
		/* get the xml document container */
		doc             = axl_node_get_doc (params);
		comp_name_lower = xml_rpc_support_to_lower (comp_name);
		
		/* the params node have childs, which means the
		 * service have parameters */
		aux2 = axl_node_get_child_nth (params, 0);
		do {
			/* get a reference to the type node */
			aux3 = axl_node_get_child_nth (aux2, 1);
			
			/* get the type */
			type = axl_node_get_content_trim (aux3, NULL);

			/* get a reference to the name */
			aux  = axl_node_get_child_nth (aux2, 0);
			
			/* get the type */
			name = axl_node_get_content_trim (aux, NULL);

			/* check for first time */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type)) {
				if (first_time) {
					xml_rpc_support_write ("/* temporal variable declaration */\n");
					xml_rpc_support_write ("axl_bool  unmarshall_failure = axl_false;\n\n");
					first_time = axl_false;
				}
			}

			
			/* write type found */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type)) {
				/* get lower type name */
				type_lower  = xml_rpc_support_to_lower (type);

				/* check which is the marshall type */
				if (xml_rpc_c_stub_type_is_struct (doc, type)) {
					xml_rpc_support_write ("/* marshall the struct parameter into a native type */\n");
					xml_rpc_support_write ("%s * %s = %s_%s_unmarshall (method_call_get_param_value_as_struct (method_call, %d), axl_false);\n", 
							       type, name, comp_name_lower, type_lower, iterator);
				} else {
					/* it is an array */
					xml_rpc_support_write ("/* marshall the array parameter into a native type */\n");
					xml_rpc_support_write ("%s * %s = %s_%s_unmarshall (method_call_get_param_value_as_array (method_call, %d), axl_false);\n", 
							       type, name, comp_name_lower, type_lower, iterator);
				} /* end if */
				
				/* free type lower */
				axl_free (type_lower);
				
			} /* end if */
			
			/* update the iterator count */
			iterator++;

		}while ((aux2 = axl_node_get_next (aux2)) != NULL);

		/* the params node have childs, which means the
		 * service have parameters */
		aux2 = axl_node_get_child_nth (params, 0);
		do {
			/* get a reference to the type node */
			aux3 = axl_node_get_child_nth (aux2, 1);
			
			/* get the type */
			type = axl_node_get_content_trim (aux3, NULL);

			/* get a reference to the name */
			aux  = axl_node_get_child_nth (aux2, 0);
			
			/* get the type */
			name = axl_node_get_content_trim (aux, NULL);

			/* write type found */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type)) {
				xml_rpc_support_write ("if (%s == NULL)\n", name);
				xml_rpc_support_write ("\tunmarshall_failure = axl_true;\n");
			}
			
			/* update the iterator count */
			iterator++;

		}while ((aux2 = axl_node_get_next (aux2)) != NULL);
	} /* end if */

	/* check for a temporal variable declaration */
	if (! first_time) {
		xml_rpc_support_write ("\n");
	}

	/* free allocated comp name */
	if (comp_name_lower != NULL)
		axl_free (comp_name_lower);

	/* nothing more */
	return (!first_time);
}

/** 
 * @internal
 *
 * Writes all deallocation instructions to release temporal variables.
 * 
 * @param params The root node that contains the parameters.
 */
axl_bool  xml_rpc_c_server_write_temporal_parameter_releasing (char  * comp_name, 
							       axlNode * params)
{
	axlNode * aux;
	axlNode * aux2;
	axlNode * aux3;	
	axlDoc  * doc;

	char    * type;
	char    * type_lower;
	char    * comp_name_lower = NULL;
	char    * name;

	int       iterator;
	axl_bool  first_time = axl_true;
	axl_bool  found      = axl_false;


	/* now write the service parameter spec */
	iterator = 0;
	if (axl_node_have_childs (params)) {
		/* get the xml document container */
		doc             = axl_node_get_doc (params);
		comp_name_lower = xml_rpc_support_to_lower (comp_name);
		
		/* the params node have childs, which means the
		 * service have parameters */
		aux2 = axl_node_get_child_nth (params, 0);
		do {
			/* get a reference to the type node */
			aux3 = axl_node_get_child_nth (aux2, 1);
			
			/* get the type */
			type = axl_node_get_content_trim (aux3, NULL);

			/* get a reference to the name */
			aux  = axl_node_get_child_nth (aux2, 0);
			
			/* get the type */
			name = axl_node_get_content_trim (aux, NULL);

			/* check for first time */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type)) {
				if (first_time) {
					xml_rpc_support_write ("/* temporal variable deallocation */\n");
					first_time = axl_false;
				}
			}

			
			/* write type found */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type)) {
				/* flag that an struct or array was found */
				found = axl_true;

				/* get lower type name */
				type_lower  = xml_rpc_support_to_lower (type);
				
				xml_rpc_support_write ("%s_%s_free (%s);\n", 
						       comp_name_lower, type_lower, name);
				
				/* free type lower */
				axl_free (type_lower);
			}
			
			/* update the iterator count */
			iterator++;

		}while ((aux2 = axl_node_get_next (aux2)) != NULL);
	}

	/* check for a temporal variable declaration */
	if (! first_time) {
		xml_rpc_support_write ("\n");
	}

	/* free allocated comp name */
	if (comp_name_lower != NULL)
		axl_free (comp_name_lower);

	/* nothing more */
	return found;
}

/** 
 * @internal
 * 
 * Writes the parameter getting sentences required to get from the
 * XmlRpcMethodCall, all values inside, to be passed in to the user
 * implementation.
 * 
 * @param params An axlNode pointing to the <params> node.
 */
void xml_rpc_c_server_write_parameter_getting (axlNode * params)
{
	axlNode * aux;
	axlNode * aux2;
	axlNode * aux3;	
	axlDoc  * doc;

	char    * type;
	char    * name;

	int       iterator;


	/* now write the service parameter spec */
	iterator = 0;
	if (axl_node_have_childs (params)) {

		/* get the xml document container */
		doc = axl_node_get_doc (params);
		
		/* the params node have childs, which means the
		 * service have parameters */
		aux2 = axl_node_get_child_nth (params, 0);
		do {
			/* get a reference to the type node */
			aux3 = axl_node_get_child_nth (aux2, 1);
			
			/* get the type */
			type = axl_node_get_content_trim (aux3, NULL);

			/* get a reference to the name node */
			aux  = axl_node_get_child_nth (aux2, 0);
			
			/* get the type */
			name = axl_node_get_content_trim (aux, NULL);
			
			/* write type found */
			if (xml_rpc_c_stub_type_is_struct (doc, type) ||
			    xml_rpc_c_stub_type_is_array (doc, type))
				xml_rpc_support_sl_write ("%s, ", name);
			else if (axl_cmp (type, "bool")) 
				xml_rpc_support_sl_write ("method_call_get_param_value_as_int (method_call, %d), ",
							  iterator);
			else
				xml_rpc_support_sl_write ("method_call_get_param_value_as_%s (method_call, %d), ",
							  type, iterator);

			/* update the iterator count */
			iterator++;

		}while ((aux2 = axl_node_get_next (aux2)) != NULL);
	}

	/* nothing more */
	return;
}

/** 
 * @internal
 *
 * Write the service file, that implements the service unmarshaller
 * and the service implementation.
 * 
 * @param service The service node that contains all spec information.
 *
 * @param out_dir The directory where the service generation will be
 * placed.
 *
 * @param comp_name The component name, that is, the XML-RPC
 * component.
 */
void xml_rpc_c_server_create_write_service (axlNode * service, 
					    char    * out_dir, 
					    char    * comp_name)
{
	char    * service_name;
	char    * return_type;
	char    * return_type_lower;

	int       param_count;

	axlNode * aux;
	axlDoc  * doc;

	axlNode    * code;
	axlNode    * body;
	char       * service_content;

	char    * type_prefix;

	char    * comp_name_upper;
	char    * comp_name_lower;
	char    * service_name_upper;

	/* get service name */
	aux          = axl_node_get_child_nth (service, 0);
	service_name = axl_node_get_content_trim (aux, NULL);
	doc          = axl_node_get_doc (service);

	/* get upper values */
	comp_name_upper    = xml_rpc_support_to_upper (comp_name);
	comp_name_lower    = xml_rpc_support_to_lower (comp_name);
	service_name_upper = xml_rpc_support_to_upper (service_name);
	
	/* get return type */
	aux               = axl_node_get_next (aux);
	return_type       = axl_node_get_content_trim (aux, NULL);
	return_type_lower = xml_rpc_support_to_lower (return_type);

	/* check if the service is inside a resource or next node is
	 * the params */
	aux          = axl_node_get_next (aux);
	if (NODE_CMP_NAME (aux, "resource")) {
		/* get the reference to the params node */
		aux  = axl_node_get_next (aux);
	}
	
	/* open the service implementation file */
	type_prefix  = xml_rpc_support_get_function_type_prefix (aux);

	param_count  = axl_node_get_child_num (aux);

	/* open the file */
	xml_rpc_support_open_file ("%s/%s_%s%s.h", 
				   out_dir, 
				   comp_name_lower, 
				   service_name, 
				   (type_prefix != NULL) ? type_prefix : "");

	/* write initial header */
	xml_rpc_c_server_write_header (comp_name);

	xml_rpc_support_write ("#ifndef __SERVER_%s_%s_XML_RPC_H__\n",
			       comp_name_upper, service_name_upper);
	
	xml_rpc_support_write ("#define __SERVER_%s_%s_XML_RPC_H__\n\n",
			       comp_name_upper, service_name_upper);

	xml_rpc_support_write ("/* include base library */\n");
	xml_rpc_support_write ("#include <vortex.h>\n");
	xml_rpc_support_write ("/* include xml-rpc library */\n");
	xml_rpc_support_write ("#include <vortex_xml_rpc.h>\n");
	xml_rpc_support_write ("#include <%s_types.h>\n\n",
			       comp_name_lower);

	/* write the service header definition */
	xml_rpc_c_server_write_service_header (service_name, param_count, return_type, type_prefix,
					       aux, axl_true);
	xml_rpc_support_write ("\n\n");

	/* write the service unmarshaller definition */
	xml_rpc_support_write ("XmlRpcMethodResponse * __%s_%d%s (XmlRpcMethodCall * method_call, VortexChannel * channel);\n\n",
			       service_name, param_count, type_prefix);
	
	xml_rpc_support_write ("#endif\n");
	
	/* close the file */
	xml_rpc_support_close_file ();

	/* now write the service body implementation */
	xml_rpc_support_open_file ("%s/%s_%s%s.c", 
				   out_dir, 
				   comp_name_lower, 
				   service_name, 
				   (type_prefix != NULL) ? type_prefix : "");
	/* write initial header */
	xml_rpc_c_server_write_header (comp_name);

	xml_rpc_support_write ("/* include base library */\n");
	xml_rpc_support_write ("#include <vortex.h>\n");
	xml_rpc_support_write ("/* include xml-rpc library */\n");
	xml_rpc_support_write ("#include <vortex_xml_rpc.h>\n");
	xml_rpc_support_write ("#include <%s_types.h>\n\n",
			       comp_name_lower);

	/* include additional content if found */
	body = axl_node_get_child_called (service, "options");
	if (body != NULL) {
		body = axl_node_get_child_called (body, "body");
		if (body != NULL) {
			/* flag where the content was extract from */
			xml_rpc_support_write ("/* code included from: %s */\n", ATTR_VALUE (body, "from"));
			
			/* get content and translate all definitions */
			service_content = axl_node_get_content_trans (body, NULL);

			xml_rpc_support_write ("%s\n", service_content);

			/* release */
			axl_free (service_content);
		}
	} /* end if */

	/* write the user service, in the mean time, empty */
	xml_rpc_c_server_write_service_header (service_name, param_count, return_type, type_prefix,
					       aux, axl_false);
	xml_rpc_support_write ("\n{\n");
	
	/* push the indent to write the empty body */
	xml_rpc_support_push_indent ();

	/* check if the user have defined a service implementation, if
	 * not, put the WRITE HERE notice */

	/* get a reference to the optional code node */
	code = axl_node_get_next (aux);
	if (NODE_CMP_NAME (code, "code")) {
		/* get a reference to the user service code */
		code            = axl_node_get_child_nth (code, 0);
		service_content = axl_node_get_content_trans (code, NULL);

		/* write the user code */
		xml_rpc_support_write (service_content);

		/* release content */
		axl_free (service_content);
	}else {
		xml_rpc_support_write ("/* WRITE HERE YOUR CODE */\n");
		xml_rpc_support_write ("REPLY_FAULT (\"Service is not implemented yet.\", -1, ");

		if (axl_cmp (return_type, "int") || 
		    axl_cmp (return_type, "double") || 
		    axl_cmp (return_type, "bool"))
			xml_rpc_support_sl_write ("0");
	
		if (axl_cmp (return_type, "string") || 
		    axl_cmp (return_type, "struct") || 
		    axl_cmp (return_type, "array") || 
		    axl_cmp (return_type, "base64"))
			xml_rpc_support_sl_write ("NULL");
		
		xml_rpc_support_sl_write (");\n");
	}

	/* restore the indent */
	xml_rpc_support_pop_indent ();

	xml_rpc_support_write ("\n}\n\n");

	/* write the service unmarshaller definition */
	xml_rpc_support_write ("/* This is a support function to invoke \'%s\' service , do not modify it!! */\n",
			       service_name);
	xml_rpc_support_write ("XmlRpcMethodResponse * __%s_%d%s (XmlRpcMethodCall * method_call, VortexChannel * channel)\n{\n",
			       service_name, param_count, type_prefix);

	/* push the indent to write the service body */
	xml_rpc_support_push_indent ();

	/* write variable declarations */
	xml_rpc_support_multiple_write ("/* error support variables */\n",
					"VortexCtx * ctx         = METHOD_CALL_CTX(method_call);\n",
					"char      * fault_error = NULL;\n",
					"int         fault_code  = -1;\n",
					NULL);

	/* write the variable that will hold the value */
	if (axl_cmp (return_type, "int"))
		xml_rpc_support_write ("int ");

	if (axl_cmp (return_type, "double"))
		xml_rpc_support_write ("double ");

	if (axl_cmp (return_type, "bool"))
		xml_rpc_support_write ("int ");

	if (axl_cmp (return_type, "string"))
		xml_rpc_support_write ("char * ");

	if (axl_cmp (return_type, "base64"))
		xml_rpc_support_write ("char * ");

	if (xml_rpc_c_stub_type_is_struct (doc, return_type) ||
	    xml_rpc_c_stub_type_is_array (doc, return_type)) {
		xml_rpc_support_write ("%s * ", return_type);
	}

	if (axl_cmp (return_type, "string") ||
	    axl_cmp (return_type, "base64") ||
	    xml_rpc_c_stub_type_is_struct (doc, return_type) ||
	    xml_rpc_c_stub_type_is_array (doc, return_type))
		xml_rpc_support_sl_write ("   result = NULL;\n\n");
	else if (axl_cmp (return_type, "int"))
		xml_rpc_support_sl_write ("   result = -1;\n\n");
	else if (axl_cmp (return_type, "double"))
		xml_rpc_support_sl_write ("   result = 0;\n\n");
	else if (axl_cmp (return_type, "bool"))
		xml_rpc_support_sl_write ("   result = axl_false;\n\n");
	else
		xml_rpc_support_sl_write ("   result;\n\n");

	if (xml_rpc_c_stub_type_is_array (doc, return_type))
		xml_rpc_support_write ("XmlRpcArray * _result;\n");
	else if (xml_rpc_c_stub_type_is_struct (doc, return_type))
		xml_rpc_support_write ("XmlRpcStruct * _result;\n");

	/* check for variables to be declared and write them */
	if (xml_rpc_c_server_write_temporal_parameter_getting (comp_name, aux)) {
		/* write the invocation */
		xml_rpc_support_write ("/* call to the user implementation */\n");
		xml_rpc_support_write ("if (! unmarshall_failure)\n");
		xml_rpc_support_write ("\tresult = %s_%d%s (", service_name, param_count, type_prefix);
	} else {
		/* write the invocation */
		xml_rpc_support_write ("/* call to the user implementation */\n");
		xml_rpc_support_write ("result = %s_%d%s (", service_name, param_count, type_prefix);
	}

	/* write parameters */
	xml_rpc_c_server_write_parameter_getting (aux);

	xml_rpc_support_sl_write (" &fault_error, &fault_code, channel);\n\n");

	/* check for temporal variables to be released */
	if (xml_rpc_c_server_write_temporal_parameter_releasing (comp_name, aux)) {

		/* because the function have reported "true",
		 * signaling that there are parameters that are
		 * marshalled, place an additional restriction */
		xml_rpc_support_write ("if (unmarshall_failure) {\n");
		xml_rpc_support_push_indent ();
		xml_rpc_support_write ("return CREATE_FAULT_REPLY (-1, \"Found marshalling error. Unable to translate XML-RPC structures to native types\");\n");
		xml_rpc_support_pop_indent ();
		xml_rpc_support_write ("} /* end if */\n\n");
	} /* end if */

	/* write result checking for a negative reply */
	xml_rpc_support_write ("/* check error reply looking at the fault_error */\n");
	
	xml_rpc_support_write ("if (fault_error != NULL) {\n");

	xml_rpc_support_push_indent ();
	xml_rpc_support_multiple_write ("/* we have a error reply */\n",
					"return CREATE_FAULT_REPLY (fault_code, fault_error);\n",
					NULL);
	xml_rpc_support_pop_indent ();

	xml_rpc_support_write ("}\n\n");

	/* write additionall checking for null pointers */
	if (xml_rpc_c_stub_type_is_array (doc, return_type) ||
	    xml_rpc_c_stub_type_is_struct (doc, return_type) ||
	    axl_cmp (return_type, "string") || 
	    axl_cmp (return_type, "base64")) {
		xml_rpc_support_write ("if (result == NULL) {\n");
		
		xml_rpc_support_push_indent ();
		xml_rpc_support_multiple_write ("/* we have a error reply */\n",
						"return CREATE_FAULT_REPLY (-2, \"Returned a NULL value from a service which returns a pointer type. This is not allowed.\");\n",
						NULL);
		xml_rpc_support_pop_indent ();
		
		xml_rpc_support_write ("}\n\n");
	}


	/* write unmarshalling code for array and structs */
	if (xml_rpc_c_stub_type_is_array (doc, return_type) ||
	    xml_rpc_c_stub_type_is_struct (doc, return_type)) {
		xml_rpc_support_write ("/* Translate structure returned by the service */\n");
		xml_rpc_support_write ("_result = %s_%s_marshall (ctx, result, axl_true);\n\n",
				       comp_name_lower, return_type_lower);
	}

	xml_rpc_support_multiple_write ("/* return reply generated */\n",
					"return CREATE_OK_REPLY (ctx, ", 
					NULL);

	/* write the variable that will hold the value */
	if (axl_cmp (return_type, "int"))
		xml_rpc_support_sl_write ("XML_RPC_INT_VALUE, INT_TO_PTR (result));\n");

	if (axl_cmp (return_type, "double"))
		xml_rpc_support_sl_write ("XML_RPC_DOUBLE_VALUE, &result);\n");

	if (axl_cmp (return_type, "bool"))
		xml_rpc_support_sl_write ("XML_RPC_BOOLEAN_VALUE, INT_TO_PTR (result));\n");

	if (axl_cmp (return_type, "string"))
		xml_rpc_support_sl_write ("XML_RPC_STRING_REF_VALUE, result);\n");

	if (axl_cmp (return_type, "base64"))
		xml_rpc_support_sl_write ("XML_RPC_BASE64_REF_VALUE, result);\n");

	if (xml_rpc_c_stub_type_is_struct (doc, return_type))
		xml_rpc_support_sl_write ("XML_RPC_STRUCT_VALUE, _result);\n");

	if (xml_rpc_c_stub_type_is_array (doc, return_type))
		xml_rpc_support_sl_write ("XML_RPC_ARRAY_VALUE, _result);\n");
	

	/* restore the indent and close the function */
	xml_rpc_support_pop_indent ();
	xml_rpc_support_write ("}\n");

	/* close the file */
	xml_rpc_support_close_file ();

	/* release memory used by allocated variables */
	axl_free (comp_name_lower);
	axl_free (comp_name_upper);
	axl_free (service_name_upper);
	axl_free (return_type_lower);

	/* write the service unmarshaller */
	return;
}


/** 
 * @internal
 *
 * Write a module for every service exported.
 */
void xml_rpc_c_server_create_writes_services (axlDoc * doc, 
					      char   * result, 
					      char   * comp_name)
{
	axlNode * service;

	
	/* get the first service received */
	service = axl_doc_get (doc, "/xml-rpc-interface/service");
	while (service != NULL) {
		
		/* check we got a service node */
		if (! (NODE_CMP_NAME (service, "service"))) {
			/* get the next service node */
			service = axl_node_get_next (service);

			continue;
		} /* end if */

		/* write the service */
		xml_rpc_c_server_create_write_service (service, result, comp_name);

		/* get the next service */
		service = axl_node_get_next (service);
		
	} /* end while */

	/* all services files writen */
	return;
}

/** 
 * @internal
 * 
 * Creates the default xml configuration for the server stub being
 * created. It contains default startup configuration such host and
 * port.
 */
void xml_rpc_c_server_create_default_xml_config (char  * out_dir)
{
	/* do not produce the conf.xml file if the main file is
	 * disabled */
	if (exarg_is_defined ("disable-main-file")) 
		return;

	/* open the xml configuration file */
	xml_rpc_support_open_file ("%s/conf.xml", out_dir);

	/* write default configuration */
	xml_rpc_support_multiple_write ("<vortex-listener>\n",
					"  <listener>\n",
					"     <hostname>0.0.0.0</hostname>\n",
					"     <port>44000</port>\n",
					"  </listener>\n",
					"</vortex-listener>\n", NULL);
	
	/* close the configuration file */
	xml_rpc_support_close_file ();
	
}

/** 
 * @internal
 *
 * Produces the server stub implementation, filling the services with
 * the optional code provided.
 * 
 * @param doc The \ref axlDoc reference where the xml document is
 * located.
 *
 * @param out_dir An output directory where the server stub will be
 * created.
 *
 * @param comp_name The XML-RPC component name.
 */
void xml_rpc_c_server_create (axlDoc * doc, char   * out_dir, 
			      char   * comp_name)
{
	char    * result;

	/* create the the server stub directory */
	if (exarg_is_defined ("out-server-dir")) 
		result = xml_rpc_support_create_dir (exarg_get_string ("out-server-dir"));
	else
		result = xml_rpc_support_create_dir ("%s/server-%s", out_dir, comp_name);

	/* create the main.c file */
	if (! exarg_is_defined ("disable-main-file"))
		xml_rpc_c_server_create_main_c (doc, result, comp_name);

	/* create the services_dispatch file */
	xml_rpc_c_server_create_services_dispatch_file (doc, result, comp_name);

	/* write all services */
	xml_rpc_c_server_create_writes_services (doc, result, comp_name);

	/* write xml configuration file */
	xml_rpc_c_server_create_default_xml_config (result);

	/* write all struct and array definitions */
	xml_rpc_c_stub_write_all_struct_and_array_defs (doc, comp_name, result);

	/* write the type header */
	xml_rpc_c_stub_write_type_header (result, comp_name, doc);

	/* free the directory reference */
	axl_free (result);
	
	return;
}

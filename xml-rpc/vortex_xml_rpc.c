/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
/* include local header */
#include <vortex_xml_rpc.h>

/* include inline dtd */
#include <xml-rpc-boot.dtd.h>

#define LOG_DOMAIN "vortex-xml-rpc"

/** 
 * @internal Macro definition to access to service dispatch list.
 */
#define VORTEX_XML_RPC_SERVICE_DISPATCH "vo:xml-rpc:sd"

/**
 * @internal Macro definition to access to the xml-rpc boot dtd.
 */
#define VORTEX_XML_RPC_BOOT_DTD         "vo:xml-rpc:dtd"

/** 
 * @internal The following data definition is used to hold one service
 * dispatch node and its associated data, along with the validation
 * handler.
 */
typedef struct _VortexXmlRpcServiceDispatchNode {
	/** 
	 * @internal The dispatch handler stored.
	 */
	VortexXmlRpcServiceDispatch   dispatch;
	/** 
	 * @internal User space data associated to the dispatch
	 * handler.
	 */
	axlPointer                    dispatch_data;

	/** 
	 * @internal The validate handler.
	 */
	VortexXmlRpcValidateResource  validate;
	/** 
	 * @internal The data associated to the validation handler.
	 */
	axlPointer                    validate_data;
} VortexXmlRpcServiceDispatchNode;

/**
 * \defgroup vortex_xml_rpc Vortex XML-RPC: XML-RPC profile support and related functions
 */

/* \addtogroup vortex_xml_rpc */
/* @{ */

/** 
 * @internal
 * @brief Support user space notification for XML-RPC boot status notification.
 * 
 * @param channel    The channel created due to the boot process or NULL if fails.
 * @param status     A process status
 * @param message    The message to report. A textual diagnostic.
 * @param user_data  User space data.
 */
void __vortex_xml_rpc_notify (VortexXmlRpcBootNotify    process_status, 
			      VortexChannel           * channel,
			      VortexStatus              status,
			      char                    * message, 
			      axlPointer                user_data)
{
	/* get context */
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = vortex_channel_get_ctx (channel);
#endif

	/* drop to the console a log */
	switch (status) {
	default:
	case VortexError:
		vortex_log (VORTEX_LEVEL_CRITICAL, (message != NULL) ? message : "no message to report");

		/* close the channel if an error have happened while
		 * booting the XML-RPC channel. */
		if (channel != NULL)
			vortex_channel_close (channel, NULL);
		break;
	case VortexOk:
		vortex_log (VORTEX_LEVEL_DEBUG, (message != NULL) ? message : "no message to report");

		/* set the channel to be ready. */
		vortex_channel_set_data (channel, XML_RPC_BOOT_STATE, "ready");
		break;
	}

	/* notify user space that something to be notify have
	 * happen */
	if (process_status != NULL)
		process_status (channel, status, message, user_data);

	/* that's all man */
	return;
}

/** 
 * @internal 
 *
 * @brief Support function to perform user space notification with
 * operation status for xml-rpc invocations.
 *
 * This function is mainly used by the following functions:
 *
 *  - \ref vortex_xml_rpc_invoke
 *  - \ref vortex_xml_rpc_invoke_sync
 *
 * What the function do is to notify to user space code with the
 * result and, according to the results, drop a log.
 *
 * The function also creates a method response object containing the
 * error message if an error is detected.
 * 
 * @param notify The \ref XmlRpcInvokeNotify handler.
 * 
 * @param status The status to report.
 *
 * @param status_message The status message to report.
 *
 * @param channel The channel where the invocation was performed.
 *
 * @param response The response object get from the remote side or
 * NULL if no response object was generated. In this case
 * <i>status</i> variable should be checked to generate a new \ref
 * XmlRpcMethodResponse object reporting the failure.
 *
 * @param user_data User space data.
 */
void __vortex_xml_rpc_notify_response (XmlRpcInvokeNotify     notify,
				       XmlRpcResponseStatus   status,
				       int                    fault_code,
				       char                 * fault_message, 
				       VortexChannel        * channel,
				       XmlRpcMethodResponse * response,
				       axlPointer             user_data)
{
	/* get context */
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = vortex_channel_get_ctx (channel);
#endif

	switch (status) {
	case XML_RPC_OK:
		/* ok reply received */
		vortex_log (VORTEX_LEVEL_DEBUG, "invocation reply received!");
		break;
	default:
		vortex_log (VORTEX_LEVEL_WARNING, "invocation failed, message received: (code: %d) %s", 
		       fault_code, fault_message);
		/* bad reply received */
		if (response == NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "building method response stub containing the textual error");
			/* create a XmlRpcMethodResponse object */
			response = method_response_new (status, fault_code, fault_message, NULL);
		}
		break;
		
	}
	/* notify user space about response received */
	if (notify != NULL)
		notify (channel, response, user_data);

	return;
}
				       

/** 
 * @internal
 *
 * Internal function to support parsing xml content received,
 * representing an struct construction. This function is called from
 * __vortex_xml_rpc_parse_value.
 * 
 * @param child The node received represents the xml node reference
 * pointing to the <struct>.
 * 
 * @return A newly allocated method value, containing a representation
 * of a struct or NULL if fails.
 */
XmlRpcMethodValue * __vortex_xml_rpc_parse_struct_value (VortexCtx * ctx, axlNode * struct_node)
{
	/* the variable which will hold the result */
	const char         * member_name;
	XmlRpcMethodValue  * member_value;
	XmlRpcStruct       * _struct;
	XmlRpcStructMember * member;
	

	/* some node references */
	axlNode            * name;
	axlNode            * value;
	axlNode            * node_member;


	/* some iterators */
	int                  iterator;
	int                  length;

	/* check the struct node name */
	if (! NODE_CMP_NAME (struct_node, "struct")) {
		/* it seems we have received a node reference which is
		   not an struct */
		return NULL; 
	} 

	/* get the count number of childs inside the struct */
	length   = axl_node_get_child_num (struct_node);

	if (length == 0) {
		/* it seems we have received an struct with no
		 * members */
		return NULL;
	}

	/* create the struct */
	_struct = vortex_xml_rpc_struct_new (length);

	/* iterate over all childs (<member> items) inside the
	 * <struct> received */
	iterator = 0;
	while (iterator < length) {
		/* get a referenced to the member node */
		node_member = axl_node_get_child_nth (struct_node, iterator);
		
		/* get the member name */
		name         = axl_node_get_child_nth (node_member, 0);
		member_name  = axl_node_get_content (name, NULL);

		vortex_log (VORTEX_LEVEL_DEBUG, "parsing struct member name=%s",
			    member_name);
		
		/* get the member content */
		value        = axl_node_get_child_nth (node_member, 1);
		member_value = __vortex_xml_rpc_parse_value (ctx, value);

		/* create the member and add it to the struct */
		member = vortex_xml_rpc_struct_member_new (member_name, member_value);
		vortex_xml_rpc_struct_add_member (_struct, member);
		
		/* next member to parse */
		iterator++;
	}

	/* return the structure parsed */
	return method_value_new (ctx, XML_RPC_STRUCT_VALUE, _struct);
}

/** 
 * @internal
 *
 * Parses a received xml stream representing an xml rpc array.
 *
 * @param array_node The node pointing to the first node <array>.
 * 
 * @return A newly allocated xml-rpc array.
 */
XmlRpcMethodValue * __vortex_xml_rpc_parse_array_value (VortexCtx * ctx, axlNode * array_node)
{
	/* the variable which will hold the result */
	XmlRpcMethodValue  * method_value;
	XmlRpcArray        * array;
	
	/* some node references */
	axlNode            * value  = NULL;
	axlNode            * data   = NULL;

	/* some iterators */
	int                  iterator;
	int                  length;

	/* check the struct node name */
	if (! NODE_CMP_NAME (array_node, "array")) {
		/* it seems we have received a node reference which is
		   not an struct */
		return NULL; 
	} 

	/* get a reference to the child node */
	data = axl_node_get_child_nth (array_node, 0);

	/* check the struct node name */
	if (! NODE_CMP_NAME (data, "data")) {
		/* it seems we have received a node reference which is
		   not an struct */
		return NULL; 
	} 

	/* get the count number of childs inside the array (<data> node) */
	length   = axl_node_get_child_num (data);

	/* create the struct */
	vortex_log (VORTEX_LEVEL_DEBUG, "creating xml rpc array with %d items", length);
	array = vortex_xml_rpc_array_new (length);

	/* iterate over all childs inside the <data> node */
	iterator = 0;
	while (iterator < length) {

		if (iterator == 0) {
			/* get the first value inside the <data> node */
			value = axl_node_get_child_nth (data, iterator);
		}else {
			/* get the next element in the sequence */
			value = axl_node_get_next (value);
		}

		/* parse the value node */
		method_value = __vortex_xml_rpc_parse_value (ctx, value);

		/* create the member and add it to the struct */
		vortex_xml_rpc_array_add (array, method_value);

		/* next member to parse */
		iterator++;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "returning an array with (%d) items", 
		    vortex_xml_rpc_array_count (array));

	/* return the structure parsed */
	return method_value_new (ctx, XML_RPC_ARRAY_VALUE, array);
}

/** 
 * @internal 
 *
 * @brief Parse the given chunk of xml data, supposing that it points
 * to ta <value /> node.
 *
 * The function returns an allocated \ref XmlRpcMethodValue
 * representing the value that the XML chunk contains.
 * 
 * @param cursor The cursor representing the value to be read.
 * 
 * @return A newly allocated \ref XmlRpcMethodValue.
 */
XmlRpcMethodValue * __vortex_xml_rpc_parse_value (VortexCtx * ctx, axlNode * value)
{
	const char        * string_value = NULL;
	XmlRpcMethodValue * result       = NULL;
	axlNode           * child        = NULL;

	v_return_val_if_fail (value, NULL);

	/* check that the received cursor contains a <value/> node */
	if (! NODE_CMP_NAME (value, "value")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "received a wrong top level node where expected <value />. Received <%s>.", 
		       axl_node_get_name (value));
		return NULL;		
	}
	
	/* get children node */
	child = axl_node_get_child_nth (value, 0);
	vortex_log (VORTEX_LEVEL_DEBUG, "received a value tag, containing a <%s>", 
	       axl_node_get_name (child));

	/* check for the none value */
	if (NODE_CMP_NAME (child, "none")) {
		vortex_log (VORTEX_LEVEL_DEBUG, "none value received");
		return method_value_new (ctx, XML_RPC_NONE_VALUE, NULL);
	}

	/* in the case of simple top level values, get the reference
	 * to the internal content */
	if (NODE_CMP_NAME (child, "i4") ||
	    NODE_CMP_NAME (child, "int") ||
	    NODE_CMP_NAME (child, "boolean") ||
	    NODE_CMP_NAME (child, "string") ||
	    NODE_CMP_NAME (child, "double") ||
	    NODE_CMP_NAME (child, "dateTime.iso8601") ||
	    NODE_CMP_NAME (child, "base64")) {

		/* reference to the internal content for simple,
		 * non-composite values. */
		string_value = axl_node_get_content (child, NULL);
		v_return_val_if_fail (string_value, NULL);

		vortex_log (VORTEX_LEVEL_DEBUG, "received simple value='%s'", 
			    string_value ? string_value : "NULL");

		/* for each value tag, convert into XML-RPC parameter */
		if (NODE_CMP_NAME (child, "i4") || NODE_CMP_NAME (child, "int"))
			result        = method_value_new_from_string (ctx, XML_RPC_INT_VALUE, string_value);
		
		/* boolean case */
		if (NODE_CMP_NAME (child, "boolean"))
			result        = method_value_new_from_string (ctx, XML_RPC_BOOLEAN_VALUE, string_value);
		
		/* string case */
		if (NODE_CMP_NAME (child, "string"))
			result        = method_value_new_from_string (ctx, XML_RPC_STRING_VALUE, string_value);
		       
		/* double case */
		if (NODE_CMP_NAME (child, "double"))
			result        = method_value_new_from_string (ctx, XML_RPC_DOUBLE_VALUE, string_value);
		
		/* dateTime.iso8601 */
		if (NODE_CMP_NAME (child, "dateTime.iso8601"))
			result        = method_value_new_from_string (ctx, XML_RPC_DATE_VALUE, string_value);
		
		/* base64 case */
		if (NODE_CMP_NAME (child, "base64"))
			result        = method_value_new_from_string (ctx, XML_RPC_BASE64_VALUE, string_value);
		
		/* free and return */
		return result;
	}
	
	/* struct case */
	if (NODE_CMP_NAME (child, "struct")) {
		vortex_log (VORTEX_LEVEL_DEBUG, "received a complex value <struct>");
		
		/* parse the struct value */
		result = __vortex_xml_rpc_parse_struct_value (ctx, child);
		
		vortex_log (VORTEX_LEVEL_DEBUG, "struct parsed: type %d", 
			    method_value_get_type (result));

	}else if (NODE_CMP_NAME (child, "array")) {
		/* array case */
		vortex_log (VORTEX_LEVEL_DEBUG, "received a complex value <array> (2)");
		
		/* parse the array value */
		result = __vortex_xml_rpc_parse_array_value (ctx, child);

		vortex_log (VORTEX_LEVEL_DEBUG, "array parsed: type %d", 
			    method_value_get_type (result));
	}
	
	/* release memory no longer used */
	return result;
}


typedef struct _VortexXmlRpcBootData {
	VortexConnection        * connection;
	char                    * serverName;
	char                    * resourceName;
	VortexXmlRpcBootNotify    process_status;
	axlPointer                user_data;
}VortexXmlRpcBootData;

/** 
 * @internal
 *
 * @brief Support function for \ref vortex_xml_rpc_boot_channel, which
 * actually performs the work.
 * 
 * @param data An \ref VortexXmlRpcBootData node;
 * 
 * @return NULL, new channel created will be notified through
 * <i>process_status</i> handler.
 */
axlPointer __vortex_xml_rpc_boot_channel_process (VortexXmlRpcBootData * data)
{
	VortexConnection        * connection      = data->connection;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx               * ctx             = vortex_connection_get_ctx (connection);
#endif
	char                    * serverName      = data->serverName;
	char                    * resourceName    = data->resourceName;
	VortexXmlRpcBootNotify    process_status  = data->process_status;
	VortexChannel           * channel;
	axlPointer                user_data       = data->user_data;
	VortexAsyncQueue        * queue;
	VortexFrame             * reply;
	char                    * msg;
	int                       code;
	char                    * string_aux;
	
	/* unref no longer needed data */
	axl_free (data);

	/* create the channel piggybacking initial data (the bootmsg
	 * token) */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new_fullv (connection,   /* the connection where the channel will be created */
					    0,            /* let Vortex Library to choose the next channel num ready */
					    serverName,   /* serverName value to request virtual hosting */
					    VORTEX_XML_RPC_PROFILE, /* the BEEP unique URI profile */
					    EncodingNone, /* no profile content encoding used */
					    NULL, NULL,   /* default close channel handling */
					    vortex_channel_queue_reply, queue,   /* set frame receiving handling using the queue */
					    NULL, NULL,   /* perform channel creation in a blocking fashion */
					    "<bootmsg resource='%s' />",
					    (resourceName != NULL) ? resourceName : "/");

	/* record resource */
	vortex_channel_set_data_full (channel, 
				      /* key */
				      XML_RPC_RESOURCE, resourceName,
				      /* value */
				      NULL, axl_free);


	/* free no longer needed data */
	axl_free (serverName);
	
	/* check for the channel returned. */
	if (channel == NULL) {
		/* unref queue */
		vortex_async_queue_unref (queue);

		/* unable to create channel, notify user space */
		if (vortex_connection_pop_channel_error (connection, &code, &string_aux)) {
			/* build the message */
			msg = axl_strdup_printf ("Unable to create a channel under the profile XML-RPC: %d %s", code, string_aux);
			axl_free (string_aux);
		} else {
			msg = axl_strdup ("Unable to create a channel under the profile XML-RPC");
		}

		/* notify error message, dealloc and return null reference */
		__vortex_xml_rpc_notify (process_status, NULL, VortexError, msg,  user_data);

		/* set the message to be deallocated once closed the connection */
		vortex_connection_set_data_full (connection, "vo:xml-rpc:error", msg, NULL, axl_free);

		return NULL;
	}

	/* the next frame from listener peer (it doesn't matter to get
	 * blocked this way). The following source code will support
	 * not only piggybacking but also the first frame received on
	 * this channel */
	reply = vortex_channel_get_reply (channel, queue);

	/* unref queue */
	vortex_async_queue_unref (queue);

	if (reply == NULL) {
		/* unable to create channel, notify user space */
		__vortex_xml_rpc_notify (process_status, NULL, VortexError,
					 "Timeout reached while waiting for the first reply for the profile XML-RPC",  user_data);
		return NULL;
	}

	/* now we have a reply check for errors */
	if (vortex_frame_is_error_message (reply, NULL, &msg)) {
		/* notify error while booting XML-RPC channel */
		__vortex_xml_rpc_notify (process_status, channel, VortexError, msg, user_data);

		/* nullify the channel */
		channel = NULL;

		goto finish_boot_xml_rpc_channel;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "checking resource validation, received: %s",
		    vortex_frame_get_payload (reply));

	/* seems that the remote peer have accepted to create the
	 * XML-RPC channel. Because it is not a good idea to exec the
	 * xml engine to parse only one tag against the DTD, we simply
	 * check for the tag to be received and its content-lenght. */
	if (!axl_cmp (vortex_frame_get_payload (reply), "<bootrpy />") ||
	    vortex_frame_get_payload_size (reply) != 11) {
		
		/* notify error while booting XML-RPC channel */
		__vortex_xml_rpc_notify (process_status, channel, VortexError, 
					 "Received wrong bootmsg reply while starting a XML-RPC channel", user_data);

		/* nullify the channel */
		channel = NULL;

		goto finish_boot_xml_rpc_channel;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "XML-RPC channel accepted");

	/* notify user space level */
	__vortex_xml_rpc_notify (process_status, channel, VortexOk,
				 "Channel XML-RPC ready and waiting, now let's RPC", user_data);
	    
 finish_boot_xml_rpc_channel:

	/* free message and error code if were defined */
	vortex_support_free (2, msg, axl_free, reply, vortex_frame_unref);

	return channel;
}

/** 
 * @brief Perform initial boot step to get confirmation from remote
 * server to accept incoming XML-RPC under the given resource.
 *
 * XML-RPC invocation follows the next diagram:
 *
 * \image html xml-rpc-invocation.png "XML-RPC invocation diagram"
 *
 * From the execution of this function, a new channel (\ref
 * VortexChannel) is get, already initialized under the XML-RPC
 * profile. This channel will be used \ref vortex_xml_rpc_invoke.
 * 
 * @param connection The connection where the XML-RPC invocation will
 * be performed, actually, the connection where the XML-RPC channel
 * will be created.
 *
 * @param serverName For those connection that didn't server
 * serverName virtual host mechanism, this value will be used. Keep in
 * mind that previous channels could be already negotiated the
 * serverName identity making this value to be ignored. The serverName
 * value is negotiated for the first channel requesting it, but once
 * done, its value is global to the session, that is the given
 * connection you are using. This is parameter is optional. If a NULL
 * value is provided, the serverName attribute will be not used.
 *
 * @param resourceName The resource name requested to be accepted.
 * This parameter is not optional. If a NULL value is
 * provided, the resource "/" will be used as default value.
 *
 * @param process_status A mandatory notification handler where the
 * XML-RPC initial boot result will be notified.
 *
 * @param user_data User defined data to be passed in to the
 * <i>process_status</i> notify handler.
 */
void     vortex_xml_rpc_boot_channel (VortexConnection        * connection,
				      const char              * serverName,
				      const char              * resourceName,
				      VortexXmlRpcBootNotify    process_status,
				      axlPointer                user_data)
{
	VortexXmlRpcBootData * data;
	VortexCtx            * ctx = vortex_connection_get_ctx (connection);

	/* perform XML-RPC initial boot operations. */
	if (!vortex_connection_is_ok (connection, axl_false)) {
		__vortex_xml_rpc_notify (process_status, NULL, VortexError,
					 "Provided a non-connected connection while requested to perform XML-RPC boot process",
					 user_data);
		return;
	}
	
	/* check for process status function */
	if (process_status == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "notify function is mandatory, unable to perform XML-RPC boot request");
		return;
	}

	/* prepare data for the process */
	data                 = axl_new (VortexXmlRpcBootData, 1);
	data->connection     = connection;
	data->serverName     = copy_if_not_null (serverName);
	data->resourceName   = copy_if_not_null (resourceName);
	data->process_status = process_status;
	data->user_data      = user_data;

	/* perform rpc boot process */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_xml_rpc_boot_channel_process,  data);
	return;
}

/** 
 * @internal
 *
 * @brief Support function for \ref vortex_tls_start_negotiation_sync
 * function.
 */
void __vortex_xml_rpc_boot_channel_sync_process  (VortexChannel    * booted_channel,
						  VortexStatus       status,
						  char             * message,
						  axlPointer         user_data)
{
	VortexAsyncQueue * queue = user_data;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx   = vortex_channel_get_ctx (booted_channel);
#endif

	/* push and unref man! (first status, then status_message and
	 * finally the connection). This must follow this order. */
	QUEUE_PUSH  (queue, INT_TO_PTR (status));
	QUEUE_PUSH  (queue, message);

	/* only notify the booted channel if not null */
	if (booted_channel != NULL) {
		QUEUE_PUSH  (queue, booted_channel); 
	}
	vortex_async_queue_unref (queue);
	
	return;
}

/** 
 * @brief Perform a synchronous (blocking) XML-RPC channel boot.
 *
 * This function perform the same operation like \ref
 * vortex_xml_rpc_boot_channel function but in a blocking
 * manner. This function is ideal for batch process and every system
 * that requires a blocking remote method invocation.
 *
 * In the case that an asynchronous method invocation is required,
 * \ref vortex_xml_rpc_boot_channel should be used.
 *
 * The function allows to provide references to an optional
 * <b>status</b> and an optional <b>message</b> variables, so the
 * caller could perform a better error reporting.
 * 
 * @param connection The connection where the XML-RPC invocation will
 * be performed, actually, the connection where the XML-RPC channel
 * will be created.
 *
 * @param serverName For those connection that didn't server
 * serverName virtual host mechanism, this value will be used. Keep in
 * mind that previous channels could be already negotiated the
 * serverName identity making this value to be ignored. The serverName
 * value is negotiated for the first channel requesting it, but once
 * done, its value is global to the session, that is the given
 * connection you are using. This parameter is optional. If a NULL
 * value is provided, the serverName attribute will be not used.
 *
 * @param resourceName The resource name requested to be
 * accepted. This parameter is not optional. If a NULL value is
 * provided, the resource "/" will be used as default value.
 *
 * @param status An optional status reference that will be filled with
 * the process termination status.
 *
 * @param status_message An optional message reference that will be filled
 * with a textual diagnostic, explaining termination status.
 * 
 * @return A channel already initialized with the XML-RPC profile or
 * NULL if it fails.
 */
VortexChannel     * vortex_xml_rpc_boot_channel_sync       (VortexConnection        * connection,
							    const char              * serverName,
							    const char              * resourceName,
							    VortexStatus            * status,
							    char                   ** status_message)
{
	VortexAsyncQueue   * queue     = NULL;
	VortexChannel      * _channel  = NULL;
	VortexStatus       * _status   = NULL;
	VortexCtx          * ctx;

	/* check environment values */
	if (! vortex_connection_is_ok (connection, axl_false))
		return NULL;
	
	/* creates the async queue */
	queue = vortex_async_queue_new ();
	vortex_async_queue_ref (queue);

	/* begin XML-RPC bootstrapping */
	vortex_xml_rpc_boot_channel (connection, serverName, resourceName,
				     __vortex_xml_rpc_boot_channel_sync_process, 
				     queue);

	/* get status */
	ctx     = vortex_connection_get_ctx (connection);
	_status = vortex_async_queue_timedpop (queue, vortex_connection_get_timeout (ctx));
	if (_status == NULL) {
		/* seems timeout have happen while waiting for SASL to
		 * end */
		if (status != NULL)
			(* status)         = VortexError;
		if (status_message != NULL)
			(* status_message) = "Timeout have been reached while waiting for XML-RPC boot to finish";
		return NULL;
	}

	/* get status */
	if (status != NULL)
		(* status) = PTR_TO_INT (_status);
	
	/* get message */
	if (status_message != NULL)
		(* status_message) = vortex_async_queue_pop (queue);
	else
		vortex_async_queue_pop (queue);

	/* get connection */
	if (PTR_TO_INT(_status) == VortexOk)
		_channel = vortex_async_queue_pop (queue);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	/* return received value */
	return _channel;
}

/** 
 * @internal Structure used by the channel pool mechanism inside the
 * xml-rpc module.
 */
typedef struct _XmlRpcCreateChannelData {
	char  * serverName;
	char  * resourceName;
}XmlRpcCreateChannelData;


XmlRpcCreateChannelData * __vortex_xml_rpc_create_channel_data (const char  * serverName,
								const char  * resourceName)
{
	XmlRpcCreateChannelData * result;

	/* create the data */
	result = axl_new (XmlRpcCreateChannelData, 1);

	/* check for server name */
	if (serverName != NULL)
		result->serverName   = axl_strdup (serverName);

	/* check for resource value */
	if (resourceName != NULL)
		result->resourceName = axl_strdup (resourceName);

	/* return result */
	return result;
}

void __vortex_xml_rpc_destroy_channel_data (XmlRpcCreateChannelData * data)
{
	/* do not free anything if null reference is received */
	if (data == NULL)
		return;
	
	/* check for server name */
	if (data->serverName != NULL)
		axl_free (data->serverName);

	/* check for resource value */
	if (data->resourceName != NULL)
		axl_free (data->resourceName);

	/* free the node itself  */
	axl_free (data);

	return;
}

/** 
 * @internal implementation to support channel pool mechanism for the
 * XML-RPC module.
 * 
 * @return A newly created channel or null if it fails.
 */
VortexChannel * __vortex_xml_rpc_create_channel (VortexConnection     * connection,
						 int                    channel_num,
						 const char           * profile,
						 VortexOnCloseChannel   on_close, 
						 axlPointer             on_close_user_data,
						 VortexOnFrameReceived  on_received, 
						 axlPointer             on_received_user_data,
						 axlPointer             create_channel_user_data,
						 axlPointer             user_data)
{
	/* reference to the creation data */
	XmlRpcCreateChannelData * data = create_channel_user_data;
	VortexChannel           * channel;
	
	/* create the channel */
	channel = vortex_xml_rpc_boot_channel_sync (connection, 
						    data != NULL ? data->serverName   : NULL,
						    data != NULL ? data->resourceName : NULL,
						    NULL, NULL);
	/* return the channel created */
	return channel;
}

/** 
 * @brief Allows to create and attach a channel pool containing
 * XML-RPC channels that are created using the provided serverName and
 * resourceName value.
 *
 * An efficient alternative to create a XML-RPC channel (\ref
 * vortex_xml_rpc_boot_channel), perform the invocation (\ref
 * vortex_xml_rpc_invoke), and then release the channel (automatically
 * done by the invocation API) is to use a channel pool.
 *
 * The channel pool maintains a set of channels available to be used,
 * and them are reused across invocations. In the case more channels
 * are required, the channel pool automatically negotiate them for
 * you.
 *
 * The idea is to create a channel pool (you can create several
 * channel pools on the channel connection) using the function, making
 * all channels inside the pool created to use the serverName and the
 * resourceName value provided.
 *
 * \code
 * void create_pool () {
 *   VortexChannelPool * pool;
 *  
 *   // create a pool that uses as resource "/API/v1.0"
 *   pool = vortex_xml_rpc_create_channel_pool (connection, 
 *                                              // no server name value 
 *                                              NULL, 
 *                                              // resource value
 *                                              "/API/v1.0",
 *                                              // no pool creation notify
 *                                              NULL, NULL);
 * }
 * \endcode
 *
 * Later in your code, if you require a XML-RPC channel to perform an
 * invocation, you get it from the channel pool:
 *
 * \code
 * void invoke (XmlRpcMethodCall * invocator) {
 *   VortexChannel        * channel;
 *   XmlRpcMethodResponse * response;
 *
 *   // get a channel (creating it if not available, using default pool: 1)
 *   channel = vortex_xml_rpc_channel_pool_get_next (connection, axl_true, 1);
 * 
 *   // now perform the invocation
 *   response = vortex_xml_rpc_invoke_sync (channel, invocator);
 *
 *   // some something with the reply
 *
 *   // the channel is not required to be released by calling to
 *   // vortex_channel_pool_release_channel because it is already done
 *   // by the invocation API: vortex_xml_rpc_invoke (_sync)
 * }
 * \endcode
 * 
 * @param connection The connection where the channel pool is created.
 *
 * @param serverName The serverName value to be used while creating
 * more channels in the pool.
 *
 * @param resourceName The resource value to be used while creating
 * more channel in the pool.
 * 
 * @param on_pool_created The channel pool created notification. See
 * handler documentation. If the handler is provided, the function
 * will return NULL and the channel pool reference created will be
 * received at the handler notification.
 * 
 * @param user_data User defined data to be passed in to the on_pool_created handler.
 * 
 * @return A newly created \ref VortexChannelPool or NULL if it fails
 * or a channel pool created notification was received.
 */
VortexChannelPool * vortex_xml_rpc_create_channel_pool     (VortexConnection            * connection,
							    const char                  * serverName,
							    const char                  * resourceName,
							    VortexOnChannelPoolCreated    on_pool_created,
							    axlPointer                    user_data)
{
	VortexChannelPool       * result;
	XmlRpcCreateChannelData * data;

	/* get the connection reference */
	if (connection == NULL)
		return NULL;
	
	/* create data to be provided to the creation function */
	data   = __vortex_xml_rpc_create_channel_data (serverName, resourceName);

	/* create the channel pool */	
	result = vortex_channel_pool_new_full (connection,
					       VORTEX_XML_RPC_PROFILE,
					       /* initially one channel in the pool */
					       1,
					       /* creation handler */
					       __vortex_xml_rpc_create_channel, data,
					       /* default close handler */
					       NULL, NULL,
					       /* default frame received */
					       NULL, NULL,
					       /* pool created notification */
					       on_pool_created, user_data);

	/* configure the default serverName and resource to be used */
	vortex_connection_set_data_full (connection, 
					 /* the key to access the data */
					 axl_strdup_printf ("vortex-xml-rpc:channel-pool-data:%d", vortex_channel_pool_get_id (result)), 
					 /* data associated */
					 data,
					 /* destroy functions */
					 axl_free, 
					 /* data and its destroy function */
					 (axlDestroyFunc) __vortex_xml_rpc_destroy_channel_data);

	/* return channel pool reference */
	return result;
}

/** 
 * @brief Allows to get next channel ready on the channel pool created
 * on the connection provided.
 *
 * Because there could be several channel pools created on the
 * connection, the function also accept receiving the pool_id to be
 * used to locate the channel pool. Under situations where only one
 * channel pool is created on the provided connection, you can safely
 * set pool_id to -1.
 *
 * The auto_inc parameters is passed directly to the \ref vortex_channel_pool_get_next_ready_full function and acts as a
 * signal to make the channel pool to automatically resize its pool,
 * creating more channels, if the current pool doesn't have any ready
 * to use channel.
 *
 * Once you have finished with the channel returned by this function,
 * you must call to \ref vortex_channel_pool_release_channel.
 *
 * <i>NOTE:</i> The function will use as values for new channels
 * created for resourceName and serverName the one provided at the
 * \ref vortex_xml_rpc_create_channel_pool.
 * 
 * @param connection The connection where the channels will be created.
 *
 * @param auto_inc Creates more channels if no channel is ready when
 * the function is called.
 *
 * @param pool_id The pool_id to use to get the next channel or 1 if
 * the function must use the default channel pool.
 * 
 * @return A newly allocated channel, already included in the pool or
 * NULL if it fails.
 */
VortexChannel     * vortex_xml_rpc_channel_pool_get_next   (VortexConnection * connection, 
							    axl_bool           auto_inc, 
							    int                pool_id)
{
	VortexChannelPool       * pool = NULL;
	XmlRpcCreateChannelData * data = NULL;
	char                    * key;

	/* check incoming parameters */
	if (connection == NULL || pool_id <= 0)
		return NULL;
	
	/* get the pool */
	pool = vortex_connection_get_channel_pool (connection, pool_id);
	if (pool == NULL)
		return NULL;

	/* get the data (resourceName and serverName) associated to
	 * the pool */
	key  = axl_strdup_printf ("vortex-xml-rpc:channel-pool-data:%d", vortex_channel_pool_get_id (pool));

	/* get data and free key */
	data = vortex_connection_get_data (connection, key);
	axl_free (key);

	/* get the channel */
	return vortex_channel_pool_get_next_ready_full (pool, auto_inc, data);
}


/** 
 * @internal
 *
 * @brief Internal definition to transport parameters from \ref
 * vortex_xml_rpc_invoke to the threaded function that finally do the
 * work.
 */
typedef struct _VortexXmlRpcInvokeData {
	VortexChannel        * channel;
	XmlRpcMethodCall     * invocator;
	XmlRpcInvokeNotify     reply_notify;
	axlPointer             user_data;
}VortexXmlRpcInvokeData;


/** 
 * @internal
 * 
 * Internal function used to get faultCode and faultString values,
 * from the XML-RPC response received. The node received points to the
 * <value> node which contains the <struct> structure with the desired
 * values.
 */
axl_bool      __vortex_xml_rpc_get_fault_values (VortexCtx          * ctx,
						 axlNode            * node, 
						 int                * faultCode, 
						 char              ** faultString,
						 XmlRpcMethodValue ** _value)
{
	XmlRpcMethodValue * value;
	XmlRpcStruct      * _struct;

	/* get the struct */
	value = __vortex_xml_rpc_parse_value (ctx, node);

	/* check that we have received an struct */
	if (method_value_get_type (value) != XML_RPC_STRUCT_VALUE) {
		method_value_free (value);
		return axl_false;
	}

	/* get the struct inside */
	_struct = vortex_xml_rpc_method_value_get_as_struct (value);

	/* get the fault code */
	*faultCode   = vortex_xml_rpc_struct_get_member_value_as_int (_struct, "faultCode");
	
	/* get the fault String */
	*faultString = vortex_xml_rpc_struct_get_member_value_as_string (_struct, "faultString");

	/* set the value reference */
	*_value    = value;
	
	return axl_true;
}

/** 
 * @internal
 *
 * @brief Support function for the \ref __vortex_xml_rpc_invoke
 * function which actually perform the work of unmarshalling data
 * received due to a previous request performed, and executing the
 * proper handler into the application space.
 * 
 * @param channel The channel where the XML-RPC reply is being
 * received.
 *
 * @param connection The connection where the channel lives.
 *
 * @param frame The frame containing the XML-RPC method response data.
 *
 * @param user_data A VortexXmlRpcInvokeData containing, among other
 * data, the method to invoke.
 */
void __vortex_xml_rpc_invoke_process_reply (VortexChannel      * channel, 
					    VortexConnection   * connection, 
					    VortexFrame        * frame, 
					    axlPointer           __invocator_data)
{
	/* invocation specific data received inside user_data and
	 * provided once the invocation was initiated at
	 * __vortex_xml_rpc_invoke */
	VortexXmlRpcInvokeData * invocator_data  = __invocator_data;
	XmlRpcInvokeNotify       reply_notify    = invocator_data->reply_notify;
	axlPointer               user_data       = invocator_data->user_data;
	VortexCtx              * ctx             = vortex_connection_get_ctx (connection);
	XmlRpcMethodValue      * value;
	XmlRpcMethodResponse   * response        = NULL;

	/* variable declaration for the xml stuff */
	axlDoc                 * doc;
	axlNode                * node;
	axlError               * error;

	/* variables for fault errors */
	char                   * faultString;
	int                      faultCode;
	

	/* release memory hold by user_data, which contains a node of
	   type VortexXmlRpcInvokeData */
	axl_free (invocator_data);
	vortex_log (VORTEX_LEVEL_DEBUG, "reply received, processing..");

	/* flag the channel to have its reply processed */
	vortex_channel_flag_reply_processed (channel, axl_true);
	
	/* parse incoming data */
	doc = axl_doc_parse (vortex_frame_get_payload (frame), 
			     vortex_frame_get_payload_size (frame), &error);
	if (!doc) {
		/* drop a log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to parse document received, error was: %s", axl_error_get (error));
		axl_error_free (error);

		/* notify user space  */
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_BAD_REPLY_RECEIVED,
						  -1, "unable to parse reply received due to a XML-RPC invocation, xml parsing in memory has failed.",
						  channel, NULL, user_data);
		/* nothing more to do */
		return;
	}
	
	/* get the root node */
	node = axl_doc_get_root (doc);
	if (! NODE_CMP_NAME (node, "methodResponse")) {

		/* notify user space  */
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_BAD_REPLY_RECEIVED,
						  -1, "received a different top level tag where expected a <methodResponse>",
						  channel, NULL, user_data);
		/* nothing more to do */
		axl_doc_free (doc);
		return;
	}

	/* walk into the first child */
	node = axl_node_get_child_nth (node, 0);

	/* check if we have received a fault reply */
	if (NODE_CMP_NAME (node, "fault")) {

		/* we got a fault reply, now point to the <value />  */
		node = axl_node_get_child_nth (node, 0);

		/* get the fault error code and the fault string */
		if (! __vortex_xml_rpc_get_fault_values (ctx, node, &faultCode, &faultString, &value)) {
			/* notify the user space */
			__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_BAD_REPLY_RECEIVED,
							  -1, "Unable to decode the fault structure received, the remote peer is sending a not properly formated fault struct",
							  channel, NULL, user_data);
		}else {
			/* notify the user space */
			__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_FAULT_REPLY,
							  faultCode,  faultString,
							  channel, NULL, user_data);
			/* free the struct received */
			method_value_free (value);
		}

		/* free the document */
		axl_doc_free (doc);
		return;
	}
	
	/* we got something else, check if that is a params as a first
	 * top level node */
	if (NODE_CMP_NAME (node, "params")) {
		/* perform here the code to read data received */
		vortex_log (VORTEX_LEVEL_DEBUG, "received a positive response..");

		/* we got a fault reply, now point to the <param />  */
		node = axl_node_get_child_nth (node, 0);

		/* now get the <value> node */
		node = axl_node_get_child_nth (node, 0);

		/* get the value */
		value    = __vortex_xml_rpc_parse_value (ctx, node);
		if (value != NULL) {

			vortex_log (VORTEX_LEVEL_DEBUG, "received method value with type: (%d)", 
				    method_value_get_type (value));
			
			/* create the response object */
			response = vortex_xml_rpc_method_response_new (XML_RPC_OK,
								       -1, NULL, /* fault code especification */
								       value);
			/* notify reply received */
			__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_OK,
							  -1, NULL,
							  channel, response, user_data);
		}else {
			/* notify reply received */
			__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_BAD_REPLY_RECEIVED,
							  -1, "received a positive formated method response but, the content inside, is not properly formated",
							  channel, NULL, user_data);
		}

		/* free the document */
		axl_doc_free (doc);
		return;
	}
	
	
	/* it seems that we have received something that is not the
	 * data expected */
	__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_BAD_REPLY_RECEIVED,
					  -1, "received a bad child top level node to <methodResponse>",
					  channel, NULL, user_data);
	/* nothing more to do */
	axl_doc_free (doc);


	return;
}


/** 
 * @internal
 * @brief Perform the xml-rpc invocation in a blocking manner.
 *
 * Because this function is executed from a separated thread, caller
 * is already unblocked. Rather than using asynchronous callbacks to
 * write this function, which would make the code harder to read and
 * maintain, it is preferred to perform the job in a linear manner.
 *
 * This function converts the invocator object (invocator attribute
 * received) into a XML representation following the definition found
 * for the XML-RPC.
 *
 * Then it waits for the data to be received, until a timeout is
 * reached.
 * 
 * @param data The data needed to perform the invocation.
 * 
 * @return This function always return NULL.
 */
axlPointer __vortex_xml_rpc_invoke (VortexXmlRpcInvokeData * data)
{
	VortexChannel        * channel       = data->channel;
	XmlRpcMethodCall     * invocator     = data->invocator;
	XmlRpcInvokeNotify     reply_notify  = data->reply_notify;
	axlPointer             user_data     = data->user_data;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx            * ctx           = CHANNEL_CTX(channel);
#endif
	char                 * message;
	int                    message_size;

	/* marshall the message */
	message = vortex_xml_rpc_method_call_marshall (invocator, &message_size);
	
	/* set the new frame receive handler and its data */
	vortex_channel_set_received_handler (channel, __vortex_xml_rpc_invoke_process_reply, data);
	
	/* perform the invocation */
	vortex_log (VORTEX_LEVEL_DEBUG, "sending XML-RPC message: %s", message ? message : "null");
	if (!vortex_channel_send_msg (channel, message, message_size, NULL)) {
		/* seems the invocation has failed */
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_INVOCATION_FAILURE, 
						  -1, "An error have happened while sending the XML-RPC request message.",
						  channel, NULL, user_data);
		/* release data node */
		axl_free (data);

		/* update the reference of the data refernce to NULL */
		vortex_channel_set_received_handler (channel, __vortex_xml_rpc_invoke_process_reply, NULL);
	}
	
	/* release data no longer needed */
	if (vortex_xml_rpc_method_call_must_release (invocator))
		vortex_xml_rpc_method_call_free (invocator);
	axl_free (message);

	/* unref channel (reference acquired at
	 * vortex_xml_rpc_invoke) */
	vortex_channel_unref (channel);
	
	/* nothing more to do */
	return NULL;
}

/** 
 * @brief Perform an asynchronous invocation using the XML-RPC profile. 
 *
 * This function allows to perform an asynchronous invocation on the
 * given channel, that is already running the XML-RPC profile, using
 * as method invocator the <b>method_call</b> provided.
 *
 * The channel received on this function should be get from: 
 *   - \ref vortex_xml_rpc_boot_channel
 *   - \ref vortex_xml_rpc_boot_channel_sync
 *
 * Once the method is invoked, the reply received is notified on the
 * <b>reply_notify</b> callback. 
 *
 * Because the function will perform the invocation in an asynchronous
 * mode, the caller will not be blocked while calling to this
 * function. If a blocking invocation mode is required check \ref
 * vortex_xml_rpc_invoke_sync.
 *
 * See also: \ref XmlRpcMethodCall
 * 
 * @param channel The channel, running the XML-RPC profile, where the
 * invocation will take place.
 *
 * @param method_call The method call invocator, representing the
 * method to be invoked plus its parameters.
 *
 * @param reply_notify The notify function where the reply will be
 * reported.
 *
 * @param user_data User defined data that will be passed to the
 * notify function along the reply received.
 *
 * @return axl_true if the invocation was performed (only the RPC call),
 * otherwise axl_false is returned.
 */
axl_bool                 vortex_xml_rpc_invoke                  (VortexChannel           * channel,
								 XmlRpcMethodCall        * method_call,
								 XmlRpcInvokeNotify        reply_notify,
								 axlPointer                user_data)
{
	VortexXmlRpcInvokeData * data;
	VortexCtx              * ctx;

	/* perform some environmental checks for incoming data */
	if (channel == NULL || method_call == NULL || reply_notify == NULL)
		return axl_false;

	/* get the context reference */
	ctx = vortex_channel_get_ctx (channel);

	/* check for the XML-RPC profile to be running */
	if (!vortex_channel_is_running_profile (channel, VORTEX_XML_RPC_PROFILE)) {
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_NOT_XML_RPC_CHANNEL,
						  -1, "An invocation has been detected over a channel that is not running the XML-RPC profile.",
						  channel, NULL, user_data);
		return axl_false;
	}

	/* check that the channel provided is at the ready state */
	if (vortex_xml_rpc_channel_status (channel) != XmlRpcStateReady) {
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_CHANNEL_NOT_READY,
						  -1, "Detected an XML-RPC invocation over a channel that is not in the ready state",
						  channel, NULL, user_data);
		return axl_false;
	}

	/* check if the channel is waiting for a previous reply */
	if (! vortex_channel_is_ready (channel)) {
		__vortex_xml_rpc_notify_response (reply_notify, XML_RPC_WAITING_PREVIOUS,
						  -1, "An XML-RPC invocation was detected over a channel that is actually waiting for a previous invocation.",
						  channel, NULL, user_data);
		return axl_false;
	}

	/* prepare data to be passed in to the thread function */
	data               = axl_new (VortexXmlRpcInvokeData, 1);
	data->channel      = channel;
	/* get a reference to the channel during the process */
	vortex_channel_ref (channel);
	data->invocator    = method_call;
	data->reply_notify = reply_notify;
	data->user_data    = user_data;

	/* perform the invocation in a non blocking manner */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_xml_rpc_invoke, data);
	return axl_false;
}

/** 
 * @internal
 *
 * @brief Internal support function for \ref
 * vortex_xml_rpc_invoke_sync, that queues the method response
 * received.
 *
 * 
 * @param channel The channel where the method response reply was received.
 * @param response The method response received.
 * @param user_data User space data.
 */
void __vortex_xml_rpc_invoke_sync_process (VortexChannel        * channel,
					   XmlRpcMethodResponse * response,
					   axlPointer             user_data)
{
	VortexAsyncQueue * queue = user_data;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx   = vortex_channel_get_ctx (channel);
#endif

	/* push reply received */
	QUEUE_PUSH (queue, response);

	/* decrease queue reference and return */
	vortex_async_queue_unref (queue);
	
	return;
}


/** 
 * @brief Perform a synchronous XML-RPC invocation using a method call
 * already built over an already booted XML-RPC channel.
 *
 * The function will take the method definition found in
 * <b>method_call</b> and will perform a XML-RPC invocation, blocking
 * the caller until finished completely, over the channel selected. To
 * boot a channel running the XML-RPC over BEEP profile use the
 * following function: \ref vortex_xml_rpc_boot_channel_sync.
 *
 * This function is built on top of \ref vortex_xml_rpc_invoke, so
 * check documentation for that function to know more.  See also: \ref
 * vortex_xml_rpc_method_call_new in special automatic invocator deallocation
 * performed by the XML-RPC engine.
 * 
 *
 * Here is an example:
 * \code
 * // peform a synchronous method 
 * response = vortex_xml_rpc_invoke_sync (channel, invocator);
 *
 * // check result received 
 * switch (method_response_get_status (response)) {
 * case XML_RPC_OK:
 *	printf ("Reply received ok, result is: %s\n", method_response_stringify (response));
 *	break;
 * default:
 *	printf ("RPC invocation have failed: (code: %d) : %s\n",
 *		 method_response_get_fault_code (response),
 *		 method_response_get_fault_string (response));
 *	break;
 * }
 *
 * // free the response received  
 * method_response_free (response);
 *
 * // but DONT FREE method call. See XmlRpcMethodCall object information.
 * \endcode
 * 
 * @param channel The channel where the invocation will be performed.
 * @param method_call The invocator object, representing the method call.
 * 
 * @return A \ref XmlRpcMethodResponse with the reply or NULL if
 * fails.
 */
XmlRpcMethodResponse * vortex_xml_rpc_invoke_sync          (VortexChannel           * channel,
							    XmlRpcMethodCall        * method_call)
{
	VortexAsyncQueue     * queue;
	XmlRpcMethodResponse * response;
	VortexCtx            * ctx;

	/* perform some additional checks */
	if (channel == NULL || method_call == NULL)
		return NULL;
	
	/* get a reference to the context */
	ctx = vortex_channel_get_ctx (channel);

	/* creates the asynchronous queue */
	queue = vortex_async_queue_new ();
	vortex_async_queue_ref (queue);

	/* perform invocation */
	vortex_log (VORTEX_LEVEL_DEBUG, "synchronous invocation performed..");
	vortex_xml_rpc_invoke (channel, method_call, __vortex_xml_rpc_invoke_sync_process, queue);

	/* wait until data reach */
	response = vortex_async_queue_timedpop (queue, vortex_connection_get_timeout (ctx));
	vortex_log (VORTEX_LEVEL_DEBUG, "method response received..");

	/* deallocate queue */
	vortex_async_queue_unref (queue);

	/* it seems a timeout have happened, stub out a response */
	if (response == NULL)
		response = method_response_new (XML_RPC_TIMEOUT_ERROR, -1, "Invocation timeout, timeout have been reached while waiting for method response.", NULL);
	return response;
}

/** 
 * @brief Allows to get current XML-RPC channel boot status for the
 * given channel.
 *
 * According to the XML-RPC: there are two states in the BEEP profile
 * for XML-RPC, "boot", the profile's initial state, and "ready".
 *
 * On the "boot" state the channel have been successfully created under
 * the semantic of the XML-RPC profile but not it isn't still booted
 * under some resource. 
 *
 * On the "ready" state the channel is ready to perform XML-RPC
 * requests because both peers agree about resource being accessed.
 *
 * 
 * @param channel The channel to get XML-RPC channel status.
 * 
 * @return Current channel status.
 */
VortexXmlRpcState   vortex_xml_rpc_channel_status   (VortexChannel * channel)
{
	char      * boot_state;
	/* get a reference to the context */
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = vortex_channel_get_ctx (channel);
#endif

	/* check some environment conditions  */
	if ((channel == NULL) ||
	    !vortex_channel_is_running_profile (channel, VORTEX_XML_RPC_PROFILE)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "channel provided is null or it is not running the XML-RPC profile, unable to return channel status");
		return XmlRpcStateUnknown;
	}

	boot_state = vortex_channel_get_data (channel, XML_RPC_BOOT_STATE);
	if (axl_cmp (boot_state, "boot"))
		return XmlRpcStateBoot;
	if (axl_cmp (boot_state, "ready"))
		return XmlRpcStateReady;
	
	vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get current XML-RPC channel status, returning channel state: unknown");
	return XmlRpcStateUnknown;
}


/** 
 * @internal
 *
 * @brief Support function which perform the unmarshalling from an xml
 * representation into the XmlRpcMethodCall representation.
 * 
 * @param frame The frame containing a xml rpc method call object.
 * 
 * @return A newly allocated XmlRpcMethodCall or NULL if fails.
 */
XmlRpcMethodCall * __vortex_xml_rpc_frame_received_parse_method_call (VortexFrame * frame)
{
	axlDoc            * doc;
	axlNode           * node;
	axlNode           * node_aux;
	
	const char        * method_name;
	int                 param_count;
	int                 iterator;

	XmlRpcMethodCall  * method_call;
	XmlRpcMethodValue * value;
	/* get a reference to the context */
	VortexCtx         * ctx = vortex_frame_get_ctx (frame);

	/* parse incoming xml data */
	doc = axl_doc_parse (vortex_frame_get_payload (frame), 
			     vortex_frame_get_payload_size (frame), NULL);
	if (!doc) {
		vortex_log (VORTEX_LEVEL_DEBUG, "unable to parse incoming xml data");
		return NULL;
	}
	
	/* get method name */
	method_name = axl_doc_get_content_at (doc, "/methodCall/methodName", NULL);
	if (method_name == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get method name value");

		/* free the document */
		axl_doc_free (doc);
		return NULL;
	}

	/* get a reference to the <params> node */
	node        = axl_doc_get (doc, "/methodCall/params");

	/* get how many childs have the <params> tag */
	if (node != NULL)
		param_count = axl_node_get_child_num (node);
	else
		param_count = 0;

	vortex_log (VORTEX_LEVEL_DEBUG, "invocation detected for: %s, with %d parameters", method_name, param_count);

	/* build the method call */
	method_call = method_call_new (vortex_frame_get_ctx (frame), method_name, param_count);

	/* now get all parameters */
	iterator = 0;
	while ((param_count > 0) && (iterator < axl_node_get_child_num (node))) {
		
		/* get a reference a reference to the <param> tag */
		node_aux = axl_node_get_child_nth (node, iterator);

		vortex_log (VORTEX_LEVEL_DEBUG, "node name=<%s>", 
		       axl_node_get_name (node_aux));
		
		/* get a referece to the <value> tag inside */
		node_aux = axl_node_get_child_nth (node_aux, 0);

		vortex_log (VORTEX_LEVEL_DEBUG, "node name(2)=<%s>", 
		       axl_node_get_name (node_aux));

		/* parse param value and add it to the method call */
		value = __vortex_xml_rpc_parse_value (ctx, node_aux);
		
		/* add the value to the method call */
		method_call_add_value (method_call, value);
		
		/* get a reference to the next element */
		iterator++;
	}

	/* free xml document */
	axl_doc_free (doc);

	/* return method call received */
	return method_call;
}

/** 
 * @internal
 * @brief Frame received handler for XML-RPC channels.
 */
void __vortex_xml_rpc_frame_received (VortexChannel    * channel, 
				      VortexConnection * connection, 
				      VortexFrame      * frame, 
				      axlPointer         user_data)
{
	/* get current context */
	VortexCtx                        * ctx = vortex_channel_get_ctx (channel);
	XmlRpcMethodCall                 * method_call;
	XmlRpcMethodResponse             * method_response = NULL;
	VortexXmlRpcServiceDispatchNode  * dispatch_node;
	int                                iterator;
	axlList                          * service_dispatch;

	vortex_log (VORTEX_LEVEL_DEBUG, "frame received on XML-RPC channel");

	/* get method call received */
	method_call = __vortex_xml_rpc_frame_received_parse_method_call (frame);
	if (method_call == NULL) {
		/* return without doing nothing */
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "method call name received: %s (params %d), dispatching...",
	       method_call_get_name (method_call), method_call_get_num_params (method_call));

	/* set method call reply data */
	__vortex_xml_rpc_method_call_set_reply_data (method_call, channel, 
						     vortex_frame_get_msgno (frame));

	/* first check for an specific dispatch handler for the given
	 * method call */
	
	/* now see for the general dispatch function is available if
	 * the previous one was not found */
	service_dispatch = vortex_ctx_get_data (ctx, VORTEX_XML_RPC_SERVICE_DISPATCH);
	if (service_dispatch != NULL) {
		
		/* invoke and get the method response */
		iterator = 0;
		while ((method_response == NULL) && (iterator < axl_list_length (service_dispatch))) {
			/* get the dispatch node */
			dispatch_node = axl_list_get_nth (service_dispatch, iterator);
			
			/* check the validate handler to see if the
			 * method call can be handled for the resource
			 * provided */
			if (dispatch_node != NULL && dispatch_node->validate) {
				if (dispatch_node->validate (connection, 
							     vortex_channel_get_number (channel), 
							     vortex_connection_get_server_name (connection),
							     vortex_xml_rpc_channel_get_resource (channel),
							     dispatch_node->validate_data)) {

					/* found the validation node, dispatch with the associated handler */
					method_response = dispatch_node->dispatch (channel, 
										   method_call, 
										   dispatch_node->dispatch_data);
					/* do not check data here and break the loop */
					break;
				} /* end if */
			} /* end if */

			/* Check dispatch handler in a generic
			 * maner. If the dispatch function returns
			 * content we can break the loop.  */
			if (dispatch_node != NULL && dispatch_node->dispatch) {
				/* found the validation node, dispatch with the associated handler */
				method_response = dispatch_node->dispatch (channel, 
									   method_call, 
									   dispatch_node->dispatch_data);
				/* break if found a method response */
				if (method_response != NULL)
					break;
			} /* end if */

			/* update the iterator */
			iterator++;

		} /* end while */
	} /* end if */

	/* if method response replied is null, the method invocation
	 * reply has been deferred, just return */
	if (method_response == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "XML-RPC reply deferred, leaving frame received handler");
		return;
	}
	
	/* perform reply */
	vortex_log (VORTEX_LEVEL_DEBUG, 
	       "XML-RPC reply performed, sending it back to the client");
	vortex_xml_rpc_notify_reply (method_call, method_response);
	return;
}

/** 
 * @internal
 *
 * @brief Internal vortex function to parse <bootmsg/> entity, getting
 * resource value inside it. 
 *
 * The function also checks for the message receivced, generating the
 * proper message to be returned in case of error.
 * 
 * @param profile_content The profile content.
 *
 * @param resource A pointer where the resource value will be stored.
 *
 * @param profile_content_reply A pointer where a possible error reply
 * will be stored.
 * 
 * @return axl_true if the message was successfully parsed, or axl_false if
 * some error have happened. In that case, profile_content_reply is
 * filled.
 */
axl_bool      __vortex_xml_rpc_parse_bootmsg (VortexCtx   * ctx,
					      const char  *  profile_content, 
					      char       ** resource, 
					      char       ** profile_content_reply)
{
	axlDtd    * xml_rpc_dtd;
	axlDoc    * doc;
	axlNode   * bootmsg;

	/* get channel management DTD */
	if ((xml_rpc_dtd = vortex_ctx_get_data (ctx, VORTEX_XML_RPC_BOOT_DTD)) == NULL) {
		(* profile_content_reply ) = 
			vortex_frame_get_error_message ("451",
							"unable to load dtd file (xml-rpc.dtd), cannot validate incoming message, returning error frame",
							NULL);
		return axl_false;
	}

	/* parser xml document */
	doc =  axl_doc_parse (profile_content, strlen (profile_content), NULL);
	if (!doc) {
		(* profile_content_reply ) = 
			vortex_frame_get_error_message ("501",
							"Invalid xml received while creating a XML-RPC channel.",
							NULL);
		return axl_false;
	}

	/* validate xml received */
	if (! axl_dtd_validate (doc, xml_rpc_dtd, NULL)) {
		/* free the document */
		(* profile_content_reply ) = 
			vortex_frame_get_error_message ("501",
							"xml validation failed while creating a XML-RPC channel.",
							NULL);
		/* free the document */
		axl_doc_free (doc);
		return axl_false;
	}

	/* get a reference to document root (the bootmsg) */
	bootmsg  = axl_doc_get_root (doc);
	if (! NODE_CMP_NAME (bootmsg,  "bootmsg")) {
		/* report the reply */
		(* profile_content_reply ) = 
			vortex_frame_get_error_message ("501",
							"unable to find as document root message the <bootmsg/> entity",
							NULL);
		/* free the document loaded */
		axl_doc_free (doc);
		return axl_false;
	}

	/* get the resource value */
	(* resource ) = axl_node_get_attribute_value_copy (bootmsg, "resource");
	if ((* resource ) == NULL) {
		(* profile_content_reply ) = 
			vortex_frame_get_error_message ("501",
							"resource not especified inside the <bootmsg/> received",
							NULL);
		/* free the document loaded */
		axl_doc_free (doc);
		return axl_false;
	}

	/* free the document loaded */
	axl_doc_free (doc);

	/* return to the upper level function the resource read */
	return axl_true;
}

/** 
 * @internal
 * @brief Start handler for all incoming request wanting to create an
 * XML-RPC channel.
 * 
 * @param profile The profile received, in this case the XML-RPC profile.
 * @param channel_num The channel number received.
 * @param connection The connection where the XML-RPC was received.
 * @param serverName The serverName value to initiate the virtual hosting
 *
 * @param profile_content Profile content, in this case, it could have
 * several probabilities to be the boot resouce message.
 *
 * @param profile_content_reply This variable should contain the value
 * to be replied. (in this case it should be filled with the bootrpy).
 *
 * @param encoding The profile content enconding.
 *
 * @param user_data Application defined data. In this case NULL.
 * 
 * @return The function should return axl_true if the resource requested
 * is accepted or not accepted but so the channel. axl_false should be
 * returned only if the channel and the resource are denied.
 */
axl_bool      __vortex_xml_rpc_start_msg (const char        * profile,
					  int                 channel_num,
					  VortexConnection  * connection,
					  const char        * serverName,
					  const char        * profile_content,
					  char             ** profile_content_reply,
					  VortexEncoding      encoding,
					  axlPointer          user_data)
{
	/* get current context */
	VortexCtx                        * ctx = vortex_connection_get_ctx (connection);
	char                             * resource;
	int                                iterator;
	VortexXmlRpcServiceDispatchNode  * dispatch_node;
	axl_bool                           accept = axl_false;
	VortexChannel                    * channel;
	axlList                          * service_dispatch;

	if (!__vortex_xml_rpc_parse_bootmsg (ctx, profile_content, &resource, profile_content_reply))
		return axl_false;

	vortex_log (VORTEX_LEVEL_DEBUG, "start message received on XML-RPC channel resource=%s",
	       resource);

	/* invoke resource validation */
	service_dispatch = vortex_ctx_get_data (ctx, VORTEX_XML_RPC_SERVICE_DISPATCH);
	if (service_dispatch == NULL) {
		accept = axl_true;
	}else {
		/* invoke application level resource validation */
		iterator      = 0;
		dispatch_node = NULL;
		while (!accept && (iterator < axl_list_length (service_dispatch))) {
			/* get the validate node */
			dispatch_node = axl_list_get_nth (service_dispatch, iterator);

			/* check and invoke */
			if (dispatch_node != NULL && dispatch_node->validate != NULL) {
				/* invoke providing the appropiate data */
				accept = dispatch_node->validate (connection, channel_num, serverName, resource, dispatch_node->validate_data);
			}

			/* update iterator */
			iterator++;
		} /* end while */
	} /* end if */
	
	/* reply with a bootrpy or an error message, accepting the channel to be created */
	if (accept) {
		/* configure positive reply */
		(* profile_content_reply) = axl_strdup ("<bootrpy />");
		
		/* set the resource booted to allow the channel to be classified */
		channel = vortex_connection_get_channel (connection, channel_num);
		vortex_log (VORTEX_LEVEL_DEBUG, "configure channel=%d resource=%s", 
			    channel_num, resource);
		vortex_channel_set_data_full (channel, XML_RPC_RESOURCE, resource, NULL, axl_free);
		
	} else {
		/* free resource because a negative reply is being generated */
		axl_free (resource);
		(* profile_content_reply) = vortex_frame_get_error_message ("550", 
									    "resource requested not accepted by this XML-RPC instance",
									    NULL);
	} /* end if */
	
	return axl_true;
}

/** 
 * @brief Allows to get the channel resource used to boot the provided
 * channel.
 *
 * @param channel The channel to get the resource inside.
 * 
 * @return The resource value or NULL if it fails. The value returned
 * is an internal reference that mustn't be deallocated. To get a
 * properly result, the channel must be already booted (\ref XmlRpcStateReady, use \ref vortex_xml_rpc_channel_status).
 */
const char  * vortex_xml_rpc_channel_get_resource (VortexChannel * channel)
{
	/* get the channel resource */
	return vortex_channel_get_data (channel, XML_RPC_RESOURCE);
}

/** 
 * @brief Allows to notify a reply that has been generated from the
 * given \ref XmlRpcMethodCall object.
 *
 * Service invocation could be deferred by returning a NULL value at
 * the service dispatch function. Once a reply is generated, it could
 * be send by using this function.
 *
 * This function not only generates the reply but also deallocates the
 * memory reserved by the two incoming parameters.
 *
 * @param method_call The method call that is being replied.
 *
 * @param method_response The method response representing a reply.
 *
 * @return The function could return axl_false (operation not completed)
 * if the parameters provided are NULL or rpc support is not
 * enabled. Otherwise, axl_true is returned.
 */
axl_bool  vortex_xml_rpc_notify_reply (XmlRpcMethodCall     * method_call, 
				       XmlRpcMethodResponse * method_response)
{
	VortexChannel * channel      = NULL;
	int             msg_no       = -1;
	char          * reply_string = NULL;
	int             reply_size   = -1;
	VortexCtx     * ctx;

	/* check that the reply notification facility is received
	 * right data */
	if (method_response == NULL || method_call == NULL)
		return axl_false;

	/* get reply data from the given method call */
	__vortex_xml_rpc_method_call_get_reply_data (method_call, &channel, &msg_no);
	
	/* get a reference to the context */
	ctx = vortex_channel_get_ctx (channel);

	/* get the xml representation for the method reply */
	reply_string = vortex_xml_rpc_method_response_marshall (method_response, &reply_size);

	/* check if a marshalled reply was received */
	if (reply_string != NULL && reply_size > 0) {
		/* perform the reply */
		if (! vortex_channel_send_rpy (channel, reply_string, reply_size, msg_no)) 
			vortex_log (VORTEX_LEVEL_CRITICAL, "the XML-RPC reply was not possible to be performed");
	}else {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to perform method reply, response message marshalling have failed..");
	}
		
	/* release memory used */
	vortex_support_free (3, 
			     reply_string, axl_free,
			     method_call, vortex_xml_rpc_method_call_free,
			     method_response, vortex_xml_rpc_method_response_free);
	
	/* nothing more to do man! */
	vortex_log (VORTEX_LEVEL_DEBUG, "reply performed on channel %d, connection id=%d, ctx: %p..",
		    vortex_channel_get_number (channel), 
		    vortex_connection_get_id (vortex_channel_get_connection (channel)),
		    ctx);
	return axl_true;
}

/** 
 * @internal Handler used by vortex_xml_rpc_accept_negotiation.
 */
axl_bool  __vortex_xml_rpc_default_validate (VortexConnection * connection,
					     int                channel_number,
					     const char       * serverName,
					     const char       * resource_path,
					     axlPointer         user_data)
{
	/* default validation handler used by
	 * vortex_xml_rpc_accept_negotiation in the case no validation
	 * handler is provided */
	return axl_true;
}

/** 
 * @brief Allow to start receiving incoming XML-RPC request, setting
 * two handlers to validate and process them.
 *
 * This function is provided to allow Vortex BEEP listeners to accept
 * incoming XML-RPC invocations, by providing a resource validation
 * handler and the service dispatch handler.
 *
 * The first handler, the validation resource, is required to notify
 * remote peers that we accept, and support a particular resource. See
 * the \ref VortexXmlRpcValidateResource "validation resource handler"
 * to know more about this. This handler is not required. If NULL is
 * provided, all validation request will be accepted (in fact an
 * internal validation handler will be assigned which accept all
 * incoming requests).
 *
 * Once the resource validation process is accepted, the XML-RPC
 * channel is created, and XML-RPC incoming request can start.
 *
 * The second handler is provided to allow the user space to dispath
 * service invocation received. This handler is not optional. Without
 * it, the XML-RPC invocation can't be produced.
 * 
 * The function also register the XML-RPC profile to be advertised on
 * the greetings phase.
 *
 * Here is an example of a listener initializing the vortex engine to
 * accept incoming XML-RPC requests:
 * \code
 * // global context to be used
 * VortexCtx * ctx = NULL;
 *
 * axl_bool  validate_resource (VortexConnection * connection, 
 *                              int                channel_number,
 *                              char             * serverName,
 *                              char             * resource_path)
 * {
 *
 *	printf ("Resource to validate: '%s' (serverName='%s')\n", 
 *		 resource_path,
 *		 serverName != NULL ? 
 *               serverName : "no serverName value received");
 *	return axl_true;
 * }
 *
 * int  __sum_2_int_int (int  a, int  b, 
 *                       char  ** fault_error, int  * fault_code)
 * {
 *	if (a == 2 && b == 7) {
 *		// error reply example
 *		REPLY_FAULT ("Unable to sum the 2 and 7 values", -1, 0);
 *	}
 *	// return reply 
 *	return a + b;
 * }
 *
 * XmlRpcMethodResponse * sum_2_int_int (XmlRpcMethodCall * method_call)
 * {
 *	// get method values 
 *	int                 result;
 *	char              * fault_error = NULL;
 *	int                 fault_code  = -1;
 *
 *	// get parameters 
 *	int  a = method_call_get_param_value_as_int (method_call, 0);
 *	int  b = method_call_get_param_value_as_int (method_call, 1);
 *
 *	// perform invocation 
 *	result = __sum_2_int_int (a, b, &fault_error, &fault_code);
 *	
 *	// check error reply looking at the fault_error 
 *	if (fault_error != NULL) {
 *		// we have a error reply 
 *		return CREATE_FAULT_REPLY (fault_code, fault_error);
 *	}
 *
 *	// return reply generated 
 *	return CREATE_OK_REPLY (XML_RPC_INT_VALUE, INT_TO_PTR (result));
 * }
 *
 * XmlRpcMethodResponse *  service_dispatch (VortexChannel    * channel, 
 *                                           XmlRpcMethodCall * method_call, 
 *                                           axlPointer         user_data) 
 * {
 *	
 *	printf ("*** channel %d, method call received %s (params %d)***\n",
 *		 vortex_channel_get_number (channel),
 *		 method_call_get_name (method_call), 
 *		 method_call_get_num_params (method_call));
 *
 *	// check if the incoming method call is called sum, and has
 *	// two arguments 
 *	if (method_call_is (method_call, "sum", 2, 
 *                          XML_RPC_INT_VALUE, XML_RPC_INT_VALUE, -1)) {
 *		printf ("*** sum method found... ***\n");
 *		return sum_2_int_int (method_call);
 *		
 *	}
 *	       
 *	// return that the method to be invoked, is not supported 
 *	return CREATE_FAULT_REPLY (-1, "Method call it not supported by this server");
 * }
 *
 * int  main (int  argc, char  ** argv) 
 * {
 *
 *      // create an empty context 
 *      ctx = vortex_ctx_new ();
 *
 *      // init the context
 *      if (! vortex_init_ctx (ctx)) {
 *         printf ("failed to init the library..\n");
 *      } 
 *
 *	// enable XML-RPC profile 
 *	vortex_xml_rpc_accept_negotiation (ctx, validate_resource,
 *					   // no user space data for
 *					   // the validation resource
 *					   // function. 
 *					   NULL, 
 *					   service_dispatch,
 *					   // no user space data for
 *					   // the dispatch function. 
 *					   NULL);
 *
 *	// create a vortex server 
 *	vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);
 *
 *	// wait for listeners (until vortex_exit is called) 
 *	vortex_listener_wait (ctx);
 *	
 *	// end vortex function 
 *	vortex_exit_ctx (ctx, axl_true);
 *
 *	return 0;
 * }
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param validate_resource The resource validation handler. This
 * handler is not optional.
 *
 * @param validate_user_data Optional user data to be passed in to the
 * validation method.
 *
 * @param service_dispatch The service dispatch handler. This handler
 * is not optional.
 *
 * @param dispatch_user_data Optional user data to be passed in to the
 * dispatch method.
 *
 * @return axl_true if activation is complete, otherwise axl_false is
 * returned. 
 */
axl_bool            vortex_xml_rpc_accept_negotiation      (VortexCtx                    * ctx,
							    VortexXmlRpcValidateResource   validate_resource,
							    axlPointer                     validate_user_data,
							    VortexXmlRpcServiceDispatch    service_dispatch,
							    axlPointer                     dispatch_user_data)
							    
{
	VortexXmlRpcServiceDispatchNode * node;
	axlList                         * service_dispatch_nodes;

	/* validate incoming values */
	if (service_dispatch == NULL || ctx == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "passed in a null value for the service dispatcher");
		return axl_false;
	}

	/* call to init vortex xml-rpc module */
	if (! vortex_xml_rpc_init (ctx)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to start accepting incoming XML-RPC requests, failed to start xml-rpc library");
		return axl_false;
	} /* end if */

	/* get reference to the list */
	service_dispatch_nodes = vortex_ctx_get_data (ctx, VORTEX_XML_RPC_SERVICE_DISPATCH);

	/* store both handlers */
	node                = axl_new (VortexXmlRpcServiceDispatchNode, 1);
	node->dispatch      = service_dispatch;
	node->dispatch_data = dispatch_user_data;
	node->validate      = validate_resource != NULL ? validate_resource : __vortex_xml_rpc_default_validate;
	node->validate_data = validate_user_data;
	
	/* add the node to the list */
	axl_list_add (service_dispatch_nodes, node);

	/* enable XML-RPC profile */
	vortex_profiles_register (ctx, VORTEX_XML_RPC_PROFILE,
				  /* the start handler */
				  NULL, NULL, 
				  /* close handler */
				  NULL, NULL, 
				  /* frame received handler */
				  __vortex_xml_rpc_frame_received, NULL);
	
	/* now set the extended start message handler to process all
	 * incoming start messages received */
	vortex_profiles_register_extended_start (ctx, VORTEX_XML_RPC_PROFILE,
						 __vortex_xml_rpc_start_msg, NULL);

	return axl_true;
}

/** 
 * @brief Support function for xml-rpc listeners created by the
 * xml-rpc-gen tool, that reads a xml file that contains listener
 * information and starts the listener.
 * 
 * See also \ref vortex_listener_parse_conf_and_start.
 * 
 * @return axl_true if the listener was started because the file was read
 * successfully otherwise axl_false is returned.
 */
axl_bool                 vortex_xml_rpc_listener_parse_conf_and_start_listeners (VortexCtx * ctx)
{
	/* call to legacy implementation */
	return vortex_listener_parse_conf_and_start (ctx);
}

/** 
 * @brief (Un)Marshaller function used by the xml-rpc-gen tool, at the
 * client C stub generated used as \ref XmlRpcInvokeNotify function.
 * 
 * The function expect to receive a int reply, inside an \ref
 * XmlRpcMethodResponse, unmarshall it into a integer value and the
 * call to the process function, defined by \ref XmlRpcProcessInt
 * handler, that is received as an argument at <b>user_data</b>
 * parameter.
 *
 * @param channel The channel where the reply was received.
 *
 * @param response The method response received (\ref
 * XmlRpcMethodResponse).
 *
 * @param user_data The process reply, an user space callback, that
 * must fit the definition for \ref XmlRpcProcessInt.
 */
void vortex_xml_rpc_unmarshall_int    (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data)
{
	XmlRpcProcessInt      process = user_data;
	XmlRpcMethodValue   * value;
	XmlRpcResponseStatus  status  = method_response_get_status (response);
	int                   result;
	
	/* positive reply */
	if (status == XML_RPC_OK) {
		/* get the value inside */
		value  = method_response_get_value (response);
		result = method_value_get_as_int (value);
		
		/* notify reply received */
		process (result, status, -1, NULL);
	}else {
		/* notify error reply */
		process (-1, status, 
			 /* fault code */
			 method_response_get_fault_code (response),
			 /* fault string */
			 method_response_get_fault_string (response));
	}

	/* release memory used by the XmlRpcResponse */
	method_response_free (response);

	/* nothing more to do */
	return;
}


/** 
 * @internal
 * 
 * @param response The method response to get the value inside.
 *
 * @param status A reference to the method response status to fill
 * with the status received.
 *
 * @param fault_code A fault code reference to fill with the fault
 * code received or -1 if no error was detected.
 *
 * @param fault_string A fault string reference to fill with the fault
 * string received or NULL if no error was detected. The value
 * returned if defined, is dinamically allocated, so the caller must
 * deallocate it when no longer needed.
 * 
 * @return The method value inside or NULL if it fails. In that case,
 * variables received will be filled with error values.
 */
XmlRpcMethodValue * __vortex_xml_rpc_unmarshall_common_sync (XmlRpcMethodResponse  * response,
							     XmlRpcResponseStatus  * status, 
							     VortexChannel         * channel,
							     int                   * fault_code, 
							     char                 ** fault_string)
{
	XmlRpcResponseStatus   _status;
	char                 * string;
	/* get a reference to the context */
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx            * ctx = vortex_channel_get_ctx (channel);
#endif
	
	/* fill the status received */
	_status = method_response_get_status (response);

	/* report status */
	if (status != NULL)
		*status = _status;

	if (_status == XML_RPC_OK) {
		/* report fault code */
		if (fault_code != NULL)
			*fault_code   = -1;
		
		/* report fault string */
		if (fault_string != NULL)
			*fault_string = "No error reported";
		return method_response_get_value (response);
	}

	/* report fault code */
	if (fault_code != NULL) {
		*fault_code   = method_response_get_fault_code (response);
	}

	/* report fault string */
	if (fault_string != NULL) {
		string        = method_response_get_fault_string (response);
		if (string != NULL)
			string = axl_strdup (string);
		*fault_string = string;

		/* se the fault string into the channel hash to be
		 * automatically deallocated on channel close or on
		 * the next invocation. */
		if (string != NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "configuring automatic deallocation for: %s", string);
			vortex_channel_set_data_full (channel, string, string, axl_free, NULL);
		}
	}
	
	/* no method value */
	return NULL;
}

/** 
 * @brief Gets the integer value from the \ref XmlRpcMethodResponse,
 * setting the fault code and the fault string, releasing the \ref
 * XmlRpcMethodResponse passed in.
 *
 * @param response The XmlRpcMethodResponse to unmarshall.
 * 
 * @param status A reference to a XmlRpcResponseStatus to return value
 * stored in the method response.
 *
 * @param channel The channel where the XML-RPC message was received.
 *
 * @param fault_code The fault code reference to fill up the reference
 * if an error is found.
 *
 * @param fault_string The fault string reference to fill up if an
 * error is found. If the fault string variable is defined (on error
 * status: status != XML_RPC_OK) , it must be deallocated after using
 * it.
 * 
 * @return Returns the value store inside the method response (\ref
 * XmlRpcMethodResponse).
 */
int  vortex_xml_rpc_unmarshall_int_sync (XmlRpcMethodResponse  * response, 
					 XmlRpcResponseStatus  * status, 
					 VortexChannel         * channel,
					 int                   * fault_code,
					 char                 ** fault_string)
{
	XmlRpcMethodValue * value;
	int                 result = -1;
	
	/* get the value inside and status/fault_code/fault_string */
	value = __vortex_xml_rpc_unmarshall_common_sync (response, status, channel, fault_code, fault_string);
	
	/* if the value returned is not NULL, get the integer value */
	if (value != NULL)
		result = method_value_get_as_int (value);

	/* free response received */
	method_response_free (response);

	/* return result */
	return result;
}


/** 
 * @brief (Un)Marshaller function used by the xml-rpc-gen tool, at the
 * client C stub generated used as \ref XmlRpcInvokeNotify function.
 * 
 * The function expect to receive a string reply, inside an \ref
 * XmlRpcMethodResponse, unmarshall it into a string value and the
 * call to the process function, defined by \ref XmlRpcProcessString
 * handler, that is received as an argument at <b>user_data</b>
 * parameter.
 *
 * @param channel The channel where the reply was received.
 *
 * @param response The method response received (\ref
 * XmlRpcMethodResponse).
 *
 * @param user_data The process reply, an user space callback, that
 * must fit the definition for \ref XmlRpcProcessInt.
 */
void vortex_xml_rpc_unmarshall_string (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data)
{
	XmlRpcProcessString   process = user_data;
	XmlRpcMethodValue   * value;
	XmlRpcResponseStatus  status  = method_response_get_status (response);
	char                * result;
	
	/* positive reply */
	if (status == XML_RPC_OK) {
		/* get the value inside */
		value  = method_response_get_value (response);
		result = method_value_get_as_string (value);
		
		/* notify reply received */
		process (result, status, -1, NULL);
	}else {
		/* notify error reply */
		process (NULL, status, 
			 /* fault code */
			 method_response_get_fault_code (response),
			 /* fault string */
			 method_response_get_fault_string (response));
	}

	/* release memory used by the XmlRpcResponse */
	method_response_free (response);

	/* nothing more to do */
	return;
}

/** 
 * @brief Gets the string value from the \ref XmlRpcMethodResponse,
 * setting the fault code and the fault string, releasing the \ref
 * XmlRpcMethodResponse passed in.
 *
 * 
 * @param response The XmlRpcMethodResponse to unmarshall.
 * 
 * @param status A reference to a XmlRpcResponseStatus to return value
 * stored in the method response.
 *
 * @param channel The channel where the XML-RPC message was received.
 *
 * @param fault_code The fault code reference to fill up the reference
 * if an error is found.
 *
 * @param fault_string The fault string reference to fill up if an
 * error is found. If the fault string variable is defined (on error
 * status: status != XML_RPC_OK) , it must be deallocated after using
 * it.
 * 
 * @return Returns the value store inside the method response (\ref
 * XmlRpcMethodResponse).
 */
char  * vortex_xml_rpc_unmarshall_string_sync (XmlRpcMethodResponse  * response, 
					       XmlRpcResponseStatus  * status, 
					       VortexChannel         * channel,
					       int                   * fault_code,
					       char                 ** fault_string)
{
	XmlRpcMethodValue * value;
	char              * result = NULL;
	
	/* get the value inside and status/fault_code/fault_string */
	value = __vortex_xml_rpc_unmarshall_common_sync (response, status, channel, fault_code, fault_string);
	
	/* if the value returned is not NULL, get the integer value */
	if (value != NULL) {
		result = method_value_get_as_string (value);
	
		/* nullify the value inside so the response free function
		 * doesn't free the string returned */
		vortex_xml_rpc_method_value_nullify (value);
	}

	/* free response received */
	method_response_free (response);

	/* return result */
	return result;
}

/** 
 * @brief (Un)Marshaller function used by the xml-rpc-gen tool, at the
 * client C stub generated used as \ref XmlRpcInvokeNotify function.
 * 
 * The function expect to receive a double reply, inside an \ref
 * XmlRpcMethodResponse, unmarshall it into a double value and the
 * call to the process function, defined by \ref XmlRpcProcessDouble
 * handler, that is received as an argument at <b>user_data</b>
 * parameter.
 *
 * @param channel The channel where the reply was received.
 *
 * @param response The method response received (\ref
 * XmlRpcMethodResponse).
 *
 * @param user_data The process reply, an user space callback, that
 * must fit the definition for \ref XmlRpcProcessDouble.
 */
void vortex_xml_rpc_unmarshall_double (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data)
{
	XmlRpcProcessDouble   process = user_data;
	XmlRpcMethodValue   * value;
	XmlRpcResponseStatus  status  = method_response_get_status (response);
	double                result;
	
	/* positive reply */
	if (status == XML_RPC_OK) {
		/* get the value inside */
		value  = method_response_get_value (response);
		result = method_value_get_as_double (value);
		
		/* notify reply received */
		process (result, status, -1, NULL);
	}else {
		/* notify error reply */
		process (0, status, 
			 /* fault code */
			 method_response_get_fault_code (response),
			 /* fault string */
			 method_response_get_fault_string (response));
	}

	/* release memory used by the XmlRpcResponse */
	method_response_free (response);

	/* nothing more to do */
	return;	
}

/** 
 * @brief Gets the double value from the \ref XmlRpcMethodResponse,
 * setting the fault code and the fault string, releasing the \ref
 * XmlRpcMethodResponse passed in.
 *
 * 
 * @param response The XmlRpcMethodResponse to unmarshall.
 * 
 * @param status A reference to a XmlRpcResponseStatus to return value
 * stored in the method response.
 *
 * @param channel The channel where the XML-RPC message was received.
 *
 * @param fault_code The fault code reference to fill up the reference
 * if an error is found.
 *
 * @param fault_string The fault string reference to fill up if an
 * error is found. If the fault string variable is defined (on error
 * status: status != XML_RPC_OK) , it must be deallocated after using
 * it.
 * 
 * @return Returns the value store inside the method response (\ref
 * XmlRpcMethodResponse).
 */
double  vortex_xml_rpc_unmarshall_double_sync (XmlRpcMethodResponse  * response, 
					       XmlRpcResponseStatus  * status, 
					       VortexChannel         * channel,
					       int                   * fault_code,
					       char                 ** fault_string)
{
	XmlRpcMethodValue * value;
	double              result = 0;
	
	/* get the value inside and status/fault_code/fault_string */
	value = __vortex_xml_rpc_unmarshall_common_sync (response, status, channel, fault_code, fault_string);
	
	/* if the value returned is not NULL, get the integer value */
	if (value != NULL)
		result = method_value_get_as_double (value);

	/* free response received */
	method_response_free (response);

	/* return result */
	return result;
}


/** 
 * @brief (Un)Marshaller function used by the xml-rpc-gen tool, at the
 * client C stub generated used as \ref XmlRpcInvokeNotify function.
 * 
 * The function expect to receive a \ref XmlRpcStruct reply, inside an
 * \ref XmlRpcMethodResponse, unmarshall it into a \ref XmlRpcStruct
 * value and the call to the process function, defined by \ref
 * XmlRpcProcessStruct handler, that is received as an argument at
 * <b>user_data</b> parameter.
 *
 * @param channel The channel where the reply was received.
 *
 * @param response The method response received (\ref
 * XmlRpcMethodResponse).
 *
 * @param user_data The process reply, an user space callback, that
 * must fit the definition for \ref XmlRpcProcessStruct.
 */
void vortex_xml_rpc_unmarshall_struct (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data)
{
	XmlRpcProcessStruct   process = user_data;
	XmlRpcMethodValue   * value;
	XmlRpcResponseStatus  status  = method_response_get_status (response);
	XmlRpcStruct        * result;
	
	/* positive reply */
	if (status == XML_RPC_OK) {
		/* get the value inside */
		value  = method_response_get_value (response);
		result = method_value_get_as_struct (value);
		
		/* notify reply received */
		process (result, status, -1, NULL);
	}else {
		/* notify error reply */
		process (NULL, status, 
			 /* fault code */
			 method_response_get_fault_code (response),
			 /* fault string */
			 method_response_get_fault_string (response));
	}

	/* release memory used by the XmlRpcResponse */
	method_response_free (response);

	/* nothing more to do */
	return;
}

/** 
 * @brief Gets the struct value from the \ref XmlRpcMethodResponse,
 * setting the fault code and the fault string, releasing the \ref
 * XmlRpcMethodResponse passed in.
 *
 * The function will use the provided unmarshaller function to
 * generate a higher level structure result. This handler is
 * optional. In the case it is not provided, a reference to a \ref
 * XmlRpcStruct will be returned. 
 *
 * This unmarshaller function is mainly used by the xml-rpc-gen tool
 * to create a C client stub that is more suitable for the common use
 * (rather interfacing with \ref XmlRpcStruct).
 *
 * 
 * @param response The XmlRpcMethodResponse to unmarshall.
 *
 * @param unmarshaller User space, higher level unmarshaller function.
 *
 * @param channel The channel where the XML-RPC message was received.
 * 
 * @param status A reference to a XmlRpcResponseStatus to return value
 * stored in the method response.
 *
 * @param fault_code The fault code reference to fill up the reference
 * if an error is found.
 *
 * @param fault_string The fault string reference to fill up if an
 * error is found. If the fault string variable is defined (on error
 * status: status != XML_RPC_OK) , it must be deallocated after using
 * it.
 * 
 * @return Returns the value store inside the method response (\ref
 * XmlRpcMethodResponse).
 */
axlPointer vortex_xml_rpc_unmarshall_struct_sync (XmlRpcMethodResponse      * response, 
						  XmlRpcStructUnMarshaller    unmarshaller,
						  XmlRpcResponseStatus      * status, 
						  VortexChannel           * channel,
						  int                       * fault_code, 
						  char                     ** fault_string)
{
	XmlRpcMethodValue * value;
	XmlRpcStruct      * _struct = NULL;
	axlPointer          result;
	
	/* get the value inside and status/fault_code/fault_string */
	value = __vortex_xml_rpc_unmarshall_common_sync (response, status, channel, fault_code, fault_string);

	/* if the value returned is not NULL, get the integer value */
	if (value == NULL) {
		/* free response received, but after calling value */
		method_response_free (response);

		/* nothing to do, just return NULL */
		return NULL;
	}
	
	/* get the struct inside */
	_struct = method_value_get_as_struct (value);

	/* check if the unmarshaller function is provided */
	if (unmarshaller != NULL) {
		/* unmarshall */
		result = unmarshaller (_struct, axl_false);
	} else {
		/* return the structure */
		result = _struct;
	}

	/* free response received, but after calling value */
	method_response_free (response);

	/* return result */
	return result;
}

/** 
 * @brief (Un)Marshaller function used by the xml-rpc-gen tool, at the
 * client C stub generated used as \ref XmlRpcInvokeNotify function.
 * 
 * The function expect to receive a \ref XmlRpcArray reply, inside an
 * \ref XmlRpcMethodResponse, unmarshall it into a \ref XmlRpcArray
 * value and the call to the process function, defined by \ref
 * XmlRpcProcessArray handler, that is received as an argument at
 * <b>user_data</b> parameter.
 *
 * @param channel The channel where the reply was received.
 *
 * @param response The method response received (\ref
 * XmlRpcMethodResponse).
 *
 * @param user_data The process reply, an user space callback, that
 * must fit the definition for \ref XmlRpcProcessArray.
 */
void vortex_xml_rpc_unmarshall_array (VortexChannel *channel, 
				      XmlRpcMethodResponse *response, 
				      axlPointer user_data)
{
	XmlRpcProcessArray    process = user_data;
	XmlRpcMethodValue   * value;
	XmlRpcResponseStatus  status  = method_response_get_status (response);
	XmlRpcArray         * result;
	
	/* positive reply */
	if (status == XML_RPC_OK) {
		/* get the value inside */
		value  = method_response_get_value (response);
		result = method_value_get_as_array (value);
		
		/* notify reply received */
		process (result, status, -1, NULL);
	}else {
		/* notify error reply */
		process (NULL, status, 
			 /* fault code */
			 method_response_get_fault_code (response),
			 /* fault string */
			 method_response_get_fault_string (response));
	}

	/* release memory used by the XmlRpcResponse */
	method_response_free (response);

	/* nothing more to do */
	return;	
}

/** 
 * @brief Gets the array value from the \ref XmlRpcMethodResponse,
 * setting the fault code and the fault string, releasing the \ref
 * XmlRpcMethodResponse passed in.

 * The function will use the provided unmarshaller function to
 * generate a higher level structure result. This handler is
 * optional. In the case it is not provided, a reference to a \ref
 * XmlRpcArray will be returned. 
 *
 * This unmarshaller function is mainly used by the xml-rpc-gen tool
 * to create a C client stub that is more suitable for the common use
 * (rather interfacing with \ref XmlRpcStruct).
 *
 * 
 * @param response The XmlRpcMethodResponse to unmarshall.
 *
 * @param unmarshaller User space, higher level unmarshaller function.
 *
 * @param channel The channel where the XML-RPC message was received.
 * 
 * @param status A reference to a XmlRpcResponseStatus to return value
 * stored in the method response.
 *
 * @param fault_code The fault code reference to fill up the reference
 * if an error is found.
 *
 * @param fault_string The fault string reference to fill up if an
 * error is found. If the fault string variable is defined (on error
 * status: status != XML_RPC_OK) , it must be deallocated after using
 * it.
 * 
 * @return Returns the value store inside the method response (\ref
 * XmlRpcMethodResponse).
 */
axlPointer vortex_xml_rpc_unmarshall_array_sync (XmlRpcMethodResponse    * response, 
						 XmlRpcArrayUnMarshaller   unmarshaller,
						 XmlRpcResponseStatus    * status, 
						 VortexChannel           * channel,
						 int                     * fault_code,
						 char                   ** fault_string)
{
	XmlRpcMethodValue * value;
	XmlRpcArray       * result = NULL;
	axlPointer          higher_result;
	
	/* get the value inside and status/fault_code/fault_string */
	value = __vortex_xml_rpc_unmarshall_common_sync (response, status, channel, fault_code, fault_string);

	/* check method value received */
	if (value == NULL) {
		/* free response received, but after calling value */
		method_response_free (response);

		/* nothing to do, just return NULL */
		return NULL;
	}

	/* translate the array received */
	result = method_value_get_as_array (value);

	/* check if the unmarshaller function is provided */
	if (unmarshaller != NULL) {
		/* unmarshall */
		higher_result = unmarshaller (result, axl_false);

		/* free response received */
		method_response_free (response);

		/* return the higher result */
		return higher_result;
	}

	/* free response received */
	method_response_free (response);

	/* return result */
	return result;
}


/** 
 * @brief Inits the vortex xml-rpc module state.
 *
 * In the case XML-RPC over BEEP implementation is used, a call to
 * this function is required to start its internal function.
 *
 * Once finished, a call to \ref vortex_xml_rpc_cleanup can be used to
 * ensure the XML-RPC component terminates its function and dealloc
 * all resources used.
 * 
 * @param ctx The context where the module state will be initialized.
 *
 * @return axl_true if the initialization was properly done, otherwise
 * axl_false is returned.
 */
axl_bool  vortex_xml_rpc_init    (VortexCtx * ctx)
{
	axlList * service_dispatch;
	axlDtd  * dtd;

	v_return_val_if_fail (ctx, axl_false);

	/* check if the xml-rpc module was already initialized */
	service_dispatch  = vortex_ctx_get_data (ctx, VORTEX_XML_RPC_SERVICE_DISPATCH);
	if (service_dispatch != NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "xml-rpc module was already initialized, return as initialized");
		return axl_true;
	} /* end if */

	/* create the service dispatch list */
	service_dispatch  = axl_list_new (axl_list_always_return_1, axl_free);
	vortex_ctx_set_data_full (ctx, 
				  /* key and value */
				  VORTEX_XML_RPC_SERVICE_DISPATCH, service_dispatch,
				  /* key and value destroy functions */
				  NULL, (axlDestroyFunc) axl_list_free);
	
	/* load xml-rpc boot dtd definition */
	if (!vortex_dtds_load_dtd (ctx, &dtd, XML_RPC_BOOT_DTD)) {
                fprintf (stderr, "VORTEX_ERROR: unable to load inline xml-rpc-boot.dtd file.\n");
		return axl_false;
        } /* end if */
	vortex_ctx_set_data_full (ctx,
				  /* key and value */
				  VORTEX_XML_RPC_BOOT_DTD, dtd,
				  /* key and value destroy function */
				  NULL, (axlDestroyFunc) axl_dtd_free);
	return axl_true;
}

/** 
 * @brief Terminates the xml-rpc module state from the provided
 * context.
 * 
 * @param ctx The context where the xml-rpc module state will be
 * cleared.
 */
void vortex_xml_rpc_cleanup (VortexCtx * ctx)
{
	v_return_if_fail (ctx);

	/* free the list (setting null value will call to destroy
	 * function) */
	vortex_ctx_set_data (ctx, VORTEX_XML_RPC_SERVICE_DISPATCH, NULL);
	
	/* free dtd */
	vortex_ctx_set_data (ctx, VORTEX_XML_RPC_BOOT_DTD, NULL);
	
	return;
}

/* @} */

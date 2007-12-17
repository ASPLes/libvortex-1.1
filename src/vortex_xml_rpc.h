/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
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
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */

#include <vortex_xml_rpc_types.h>

#ifndef __VORTEX_XML_RPC_H__
#define __VORTEX_XML_RPC_H__



/**
 * \addtogroup vortex_xml_rpc  
 * @{ 
 */

/** 
 * @brief Unique uri profile identificador for the XML-RPC.
 */
#define VORTEX_XML_RPC_PROFILE "http://iana.org/beep/xmlrpc"

/** 
 * @brief String used to flag xml-rpc channel status.
 */
#define XML_RPC_BOOT_STATE "vortex-xml-rpc:state"

/** 
 * @brief String used to flag xml-rpc channel resource used to boot.
 */
#define XML_RPC_RESOURCE "vortex-xml-rpc:resource"

/** 
 * @brief Enum value to represent XML-RPC channel initialization
 * status.
 */
typedef enum {
	/** 
	 * @brief Value representing an error while getting current
	 * XML-RPC channel status, either because the channel checked
	 * is not properly created or it is created under a different
	 * profile than XML-RPC profile.
	 */
	XmlRpcStateUnknown,
	/** 
	 * @brief The XML-RPC channel is running under boot state,
	 * that is, it is still not possible to perform XML-RPC,
	 * resource initialization wasn't done yet.
	 */
	XmlRpcStateBoot,
	/** 
	 * @brief The XML-RPC channel is running under ready state,
	 * that is, it is ready to perform XML-RPC requests.
	 */
	XmlRpcStateReady,
}VortexXmlRpcState;

bool                vortex_xml_rpc_is_enabled              ();

void                vortex_xml_rpc_boot_channel            (VortexConnection        * connection,
							    const char              * serverName,
							    const char              * resourceName,
							    VortexXmlRpcBootNotify    process_status,
							    axlPointer                  user_data);

/** 
 * @brief Allows to boot an XML-RPC channel, in a synchronous way,
 * calling to \ref vortex_xml_rpc_boot_channel_sync.
 *
 * The macro will create an XML-RPC channel under the provided
 * resource (res). The resource value could be null, so the default
 * will be used: "/".
 *
 * The macro will pass null values for the status and the status
 * message. If it is required to get a complete error reporting you
 * must use: \ref vortex_xml_rpc_boot_channel_sync. Here is an
 * example:
 *
 * \code
 * // create an XML-RPC channel under the resource "/"
 * VortexChannel * channel = BOOT_CHANNEL(connection, NULL);
 *
 * // check the result 
 * if (channel == NULL) {
 *      printf ("Something have failed..\n");
 * }
 * \endcode
 * 
 * @param connection The connection where the XML-RPC channel will be
 * created.
 *
 * @param res The resource value or NULL if it fails.
 * 
 * @return The channel created and already booted or NULL if it fails.
 */
#define BOOT_CHANNEL(connection, res) (vortex_xml_rpc_boot_channel_sync(connection, NULL, res, NULL, NULL))

VortexChannel     * vortex_xml_rpc_boot_channel_sync       (VortexConnection        * connection,
							    const char              * serverName,
							    const char              * resourceName,
							    VortexStatus            * status,
							    char                   ** status_message);

VortexChannelPool * vortex_xml_rpc_create_channel_pool     (VortexConnection            * connection,
							    const char                  * serverName,
							    const char                  * resourceName,
							    VortexOnChannelPoolCreated    on_pool_created,
							    axlPointer                    user_data);

VortexChannel     * vortex_xml_rpc_channel_pool_get_next   (VortexConnection * connection, 
							    bool               auto_inc, 
							    int                pool_id);


bool                 vortex_xml_rpc_invoke               (VortexChannel           * channel,
							  XmlRpcMethodCall        * method_call,
							  XmlRpcInvokeNotify        reply_notify,
							  axlPointer                user_data);

bool                   vortex_xml_rpc_notify_reply         (XmlRpcMethodCall        * method_call, 
							    XmlRpcMethodResponse    * method_response);

XmlRpcMethodResponse * vortex_xml_rpc_invoke_sync          (VortexChannel           * channel,
							    XmlRpcMethodCall        * method_call);
							    

VortexXmlRpcState   vortex_xml_rpc_channel_status          (VortexChannel * channel);

const char        * vortex_xml_rpc_channel_get_resource    (VortexChannel * channel);

bool                vortex_xml_rpc_accept_negociation      (VortexCtx                    * ctx,
							    VortexXmlRpcValidateResource   validate_resource,
							    axlPointer                     validate_user_data,
							    VortexXmlRpcServiceDispatch    service_dispatch,
							    axlPointer                     dispatch_user_data);

bool                vortex_xml_rpc_listener_parse_conf_and_start_listeners (VortexCtx * ctx);

/** 
 * @internal
 * 
 * Internal prototype declaration to support a reference from
 * __vortex_xml_rpc_parse_struct_value to this function.
 */
XmlRpcMethodValue * __vortex_xml_rpc_parse_value (VortexCtx * ctx, axlNode * value);

/* int unmarshallers */
void vortex_xml_rpc_unmarshall_int      (VortexChannel *channel, 
					 XmlRpcMethodResponse *response, 
					 axlPointer user_data);

int   vortex_xml_rpc_unmarshall_int_sync (XmlRpcMethodResponse  * response, 
					  XmlRpcResponseStatus  * status, 
					  VortexChannel         * channel,
					  int                   * fault_code,
					  char                 ** fault_string);

/* string unmarshallers */
void vortex_xml_rpc_unmarshall_string      (VortexChannel *channel, 
					    XmlRpcMethodResponse *response, 
					    axlPointer user_data);

char  * vortex_xml_rpc_unmarshall_string_sync (XmlRpcMethodResponse  * response, 
					       XmlRpcResponseStatus  * status, 
					       VortexChannel         * channel,
					       int                   * fault_code,
					       char                 ** fault_string);

/* double unmarshallers */
void vortex_xml_rpc_unmarshall_double (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data);

double  vortex_xml_rpc_unmarshall_double_sync (XmlRpcMethodResponse  * response, 
					       XmlRpcResponseStatus  * status, 
					       VortexChannel         * channel,
					       int                   * fault_code,
					       char                 ** fault_string);

/* struct unmarshallers */
void vortex_xml_rpc_unmarshall_struct (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data);

axlPointer vortex_xml_rpc_unmarshall_struct_sync (XmlRpcMethodResponse     * response, 
						  XmlRpcStructUnMarshaller   unmarshaller,
						  XmlRpcResponseStatus     * status, 
						  VortexChannel            * channel,
						  int                      * fault_code,
						  char                    ** fault_string);

/* array unmarshallers */
void vortex_xml_rpc_unmarshall_array  (VortexChannel *channel, 
				       XmlRpcMethodResponse *response, 
				       axlPointer user_data);


axlPointer vortex_xml_rpc_unmarshall_array_sync (XmlRpcMethodResponse    * response, 
						 XmlRpcArrayUnMarshaller   unmarshaller,
						 XmlRpcResponseStatus    * status, 
						 VortexChannel           * channel,
						 int                     * fault_code,
						 char                   ** fault_string);

void vortex_xml_rpc_init    (VortexCtx * ctx);

void vortex_xml_rpc_cleanup (VortexCtx * ctx);

#endif

/* @} */

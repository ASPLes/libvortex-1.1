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
#ifndef __VORTEX_TUNNEL_H__
#define __VORTEX_TUNNEL_H__

#include <vortex.h>

BEGIN_C_DECLS

/** 
 * \addtogroup vortex_tunnel
 * @{
 */

/**
 * @brief Opaque type representing a proxy configuration used by the
 * Vortex Engine to create a new connection to a remote point through
 * an application layer gateway implementing the TUNNEL profile.
 *
 * See for more details: 
 *  
 *   - \ref vortex_tunnel_settings_new
 *   - \ref vortex_tunnel_settings_add_hop
 * 
 */
typedef struct _VortexTunnelSettings VortexTunnelSettings;

/**
 * @brief Proxy TUNNEL item configuration. 
 * 
 * Each item provided allow to configure a particular value conforming
 * a next hop BEEP proxy to be traversed to reach the destination.
 * 
 * See \ref vortex_tunnel_new for more details about using this
 * enumerations.
 */

typedef enum {
	/**
	 * @brief Beacon used by the \ref
	 * vortex_tunnel_settings_add_hop function to signal that no
	 * more configuration items are expected.
	 */
	TUNNEL_END_CONF = 1,
	/**
	 * @brief Full qualified domain name to be configured a
	 * particular hop. 
	 */
	TUNNEL_FQDN = 2,
	/**
	 * @brief Tcp end point port for the hop configuration.
	 */
	TUNNEL_PORT = 3,
	/**
	 * @brief IP version 4 address for the hop configuration.
	 */
	TUNNEL_IP4 = 4,
	/**
	 * @brief IP version 6 address for the hop configuration.
	 */
	TUNNEL_IP6 = 5,
	/**
	 * @brief DNS service record configuration.
	 */
	TUNNEL_SRV = 6,
	/**
	 * @brief An URI value to be proxied.
	 * 
	 * Under normal operations a connection created through a BEEP
	 * proxy is configured with \ref TUNNEL_IP4 and \ref
	 * TUNNEL_PORT values. However, TUNNEL profile also allows
	 * requesting to proxy a particular profile, letting the
	 * decision to map that "profile" to a particular host
	 * location to the BEEP proxy. 
	 *
	 * How is mapped the "profile" request to a particular host
	 * location is a matter of provisioning for each BEEP TUNNEL
	 * server.
	 *
	 * This value is only allowed to define the remote endpoint to
	 * connect to.
	 */
	TUNNEL_URI = 7,
	/**
	 * @brief An endpoint configuration, a user defined string
	 * configured by the provision of each BEEP proxy traversed.
	 *
	 * This value provides an alternative hop configuration to
	 * create tunnels. It allows to configure a final destination
	 * providing a string description like: "application server",
	 * "console operator", etc.
	 *
	 * How is mapped the "profile" request to a particular host
	 * location is a matter of provisioning for each BEEP TUNNEL
	 * server.
	 *
	 * This value is only allowed to define the remote endpoint to
	 * connect to.
	 */
	TUNNEL_ENDPOINT = 8
}VortexTunnelItem;

/** 
 * @brief Handler definition for the tunnel location resolution.
 *
 * This handler is used by the TUNNEL implementation to provide a way
 * to the user space code to translate tunnel locations provided. 
 *
 * Currently this is used by Turbulence to provide run-time
 * translation for endpoint and profile configurations into host and
 * port locations.
 * 
 * @param tunnel_spec The xml string defining the tunnel spec as
 * defined in RFC3620.
 * 
 * @param tunnel_sepc_size The size of the xml content.
 *
 * @param user_data Reference to user defined data.
 *
 * @param doc A reference to an already parsed document. 
 * 
 * @return A reference to the \ref VortexTunnelSettings created with
 * the new values. If null reference is returned, the TUNNEL engine
 * will use the content as provided, without performing any
 * translation.
 */
typedef VortexTunnelSettings * (* VortexTunnelLocationResolver) (const char  * tunnel_spec,
								 int           tunnel_spec_size,
								 axlDoc      * tunnel_doc,
								 axlPointer    user_data);

/** 
 * @brief Uri reference to the BEEP TUNNEL profile identifier.
 */
#define TUNNEL_PROFILE "http://iana.org/beep/TUNNEL"

#include <vortex.h>

VortexTunnelSettings * vortex_tunnel_settings_new       (VortexCtx * ctx);

void                   vortex_tunnel_settings_set_options (VortexTunnelSettings * settings,
							   VortexConnectionOpts * options);

VortexTunnelSettings * vortex_tunnel_settings_new_from_xml (VortexCtx * ctx,
							    char      * content, 
							    int         size);

void                   vortex_tunnel_settings_add_hop   (VortexTunnelSettings * settings,
							 ...);

void                   vortex_tunnel_settings_free      (VortexTunnelSettings * settings);

VortexConnection     * vortex_tunnel_new                (VortexTunnelSettings * settings, 
							 VortexConnectionNew    on_connected,
							 axlPointer             user_data);

int                    vortex_tunnel_accept_negotiation (VortexCtx                  * ctx,
							 VortexOnAcceptedConnection   accept_tunnel,
							 axlPointer                   accept_tunnel_data);

void                   vortex_tunnel_set_resolver       (VortexCtx                    * ctx,
							 VortexTunnelLocationResolver   resolver,
							 axlPointer                     resolver_data);

/* private api, do not use directly (use vortex_tunnel_new) */
VortexConnection     * __vortex_tunnel_new_common       (VortexTunnelSettings * settings,
							 axl_bool               do_tunning_rest,
							 VortexConnectionNew    on_connected,
							 axlPointer             user_data);

END_C_DECLS

#endif

/* @} */

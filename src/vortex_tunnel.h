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
#ifndef __VORTEX_TUNNEL_H__
#define __VORTEX_TUNNEL_H__

/**
 * \addtogroup vortex_tunnel
 * @{
 */

/** 
 * @brief Uri reference to the BEEP TUNNEL profile identifier.
 */
#define TUNNEL_PROFILE "http://iana.org/beep/TUNNEL"

#include <vortex.h>

bool                   vortex_tunnel_is_enabled         ();

VortexTunnelSettings * vortex_tunnel_settings_new       ();

VortexTunnelSettings * vortex_tunnel_settings_new_from_xml (char * content, 
							    int    size);

void                   vortex_tunnel_settings_add_hop   (VortexTunnelSettings * settings,
							 ...);

void                   vortex_tunnel_settings_free      (VortexTunnelSettings * settings);

VortexConnection     * vortex_tunnel_new                (VortexTunnelSettings * settings, 
							 VortexConnectionNew    on_connected,
							 axlPointer             user_data);

bool                   vortex_tunnel_accept_negotiation (VortexOnAcceptedConnection accept_tunnel,
							 axlPointer                 accept_tunnel_data);

void                   vortex_tunnel_set_resolver       (VortexTunnelLocationResolver resolver,
							 axlPointer                   resolver_data);

/* private api, do not use directly (use vortex_tunnel_new) */
VortexConnection     * __vortex_tunnel_new_common       (VortexTunnelSettings * settings,
							 bool                   do_tunning_rest,
							 VortexConnectionNew    on_connected,
							 axlPointer             user_data);
#endif

/* @} */

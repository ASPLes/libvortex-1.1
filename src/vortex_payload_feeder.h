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

#ifndef __VORTEX_PAYLOAD_FEEDER_H__
#define __VORTEX_PAYLOAD_FEEDER_H__

#include <vortex.h>

/** 
 * \addtogroup vortex_payload_feeder Vortex Payload Feeder: an abstraction to efficiently feed content into vortex sending engine
 * @{
 */

VortexPayloadFeeder * vortex_payload_feeder_new (VortexPayloadFeederHandler   handler,
						 axlPointer                   user_data);

VortexPayloadFeeder * vortex_payload_feeder_file (const char * path, 
						  axl_bool     add_mime_head);

int                   vortex_payload_feeder_get_pending_size (VortexPayloadFeeder * feeder);

int                   vortex_payload_feeder_get_content (VortexPayloadFeeder * feeder,
							 int                   size_to_copy, 
							 char                * buffer);

axl_bool              vortex_payload_feeder_is_finished (VortexPayloadFeeder * feeder);

void                  vortex_payload_feeder_set_on_finished (VortexPayloadFeeder                * feeder,
							     VortexPayloadFeederFinishedHandler   on_finished,
							     axlPointer                           user_data);

axl_bool              vortex_payload_feeder_pause       (VortexPayloadFeeder * feeder,
							 axl_bool              close_transfer);

void                  vortex_payload_feeder_status      (VortexPayloadFeeder       * feeder, 
							 VortexPayloadFeederStatus * status);

axl_bool              vortex_payload_feeder_ref         (VortexPayloadFeeder * feeder);

void                  vortex_payload_feeder_unref       (VortexPayloadFeeder * feeder);

void                  vortex_payload_feeder_free        (VortexPayloadFeeder * feeder);

#endif

/** 
 * @}
 */

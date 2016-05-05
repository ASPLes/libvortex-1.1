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
#ifndef __VORTEX_PROFILES_H__
#define __VORTEX_PROFILES_H__

#include <vortex.h>


axl_bool vortex_profiles_register                (VortexCtx             * ctx,
						  const char            * uri,
						  VortexOnStartChannel    start,
						  axlPointer              start_user_data,
						  VortexOnCloseChannel    close,
						  axlPointer              close_user_data,
						  VortexOnFrameReceived   received,
						  axlPointer              received_user_data);

axl_bool vortex_profiles_unregister              (VortexCtx             * ctx,
						  const char            * uri);

axl_bool vortex_profiles_set_received_handler    (VortexCtx             * ctx,
						  const char            * uri,
						  VortexOnFrameReceived   received,
						  axlPointer              user_data);
	
axl_bool vortex_profiles_set_mime_type           (VortexCtx             * ctx,
						  const char            * uri,
						  const char            * mime_type,
						  const char            * transfer_encoding);

const char   * vortex_profiles_get_mime_type           (VortexCtx       * ctx,
							const char      * uri);

const char   * vortex_profiles_get_transfer_encoding   (VortexCtx        * ctx,
							const char       * uri);

axl_bool  vortex_profiles_register_extended_start (VortexCtx                    * ctx,
						   const char                   * uri,
						   VortexOnStartChannelExtended   extended_start,
						   axlPointer                     extended_start_user_data);

axl_bool  vortex_profiles_invoke_start            (const char         * uri, 
						   int                  channel_num, 
						   VortexConnection   * connection,
						   const char         * serverName, 
						   const char         * profile_content, 
						   char              ** profile_content_reply, 
						   VortexEncoding       encoding);

axl_bool  vortex_profiles_is_defined_start        (VortexCtx   * ctx,
						   const char  * uri);

axl_bool  vortex_profiles_invoke_close            (char  * uri,
						   int  channel_nu,
						   VortexConnection * connection);

axl_bool  vortex_profiles_is_defined_close        (VortexCtx   * ctx,
						   const char  * uri);

axl_bool  vortex_profiles_invoke_frame_received   (const char       * uri,
						   int                channel_num,
						   VortexConnection * connection,
						   VortexFrame      * frame);

axl_bool  vortex_profiles_is_defined_received     (VortexCtx        * ctx,
						   const char       * uri);

axlList * vortex_profiles_get_actual_list         (VortexCtx        * ctx);

axlList * vortex_profiles_get_actual_list_ref     (VortexCtx        * ctx);

axl_bool  vortex_profiles_has_profiles            (VortexCtx        * ctx);

axlList * vortex_profiles_acquire                 (VortexCtx        * ctx);

void      vortex_profiles_release                 (VortexCtx        * ctx);

int       vortex_profiles_registered              (VortexCtx        * ctx);

axl_bool  vortex_profiles_is_registered           (VortexCtx        * ctx,
						   const char       * uri);

void      vortex_profiles_set_automatic_mime      (VortexCtx        * ctx,
						   const char       * uri, 
						   int                value);

int       vortex_profiles_get_automatic_mime      (VortexCtx        * ctx,
						   const char       * uri);

void      vortex_profiles_init                    (VortexCtx   * ctx);

void      vortex_profiles_cleanup                 (VortexCtx   * ctx);


#endif

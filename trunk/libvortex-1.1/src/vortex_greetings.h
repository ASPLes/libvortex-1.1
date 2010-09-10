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
#ifndef __VORTEX_GREETINGS_H__
#define __VORTEX_GREETINGS_H__

#include <vortex.h>

axl_bool       vortex_greetings_send           (VortexConnection     * connection, 
						VortexConnectionOpts * options);

axl_bool       vortex_greetings_is_reply_ok    (VortexFrame          * frame,
						VortexConnection     * connection,
						VortexConnectionOpts * options);

VortexFrame *  vortex_greetings_process        (VortexConnection     * connection,
						VortexConnectionOpts * options);

axl_bool       vortex_greetings_client_send    (VortexConnection     * connection,
						VortexConnectionOpts * options);

VortexFrame *  vortex_greetings_client_process (VortexConnection     * connection,
						VortexConnectionOpts * options);

void           vortex_greetings_error_send     (VortexConnection     * connection,
						const char           * xml_lang,
						const char           * code,
						const char           * message,
						...);

void           vortex_greetings_set_features   (VortexCtx        * ctx,
						const char       * feature);

const char  *  vortex_greetings_get_features   (VortexCtx        * ctx);

void           vortex_greetings_set_localize   (VortexCtx        * ctx,
						const char       * localize);

const char  *  vortex_greetings_get_localize   (VortexCtx        * ctx);

void           vortex_greetings_cleanup        (VortexCtx        * ctx);

/** 
 * @internal Definition to handle pending partial frame received on
 * greetings.
 */
#define VORTEX_GREETINGS_PENDING_FRAME "vo:gr:pe"

#endif

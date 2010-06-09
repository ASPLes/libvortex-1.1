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
#ifndef __VORTEX_ALIVE_H__
#define __VORTEX_ALIVE_H__

#include <vortex.h>

BEGIN_C_DECLS

/** 
 * \addtogroup vortex_alive 
 * @{ 
 */

/** 
 * @brief ALIVE Profile unique URI identifier.
 */
#define VORTEX_ALIVE_PROFILE_URI "urn:aspl.es:beep:profiles:ALIVE"

/** 
 * @brief Alive handler that is called when an alive test failure was
 * detected on the provided connection. This handler is configured
 * vortex_alive_enable_check. The handler receives the check period
 * that was configured and how many unreplied messages are until
 * handler is called.
 *
 * @param conn The connection where the alive failure was detected.
 *
 * @param check_period The check period that was configured at \ref
 * vortex_alive_enable_check.
 *
 * @param unreplied_count How many unreplied messages so far.
 */
typedef void (*VortexAliveFailure) (VortexConnection * conn, 
				    long               check_period, 
				    int                unreply_count);

axl_bool           vortex_alive_init                       (VortexCtx * ctx);

axl_bool           vortex_alive_enable_check               (VortexConnection * conn,
							    long               check_period,
							    int                max_unreply_count,
							    VortexAliveFailure failure_handler);

END_C_DECLS

#endif
/* @} */

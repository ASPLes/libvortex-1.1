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

/** 
 * \defgroup vortex_tls Vortex ALIVE: A profile extension that allows to check peer alive status
 */

/** 
 * \addtogroup vortex_tls
 * @{
 */

#include <vortex_alive.h>

void vortex_alive_frame_received (VortexChannel    * channel,
				  VortexConnection * conn,
				  VortexFrame      * frame,
				  axlPointer         user_data)
{
	/* check for incoming alive requests */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) 
		vortex_channel_send_msg (channel, "", 0, NULL);
	
	return;
}

/** 
 * @brief Allows to init alive module on the current context. This is
 * required to receive alive requests, allowing remote peer to check
 * if we are reachable.
 *
 * @param ctx The context where the alive profile will be enabled. 
 *
 */
axl_bool           vortex_alive_init                       (VortexCtx * ctx)
{
	/* register profile */
	vortex_profiles_register (ctx,
				  VORTEX_ALIVE_PROFILE_URI,
				  /* on start */
				  NULL, NULL,
				  /* on close */
				  NULL, NULL,
				  /* on frame received */
				  vortex_alive_frame_received, NULL);
	return axl_true;
}

/** 
 * @brief Allows to enable alive supervision on the provided connection.
 *
 * The function enables a permanent test every <b>check_period</b> to
 * ensure the connection is working. The check is implemented by a
 * simply request/reply with an unreplied tracking to ensure we detect
 * connection lost even when power failures or network cable unplug.
 *
 * The function accepts a max_unreply_count which usually is 0 (no
 * unreplied messages accepted) or the amount of unreplied messages we
 * are accepting until calling to failure handler or shutdown the
 * connection.
 *
 * The failure_handler is optional. If not provided, the alive check
 * will call to shutdown the connection (\ref
 * vortex_connection_shutdown), activating connection on close
 * configured (\ref vortex_connection_set_on_close_full) so close
 * handling is unified on the same place. 
 *
 * However, if failure_handler is defined, the alive check will not
 * shutdown the connection and will call the handler.
 *
 * @param conn The connection where the check will be enabled.
 *
 * @param check_period The check period. To check every 20ms a
 * connection pass 20000.
 *
 * @param max_unreply_count The maximum amount of unreplied messages we accept.
 *
 * @param failure_handler Optional handler called when a failure is
 * detected.
 */
void               vortex_alive_enable_check               (VortexConnection * conn,
							    long               check_period,
							    int                max_unreply_count,
							    VortexAliveFailure failure_handler)
{
	return;
}


/* @} */

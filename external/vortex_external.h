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
#ifndef __VORTEX_EXTERNAL_H__
#define __VORTEX_EXTERNAL_H__

#include <vortex.h>

BEGIN_C_DECLS

/** 
 * \addtogroup vortex_external
 * @{
 */

/** 
 * @brief Connection setup object. Allows to configure additional
 * settings required to perform a connection by using \ref
 * vortex_external_connection_new.
 */
typedef struct _VortexExternalSetup VortexExternalSetup;

/** 
 * @brief On prepare handler, a handler called after the connection
 * (\ref VortexConnection) was created and before any operation is
 * done with the connection.
 *
 * This handler is configured through configuration key: \ref VORTEX_EXTERNAL_ON_PREPARE_HANDLER
 *
 * \code
 * vortex_external_setup_conf (setup, VORTEX_EXTERNAL_ON_PREPARE_HANDLER, __on_prepare_handler);
 * \endcode
 *
 * @param ctx The context where to operate
 *
 * @param conn The connection that was created
 *
 * @param user_data User data configured with \ref VORTEX_EXTERNAL_ON_PREPARE_USER_DATA
 *
 * @param user_data2 User data configured with \ref VORTEX_EXTERNAL_ON_PREPARE_USER_DATA2
 *
 */
typedef void (*VortexExternalOnPrepare) (VortexCtx * ctx, VortexConnection * conn, axlPointer user_data, axlPointer user_data2);

/** 
 * @brief Configures the handler that will be called to accept the
 * incoming socket on the provided listener.
 *
 * @param ctx The context where the operation happens
 *
 * @param listener The listener where the connection was received
 *
 * @param on_accept_data User defined pointer configured at \ref vortex_external_listener_new
 *
 * @return The function returns a working socket accepted otherwise
 * returns VORTEX_INVALID_SOCKET or -1.
 */
typedef VORTEX_SOCKET (*VortexExternalOnAccept) (VortexCtx * ctx, VortexConnection * listener, VORTEX_SOCKET _listener_socket, axlPointer on_accept_data);

/** 
 * @brief Configurations allowed to be set on \ref VortexExternalSetup.
 */
typedef enum {
	/** 
	 * @brief Allows to configure an internal mutex to ensure
	 * VortexSendHandler and VortexReceiveHandler that are
	 * configured aren't called at the same time. By default, if
	 * this is not provided, those handlers can be called at any
	 * time even at the same time.
	 *
	 * \code
	 * // make receive and send operation to be mutually exclusive
	 * vortex_external_setup_conf (setup, VORTEX_EXTERNAL_CONF_MUTEX_IO, PTR_TO_INT (axl_true));
	 * \endcode
	 */
	VORTEX_EXTERNAL_CONF_MUTEX_IO = 1,
	/** 
	 * @brief Allows to configure the host name that should be
	 * reported by this connection. Functions like \ref
	 * vortex_connection_get_host will report this.
	 *
	 * \code
	 * // make receive and send operation to be mutually exclusive
	 * vortex_external_setup_conf (setup, VORTEX_EXTERNAL_CONF_HOST, (axlPointer) "host.external1.com");
	 * \endcode
	 *
	 * NOTE: this call is entirely optional.
	 */
	VORTEX_EXTERNAL_CONF_HOST = 2,
	/** 
	 * @brief Allows to configure the port that should be
	 * reported by this connection. Functions like \ref
	 * vortex_connection_get_port will report this.
	 *
	 * \code
	 * // make receive and send operation to be mutually exclusive
	 * vortex_external_setup_conf (setup, VORTEX_EXTERNAL_CONF_PORT, (axlPointer) "23455");
	 * \endcode
	 *
	 * NOTE: this call is entirely optional.
	 */
	VORTEX_EXTERNAL_CONF_PORT = 3,
	/** 
	 * @brief Allows to configure a handler that is called just
	 * after creating the connection but before starting doing
	 * anything with it, so it is possible to implement further
	 * setup from with int.
	 *
	 * Handler configured is \ref VortexExternalOnPrepare
	 */
	VORTEX_EXTERNAL_ON_PREPARE_HANDLER = 4,
	/** 
	 * @brief Allows to configure user data pointer to be passed
	 * in into \ref VortexExternalOnPrepare (\ref
	 * VORTEX_EXTERNAL_ON_PREPARE_HANDLER)
	 *
	 */
	VORTEX_EXTERNAL_ON_PREPARE_USER_DATA = 5,
	/** 
	 * @brief Allows to configure user data2 pointer to be passed
	 * in into \ref VortexExternalOnPrepare (\ref
	 * VORTEX_EXTERNAL_ON_PREPARE_HANDLER)
	 *
	 */
	VORTEX_EXTERNAL_ON_PREPARE_USER_DATA2 = 6,
} VortexExternalConfItem;

VortexExternalSetup  * vortex_external_setup_new      (VortexCtx * ctx);

axl_bool               vortex_external_setup_ref      (VortexExternalSetup * setup);

void                   vortex_external_setup_unref    (VortexExternalSetup * setup);

void                   vortex_external_setup_conf     (VortexExternalSetup      * setup,
						       VortexExternalConfItem     item,
						       axlPointer                 value);

VortexConnection * vortex_external_connection_new (VortexCtx                 * ctx,
						   VORTEX_SOCKET               _session,
						   VortexSendHandler           _send_handler,
						   VortexReceiveHandler        _received_handler,
						   VortexExternalSetup       * setup,
						   VortexConnectionNew         on_connected, 
						   axlPointer                  user_data);

VortexConnection * vortex_external_listener_new   (VortexCtx                 * ctx,
						   VORTEX_SOCKET               _session,
						   VortexSendHandler           _send_handler,
						   VortexReceiveHandler        _received_handler,
						   VortexExternalSetup       * setup,
						   VortexExternalOnAccept      on_accept_handler,
						   axlPointer                  on_accept_data);



END_C_DECLS

#endif

/* @} */

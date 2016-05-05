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

#ifndef __VORTEX_CONNECTION_H__
#define __VORTEX_CONNECTION_H__

#include <vortex.h>

/** 
 * \addtogroup vortex_connection
 * @{
 */

/** 
 * @brief Allows to get the associated serverName value under which
 * the connection is working. The serverName value is a centric
 * concept inside BEEP and defines under which name is acting the
 * connection. In general, this value is used to implement resource
 * separation based on the serverName configured.
 *
 * A key concept is that the serverName value is negociated, accepted
 * and fixed for the rest of the session on the first successful
 * channel created on the connection. Once done, no diferent
 * serverName can be negociated.
 *
 * @param channel The channel that is requested to return the
 * serverName associated to the connection holding the channel.
 * 
 * @return The serverName associated or NULL if none is defined.
 */
#define SERVER_NAME_FROM_CHANNEL(channel) (vortex_connection_get_server_name (vortex_channel_get_connection (channel)))

/** 
 * @brief Allows to get the context associated to the provided
 * connection, logging a more verbose message if the context is null
 * or the connection provided is null.
 *
 * For vortex, the context object is central for its
 * function. Providing a function that warns that a null context is
 * returned is a key to find bugs, since no proper result could be
 * expected if no context is provided (\ref VortexCtx). 
 *
 * @param c The connection that is required to return the context
 * associated.
 */
#define CONN_CTX(c) vortex_connection_get_ctx(c)

VortexConnection  * vortex_connection_new                    (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data);

VortexConnection  * vortex_connection_new_full               (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionOpts * options,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data);

VortexConnection  * vortex_connection_new6                   (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data);

VortexConnection  * vortex_connection_new_full6              (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionOpts * options,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data);

axl_bool            vortex_connection_reconnect              (VortexConnection * connection,
							      VortexConnectionNew on_connected,
							      axlPointer user_data);

axl_bool            vortex_connection_close                  (VortexConnection  * connection);

VORTEX_SOCKET       vortex_connection_sock_connect           (VortexCtx   * ctx,
							      const char  * host,
							      const char  * port,
							      int         * timeout,
							      axlError   ** error);

VORTEX_SOCKET       vortex_connection_sock_connect_common    (VortexCtx            * ctx,
							      const char           * host,
							      const char           * port,
							      int                  * timeout,
							      VortexNetTransport     transport,
							      axlError            ** error);

axl_bool            vortex_connection_do_greetings_exchange  (VortexCtx            * ctx, 
							      VortexConnection     * connection, 
							      VortexConnectionOpts * options,
							      int                    timeout);

axl_bool            vortex_connection_close_all_channels     (VortexConnection * connection, 
							      axl_bool           also_channel_0);

axl_bool            vortex_connection_ref                    (VortexConnection * connection,
							      const char       * who);

axl_bool            vortex_connection_uncheck_ref            (VortexConnection * connection);

void                vortex_connection_unref                  (VortexConnection * connection,
							      const char       * who);

int                 vortex_connection_ref_count              (VortexConnection * connection);

VortexConnection  * vortex_connection_new_empty              (VortexCtx        * ctx,
							      VORTEX_SOCKET      socket,
							      VortexPeerRole     role);

VortexConnection  * vortex_connection_new_empty_from_connection (VortexCtx        * ctx,
								 VORTEX_SOCKET      socket, 
								 VortexConnection * __connection,
								 VortexPeerRole     role);

VortexConnection  * vortex_connection_new_empty_from_connection2         (VortexCtx        * ctx,
									  VORTEX_SOCKET      socket, 
									  VortexConnection * __connection,
									  VortexPeerRole     role,
									  axl_bool           skip_naming);

axl_bool            vortex_connection_set_socket                (VortexConnection * conn,
								 VORTEX_SOCKET      socket,
								 const char       * real_host,
								 const char       * real_port);

void                vortex_connection_timeout                (VortexCtx        * ctx,
							      long               microseconds_to_wait);
void                vortex_connection_connect_timeout        (VortexCtx        * ctx,
							      long               microseconds_to_wait);

long                vortex_connection_get_timeout            (VortexCtx        * ctx);
long                vortex_connection_get_connect_timeout    (VortexCtx        * ctx);

axl_bool            vortex_connection_is_ok                  (VortexConnection * connection, 
							      axl_bool           free_on_fail);

const char        * vortex_connection_get_message            (VortexConnection * connection);

VortexStatus        vortex_connection_get_status             (VortexConnection * connection);

axl_bool            vortex_connection_pop_channel_error      (VortexConnection  * connection, 
							      int               * code,
							      char             ** msg);

void                __vortex_connection_shutdown_and_record_error (VortexConnection * conn,
								   VortexStatus       status,
								   const char       * message,
								   ...);

void                vortex_connection_push_channel_error     (VortexConnection  * connection, 
							      int                 code,
							      const char        * msg);

void                vortex_connection_free                   (VortexConnection * connection);

axlList           * vortex_connection_get_remote_profiles    (VortexConnection * connection);

int                 vortex_connection_set_profile_mask       (VortexConnection      * connection,
							      VortexProfileMaskFunc   mask,
							      axlPointer              user_data);

axl_bool            vortex_connection_is_profile_filtered    (VortexConnection      * connection,
							      int                     channel_num,
							      const char            * uri,
							      const char            * profile_content,
							      VortexEncoding          encoding,
							      const char            * serverName,
							      VortexFrame           * frame,
							      char                 ** error_msg);

axl_bool            vortex_connection_is_profile_supported   (VortexConnection * connection, 
							      const char       * uri);

axl_bool            vortex_connection_channel_exists         (VortexConnection * connection, 
							      int  channel_num);

int                 vortex_connection_channels_count         (VortexConnection * connection);

int                 vortex_connection_foreach_channel        (VortexConnection   * connection,
							      axlHashForeachFunc   func,
							      axlPointer           user_data);

int                 vortex_connection_get_next_channel       (VortexConnection * connection);

VortexChannel     * vortex_connection_get_channel            (VortexConnection * connection, 
							      int  channel_num);

VortexChannel     * vortex_connection_get_channel_by_uri     (VortexConnection * connection,
							      const char       * profile);

VortexChannel     * vortex_connection_get_channel_by_func    (VortexConnection     * connection,
							      VortexChannelSelector  selector,
							      axlPointer             user_data);

int                 vortex_connection_get_channel_count      (VortexConnection     * connection,
							      const char           * profile);

VORTEX_SOCKET       vortex_connection_get_socket             (VortexConnection * connection);

void                vortex_connection_set_close_socket       (VortexConnection * connection,
							      axl_bool           action);

axl_bool            vortex_connection_add_channel            (VortexConnection * connection, 
							      VortexChannel * channel);

axl_bool            vortex_connection_add_channel_common     (VortexConnection * connection,
							      VortexChannel    * channel,
							      axl_bool           do_notify);

void                vortex_connection_remove_channel         (VortexConnection * connection, 
							      VortexChannel * channel);

void                vortex_connection_remove_channel_common  (VortexConnection * connection, 
							      VortexChannel    * channel,
							      axl_bool           do_notify);

const char        * vortex_connection_get_host               (VortexConnection * connection);

const char        * vortex_connection_get_host_ip            (VortexConnection * connection);

const char        * vortex_connection_get_port               (VortexConnection * connection);

const char        * vortex_connection_get_local_addr         (VortexConnection * connection);

const char        * vortex_connection_get_local_port         (VortexConnection * connection);

void                vortex_connection_set_host_and_port      (VortexConnection * connection, 
							      const char       * host,
							      const char       * port,
							      const char       * host_ip);

int                 vortex_connection_get_id                 (VortexConnection * connection);

const char        * vortex_connection_get_server_name        (VortexConnection * connection);

void                vortex_connection_set_server_name        (VortexConnection * connection,
							      const       char * serverName);

axl_bool            vortex_connection_set_blocking_socket    (VortexConnection * connection);

axl_bool            vortex_connection_set_nonblocking_socket (VortexConnection * connection);

axl_bool            vortex_connection_set_sock_tcp_nodelay   (VORTEX_SOCKET socket,
							      axl_bool      enable);

axl_bool            vortex_connection_set_sock_block         (VORTEX_SOCKET socket,
							      axl_bool      enable);

void                vortex_connection_set_data               (VortexConnection * connection,
							      const char       * key,
							      axlPointer         value);

void                vortex_connection_set_data_full          (VortexConnection * connection,
							      char             * key,
							      axlPointer         value,
							      axlDestroyFunc     key_destroy,
							      axlDestroyFunc     value_destroy);

void                vortex_connection_set_hook               (VortexConnection * connection,
							      axlPointer         ptr);

axlPointer          vortex_connection_get_hook               (VortexConnection * connection);

void                vortex_connection_delete_key_data        (VortexConnection * connection,
							      const char       * key);

void                vortex_connection_set_connection_actions (VortexCtx              * ctx,
							      VortexConnectionStage    stage,
							      VortexConnectionAction   action_handler,
							      axlPointer               handler_data);


int                 vortex_connection_actions_notify         (VortexCtx                * ctx,
							      VortexConnection        ** caller_conn,
							      VortexConnectionStage      stage);

axlPointer          vortex_connection_get_data               (VortexConnection * connection,
							      const char       * key);

VortexHash        * vortex_connection_get_data_hash          (VortexConnection * connection);

VortexHash        * vortex_connection_get_channels_hash      (VortexConnection * connection);

VortexChannelPool * vortex_connection_get_channel_pool       (VortexConnection * connection,
							      int                pool_id);

int                 vortex_connection_get_pending_msgs       (VortexConnection * connection);

VortexPeerRole      vortex_connection_get_role               (VortexConnection * connection);

const char        * vortex_connection_get_features           (VortexConnection * connection);

const char        * vortex_connection_get_localize           (VortexConnection * connection);

int                 vortex_connection_get_opened_channels    (VortexConnection * connection);

VortexConnection  * vortex_connection_get_listener           (VortexConnection * connection);

VortexCtx         * vortex_connection_get_ctx                (VortexConnection * connection);

VortexSendHandler      vortex_connection_set_send_handler    (VortexConnection * connection,
							      VortexSendHandler  send_handler);

VortexReceiveHandler   vortex_connection_set_receive_handler (VortexConnection * connection,
							      VortexReceiveHandler receive_handler);

void                   vortex_connection_set_default_io_handler (VortexConnection * connection);
								 

void                   vortex_connection_set_on_close       (VortexConnection * connection,
							     VortexConnectionOnClose on_close_handler);

void                    vortex_connection_set_on_close_full  (VortexConnection * connection,
							      VortexConnectionOnCloseFull on_close_handler,
							      axlPointer data);

void                    vortex_connection_set_on_close_full2  (VortexConnection * connection,
							       VortexConnectionOnCloseFull on_close_handler,
							       axl_bool                    insert_last,
							       axlPointer data);

axl_bool            vortex_connection_remove_on_close_full    (VortexConnection              * connection, 
							       VortexConnectionOnCloseFull     on_close_handler,
							       axlPointer                      data);

int                 vortex_connection_invoke_receive         (VortexConnection * connection,
							      char             * buffer,
							      int                buffer_len);

int                 vortex_connection_invoke_send            (VortexConnection * connection,
							      const char       * buffer,
							      int                buffer_len);

void                vortex_connection_sanity_socket_check        (VortexCtx * ctx, axl_bool      enable);

axl_bool            vortex_connection_parse_greetings_and_enable (VortexConnection * connection, 
								  VortexFrame      * frame);

void                vortex_connection_set_preread_handler        (VortexConnection * connection, 
								  VortexConnectionOnPreRead pre_accept_handler);

axl_bool            vortex_connection_is_defined_preread_handler (VortexConnection * connection);

void                vortex_connection_invoke_preread_handler     (VortexConnection * connection);

void                vortex_connection_set_tlsfication_status     (VortexConnection * connection,
								  axl_bool           status);

axl_bool            vortex_connection_is_tlsficated              (VortexConnection * connection);

void                vortex_connection_shutdown                   (VortexConnection * connection);

void                vortex_connection_shutdown_socket            (VortexConnection * connection);

axlPointer          vortex_connection_set_channel_added_handler   (VortexConnection                * connection,
								   VortexConnectionOnChannelUpdate   added_handler,
								   axlPointer                        user_data);

axlPointer          vortex_connection_set_channel_removed_handler  (VortexConnection                * connection,
								    VortexConnectionOnChannelUpdate   removed_handler,
								    axlPointer                        user_data);

void                vortex_connection_remove_handler               (VortexConnection                * connection,
								    VortexConnectionHandler           handler_type,
								    axlPointer                        handler_id);

void                vortex_connection_set_receive_stamp            (VortexConnection * conn, long bytes_received, long bytes_sent);

void                vortex_connection_get_receive_stamp            (VortexConnection * conn, long * bytes_received, long * bytes_sent, long * last_idle_stamp);

void                vortex_connection_check_idle_status            (VortexConnection * conn, VortexCtx * ctx, long time_stamp);

void                vortex_connection_block                        (VortexConnection * conn,
								    axl_bool           enable);

axl_bool            vortex_connection_is_blocked                   (VortexConnection  * conn);

axl_bool            vortex_connection_half_opened                  (VortexConnection  * conn);

int                 vortex_connection_get_next_frame_size          (VortexConnection * connection,
								    VortexChannel    * channel,
								    int                next_seq_no,
								    int                message_size,
								    int                max_seq_no);

void                vortex_connection_seq_frame_updates            (VortexConnection * connection,
								    axl_bool           is_disabled);

axl_bool            vortex_connection_seq_frame_updates_status     (VortexConnection * connection);

VortexChannelFrameSize  vortex_connection_set_next_frame_size_handler (VortexConnection        * connection,
								       VortexChannelFrameSize    next_frame_size,
								       axlPointer                user_data);

VortexChannelFrameSize  vortex_connection_set_default_next_frame_size_handler (VortexCtx               * ctx,
									       VortexChannelFrameSize    next_frame_size,
									       axlPointer                user_data);

/** 
 * @internal
 * Do not use the following functions, internal Vortex Library purposes.
 */

void                vortex_connection_init                   (VortexCtx        * ctx);

void                vortex_connection_cleanup                (VortexCtx        * ctx);

void                __vortex_connection_set_not_connected    (VortexConnection * connection, 
							      const char       * message,
							      VortexStatus       status);

int                 vortex_connection_do_a_sending_round     (VortexConnection * connection);

void                vortex_connection_lock_channel_pool      (VortexConnection * connection);

void                vortex_connection_unlock_channel_pool    (VortexConnection * connection);

int                 vortex_connection_next_channel_pool_id   (VortexConnection * connection);

void                vortex_connection_add_channel_pool       (VortexConnection  * connection,
							      VortexChannelPool * pool);

void                vortex_connection_remove_channel_pool    (VortexConnection  * connection,
							      VortexChannelPool * pool);

axl_bool            __vortex_connection_parse_greetings      (VortexConnection * connection, 
							      VortexFrame * frame);

void                __vortex_connection_check_and_notify     (VortexConnection * connection, 
							      VortexChannel    * channel, 
							      axl_bool           is_added);

int                 vortex_connection_get_mss                (VortexConnection * connection);

axl_bool            vortex_connection_check_socket_limit     (VortexCtx        * ctx, 
							      VORTEX_SOCKET      socket);

/** private API **/
axl_bool               vortex_connection_ref_internal                    (VortexConnection * connection, 
									  const char       * who,
									  axl_bool           check_ref);

void                vortex_connection_set_initial_accept     (VortexConnection * conn,
							      axl_bool           status);

/** 
 * @internal Flag used to flag a connection to skip futher handling.
 */
#define VORTEX_CONNECTION_SKIP_HANDLING "vo:co:sk:ha"

#endif

/* @} */


/** 
 * \addtogroup vortex_connection_opts
 * @{
 */

/** 
 * @brief Allows to easily signal connection features with a simple
 * macro that already signals to finish connection option once
 * connection is created.
 *
 * The macro receives a list of connection options, separated by (,)
 * and returns a newly created \ref VortexConnectionOpts.
 *
 * <b>IMPORTANT NOTE: you must end that list with VORTEX_OPTS_END.</b>
 *
 * See next examples:
 * \code
 * // create default options 
 * opts = CONN_OPTS (VORTEX_SERVERNAME_FEATURE, "some-server-name.local", VORTEX_OPTS_END);
 * \endcode
 *
 */
#define CONN_OPTS vortex_connection_opts_default

VortexConnectionOpts * vortex_connection_opts_new (VortexConnectionOptItem opt_item, ...);

VortexConnectionOpts * vortex_connection_opts_default (VortexConnectionOptItem opt_item, ...);

const char * vortex_connection_opts_get_serverName (VortexConnection     * conn);

void vortex_connection_opts_free (VortexConnectionOpts * conn_opts);

void vortex_connection_opts_check_and_release (VortexConnectionOpts * conn_opts);

/* @} */

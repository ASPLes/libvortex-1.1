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
#ifndef __VORTEX_HANDLERS_H__
#define __VORTEX_HANDLERS_H__

#include <vortex.h>

/** 
 * \defgroup vortex_handlers Vortex Handlers: Handlers used across Vortex Library for async notifications.
 */

/** 
 * \addtogroup vortex_handlers
 * @{
 */


/** 
 * @brief Async notification for listener creation.
 *
 * Functions using this handler:
 * - \ref vortex_listener_new
 *
 * Optional handler defined to report which host and port have
 * actually allocated a listener peer. If host and port is null means
 * listener have failed to run.
 *
 * You should not free any parameter received, vortex system will do
 * this for you.  If you want to actually keep a copy you should use
 * axl_strdup.
 * 
 * @param host the final host binded
 * @param port the final port binded
 * @param status the listener creation status.
 * @param message the message reporting the listener status creation.
 * @param user_data user data passed in to this async notifier.
 */
typedef void (*VortexListenerReady)           (char  * host, int  port, VortexStatus status, 
					       char  * message, axlPointer user_data);

/** 
 * @brief Async notification for listener creation, similar to \ref
 * VortexListenerReady but providing the reference for the \ref
 * VortexConnection created (representing the listener created).
 *
 * Functions using this handler:
 * - \ref vortex_listener_new_full
 *
 * Optional handler defined to report which host and port have
 * actually allocated a listener peer. If host and port is null means
 * listener have failed to run.
 *
 * You should not free any parameter received, vortex system will do
 * this for you.  If you want to actually keep a copy you should use
 * axl_strdup (deallocating your copy with axl_free).
 *
 * This function is similar to \ref VortexListenerReady but it also
 * notifies the connection created.
 * 
 * @param host The final host binded.
 *
 * @param port The final port binded.
 *
 * @param status The listener creation status.
 *
 * @param message The message reporting the listener status creation.
 *
 * @param connection The connection representing the listener created
 * (or a NULL reference if status is not \ref VortexOk).
 * 
 * @param user_data user data passed in to this async notifier.
 */
typedef void (*VortexListenerReadyFull)           (char  * host, int  port, VortexStatus status, 
						   char  * message, VortexConnection * connection, 
						   axlPointer user_data);



/** 
 * @brief Async notification for connection creation process.
 *
 * Functions using this handler:
 *  - \ref vortex_connection_new 
 * 
 * Optional handler defined to report the final status for a \ref
 * vortex_connection_new process. This handler allows to create a new
 * connection to a vortex server (BEEP enabled peer) in a non-blocking
 * way.
 * 
 * Once the connection creation process have finished, no matter which
 * is the final result, the handler is invoked.
 *
 * @param connection The connection created (or not, check \ref vortex_connection_is_ok).
 * @param user_data user defined data passed in to this async notifier.
 */
typedef void (*VortexConnectionNew) (VortexConnection * connection, axlPointer user_data);


/** 
 * @brief Async notification for start channel message received for a given profile.
 *
 * Functions using this handler:
 *  - \ref vortex_profiles_register
 *
 * Handler is used to notify peer role that a channel is being asked
 * to be created. The handler receives a reference to the connection
 * where the channel is being requested. 
 * 
 * If peer agree to create the channel axl_true must be
 * returned. Otherwise, axl_false must be used.
 * 
 * You can get the reference to the \ref VortexChannel object in
 * trasint to be created, by using \ref vortex_connection_get_channel,
 * but keep in mind the channel is it transit to be created. This is
 * useful if you want to configure something on the channel.
 *
 * @param channel_num the channel num for the new channel attempted to be created.
 * @param connection the connection where the channel is going to be created.
 * @param user_data user defined data passed in to this async notifier.
 * 
 * @return axl_true if the new channel can be created or axl_false if not.
 */
typedef  axl_bool      (*VortexOnStartChannel)      (int  channel_num,
						     VortexConnection * connection,
						     axlPointer user_data);


/** 
 * @brief Channel start message received handler with support for
 * extended attributes.
 * 
 * In most situations, while using Vortex Library, you can assume the
 * default implementation to handle the start channel request.
 *
 * However, a start message request could have several attributes and
 * additional parameters that are notified in the form of a "piggyback",
 * to set up new channels, performing especial operations, as happens
 * with TLS negotiation. 
 * 
 * This handler allows to get start message notification with all
 * possible data received.
 * 
 * Here is an channel start message example:
 * \code
 *       <start number='1' serverName='my.domain.com'>
 *            <profile uri='http://some-unique-profile-uri/mech' encoding='none'>
 *                <![CDATA[some necessary initial round trip data]]>
 *            </profile>
 *       </start>
 * \endcode
 * 
 * This handler will notify the channel number being requested, the
 * serverName value, the profile requested, the piggyback encoding and
 * the optional piggyback.
 * 
 * Keep in mind that the encoding is an implicit attribute. Having it
 * not defined yields to the default value 'none', which is
 * represented by \ref EncodingNone.
 *
 * Functions using this handler:
 *
 *   - \ref vortex_profiles_register_extended_start
 * 
 * @param profile The profile being request to create a new channel.
 * 
 * @param channel_num Channel being requested for creation.
 *
 * @param connection Connection where the channel creation request was
 * received.
 *
 * @param serverName Value for this optional start message
 * attribute. This is used
 *
 * @param profile_content Optional profile content received inside the
 * channel start message.
 *
 * @param profile_content_reply Optional profile content reply to be
 * used for channel start reply, it will be freed by the library after
 * the reply is sent.
 *
 * @param encoding Encoding used for the profile content.
 *
 * @param user_data User defined data to be passed in to the handler
 * when it is executed.
 * 
 * @return axl_true if the new channel can be created or axl_false if not. 
 */
typedef axl_bool      (*VortexOnStartChannelExtended) (const char        * profile,
						       int                 channel_num,
						       VortexConnection  * connection,
						       const char        * serverName,
						       const char        * profile_content,
						       char             ** profile_content_reply,
						       VortexEncoding      encoding,
						       axlPointer          user_data);
/** 
 * @brief Async notification for incoming close channel request.
 * 
 * Functions using this handler:
 *
 *   - \ref vortex_profiles_register
 *   - \ref vortex_channel_new
 * 
 * This handler allows to control what to do on close channel
 * notification. Close notification is received when remote peer wants
 * to close some channel.
 * 
 * The handler must return axl_true if the channel can be close, otherwise
 * axl_false must be returned.
 *
 * If you need to not perform an answer at this function, allowing to
 * deffer the decision, you can use \ref
 * VortexOnNotifyCloseChannel. That handler will allow to hold the
 * close channel notification, to later complete the request by
 * calling to a finish function.
 * 
 * @param channel_num The channel num requesting to be closed.
 *
 * @param connection The connection where the channel resides.
 *
 * @param user_data User defined data passed in to this Async notifier.
 * 
 * @return axl_true if channel can be closed. To reject the close request
 * return axl_false.
 */
typedef  axl_bool      (*VortexOnCloseChannel)      (int  channel_num,
						     VortexConnection * connection,
						     axlPointer user_data);

/** 
 * @brief Async notification for incoming close channel requests. This
 * handler allows to only receive the notification and defer the
 * decision to close or not the channel.
 *
 * This is an alternative API to \ref VortexOnCloseChannel, that
 * allows to configure a handler (this one) to be executed to get
 * notification about channel close request (\ref
 * vortex_channel_set_close_notify_handler), and later the request is
 * completed by calling to \ref vortex_channel_notify_close.
 *
 * @param channel The channel that was notified to be closed.
 *
 * @param user_data User defined data to be passed to the handler
 * execution.
 */
typedef void (*VortexOnNotifyCloseChannel) (VortexChannel * channel,
					    int             msg_no,
					    axlPointer      user_data);

/** 
 * @brief Async notifier for frame received event.
 *
 * Functions using this handler:
 *   - \ref vortex_profiles_register
 *   - \ref vortex_channel_new
 *   - \ref vortex_channel_new_full
 *   - \ref vortex_channel_new_fullv
 *   - \ref vortex_channel_set_received_handler
 *   - \ref vortex_channel_pool_new
 *   - \ref vortex_channel_queue_reply
 * 
 * This handler allows to control received frames from remote
 * peers. <b>channel</b> parameter is the channel where the
 * <b>frame</b> was received. <b>connection</b> parameter represent
 * the connection where the channel is running. Finally,
 * <b>user_data</b>, is a user defined pointer passed in to this
 * function and defined when the handler was registered.
 *
 * You must not free received frame. Vortex library will do it for you.
 * Remember that, if you want to have permanent access to frame payload,
 * make a copy. After running this handler, Vortex library will free
 * all VortexFrame resources.
 * 
 * @param channel the channel where the frame have been received.
 * @param connection the connection where the channel is running
 * @param frame the frame received
 * @param user_data user defined data passed in to this async notifier.
 */
typedef  void     (*VortexOnFrameReceived)     (VortexChannel    * channel,
						VortexConnection * connection,
						VortexFrame      * frame,
						axlPointer user_data);


/** 
 * @brief Async notifier for channel creation process.
 *
 * Function using this handler:
 *  - \ref vortex_channel_new
 *
 * This handler allows to define a notify function that will be
 * called once the channel is created or something have failed with
 * errors such as timeout reached or channel creation process
 * failures.
 *
 * If something fails, a -1 will be received on channel_num and
 * channel will be NULL.  Otherwise, you will received the channel
 * number created and a reference to channel created.  It is not
 * needed to hold a reference to channel. You can get it by using
 * \ref vortex_connection_get_channel.
 * 
 * In the case an error is found (channel reference is NULL or
 * channel_num is -1) you can use \ref
 * vortex_connection_pop_channel_error to get more details.
 * 
 * @param channel_num the channel number for the new channel created
 * @param channel the channel created
 * @param user_data user defined data passed in to this async notifier.
 */
typedef void      (*VortexOnChannelCreated)  (int                channel_num,
					      VortexChannel    * channel,
					      VortexConnection * conn,
					      axlPointer         user_data);

/** 
 * @brief Async notifier for Vortex Channel Pool creation.
 *
 * Function using this handler:
 *  - \ref vortex_channel_pool_new
 *
 * This handler allows to make an async notification once the channel
 * pool process have finished. Because the channel pool creation
 * process can take a long time this handler is really useful to keep
 * on doing other things such as interface updating.
 * 
 * @param pool the channel pool created
 * @param user_data user defined data passed in to this async notifier.
 */
typedef void      (*VortexOnChannelPoolCreated) (VortexChannelPool * pool,
						 axlPointer user_data);


/** 
 * @brief Synchronous handler definition used by the channel pool
 * module to create a new channel.
 *
 * This function allows to notify the user code that a new channel
 * must be created for the channel pool. This allows to place the
 * channel creation code outside the channel pool module while the
 * channel pool features remain the same. 
 *
 * The function has pretty much the same parameters as \ref
 * vortex_channel_new, which is quite straightforward, but two
 * additional parametes are provided: <b>create_channel_user_data</b>
 * and <b>get_next_data</b>. Both a pointers that are defined at the
 * vortex_channel_pool_new_full in the first case, and at the \ref vortex_channel_pool_get_next_ready_full.
 *
 * See \ref vortex_channel_pool_new_full for more information.
 * 
 * @param connection The connection where the channel pool is asking
 * to createa a new channel.
 *
 * @param channel_num The channel number to create.
 *
 * @param profile The profile for the channel.
 *
 * @param on_close The on close handler to be executed once received a
 * close notification.
 *
 * @param on_close_user_data The on close handler user data (data
 * provided to the on close handler).
 *
 * @param on_received The frame received handler to be executed one
 * data is received. Once the channel is created and returned by \ref
 * vortex_channel_pool_get_next_ready or \ref
 * vortex_channel_pool_get_next_ready_full this value can be
 * reconfigured using \ref vortex_channel_set_received_handler.
 *
 * @param on_received_user_data User defined data to be passed to the handler.
 *
 * @param create_channel_user_data User defined data provided at the
 * channel pool creation function (\ref vortex_channel_pool_new_full).
 *
 * @param get_next_data Optional user reference defined at \ref vortex_channel_pool_get_next_ready_full.
 *
 * @return A newly created \ref VortexChannel reference or NULL if it
 * fails.
 */
typedef VortexChannel * (* VortexChannelPoolCreate) (VortexConnection     * connection,
						     int                    channel_num,
						     const char           * profile,
						     VortexOnCloseChannel   on_close, 
						     axlPointer             on_close_user_data,
						     VortexOnFrameReceived  on_received, 
						     axlPointer             on_received_user_data,
						     /* additional pointers */
						     axlPointer             create_channel_user_data,
						     axlPointer             get_next_data);

/** 
 * @brief Async notifier for the channel close process, with support
 * for a user defined data.
 *
 * Function using this handler:
 *  - \ref vortex_channel_close_full
 *
 * This function handler works as defined by \ref
 * VortexOnClosedNotification but without requiring an optional user
 * data pointer. See also \ref vortex_channel_close.
 *
 * This handler allows to defined a notify function that will be
 * called once the channel close indication is received. This is
 * mainly used by \ref vortex_channel_close to avoid caller get
 * blocked by calling that function.
 *
 * Data received is the channel num this notification applies to, the
 * channel closing status and if was_closed is axl_false (the channel was
 * not closed) code hold error code returned and msg the message
 * returned by remote peer. 
 *
 * @param connection the connection on which the channel was closed
 * @param channel_num the channel num identifying the channel closed
 * @param was_closed status for the channel close process.
 * @param code the code representing the channel close process.
 * @param msg the message representing the channel close process.
 * @param user_data A user defined pointer established at function
 * which received this handler.
 */
typedef void     (*VortexOnClosedNotificationFull) (VortexConnection * connection,
						    int                channel_num,
						    axl_bool           was_closed,
						    const char       * code,
						    const char       * msg,
						    axlPointer         user_data);

/** 
 * @brief Async notifier for the channel close process
 *
 * Function using this handler:
 *  - \ref vortex_channel_close
 *
 * In the case you require to provide a user defined pointer to the
 * close function you can try \ref vortex_channel_close_full (and its
 * handler \ref VortexOnClosedNotificationFull).
 *
 * This handler allows to defined a notify function that will be
 * called once the channel close indication is received. This is
 * mainly used by \ref vortex_channel_close to avoid caller get
 * blocked by calling that function.
 *
 * Data received is the channel num this notification applies to, the
 * channel closing status and if was_closed is axl_false (the channel was
 * not closed) code hold error code returned and msg the message
 * returned by remote peer. 
 *
 * @param connection the connection on which the channel was closed
 * @param channel_num the channel num identifying the channel closed
 * @param was_closed status for the channel close process.
 * @param code the code representing the channel close process.
 * @param msg the message representing the channel close process.
 * @param user_data A user defined pointer established at function
 * which received this handler.
 */
typedef void     (*VortexOnClosedNotification) (int             channel_num,
						axl_bool        was_closed,
						const char     * code,
						const char     * msg);

/** 
 * @brief Defines the writers handlers used to actually send data
 * through the underlaying socket descriptor.
 * 
 * This handler is used by: 
 *  - \ref vortex_connection_set_send_handler
 * 
 * @param connection Vortex Connection where the data will be sent.
 * @param buffer     The buffer holding data to be sent
 * @param buffer_len The buffer len.
 * 
 * @return     How many data was actually sent.
 */
typedef int      (*VortexSendHandler)         (VortexConnection * connection,
					       const char       * buffer,
					       int                buffer_len);

/** 
 * @brief Defines the readers handlers used to actually received data
 * from the underlying socket descriptor.
 *  
 * This handler is used by: 
 *  - \ref vortex_connection_set_receive_handler
 * 
 * @param connection The Vortex connection where the data will be received.
 * @param buffer     The buffer reference used as container for data received.
 * @param buffer_len The buffer len use to know the limits of the buffer.
 * 
 * @return How many data was actually received.
 */
typedef int      (*VortexReceiveHandler)         (VortexConnection * connection,
						  char             * buffer,
						  int                buffer_len);

/** 
 * @brief Allows to set a handler that will be called when a
 * connection is about being closed.
 * 
 * This method could be used to establish some especial actions to be
 * taken before the connection is completely unrefered. This function
 * could be used to detect connection broken.
 * 
 * This handler is used by:
 *  - \ref vortex_connection_set_on_close
 * 
 * <i><b>NOTE:</b> You must not free the connection received at the
 * handler. This is actually done by the library. </i>
 * 
 * @param connection The connection that is about to be unrefered.
 * 
 */
typedef void     (*VortexConnectionOnClose)      (VortexConnection * connection);

/** 
 * @brief Extended version for \ref VortexConnectionOnClose, which
 * also supports passing an user defined data.
 *
 * This handler definition is the same as \ref
 * VortexConnectionOnClose, being notified at the same time, but
 * providing an user defined pointer.
 *
 * See also:
 * 
 *  - \ref vortex_connection_set_on_close_full
 *  - \ref vortex_connection_set_on_close
 * 
 * @param The connection that is about to be closed.
 * @param The user defined pointer configured at \ref vortex_connection_set_on_close_full.
 * 
 * @return 
 */
typedef void     (*VortexConnectionOnCloseFull)  (VortexConnection * connection, axlPointer data);


/** 
 * @brief Async handler definition to get a notification for
 * connections created at the server side (peer that is able to accept
 * incoming connections).
 *
 * This handler is executed once the connection is accepted and
 * registered in the vortex engine. If the function return axl_false, the
 * connection will be dropped, without reporting any error to the
 * remote peer.
 *
 * This function could be used as a initial configuration for every
 * connection. So, if it is only required to make a connection
 * initialization, the handler provided must always return axl_true to
 * avoid dropping the connection.
 *
 * This handler is also used by the TUNNEL profile implementation to
 * notify the application layer if the incoming TUNNEL profile should
 * be accepted. 
 *
 * Note this handler is called twice on TLS activation: one for the
 * first connection and one for the connection creation after TLS
 * activation. This is because both connections are diferent objects
 * with different states. This also allows to setup or run different
 * configurations for non TLS or and for TLS enabled clients.
 *
 * This handler is used by:
 * 
 *  - \ref vortex_listener_set_on_connection_accepted 
 *
 * @param connection The connection that has been accepted to be
 * managed by the listener.
 *
 * @param data The connection data to be passed in to the handler. 
 * 
 * @return axl_true if the connection is finally accepted. axl_false to drop
 * the connection, rejecting the incoming connection.
 */
typedef axl_bool      (*VortexOnAcceptedConnection)   (VortexConnection * connection, axlPointer data);

/** 
 * @brief Pre read handler definition.
 * 
 * This handler definition is mainly used to perform especial
 * operations prior to fully accept a connection into the vortex
 * library. This handler is detected and invoked in an abstract manner
 * from inside the Vortex Reader.
 *
 * Functions using this handler are: 
 *  - \ref vortex_connection_set_preread_handler
 * 
 * @param connection The connection which is receiving the on pre read
 * event.X
 */
typedef void   (* VortexConnectionOnPreRead)       (VortexConnection * connection);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while createing a new fd set.
 *
 * @param ctx The context where the IO set will be created.
 *
 * @param wait_to Allows to configure the file set to be prepared to
 * be used for the set of operations provided. 
 * 
 * @return A newly created fd set pointer, opaque to Vortex, to a
 * structure representing the fd set, that will be used to perform IO
 * waiting operation at the \ref vortex_io "Vortex IO module".
 * 
 */
typedef axlPointer   (* VortexIoCreateFdGroup)        (VortexCtx * ctx, VortexIoWaitingFor wait_to);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while destroying a fd set. 
 *
 * The reference that the handler will receive is the one created by
 * the \ref VortexIoCreateFdGroup handler.
 * 
 * @param VortexIoDestroyFdGroup The fd_set, opaque to vortex, pointer
 * to a structure representing the fd set to be destroy.
 * 
 */
typedef void     (* VortexIoDestroyFdGroup)        (axlPointer             fd_set);

/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while clearing a fd set.
 * 
 * @param VortexIoClearFdGroup The fd_set, opaque to vortex, pointer
 * to a structure representing the fd set to be clear.
 * 
 */
typedef void     (* VortexIoClearFdGroup)        (axlPointer             fd_set);



/** 
 * @brief IO handler definition to allow defining the method to be
 * used while performing a IO blocking wait, by default implemented by
 * the IO "select" call.
 *
 * @param VortexIoWaitOnFdGroup The handler to set.
 *
 * @param The maximum value for the socket descriptor being watched.
 *
 * @param The requested operation to perform.
 * 
 * @return An error code according to the description found on this
 * function: \ref vortex_io_waiting_set_wait_on_fd_group.
 */
typedef int      (* VortexIoWaitOnFdGroup)       (axlPointer             fd_group,
						  int                    max_fds,
						  VortexIoWaitingFor     wait_to);

/** 
 * @brief IO handler definition to perform the "add to" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 * 
 * @return returns axl_true if the socket descriptor was added, otherwise,
 * axl_false is returned.
 */
typedef axl_bool      (* VortexIoAddToFdGroup)        (int                    fds,
						       VortexConnection     * connection,
						       axlPointer             fd_group);

/** 
 * @brief IO handler definition to perform the "is set" the fd set
 * operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_group The socket descriptor group to be used as
 * destination for the socket.
 *
 * @param user_data User defined pointer provided to the function.
 *
 * @return axl_true if the socket descriptor is active in the given fd
 * group.
 *
 */
typedef axl_bool      (* VortexIoIsSetFdGroup)        (int                    fds,
						       axlPointer             fd_group,
						       axlPointer             user_data);

/** 
 * @brief Handler definition to allow implementing the have dispatch
 * function at the vortex io module.
 *
 * An I/O wait implementation must return axl_true to notify vortex engine
 * it support automatic dispatch (which is a far better mechanism,
 * supporting better large set of descriptors), or axl_false, to notify
 * that the \ref vortex_io_waiting_set_is_set_fd_group mechanism must
 * be used.
 *
 * In the case the automatic dispatch is implemented, it is also
 * required to implement the \ref VortexIoDispatch handler.
 * 
 * @param fd_group A reference to the object created by the I/O waiting mechanism.
 * p
 * @return Returns axl_true if the I/O waiting mechanism support automatic
 * dispatch, otherwise axl_false is returned.
 */
typedef axl_bool      (* VortexIoHaveDispatch)         (axlPointer             fd_group);

/** 
 * @brief User space handler to implement automatic dispatch for I/O
 * waiting mechanism implemented at vortex io module.
 *
 * This handler definition is used by:
 * - \ref vortex_io_waiting_invoke_dispatch
 *
 * Do not confuse this handler definition with \ref VortexIoDispatch,
 * which is the handler definition for the actual implemenation for
 * the I/O mechanism to implement automatic dispatch.
 * 
 * @param fds The socket that is being notified and identified to be dispatched.
 * 
 * @param wait_to The purpose of the created I/O waiting mechanism.
 *
 * @param connection Connection where the dispatch operation takes
 * place.
 * 
 * @param user_data Reference to the user data provided to the dispatch function.
 */
typedef void     (* VortexIoDispatchFunc)         (int                    fds,
						   VortexIoWaitingFor     wait_to,
						   VortexConnection     * connection,
						   axlPointer             user_data);

/** 
 * @brief Handler definition for the automatic dispatch implementation
 * for the particular I/O mechanism selected.
 *
 * This handler is used by:
 *  - \ref vortex_io_waiting_set_dispatch
 *  - \ref vortex_io_waiting_invoke_dispatch (internally)
 *
 * If this handler is implemented, the \ref VortexIoHaveDispatch must
 * also be implemented, making it to always return axl_true. If this two
 * handler are implemented, its is not required to implement the "is
 * set?" functionality provided by \ref VortexIoIsSetFdGroup (\ref
 * vortex_io_waiting_set_is_set_fd_group).
 * 
 * @param fd_group A reference to the object created by the I/O
 * waiting mechanism.
 * 
 * @param dispatch_func The dispatch user space function to be called.
 *
 * @param changed The number of descriptors that changed, so, once
 * inspected that number, it is not required to continue.
 *
 * @param user_data User defined data provided to the dispatch
 * function once called.
 */
typedef void     (* VortexIoDispatch)             (axlPointer             fd_group,
						   VortexIoDispatchFunc   dispatch_func,
						   int                    changed,
						   axlPointer             user_data);


/** 
 * @brief Profile mask handler used to perform profile filtering
 * functions.
 *
 * This handler definition is used by:
 *  - \ref vortex_connection_set_profile_mask
 *
 * The handler (a mask) is executed for each profile to be
 * filtered. The function based on the data received must return axl_true
 * (to filter a profile) or axl_false (to not filter it).
 *
 * The function receives the connection where the filtering is taking
 * place, the profile uri and a user defined pointer configured at the
 * function used to install the mask, along with some data which is
 * optional according to the place that the mask is executed.
 *
 * Because the profile mask is used by the vortex engine to filter
 * profiles at the greeting process and filter profiles at the channel
 * creation process, data available on such stages differs. 
 * 
 * At the greetings process, only <b>uri</b> is available. The value
 * provided for the <b>channel_num</b> is -1 and NULL for
 * <b>profile_content</b> and <b>serverName</b>.
 *
 * At the channel creation process all values are defined (with the
 * exception of <b>profile_content</b> and <b>serverName</b> which are
 * optional).
 *
 * @param connection The connection that where the filter process will
 * take place.
 *
 * @param channel_num The channel number that is requested in the
 * channel creation stage.
 *
 * @param uri A uri reference to the profile check if it should be
 * filtered.
 *
 * @param profile_content The piggyback provided at the start channel
 * request along with the profile.
 *
 * @param encoding Profile content encoding.
 *
 * @param serverName A request for the connection to act as
 * serverName.
 *
 * @param frame The frame that contains the channel start request
 * (when defined: channel_num > 0).
 *
 * @param error_msg Optional variable to configure an error message to
 * be returned to the remote peer extending the 554 error used by
 * default. The handler can define a textual message on this variable
 * (dinamically allocated) and then used by vortex to build the error
 * reply. The message configured will be deallocated using
 * axl_free. For example, you can use the following:
 * \code
 * // check we are not at the greetings process and also check error_msg
 * // which may not be defined
 * if (channel_num > 0 && error_msg) {
 *      (* error_msg) = axl_strdup ("Profile not accepted due to policy configuration");
 *      // return axl_true to filter the uri
 *      return axl_true;
 * }
 * \endcode
 * 
 * NOTE: You must not define error_msg and return axl_false. 
 *
 * @param user_data User defined pointer passed to the function.
 * 
 * @return axl_true to filter the uri, axl_false if not.
 */
typedef axl_bool      (* VortexProfileMaskFunc)       (VortexConnection      * connection,
						       int                     channel_num,
						       const char            * uri,
						       const char            * profile_content,
						       VortexEncoding          encoding,
						       const char            * serverName,
						       VortexFrame           * frame,
						       char                 ** error_msg,
						       axlPointer              user_data);

/** 
 * @brief Handler definition for the set of functions that allow the
 * users space to control how frames are splited by the vortex
 * sequencer process.
 *
 * This handler is used by the following functions:
 *
 * - \ref vortex_channel_get_next_frame_size
 * - \ref vortex_channel_set_next_frame_size_handler
 * - \ref vortex_connection_get_next_frame_size
 * - \ref vortex_connection_set_next_frame_size_handler
 * - \ref vortex_connection_set_default_next_frame_size_handler
 *
 * @param channel The channel that is required to return next frame size.
 *
 * @param next_seq_no This value represent the next sequence number
 * for the first octect to be sent on the frame.
 *
 * @param message_size This value represent the size of the payload to
 * be sent.
 *
 * @param max_seq_no Is the maximum allowed seqno accepted by the
 * remote peer. Beyond this value, the remote peer will close the
 * connection.
 *
 * @param user_data User defined pointer passed to the handler when it
 * is executed.
 * 
 * @return The amount of payload to use into the next frame to be
 * built. The function will return -1 if the channel reference
 * received is NULL.
 */
typedef int (*VortexChannelFrameSize) (VortexChannel * channel,
				       int             next_seq_no,
				       int             message_size,
				       int             max_seq_no,
				       axlPointer      user_data);
				      

/** 
 * @brief Handler used by \ref vortex_connection_get_channel_by_func
 * which is called to check if the channel provided must be returned
 * as selected.
 *
 * @param channel The function receives the channel that is asked to
 * be selected or not.
 *
 * @param user_data User defined data which is provided to the \ref
 * vortex_connection_get_channel_by_func.
 *
 * @return axl_true to select the channel, otherwise axl_false must be
 * returned.
 */
typedef axl_bool  (*VortexChannelSelector) (VortexChannel * channel, 
					    axlPointer      user_data);

/** 
 * @brief Handler definition used to notify that a channel was added
 * or removed from a particular connection.
 *
 * In the case the channel is added, the function is called just after
 * it was fully added to the connection. In the case the channel was
 * removed, the handler is called just before.
 *
 * The handler is used the following two functions:
 * 
 * - \ref vortex_connection_set_channel_added_handler
 * - \ref vortex_connection_set_channel_removed_handler
 *
 * This function is handler definition is useful to get notifications
 * at the server side for channels created or removed in the case a
 * particular operation must be triggered on that moment.
 *
 * @param channel The channel that is being removed or added to the
 * connection.
 *
 * @param user_data The user defined data that was configured with the
 * handle.r
 * 
 */
typedef void (*VortexConnectionOnChannelUpdate) (VortexChannel * channel, axlPointer user_data);

/** 
 * @brief Handler definition for the set of functions that are called
 * during the connection creation and configured by \ref
 * vortex_connection_set_connection_actions.
 *
 * See \ref VortexConnectionStage definition for available events.
 *
 * @param ctx The context where the operation is taking place.
 * 
 * @param conn The connection that is notified.
 *
 * @param new_conn In the case the action creates a new connection
 * that must replace the connection received, this variable is used to
 * notify the new reference.
 *
 * @param stage The stage during the notification is taking place.
 *
 * @param user_data User defined data associated to the action. This
 * was configured at \ref vortex_connection_set_connection_actions.
 * 
 * @return The function must return a set of codes that are used by
 * the library to handle errors. The action must return:
 *
 * - (-1) in the case an error is found during the action
 * processing. In this case, the connection is shutted down and a non
 * connected reference is returned to the caller.
 *
 * - (0) in the case no error was found and the function wants to stop
 * action processing. This is useful to block other actions.
 *
 * - (1) in the case no error was found and the function don't care
 *     about other actions being executed. the function executed the
 *     action without errors.
 *
 * - (2) in the case the function creates a new connection and updates
 *     references (using new_conn). In this case, the action handler
 *     is responsible of the connection received and its deallocation.
 */
typedef int (*VortexConnectionAction)    (VortexCtx               * ctx,
					  VortexConnection        * conn,
					  VortexConnection       ** new_conn,
					  VortexConnectionStage     stage,
					  axlPointer                user_data);

/** 
 * @brief Handler definition that allows to get a notification that
 * the channel is being disconnected from the connection (because the
 * connection is closed and in process of being deallocated).
 *
 * This function is used by: 
 * 
 * - \ref vortex_channel_set_closed_handler 
 *
 * NOTE: The connection reference holding the channel should not be
 * used from inside this handler.
 * 
 * @param channel The channel about to be unrefered (the reference owned by the connection).
 *
 * @param user_data User defined pointer configured at: \ref vortex_channel_set_closed_handler.
 *
 * @see vortex_channel_set_closed_handler
 */
typedef void (*VortexOnClosedChannel) (VortexChannel * channel, axlPointer user_data);

/** 
 * @brief Handler definition that allows a client to print log
 * messages itself.
 *
 * This function is used by: 
 * 
 * - \ref vortex_log_set_handler
 * - \ref vortex_log_get_handler
 *
 * @param file The file that produced the log.
 *
 * @param line The line where the log was produced.
 *
 * @param log_level The level of the log
 *
 * @param message The message being reported.
 *
 * @param args Arguments for the message.
 */
typedef void (*VortexLogHandler) (const char       * file,
				  int                line,
				  VortexDebugLevel   log_level,
				  const char       * message,
				  va_list            args);

/** 
 * @brief Handler definition that allows a client to print log
 * messages itself.
 *
 * This function is used by: 
 * 
 * - \ref vortex_log_set_handler_full
 *
 * @param ctx The context where the log is happening.
 *
 * @param file The file that produced the log.
 *
 * @param line The line where the log was produced.
 *
 * @param log_level The level of the log
 *
 * @param message The message being reported.
 *
 * @param user_data A reference to the user defined reference.
 *
 * @param args Arguments for the message.
 */
typedef void (*VortexLogHandlerFull) (VortexCtx        * ctx,
				      const char       * file,
				      int                line,
				      VortexDebugLevel   log_level,
				      const char       * message,
				      axlPointer         user_data,
				      va_list            args);

/** 
 * @brief Handler definition used by \ref vortex_async_queue_foreach
 * to implement a foreach operation over all items inside the provided
 * queue, blocking its access during its process.
 *
 * @param queue The queue that will receive the foreach operation.
 *
 * @param item_stored The item stored on the provided queue.
 *
 * @param position Item position inside the queue. 0 position is the
 * next item to pop.
 *
 * @param user_data User defined optional data provided to the foreach
 * function.
 */
typedef void (*VortexAsyncQueueForeach) (VortexAsyncQueue * queue,
					 axlPointer         item_stored,
					 int                position,
					 axlPointer         user_data);

/** 
 * @brief Handler used by Vortex library to create a new thread. A custom handler
 * can be specified using \ref vortex_thread_set_create
 *
 * @param thread_def A reference to the thread identifier created by
 * the function. This parameter is not optional.
 *
 * @param func The function to execute.
 *
 * @param user_data User defined data to be passed to the function to
 * be executed by the newly created thread.
 *
 * @return The function returns axl_true if the thread was created
 * properly and the variable thread_def is defined with the particular
 * thread reference created.
 *
 * @see vortex_thread_create
 */
typedef axl_bool (* VortexThreadCreateFunc) (VortexThread      * thread_def,
                                             VortexThreadFunc    func,
                                             axlPointer          user_data,
                                             va_list             args);

/** 
 * @brief Handler used by Vortex Library to release a thread's resources.
 * A custom handler can be specified using \ref vortex_thread_set_destroy
 *
 * @param thread_def A reference to the thread that must be destroyed.
 *
 * @param free_data Boolean that set whether the thread pointer should
 * be released or not.
 *
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 *
 * @see vortex_thread_destroy
 */
typedef axl_bool (* VortexThreadDestroyFunc) (VortexThread      * thread_def,
                                              axl_bool            free_data);

/** 
 * @brief Handler used by \ref vortex_ctx_set_on_finish which is
 * called when the vortex reader process detects no more pending
 * connections are available to be watched which is a signal that no
 * more pending work is available. This handler can be used to detect
 * and finish a process when no more work is available. For example,
 * this handler is used by turbulence to finish processes that where
 * created specifically to manage an incoming connection.
 *
 * @param ctx The context where the finish status as described is
 * signaled. The function executes in the context of the vortex
 * reader.
 *
 * @param user_data User defined pointer configured at \ref vortex_ctx_set_on_finish
 */
typedef void     (* VortexOnFinishHandler)   (VortexCtx * ctx, axlPointer user_data);

/** 
 * @brief Handler used by async event handlers activated via \ref
 * vortex_thread_pool_new_event, which causes the handler definition
 * to be called at the provided milliseconds period.
 *
 * @param ctx The vortex context where the async event will be fired.
 * @param user_data User defined pointer that was defined at \ref vortex_thread_pool_new_event function.
 * @param user_data2 Second User defined pointer that was defined at \ref vortex_thread_pool_new_event function.
 *
 * @return The function returns axl_true to signal the system to
 * remove the handler. Otherwise, axl_false must be returned to cause
 * the event to be fired again in the future at the provided period.
 */
typedef axl_bool (* VortexThreadAsyncEvent)        (VortexCtx  * ctx, 
						    axlPointer   user_data,
						    axlPointer   user_data2);

/** 
 * @brief Handler called to notify idle state reached for a particular
 * connection. The handler is configured a vortex context level,
 * applying to all connections created under this context. This
 * handler is configured by:
 *
 * - \ref vortex_ctx_set_idle_handler
 */
typedef void (* VortexIdleHandler) (VortexCtx        * ctx, 
				    VortexConnection * conn,
				    axlPointer         user_data,
				    axlPointer         user_data2);

/** 
 * @brief Function used to retrieve content to be send when required
 * by vortex sequencer. The function receives the context where the
 * operation happens and the vortex sequencer data that describes the
 * send operation.
 */
typedef axl_bool (* VortexPayloadFeederHandler) (VortexCtx              * ctx,
						 VortexPayloadFeederOp    op_type,
						 VortexPayloadFeeder    * feeder,
						 axlPointer               param1,
						 axlPointer               param2,
						 axlPointer               user_data);

/** 
 * @brief Optional handler definition that allows to get a
 * notification when a feeder has finished sending.
 *
 * This handler is used by:
 *
 * - \ref vortex_payload_feeder_set_on_finished
 *
 * @param ctx The context where the notification happens
 * @param channel The channel where the send operation happens
 * @param user_data User defined pointer.
 */
typedef void (* VortexPayloadFeederFinishedHandler) (VortexChannel        * channel,
						     VortexPayloadFeeder  * feeder,
						     axlPointer             user_data);

/** 
 * @brief Port sharing handler definition used by those functions that
 * tries to detect alternative transports that must be activated
 * before continue with normal BEEP course.
 *
 * The handler must return the following codes to report what they
 * found:
 *  - 1 : no error, nothing found for this handler (don't call me again).
 *  - 2 : found transport, stop calling the next.
 *  - (-1) : transport detected but failure found while activating it.
 *
 * @param ctx The vortex context where the operation takes place.
 *
 * @param listener The listener where this connection was received.
 *
 * @param conn The connection where port sharing is to be
 * detected. The function must use PEEK operation over the socket
 * (_session) to detect transports until the function is sure about
 * what is going to do. If the function do complete reads during the
 * testing part (removing data from the socket queue) the rest of the
 * BEEP session will fail.
 *
 * @param _session The socket associated to the provided \ref VortexConnection.
 *
 * @param bytes Though the function can do any PEEK read required, the
 * engine reads 4 bytes from the socket (or tries) and pass them into
 * this reference. It should be enough for most cases.
 *
 * @param user_data User defined pointer pass in to the handler (as defined by \ref vortex_listener_set_port_sharing_handling 's last parameter).
 */
typedef int (*VortexPortShareHandler) (VortexCtx * ctx, VortexConnection * listener, VortexConnection * conn, VORTEX_SOCKET _session, const char * bytes, axlPointer user_data);

				      
/** 
 * @internal Handler used for debugging. Not really useful for end user application.
 */
typedef void (* VortexClientConnCreated) (VortexCtx        * ctx,
					  VortexConnection * conn,
					  axlPointer         user_data);
#endif

/* @} */

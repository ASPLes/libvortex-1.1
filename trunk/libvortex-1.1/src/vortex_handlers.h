/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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
 * vortex_connection_new process. This handler allows you to create a
 * new connection to a vortex server (BEEP enabled peer) in a
 * non-blocking (or asynchronously way). 
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
 * If peer agree to create the channel true must be
 * returned. Otherwise, false must be used.
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
 * @return true if the new channel can be created or false if not.
 */
typedef  bool     (*VortexOnStartChannel)      (int  channel_num,
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
 * @return true if the new channel can be created or false if not. 
 */
typedef bool     (*VortexOnStartChannelExtended) (char              * profile,
						  int                 channel_num,
						  VortexConnection  * connection,
						  char              * serverName,
						  char              * profile_content,
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
 * The handler must return true if the channel can be close, otherwise
 * false must be returned.
 *
 * If you need to not perform an answer at this function, allowing to
 * deffer the decesion, you can use \ref
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
 * @return true if channel can be closed. To reject the close request
 * return false.
 */
typedef  bool     (*VortexOnCloseChannel)      (int  channel_num,
						VortexConnection * connection,
						axlPointer user_data);

/**
 * @brief Async notification for incoming close channel requests that
 * allows to only get notifications about that situation, deferring
 * the channel close decision.
 *
 * This is an alternative API to the \ref VortexOnCloseChannel, that
 * allows to configure a handler (this one) to be executed to get
 * notifications about the channel closed (\ref
 * vortex_channel_set_close_notify_handler), and later the request is
 * completed calling to \ref vortex_channel_notify_close.
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
 * This handler allows you to control received frames from remote
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
 * This handler allows you to define a notify function that will be
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
 * 
 * @param channel_num the channel number for the new channel created
 * @param channel the channel created
 * @param user_data user defined data passed in to this async notifier.
 */
typedef void      (*VortexOnChannelCreated)  (int             channel_num,
					      VortexChannel * channel,
					      axlPointer      user_data);

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
 * @param create_channel_user_data User defined data provided at the channel pool creation function.
 *
 * @return A newly created \ref VortexChannel reference or NULL if it
 * fails.
 */
typedef VortexChannel * (* VortexChannelPoolCreate) (VortexConnection     * connection,
						     int                    channel_num,
						     char                 * profile,
						     VortexOnCloseChannel   on_close, 
						     axlPointer             on_close_user_data,
						     VortexOnFrameReceived  on_received, 
						     axlPointer             on_received_user_data,
						     axlPointer             create_channel_user_data);

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
 * This handler allows you to defined a notify function that will be
 * called once the channel close indication is received. This is
 * mainly used by \ref vortex_channel_close to avoid caller get
 * blocked by calling that function.
 *
 * Data received is the channel num this notification applies to, the
 * channel closing status and if was_closed is false (the channel was
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
						    bool               was_closed,
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
 * This handler allows you to defined a notify function that will be
 * called once the channel close indication is received. This is
 * mainly used by \ref vortex_channel_close to avoid caller get
 * blocked by calling that function.
 *
 * Data received is the channel num this notification applies to, the
 * channel closing status and if was_closed is false (the channel was
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
						bool            was_closed,
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
 * registered in the vortex engine. If the function return false, the
 * connection will be dropped, without reporting any error to the
 * remote peer.
 *
 * This function could be used as a initial configuration for every
 * connection. So, if it is only required to make a connection
 * initialization, the handler provided must always return true to
 * avoid dropping the connection.
 *
 * This handler is also used by the TUNNEL profile implementation to
 * notify the application layer if the incoming TUNNEL profile should
 * be accepted. 
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
 * @return true if the connection is finally accepted. false to drop
 * the connection, rejecting the incoming connection.
 */
typedef bool     (*VortexOnAcceptedConnection)   (VortexConnection * connection, axlPointer data);

/** 
 * @brief Async notifications for TLS activation.
 *
 * Once the process for TLS negotiation have started, using \ref
 * vortex_tls_start_negotiation function, the status for such process is notified
 * using this handler type definition.
 *
 * The <i>status</i> value have to be checked in order to know if the
 * transport negotiation have finished successfully. Along the previous
 * variable, the <i>status_message</i> have a textual diagnostic about
 * the current status received.
 * 
 * While invoking \ref vortex_tls_start_negotiation you could provide an user
 * space pointer, using the <i>user_data</i> parameter. That user data
 * is received on this handler.
 *
 * Functions using this handler:
 *  \ref vortex_tls_start_negotiation
 * 
 * 
 * @param connection The connection where the TLS activation status is
 * being notified.
 *
 * @param status The process status.
 *
 * @param status_message A textual message representing the process
 * status.
 *
 * @param user_data A user defined pointer established at function
 * which received this handler.
 */
typedef void     (*VortexTlsActivation)          (VortexConnection * connection,
						  VortexStatus       status,
						  char             * status_message,
						  axlPointer         user_data);


/** 
 * @brief Handler definition for those function used to configure if a
 * given TLS request should be accepted or denied.
 *
 * Once a TLS request is received this handler will be executed to
 * notify user space application on top of Vortex Library if the request should be denied or not. The
 * handler will receive the connection where the request was received
 * and an optional value serverName which represents a request to act as 
 * the server name the value represent.
 * 
 * This handler definition is used by:
 *   - \ref vortex_tls_accept_negotiation
 * 
 * @param connection The connection where the TLS request was received.
 * @param serverName Optional serverName value requesting, if defined, to act as the server defined by this value.
 * 
 * @return true if the TLS request should be accepted. false to deny
 * the session TLS-fication.
 */
typedef bool     (*VortexTlsAcceptQuery) (VortexConnection * connection,
					  char             * serverName);

/** 
 * @brief Handler definition for those function allowing to locate the
 * certificate file to be used while enabling TLS support.
 * 
 * Once an TLS negotiation is started two files are required to
 * enable TLS ciphering: the certificate and the private key. Two
 * handlers are used by the Vortex Library to allow user app level to
 * configure file locations for both files.
 * 
 * This handler is used to configure location for the certificate
 * file. The function will receive the connection where the TLS is
 * being request to be activated and the serverName value which hold a
 * optional host name value requesting to act as the server configured
 * by this value.
 * 
 * The function must return a path to the certificate using a
 * dynamically allocated value. Once finished, the Vortex Library will
 * unref it.
 * 
 * <b>The function should return a basename file avoiding full path file
 * names</b>. This is because the Vortex Library will use \ref
 * vortex_support_find_data_file function to locate the file
 * provided. That function is configured to lookup on the configured
 * search path provided by \ref vortex_support_add_search_path or \ref
 * vortex_support_add_search_path_ref.
 * 
 * As a consequence: 
 * 
 * - If all certificate files are located at
 *  <b>/etc/repository/certificates</b> and the <b>serverName.cert</b> is to
 *   be used <b>DO NOT</b> return on this function <b>/etc/repository/certificates/serverName.cert</b>
 *
 * - Instead, configure <b>/etc/repository/certificates</b> at \ref
 *    vortex_support_add_search_path and return <b>servername.cert</b>.
 * 
 * - Doing previous practices will allow your code to be as
 *   platform/directory-structure independent as possible. The same
 *   function works on every installation, the only question to be
 *   configured are the search paths to lookup.
 *  
 * 
 * @param connection The connection where the TLS negotiation was received.
 *
 * @param serverName An optional value requesting to as the server
 * <b>serverName</b>. This value is supposed to be used to select the
 * right certificate file.
 * 
 * This handler is used by:
 *  - \ref vortex_tls_accept_negotiation 
 * 
 * @return A newly allocated value containing the path to the certificate file.
 */
typedef char  * (* VortexTlsCertificateFileLocator) (VortexConnection * connection,
						     char             * serverName);
	
/** 
 * @brief Handler definition for those function allowing to locate the
 * private key file to be used while enabling TLS support.
 * 
 * See \ref VortexTlsCertificateFileLocator handler. This handler
 * allows to define how is located the private key file used for the
 * session TLS-fication.
 * 
 * This handler is used by:
 *  - \ref vortex_tls_accept_negotiation 
 * 
 * @return A newly allocated value containing the path to the private key file.
 */
typedef char  * (* VortexTlsPrivateKeyFileLocator) (VortexConnection * connection,
						    char             * serverName);

/** 
 * @brief Handler definition used by the TLS profile, to allow the
 * application level to provide the function that must be executed to
 * create an (SSL_CTX *) object, used to perform the TLS activation.
 *
 * This handler is used by: 
 *  - \ref vortex_tls_set_ctx_creation
 *  - \ref vortex_tls_set_default_ctx_creation
 *
 * By default the Vortex TLS implementation will use its own code to
 * create the SSL_CTX object if not provided the handler. However,
 * such code is too general, so it is recomended to provide your own
 * context creation.
 *
 * Inside this function you must configure all your stuff to tweak the
 * OpenSSL behaviour. Here is an example:
 * 
 * \code
 * axlPointer * __ctx_creation (VortexConnection * conection,
 *                              axlPointer         user_data)
 * {
 *     SSL_CTX * ctx;
 *
 *     // create the context using the TLS method (for client side)
 *     ctx = SSL_CTX_new (TLSv1_method ());
 *
 *     // configure the root CA and its directory to perform verifications
 *     if (SSL_CTX_load_verify_locations (ctx, "your-ca-file.pem", "you-ca-directory")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *     if (SSL_CTX_set_default_verify_paths () != 1) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client certificate (public key)
 *     if (SSL_CTX_use_certificate_chain_file (ctx, "your-client-certificate.pem")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client private key 
 *     if (SSL_CTX_use_PrivateKey_file (ctx, "your-client-private-key.rpm", SSL_FILETYPE_PEM)) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // set the verification level for the client side
 *     SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
 *     SSL_CTX_set_verify_depth(ctx, 4);
 *
 *     // our ctx is configured
 *     return ctx;
 * }
 * \endcode
 *
 * For the server side, the previous example mostly works, but you
 * must reconfigure the call to SSL_CTX_set_verify, providing
 * something like this:
 * 
 * \code
 *    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
 * \endcode
 *
 * See OpenSSL documenation for SSL_CTX_set_verify and SSL_CTX_set_verify_depth.
 * 
 * @param connection The connection that has been requested to be
 * activated the TLS profile, for which a new SSL_CTX must be created. 
 * 
 * @param user_data An optional user pointer defined at either \ref
 * vortex_tls_set_default_ctx_creation and \ref
 * vortex_tls_set_ctx_creation.
 * 
 * @return You must return a newly allocated SSL_CTX or NULL if the
 * handler must signal that the TLS activation must not be performed.
 */
typedef axlPointer (* VortexTlsCtxCreation) (VortexConnection * connection,
					     axlPointer         user_data);

/** 
 * @brief Allows to configure a post-condition function to be executed
 * to perform an addiontional checking.
 *
 * This handler is used by:
 * 
 *  - \ref vortex_tls_set_post_check
 *  - \ref vortex_tls_set_default_post_check
 *
 * The function must return true to signal that checkings are passed,
 * otherwise false must be returned. In such case, the connection will
 * be dropped.
 * 
 * @param connection The connection that was TLS-fixated and
 * addiontional checks was configured.
 * 
 * @param user_data User defined data passed to the function, defined
 * at \ref vortex_tls_set_post_check and \ref
 * vortex_tls_set_default_post_check.
 *
 * @param ssl The SSL object created for the process.
 * 
 * @param ctx The SSL_CTX object created for the process.
 * 
 * @return true to accept the connection, otherwise, false must be
 * returned.
 */
typedef bool (*VortexTlsPostCheck) (VortexConnection * connection, 
				    axlPointer         user_data, 
				    axlPointer         ssl, 
				    axlPointer         ctx);
						  

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
 * @return returns true if the socket descriptor was added, otherwise,
 * false is returned.
 */
typedef bool     (* VortexIoAddToFdGroup)        (int                    fds,
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
 * @return true if the socket descriptor is active in the given fd
 * group.
 *
 */
typedef bool     (* VortexIoIsSetFdGroup)        (int                    fds,
						  axlPointer             fd_group,
						  axlPointer             user_data);

/** 
 * @brief Handler definition to allow implementing the have dispatch
 * function at the vortex io module.
 *
 * An I/O wait implementation must return true to notify vortex engine
 * it support automatic dispatch (which is a far better mechanism,
 * supporting better large set of descriptors), or false, to notify
 * that the \ref vortex_io_waiting_set_is_set_fd_group mechanism must
 * be used.
 *
 * In the case the automatic dispatch is implemented, it is also
 * required to implement the \ref VortexIoDispatch handler.
 * 
 * @param fd_group A reference to the object created by the I/O waiting mechanism.
 * p
 * @return Returns true if the I/O waiting mechanism support automatic
 * dispatch, otherwise false is returned.
 */
typedef bool     (* VortexIoHaveDispatch)         (axlPointer             fd_group);

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
 * also be implemented, making it to always return true. If this two
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
 * @brief Profile mask handler to perform profile filtering functions.
 *
 * This handler definition is used by:
 *  - \ref vortex_connection_set_profile_mask
 *
 * The handler (a mask) is executed for each profile to be
 * filtered. The function based on the data received must return true
 * (to filter a profile) or false (to not filter it).
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
 * @param serverName A request for the connection to act as
 * serverName.
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
 *      // return true to filter the uri
 *      return true;
 * }
 * \endcode
 * 
 * NOTE: You must not define error_msg and return false. 
 *
 * @param user_data User defined pointer passed to the function.
 * 
 * @return true to filter the uri, false if not.
 */
typedef bool     (* VortexProfileMaskFunc)       (VortexConnection      * connection,
						  int                     channel_num,
						  const char            * uri,
						  const char            * profile_content,
						  const char            * serverName,
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
 * @return true to select the channel, otherwise false must be
 * returned.
 */
typedef bool (*VortexChannelSelector) (VortexChannel * channel, 
				       axlPointer      user_data);

/** 
 * @brief Handler definition used to notify that a channel was added
 * or removed from a particular connection.
 *
 * In the case the channel is added, the function is just after it was
 * fully added to the connection. In the case the channel was removed,
 * the handler is called just before.
 *
 * The handler is used the following two functions:
 * 
 * - \ref vortex_connection_set_channel_added_handler
 * - \ref vortex_connection_set_channel_removed_handler
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
 * @param ctx The context where the operation is taking place.
 * 
 * @param conn The connection that is notified.
 *
 * @param new_conn In the case the action creates a new connection
 * that must replace the connection received, this variable is used to
 * notify the new reference.
 *
 * @param state The stage during the notification is taking place.
 *
 * @param user_data User defined data associated to the action. This
 * was configured at \ref vortex_connection_set_connection_actions.
 * 
 * @return The function must return a set of codes that are used by
 * the library to handle errors. The action must return:
 *
 * - (-1) in the case an error was found during the action
 * processing. In this case, the connection is closed and a closed
 * connection is returned to the caller of \ref vortex_connection_new.
 *
 * - (0) in the case the function want to stop action processing. This
 * is useful to block other actions.
 *
 * - (1) in the case the function executed the action without errors.
 *
 * - (2) in the case the function creates a new connection and it must
 * be replaced, the caller must return (2) and fill the variable
 * new_connection. In this case, the action is entirely responsible of
 * the connection received and its deallocation.
 */
typedef int (*VortexConnectionAction)    (VortexCtx               * ctx,
					  VortexConnection        * conn,
					  VortexConnection       ** new_conn,
					  VortexConnectionStage     state,
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

#endif

/* @} */

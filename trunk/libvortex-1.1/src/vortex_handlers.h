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
 *  - \ref vortex_tunnel_accept_negotiation
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
 * vortex_tls_start_negociation function, the status for such process is notified
 * using this handler type definition.
 *
 * The <i>status</i> value have to be checked in order to know if the
 * transport negotiation have finished successfully. Along the previous
 * variable, the <i>status_message</i> have a textual diagnostic about
 * the current status received.
 * 
 * While invoking \ref vortex_tls_start_negociation you could provide an user
 * space pointer, using the <i>user_data</i> parameter. That user data
 * is received on this handler.
 *
 * Functions using this handler:
 *  \ref vortex_tls_start_negociation
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
 *   - \ref vortex_tls_accept_negociation
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
 *  - \ref vortex_tls_accept_negociation 
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
 *  - \ref vortex_tls_accept_negociation 
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
 * @brief Async notifications for SASL auth process. 
 *
 * This notification handler is used to report user space what is the
 * final status for the SASL profile negotiation selected. 
 *
 * The handler report an <b>status</b> variable and a textual
 * diagnostic error on <b>status_message</b>.
 *
 * This function is used by: 
 *   - \ref vortex_sasl_start_auth
 *
 *
 * @param connection     The connection where the SASL process have ended
 * @param status         Final status for the SASL process
 * @param status_message A textual diagnostic
 * @param user_data      User defined data provided at the function receiving this handler.
 */
typedef void     (*VortexSaslAuthNotify)          (VortexConnection * connection,
						   VortexStatus       status,
						   char             * status_message,
						   axlPointer         user_data);

/** 
 * @brief Asynchronous notification to enable user space to accept or
 * deny authentication for SASL EXTERNAL profile.
 *
 * Functions using this handler are:
 *   - \ref vortex_sasl_set_external_validation
 * 
 * @param connection The connection where the request was received.
 * @param authorization_id The authorization id to authenticate.
 * 
 * @return true to authenticate and allow request received or false to deny it.
 */
typedef bool         (*VortexSaslAuthExternal)        (VortexConnection * connection,
						       const char       * authorization_id);

/** 
 * @brief Asynchronous notification to enable user space to accept or
 * deny authentication for SASL EXTERNAL profile, and passes a user defined pointer.
 *
 * Functions using this handler are:
 *   - \ref vortex_sasl_set_external_validation_full
 * 
 * @param connection The connection where the request was received.
 * @param authorization_id The authorization id to authenticate.
 * @param user_data The user defined pointer to be passed to future SASL callbacks
 * 
 * @return true to authenticate and allow request received or false to deny it.
 */
typedef bool         (*VortexSaslAuthExternalFull)        (VortexConnection * connection,
						       const char       * authorization_id,
							   axlPointer         user_data);

/** 
 * @brief Asynchronous notification to enable user space to accept or
 * deny anonymous authentication for SASL ANONYMOUS profile.
 * 
 * Function using this definition:
 * - \ref vortex_sasl_set_anonymous_validation
 * 
 * @param connection The connection where the anonymous request was received.
 *
 * @param anonymous_token The anonymous token provided by the remote
 * client side. You must not modify or deallocate this value.
 * 
 * @return true to validate the incoming anonymous auth request. false to deny it.
 */
typedef bool         (*VortexSaslAuthAnonymous)       (VortexConnection * connection,
						       const char       * anonymous_token);

							   /** 
 * @brief Asynchronous notification to enable user space to accept or
 * deny anonymous authentication for SASL ANONYMOUS profile.
 * 
 * Function using this definition:
 * - \ref vortex_sasl_set_anonymous_validation_full
 * 
 * @param connection The connection where the anonymous request was received.
 *
 * @param anonymous_token The anonymous token provided by the remote
 * client side. You must not modify or deallocate this value.
 * 
 * @param user_data The user defined pointer to be passed to future SASL callbacks
 *
 * @return true to validate the incoming anonymous auth request. false to deny it.
 */
typedef bool         (*VortexSaslAuthAnonymousFull)       (VortexConnection * connection,
						       const char       * anonymous_token,
							   axlPointer         user_data);

/** 
 * @brief Asynchronous notification to enable user space code to
 * validate SASL PLAIN request received.
 * 
 * Function using this handler are:
 *  - \ref vortex_sasl_set_plain_validation
 * 
 * 
 * @param connection The connection where the SASL notification was received.
 * @param auth_id User identifier
 * @param authorization_id User identifier which is acting on behalf of.
 * @param password Current user password
 * 
 * @return true to accept incoming SASL request or false to deny it.
 */
typedef bool         (*VortexSaslAuthPlain)           (VortexConnection * connection,
						       const char       * auth_id,
						       const char       * authorization_id,
						       const char       * password);

/** 
 * @brief Asynchronous notification to enable user space code to
 * validate SASL PLAIN request received.
 * 
 * Function using this handler are:
 *  - \ref vortex_sasl_set_plain_validation_full
 * 
 * 
 * @param connection The connection where the SASL notification was received.
 * @param auth_id User identifier
 * @param authorization_id User identifier which is acting on behalf of.
 * @param password Current user password
 * @param user_data The user defined pointer to be passed to future SASL callbacks.
 * 
 * @return true to accept incoming SASL request or false to deny it.
 */
typedef bool         (*VortexSaslAuthPlainFull)           (VortexConnection * connection,
							   const char       * auth_id,
							   const char       * authorization_id,
							   const char       * password,
							   axlPointer         user_data);

/** 
 * @brief Asynchronous notification to enable user space code to
 * validate SASL CRAM MD5 request received.
 *
 * Inside this function, the programmer must implement the password
 * lookup for the given user (<b>auth_id</b>) and return a copy to it
 * in clear text.
 *
 * Functions using this handler are:
 *  - \ref vortex_sasl_set_cram_md5_validation
 * 
 * @param connection The connection where the SASL notification was received
 * @param auth_id User id to authenticate.
 * 
 * @return The password the given auth_id have or NULL if the request
 * must be denied. Password returned must be the in plain text,
 * without any modification, as it would be introduced by the remote
 * peer. Later, the SASL engine will perform the hash operation to be
 * compared with the hash value received from the client
 * side. <b>Returned value must be dynamically allocated. </b>
 */
typedef char  *      (*VortexSaslAuthCramMd5)         (VortexConnection * connection,
						       const char       * auth_id);

/** 
 * @brief Asynchronous notification to enable user space code to
 * validate SASL CRAM MD5 request received.
 *
 * Inside this function, the programmer must implement the password
 * lookup for the given user (<b>auth_id</b>) and return a copy to it
 * in clear text.
 *
 * Functions using this handler are:
 *  - \ref vortex_sasl_set_cram_md5_validation_full
 * 
 * @param connection The connection where the SASL notification was received
 * @param auth_id User id to authenticate.
 * @param user_data The user defined pointer to be passed to future SASL callbacks.
 * 
 * @return The password the given auth_id have or NULL if the request
 * must be denied. Password returned must be the in plain text,
 * without any modification, as it would be introduced by the remote
 * peer. Later, the SASL engine will perform the hash operation to be
 * compared with the hash value received from the client
 * side. <b>Returned value must be dynamically allocated. </b>
 */
typedef char  *      (*VortexSaslAuthCramMd5Full)         (VortexConnection * connection,
							   const char       * auth_id,
							   axlPointer         user_data);

/** 
 * @brief Asynchronous notification to enable user space to validate
 * SASL DIGEST MD5 received requests.
 *
 * Inside this function, the programmer must implement the password
 * lookup for the given user (<b>auth_id</b>) and return a copy to it
 * in clear text.
 *
 * Functions using this handler are:
 *   - \ref vortex_sasl_set_digest_md5_validation
 * 
 * @param connection The connection where the SASL notification was received.
 * @param auth_id User identifier to authenticate
 * @param authorization_id If set, requesting auth_id is asking to get authorized to act as this value.
 * @param realm Optional realm value where the auth_id and the authorization_id will be validated.
 * 
 * @return The password the given auth_id have or NULL if the request
 * must be denied. Password returned must be the in plain text,
 * without any modification, as it would be introduced by the remote
 * peer. Later, the SASL engine will perform the hash operation to be
 * compared with the hash value received from the client
 * side. <b>Returned value must be dynamically allocated. </b>
 */
typedef char  *      (*VortexSaslAuthDigestMd5)       (VortexConnection * connection,
						       const char       * auth_id,
						       const char       * authorization_id,
						       const char       * realm);

/** 
 * @brief Asynchronous notification to enable user space to validate
 * SASL DIGEST MD5 received requests.
 *
 * Inside this function, the programmer must implement the password
 * lookup for the given user (<b>auth_id</b>) and return a copy to it
 * in clear text.
 *
 * Functions using this handler are:
 *   - \ref vortex_sasl_set_digest_md5_validation_full
 * 
 * @param connection The connection where the SASL notification was received.
 * @param auth_id User identifier to authenticate
 * @param authorization_id If set, requesting auth_id is asking to get authorized to act as this value.
 * @param realm Optional realm value where the auth_id and the authorization_id will be validated.
 * @param user_data The user defined pointer to be passed to future SASL callbacks.
 * 
 * @return The password the given auth_id have or NULL if the request
 * must be denied. Password returned must be the in plain text,
 * without any modification, as it would be introduced by the remote
 * peer. Later, the SASL engine will perform the hash operation to be
 * compared with the hash value received from the client
 * side. <b>Returned value must be dynamically allocated. </b>
 */
typedef char  *      (*VortexSaslAuthDigestMd5Full)       (VortexConnection * connection,
							   const char       * auth_id,
							   const char       * authorization_id,
							   const char       * realm,
							   axlPointer         user_data);

/** 
 * @brief Async notification handler for the XML-RPC channel boot.
 *
 * This async handler is used by: 
 *  - \ref vortex_xml_rpc_boot_channel
 * 
 * @param booted_channel The channel already booted or NULL if
 * something have happened.
 *
 * @param status The status value for the XML-RPC boot process
 * request, \ref VortexOk if boot was ok, \ref VortexError if something
 * have happened.
 *
 * @param message A textual diagnostic message reporting current status.
 *
 * @param user_data User space data defined at the function requesting
 * this handler.
 */
typedef void (* VortexXmlRpcBootNotify)               (VortexChannel    * booted_channel,
						       VortexStatus       status,
						       char             * message,
						       axlPointer         user_data);

/** 
 * @brief This async handler allows to control how is accepted XML-RPC
 * initial boot channel based on a particular resource.
 *
 * XML-RPC specificaiton states that the method invocation is
 * considered as a two-phase invocation. 
 *
 * The first phase, the channel boot, is defined as a channel
 * creation, where the XML-RPC profile is prepared and, under the same
 * step, it is asked for a particular resource value to be supported
 * by the server (or the listener which is actually the entity
 * receiving the invocation).
 *
 * The second step is actually the XML-RPC invocation. 
 *
 * This resource value, by default, is "/". This allows to group
 * services under resources like: "/sales", "/sales/dep-a", etc. This
 * also allows to ask to listerners if they support a particular
 * interface. Think about grouping services, which represents a
 * concrete interface, under the same resource. Then, listener that
 * supports the resource are reporting that they, indeed, support a
 * particular interface.
 *
 * However, resource validation is not required at all. You can live
 * without it safely.
 *
 * As a note, you can pass a NULL handler reference to the \ref
 * vortex_xml_rpc_accept_negociation, making the Vortex XML-RPC engine
 * to accept all resources (resource validation always evaluated to
 * true).
 *
 * This function is used by:
 *
 *  - \ref vortex_xml_rpc_accept_negociation
 *
 * Here is an example of a resource validation handler that accept all
 * resources:
 * \code
 * bool     validate_all_resources (VortexConnection * connection,
 *                                  int                channel_number,
 *                                  char             * serverName,
 *                                  char             * resource_path,
 *                                  axlPointer         user_data)
 * {
 *     // This is a kind of useless validation handler because 
 *     // it is doing the same like passing a NULL reference.
 *     // 
 *     // In this handler could be programmed any policy to validate a resource,
 *     // maybe based on the connection source,  the number of channels that the 
 *     // connection have, etc.
 *     return true;
 * }
 * \endcode
 * 
 * @param connection     The connection where the resource bootstrapping request was received.
 * @param channel_number The channel number to create requested.
 * @param serverName     An optional serverName value to act as.
 * @param resource_path  The resource path requested.
 * @param user_data      User space data.
 * 
 * @return true to accept resource requested. false if not.
 */
typedef bool     (*VortexXmlRpcValidateResource) (VortexConnection * connection, 
						  int                channel_number,
						  const char       * serverName,
						  const char       * resource_path,
						  axlPointer         user_data);


/** 
 * @brief Async handler to process all incoming service invocation
 * through XML-RPC.
 *
 * This handler is executed to notify user space that a method
 * invocation has been received and should be dispatched, returning a
 * \ref XmlRpcMethodResponse. This reponse (\ref XmlRpcMethodResponse)
 * will be used to generate a reply as fast as possible.
 *
 * The method invocation, represented by this async handler, could
 * also return a NULL value, making it possible to not reply at the
 * moment the reply was received, but once generated the reply, the
 * method call object (\ref XmlRpcMethodCall) received should be used
 * in conjuction with the method response associated (\ref
 * XmlRpcMethodResponse) calling to \ref vortex_xml_rpc_notify_reply.
 *
 * Previous function, \ref vortex_xml_rpc_notify_reply, could allow to
 * marshall the invocation into another language execution, making it
 * easy because it is only required to marshall the method invocator
 * received but not to have a handler inside the runtime for the other
 * language. However, this is mainly provided to make it possible to defer
 * the reply.
 *
 *
 * @param channel The channel where the method call was received.
 *
 * @param method_call The method call object received, representing
 * the method being invoked (\ref XmlRpcMethodCall).
 *
 * @param user_data User space data. 
 *
 * @return The dispatch function should return a \ref
 * XmlRpcMethodResponse containing the value reply to be returned or
 * NULL if the method invocation have been deferred. 
 */
typedef XmlRpcMethodResponse *  (*VortexXmlRpcServiceDispatch)  (VortexChannel    * channel,
								 XmlRpcMethodCall * method_call,
								 axlPointer         user_data);

/** 
 * @brief Async method response notifier.
 *
 * This handler is provided to enable Vortex XML-RPC engine to notify
 * the user space that a method response has been received.
 * 
 * This handler is used by:
 *
 *   - \ref vortex_xml_rpc_invoke
 * 
 * @param channel  The channel where the invocation took place.
 * @param response The reply received from the remote invocation.
 * @param user_data User defined data.
 * 
 */
typedef void     (* XmlRpcInvokeNotify)          (VortexChannel        * channel, 
						  XmlRpcMethodResponse * response,
						  axlPointer             user_data);


/** 
 * @brief IO handler definition to allow defining the method to be
 * invoked while createing a new fd set.
 *
 * @param wait_to Allows to configure the file set to be prepared to
 * be used for the set of operations provided. 
 * 
 * @return A newly created fd set pointer, opaque to Vortex, to a
 * structure representing the fd set, that will be used to perform IO
 * waiting operation at the \ref vortex_io "Vortex IO module".
 * 
 */
typedef axlPointer   (* VortexIoCreateFdGroup)        (VortexIoWaitingFor wait_to);

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
 * @param user_data User defined pointer passed to the function.
 * 
 * @return true to filter the uri, false if not.
 */
typedef bool     (* VortexProfileMaskFunc)       (VortexConnection      * connection,
						  int                     channel_num,
						  const char            * uri,
						  const char            * profile_content,
						  const char            * serverName,
						  axlPointer              user_data);

/** 
 * @brief Handler definition for the tunnel location resolution.
 *
 * This handler is used by the TUNNEL implementation to provide a way
 * to the user space code to translate tunnel locations provided. 
 *
 * Currently this is used by Turbulence to provide run-time
 * translation for endpoint and profile configurations into host and
 * port locations.
 * 
 * @param tunnel_spec The xml string defining the tunnel spec as
 * defined in RFC3620.
 * 
 * @param tunnel_sepc_size The size of the xml content.
 *
 * @param user_data Reference to user defined data.
 *
 * @param doc A reference to an already parsed document. 
 * 
 * @return A reference to the \ref VortexTunnelSettings created with
 * the new values. If null reference is returned, the TUNNEL engine
 * will use the content as provided, without performing any
 * translation.
 */
typedef VortexTunnelSettings * (* VortexTunnelLocationResolver) (const char  * tunnel_spec,
								 int           tunnel_spec_size,
								 axlDoc      * tunnel_doc,
								 axlPointer    user_data);

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
 * once a connection is created. This handler is used by the
 * following function:
 *
 * - \ref vortex_connection_notify_new_connections
 *
 * @param created The connection that was created and properly
 * registered.
 *
 * @param user_data User defined data configured at the same time as
 * the handler.
 * 
 */
typedef void (*VortexConnectionNotifyNew) (VortexConnection * created, axlPointer user_data);

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

#endif

/* @} */

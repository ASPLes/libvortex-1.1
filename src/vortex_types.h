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
#ifndef __VORTEX_TYPES_H__
#define __VORTEX_TYPES_H__

/**
 * \defgroup vortex_types Vortex Types: Types definitions used across Vortex Library.
 */
 
/** 
 * \addtogroup vortex_types
 * @{
 */


/*
 * @brief Debug levels to be used with \ref _vortex_log, which is used
 * through vortex_log macro.
 *
 * The set of functions allowing to activate the debug at run time and
 * its variants are:
 * 
 * - \ref vortex_log_is_enabled
 * - \ref vortex_log2_is_enabled
 * - \ref vortex_log_enable
 * - \ref vortex_log2_enable
 *
 * Activate console color log (using ansi characters):
 * 
 * - \ref vortex_color_log_is_enabled
 * - \ref vortex_color_log_enable
 *
 * To lock the log during its emision to avoid several threads doing
 * log at the same time:
 * 
 * - \ref vortex_log_is_enabled_acquire_mutex
 * - \ref vortex_log_acquire_mutex
 *
 * Finally, to make the application level to configure a handler:
 * 
 * - \ref vortex_log_set_handler
 * - \ref vortex_log_get_handler
 * 
 * @param domain Domain that is registering a log.
 *
 * @param level Log level that is being registered.
 *
 * @param message Message that is being registered.
 */
typedef enum {
	/** 
	 * @internal Log a message as a debug message.
	 */
	VORTEX_LEVEL_DEBUG    = 1 << 0,
	/** 
	 * @internal Log a warning message.
	 */
	VORTEX_LEVEL_WARNING  = 1 << 1,
	/** 
	 * @internal Log a critical message.
	 */
	VORTEX_LEVEL_CRITICAL = 1 << 2,
} VortexDebugLevel;

/**
 * @brief Macro declaration to protect code from misspelling
 * "MIME-Version" MIME header. This macro can be used by \ref
 * vortex_frame_set_mime_header and \ref vortex_frame_get_mime_header.
 */
#define MIME_VERSION "MIME-Version"
  
/**
 * @brief Macro declaration to protect code from misspelling
 * "Content-Type" MIME header. This macro can be used by \ref
 * vortex_frame_set_mime_header and \ref vortex_frame_get_mime_header.
 */
#define MIME_CONTENT_TYPE "Content-Type"

/**
 * @brief Macro declaration to protect code from misspelling
 * "Content-Transfer-Encoding" MIME header. This macro can be used by \ref
 * vortex_frame_set_mime_header and \ref vortex_frame_get_mime_header.
 */
#define MIME_CONTENT_TRANSFER_ENCODING "Content-Transfer-Encoding"

/**
 * @brief Macro declaration to protect code from misspelling
 * "Content-ID" MIME header. This macro can be used by \ref
 * vortex_frame_set_mime_header and \ref vortex_frame_get_mime_header.
 */
#define MIME_CONTENT_ID "Content-ID"

/**
 * @brief Macro declaration to protect code from misspelling
 * "Content-Description" MIME header. This macro can be used by \ref
 * vortex_frame_set_mime_header and \ref vortex_frame_get_mime_header.
 */
#define MIME_CONTENT_DESCRIPTION "Content-Description"


/** 
 * @brief Describes the type allowed for a frame or the type a frame actually have.
 *
 * Every frame used inside Vortex Library have one of the following values.
 */
typedef enum {
	/** 
	 * @brief The frame type is unknown, used to represent errors
	 * across the Vortex Library.
	 * 
	 * This only means an error have happen while receiving the
	 * frame or while creating a new one. This frame have no valid
	 * use inside the Vortex Library.
	 */
	VORTEX_FRAME_TYPE_UNKNOWN, 
	/**
	 * @brief Frame type is MSG. It is used to represent new
	 * Vortex messages.
	 *
	 * This frame type represent a new message or a frame which
	 * belongs to a set of frame conforming a message. This types
	 * of frames are generated while using the following function:
	 *
	 *   - \ref vortex_channel_send_msg
	 *   - \ref vortex_channel_send_msgv
	 *   - \ref vortex_channel_send_msg_and_wait
	 *   - \ref vortex_channel_send_msg_and_waitv
	 */
	VORTEX_FRAME_TYPE_MSG,
	/** 
	 * @brief Frame type is RPY. It is used to represent reply
	 * frames to MSG frames called positive replies.
	 *
	 * This frame type represent replies produced to frame with
	 * type MSG. The following function allows to generate frame
	 * types for RPY:
	 * 
	 *  - \ref vortex_channel_send_rpy
	 *  - \ref vortex_channel_send_rpyv
	 */
	VORTEX_FRAME_TYPE_RPY,
	/**
	 * @brief Frame type is ANS. It is used to represent reply
	 * frames to MSG frames using the type ANS.
	 */
	VORTEX_FRAME_TYPE_ANS,
	/**
	 * @brief Frame type is ERR. It is used to represent reply
	 * frames to MSG frames called negative replies.
	 *
	 * This frame type is used to represent negative replies to
	 * received frames with type MSG. The following functions
	 * allows to generate ERR frames:
	 *    
	 *   - \ref vortex_channel_send_err
	 *   - \ref vortex_channel_send_errv
	 */

	VORTEX_FRAME_TYPE_ERR,
	/**
	 * @brief Frame type is NUL. It is used to represent reply
	 * frames to MSG ending a set of ANS frame replies.
	 */
	VORTEX_FRAME_TYPE_NUL,
	/** 
	 * @brief Frame type is SEQ. This is a especial frame type
	 * used to allow application level to tweak how big is the
	 * window size for a given channel.
	 */
	VORTEX_FRAME_TYPE_SEQ
}VortexFrameType;


/** 
 * @brief Maximum buffer value to be read from the network in a
 * single operation.
 */
#define VORTEX_MAX_BUFFER_SIZE 131072

/** 
 * @brief Maximum sequence number allowed to be used for a channel created.
 * 
 * The sequencer number is used inside Vortex Library to keep up to
 * date synchronization inside a channel while receiving and sending
 * data. This number represent the byte where the stream the channel
 * is transporting is supposed to start for further transmissions.
 * 
 */
#define MAX_SEQ_NO     (INT64_CONSTANT(4294967295))

/** 
 * @brief Module number used to rotate sequence number for a channel if it overcomes \ref MAX_SEQ_NO.
 *
 * While Vortex Library is operating with a channel, it may happen a
 * channel overcome \ref MAX_SEQ_NO so channel sequence number is
 * rotated using a modulo operation with \ref MAX_SEQ_MOD which is
 * just \ref MAX_SEQ_NO plus 1 unit.
 * 
 * 
 */
#define MAX_SEQ_MOD (INT64_CONSTANT(4294967296))

/** 
 * @brief Maximum number of messages allowed to be sent for a specific channel instance.
 *
 * As defined by RFC 3080 every channel created cannot repeat message
 * number sent. Every message sent have an unique identifier used to
 * reply to it and to differentiate them from each other.
 *
 * A channel cannot sent more messages that the number defined for
 * MAX_MSG_NO. Once a channel reach that number it cannot be used for
 * any new message. This doesn't mean this channel must be closed. The
 * channel still have pending to receive all replies generated for
 * every message created.
 *
 */
#define MAX_MSG_NO     2147483647

/**
 * @brief Maximum number of channel allowed to be created inside a VortexConnection.
 * 
 * This number defines how many channels can be created inside a Vortex Connection.
 */
#define MAX_CHANNEL_NO 2147483647


/**
 * @brief The maximum number which can identify a message.
 * 
 */
#define MAX_MESSAGE_NO  2147483647


/**
 * @brief the maximum number of channel which can be created on an connection (or session).
 * 
 */
#define MAX_CHANNELS_NO 2147483647

/**
 * @brief The maximum number to be used on sequence frame payload 
 * 
 */
#define MAX_SEQUENCE_NO 4294967295

typedef enum {
 UPDATE_SEQ_NO         = 1 << 0,
 UPDATE_MSG_NO         = 1 << 1,
 UPDATE_RPY_NO         = 1 << 2,
 UPDATE_ANS_NO         = 1 << 3,
 UPDATE_RPY_NO_WRITTEN = 1 << 4,
 DECREASE_MSG_NO       = 1 << 5,
 DECREASE_RPY_NO       = 1 << 6
} WhatUpdate;

/** 
 * @brief Allows to specify which type of operation should be
 * implemented while calling to Vortex Library internal IO blocking
 * abstraction.
 */
typedef enum {
	/** 
	 * @brief A read watching operation is requested. If this
	 * value is received, the fd set containins a set of socket
	 * descriptors which should be watched for incoming data to be
	 * received.
	 */
	READ_OPERATIONS  = 1 << 0, 
	/** 
	 * @brief A write watching operation is requested. If this
	 * value is received, the fd set contains a set of socket that
	 * is being requested for its availability to perform a write
	 * operation on them.
	 */
	WRITE_OPERATIONS = 1 << 1
} VortexIoWaitingFor;

/**
 * @brief Vortex library context. This object allows to store the
 * library status into a single reference, representing an independent
 * execution context.
 *
 * Its normal usage is to create a context variable using \ref
 * vortex_ctx_new and then start an execution context by calling to
 * \ref vortex_init_ctx. This makes the library to start its
 * operation.
 *
 * Once finished, a call to stop the context is required: \ref
 * vortex_exit_ctx followed by a call to \ref vortex_ctx_free.
 *
 */
typedef struct _VortexCtx VortexCtx;

/**
 * @brief A Vortex Connection object (BEEP session representation).
 *
 * This object represent a connection inside the Vortex Library. It
 * can be created using \ref vortex_connection_new and then checked
 * with \ref vortex_connection_is_ok.
 *
 * Internal \ref VortexConnection representation is not exposed to
 * user code space to ensure the minimal impact while improving or
 * changing Vortex Library internals. 
 * 
 * To operate with a \ref VortexConnection object \ref vortex_connection "check out its API documentation".
 * 
 */
typedef struct _VortexConnection  VortexConnection;

/**
 * @brief A Vortex Frame object.
 *
 * This object represent a frame received or to be sent to remote Vortex (or BEEP enabled) nodes.
 *
 * Internal \ref VortexFrame representation is not exposed to user
 * code space to ensure the minimal impact while improving or changing Vortex Library internals. 
 *
 * To operate with a \ref VortexFrame object \ref vortex_frame "check out its API documentation".
 * 
 */
typedef struct _VortexFrame       VortexFrame;

/**
 * @brief A Vortex Channel object.
 *
 * This object represents a channel which enables to send and receive
 * data. A channel must be created inside a \ref VortexConnection by using \ref vortex_channel_new. 
 *
 * Internal \ref VortexChannel representation is not exposed to user
 * code space to ensure minimal impact while improving or changing
 * Vortex Library internals.
 *
 * To operate with a \ref VortexChannel object \ref vortex_channel "check out its API documentation".
 * 
 */
typedef struct _VortexChannel     VortexChannel;

/** 
 * @brief Thread safe hash table based on Glib Hash table intensively used across Vortex Library.
 *
 * Internal \ref VortexHash representation is not exposed to user
 * code space to ensure minimal impact while improving or changing
 * Vortex Library internals.
 *
 * To operate with a \ref VortexHash object \ref vortex_hash "check out its API documentation".
 * 
 */
typedef struct _VortexHash        VortexHash;

/** 
 * @brief Thread safe queue table based on Glib Queue used across Vortex Library.
 *
 * Internal \ref VortexQueue representation is not exposed to user
 * code space to ensure minimal impact while improving or changing
 * Vortex Library internals.
 *
 * To operate with a \ref VortexQueue object \ref vortex_queue "check out its API documentation".
 * 
 */
typedef struct _VortexQueue       VortexQueue;


/** 
 * @brief Circular byte buffer used by Vortex to efficiently pack
 * bytes in a circular buffer.
 */
typedef struct _VortexCBuffer     VortexCBuffer;

/** 
 * @brief Vortex Channel Pool definition.
 *
 * The \ref VortexChannelPool is an abstraction which allows managing several \ref vortex_channel "channels"
 *  using the same profile under the same \ref vortex_connection "connection". 
 *
 * The \ref VortexChannelPool allows to get better performance reducing the
 * impact of channel creation and channel closing because enforces
 * reusing the channels created in a manner the channels used from the
 * pool every time are always ready. 
 *
 * Internal \ref VortexChannelPool representation is not exposed to user
 * code space to ensure minimal impact while improving or changing
 * Vortex Library internals.
 *
 * To operate with a \ref VortexChannelPool object \ref vortex_channel_pool "check out its API documentation".
 * 
 */
typedef struct _VortexChannelPool VortexChannelPool;

/** 
 * @brief MIME header content representation, allowing to get the
 * content associated and the next value associated on the same header
 * (if were defined).
 *
 * This structure represents a single MIME header instance found on a
 * message. To get the MIME header name (left part of a MIME heeader
 * definition) use \ref vortex_frame_mime_header_name.
 *
 * To get the content associated to this MIME header instance use \ref
 * vortex_frame_mime_header_content. 
 *
 * To check if there are more instances defined for the same MIME
 * header name, use \ref vortex_frame_mime_header_next.
 */
typedef struct _VortexMimeHeader     VortexMimeHeader;

/** 
 * @brief Optional structure used to signal additional values to
 * modify how a connection is created. This is mainly used to request
 * features on session initialization.
 *
 * This type is used by \ref vortex_connection_new_full. See \ref
 * vortex_connection_opts_new for details on how to use this function.
 */
typedef struct _VortexConnectionOpts VortexConnectionOpts;

/** 
 * @brief Connection options. This options are used to modify default
 * connection behaviour like default serverName notification on first
 * channel.
 */
typedef enum {
	/** 
	 * @brief Signals option list termination.
	 */
	VORTEX_OPTS_END           = 0,
	/** 
	 * @brief Allows to request serverName feature (currently
	 * implemented as x-serverName). This connection option must
	 * be followed with a constant string having the serverName
	 * value that must be requested.
	 */
	VORTEX_SERVERNAME_FEATURE = 1,
	
	/** 
	 * @brief Allows to configure the serverName greetings feature
	 * as defined in the connection host name. This is the default
	 * behaviour starting from Vortex 1.1 (release 1.1.3). You can
	 * disable by settings this connection option to
	 * axl_false. This connection option is ignored in the case
	 * \ref VORTEX_SERVERNAME_FEATURE is provided.
	 */
	VORTEX_SERVERNAME_ACQUIRE = 2,

	/** 
	 * @brief In general you can reuse connection options (\ref
	 * VortexConnectionOpts) across connection creation operations
	 * (\ref vortex_connection_new_full). 
	 * 
	 * However, \ref CONN_OPTS macro is provided to allow
	 * configuring particular connection options for each
	 * operation. Thus this connection option is used to signal
	 * \ref vortex_connection_new_full to release memory
	 * allocated. 
	 * 
	 * This option must be followed by a boolean value: axl_true
	 * to signal release after connection creation, otherwise
	 * axl_false must be used (default value already configured).
	 */
	VORTEX_OPTS_RELEASE     = 3, 
	
} VortexConnectionOptItem;

/** 
 * @brief Vortex Operation Status.
 * 
 * This enum is used to represent different Vortex Library status,
 * especially while operating with \ref VortexConnection
 * references. Values described by this enumeration are returned by
 * \ref vortex_connection_get_status.
 */
typedef enum {
	/** 
	 * @brief Represents an Error while Vortex Library was operating.
	 *
	 * The operation asked to be done by Vortex Library could be
	 * completed.
	 */
	VortexError                  = 1,
	/** 
	 * @brief Represents the operation have been successfully completed.
	 *
	 * The operation asked to be done by Vortex Library have been
	 * completed.
	 */
	VortexOk                     = 2,

	/** 
	 * @brief The operation wasn't completed because an error to
	 * tcp bind call. This usually means the listener can be
	 * started because the port is already in use.
	 */
	VortexBindError              = 3,

	/** 
	 * @brief The operation can't be completed because a wrong
	 * reference (memory address) was received. This also include
	 * NULL references where this is not expected.
	 */
	VortexWrongReference         = 4,

	/** 
	 * @brief The operation can't be completed because a failure
	 * resolving a name was found (usually a failure in the
	 * gethostbyname function). 
	 */ 
	VortexNameResolvFailure      = 5,

	/** 
	 * @brief A failure was found while creating a socket.
	 */
	VortexSocketCreationError    = 6,

	/** 
	 * @brief Found socket created to be using reserved system
	 * socket descriptors. This will cause problems.
	 */
	VortexSocketSanityError      = 7,

	/** 
	 * @brief Connection error. Unable to connect to remote
	 * host. Remote hosting is refusing the connection.
	 */
	VortexConnectionError        = 8,

	/** 
	 * @brief Connection error after timeout. Unable to connect to
	 * remote host after after timeout expired.
	 */
	VortexConnectionTimeoutError = 9,

	/** 
	 * @brief Greetings error found. Initial BEEP senquence not
	 * completed, failed to connect.
	 */
	VortexGreetingsFailure       = 10,
	/** 
	 * @brief Failed to complete operation due to an xml
	 * validation error.
	 */
	VortexXmlValidationError     = 11,
	/** 
	 * @brief Connection is in transit to be closed. This is not
	 * an error just an indication that the connection is being
	 * closed at the time the call to \ref
	 * vortex_connection_get_status was done.
	 */
	VortexConnectionCloseCalled  = 12,
	/** 
	 * @brief The connection was terminated due to a call to \ref
	 * vortex_connection_shutdown or an internal implementation
	 * that closes the connection without taking place the BEEP
	 * session close negociation.
	 */
	VortexConnectionForcedClose  = 13,
	/** 
	 * @brief Found a protocol error while operating.
	 */
	VortexProtocolError          = 14,
	/** 
	 * @brief  The connection was closed or not accepted due to a filter installed. 
	 */
	VortexConnectionFiltered     = 15,
	/** 
	 * @brief Memory allocation failed.
	 */
	VortexMemoryFail             = 16,
	/** 
	 * @brief When a connection is closed by the remote side but
	 * without going through the BEEP clean close.
	 */
	VortexUnnotifiedConnectionClose = 17
} VortexStatus;


/** 
 * @brief In the context of the initial session creation, the BEEP
 * peer allows you to get which is the role of a given \ref VortexConnection object.
 *
 * This role could be used to implement some especial features
 * according to the peer type. Allowed values are: VortexRoleInitiator and
 * VortexRoleListener. 
 *
 * VortexListener value represent those connection that is accepting
 * inbound connections, while VortexInitiator value represent those
 * connections initiated to a BEEP peer acting as a server.
 *
 * You can get current role for a given connection using \ref vortex_connection_get_role.
 * 
 */
typedef enum {
	/** 
	 * @brief This value is used to represent an unknown role state.
	 */
	VortexRoleUnknown,
	
	/** 
	 * @brief The connection is acting as an Initiator one.
	 */
	VortexRoleInitiator,

	/** 
	 * @brief The connection is acting as a Listener one.
	 */
	VortexRoleListener,
	
	/** 
	 * @brief This especial value for the this enumeration allows
	 * to know that the connection is a listener connection
	 * accepting connections. It is not playing any role because
	 * any BEEP session was initiated.
	 *
	 * Connections reference that were received with the following
	 * functions are the only ones that can have this value:
	 *
	 * - \ref vortex_listener_new
	 * - \ref vortex_listener_new2
	 * - \ref vortex_listener_new_full
	 */
	VortexRoleMasterListener
	
} VortexPeerRole;


/** 
 * @brief Enum value used to report which encoding is being used for
 * the content profile send at the channel start phase. 
 * 
 * Check \ref vortex_channel_new_full documentation to know more about
 * how this enum is used.
 * 
 * By default, while sending profile content data, no encoding is used
 * and \ref EncodingNone must be used to report this fact. BEEP
 * definition allows to encode the profile content using Base64. On
 * this case, you have to use \ref EncodingBase64.
 */
typedef enum {
	/** 
	 * @brief Default value used to report that the encoding information should be ignored.
	 */
	EncodingUnknown = 1,
	/** 
	 * @brief No encoding was performed to the profile content data. 
	 */
	EncodingNone    = 2,
	/** 
	 * @brief The profile content data was encoded with Base64.
	 */
	EncodingBase64  = 3
} VortexEncoding;

/** 
 * @brief Type used to represent a payload feeder. This object
 * represents an abstration that allows to feed content directly into
 * the vortex sequencer, querying the object for data at the right
 * time with optimal sizes available on each moment.
 *
 * See \ref vortex_payload_feeder_file for a payload feeder example
 * that allows to feed as body for a message the content found at a
 * particular file.
 *
 * Once created the feeder, you send a message or reply using \ref
 * vortex_channel_send_msg_from_feeder, \ref
 * vortex_channel_send_ans_rpy_from_feeder...
 */
typedef struct _VortexPayloadFeeder VortexPayloadFeeder;


/** 
 * @brief Public structure that represents transfer status associated
 * to a single payload feeder transfer.
 */
typedef struct _VortexPayloadFeederStatus {
	/** 
	 * @brief How many bytes did the feeder sent so far.
	 */
	long bytes_transferred;
	/** 
	 * @brief How many bytes are expected to be sent.
	 */
	long total_size;
	/** 
	 * @brief Signals that the feeder has finished sending all
	 * content associated.
	 */
	axl_bool is_finished;
	
	/** 
	 * @brief Signals if the feeder is in pause state. It was
	 * issued a call to send content using this feeder but it was
	 * caused using \ref vortex_payload_feeder_pause.
	 */
	axl_bool is_paused;
} VortexPayloadFeederStatus;

/**
 * @internal
 *
 * This type allows vortex_channel_send_msg and
 * vortex_channel_common_rpy to communicate the sequencer a new
 * message must be sent.  This a private structure for inter thread
 * communication and *MUST NOT* be used by vortex library consumer.
 */
typedef struct _VortexSequencerData {
	/** 
	 * @brief The channel where the message should be sequenced.
	 */
	VortexChannel   * channel;

	/**
	 * @brief Reference to the connection.
	 */
	VortexConnection * conn;

	/** 
	 * @brief The type of the frame to be sequenced.
	 */
	VortexFrameType   type;

	/** 
	 * @brief The channel number to be sequenced (this is a
	 * redundancy data used for checking integrity).
	 */
	int              channel_num;

	/** 
	 * @brief The message number value to be used for frames
	 * sequenced from this message.
	 */
	int              msg_no;

	/** 
	 * Next sequence number to be used for the first byte on the
	 * message hold by this structure.
	 */
	unsigned int     first_seq_no;

	/** 
	 * @brief The content to be sequenced into frames.
	 */
	char            * message;

	/** 
	 * @brief The message size content.
	 */
        int              message_size;

	/** 
	 * @brief This is a tricky value and it is used to allow
	 * vortex sequencer to keep track about byte stream to be used
	 * while sending remaining data.
	 *
	 * Because it could happened that a message to be sent doesn't
	 * hold into the buffer that the remote peer holds for the
	 * channel, the sequencer could find that the entire message
	 * could not be send because the channel is stale. 
	 * 
	 * On this context the vortex sequencer queue the message to
	 * be pending and flags on steps how many bytes remains to be
	 * sent *for the given message.
	 */
	unsigned int     step;

	/** 
	 * @brief The ansno value to be used.
	 */
	int              ansno;

	/**
	 * @brief Discard flag used internally to drop packages that
	 * were queued to be sent.
	 */
	axl_bool         discard;

	/** 
	 * @brief Used to signal that the data contains a list of
	 * ANS/NUL pending replies that must be sent.
	 */
	axlList        * ans_nul_list;

	/** 
	 * @brief Optional feeder defined for this send operation.
	 */
	VortexPayloadFeeder * feeder;

	/** 
	 * @brief Signals if all frames sent due to this delivery
	 * should have all of them the more flag enabled.
	 */
	axl_bool              fixed_more;
} VortexSequencerData;



/**
 * @internal
 *
 * This type is used to transport the message to be sent into the
 * vortex writer process. At the end of the process, the vortex writer
 * relies on vortex_connection_do_a_sending_round to actually send
 * message waiting to be sent in a round robin fashion.
 * 
 * Because some internal vortex process needs to waits (and ensure)
 * for a message to be sent, this type enable that function, the
 * ..do_a_sending_round, to signal a possible thread waiting for a
 * message to be sent.
 */
typedef struct _VortexWriterData {
	VortexFrameType   type;
	axl_bool          msg_no;
	char            * the_frame;
	axl_bool          the_size;
	axl_bool          is_complete;
	axl_bool          fixed_more;
}VortexWriterData;

/** 
 * @brief Helper macro which allows to push data into a particular
 * queue, checking some conditions, which are logged at the particular
 * position if they fail.
 *
 * @param queue The queue to be used to push the new data. This reference can't be null.
 *
 * @param data The data to be pushed. This data can't be null.
 */
#define QUEUE_PUSH(queue, data)\
do {\
    if (queue == NULL) { \
       vortex_log (VORTEX_LEVEL_CRITICAL, "trying to push data in a null reference queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else if (data == NULL) {\
       vortex_log (VORTEX_LEVEL_CRITICAL, "trying to push null data in a queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else { \
       vortex_async_queue_push (queue,data);\
    }\
}while(0)

/** 
 * @brief Helper macro which allows to PRIORITY push data into a
 * particular queue, checking some conditions, which are logged at the
 * particular position if they fail. 
 *
 * @param queue The queue to be used to push the new data. This reference can't be null.
 *
 * @param data The data to be pushed. This data can't be null.
 */
#define QUEUE_PRIORITY_PUSH(queue, data)\
do {\
    if (queue == NULL) { \
       vortex_log (VORTEX_LEVEL_CRITICAL, "trying to push priority data in a null reference queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else if (data == NULL) {\
       vortex_log (VORTEX_LEVEL_CRITICAL, "trying to push priority null data in a queue at: %s:%d", __AXL_FILE__, __AXL_LINE__); \
    } else { \
       vortex_async_queue_priority_push (queue,data);\
    }\
}while(0)

/** 
 * @brief Configuration options available for \ref vortex_mutex_create_full function.
 */ 
typedef enum {
	/** 
	 * @brief Creates a non recursive mutex (calling twice to
	 * vortex_mutex_lock will block).
	 */
	VORTEX_MUTEX_CONF_NONRECURSIVE = 1 << 0,

	/** 
	 * @brief Creates a recursive mutex (calling twice to
	 * vortex_mutex_lock will not block).
	 */
	VORTEX_MUTEX_CONF_RECURSIVE    = 1 << 1,
} VortexMutexConf;


/** 
 * @internal Definitions to accomodate the underlaying thread
 * interface to the Vortex thread API.
 */
#if defined(AXL_OS_WIN32)

typedef struct _VortexWin32Mutex {
	HANDLE     mutex;
	axl_bool   recursive;
	VortexMutexConf conf;
	axlPointer pad;
	axlPointer pad2;
} VortexWin32Mutex;

#define __OS_THREAD_TYPE__ win32_thread_t
#define __OS_MUTEX_TYPE__  HANDLE
#define __OS_COND_TYPE__   win32_cond_t

typedef struct _win32_thread_t {
	HANDLE    handle;
	void*     data;
	unsigned  id;	
} win32_thread_t;

/** 
 * @internal pthread_cond_t definition, fully based on the work done
 * by Dr. Schmidt. Take a look into his article (it is an excelent article): 
 * 
 *  - http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 * 
 * Just a wonderful work. 
 *
 * Ok. The following is a custom implementation to solve windows API
 * flaw to support conditional variables for critical sections. The
 * solution provided its known to work under all windows platforms
 * starting from NT 4.0. 
 *
 * In the case you are experimenting problems for your particular
 * windows platform, please contact us through the mailing list.
 */
typedef struct _win32_cond_t {
	/* Number of waiting threads. */
	int waiters_count_;
	
	/* Serialize access to <waiters_count_>. */
	CRITICAL_SECTION waiters_count_lock_;

	/* Semaphore used to queue up threads waiting for the
	 * condition to become signaled. */
	HANDLE sema_;

	/* An auto-reset event used by the broadcast/signal thread to
	 * wait for all the waiting thread(s) to wake up and be
	 * released from the semaphore.  */
	HANDLE waiters_done_;

	/* Keeps track of whether we were broadcasting or signaling.
	 * This allows us to optimize the code if we're just
	 * signaling. */
	size_t was_broadcast_;
	
} win32_cond_t;

#elif defined(AXL_OS_UNIX)

#include <pthread.h>
#define __OS_THREAD_TYPE__ pthread_t
#define __OS_MUTEX_TYPE__  pthread_mutex_t
#define __OS_COND_TYPE__   pthread_cond_t

#endif

/** 
 * @brief Thread definition, which encapsulates the os thread API,
 * allowing to provide a unified type for all threading
 * interface. 
 */
typedef __OS_THREAD_TYPE__ VortexThread;

/** 
 * @brief Mutex definition that encapsulates the underlaying mutex
 * API.
 */
typedef __OS_MUTEX_TYPE__  VortexMutex;

/** 
 * @brief Conditional variable mutex, encapsulating the underlaying
 * operating system implementation for conditional variables inside
 * critical sections.
 */
typedef __OS_COND_TYPE__   VortexCond;

/** 
 * @brief Message queue implementation that allows to communicate
 * several threads in a safe manner. 
 */
typedef struct _VortexAsyncQueue VortexAsyncQueue;

/** 
 * @brief Handle definition for the family of function that is able to
 * accept the function \ref vortex_thread_create.
 *
 * The function receive a user defined pointer passed to the \ref
 * vortex_thread_create function, and returns an pointer reference
 * that must be used as integer value that could be retrieved if the
 * thread is joined.
 *
 * Keep in mind that there are differences between the windows and the
 * posix thread API, that are supported by this API, about the
 * returning value from the start function. 
 * 
 * While POSIX defines as returning value a pointer (which could be a
 * reference pointing to memory high above 32 bits under 64
 * architectures), the windows API defines an integer value, that
 * could be easily used to return pointers, but only safe on 32bits
 * machines.
 *
 * The moral of the story is that you must use another mechanism to
 * return data from this function to the thread that is expecting data
 * from this function. 
 * 
 * Obviously if you are going to return an status code, there is no
 * problem. This only applies to user defined data that is returned as
 * a reference to allocated data.
 */
typedef axlPointer (* VortexThreadFunc) (axlPointer user_data);

/** 
 * @brief Thread configuration its to modify default behaviour
 * provided by the thread creation API.
 */
typedef enum  {
	/** 
	 * @brief Marker used to signal \ref vortex_thread_create that
	 * the configuration list is finished.
	 * 
	 * The following is an example on how to create a new thread
	 * without providing any configuration, using defaults:
	 *
	 * \code
	 * VortexThread thread;
	 * if (! vortex_thread_created (&thread, 
	 *                              some_start_function, NULL,
	 *                              VORTEX_THREAD_CONF_END)) {
	 *      // failed to create the thread 
	 * }
	 * // thread created
	 * \endcode
	 */
	VORTEX_THREAD_CONF_END = 0,
	/** 
	 * @brief Allows to configure if the thread create can be
	 * joined and waited by other. 
	 *
	 * Default state for all thread created with \ref
	 * vortex_thread_create is true, that is, the thread created
	 * is joinable.
	 *
	 * If configured this value, you must provide as the following
	 * value either axl_true or axl_false.
	 *
	 * \code
	 * VortexThread thread;
	 * if (! vortex_thread_create (&thread, some_start_function, NULL, 
	 *                             VORTEX_THREAD_CONF_JOINABLE, axl_false,
	 *                             VORTEX_THREAD_CONF_END)) {
	 *    // failed to create the thread
	 * }
	 * 
	 * // Nice! thread created
	 * \endcode
	 */
	VORTEX_THREAD_CONF_JOINABLE  = 1,
	/** 
	 * @brief Allows to configure that the thread is in detached
	 * state, so no thread can join and wait for it for its
	 * termination but it will also provide.
	 */
	VORTEX_THREAD_CONF_DETACHED = 2,
}VortexThreadConf;

/**
 * @brief Wait Reply data used for \ref vortex_manual_wait_reply "Wait Reply Method".
 *
 * See \ref vortex_manual_wait_reply "this section to know more about Wait Reply Method.
 * 
 */
typedef struct _WaitReplyData WaitReplyData;

/**
 * @brief Enumeration type that allows to use the waiting mechanism to
 * be used by the core library to perform wait on changes on sockets
 * handled.
 */

typedef enum {
	/**
	 * @brief Allows to configure the select(2) system call based
	 * mechanism. It is known to be available on any platform,
	 * however it has some limitations while handling big set of
	 * sockets, and it is limited to a maximum number of sockets,
	 * which is configured at the compilation process.
	 *
         * Its main disadvantage it that it can't handle
	 * more connections than the number provided at the
	 * compilation process. See <vortex.h> file, variable
	 * FD_SETSIZE and VORTEX_FD_SETSIZE.
	 */
	VORTEX_IO_WAIT_SELECT = 1,
	/**
	 * @brief Allows to configure the poll(2) system call based
	 * mechanism. 
	 * 
	 * It is also a widely available mechanism on POSIX
	 * envirionments, but not on Microsoft Windows. It doesn't
	 * have some limitations found on select(2) call, but it is
	 * known to not scale very well handling big socket sets as
	 * happens with select(2) (\ref VORTEX_IO_WAIT_SELECT).
	 *
	 * This mechanism solves the runtime limitation that provides
	 * select(2), making it possible to handle any number of
	 * connections without providing any previous knowledge during
	 * the compilation process. 
	 * 
	 * Several third party tests shows it performs badly while
	 * handling many connections compared to (\ref VORTEX_IO_WAIT_EPOLL) epoll(2).
	 *
	 * However, reports showing that results, handles over 50.000
	 * connections at the same time (up to 400.000!). In many
	 * cases this is not going your production environment.
	 *
	 * At the same time, many reports (and our test results) shows
	 * that select(2), poll(2) and epoll(2) performs the same
	 * while handling up to 10.000 connections at the same time.
	 */
	VORTEX_IO_WAIT_POLL   = 2,
	/**
	 * @brief Allows to configure the epoll(2) system call based
	 * mechanism.
	 * 
	 * It is a mechanism available on GNU/Linux starting from
	 * kernel 2.6. It is supposed to be a better implementation
	 * than poll(2) and select(2) due the way notifications are
	 * done.
	 *
	 * It is currently selected by default if your kernel support
	 * it. It has the advantage that performs very well with
	 * little set of connections (0-10.000) like
	 * (\ref VORTEX_IO_WAIT_POLL) poll(2) and (\ref VORTEX_IO_WAIT_SELECT)
	 * select(2), but scaling much better when going to up heavy
	 * set of connections (50.000-400.000).
	 *
	 * It has also the advantage to not require defining a maximum
	 * socket number to be handled at the compilation process.
	 */
	VORTEX_IO_WAIT_EPOLL  = 3,
} VortexIoWaitingType;


/** 
 * @brief Enum definition to configure stages during the connection
 * creation. 
 */
typedef enum {
	/** 
	 * @brief Actions to be executed after the connection
	 * creation. Only connections properly created are
	 * notified. Handler associated to this stage is run just
	 * after the BEEP session is stablished. This handler is
	 * executed after handlers configured at \ref
	 * vortex_listener_set_on_connection_accepted. 
	 *
	 * Note this handler is called twice on TLS activation: one
	 * for the first connection and one for the connection
	 * creation after TLS activation. This is because both
	 * connections are diferent objects with different
	 * states. This also allows to setup or run different
	 * configurations for non TLS or and for TLS enabled clients.
	 */
	CONNECTION_STAGE_POST_CREATED = 1,

	/** 
	 * @brief Action to be executed on listener side to process
	 * greetings features received on the connection. This handler
	 * is executed after the connection was accepted but just
	 * before fully registered so the handler configured for this
	 * connection stage can shutdown the reference (in such case
	 * must returned -1) or return 0 in the case the connection
	 * must be finally accepted.
	 */
	CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES = 2 
} VortexConnectionStage;

/** 
 * @brief List of possible handlers that can be uninstalled from a
 * connection after being added.
 */
typedef enum {
	/** 
	 * @brief Enum type that represents handlers installed by \ref
	 * vortex_connection_set_channel_added_handler.
	 */
	CONNECTION_CHANNEL_ADD_HANDLER = 1,
	/** 
	 * @brief Enum type that represents handlers installed by \ref
	 * vortex_connection_set_channel_removed_handler.
	 */
	CONNECTION_CHANNEL_REMOVE_HANDLER = 2,
} VortexConnectionHandler;

/**
 * @brief Enumeration type used to represent the query operations
 * that must support a VortexPayloadFeeder implementation.
 */
typedef enum {
	/** 
	 * @brief Notifies the feed handler that it must return total
	 * amount of bytes pending to be send. 
	 *
	 * Currently PAYLOAD_FEEDER_GET_SIZE should return the entire
	 * message that was meant to be sent...however you can play
	 * with feeder by "increasing" dynamically this value (always
	 * increasing) if you want to send more content than the one
	 * initially expected.
	 *
	 * As conclusion, you can always return the entire message or
	 * a bigger value then the last reported.
	 */
	PAYLOAD_FEEDER_GET_SIZE = 1,
	/** 
	 * @brief Notifies the feed handler to return next payload
	 * content to be send. The handler receives in "data" the
	 * maximum amount of content required at this time.
	 */
	PAYLOAD_FEEDER_GET_CONTENT = 2,
	/** 
	 * @brief Notifies the feed handler to return if the feeder
	 * have sent all content it represents. The feed handler must
	 * return axl_true to notify finished or axl_false to notify
	 * there are pending content.
	 */
	PAYLOAD_FEEDER_IS_FINISHED = 3,
	/** 
	 * @brief Notifies the feed handler to release all resources
	 * acquired. When this signal is received, the feeder is about
	 * to be collected.
	 */
	PAYLOAD_FEEDER_RELEASE = 4
} VortexPayloadFeederOp;

/** 
 * @brief Definition for supported network transports.
 */ 
typedef enum { 
	/** 
	 * @brief IPv4 transport.
	 */
	VORTEX_IPv4 = 1,
	/** 
	 * @brief IPv6 transport.
	 */
	VORTEX_IPv6 = 2
} VortexNetTransport;

#endif

/* @} */

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
#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>
#include <vortex_connection_private.h>

#define LOG_DOMAIN "vortex-frame-factory"

/** 
 * @internal
 * @brief Internal Note
 *
 * While writing \r\n CR+LF you have to use \\x0D\\x0A
 *  \\n is portable mapped to \\x0A
 *  \\r is portable mapped to \\x0D
 * 
 * More information about writing portable CR LF bytes on
 * wikipedia. Fabulous article at:
 *
 *     http://en.wikipedia.org/wiki/CRLF
 */

/** 
 * @internal Node that allows to support several MIME headers defined
 * on the same value, for example, "Received".
 */
struct _VortexMimeHeader {
	/* @internal Pointer to the same header (a practical pointer
	 * to allow getting which header correspond to the content) */
	char             * name;

	/* @internal The MIME header content found for one declaration. */
	char             * content;

	/* @interanl Next pointer to the next content found. */
	VortexMimeHeader * next;

	/* @interanl Next pointer to the next content found. */
	VortexMimeHeader * next_header;
};

typedef struct _VortexMimeStatus {
	/* memory factory */
	axlFactory    * header_factory;
	axlStrFactory * mime_headers_name;
	axlStrFactory * mime_headers_content;
	
	/* common headers */
	VortexMimeHeader * content_type;
	VortexMimeHeader * content_transfer_encoding;
	VortexMimeHeader * mime_version;
	VortexMimeHeader * content_id;
	VortexMimeHeader * content_description;

	/* all mime headers */
	VortexMimeHeader * first;
	
	/* headers count */
	int                counting;
	
	/* ref counting */
	int                ref_count;
	VortexMutex        ref_mutex;
} VortexMimeStatus;

/** 
 * @internal Creates a new vortex mime status (a structure to hold all
 * information about MIME for the particular frame).
 */
VortexMimeStatus * vortex_frame_mime_status_new (void)
{
	VortexMimeStatus * status;

	/* create initial node */
	status                       = axl_new (VortexMimeStatus, 1);
	VORTEX_CHECK_REF (status, NULL);

	status->mime_headers_name    = axl_string_factory_create ();
	status->mime_headers_content = axl_string_factory_create ();
	status->header_factory       = axl_factory_create (sizeof (VortexMimeHeader));

	/* check memory allocation status */
	if (status->mime_headers_name    == NULL  ||
	    status->mime_headers_content == NULL ||
	    status->header_factory       == NULL) {
		axl_string_factory_free (status->mime_headers_name);
		axl_string_factory_free (status->mime_headers_content);
		axl_factory_free        (status->header_factory);
		axl_free (status);
		return NULL;
	} /* end if */

	/* init reference counting */
	vortex_mutex_create (&status->ref_mutex);
	status->ref_count = 1;

	return status;
}

/** 
 * @internal Function that allows to update current internal reference
 * counting. There is no unref because it is implicitly called at
 * vortex_frame_mime_status_free.
 */
void vortex_frame_mime_status_ref (VortexMimeStatus * status)
{
	if (status == NULL)
		return;
	vortex_mutex_lock (&status->ref_mutex);
	status->ref_count ++;
	vortex_mutex_unlock (&status->ref_mutex);
	return;
}

void vortex_frame_mime_status_free (VortexMimeStatus * status)
{
	if (status == NULL)
		return;

	/* check reference counting */
	vortex_mutex_lock (&status->ref_mutex);
	status->ref_count--;
	if (status->ref_count != 0) {
		/* this caller won't terminate this mime status */
		vortex_mutex_unlock (&status->ref_mutex);
		return;
	}
	vortex_mutex_unlock (&status->ref_mutex);

	/* free factories */
	axl_string_factory_free (status->mime_headers_name);
	axl_string_factory_free (status->mime_headers_content);
	axl_factory_free        (status->header_factory);
	vortex_mutex_destroy    (&status->ref_mutex);
	axl_free (status);

	return;
}

void vortex_frame_mime_check_and_update_fast_ref (VortexMimeStatus * status, VortexMimeHeader * header)
{
	/* find usual MIME headers */
	if (axl_casecmp (header->name, "content-type"))
		status->content_type              = header;
	else if (axl_casecmp (header->name, "content-transfer-encoding"))
		status->content_transfer_encoding = header;
	else if (axl_casecmp (header->name, "mime-version"))
		status->mime_version              = header;
	else if (axl_casecmp (header->name, "content-id"))
		status->content_id                = header;
	else if (axl_casecmp (header->name, "content-description"))
		status->content_description       = header;

	return;
}

VortexMimeHeader * vortex_frame_mime_find (VortexMimeStatus * status, const char * mime_header)
{
	VortexMimeHeader * header;

	/* find usual MIME headers */
	if (axl_casecmp (mime_header, "content-type"))
		return status->content_type;
	if (axl_casecmp (mime_header, "content-transfer-encoding"))
		return status->content_transfer_encoding;
	if (axl_casecmp (mime_header, "mime-version"))
		return status->mime_version;
	if (axl_casecmp (mime_header, "content-id"))
		return status->content_id;
	if (axl_casecmp (mime_header, "content-description"))
		return status->content_description;

	/* for each different header found do */
	header = status->first;
	while (header) {

		/* check the header name */
		if (axl_casecmp (mime_header, header->name)) 
			return header;

		/* next header name */
		header = header->next_header;
	} /* end while */
	
	/* no header found */
	return NULL;
}

struct _VortexFrame {
	/**
	 * Context where the frame was created.
	 */ 
	VortexCtx       * ctx;

	/** 
	 * Frame unique identifier. Every frame read have a different
	 * frame id. This is used to track down frames that are
	 * allocated and deallocated.
	 */
	int               id;
	
	/* frame header definition */
	VortexFrameType   type;
	int               channel;
	int               msgno;
	axl_bool          more;
	char              more_char;
	unsigned int      seqno;
	int               size;
	int               ansno;
	
	/* the size of the mime headers, including last
	 * \x0D\x0A\x0D\x0A */
	int                  mime_headers_size;
	
	/* the payload message, that is, all the message content
	 * (including MIME headers and body) */
	axlPointer           payload;

	/* real reference to the memory allocated, having all the
	 * frame received (including trailing END\x0A\x0D but
	 * nullified), this is used to avoid double allocating memory
	 * to receive the content and memory to place the content. See
	 * vortex_frame_get_next for more information. */
	axlPointer           buffer;

	/* reference to the payload content, from a MIME
	 * perspective. This is the body part of the frame received */
	axlPointer           content;

	/* a reference to the channel where the frame was received */
	VortexChannel      * channel_ref;
	
	/* frame reference counting */
	int                  ref_count;

	/* mime status for the current frame (a reference that could
	 * be shared with other frames, mostly due to
	 * vortex_frame_copy and vortex_frame_join* functions */
	VortexMimeStatus   * mime_headers;
};

/** 
 * @internal
 *
 * @brief Support function for frame identificators.  
 *
 * This is used to generate and return the next frame identifier, an
 * unique integer value to track frames created.
 *
 * @return Next frame identifier available.
 */
int  __vortex_frame_get_next_id (VortexCtx * ctx, char  * from)
{
	/* get current context */
	int         result;

	vortex_mutex_lock (&ctx->frame_id_mutex);
	
	result = ctx->frame_id;
	ctx->frame_id++;

	vortex_log (VORTEX_LEVEL_DEBUG, "Created frame id=%d", result);

	vortex_mutex_unlock (&ctx->frame_id_mutex);

	return result;
}

/**
 * \defgroup vortex_frame Vortex Frame Factory: Function to manipulate frames inside Vortex Library
 */

/**
 * \addtogroup vortex_frame
 * @{
 */


/**
 * @brief Returns the frame in plain text form from a VortexFrame object.
 * 
 * Returns actual frame message (the complete string) <i>frame</i> represents. The return value
 * must be freed when no longed is needed.
 *
 * @param frame the frame to get as raw text.
 * 
 * @return the raw frame or NULL if fails.
 **/
char  *       vortex_frame_get_raw_frame         (VortexFrame * frame)
{
	/* check reference received */
	if (frame == NULL)
		return NULL;

	return vortex_frame_build_up_from_params_s (frame->type,
						    frame->channel,
						    frame->msgno,
						    frame->more,
						    frame->seqno,
						    frame->size,
						    frame->ansno,
						    vortex_frame_get_content_type (frame),
						    vortex_frame_get_transfer_encoding (frame),
						    frame->payload,
						    NULL);
}

#define header_place(content) buffer[position] = content;	\
	position++;

int vortex_frame_build_header (char        * buffer,
			       int           buffer_size,
			       int         * real_size,
			       char        * message_type,
			       int           channel, 
			       int           msgno,
			       axl_bool      more,
			       unsigned int  seqno,
			       int           size,
			       int           ansno,
			       const char  * content_type,
			       const char  * transfer_encoding)
{
	int position = 0;
	int result;
	int length;

	/* copy message type header */
	while (position < 3) {
		buffer[position] = message_type[position];
		position++;
	} /* end while */

	/* place white space */
	header_place (' ');

	/* place channel */
	result = vortex_support_itoa (channel, buffer + position, 10);
	if (result == -1)
		return -1;
	position += result;

	/* place white space */
	header_place (' ');

	/* place msqno */
	result = vortex_support_itoa (msgno, buffer + position, 10);
	if (result == -1)
		return -1;
	position += result;

	/* place white space */
	header_place (' ');

	if (message_type[0] != 'S') {
		/* more indication */
		buffer[position] = (more ? '*' : '.');
		position++;
		
		/* place white space */
		header_place (' ');
	}

	/* place seqno */
	result = vortex_support_itoa (seqno, buffer + position, 10);
	if (result == -1)
		return -1;
	position += result;

	/* check if we have a SEQ frame here to end and return */
	if (message_type[0] == 'S') {
		/* place last \r\n */
		memcpy (buffer + position, "\x0D\x0A", 2);
		position += 2;
		
		*real_size = position;
		return position;
	}

	/* place white space */
	header_place (' ');

	/* place size */
	result = vortex_support_itoa (size, buffer + position, 10);
	if (result == -1)
		return -1;
	position += result;
		
	if (ansno >= 0) {
		/* place white space */
		header_place (' ');

		/* place ansno */
		result = vortex_support_itoa (ansno, buffer + position, 10);
		if (result == -1)
			return -1;
		position += result;
	} /* end if */

	/* place last \r\n */
	memcpy (buffer + position, "\x0D\x0A", 2);
	position += 2;

	/* configure content type if defined */
	if (content_type) {
		memcpy (buffer + position, "Content-Type: ", 14);
		position += 14;

		length = strlen (content_type);
		memcpy (buffer + position, content_type, length);
		position += length;

		memcpy (buffer + position, "\x0D\x0A", 2);
		position += 2;
	}

	/* configure transfer encoding if defined */
	if (transfer_encoding) {
		memcpy (buffer + position, "Content-Transfer-Encoding: ", 27);
		position += 27;

		length = strlen (transfer_encoding);
		memcpy (buffer + position, transfer_encoding, length);
		position += length;

		memcpy (buffer + position, "\x0D\x0A", 2);
		position += 2;
	}

	/* configure last \r\n */
	if (content_type || transfer_encoding) {
		memcpy (buffer + position, "\x0D\x0A", 2);
		position += 2;
	}

	/* return length */
	*real_size = position;
	return position;
}



/** 
 * @brief Allows to create a new SEQ frame.
 *
 * SEQ frame are especial to BEEP because they allows to control how
 * flow control is handled based on channel window size. 
 * 
 * @param channel_num The channel number where the channel will 
 *
 * @param ackno The sequence number expected to be received at the
 * time the SEQ frame was generated.
 *
 * @param window_size The window size that the SEQ frame will
 * acknowledge.
 * 
 * @return A newly allocated raw SEQ frame. Returned value should be
 * deallocated using axl_free.
 */
char  * vortex_frame_seq_build_up_from_params (int           channel_num,
					       unsigned int  ackno,
					       int           window_size)
{
	/* just build and return the string */
	return axl_stream_strdup_printf ("SEQ %d %u %d\x0D\x0A", channel_num, ackno, window_size);
}

/** 
 * @brief Allows to create a new SEQ frame placing the result into the
 * provided buffer.
 *
 * SEQ frame are especial to BEEP because they allows to control how
 * flow control is handled based on channel window size. 
 *
 * @param channel_num The channel number where the channel will 
 *
 * @param ackno The sequence number expected to be received at the
 * time the SEQ frame was generated.
 *
 * @param window_size The window size that the SEQ frame will
 * acknowledge.
 *
 * @param buffer The buffer that will hold the content.
 *
 * @param buffer_size The buffer size.
 *
 * @param result_size Reference to an integer variable where the size
 * of the result will be returned.
 * 
 * @return A newly allocated raw SEQ frame. Returned value should be
 * deallocated using axl_free.
 */
char  * vortex_frame_seq_build_up_from_params_buffer (int         channel_num,
						      int         ackno,
						      int         window_size, 
						      char      * buffer,
						      int         buffer_size,
						      int       * result_size)
{
	int    real_size = 0;

	/* just build and return the string */
	if (buffer != NULL && buffer_size > 0) {
		/* clear result size */
		if (result_size)
			(*result_size) = 0;

		vortex_frame_build_header (buffer, buffer_size, &real_size, 
					   "SEQ", channel_num, ackno, axl_false, window_size, 0, -1, NULL, NULL);
		buffer[real_size] = 0;
		
		/* axl_stream_printf_buffer (buffer, buffer_size, &real_size, 
		   "SEQ %d %u %d\x0D\x0A", channel_num, ackno, window_size);*/
		if (real_size > buffer_size) {
			return NULL;
		} /* end if */
		
		/* configure resulting size */
		if (result_size)
			(*result_size) = real_size;

		/* return the same reference */
		return buffer;
	} /* end if */

	/* use allocated function */
	return axl_stream_strdup_printf ("SEQ %d %u %d\x0D\x0A", channel_num, ackno, window_size);
}

/** 
 * @brief Builds a frame using the given parameters.
 *
 * This function should not be useful for Vortex Library consumer
 * because all frames received and sent are actually builded by Vortex
 * Library.
 *
 * If you need to get the size of the frame returned at the same time
 * it is built, you should call to \ref
 * vortex_frame_build_up_from_params_s. This function implements the
 * same operation like this function but also returning the frame
 * size. 
 * 
 * <b>NOTE: This function is considered to be deprecated because there
 * is no accurate way to get current size for the frame returned.</b>
 * 
 * Using binary protocols that includes 0x0 as a valid value for its
 * charset could make to produce buggy results from using functions
 * such strlen to calculate current frame size returned. This is
 * because this functions looks for the next 0x0 value found on the
 * string to delimit it, but, as stated before this value couldn't be
 * the last value. It is preferred to use \ref vortex_frame_build_up_from_params_s.
 *
 * @param type    Frame type.
 * @param channel The channel number for the frame.
 * @param msgno   The message number for the frame.
 * @param more    More flag status for the frame.
 * @param seqno   Sequence number for the frame
 * @param size    The frame payload size.
 * @param ansno   The answer number for the frame
 * @param payload The payload is going to have the frame.
 * 
 * @return A newly created frame that should be freed using axl_free
 */
char  * vortex_frame_build_up_from_params (VortexFrameType   type,
					   int               channel,
					   int               msgno,
					   axl_bool          more,
					   unsigned int      seqno,
					   int               size,
					   int               ansno,
					   const void *      payload)
{
	return vortex_frame_build_up_from_params_s (type, channel, msgno, more, seqno, size, ansno, NULL, NULL, payload, NULL);
}

/**
 * @internal Common macro that holds all variables to be placed to
 * create a BEEP header using the following printf format:
 *
 *   "%s %d %d %c %u %d %d\x0D\x0A%s%s%s%s%s%s%s"  -> for ans header
 *   "%s %d %d %c %u %d%s\x0D\x0A%s%s%s%s%s%s%s"   -> for the rest
 */
#define VORTEX_FRAME_COMMON_BEEP_HEADER(ansno_decl)                           \
    /* BEEP header values */                                                  \
    message_type,                                                             \
    channel,                                                                  \
    msgno,                                                                    \
    more ? '*' : '.',                                                         \
    seqno,                                                                    \
    /* payload size, do not use "size" because it specify                     \
       the payload size without considering content type                      \
       and content transfer encoding.                                         \
                                                                              \
       "header_size" is not the BEEP frame header size but                    \
       the "size" value that goes inside the BEEP                             \
       header. */                                                             \
    header_size,                                                              \
    /* place ansno decl if provided by the caller */                          \
    ansno_decl,                                                               \
    /* content type values */                                                 \
    place_content_type ? "Content-Type: "   : "",                             \
    place_content_type ? content_type       : "",                             \
    place_content_type ? "\x0D\x0A"         : "",                             \
    /* content transfer encoding */                                           \
    place_transfer_encoding ? "Content-Transfer-Encoding: "   : "",           \
    place_transfer_encoding ? transfer_encoding               : "",           \
    place_transfer_encoding ? "\x0D\x0A"                      : "",           \
    /* place entity header trailing header if some of them is defined */      \
    ( place_content_type || place_transfer_encoding) ? "\x0D\x0A" : ""

/** 
 * @brief Creates a new frame, using the given data and returning
 * current frame size resulting from the operation, and placing the
 * content into the buffer provided if defined.
 *
 * This function replaces \ref vortex_frame_build_up_from_params and
 * it is considered to be more accurate and secure.
 *
 * @param type              Frame type.
 *
 * @param channel           The channel number for the frame.
 *
 * @param msgno             The message number for the frame.
 *
 * @param more              More flag status for the frame.
 *
 * @param seqno             Sequence number for the frame
 *
 * @param size              The frame payload size.
 *
 * @param ansno             The answer number for the frame
 *
 * @param content_type Optional content type to be used for the frame
 * to be sent. If no value is provided no content type will be
 * placed. If content type provided is also the default value then no
 * content type will be placed.
 *
 * @param transfer_encoding Optional content transfer encoding. If no
 * value is provided no content transfer type will be placed. If
 * content transfer type provided is also the default value then no
 * content type will be placed.
 *
 * @param payload           The payload is going to have the frame.
 *
 * @param frame_size An optional pointer to an integer value which
 * holds current frame size.
 *
 * @param buffer The buffer where the content will be placed.
 *
 * @param buffer_size The size of the buffer.
 *
 * @return A newly created frame that should be freed using axl_free.
 */
char  * vortex_frame_build_up_from_params_s_buffer (VortexFrameType   type,
 						    int               channel,
 						    int               msgno,
 						    axl_bool          more,
 						    unsigned int      seqno,
 						    int               size,
 						    int               ansno,
 						    const char   *    content_type,
 						    const char   *    transfer_encoding,
 						    const void   *    payload,
 						    int    *          frame_size,
 						    char         *    buffer,
 						    int               buffer_size)
{
	char     * message_type = NULL;
	char     * value;
	int        header_size;
	int        header_length;
 	int        real_size    = 0;
	axl_bool   place_content_type;
	axl_bool   place_transfer_encoding;

	/*
	 * According to the frame type build the initial frame header
	 * using BEEP prefix.
	 */
	switch (type) {
	case VORTEX_FRAME_TYPE_UNKNOWN:
		return NULL;
	case VORTEX_FRAME_TYPE_MSG:
		message_type = "MSG";
		break;
	case VORTEX_FRAME_TYPE_RPY:
		message_type = "RPY";
		break;
	case VORTEX_FRAME_TYPE_ANS:
		message_type = "ANS";
		break;
	case VORTEX_FRAME_TYPE_ERR:
		message_type = "ERR";
		break;
	case VORTEX_FRAME_TYPE_NUL:
		message_type = "NUL";
		break;
	case VORTEX_FRAME_TYPE_SEQ:
		/* use seq frame building function */
		return vortex_frame_seq_build_up_from_params_buffer (channel, seqno, size, buffer, buffer_size, NULL);
	default:
		if (frame_size != NULL)
			(* frame_size ) = -1;
		return NULL;
	}

	/*
	 * check if we have to build content type and content transfer
	 * encoding headers ... */
	place_content_type      = (content_type      != NULL) && ! axl_stream_cmp (content_type, "application/octet-stream", 24);
	place_transfer_encoding = (transfer_encoding != NULL) && ! axl_stream_cmp (transfer_encoding, "binary", 6);

	/* payload size for the BEEP header */
	header_size = size;
	
	/* aggregate the content type, if defined, to the payload
	 * size */
	if (place_content_type)
		header_size += 16 + strlen (content_type);

	/* aggregate the content transfer type, if defined, to the
	 * payload size */
	if (place_transfer_encoding)
		header_size += 29 + strlen (transfer_encoding);

	/* aggregate two bytes more to the payload size of one of the
	 * previous entity headers were defined, representing \r\n */
	if (place_transfer_encoding || place_content_type)
		header_size += 2;

	/* build BEEP header */
	if (buffer && buffer_size > 0) {
		/* check if we are building a BEEP ANS header */
		if (type == VORTEX_FRAME_TYPE_ANS) {
			/* use header builder */
			header_length  = vortex_frame_build_header (
				buffer, buffer_size, 
				&real_size, 
				message_type, channel, msgno,
				more, seqno, header_size, ansno,
				content_type, 
				transfer_encoding);
		} else {
			/* use header builder */
			header_length  = vortex_frame_build_header (
				buffer, buffer_size, &real_size, 
				message_type, channel, msgno,
				more, seqno, header_size, -1,
				content_type, 
				transfer_encoding);
		} /* end if */

		/* check return status */
		if (real_size >= buffer_size) {
			if (frame_size != NULL)
				(* frame_size ) = -1;
			return NULL;
		} /* end if */

		/* configue value to point to the buffer */
		value = buffer;

	} else {
		if (type == VORTEX_FRAME_TYPE_ANS) {
			value  = axl_stream_strdup_printf_len (
				/* provide the printf message format */
				"%s %d %d %c %u %d %d\x0D\x0A%s%s%s%s%s%s%s",
				/* provide a reference to get the header length */
				&header_length,
				VORTEX_FRAME_COMMON_BEEP_HEADER(ansno));
		} else {
			value  = axl_stream_strdup_printf_len (
				/* provide the printf message format */
				"%s %d %d %c %u %d%s\x0D\x0A%s%s%s%s%s%s%s",
				/* provide a reference to get the header length */
				&header_length,
				VORTEX_FRAME_COMMON_BEEP_HEADER(""));
		} /* end if */
	} /* end if */

	/*
	 * Now build the frame according to previous BEEP header and
	 * payload size received.  Next memory allocation stands from
	 * counting header size plus payload size and then 6
	 * additional bytes: END\\x0D\\x0A which are 5 bytes ended by a
	 * 0x0 value meaning 1 byte more.
	 */

	if (buffer == NULL && buffer_size == 0) {
		/*
		 * Allocate a memory chunk to hold frame values, including
		 * header length and payload content. On this case we use
		 * "size" rather than "header_size". This is because
		 * header_length includes all size for the BEEP header
		 * (including Content-Type and Content-Transfer-Encoding
		 * entity headers) and "size" is the payload size. 
		 */
		value = axl_realloc (value, header_length + size + 6);
		VORTEX_CHECK_REF (value, NULL);
	} /* end if */
	
	/* copy BEEP frame payload */
	memcpy (value + header_length, payload, size);

	/* copy BEEP frame trailing */
	memcpy (value + header_length + size, "END\x0D\x0A", 5);

	/* nullify the last char */
	value [header_length + size + 5] = 0;

	/*
	 * return calculated frame size, NOTE: that the returned size
	 * for constant value is 7 rather than 8 as stated on previous
	 * statement allocating memory, this is because we don't want
	 * to write to the socket a 0x0 value. 
	 */
	if (frame_size != NULL)
		(* frame_size) = header_length + 5 + size;

	return value;	
}

/** 
 * @brief Creates a new frame from using the given data and returning
 * current frame size resulting from the operation.
 *
 * This function replaces \ref vortex_frame_build_up_from_params and
 * it is considered to be more accurate and secure.
 *
 * @param type              Frame type.
 *
 * @param channel           The channel number for the frame.
 *
 * @param msgno             The message number for the frame.
 *
 * @param more              More flag status for the frame.
 *
 * @param seqno             Sequence number for the frame
 *
 * @param size              The frame payload size.
 *
 * @param ansno             The answer number for the frame
 *
 * @param content_type Optional content type to be used for the
 * channel being sent. If no value is provided no content type will be
 * placed. If content type provided is also the default value then no
 * content type will be placed.
 *
 * @param transfer_encoding Optional content transfer encoding. If no
 * value is provided no content transfer type will be placed. If
 * content transfer type provided is also the default value then no
 * content type will be placed.
 *
 * @param payload           The payload is going to have the frame.
 *
 * @param frame_size An optional pointer to an integer value which
 * holds current frame size.
 *
 * @return A newly created frame that should be freed using axl_free.
 */
char  * vortex_frame_build_up_from_params_s (VortexFrameType   type,
					     int               channel,
					     int               msgno,
					     axl_bool          more,
					     unsigned int      seqno,
					     int               size,
					     int               ansno,
					     const char   *    content_type,
					     const char   *    transfer_encoding,
					     const void   *    payload,
					     int    *          frame_size)
{
	/* call to common implementation without using a buffer */
	return vortex_frame_build_up_from_params_s_buffer (type,
							   channel, 
							   msgno,
							   more, 
							   seqno,
							   size, 
							   ansno,
							   content_type,
							   transfer_encoding,
							   payload,
							   frame_size,
							   NULL, 0);
}

/** 
 * @brief Creates a new frame object (\ref VortexFrame) using the given data.
 *
 * Also check \ref vortex_frame_create_full for a full support on
 * creating new frames, also specifying content type and content
 * transfer encoding values.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param type The frame type to build.
 *
 * @param channel The channel number the frame have.
 *
 * @param msgno The message number the frame have.
 *
 * @param more More flag configuration for the frame.
 *
 * @param seqno The sequence number for the frame to build.
 *
 * @param size The frame payload size.
 *
 * @param ansno The answer number the frame have.
 *
 * @param payload The payload the frame hold.
 * 
 * @return A newly created \ref VortexFrame object that must be
 * unrefered using \ref vortex_frame_free when no longer needed.
 */
VortexFrame * vortex_frame_create               (VortexCtx       * ctx,
						 VortexFrameType   type,
						 int               channel,
						 int               msgno,
						 axl_bool          more,
						 unsigned int      seqno,
						 int               size,
						 int               ansno,
						 const void      * payload)
{
	/* create a new frame using new_full interface and return
	 * it. */
	return vortex_frame_create_full (ctx, type, channel, msgno, more, seqno, size, ansno, 
					 NULL,     /* content type value */
					 NULL,     /* content transfer encoding value */
					 payload); /* frame payload */
}


/** 
 * @brief Creates a new frame as \ref vortex_frame_create but also
 * allowing to specify mime header content, that is, Content-Type and
 * Content-Transfer-Encoding values.
 *
 * This function works like \ref vortex_frame_create function but
 * allowing to specify every parameter used (possible) for a frame.
 * 
 * The function also performs a local copy for the content type ("Content-Type:" ) value  and
 * the transfer encoding ("Content-Transfer-Encoding:" )  value provided.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param type The frame type to build.
 *
 * @param channel The channel number the frame have.
 *
 * @param msgno The message number the frame have.
 *
 * @param more More flag configuration for the frame.
 *
 * @param seqno The sequence number for the frame to build.
 *
 * @param size The frame payload size.
 *
 * @param ansno The answer number the frame have.
 *
 * @param content_type The MIME Content-Type value to be used for this
 * frame. Optional value.
 *
 * @param transfer_encoding The MIME Content-Transfer-Encoding value
 * to be used for this frame. Optional value.
 *
 * @param payload The payload the frame hold.
 * 
 * @return A newly created \ref VortexFrame object that must be
 * unrefered using \ref vortex_frame_free when no longer
 * needed. Function may return NULL reference if allow fails or ctx
 * received is NULL.
 */
VortexFrame * vortex_frame_create_full          (VortexCtx       * ctx,
						 VortexFrameType   type,
						 int               channel,
						 int               msgno,
						 axl_bool          more,
						 unsigned int      seqno,
						 int               size,
						 int               ansno,
						 const char      * content_type,
						 const char      * transfer_encoding,
						 const void      * payload)
{
	VortexFrame * result;

	/* check context received */
	if (ctx == NULL)
		return NULL;

	/* build base object */
	result = axl_new (VortexFrame, 1);
	VORTEX_CHECK_REF (result, NULL);

	/* acquire a reference to the context */
	vortex_ctx_ref2 (ctx, "new frame");
	
	result->id           = __vortex_frame_get_next_id (ctx, "create-full");
	result->ctx          = ctx;
	result->ref_count    = 1;
	result->type         = type;
	result->channel      = channel;
	result->msgno        = msgno;
	result->seqno        = seqno;
	result->size         = size;
	result->ansno        = ansno;

	/* copy the payload */
	if (size > 0 && payload != NULL) {
		result->payload           = axl_new (char , size + 1);
		VORTEX_CHECK_REF2 (result->payload, NULL, result, axl_free);
		memcpy (result->payload, payload, size);
	}

	/* copy content type */
	if (content_type != NULL) {
		/* set content type header */
		vortex_frame_set_mime_header (result, MIME_CONTENT_TYPE, content_type);
	}

	/* copy content transfer encoding */
	if (transfer_encoding != NULL) {
		/* set content type header */
		vortex_frame_set_mime_header (result, MIME_CONTENT_TRANSFER_ENCODING, transfer_encoding);
	}
		
	return result;	
}


/** 
 * @brief Creates a new frame as \ref vortex_frame_create but also
 * allowing to specify mime header content, that is, Content-Type and
 * Content-Transfer-Encoding values. The frame created will "own" the
 * reference to the payload provided, without allocating memory for a
 * new one. This function is useful to reuse the memory associated to
 * the payload provided.
 *
 * This function works like \ref vortex_frame_create function but
 * allowing to specify every parameter used (possible) for a frame.
 * 
 * The function also performs a local copy for the content type ("Content-Type:" ) value  and
 * the transfer encoding ("Content-Transfer-Encoding:" )  value provided.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param type The frame type to build.
 *
 * @param channel The channel number the frame have.
 *
 * @param msgno The message number the frame have.
 *
 * @param more More flag configuration for the frame.
 *
 * @param seqno The sequence number for the frame to build.
 *
 * @param size The frame payload size.
 *
 * @param ansno The answer number the frame have.
 *
 * @param content_type The MIME Content-Type value to be used for this
 * frame. Optional value.
 *
 * @param transfer_encoding The MIME Content-Transfer-Encoding value
 * to be used for this frame. Optional value.
 *
 * @param payload The payload the frame hold.
 * 
 * @return A newly created \ref VortexFrame object that must be
 * unrefered using \ref vortex_frame_free when no longer needed.
 */
VortexFrame * vortex_frame_create_full_ref      (VortexCtx       * ctx,
						 VortexFrameType   type,
						 int               channel,
						 int               msgno,
						 axl_bool          more,
						 unsigned int      seqno,
						 int               size,
						 int               ansno,
						 const char      * content_type,
						 const char      * transfer_encoding,
						 void            * payload)
{
	VortexFrame * result;

	/* check context received */
	if (ctx == NULL)
		return NULL;

	/* create base object */
	result = axl_new (VortexFrame, 1);
	VORTEX_CHECK_REF (result, NULL);

	/* acquire a reference to the context */
	vortex_ctx_ref2 (ctx, "new frame");
	
	result->id           = __vortex_frame_get_next_id (ctx, "create-full");
	result->ctx          = ctx;
	result->ref_count    = 1;
	result->type         = type;
	result->channel      = channel;
	result->msgno        = msgno;
	result->seqno        = seqno;
	result->size         = size;
	result->ansno        = ansno;

	/* copy the payload */
	if (size > 0 && payload != NULL) {
		result->payload           = (axlPointer) payload;
	}

	/* copy content type */
	if (content_type != NULL) {
		/* set content type header */
		vortex_frame_set_mime_header (result, MIME_CONTENT_TYPE, content_type);
	}

	/* copy content transfer encoding */
	if (transfer_encoding != NULL) {
		/* set content type header */
		vortex_frame_set_mime_header (result, MIME_CONTENT_TRANSFER_ENCODING, transfer_encoding);
	}
	
	return result;	
}



/** 
 * @brief Perform a copy from the received frame. 
 *
 * The function will also copy the channel where the frame was
 * received.
 * 
 * @param frame A frame to copy.
 * 
 * @return A newly allocated frame that must be unrefered when no
 * longer needed using \ref vortex_frame_free. NULL will be returned
 * if the frame received is NULL.
 */
VortexFrame * vortex_frame_copy                 (VortexFrame      * frame)
{
	VortexFrame * result; 
 	int           content_size;

	if (frame == NULL)
		return NULL;

 	/* create the frame, but check first if the frame have MIME
 	 * parsing activated. If "content" is defined, this means that
 	 * internal references were configured  */
 	if (frame->content) {
 		/* all frame content (including MIME headers) */
 		content_size = frame->size + frame->mime_headers_size;
 
 		/* create the frame copy using all the content. The
 		 * following function copy all the content but leaves
 		 * the result in the reference payload. The following
 		 * lines updates internal references to keep correct
 		 * consistency: "content" points to all the content
 		 * and "payload" points to the MIME message body.  */
 		result = vortex_frame_create (frame->ctx, 
					      frame->type, frame->channel, frame->msgno,
 					      frame->more, frame->seqno, content_size,
 					      frame->ansno, frame->content);
 
 		/* reconfigure payload pointer */
 		result->content      = result->payload;
 		result->payload      = ((char*)result->content) + (content_size - frame->size);
 
 		/* update content sizes */
 		result->mime_headers_size = frame->mime_headers_size;
 		result->size              = frame->size;
 	} else {
 		/* the frame is porting all the content inside
 		 * "payload". */
 		result = vortex_frame_create (frame->ctx,
					      frame->type, frame->channel, frame->msgno,
 					      frame->more, frame->seqno, frame->size, 
 					      frame->ansno, frame->payload);
 	} /* end if */
 
  	/* set same channel */
  	result->channel_ref = frame->channel_ref;
  
 	/* copy mime headers if found to be defined */
 	if (frame->mime_headers) {
 		/* increase reference counting on the MIME header
 		 * description */
 		vortex_frame_mime_status_ref (frame->mime_headers);
 
 		/* configure the reference */
 		result->mime_headers = frame->mime_headers;
 	} /* end if */
 
	/* set same channel */
	result->channel_ref = frame->channel_ref;

	/* return the frame created */
	return result;
}

/** 
 * @internal
 * @brief reads n bytes from the connection.
 * 
 * @param connection the connection to read data.
 * @param buffer buffer to hold data.
 * @param maxlen 
 * 
 * @return how many bytes were read.
 */
int         vortex_frame_receive_raw  (VortexConnection * connection, char  * buffer, int  maxlen)
{
	int         nread;
#if defined(ENABLE_VORTEX_LOG)
	char      * error_msg;
#endif
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx * ctx = vortex_connection_get_ctx (connection);
#endif

	/* avoid calling to read when no good socket is defined */
	if (connection->session == -1)
		return -1;

 __vortex_frame_readn_keep_reading:
	/* clear buffer */
	/* memset (buffer, 0, maxlen * sizeof (char )); */
	if ((nread = vortex_connection_invoke_receive (connection, buffer, maxlen)) == VORTEX_SOCKET_ERROR) {
		if (errno == VORTEX_EAGAIN) {
			return 0;
		}
		if (errno == VORTEX_EWOULDBLOCK) {
			return 0;
		}
		if (errno == VORTEX_EINTR)
			goto __vortex_frame_readn_keep_reading;
		
#if defined(ENABLE_VORTEX_LOG)
		if (errno != 0) {
			error_msg = vortex_errno_get_last_error ();
			vortex_log (VORTEX_LEVEL_CRITICAL, "unable to readn=%d, error was: '%s', errno=%d (conn-id=%d, session=%d)",
				    maxlen, error_msg ? error_msg : "", errno, connection->id, connection->session);
		} /* end if */
#endif
	}

	if (nread > 0) {
		/* notify here frame received (content received) */
		vortex_connection_set_receive_stamp (connection, (long) nread, 0);
	}

	/* ensure we don't access outside the array */
	if (nread < 0) 
		nread = 0;

	buffer[nread] = 0;
	return nread;
}

/**
 * @brief Read the next line, byte by byte until it gets a \n or
 * maxlen is reached. Some code errors are used to manage exceptions
 * (see return values)
 * 
 * @param connection The connection where the read operation will be done.
 *
 * @param buffer A buffer to store content read from the network.
 *
 * @param maxlen max content to read from the network.
 * 
 * @return  values returned by this function follows:
 *  0 - remote peer have closed the connection
 * -1 - an error have happened while reading
 * -2 - could read because this connection is on non-blocking mode and there is no data.
 *  n - some data was read.
 * 
 **/
int          vortex_frame_readline (VortexConnection * connection, char  * buffer, int  maxlen)
{
	int         n, rc;
	int         desp;
	char        c, *ptr;
#if defined(ENABLE_VORTEX_LOG)
	char      * error_msg;
#endif
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx * ctx = vortex_connection_get_ctx (connection);
#endif

	/* avoid calling to read when no good socket is defined */
	if (connection->session == -1)
		return -1;

	/* clear the buffer received */
	/* memset (buffer, 0, maxlen * sizeof (char ));  */

	/* check for pending line read */
	desp         = 0;
	if (connection->pending_line) {
		/* get size and check exceeded values */
		desp = strlen (connection->pending_line);
		if (desp >= maxlen) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError,
				"found fragmented frame line header but allowed size was exceeded (desp:%d >= maxlen:%d)",
				desp, maxlen);
			return -1;
		} /* end if */

		/* now store content into the buffer */
		memcpy (buffer, connection->pending_line, desp);

		/* clear from the connection the line */
		axl_free (connection->pending_line);
		connection->pending_line = NULL;
	}


	/* read current next line */
	ptr = (buffer + desp);
	for (n = 1; n < (maxlen - desp); n++) {
	__vortex_frame_readline_again:
		if (( rc = vortex_connection_invoke_receive (connection, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\x0A')
				break;
		}else if (rc == 0) {
			if (n == 1)
				return 0;
			else
				break;
		} else {
			if (errno == VORTEX_EINTR) 
				goto __vortex_frame_readline_again;
			if ((errno == VORTEX_EWOULDBLOCK) || (errno == VORTEX_EAGAIN) || (rc == -2)) {
				if (n > 0) {
					/* store content read until now */
					if ((n + desp - 1) > 0) {
						buffer[n+desp - 1] = 0;
						/* vortex_log (VORTEX_LEVEL_WARNING, "storing partially line read: '%s' n:%d, desp:%d", buffer, n, desp);*/
						connection->pending_line = axl_strdup (buffer);
					} /* end if */
				} /* end if */
				return (-2);
			}
			
#if defined(ENABLE_VORTEX_LOG)
			/* if the connection is closed, just return
			 * without logging a message */
			if (vortex_connection_is_ok (connection, axl_false)) {
				error_msg = vortex_errno_get_last_error ();
				vortex_log (VORTEX_LEVEL_CRITICAL, "unable to read a line from conn-id=%d (socket %d, rc %d), error was: %s, remote IP %s",
					    vortex_connection_get_id (connection), vortex_connection_get_socket (connection), rc,
					    error_msg ? error_msg : "", vortex_connection_get_host_ip (connection));
			}
#endif
			return (-1);
		}
	}

	/* notify here frame received (content received) */
	vortex_connection_set_receive_stamp (connection, (long) (n + desp), 0);

	*ptr = 0;
	return (n + desp);

}

int  get_int_value (VortexCtx * ctx, VortexConnection * conn, char  * string, int  * position)
{
	int iterator = 0;
	int result;
	while ((string[iterator] != ' ')    &&
	       (string[iterator] != '\x0A') && 
	       (string[iterator] != '\x0D')) {

		/* check digit values */
		if (string[iterator] != '0' &&
		    string[iterator] != '1' &&
		    string[iterator] != '2' &&
		    string[iterator] != '3' &&
		    string[iterator] != '4' &&
		    string[iterator] != '5' &&
		    string[iterator] != '6' &&
		    string[iterator] != '7' &&
		    string[iterator] != '8' &&
		    string[iterator] != '9') {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Unallowed sequence in header, shutting down connection-id=%d from %s:%s",
				    vortex_connection_get_id (conn), vortex_connection_get_host (conn), vortex_connection_get_port (conn));
			vortex_connection_shutdown (conn);
			return -1;
		}
			
		iterator++;
	}
	/* set the new terminator */
	string[iterator] = 0;

	/* translate value */
	result   = strtol (string, NULL, 10);
	/* vortex_log (VORTEX_LEVEL_DEBUG, "Translated %s -> %d", string, result);*/

	/* return the iterator position */
	*position = (*position) + iterator + 1;
	
	return result;
}

unsigned int  get_unsigned_int_value (VortexCtx * ctx, VortexConnection * conn, char  * string, int  * position)
{
	int          iterator = 0;
	unsigned int result;
	
	while ((string[iterator] != ' ')    &&
	       (string[iterator] != '\x0A') && 
	       (string[iterator] != '\x0D')) {

		/* check digit values */
		if (string[iterator] != '0' &&
		    string[iterator] != '1' &&
		    string[iterator] != '2' &&
		    string[iterator] != '3' &&
		    string[iterator] != '4' &&
		    string[iterator] != '5' &&
		    string[iterator] != '6' &&
		    string[iterator] != '7' &&
		    string[iterator] != '8' &&
		    string[iterator] != '9') {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Unallowed sequence in header, shutting down connection-id=%d from %s:%s",
				    vortex_connection_get_id (conn), vortex_connection_get_host (conn), vortex_connection_get_port (conn));
			vortex_connection_shutdown (conn);
			return -1;
		}

		iterator++;
	}
	/* set the new terminator */
	string[iterator] = 0;

	/* use atoi because we don't want to detect errors at this
	 * place */
	result   = strtoul (string, NULL, 10);

	/* return the iterator position */
	*position = (*position) + iterator + 1;
	
	return result;
}

/** 
 * @internal check that last value returned is not -2 and position
 * never overflows beep header length received.
 */
#define CHECK_INDEX_AND_RETURN(value) do{                       \
   if (value == -1)                                             \
	   return -1;                                           \
   if (value == -2 || (position > (header_length -1))) {	\
	   return -2;                                           \
   }                                                            \
} while(0)

/** 
 * @internal
 *
 * Internal function which reads the frame hacer received, and place
 * that data into the the frame reference received.
 *
 * @param connection The connection where the BEEP header is being
 * read.
 * 
 * @param beep_header An string representing the beep frame header.
 *
 * @param frame The VortexFrame header where the data will be placed.
 * 
 * @return Returns the number of bytes read from the beep header line.
 */
int  vortex_frame_get_header_data (VortexCtx * ctx, VortexConnection * connection, char  * beep_header, int header_length, VortexFrame * frame)
{
	int  position      = 0;

	/* check for null frame reader */
	if (beep_header == NULL)
		return 0;
	
	/* check for empty header */
/*	header_length = strlen (beep_header); */
	if (header_length == 0)
		return -2;
	vortex_log (VORTEX_LEVEL_DEBUG, "processing beep header: '%s' (length: %d)..",
		    beep_header, header_length);

	/* according to the frame type, read the content */
	switch (frame->type) {
	case VORTEX_FRAME_TYPE_SEQ:
		/* SEQ frame format: "%d %d %d\x0D\x0A"
		   channel seqno size */
		frame->channel    = get_int_value          (ctx, connection, beep_header, &position);
		CHECK_INDEX_AND_RETURN (frame->channel);

		frame->seqno      = get_unsigned_int_value (ctx, connection, beep_header + position, &position);
		CHECK_INDEX_AND_RETURN (frame->seqno);

		frame->size       = get_int_value          (ctx, connection, beep_header + position, &position);
		CHECK_INDEX_AND_RETURN (frame->size);
		break;
	default:
		/* where all cases matches: "%d %d %c %d %d\x0D\x0A"
		   channel msgno more_char seqno size */
		frame->channel    = get_int_value (ctx, connection, beep_header, &position);
		CHECK_INDEX_AND_RETURN (frame->channel);

		frame->msgno      = get_int_value (ctx, connection, beep_header + position, &position);
		CHECK_INDEX_AND_RETURN (frame->msgno);

		/* get more char */
		frame->more_char  = beep_header [position];
		position         += 2; /* one space and point to the
					* next item */
		frame->seqno      = get_unsigned_int_value (ctx, connection, beep_header + position, &position);
		frame->size       = get_int_value          (ctx, connection, beep_header + position, &position);
		break;

	}
	
	/* check ANSNO case */
	if (frame->type == VORTEX_FRAME_TYPE_ANS) {
		/* ANS frame format: "%d %d %c %u %d %d\x0D\x0A"
		   channel msgno more_char seqno size ansno: read one
		   more int */
		frame->ansno      = get_int_value (ctx, connection, beep_header + position, &position);
	}

	/* return current position plus one unit (\x0D and \x0A) */
	return position + 1;
	
}

/** 
 * @internal
 * 
 * Tries to get the next incomming frame inside the given connection. If
 * received frame is ok, it return a newly-allocated VortexFrame with
 * its data. This function is only useful for vortex library
 * internals. Vortex library consumer should not use it.
 *
 * This function also close you connection if some error happens.
 * 
 * @param connection the connection where frame is going to be
 * received
 * 
 * @return a frame  or NULL if frame is wrong.
 **/
VortexFrame * vortex_frame_get_next     (VortexConnection * connection)
{
	int           bytes_read;
	int           remaining;
	VortexFrame * frame;
	char          line[100];
	char        * buffer = NULL;
	VortexCtx   * ctx    = vortex_connection_get_ctx (connection);

	/* check here port sharing for this connection before reading
	 * the content. The function returns axl_true if the
	 * connection is ready to be used to read content */
	if (! __vortex_listener_check_port_sharing (ctx, connection)) 
		return NULL;

	/* before reading anything else, we have to check if previous
	 * read was complete if not, we are in a frame fragment case */
	buffer = connection->buffer;
	
	if (buffer) {
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "received more data after a frame fragment, previous read isn't still complete");
		/* get previous frame */
		frame        = connection->last_frame;
		v_return_val_if_fail (frame, NULL);

		/* get previous remaining */
		remaining    = connection->remaining_bytes;
		vortex_log (VORTEX_LEVEL_DEBUG, "remaining bytes to be read: %d", remaining);
		v_return_val_if_fail (remaining > 0, NULL);

		/* get previous bytes read */
		bytes_read   = connection->bytes_read;
		vortex_log (VORTEX_LEVEL_DEBUG, "bytes already read: %d", bytes_read);

		bytes_read = vortex_frame_receive_raw (connection, buffer + bytes_read, remaining);
		if (bytes_read == 0) {
			vortex_frame_free (frame);
			axl_free (buffer);

			connection->buffer     = NULL;
			connection->last_frame = NULL;

			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError, 
				"remote peer have closed connection while reading the rest of the frame having received part of it");
			return NULL;
		}


		vortex_log (VORTEX_LEVEL_DEBUG, "bytes ready this time: %d", bytes_read);

		/* check data received */
		if (bytes_read != remaining) {
			/* add bytes read to keep on reading */
			bytes_read += connection->bytes_read;
			vortex_log (VORTEX_LEVEL_DEBUG, "the frame fragment isn't still complete, total read: %d", bytes_read);
			goto save_buffer;
		}
		
		/* We have a complete buffer for the frame, let's
		 * continue the process but, before doing that we have
		 * to restore expected state of bytes_read. */
		bytes_read = (frame->size + 5);

		connection->buffer     = NULL;
		connection->last_frame = NULL;

		vortex_log (VORTEX_LEVEL_DEBUG, "this already complete (total size: %d", frame->size);
		goto process_buffer;
	}
	
	/* parse frame header, read the first line */
	bytes_read = vortex_frame_readline (connection, line, 99);
	if (bytes_read == -2) {
		/* count number of non-blocking operations on this
		 * connection to avoid iterating for ever */
		connection->no_data_opers++;
		if (connection->no_data_opers > 25) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexError, "too much no data available operations over this connection");
			return NULL;
		} /* end if */

                vortex_log (VORTEX_LEVEL_WARNING,
			    "no data was waiting on this non-blocking connection id=%d (EWOULDBLOCK|EAGAIN errno=%d)",
			    vortex_connection_get_id (connection), errno);
		return NULL;
	} /* end if */
	/* reset no data opers */
	connection->no_data_opers = 0;

	if (bytes_read == 0) {
		/* check if channel is expected to be closed */
		if (vortex_connection_get_data (connection, "being_closed")) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexOk, 
				"connection properly closed");
			return NULL;
		}

		/* check for connection into initial connect state */
		if (connection->initial_accept) {
			/* found a connection broken in the middle of
			 * the negotiation (just before the initial
			 * step, but after the second step) */
			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError, "found connection closed before finishing negotiation, dropping..");
			return NULL;
		}
	
		/* check if we have a non-blocking connection */
		__vortex_connection_shutdown_and_record_error (
			connection, VortexUnnotifiedConnectionClose,
			"remote side has disconnected without closing properly this session id=%d",
			vortex_connection_get_id (connection));
		return NULL;
	}
	if (bytes_read == -1) {
	        if (vortex_connection_is_ok (connection, axl_false))
		        __vortex_connection_shutdown_and_record_error (connection, VortexProtocolError, "an error have ocurred while reading socket");
		return NULL;
	}

	if (bytes_read == 1 || (line[bytes_read - 1] != '\x0A') || (line[bytes_read - 2] != '\x0D')) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "no line definition found for frame, over connection id=%d, bytes read: %d, line: '%s' errno=%d, closing session",
			    vortex_connection_get_id (connection),
			    bytes_read, line, errno);
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "no line definition found for frame");
		return NULL;
	}

	/* create a frame */
	frame       = axl_new (VortexFrame, 1);
	if (frame == NULL) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexMemoryFail, "Failed to allocate memory for frame");
		return NULL;
	} /* end if */

	/* acquire a reference to the context */
	vortex_ctx_ref2 (ctx, "new frame");

	/* set initial ref count */
	frame->ref_count = 1;

	/* associate the next frame id available */
	frame-> id  = __vortex_frame_get_next_id (ctx, "get-next");
	frame->ctx  = ctx;

	/* check initial frame spec */
	frame->type = VORTEX_FRAME_TYPE_UNKNOWN;
	if (axl_stream_cmp (line, "MSG", 3))
		frame->type = VORTEX_FRAME_TYPE_MSG;
	else if (axl_stream_cmp (line, "RPY", 3))
		frame->type = VORTEX_FRAME_TYPE_RPY;
	else if (axl_stream_cmp (line, "ANS", 3))
		frame->type = VORTEX_FRAME_TYPE_ANS;
	else if (axl_stream_cmp (line, "ERR", 3))
		frame->type = VORTEX_FRAME_TYPE_ERR;
	else if (axl_stream_cmp (line, "NUL", 3))
		frame->type = VORTEX_FRAME_TYPE_NUL;
	else if (axl_stream_cmp (line, "SEQ", 3))
		frame->type = VORTEX_FRAME_TYPE_SEQ;

	if (frame->type == VORTEX_FRAME_TYPE_UNKNOWN) {
		/* unref frame value */
		vortex_frame_free (frame);
		vortex_log (VORTEX_LEVEL_CRITICAL, "poorly-formed frame: message type not defined, line=%s",
			    line);
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "poorly-formed frame: message type not defined");
		return NULL;
	}

	/* get the next frame according to the expected values */
	bytes_read = vortex_frame_get_header_data (ctx, connection, &(line[4]), bytes_read - 4, frame);
	if (bytes_read == -2) {
		/* free frame no longer needed */
		vortex_frame_free (frame);

		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "wrong BEEP header found, closing connection..");
		return NULL;
	}

	/* configure the channel where the frame was received */
	frame->channel_ref = vortex_connection_get_channel (connection, frame->channel);
	if (frame->channel_ref == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "received a frame header pointing to a channel=%d that do not exists, closing connection",
			    frame->channel);
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "received a frame header pointing to a channel that do not exists, closing connection");
		vortex_frame_free (frame);
		return NULL;
	}
	
	/* in the case it is a SEQ frame */
	if (frame->type == VORTEX_FRAME_TYPE_SEQ) 
		return frame;

	/* check bytes read */
	if (bytes_read < 5) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError,
			"poorly-formed frame: message values are wrong  (%d < 5)", bytes_read);

		/* unref frame node allocated */
		vortex_frame_free (frame);
		return NULL;
	}

	/* check more flag */
	if (frame->more_char != '.' && frame->more_char != '*') {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "poorly-formed frame: more char is wrong");

		/* unref frame node allocated */
		vortex_frame_free (frame);
		return NULL;
	}

	/* set more flag */
	if (frame->more_char == '.')
		frame->more = axl_false;
	else
		frame->more = axl_true;

	/* check incoming frame size fits expected window size - seqno  */
	if (! vortex_channel_check_incoming_seqno (frame->channel_ref, frame)) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, 
			"received an unexpected frame size (max seqno expected: %u, but received: %u), frame seqno: %u, frame size: %d, expected: %u), closing session",
			vortex_channel_get_max_seq_no_accepted (frame->channel_ref), frame->seqno + frame->size,
			frame->seqno, frame->size, vortex_channel_get_max_seq_no_accepted (frame->channel_ref));

		/* unref frame node allocated */
		vortex_frame_free (frame);
		return NULL;
	}

	/* allocate exactly frame->size + 5 bytes */
	buffer = malloc (sizeof (char) * (frame->size + 6));
	VORTEX_CHECK_REF2 (buffer, NULL, frame, axl_free);
	
	/* read the next frame content */
	bytes_read = vortex_frame_receive_raw (connection, buffer, frame->size + 5);
 	if (bytes_read == 0 && errno != VORTEX_EAGAIN && errno != VORTEX_EWOULDBLOCK) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "remote peer have closed connection while reading the rest of the frame");

		/* unref frame node allocated */
		vortex_frame_free (frame);

		/* unref buffer allocated */
		axl_free (buffer);
		return NULL;
	}

	if (bytes_read != (frame->size + 5)) {
		/* ok, we have received few bytes than expected but
		 * this is not wrong. Non-blocking sockets behave this
		 * way. What we have to do is to store the frame chunk
		 * we already read and the rest of bytes expected. 
		 *
		 * Later on, next frame_get_next calls on the same
		 * connection will return the rest of frame to be read. */

	save_buffer:
		/* save current frame */
		connection->last_frame = frame;
		
		/* save current buffer read */
		connection->buffer = buffer;
		
		/* save remaining bytes */
		connection->remaining_bytes = (frame->size + 5) - bytes_read;

		/* save read bytes */
		connection->bytes_read      = bytes_read;

		vortex_log (VORTEX_LEVEL_DEBUG, 
		       "(ok message) received a frame fragment (expected: %d read: %d remaining: %d), storing into this connection id=%d",
		       (frame->size + 5), bytes_read, (frame->size + 5) - bytes_read, vortex_connection_get_id (connection));
		return NULL;
	}

process_buffer:

	/* check frame have ended */
	if (! axl_stream_cmp (&buffer[bytes_read - 5], "END\x0D\x0A", 5)) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "poorly formed frame: frame trailer CR LF not found, discarding content: '%s'",
			(buffer != NULL) ? buffer : "(null content)");

		/* unref frame node allocated */
		vortex_frame_free (frame);

		/* unref buffer allocated */
		axl_free (buffer);
		return NULL;
	}
	
	/* point to the content received and nullify trailing BEEP frame */
	frame->payload   = buffer;
	((char*)frame->payload) [frame->size] = 0;

	/* get a reference to the buffer to dealloc it */
	frame->buffer    = buffer;

#if defined(ENABLE_VORTEX_LOG)
	/* log frame on channel received */
	if (vortex_log_is_enabled (ctx)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "Frame received on channel %d, conn-id=%d, content type=%s, transfer encoding=%s, payload size=%d, mime content size=%d", 
 			    frame->channel,
			    vortex_connection_get_id (connection),
 			    (vortex_frame_get_content_type (frame) != NULL) ? vortex_frame_get_content_type (frame) : "" ,
 			    (vortex_frame_get_transfer_encoding (frame) != NULL) ? vortex_frame_get_transfer_encoding (frame) : "",
 			    frame->size, frame->mime_headers_size);
	} /* end if */
#endif

	/* nullify */

	return frame;

}


/** 
 * @internal
 * 
 * Sends data over the given connection
 * 
 * @param connection 
 * @param a_frame 
 * @param frame_size 
 * 
 * @return 
 */
axl_bool             vortex_frame_send_raw     (VortexConnection * connection, const char  * a_frame, int  frame_size)
{

	VortexCtx  * ctx    = vortex_connection_get_ctx (connection);
	int          bytes  = 0;
 	int          total  = 0;
	char       * error_msg;
 	int          fds;
 	int          wait_result;

	/* get timeout or use 3 as default value */
 	int          tries    = (ctx->conn_close_on_write_timeout > 0) ? ctx->conn_close_on_write_timeout : 3 ;

 	axlPointer   on_write = NULL;

	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (vortex_connection_is_ok (connection, axl_false), axl_false);
	v_return_val_if_fail (a_frame, axl_false);

 again:
	if ((bytes = vortex_connection_invoke_send (connection, a_frame + total, frame_size - total)) < 0) {
		if (errno == VORTEX_EINTR)
			goto again;
 		if ((errno == VORTEX_EWOULDBLOCK) || (errno == VORTEX_EAGAIN) || (bytes == -2)) {
 		implement_retry:
 			vortex_log (VORTEX_LEVEL_WARNING, 
 				    "unable to write data to socket (requested %d but written %d), socket not is prepared to write, doing wait",
 				    frame_size, total);
 
 			/* create and configure waiting set */
 			if (on_write == NULL) {
 				on_write = vortex_io_waiting_invoke_create_fd_group (ctx, WRITE_OPERATIONS);
 			} /* end if */
 
 			/* clear and add  fd */
 			vortex_io_waiting_invoke_clear_fd_group (ctx, on_write);
 			fds = vortex_connection_get_socket (connection);
 			if (! vortex_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_write)) {
				__vortex_connection_shutdown_and_record_error (
					connection, VortexProtocolError,
					"failed to add connection to waiting set for write operation, closing connection");
 				goto end;
 			} /* en dif */
 
 			/* perform a wait operation */
 			wait_result = vortex_io_waiting_invoke_wait (ctx, on_write, fds + 1, WRITE_OPERATIONS);
 			switch (wait_result) {
 			case -3: /* unrecoberable error */
				__vortex_connection_shutdown_and_record_error (
					connection, VortexError, "unrecoberable error was found while waiting to perform write operation, closing connection");
 				goto end;
 			case -2: /* error received while waiting (soft error like signals) */
 			case -1: /* timeout received */
 			case 0:  /* nothing changed which is a kind of timeout */
 				vortex_log (VORTEX_LEVEL_DEBUG, "found timeout while waiting to perform write operation (tries=%d)", tries);
 				tries --;

				if (ctx->disable_conn_close_on_write_timeout)
					goto again; /* connection close on write timeout is disabled, so try again */

 				if (tries == 0) {
					__vortex_connection_shutdown_and_record_error (
						connection, VortexError,
						"found timeout while waiting to perform write operation and maximum tries were reached");
 					goto end;
 				} /* end if */
 				goto again;
 			default:
 				/* default case when it is found the socket is now available */
 				vortex_log (VORTEX_LEVEL_DEBUG, "now the socket is able to perform a write operation, doing so (total written until now %d)..",
 					    total);
 				goto again;
 			} /* end switch */
			goto end;
		}
		
		/* check if socket have been disconnected (macro
		 * definition at vortex.h) */
		if (vortex_is_disconnected) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError,
				"remote peer have closed connection");
			goto end;
		}
		error_msg = vortex_errno_get_last_error ();
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError, "unable to write data to socket: %s, errno=%d (%s), socket=%d conn-id=%d, conn=%p",
			error_msg ? error_msg : "", errno, vortex_errno_get_last_error (), connection->session, connection->id, connection);
		goto end;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "bytes written: %d", bytes);

	if (bytes == 0) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError,
			"remote peer have closed before sending proper close connection, closing");
		goto end;
	}

 	/* sum total amount of data */
 	if (bytes > 0) {
		/* notify content written (content received) */
		vortex_connection_set_receive_stamp (connection, 0, bytes);

 		total += bytes;
	}

 	if (total != frame_size) {
 		vortex_log (VORTEX_LEVEL_CRITICAL, "write request mismatch with write done (%d != %d), pending tries=%d",
 			    total, frame_size, tries);
 		tries--;
 		if (tries == 0) {
 			goto end;
		}
 
 		/* do retry operation */
 		goto implement_retry;
  	}
	vortex_log (VORTEX_LEVEL_DEBUG, "write on socket request=%d written=%d", frame_size, bytes);
 end:

 	/* clear waiting set before returning */
 	if (on_write)
 		vortex_io_waiting_invoke_destroy_fd_group (ctx, on_write);
	
 	return (total == frame_size);
}


/** 
 * @brief Increases the frame reference counting.
 *
 * The function isn't thread safe, which means you should call to this
 * function from several threads. In is safe to call this function on
 * every place you receive a frame (second and first level invocation
 * handler).
 *
 * Once you use this function, you can't call directly to \ref
 * vortex_frame_free. Instead a call to \ref vortex_frame_unref is
 * required. 
 *
 * Calling to \ref vortex_frame_free when no reference count was
 * increases is the same as calling to \ref vortex_frame_unref. This
 * means that code written against Vortex Library 0.9.0 (or previous)
 * will keep working doing calls to \ref vortex_frame_free.
 * 
 * @param frame The frame to increase its reference counting. 
 * 
 * @return axl_true if the frame reference counting was
 * increased. Otherwise, axl_false is returned and the reference counting
 * is left untouched.
 */
axl_bool           vortex_frame_ref                   (VortexFrame * frame)
{

	/* check reference received */
	v_return_val_if_fail (frame, axl_false);

	/* increase the frame counting */
	frame->ref_count++;

	return axl_true;
}

/** 
 * @brief Allows to decrease the frame reference counting, making an
 * automatic call to vortex_frame_free if the reference counting reach
 * 0.
 * 
 * @param frame The frame to be unreferenced (and deallocated if the
 * ref count reach 0).
 */
void          vortex_frame_unref                 (VortexFrame * frame)
{
	if (frame == NULL)
		return;
	
	/* decrease reference counting */
	frame->ref_count--;

	/* check and dealloc */
	if (frame->ref_count == 0) {
		vortex_frame_free (frame);
	}
	
	return;
}

/** 
 * @brief Returns current reference counting for the frame received.
 * 
 * @param frame The frame that is required to return the current
 * reference counting.
 * 
 * @return Reference counting or -1 if it fails. The function could
 * only fail if a null reference is received.
 */
int           vortex_frame_ref_count             (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);
	return frame->ref_count;
}


/** 
 * @brief Deallocate the frame. You shouldn't call this directly,
 * instead use \ref vortex_frame_unref.
 *
 * Frees a allocated \ref VortexFrame. Keep in mind that, unless
 * stated, \ref VortexFrame object received from the Vortex Library
 * API are not required to be unrefered. 
 *
 * In particular, first and second level invocation automatically
 * handle frame disposing. See \ref vortex_manual_dispatch_schema
 * "Vortex Frame Dispatch schema".
 *
 * @param frame The frame to free.
 **/
void          vortex_frame_free (VortexFrame * frame)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	if (frame == NULL)
		return;

	/* log a frame deallocated message */
#if defined(ENABLE_VORTEX_LOG)
	ctx = frame->ctx;
	vortex_log (VORTEX_LEVEL_DEBUG, "deallocating frame id=%d", frame->id);
#endif

	/* free MIME headers */
	vortex_frame_mime_status_free (frame->mime_headers);

	/* free frame payload (first checking for content, and, if not
	 * defined, then payload) */
	if (frame->buffer != NULL)
		axl_free (frame->buffer);
	else if (frame->content != NULL)
		axl_free (frame->content);
	else if (frame->payload != NULL)
		axl_free (frame->payload);

	/* release reference to the context */
	vortex_ctx_unref2 (&frame->ctx, "end frame");

	/* free the frame node itself */
	axl_free (frame);
	return;
}

VortexFrame * __vortex_frame_join_common (VortexFrame * a, VortexFrame * b, axl_bool      reuse)
{
	VortexFrame * result;
	
	/* only check if frames are joinables if the first one is different from NULL */
	if (!vortex_frame_are_joinable (a, b))
		return NULL;

	/* copy current frame values */
	result                    = axl_new (VortexFrame, 1);
	VORTEX_CHECK_REF (result, NULL);

	/* acquire a reference to the context */
	vortex_ctx_ref2 (a->ctx, "new frame");

	/* set initial ref counting */
	result->ref_count         = 1;
	
	/* get next Id for this new frame */
	result->id                = __vortex_frame_get_next_id (a->ctx, "frame-join");
	result->ctx               = a->ctx;
	result->type              = a->type;
	result->channel           = a->channel;
	result->msgno             = a->msgno;
	result->more              = (a->more && b->more);
	result->seqno             = a->seqno;
	result->size              = a->size + b->size;
	result->mime_headers_size = a->mime_headers_size;
	result->ansno             = a->ansno;
	result->channel_ref       = a->channel_ref;

	/* join payload for both frames */
	if (reuse) {
		/* allocates memory only for the rest of b */
		result->payload  = axl_realloc (a->payload, a->size + b->size + 1);
		VORTEX_CHECK_REF2 (result->payload, NULL, result, axl_free);
		a->payload       = NULL;
	} else {
		/* allocates memory to hold both elements a and b */
		result->payload  = axl_new (char , a->size + b->size + 1);
		VORTEX_CHECK_REF2 (result->payload, NULL, result, axl_free);

		/* mem copy a over result */
		memcpy (result->payload, a->payload, a->size);
	}
	/* now copy b over result starting from a's ending */
	memcpy ((unsigned char *) result->payload + a->size, 
		    b->payload, b->size);

	/* because mime headers are found at the begining of the
	 * frame, joing operations will move headers */
	result->mime_headers = a->mime_headers;
	a->mime_headers      = NULL;

	return result;
}

/**
 * @brief Allows to join two frames into a newly allocated one.
 * 
 * Joins the frame a with b. This is done by concatenating frame a
 * payload followed by frame b payload and adding both payload size,
 * both seqno number.  It also checks the more flag for both frames to
 * if it detect some error it will return NULL.  
 *
 * Frame types for both are also checked so if frame type differs a
 * NULL is returned. Channel number for both frames are also checked.
 *
 * As a result, frame joining done by this function is equal to having
 * activated \ref vortex_channel_set_complete_flag "the complete flag".
 *
 * 
 * @param a The frame to join.
 * @param b The frame to join.
 * 
 * @return the frame joined or NULL if fails. Returned value is
 * newly allocated.
 **/
VortexFrame * vortex_frame_join         (VortexFrame * a, VortexFrame * b)
{
	/* join without reusing */
	return __vortex_frame_join_common (a, b, axl_false);
}

/** 
 * @brief Allows to join two frames, without allocating a new one but,
 * reusing memory allocated by the first frame, saving memory an
 * memory dumping operations.
 * 
 * This function is the same as \ref vortex_frame_join. However, it
 * returns a copy to a new frame joined, reusing memory used by the
 * frame <b>a</b>. Once finished this function, the frame <b>a</b>
 * can't be used. 
 * 
 * @param a The frame to use as base for the resulting frame.
 * @param b The frame to add.
 * 
 * @return The new frame reference, contaning the union.
 */
VortexFrame * vortex_frame_join_extending (VortexFrame * a, VortexFrame * b)
{
	/* join without reusing */
	return __vortex_frame_join_common (a, b, axl_true);
}

/** 
 * @internal
 * Check two strings to be equal, including NULL values.
 * 
 * @param value_a The first value to check.
 * @param value_b The second value to check.
 * 
 * @return 
 */
axl_bool      vortex_frame_common_string_check (const char  * value_a, const char  * value_b)
{
	if (value_a == NULL && value_b == NULL)
		return axl_true;
	if (value_a != NULL && value_b == NULL)
		return axl_false;
	if (value_a == NULL && value_b != NULL)
		return axl_false;
	return axl_cmp (value_a, value_b);
}

/** 
 * @brief Allows to check if the given frames are joinable or the first one follows the next.
 * 
 * @param a The frame to check.
 * @param b The frame to check.
 * 
 * @return axl_true if the frame b comes after a being possible to be both
 * joinable into only one frame.
 */
axl_bool      vortex_frame_are_joinable (VortexFrame * a, VortexFrame * b) 
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	/*
	 * Because frames inside vortex with mime type are handled
	 * separating payload from entity-headers (mime headers), the
	 * payload size the frame have is only the payload of the mime
	 * content.
	 *
	 * If no mime content is used the payload size is the same
	 * that the frame payload content. According to BEEP
	 * specification, the payload and its mime content is
	 * considered payload. 
	 * 
	 * However, to simply using frames under Vortex Library, and
	 * to avoid making API consumers to place code that process
	 * payload with mime headers and payload without them, frame
	 * are read in a way that the payload size is the size of the
	 * mime content, without including the mime headers.
	 *
	 * As a consequence, when this tries to check if two frames
	 * are joinable it have to consider if the frame payload size
	 * plus mime type entity headers size, if defined. 
	 *
	 * That's why this function check the seq number with the mime
	 * type headers plus the payload size.
	 */
	if (a == NULL || b == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)
	/* get the context */
	ctx = a->ctx;
#endif
	
	if (a->type != b->type) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because type mismatch");
		return axl_false;
	}

	if (!a->more && !b->more)  {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because more flag mismatch");
		return axl_false;
	}

	if (!a->more && b->more) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because more flag mismatch 2");
		return axl_false;
	}

	if (a->channel != b->channel)  {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because channel mismatch");
		return axl_false;
	}

	if (a->msgno != b->msgno)  {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because  msgno mismatch");
		return axl_false;
	}

	if (a->ansno != b->ansno)  {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because ansno mismatch");
		return axl_false;
	}

	if ((a->seqno + a->size + a->mime_headers_size) != b->seqno)  {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not joinable because seqno mismatch (%u + %d + %d != %u)",
		       a->seqno, a->size, a->mime_headers_size, b->seqno);
		return axl_false;
	}

	return axl_true;
}

/** 
 * @brief Allows to check if the given frames are equal.
 *
 * This function will check header specification and payload to
 * report if the given frames are equal.
 * 
 * @param a The frame to compare
 * @param b The frame to compare
 * 
 * @return axl_true if both frames are equal, axl_false if not.
 */
axl_bool      vortex_frame_are_equal (VortexFrame * a, VortexFrame * b)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	
	if (a == NULL || b == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)
	/* get the context */
	ctx = a->ctx;
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "checking for equality");

	/* check frame type */
	if (a->type != b->type) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to type (%d != %d)",
		       a->type, b->type);
		return axl_false;
	}
	
	/* check more flag type */
	if (a->more != b->more) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to more flag (%d != %d)",
		       a->more, b->more);
		return axl_false;
	}
	
	/* check channel number */
	if (a->channel != b->channel) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to channel number (%d != %d)",
		       a->channel, b->channel);
		return axl_false;
	}

	/* check message number */
	if (a->msgno != b->msgno) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to message number (%d != %d)",
		       a->msgno, b->msgno);
		return axl_false;
	}

	/* check sequence number */
	if (a->seqno != b->seqno) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to sequence number (%u != %u)",
		       a->seqno, b->seqno);
		return axl_false;
	}
	
	/* check frame size */
	if (a->size != b->size) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to frame size (%d != %d)",
		       a->size, b->size);
		return axl_false;
	}
	
	/* check ans number */
	if (a->ansno != b->ansno) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to ansno name (%d != %d)",
		       a->ansno, b->ansno);
		return axl_false;
	}

	/* check frame content type */
 	if (!vortex_frame_common_string_check (vortex_frame_get_content_type (a), 
 					       vortex_frame_get_content_type (b))) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to content type value");
		return axl_false;
	}

	/* check for frame transfer encoding type */
	if (!vortex_frame_common_string_check (vortex_frame_get_transfer_encoding (a), 
					       vortex_frame_get_transfer_encoding (b))) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to content transfer encoding value");
		return axl_false;
	}

	/* check payload */
	if (!vortex_frame_common_string_check (a->payload, b->payload)) {
		vortex_log (VORTEX_LEVEL_WARNING, "frames are not equal due to payload");
		return axl_false;
	}

	return axl_true;
}

/** 
 * @brief Allows to get the unique frame identifier for the given
 * frame.
 *
 * Inside Vortex Library, every frame created is flagged with an
 * unique frame identifier which is not part of the BEEP protocol,
 * transmitted or received from the remote side.
 *
 * This unique identifiers was created for the internal Vortex Library
 * support but, it has become a useful mechanism to have frames
 * identified. 
 *
 * Application level could use this value to implement fast frame
 * recognition rather than using \ref vortex_frame_are_equal, which is
 * more expensive.
 *
 * Vortex Library will ensure, as long as the process is in memory
 * that every unique identifier returned/generated for a frame will be
 * unique.
 * 
 * @param frame The frame where the unique identifier value will be
 * get.
 * 
 * @return Returns the unique frame identifier or -1 if it fails. The
 * only way for this function to fail is to receive a NULL value.
 */
int           vortex_frame_get_id                (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);
	return frame->id;
}

/**
 * @brief return actual frame type.
 *
 * Returns actual frame type.
 *
 * @param frame The frame where the type is being requested.
 * 
 * @return the actual frame type or -1 if fail. The function only
 * fails if a null frame is received.
 **/
VortexFrameType vortex_frame_get_type   (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);	

	return frame->type;
}

/** 
 * @brief Returns frame content type.
 *
 * Return actual frame's content type. The function always return a
 * content type, either because it is defined or because default value
 * is provided ("application/octet-stream").
 *
 * @param frame The frame where the content type will be returned.
 *
 * @return The content type or NULL if the function fail (because the
 * frame received is NULL or because the frame do not contains a MIME
 * message).
 **/
const char   * vortex_frame_get_content_type (VortexFrame * frame)
{
 	VortexMimeHeader * header;
	
  	v_return_val_if_fail (frame, NULL);
  
  	/* return value associated to MIME_CONTENT_TYPE entry */
 	if (frame->mime_headers || frame->content != NULL) {
 		header = vortex_frame_get_mime_header (frame, "content-type");
 		if (header != NULL)
 			return header->content;
		return "application/octet-stream";
 	} /* end if */

	return NULL;
}

/** 
 * @brief Allows to get current Content-Transfer-Encoding mime header
 * configuration for the given frame (if defined).
 *
 * <i><b>NOTE:</b> this function is deprecated. Use generic MIME
 * header access through \ref vortex_frame_get_mime_header. </i>
 * 
 * @param frame The frame where the data request will be reported.
 * 
 * @return Current mime header configuration request or NULL if not
 * defined (because the frame received is NULL or because the frame do
 * not contains a MIME message).
 */
const char   * vortex_frame_get_transfer_encoding (VortexFrame * frame)
{
	VortexMimeHeader * header;

	v_return_val_if_fail (frame, NULL);

	/* return value associated to MIME_CONTENT_TYPE entry */
	if (frame->mime_headers || frame->content != NULL) {
		header = vortex_frame_get_mime_header (frame, "content-transfer-encoding");
		if (header != NULL)
			return header->content;
		return "binary";
	} /* end if */

	return NULL;
}

/** 
 * @brief Allows to get current status of the mime header size for the given frame. 
 *
 * This function is useful to avoid using \ref
 * vortex_frame_get_transfer_encoding and \ref
 * vortex_frame_get_content_type function and then adding mime tags
 * plus trailing characters.
 * 
 * This function return the total sum for the given frame.
 * 
 * @param frame The frame where the mime header size will be reported.
 * 
 * @return 0 or the mime header size count.
 */
int           vortex_frame_get_mime_header_size  (VortexFrame * frame)
{
	v_return_val_if_fail (frame, 0);

	return frame->mime_headers_size;
}

/**
 * @brief Returns current channel used to sent or receive the given frame.
 * 
 * Returns actual frame's channel number.
 *
 * @param frame The frame where the channel number value is requested.
 *
 * @return the channel number or -1 if fails
 **/
int           vortex_frame_get_channel  (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	return frame->channel;
}

/** 
 * @brief Returns current channel reference where the frame was
 * received.
 * 
 * @param frame The frame that is required to return the channel where
 * the frame was received.
 * 
 * @return A reference to the \ref VortexChannel where the frame was
 * received. Do not dealloc this reference. It is an internal copy.
 */
VortexChannel * vortex_frame_get_channel_ref (VortexFrame * frame)
{
	v_return_val_if_fail (frame, NULL);

	/* return the channel reference */
	return frame->channel_ref;
}

/** 
 * @internal Allows to configure the channel where the frame was
 * received. This is an internal function and should be used unless
 * you know what you are doing.
 * 
 * @param frame The frame to configure.
 */
void vortex_frame_set_channel_ref (VortexFrame * frame, VortexChannel * channel)
{
	v_return_if_fail (frame);
	
	/* configure the channel to the frame */
	frame->channel_ref = channel;

	return;
}

/**
 * @brief Return current message number used for the given frame
 *
 * Returns actual frame's msgno.
 * 
 * @param frame The frame where the msgno value is requested.
 * 
 * @return the msgno or -1 if fail.
 **/
int           vortex_frame_get_msgno    (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	return frame->msgno;
}

/**
 * @brief Returns actual more flag status for the given frame.
 * 
 * Returns actual frame more flag status. If more flag is activated 1
 * will be returned.  In case of been deactivated 0 will be returned.
 *
 * @param frame The frame where the more flag value is requested.
 *
 * Return value: the actual more flag status or -1 if fails. 1 have
 * activated (*), 0 deactivated (.).
 *
 * <i><b>NOTE:</b> To properly use this function you must use the following to
 * check if frame flag is activated:</i>
 * 
 * \code
 * if (vortex_frame_get_more_flag (frame) > 0) {
 *    // some handling 
 * }
 * \endcode
 *
 * <i>This is because the function could return -1 causing the
 * following code to also report that the frame have the more flag
 * activated: </i>
 *
 * \code
 * if (vortex_frame_get_more_flag (frame)) {
 *    // some handling (WRONG: frame might be NULL)
 * }
 * \endcode
 *
 **/
int           vortex_frame_get_more_flag (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	if (frame->more)
		return 1;
	else
		return 0;
}

/**
 * @brief Returns the current sequence number for the given frame.
 * 
 * Returns the actual frame's seqno.
 * 
 * @param frame The frame where the seqno value is requested.
 *
 * @return the frame seqno or -1 if fails
 **/
unsigned int     vortex_frame_get_seqno    (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	return frame->seqno;
}

/**
 * @brief Returns the payload associated to the given frame. 
 * 
 * Return actual frame payload. You must not free returned payload.
 *
 * Keep in mind that this function returns the frame payload without
 * the mime headers. All mime headers read could be accessed through
 * \ref vortex_frame_get_content_type and \ref
 * vortex_frame_get_transfer_encoding.
 *
 * @param frame The frame where the payload is being requested.
 * 
 * @return the actual frame's payload or NULL if fail.
 **/
const void *  vortex_frame_get_payload  (VortexFrame * frame)
{
	v_return_val_if_fail (frame, NULL);

 	/* if payload (MIME body) is defined, return it rather all the content */
	return frame->payload;
}

/**
 * @brief Returns current ans number for the given frame.
 * 
 * Returns actual frame ansno.
 *
 * @param frame The frame where the ansno value is requested.
 *
 * @return the ansno value or -1 if fails
 **/
int           vortex_frame_get_ansno    (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	return frame->ansno;
}


/**
 * @brief Returns the current payload size the given frame have
 * without taking into account mime headers.
 *
 * This function will return current payload size stored on the given
 * frame, skipping mime header size. This is because Vortex Library
 * manages mime headers for every frame received, making a separation
 * between the mime header part and the mime content.
 *
 * However, if you don't use mime headers, this function will return
 * the entire payload size.
 *
 * If you need to get current frame content size, including payload
 * size and mime content size, use \ref vortex_frame_get_content_size.
 *
 * 
 * @param frame The frame where the payload size will be reported.
 *
 * @return the actual payload size or -1 if fail
 **/
int    vortex_frame_get_payload_size (VortexFrame * frame)
{
	v_return_val_if_fail (frame, -1);

	return frame->size;

}

/** 
 * @brief Allows to get current frame content size, including payload
 * and mime headers.
 *
 * Every frame have a payload that could be separated into the mime
 * headers and the mime content. 
 *
 * You can get current mime header size by using \ref
 * vortex_frame_get_mime_header_size and current payload size by
 * calling to \ref vortex_frame_get_payload_size.
 *
 * @param frame The frame where the current content size will be
 * reported.
 * 
 * @return Current content size or 0 if it fails.
 */
int           vortex_frame_get_content_size      (VortexFrame * frame)
{
	v_return_val_if_fail (frame, 0);
	
	return frame->size + frame->mime_headers_size;
	
}

/** 
 * @brief Allows to get the context reference under which the frame
 * was created.
 * 
 * @param frame The frame that is required to return its context.
 * 
 * @return A reference to the context (\ref VortexCtx) or NULL if it
 * fails.
 */
VortexCtx   * vortex_frame_get_ctx               (VortexFrame * frame)
{
	if (frame == NULL)
		return NULL;

	/* return context configured */
	return frame->ctx;
}

/** 
 * @brief Allows to get all frame content including mime headers and
 * mime body. 
 * 
 * @param frame The frame that is required to return the content.
 * 
 * @return A reference to all the content the frame has.
 */
const char *  vortex_frame_get_content           (VortexFrame * frame)
{
	v_return_val_if_fail (frame, 0);

	/* return all content */
	if (frame->content != NULL)
		return frame->content;
	if (frame->payload != NULL)
		return frame->payload;
	return NULL;
}

/* Mapping values \n -> \x0A \r -> \x0D */
#define _ok_msg  "<ok />"


/** 
 * @brief Returns the ok message.
 *
 * @return A ok message. You must not free the returned message. It is
 * a copy to an internal Vortex Frame module.
 */
const char        * vortex_frame_get_ok_message        (void)
{
	return _ok_msg;
}

/** 
 * @brief Creates and return the error message.
 *
 * Allows to create a new error message that should be unrefered when
 * no longer is needed.
 * 
 * @param code          The error code to report.
 * @param error_content The error content, actually the textual error diagnostic.
 * @param xml_lang      The language used to define the diagnostic message.
 * 
 * @return A newly allocated error message.
 */
char        * vortex_frame_get_error_message    (const char  * code, 
						 const char  * error_content,
						 const char  * xml_lang)
{
	return axl_strdup_printf ("<error code='%s'%s%s%s%s%s%s",
				  code,
				  (xml_lang != NULL)      ? " xml:lang='"       : "",
				  (xml_lang != NULL)      ? xml_lang            : "",
				  (xml_lang != NULL)      ? "' "                : "",
				  (error_content != NULL) ? " >"                : " />\x0D\x0A",
				  (error_content != NULL) ? error_content       : "",
				  (error_content != NULL) ? "</error>\x0D\x0A"  : "");
}

/** 
 * @brief Allows to check if the given frame contains a BEEP error
 * message inside the frame payload.
 *
 * The function not only returns if the frame is an error message but
 * also returns the error code and the textual message.
 *
 * This function expects to receive a BEEP error message as part of
 * the profile being implemented. This function shouldn't be used to
 * general error checking. The function expects a particular error
 * reply format, that is, the BEEP error message:
 *
 * \code
 *  <error code='501'>textual error reported</error>
 * \endcode
 *
 * If your intention is to check a generic error message received,
 * with a different format, you could use \ref vortex_frame_get_type,
 * looking at \ref VORTEX_FRAME_TYPE_ERR. This is a generic
 * recomendation because some profiles uses only MSG/RPY, implementing
 * the error reporting inside the message content received.
 * 
 * @param frame   The frame to check for error message inside.
 *
 * @param code The error code the error message have (if
 * defined). This value must be deallocated by calling to axl_free.
 *
 * @param message The textual error message (if defined). This value
 * must be deallocated by calling to axl_free.
 * 
 * @return axl_true if the frame contains an error message, axl_false if not.
 */
axl_bool           vortex_frame_is_error_message      (VortexFrame * frame,
						       char  ** code,
						       char  ** message)
{
	v_return_val_if_fail (frame, axl_false);
	return vortex_channel_validate_err (frame, code, message);
}

/** 
 * @brief Creates and return an new start message using the given data.
 * 
 * 
 * @param channel_num          The channel number to be used for the new start message
 * @param serverName           The serverName attribute to be used for the start element.
 * @param profile              The profile element to be used inside the uri value.
 * @param encoding             How is encoded the profile content.
 * @param content_profile      The content profile data
 * @param profile_content_size Content profile size.
 * 
 * @return new start message that have to be deallocated when the it
 * is no longer need.
 */
char        * vortex_frame_get_start_message     (int              channel_num,
						  const char     * serverName, 
						  const char     * profile,  
						  VortexEncoding   encoding,
						  const char     * content_profile,
						  int              profile_content_size)
{
	/* build initial start message using channel num and
	 * serverName value. */
	return axl_strdup_printf ("<start number='%d'%s%s%s>\x0D\x0A   <profile uri='%s'%s%s%s%s%s%s</start>\x0D\x0A",
				  channel_num, 
				  /* put server name definition if defined */
				  (serverName != NULL)         ? " serverName='"      : "",
				  (serverName != NULL)         ? serverName           : "",
				  (serverName != NULL)         ? "'"                  : "",
				  profile,
				  (encoding == EncodingBase64) ? " encoding='base64'" : "",
				  (content_profile == NULL)    ? " />\x0D\x0A"        : ">",
				  (content_profile != NULL)    ? "<![CDATA["          : "",
				  (content_profile != NULL)    ? content_profile      : "",
				  (content_profile != NULL)    ? "]]>"                : "",
				  (content_profile != NULL)    ? "</profile>\x0D\x0A" : "");
}

/** 
 * @brief Creates and return the close message.
 * 
 * Once the close message is no longer needed, it must be deallocated.
 * 
 * @param number        The number for the close message
 * @param code          The close code to report.
 * @param xml_lang      The xml lang value.
 * @param close_content The close message content.
 * 
 * @return A newly allocate close message which has to be deallocated
 * when no longer needed.
 */
char        * vortex_frame_get_close_message     (int           number,
						  const char  * code,
						  const char  * xml_lang,
						  const char  * close_content)
{
	return axl_strdup_printf ("<close number='%d' code='%s'%s%s%s%s%s%s",
				  number,
				  code,
				  (xml_lang != NULL)      ? " xml:lang='"       : "",
				  (xml_lang != NULL)      ? xml_lang            : "",
				  (xml_lang != NULL)      ? "' "                : "",
				  (close_content != NULL) ? " >"                : " />\x0D\x0A",
				  (close_content != NULL) ? close_content       : "",
				  (close_content != NULL) ? "</close>\x0D\x0A"  : "");
}

/** 
 * @brief Allows to create and return a start reply message.
 * 
 * @param profile The profile URI.
 * @param profile_content The optional profile content (piggyback value).
 * 
 * @return A newly allocated message that must be deallocated when no
 * longer needed.
 */
char        * vortex_frame_get_start_rpy_message (const char  * profile, 
						  const char  * profile_content)
{
	return axl_strdup_printf ("<profile uri='%s'%s%s%s%s%s",
				  profile,
				  (profile_content != NULL) ? " >"                  : " />\x0D\x0A",
				  (profile_content != NULL) ? "<![CDATA["           : "",
				  (profile_content != NULL) ? profile_content       : "",
				  (profile_content != NULL) ? "]]>"                 : "",
				  (profile_content != NULL) ? "</profile>\x0D\x0A"  : "");
}

/** 
 * @internal Function that allows to lookup for MIME header fields
 * (header name and content).
 * 
 * @return The function returns 1 if frame was found and its
 * content. 0 if nothing was found, and -1 if a MIME header error was
 * found.
 */
int vortex_frame_read_mime_header (VortexFrame  * frame, 
				   int          * caller_iterator)
{
	char             * payload    = (char *) frame->payload;
	int                iterator   = (* caller_iterator);
	int                mark;
	char             * mime_header;
	VortexMimeHeader * header;
	axl_bool           first_definition = axl_true;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx        = frame->ctx;
#endif

	/* try to read until first ':' or ' ' or '\t' */
	mark = iterator;
	while (iterator < frame->size && 
	       payload[iterator] != ':' &&
	       payload[iterator] != ' ' &&
	       payload[iterator] != '\t' &&
	       /* limit allowed values inside MIME header field */
	       (int) payload[iterator] <= 126 && (int) payload[iterator] >= 33 &&
	       /* limit amount of content a header could contain */
	       (iterator - mark) < 995)
		iterator++;

	/* check MIME header field content */
	if ((int) payload[iterator] > 126 || (int) payload[iterator] < 33) {
		vortex_log (VORTEX_LEVEL_WARNING, "found MIME header error, (RFC 2882, 2.2) using a not allowed value for header field name at %d (value '%c'(%d)",
			    iterator, payload[iterator], (int) payload[iterator]);
		return -1;
	} /* end if */

	/* check MIME header size */
	if ((iterator - mark) >= 995) {
		vortex_log (VORTEX_LEVEL_WARNING, "found MIME header error, (RFC 2882, 2.2) header field name reached maximum value allowed (998)");
		return -1;
	}

	/* check if didnt found a MIME header */
	if (iterator == frame->size) {
		vortex_log (VORTEX_LEVEL_WARNING, "no MIME header was found..");
		return -1;
	} /* end if */

	/* get memory to hold the mime headers name */
	mime_header = axl_string_factory_alloc (frame->mime_headers->mime_headers_name, iterator - mark + 1);
	if (mime_header == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to allocate memory to hold MIME header field");
		return -1;
	}
	
	/* copy content and configure mime header size */
	memcpy (mime_header, payload + mark, iterator - mark);

	/* check if the mime header was already found */
	header = vortex_frame_mime_find (frame->mime_headers, mime_header);
	if (header == NULL) {
		/* first header found */
		header       = axl_factory_get (frame->mime_headers->header_factory);
		header->name = mime_header;
	} else {
		/* found, forward to the last found until now */
		while (header->next != NULL) {
			/* go the next */
			header = header->next;
		} /* end while */

		/* create the new header */
		header->next  = axl_factory_get (frame->mime_headers->header_factory);
		header        = header->next;
		header->name  = mime_header;

		/* flag that the header was already defined */
		first_definition = axl_false;
	} /* end if */

	/* check to update references */
	vortex_frame_mime_check_and_update_fast_ref (frame->mime_headers, header);

	/* consume all spaces before reaching ":" */
	while (iterator < frame->size && payload[iterator] == ' ')
		iterator++;
	
	/* now check for terminator : */
	if (payload[iterator] != ':') {
		vortex_log (VORTEX_LEVEL_WARNING, 
			    "expected to find MIME field name delimitator ':' but found '%c', while reading for MIME header '%s'",
			    payload[iterator], mime_header);
		return -1;
	} /* end if */
		
	/* mark mime content */
	mark = iterator + 1;

	/* get all content */
	/* vortex_log (VORTEX_LEVEL_DEBUG, "reading mime header %s content at: %d", 
	   mime_header, iterator2); */
	while (axl_true) {
		/* check for CR-LF termination without being followed
		 * by a WSP */
		if (((iterator + 1) < frame->size) &&
		    payload[iterator] == '\x0D' && payload[iterator + 1] == '\x0A') {
			
			if (payload[iterator + 2] != ' ' && payload[iterator + 2] != '\t')
				break;
		} /* end if */
		
		/* check for LF termination without being followed by a WSP */
		if ((iterator < frame->size) && payload[iterator] == '\x0A') {

			if (payload[iterator + 1] != ' ' && payload[iterator + 1] != '\t') 
				break;
		} /* end if */

		/* stop the look in case frame size was reached */
 		if (iterator == frame->size)
			break;

		/* next character */
		iterator++;
	} /* end while */
	
	/* found mime content */
	header->content = axl_string_factory_alloc (frame->mime_headers->mime_headers_content, iterator - mark + 1);
	memcpy (header->content, ((char*)frame->payload) + mark, iterator - mark);
	
	/* clean string */
	axl_stream_trim (header->content);
	
	/* store in the hash (only store for the first time, next ones
	 * are updates on an instance already stored)  */
	if (first_definition) {
		/* make the new different header to point to the last first */
		header->next_header = frame->mime_headers->first;
		
		/* now make the new header to be the first */
		frame->mime_headers->first = header;
	} /* end if */
	
	/* skip to the next CRLF pairs */
	if (payload[iterator] == '\x0A')
		iterator += 1;
	else
		iterator += 2;
	
	vortex_log2 (VORTEX_LEVEL_DEBUG, "Found mime header (iterator=%d) '%s' : '%s'", 
		     iterator, header->name, header->content);
	
	/* update caller iterator */
	(*caller_iterator) = iterator;

	return 1;
}

/** 
 * @internal Reconfigures frames to make payload to point to the MIME
 * body part and content to point to all message. It also reconfigures
 * mime headers size and payload size.
 */
void vortex_frame_reconfigure_mime (VortexFrame * frame, int iterator, int step)
{
	/* make frame->content to point to all content
	 * received and frame->payload to point only
	 * to the relevant user part */
	frame->content = frame->payload;
	frame->payload = (((char*)frame->content) + iterator + step);
	/* vortex_log (VORTEX_LEVEL_DEBUG, "reconfiguring mime body start at: %d ('%d'): %s",
	   iterator + step, ((char *)frame->payload)[0], frame->payload); */
	
	/* update mime headers */
	frame->mime_headers_size   = (iterator + step);
	
	/* update payload size */
	frame->size               -= (iterator + step);

	return;
}

/** 
 * @brief Function that prepares MIME status for the frame received,
 * configuring variables, content, etc.
 * 
 * @param frame The frame to be reconfigured.
 * 
 * @return axl_true if the frame processing was ok, otherwise axl_false is
 * returned. The function returns axl_false if the reference received is
 * NULL.
 */
axl_bool           vortex_frame_mime_process          (VortexFrame * frame)
{
	int         iterator;
	/* local reference to cast the frame content */
	char      * payload;
	int         step;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	/* check reference */
	v_return_val_if_fail (frame, axl_false);
#if defined(ENABLE_VORTEX_LOG)
	/* internal check */
	ctx = frame->ctx;
#endif
	if (frame->type == VORTEX_FRAME_TYPE_SEQ) {
		vortex_log (VORTEX_LEVEL_WARNING, "something is not working properly because a SEQ frame was received to reconfigure its MIME");
		return axl_false;
	}

	/* configure global variables */
	iterator = 0; 
	payload  = frame->payload;
	vortex_log (VORTEX_LEVEL_DEBUG, "frame content size=%d: '%c%c'",
		    frame->size, payload[0],payload[1]);  

	/* do not check to init mime_headers here; this is because to
	 * enable fast implementations for mime body without
	 * headers */
	
	/* check for MIME message without headers */
	if ((frame->size >= 2 && payload[0] == '\x0D' && payload[1] == '\x0A') ||
	    (frame->size == 1 && payload[0] == '\x0A')) {
		vortex_log (VORTEX_LEVEL_DEBUG, "found MIME message without headers");
		/* configure step */
		step = (payload[0] == '\x0A') ? 1 : 2;
		
		/* reconfigure frame and terminate */
		vortex_frame_reconfigure_mime (frame, iterator, step);
		return axl_true;
	}

	/* check to initialize the mime header internal hash */
	if (frame->mime_headers == NULL)
		frame->mime_headers = vortex_frame_mime_status_new ();

	/* until frame content is exhausted.. */
	while (iterator < frame->size) {

		while (axl_true) {
			/* check to terminate mime body part */
			if ((payload[iterator] == '\x0A') || 
			    (payload[iterator] == '\x0D' && payload[iterator + 1] == '\x0A'))
				break;

			/* parse next MIME header found */
			switch (vortex_frame_read_mime_header (frame, &iterator)) {
			case -1:
				/* clear all MIME headers */
				vortex_frame_mime_status_free (frame->mime_headers);
				frame->mime_headers = NULL;

				/* MIME format error found */
				return axl_false;
			case 1:
				/* MIME header found, check for finish */
				continue;
			default:
				/* no MIME header found, but no error, see next */
				break;
			} /* end switch */
		}  /* end while */

		/* check to stop processing: LF */
		if (payload [iterator]     == '\x0A') {
			vortex_log (VORTEX_LEVEL_DEBUG, "found MIME body (unix MIME terminator LF) header termination at: %d",
				    iterator);

			/* configure step and terminate: because we
			 * found only LF, step = 1 */
			step = 1;

			/* reconfigure frame and terminate */
			vortex_frame_reconfigure_mime (frame, iterator, step);
			return axl_true;
			
		}

		/* check to stop processing: CR-LF */
		if ((iterator + 1) < frame->size &&
		    payload [iterator]     == '\x0D' &&
		    payload [iterator + 1] == '\x0A') {
			vortex_log (VORTEX_LEVEL_DEBUG, "Found MIME body (CR-LF) at %d, updating internal references..",
				    iterator);
			/* configure step: if we found CR-LF or only LF */
			step = 2;

			/* reconfigure frame and terminate */
			vortex_frame_reconfigure_mime (frame, iterator, step);

			return axl_true;
		} /* end if */

		/* next iterator */
		iterator++;
	}
	
	/* unable to finish properly, clear all mime content state */ 
	vortex_log (VORTEX_LEVEL_WARNING, "unable to perform MIME parse; failed to find mime body part");

	/* clear all MIME headers */
	vortex_frame_mime_status_free (frame->mime_headers);
	frame->mime_headers = NULL;

	return axl_false;
}

/** 
 * @brief Allows to configure a new MIME header on the provided \ref
 * VortexFrame reference.
 * 
 * @param frame The frame that is going to be configured with a new
 * MIME header. If the MIME header provided is already found, it will
 * be overwritten.
 *
 * @param mime_header The mime header to configure. Use the following
 * macros for recognized MIME headers: \ref MIME_VERSION, \ref
 * MIME_CONTENT_TYPE, \ref MIME_CONTENT_TRANSFER_ENCODING. See \ref
 * vortex_frame_get_mime_header for a full list.  In the case a NULL
 * parameter is received, it is considered as a request to remove the
 * header (only the first reference is removed).
 *
 * @param mime_header_content The mime header content to be
 * configured. The function will copy the content provided. The
 * function fail if mime_header or frame parameter are null. 
 *
 * <i><b>NOTE:</b>MIME (RFC 2045) allows to store some headers several
 * times. Because BEEP only requires to support MIME structure, it
 * must support storing several MIME header declarations (with same
 * header name). </i>.
 */
void          vortex_frame_set_mime_header       (VortexFrame * frame,
						  const char  * mime_header,
						  const char  * mime_header_content)
{
	int                size = 0;
	int                length;
	VortexMimeHeader * header;
	VortexMimeHeader * header_aux;

	v_return_if_fail (frame);
	v_return_if_fail (mime_header);

	/* check if the mime header hash is created */
	if (frame->mime_headers == NULL)
		frame->mime_headers = vortex_frame_mime_status_new ();
	
	/* check for remove header request */
	if (mime_header_content == NULL) {
		header = vortex_frame_mime_find (frame->mime_headers, mime_header);
		if (header != NULL) {
			/* check mime header size to remove */
			size += strlen (mime_header);
			size += strlen (header->content) + 2;

			/* remove the header */
			header_aux       = header;
			header           = header->next;
			header_aux->next = NULL;

			/* we can't remove because memory is handled
			 * through a factory */

			/* update */
			frame->mime_headers_size -= size;

			/* in the case we removed the last mime
			 * header, remove all content */
			if (frame->mime_headers_size == 2)
				frame->mime_headers_size = 0;
		} /* end if */
		return;
	}

	/* check if the header was previosly stored */
	header = vortex_frame_mime_find (frame->mime_headers, mime_header);
	if (header == NULL) {
		/* get memory for the header node */
		header          = axl_factory_get (frame->mime_headers->header_factory);

		/* because it is not found, add it */
		header->next_header        = frame->mime_headers->first;
		frame->mime_headers->first = header;
	} else {
		/* go the last header defined */
		while (header->next != NULL)
			header = header->next;

		/* create the header and the the header name */
		header->next        = axl_factory_get (frame->mime_headers->header_factory);
		
		/* go to the next header and configure the content */
		header              = header->next;
	} /* end if */

	/* alloc enough memory to hold content-type */
	length          = strlen (mime_header);
	header->name    = axl_string_factory_alloc (frame->mime_headers->mime_headers_name, length + 1);
	memcpy (header->name, mime_header, length);
	
	/* allow enough memory to hold content-transfer-encoding */
	length          = strlen (mime_header_content);
	header->content = axl_string_factory_alloc (frame->mime_headers->mime_headers_content, length + 1);
	memcpy (header->content, mime_header_content, length);

	/* check to update references */
	vortex_frame_mime_check_and_update_fast_ref (frame->mime_headers, header);

	/* update mime header size */
	size += strlen (mime_header);
	size += strlen (mime_header_content) + 2;
	if (frame->mime_headers_size == 0)
		size += 2;

	/* set calculated value */
	frame->mime_headers_size += size;

	return;
}


/** 
 * @brief Allows to get the content associated to a particular header.
 * 
 * @param frame The frame where the MIME header content will be
 * retrieved.
 *
 * @param mime_header The MIME header to get. See supported MIME headers. 
 * 
 * @return A reference to the content of the MIME header or NULL if it
 * fails or nothing is found. The function can only fail if some
 * parameter provided is NULL.
 * 
 * <b>Supported MIME headers</b>: 
 * 
 * The following are a set of macros that can be used to retrieve
 * common MIME headers. You can still use direct string, but using
 * them will disable compilation protection. For example:
 * 
 * \code
 * // get content for MIME header "Content-Type"
 * VortexMimeHeader * header = vortex_frame_get_mime_header (frame, MIME_CONTENT_TYPE);
 *
 * // now get the content 
 * const char       * value  = vortex_frame_mime_header_content (header);
 *
 * // for other MIME header that have allowed repeatable value values use next
 * header = vortex_frame_mime_header_next (header);
 *
 * // to simply get the first header do (returns first reference):
 * value  = VORTEX_FRAME_GET_MIME_HEADER (frame, MIME_CONTENT_TYPE);
 *
 * \endcode
 *
 * - \ref MIME_VERSION
 * - \ref MIME_CONTENT_TYPE
 * - \ref MIME_CONTENT_TRANSFER_ENCODING
 * - \ref MIME_CONTENT_ID
 * - \ref MIME_CONTENT_DESCRIPTION
 *
 * <i><b>NOTE:</b> There is an especial note about the "Content-Type" and
 * "Content-Transfer-Encoding". If they aren't defined, it is assumed
 * the following default values (RFC 3080, section 2.2):
 * 
 * - "Content-Type" is "application/octet-stream"
 * - "Content-Transfer-Encoding" is "binary"
 *
 * In the case you don't want to receive NULL values and assume
 * previous values, you must use \ref vortex_frame_get_content_type
 * and \ref vortex_frame_get_transfer_encoding, which returns the
 * appropiate values if they found the MIME status no holding such
 * values.</i>
 *
 */
VortexMimeHeader *  vortex_frame_get_mime_header       (VortexFrame * frame,
							const char  * mime_header)
{
	VortexMimeHeader * header;
	v_return_val_if_fail (frame,       NULL);
	v_return_val_if_fail (mime_header, NULL);

	/* check basic case where the hash wasn't created */
	if (frame->mime_headers == NULL)
		return NULL;

	/* return content */
	header = vortex_frame_mime_find (frame->mime_headers, mime_header);
	return header;
}

/** 
 * @brief Provided a reference to a MIME header \ref VortexMimeHeader,
 * the function allows to get the MIME header field name.
 * 
 * @param header The header where the operation will be performed.
 * 
 * @return A reference to the MIME header field name or NULL if it
 * fails. The function can only fail if the header reference received
 * is NULL.
 */
const char       * vortex_frame_mime_header_name       (VortexMimeHeader * header)
{
	v_return_val_if_fail (header, NULL);
	/* return header name */
	return header->name;
}

/** 
 * @brief Provided a reference to a MIME header \ref VortexMimeHeader,
 * the function allows to get the MIME header content.
 * 
 * @param header The header where the operation will be performed.
 * 
 * @return A reference to the MIME header content or NULL if it
 * fails. The function can only fail if the header reference received
 * is NULL.
 */
const char       * vortex_frame_mime_header_content    (VortexMimeHeader * header)
{
	v_return_val_if_fail (header, NULL);
	/* return header name */
	return header->content;
}

/** 
 * @brief Provided a reference to a MIME header \ref VortexMimeHeader,
 * the function allows to get the next MIME header found on the \ref
 * VortexFrame.
 *
 * Example: assuming you want to traverse all "Received" MIME header
 * use:
 * \code
 * // get first header 
 * VortexMimeHeader * header = vortex_frame_get_mime_header (frame, "Received");
 * 
 * // foreach header found 
 * do {
 *      printf ("MIME Header found %s : %s\n", 
 *              vortex_frame_mime_header_name (header),
 *              vortex_frame_mime_header_content (header));
 *
 *      // get next header 
 *      header = vortex_frame_mime_header_next (header);
 * } while (header != NULL);
 * \endcode
 * 
 * @param header The header where the operation will be performed.
 * 
 * @return A reference to the next MIME header or NULL if it
 * fails. The function can only fail if the header reference received
 * is NULL.
 */
VortexMimeHeader * vortex_frame_mime_header_next       (VortexMimeHeader * header)
{
	v_return_val_if_fail (header, NULL);

	/* return header name */
	return header->next;
}

/** 
 * @brief Allows to get the number of times the MIME header was
 * defined or the number of (times - 1) a call to \ref
 * vortex_frame_mime_header_next will succeed.
 * 
 * @param header The MIME header to check.
 * 
 * @return The number of times the MIME header was found in the
 * message. The function returns -1 if the reference received is NULL.
 */
int                vortex_frame_mime_header_count      (VortexMimeHeader * header)
{
	int                count = 1;
	VortexMimeHeader * header_aux;
	v_return_val_if_fail (header, -1);

	/* point to the received header */
	header_aux = header;
	while (header_aux->next) {
		count++;
		header_aux = header_aux->next;
	}

	return count;
}

/** 
 * @internal Function that allows to check if the MIME internal status
 * is activated, or default headers must be used.
 * 
 * @param frame The frame where the request will be handled.
 * 
 * @return axl_true if the mime status is activated, otherwise axl_false is
 * returned.
 */
axl_bool                vortex_frame_mime_status_is_available  (VortexFrame * frame)
{
	v_return_val_if_fail (frame, axl_false);
	/* return a reference check */
	return (frame->mime_headers != NULL);
}

/* @} */

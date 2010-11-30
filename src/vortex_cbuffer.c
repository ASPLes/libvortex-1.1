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

#include <vortex_cbuffer.h>

struct _VortexCBuffer {
	char         * buffer;
	int            buffer_size;
	int            first_byte_available;
	int            last_byte_available;
	VortexMutex    mutex;
	VortexCond     cond;
};

/** 
 * @brief Allows to create a circular byte buffer with the provided
 * byte capacity.
 *
 * Once created the buffer you must use \ref vortex_cbuffer_put to
 * place content into the buffer and \ref vortex_cbuffer_get to
 * retrieve content. 
 *
 * You can use \ref vortex_cbuffer_is_empty to know if the buffer
 * holds content to be retrieved or \ref
 * vortex_cbuffer_available_bytes to get the number of bytes that can
 * be retrieved.
 *
 * You can call to \ref vortex_cbuffer_size to get total buffer
 * capacity (without considering how many bytes are stored).
 *
 * @param buffer_size The amount of byte capacity that will be able to
 * hold this circular buffer. It must be > 0.
 *
 * @return A reference to the object created or NULL if it fails
 * (memory allocation failure).
 */
VortexCBuffer * vortex_cbuffer_new           (int buffer_size)
{
	VortexCBuffer * buffer;

	if (buffer_size <= 0)
		return NULL;

	/* create the buffer and check result */
	buffer = axl_new (VortexCBuffer, 1);
	if (buffer == NULL)
		return NULL;
	/* create internal storage */
	buffer->buffer = axl_new (char, buffer_size + 1);
	if (buffer->buffer == NULL) {
		axl_free (buffer);
		return NULL;
	} /* end if */

	/* store sizes */
	buffer->buffer_size = buffer_size;

	/* init mutex and conditional var */
	vortex_mutex_create (&buffer->mutex);
	vortex_cond_create  (&buffer->cond);

	return buffer;
}

/** 
 * @brief Allows to check if the provided buffer has content.
 *
 * If the buffer has content, a call to \ref vortex_cbuffer_get will
 * succeed.
 *
 * @param buffer The buffer where the operation will be implemented.
 *
 * @return axl_true if the buffer is empty, otherwise axl_false is
 * returned. The function also returns axl_false in case of NULL
 * pointer received.
 */
axl_bool        vortex_cbuffer_is_empty      (VortexCBuffer * buffer)
{
	if (buffer == NULL)
		return axl_false;
	/* if both indexes are equal no data is available */
	return buffer->last_byte_available == buffer->first_byte_available;
}

/** 
 * @brief Allows to get the current buffer maximum capacity.
 *
 * @param buffer The buffer capacity.
 *
 * @return The total capacity or -1 if it fails. The function also
 * returns -1 if NULL pointer is received.
 */
int             vortex_cbuffer_size            (VortexCBuffer * buffer)
{
	if (buffer == NULL)
		return -1;
	/* current size */
	return buffer->buffer_size;
}

/** 
 * @brief Allows to get the amount of bytes avaialble from the buffer.
 *
 * The function returns the amount of bytes that can be returned by a
 * call to \ref vortex_cbuffer_get.
 *
 * @param buffer The buffer where to get available bytes.
 *
 * @return The amount of bytes available or -1 it if fails. The
 * function also returns -1 if NULL pointer is received.
 */
int             vortex_cbuffer_available_bytes (VortexCBuffer * buffer)
{
	if (buffer == NULL)
		return -1;

	/* if first is small than last */
	if (buffer->first_byte_available < buffer->last_byte_available)
		return buffer->last_byte_available - buffer->first_byte_available + 1;

	/* if last is small that first */
	return (buffer->buffer_size - buffer->first_byte_available) + buffer->last_byte_available + 1;
}

/** 
 * @brief Allows to get content from the buffer.
 *
 * The function allows to put the provided data with the provided size
 * on the provided buffer. Optionally the funtion returns the number
 * of bytes that were written into the buffer. 
 *
 * If the caller sets satisfy to axl_true the function will block the
 * caller until all the data can be pushed into the buffer. 
 *
 * Keep in mind that pushing into the buffer more content than the
 * buffer is able to handle and setting satisfy to axl_true will cause
 * to always fail (the operation can't be completed).
 *
 * @param buffer The buffer where the content will be placed.
 *
 * @param data The data pointer that holds the raw byte content to be
 * transfered into the buffer.
 *
 * @param data_size The amount of bytes to be placed.
 *
 * @param satisfy If the caller must be blocked until the operation is
 * fully satisfied.
 *
 * @return The amount of bytes written into the buffer or -1 it if
 * fails. The function will fail if the buffer reference is NULL or
 * data is NULL or satisfy is axl_true and data_size is bigger than
 * total buffer capacity.
 */
int             vortex_cbuffer_put           (VortexCBuffer * buffer, 
					      const char    * data, 
					      int             data_size,
					      axl_bool        satisfy)
{
	int iterator;
	int bytes_written;

	if (buffer == NULL || data == NULL)
		return -1;
		
	if (satisfy && buffer->buffer_size < data_size)
		return -1;

	/* acquire the mutex */
	vortex_mutex_lock (&buffer->mutex);

	/* set bytes written */
	bytes_written = 0;

transfer_content:

	/* start with the last byte available */
	iterator      = buffer->last_byte_available + 1;
	while (iterator < buffer->buffer_size && bytes_written < data_size) {
		/* copy content */
		buffer->buffer[iterator] = data[bytes_written];

		/* next position */
		iterator++;
		bytes_written++;
	} /* end while */

	/* check if we have completed operation */
	if (bytes_written == data_size) {
		/* update last byte position */
		buffer->last_byte_available = (iterator - 1);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	}

	/* if reached this point, it seems we have more content to
	 * write still pending */
	iterator = 0;
	while (iterator < buffer->first_byte_available && bytes_written < data_size) {
		/* copy content */
		buffer->buffer[iterator] = data[bytes_written];

		/* next position */
		iterator++;
		bytes_written++;
	}

	/* check if we have completed operation */
	if (bytes_written == data_size) {
		/* update last byte position */
		buffer->last_byte_available = (iterator - 1);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	}

	/* reached this point, it seems there are still some bytes
	 * pending to be written, check satisfy request */
	if (! satisfy) {
		/* update last byte position */
		buffer->last_byte_available = (iterator - 1);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	} /* end if */

	/* reached this point we have pending bytes to be written and
	 * satisfy enabled, lock the caller */
	while (buffer->first_byte_available == buffer->last_byte_available)
		VORTEX_COND_WAIT (&buffer->cond, &buffer->mutex);
	
	/* unlocked, restart transfer */
	goto transfer_content;
	/* this is never reached */
}

/** 
 * @brief Allows to get content into the provided buffer from the
 * circular buffer.
 *
 * @param buffer The circular buffer that will be used to retrieve
 * content.
 *
 * @param data The buffer where the content will be stored.
 *
 * @param data_size The size of the buffer where to store the result
 * and at the same time the total amount of bytes requested to be
 * retrieved from the circular buffer.
 *
 * @param satisfy If set to axl_true, the caller will be blocked until
 * all the content requested is satisfied.
 *
 * @return The function returns the amount of bytes that were
 * transferred from the buffer or -1 it it fails. The function will
 * fail if the buffer reference is NULL or data buffer is NULL or
 * requested data_size is 0 or negative.
 */ 
int             vortex_cbuffer_get           (VortexCBuffer  * buffer, 
					      char           * data, 
					      int            * data_size,
					      axl_bool         satisfy)
{
	int iterator;

	if (buffer == NULL || data == NULL || data_size <= 0) 
		return -1;

	/* acquire the lock */
	vortex_mutex_lock (&buffer->mutex);

	/* transfer content into the caller buffer */
	iterator = buffer->first_byte_available;

	return -1;
}

/** 
 * @brief Terminates the provided circular buffer.
 * @param buffer The buffer to be finished.
 */
void            vortex_cbuffer_free          (VortexCBuffer * buffer)
{
	/* check pointer received */
	if (buffer == NULL)
		return;

	/* free internal buffer */
	axl_free (buffer->buffer);
	buffer->buffer = NULL;

	/* terminate mutex */
	vortex_mutex_destroy (&buffer->mutex);	
	vortex_cond_destroy (&buffer->cond);

	axl_free (buffer);
	return;
}




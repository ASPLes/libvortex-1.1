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
	int            ref_count;
	int            buffer_size;
	int            first_byte_available;
	int            last_byte_available;

	/* read mutex */
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
 * To terminate the circular buffer created use \ref
 * vortex_cbuffer_unref.
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

	/* set initial state for index */
	buffer->last_byte_available  = -1;
	buffer->first_byte_available = -1;
	
	/* set initial ref count state */
	buffer->ref_count = 1;

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
axl_bool        vortex_cbuffer_is_empty      (VortexCBuffer * buffer, axl_bool lock)
{
	/* call to bytes available implementation */
	return vortex_cbuffer_available_bytes (buffer, lock) == 0;
}

/** 
 * @brief Allows to get the current buffer maximum capacity.
 *
 * @param buffer The buffer capacity.
 *
 * @return The total capacity or -1 if it fails. The function also
 * returns -1 if NULL pointer is received.
 */
int             vortex_cbuffer_size            (VortexCBuffer * buffer, axl_bool lock)
{
	int size;
	if (buffer == NULL)
		return -1;

	/* lock the mutex */
	if (lock)
		vortex_mutex_lock (&buffer->mutex);

	/* current size */
	size = buffer->buffer_size;

	/* release the mutex */
	if (lock)
		vortex_mutex_unlock (&buffer->mutex);

	return size;
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
int             vortex_cbuffer_available_bytes (VortexCBuffer * buffer, axl_bool lock)
{
	int result;

	if (buffer == NULL)
		return -1;

	/* check where no content is found */
	if (buffer->first_byte_available == -1)
		return 0;

	/* lock the mutex */
	if (lock)
		vortex_mutex_lock (&buffer->mutex);

	/* if first is small than last */
	if (buffer->first_byte_available < buffer->last_byte_available) {
		result = buffer->last_byte_available - buffer->first_byte_available + 1;
	} else if (buffer->first_byte_available > buffer->last_byte_available) {
		/* if last is small that first */
		result = (buffer->buffer_size - buffer->first_byte_available) + buffer->last_byte_available + 1;
	} else {
		/* first and last are equal */
		result = 1;
	}
	/* release the mutex */
	if (lock)
		vortex_mutex_unlock (&buffer->mutex);

	return result;
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
 * the caller to be blocked until someone reads enough information to
 * unlock the caller (\ref vortex_cbuffer_get).
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
	int      iterator;
	int      bytes_written;
	int      available_space;

	if (buffer == NULL || data == NULL)
		return -1;
		
	/* acquire the mutex */
	vortex_mutex_lock (&buffer->mutex);

	/* set bytes written */
	bytes_written = 0;

transfer_content:

	/* printf ("W:(1) Attempting to write into the buffer first: %d, last: %d, buffer size: %d, available data: %d, requested: %d, bytes written: %d\n",
		buffer->first_byte_available, buffer->last_byte_available, buffer->buffer_size, vortex_cbuffer_available_bytes (buffer, axl_false),
		data_size, bytes_written);  */

	/* get available espace */
	available_space = buffer->buffer_size - vortex_cbuffer_available_bytes (buffer, axl_false);

	/* check if we have to initialize first byte indicator */
	if (buffer->first_byte_available == -1)
		buffer->first_byte_available = 0;

	/* start with the last byte available */
	iterator      = buffer->last_byte_available + 1;
	while (iterator < buffer->buffer_size && bytes_written < data_size && (available_space > 0)) {
		/* copy content */
		buffer->buffer[iterator] = data[bytes_written];

		/* update last byte position */
		buffer->last_byte_available = iterator;

		/* next position */
		iterator++;
		bytes_written++;
		available_space--;
	} /* end while */

	/* printf ("  W:(2) Bytes written %d, buffer size: %d, requested %d (first: %d, last: %d)\n", 
		bytes_written, buffer->buffer_size, data_size, 
		buffer->first_byte_available, buffer->last_byte_available);   */

	/* check if we have completed operation */
	if (bytes_written == data_size) {

		/* broadcast */
		vortex_cond_broadcast  (&buffer->cond);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	}

	/* get available espace */
	available_space = buffer->buffer_size - vortex_cbuffer_available_bytes (buffer, axl_false);

	/* printf ("  W:(3) Attempting to write at the buffer start, until now bytes written %d, buffer size: %d, requested %d (first: %d, last: %d, espace available: %d)\n", 
		bytes_written, buffer->buffer_size, data_size, 
		buffer->first_byte_available, buffer->last_byte_available, available_space);  */

	/* if reached this point, it seems we have more content to
	 * write still pending */
	iterator = 0;
	while (iterator < buffer->first_byte_available && bytes_written < data_size && available_space > 0) {
		/* copy content */
		buffer->buffer[iterator] = data[bytes_written];

		/* update last byte position */
		buffer->last_byte_available = iterator;

		/* next position */
		iterator++;
		bytes_written++;
		available_space--;
	}

	/* printf ("  W:(4) after trying to write at the buffer start, until now bytes written %d, buffer size: %d, requested %d (first: %d, last: %d)\n", 
		bytes_written, buffer->buffer_size, data_size, 
		buffer->first_byte_available, buffer->last_byte_available);   */

	/* check if we have completed operation */
	if (bytes_written == data_size) {
		/* broadcast */
		vortex_cond_broadcast  (&buffer->cond);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	}

	/* reached this point, it seems there are still some bytes
	 * pending to be written, check satisfy request */
	if (! satisfy) {
		/* broadcast */
		vortex_cond_broadcast  (&buffer->cond);

		/* release the mutex */
		vortex_mutex_unlock (&buffer->mutex);
		return bytes_written;
	} /* end if */


	/* reached this point we have pending bytes to be written and
	 * satisfy enabled, lock the caller */
	if (vortex_cbuffer_available_bytes (buffer, axl_false) == buffer->buffer_size) {
		/* awake threads pending */
		vortex_cond_broadcast (&buffer->cond);
		do {
			/* fflush (stdout);
			   printf ("   W:(5) Buffer is full, %d bytes, but pending bytes are %d, waiting\n", 
			   vortex_cbuffer_available_bytes (buffer, axl_false), data_size - bytes_written);   */
			VORTEX_COND_WAIT (&buffer->cond, &buffer->mutex);
			/* printf ("   W:(6)   unlocked, buffer status i, %d bytes, but pending bytes are %d, waiting\n", 
			   vortex_cbuffer_available_bytes (buffer, axl_false), data_size - bytes_written);    */
		} while (vortex_cbuffer_available_bytes (buffer, axl_false) == buffer->buffer_size);
	}
	
	/* unlocked, restart transfer */
	/* printf ("     W:(6)  Ready to transfer, available space: %d\n", buffer->buffer_size - vortex_cbuffer_available_bytes (buffer, axl_false) );   */
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
					      int              data_size,
					      axl_bool         satisfy)
{
	int iterator;
	int index;
	int available;
	int served;
	int limit;


	if (buffer == NULL || data == NULL || data_size <= 0) 
		return -1;

	/* acquire the lock */
	vortex_mutex_lock (&buffer->mutex);

	/* configure index */
	index    = 0;

	/* check if the buffer is empty to block */
	if (vortex_cbuffer_is_empty (buffer, axl_false)) {
		/* if satisfy is not configured, return */
		if (! satisfy) 
			return 0;

		/* lock while this condition is meet */
		while (vortex_cbuffer_available_bytes (buffer, axl_false) == 0) {
			VORTEX_COND_WAIT (&buffer->cond, &buffer->mutex);
		}
	} /* end if */

	/* continue with the transfer */
do_transfer:

	/* transfer content into the caller buffer */
	iterator  = buffer->first_byte_available;
	available = vortex_cbuffer_available_bytes (buffer, axl_false);
	served    = 0;
	/* printf ("R:(1) First byte available = %d, last = %d (requested bytes: %d, available: %d, served %d)\n", 
	   iterator, buffer->last_byte_available, data_size, available, index);    */

	/* copy the first part available (from first to last or the
	 * end of the buffer):
               1) Where first comes before last or first == last
    	       -  [         #####    ]
                            ^   ^
                            F   L
               2) Where first comes after last (circular)
    	       -  [###        #######]
                     ^        ^
                     L        F
	 */
	if (buffer->first_byte_available <= buffer->last_byte_available)
		limit = buffer->last_byte_available;
	else
		limit = buffer->buffer_size - 1;
	while (iterator <= limit && index < data_size) {
		
		/* copy content */
		data[index] = buffer->buffer[iterator];
		/* printf ("  R: Copy: %d (served %d)\n", (int)data[index], served + 1);  */
		
		/* update indexes */
		index++;
		iterator++;
		served++;

		/* flag new first position */
		buffer->first_byte_available = iterator;
	} /* end while */

	/* update first byte available to iterator or 0 in the case
	 * end of the buffer was reached */
	if (served == available) {
		buffer->first_byte_available = -1;
		buffer->last_byte_available  = -1;
	} else if (iterator == buffer->buffer_size) {
		buffer->first_byte_available = 0;
	}

	/* printf ("   R:(2) after first copy round status is served: %d (first %d, last %d)\n", 
	   served, buffer->first_byte_available, buffer->last_byte_available);    */

	if (buffer->first_byte_available >= 0) {

		/* now transfer second part */
		iterator = buffer->first_byte_available;
		while (iterator <= buffer->last_byte_available &&
		       index < data_size) {
			/* copy content */
			data[index] = buffer->buffer[iterator];
			/* printf ("  R: Copy(2): %d (served %d)\n", (int)data[index], served + 1);   */
			
			/* update indexes */
			index++;
			iterator++;
			served++;

			/* always first byte points to iterator */
			buffer->first_byte_available = iterator;

		} /* end while */

	} /* end if */

	/* reset buffer status in the case no more content is found */
	if (available == served) {
		buffer->last_byte_available  = -1;
		buffer->first_byte_available = -1;
	} /* end if */

	/* check if we have finished either because all content
	 * requested was transferred or because the buffer has no more
	 * content */
	if (index == data_size) {
		/* printf ("     R:(3) Finishing: first %d, last: %d, iterator: %d, index: %d, data_size: %d\n",
			buffer->first_byte_available, buffer->last_byte_available, iterator, index, data_size);  
			fflush (stdout);  */

		/* broadcast */
		vortex_cond_broadcast  (&buffer->cond);

		/* acquire the lock */
		vortex_mutex_unlock (&buffer->mutex);
		return index;
	} /* end if */

	if (vortex_cbuffer_is_empty (buffer, axl_false) && ! satisfy) {
		/* printf ("    R:(4) Finishing: first %d, last: %d, iterator: %d, index: %d, data_size: %d\n",
			buffer->first_byte_available, buffer->last_byte_available, iterator, index, data_size);  
			fflush (stdout); */

		/* broadcast */
		vortex_cond_broadcast  (&buffer->cond);

		/* acquire the lock */
		vortex_mutex_unlock (&buffer->mutex);
		return index;
	} /* end if */

	/* printf ("      R:(5) index is = %d, data_size = %d, bytes available %d\n", index, data_size, vortex_cbuffer_available_bytes (buffer, axl_false));  
	   fflush (stdout);  */
	
	/* reached this point no more space is available on
	 * the buffer but satisfy is axl_true */
	if (vortex_cbuffer_available_bytes (buffer, axl_false) == 0) {
		/* awake threads pending */
		vortex_cond_broadcast (&buffer->cond);
		
		do {
			/* printf ("      R:(6) no more bytes available, waiting until buffer has more, read %d, requested %d\n",
				served, data_size);  
				fflush (stdout);  */
			VORTEX_COND_WAIT (&buffer->cond, &buffer->mutex);
		} while (vortex_cbuffer_available_bytes (buffer, axl_false) == 0);
	} /* end if */

	/* do the content transfer */
	goto do_transfer;
}

/** 
 * @brief Allows to increase reference counting to the provided
 * structure.
 *
 * @buffer The buffer structure to acquire a reference.
 *
 * @return axl_true if the reference was acquired, otherwise axl_false
 * is returned. To release the reference use \ref vortex_cbuffer_unref
 */
axl_bool        vortex_cbuffer_ref            (VortexCBuffer * buffer)
{
	/* check pointer received */
	if (buffer == NULL || buffer->ref_count == 0 || buffer->buffer == NULL)
		return axl_false;

	/* lock and reduce */
	vortex_mutex_lock (&buffer->mutex);
	buffer->ref_count++;
	vortex_mutex_unlock (&buffer->mutex);
	return axl_true;

}

/** 
 * @brief Terminates the provided circular buffer.
 * @param buffer The buffer to be finished.
 */
void            vortex_cbuffer_unref          (VortexCBuffer * buffer)
{
	/* check pointer received */
	if (buffer == NULL)
		return;

	/* lock and reduce */
	vortex_mutex_lock (&buffer->mutex);
	buffer->ref_count--;
	if (buffer->ref_count != 0) {
		vortex_mutex_unlock (&buffer->mutex);
		return;
	} /* end if */

	/* free internal buffer */
	axl_free (buffer->buffer);
	buffer->buffer = NULL;

	/* terminate mutex */
	vortex_mutex_destroy (&buffer->mutex);	
	vortex_cond_destroy (&buffer->cond);

	axl_free (buffer);
	return;
}




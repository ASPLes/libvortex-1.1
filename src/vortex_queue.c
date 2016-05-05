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

#define LOG_DOMAIN "vortex-queue"

/** 
 * @internal Queue implementation based on a mutex and a wrapper
 * interface around axlList.
 */
struct _VortexQueue {
	/** 
	 * Mutex to make the API thread safe around the type.
	 */
	VortexMutex   mutex;
	/** 
	 * List used to store data, behaving as a queue.
	 */
	axlList     * queue;
};

/**
 * \defgroup vortex_queue Vortex Queue: thread safe queue definition based used across Vortex Library
 */

/**
 * \addtogroup vortex_queue
 * @{
 */

/**
 * @brief Creates a new \ref VortexQueue object.
 *
 * Creates a thread safe queue, that can be checked and used by
 * serveral threads at the same time.
 * 
 * @return a new \ref VortexQueue object.
 **/
VortexQueue * vortex_queue_new           (void)
{
	VortexQueue * result;

	result        = axl_new (VortexQueue, 1);
	vortex_mutex_create (&result->mutex);
	result->queue = axl_list_new (axl_list_always_return_1, NULL);

	return result;
}

/**
 * @brief Returns if the given queue is empty.
 *
 * @param queue The queue to check.
 * 
 * @return axl_true if the \ref VortexQueue is empty, axl_false if not.
 **/
axl_bool           vortex_queue_is_empty      (VortexQueue * queue)
{
	axl_bool      result;

	/* check parameter */
	if (queue == NULL)
		return axl_false;

	vortex_mutex_lock (&queue->mutex);

	result = axl_list_is_empty (queue->queue);

	vortex_mutex_unlock (&queue->mutex);

	return result;
	
}

/**
 * @brief Returns how many items the given queue have.
 * 
 * @param queue the queue to check.
 *
 * @return Returns how many items the given queue have or -1 if it fails.
 **/
unsigned int   vortex_queue_get_length    (VortexQueue * queue)
{
	unsigned int result;

	/* check parameter */
	if (queue == NULL)
		return -1;

	vortex_mutex_lock (&queue->mutex);

	result = axl_list_length (queue->queue);

	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/**
 * @brief Queues new data inside the given queue. 
 * 
 * push data at the queue's tail.
 *
 * @param queue the queue to use.
 * @param data user defined data to queue.
 * 
 * @return axl_true if the data was queue, axl_false if not
 **/
axl_bool           vortex_queue_push          (VortexQueue * queue, axlPointer data)
{

	/* check parameter */
	if (queue == NULL || data == NULL)
		return axl_false;

	vortex_mutex_lock   (&queue->mutex);

	/* place the data into the last position. The head queue is
	 * the first element to be poped, and the tail is where new
	 * data is placed. */
	axl_list_append (queue->queue, data);
	
	vortex_mutex_unlock (&queue->mutex);

	return axl_true;
}

/**
 * @brief Queues new data inside the given queue at the header, that
 * is, at the very next to be popped
 * 
 * Push data at the queue's head
 *
 * @param queue the queue to use.
 * @param data user defined data to queue.
 * 
 * @return axl_true if the data was queue, axl_false if not
 **/
axl_bool           vortex_queue_head_push    (VortexQueue * queue, axlPointer data)
{
	/* check parameter */
	if (queue == NULL || data == NULL)
		return axl_false;

	vortex_mutex_lock   (&queue->mutex);
	
	/* place data at the head of the queue, so it is retreived on
	 * the next pop operation. */
	axl_list_prepend (queue->queue, data);

	vortex_mutex_unlock (&queue->mutex);

	return axl_true;
}

/**
 * @brief Extracts queue data from its header.
 * 
 * pop data at the queue's head.
 *
 * @param queue the queue to operate.
 * 
 * @return the next item on the Queue's header of NULL if fails.
 **/
axlPointer      vortex_queue_pop           (VortexQueue * queue)
{
	axlPointer result;

	/* check parameter */
	if (queue == NULL)
		return NULL;

	vortex_mutex_lock   (&queue->mutex);
	
	/* extract the first element from que queue */
	result = axl_list_get_first (queue->queue);
	axl_list_remove_first (queue->queue);

	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/**
 * @brief Returns a reference without popping the item from the queue.
 * 
 * Returns next value to be popped or NULL if no data is found.
 *
 * @param queue the queue to operate
 * 
 * @return a reference for the item on the Queue's header or NULL if fails.
 **/
axlPointer      vortex_queue_peek          (VortexQueue * queue)
{
	axlPointer result;

	/* check parameter */
	if (queue == NULL)
		return NULL;

	vortex_mutex_lock   (&queue->mutex);
	
	/* extract without remove */
	result = axl_list_get_first (queue->queue);

	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/**
 * @brief Frees the \ref VortexQueue.
 *
 * @param queue the queue to free.
 * 
 **/
void          vortex_queue_free          (VortexQueue * queue)
{
	/* check parameter */
	if (queue == NULL)
		return;

	if (vortex_queue_get_length (queue) != 0) {
		return;
	} /* end if */

	vortex_mutex_destroy (&queue->mutex);
	axl_list_free        (queue->queue);
	axl_free             (queue);

	return;
}

/* @} */

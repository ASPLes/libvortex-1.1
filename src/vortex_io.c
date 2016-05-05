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

#define LOG_DOMAIN "vortex-io"

/**
 * \defgroup vortex_io Vortex IO: Vortex Library IO abstraction layer 
 */

/** 
 * @internal Macro that allows to check the kind of operation that is
 * being required.
 */
#define VORTEX_IO_IS(data,op) ((data & op) == op)

/** 
 * \addtogroup vortex_io
 * @{
 */

typedef struct _VortexSelect {
	VortexCtx          * ctx;
	fd_set               set;
	int                  length;
	VortexIoWaitingFor   wait_to;
}VortexSelect;

/** 
 * @internal
 *
 * @brief Internal vortex implementation to create a compatible
 * "select" IO call fd set reference.
 *
 * @return A newly allocated fd_set reference.
 */
axlPointer __vortex_io_waiting_default_create (VortexCtx * ctx, VortexIoWaitingFor wait_to) 
{
	VortexSelect * select = axl_new (VortexSelect, 1);

	/* set default behaviour expected for the set */
	select->wait_to       = wait_to;
	select->ctx           = ctx;
	
	/* clear the set */
	FD_ZERO (&(select->set));

	return select;
}

/** 
 * @internal
 *
 * @brief Internal vortex implementation to destroy the "select" IO
 * call created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    __vortex_io_waiting_default_destroy (axlPointer fd_group)
{
	fd_set * __fd_set = fd_group;

	/* release memory allocated */
	axl_free (__fd_set);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * @brief Internal vortex implementation to clear the "select" IO
 * call created by the default create.
 * 
 * @param fd_group The fd group to be deallocated.
 */
void    __vortex_io_waiting_default_clear (axlPointer __fd_group)
{
	VortexSelect * select = __fd_group;

	/* clear the fd set */
	select->length = 0;
	FD_ZERO (&(select->set));

	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * @brief Default internal implementation for the wait operation to
 * change its status at least one socket description inside the fd set
 * provided.
 * 
 * @param __fd_group The fd set having all sockets to be watched.
 * @param wait_to The operation requested.
 * 
 * @return The error code as described by \ref vortex_io_waiting_set_wait_on_fd_group.
 */
int __vortex_io_waiting_default_wait_on (axlPointer __fd_group, int max_fds, VortexIoWaitingFor wait_to)
{
	int                result = -1;
	struct timeval     tv;
	VortexSelect     * _select = __fd_group;

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VORTEX_IO_IS (wait_to, READ_OPERATIONS)) {
		/* init wait */
		tv.tv_sec    = 0;
		tv.tv_usec   = 500000;
		result       = select (max_fds + 1, &(_select->set), NULL,   NULL, &tv);
	} else if (VORTEX_IO_IS (wait_to, WRITE_OPERATIONS)) {
		tv.tv_sec    = 1;
		tv.tv_usec   = 0;
		result       = select (max_fds + 1, NULL, &(_select->set), NULL, &tv);
	}
	
	/* check result */
	if ((result == VORTEX_SOCKET_ERROR) && (errno == VORTEX_EINTR))
		return -1;
	
	return result;
}

/** 
 * @internal
 *
 * @brief Default Vortex Library implementation for the "add to" on fd
 * set operation.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __vortex_io_waiting_default_add_to (int                fds, 
					      VortexConnection * connection,
					      axlPointer         __fd_set)
{
	VortexSelect * select = (VortexSelect *) __fd_set;
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx    * ctx    = vortex_connection_get_ctx (connection);
#endif

#if defined(AXL_OS_UNIX)
	/* disable the following check on windows because it doesn't
	 * reuse socket descriptors, using values that are higher than
	 * VORTEX_FD_SETSIZE (actuall FD_SETSIZE) */
	if (fds >= VORTEX_FD_SETSIZE) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "reached max amount of descriptors for socket set based on select(2). See VORTEX_FD_SETSIZE=%d, fds=%d, or other I/O mechanism, closing connection.", VORTEX_FD_SETSIZE, fds);
		return axl_false;
	} /* end if */
#endif

	if (fds < 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL,
			    "received a non valid socket (%d), unable to add to the set", fds);
		return axl_false;
	}

	if (select->length == (VORTEX_FD_SETSIZE - 1)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "reached max amount of descriptors for socket set based on select(2). See VORTEX_FD_SETSIZE=%d, fds=%d, or other I/O mechanism, closing connection.", VORTEX_FD_SETSIZE, fds);
		return axl_false;
	} /* end if */

	/* set the value */
	FD_SET (fds, &(select->set));

	/* update length */
	select->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * @brief Default Vortex Library implementation for the "is set" on fd
 * set operation.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
axl_bool      __vortex_io_waiting_default_is_set (int        fds, 
						  axlPointer __fd_set, 
						  axlPointer user_data)
{
	VortexSelect * select = __fd_set;
	
	return FD_ISSET (fds, &(select->set));
}

/**
 * poll(2) system call implementation.
 */
#if defined(VORTEX_HAVE_POLL)
typedef struct _VortexPoll {
	VortexCtx           * ctx;
	int                   max;
	int                   length;
	struct pollfd       * set;
	VortexConnection   ** connections;
	VortexIoWaitingFor    wait_to;
}VortexPoll;
/** 
 * @internal
 *
 * @brief Internal vortex implementation to support poll(2) interface
 * to the file set creation interface.
 *
 * @return A newly allocated file set reference, supporting poll(2).
 */
axlPointer __vortex_io_waiting_poll_create (VortexCtx * ctx, VortexIoWaitingFor wait_to) 
{
	int          max;
	VortexPoll * poll;

	vortex_log (VORTEX_LEVEL_DEBUG, "creating empty poll(2) set");

	/* support up to 4096 connections */
	if (! vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &max)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get current max hard sock limit");
		return NULL;
	} /* end if */

	/* check if max points to something not useful */
	if (max <= 0)
		max = 4096;

	poll              = axl_new (VortexPoll, 1);
	poll->ctx         = ctx;
	poll->max         = max;
	poll->wait_to     = wait_to;
	poll->set         = axl_new (struct pollfd, max);
	poll->connections = axl_new (VortexConnection *, max);

	return poll;
}

/** 
 * @internal
 *
 * Internal implementation to destroy a file set support the poll(2)
 * interface.
 * 
 * @param fd_group The file set to be deallocated.
 */
void    __vortex_io_waiting_poll_destroy (axlPointer fd_group)
{
	VortexPoll * poll = (VortexPoll *) fd_group;

	/* free the poll */
	axl_free (poll->set);
	axl_free (poll->connections);
	axl_free (poll);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Clears the file set supporting poll(2) interface.
 */
void    __vortex_io_waiting_poll_clear (axlPointer __fd_group)
{
	VortexPoll * poll = (VortexPoll *) __fd_group;

	/* do not clear nothing, try to reuse */
	poll->length = 0;
	memset (poll->set, 0, sizeof (struct pollfd) * poll->max);

	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Add to file set implementation for poll(2) interface
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __vortex_io_waiting_poll_add_to (int                fds, 
					   VortexConnection * connection,
					   axlPointer         __fd_set)
{
	VortexPoll * poll   = (VortexPoll *) __fd_set;
	VortexCtx  * ctx    = poll->ctx;
	int          max;

	/* check if max size reached */
	if (poll->length == poll->max) {
		/* support up to 4096 connections */
		if (! vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &max)) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get current max hard sock limit, closing socket");
			return axl_false;
		} /* end if */

		if (poll->max >= max) {
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to accept more sockets, max poll set reached.");
			return axl_false;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "max amount of file descriptors reached, expanding");

		/* limit reached */
		poll->max          = max;
		poll->set          = axl_realloc (poll->set,         sizeof (struct pollfd)      * poll->max);
		poll->connections  = axl_realloc (poll->connections, sizeof (VortexConnection *) * poll->max);
	} /* end if */

	/* configure the socket to be watched */
	poll->set[poll->length].fd      = fds;
	poll->connections[poll->length] = connection;

	/* configure events to check */
	poll->set[poll->length].events = 0;
	if (VORTEX_IO_IS(poll->wait_to, READ_OPERATIONS)) {
		poll->set[poll->length].events |= POLLIN;
		poll->set[poll->length].events |= POLLPRI;
	} /* end if */
	if (VORTEX_IO_IS(poll->wait_to, WRITE_OPERATIONS))
		poll->set[poll->length].events |= POLLOUT;

	/* update length */
	poll->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * Perform a wait operation over the object support poll(2) interface.
 */
int __vortex_io_waiting_poll_wait_on (axlPointer __fd_group, int max_fds, VortexIoWaitingFor wait_to)
{
	int          result  = -1;
	VortexPoll * _poll   = (VortexPoll *) __fd_group;

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VORTEX_IO_IS (wait_to, READ_OPERATIONS)) {
		/* wait for read operations */
		result       = poll (_poll->set, _poll->length, 500);
	} else 	if (VORTEX_IO_IS (wait_to, WRITE_OPERATIONS)) {
		/* wait for write operations */
		result       = poll (_poll->set, _poll->length, 1000);
	}
	
	/* check result */
	if ((result == VORTEX_SOCKET_ERROR) && (errno == VORTEX_EINTR))
		return -1;
	
	return result;
}


/** 
 * @internal Notify that we have dispatch support.
 */
axl_bool      __vortex_io_waiting_poll_have_dispatch (axlPointer fd_group)
{
	return axl_true;
}

/** 
 * @internal
 *
 * Poll implementation for the automatic dispatch.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
void     __vortex_io_waiting_poll_dispatch (axlPointer           fd_group, 
					    VortexIoDispatchFunc dispatch_func,
					    int                  changed,
					    axlPointer           user_data)
{
	VortexPoll * poll     = (VortexPoll *) fd_group;
	int          iterator = 0;
	int          checked  = 0;

	/* for all sockets polled */
	while (iterator < poll->length && (checked < changed)) {

		/* item found now check the event */
		if (VORTEX_IO_IS(poll->wait_to, READ_OPERATIONS)) {
			
			if ((poll->set[iterator].revents & POLLIN) == POLLIN ||
			    (poll->set[iterator].revents & POLLPRI) == POLLPRI) {
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					poll->set[iterator].fd,
					/* purpose for the waiting set */
					poll->wait_to, 
					/* connection associated */
					poll->connections[iterator], 
					/* dispatch user data */
					user_data);

				/* update items checked */
				checked++;
			} /* end if */

		} /* end if */

		/* item found now check the event */
		if (VORTEX_IO_IS(poll->wait_to, WRITE_OPERATIONS)) {
			if ((poll->set[iterator].revents & POLLOUT) == POLLOUT) {
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					poll->set[iterator].fd,
					/* purpose for the waiting set */
					poll->wait_to, 
					/* connection associated */
					poll->connections[iterator], 
					/* dispatch user data */
					user_data);
				
				/* update items checked */
				checked++;
			} /* end if */
		} /* end if */
		
		/* go to the next */
		iterator++;

	} /* end if */
	
	return;
}
#endif /* VORTEX_HAVE_POLL */


/**
 * linux epoll(2) system call implementation.
 */
#if defined(VORTEX_HAVE_EPOLL)
typedef struct _VortexEPoll {
	VortexCtx           * ctx;
	int                   max;
	int                   length;
	int                   set;
	struct epoll_event  * events;
	VortexIoWaitingFor    wait_to;
}VortexEPoll;
/** 
 * @internal
 *
 * @brief Internal vortex implementation to support epoll(2) interface
 * to the file set creation interface.
 *
 * @return A newly allocated file set reference, supporting epoll(2).
 */
axlPointer __vortex_io_waiting_epoll_create (VortexCtx * ctx, VortexIoWaitingFor wait_to) 
{
	int           max;
	int           set;
	VortexEPoll * epoll;

	/* get current max support */
	if (! vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &max)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get current max hard sock limit");
		return NULL;
	} /* end if */

	/* check if max points to something not useful */
	if (max <= 0)
		max = 4096;

	set = epoll_create (max);
	if (set == -1) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create the epoll interface (epoll_create system call have failed): %s",
			    vortex_errno_get_last_error ());
		return NULL;
	} /* end if */

	/* set the epoll socket as closable on exec */
	fcntl (set, F_SETFD, fcntl(set, F_GETFD) | FD_CLOEXEC);
		
	epoll              = axl_new (VortexEPoll, 1);
	epoll->ctx         = ctx;
	epoll->max         = max;
	epoll->wait_to     = wait_to;
	epoll->set         = set;
	epoll->events      = axl_new (struct epoll_event, max);

	return epoll;
}

/** 
 * @internal
 *
 * Internal implementation to destroy a file set support the epoll(2)
 * interface.
 * 
 * @param fd_group The file set to be deallocated.
 */
void    __vortex_io_waiting_epoll_destroy (axlPointer fd_group)
{
	VortexEPoll * epoll = (VortexEPoll *) fd_group;

	/* free the poll */
	close    (epoll->set);
	axl_free (epoll->events);
	axl_free (epoll);
	
	/* nothing more to do */
	return;
}

/** 
 * @internal
 *
 * Clears the file set supporting epoll(2) interface.
 */
void    __vortex_io_waiting_epoll_clear (axlPointer __fd_group)
{
	VortexEPoll *      epoll  = (VortexEPoll *) __fd_group;

	/* close the epoll set, to avoid having to remove all sockets
	 * that were added but not removed by the kernel */
	close (epoll->set);
	epoll->set    = epoll_create (epoll->max);

	/* set the epoll socket as closable on exec */
	fcntl (epoll->set, F_SETFD, fcntl(epoll->set, F_GETFD) | FD_CLOEXEC);

	epoll->length = 0;
	return;
}

/** 
 * @internal
 *
 * Add to file set implementation for epoll(2) interface.
 * 
 * @param fds The socket descriptor to be added.
 *
 * @param fd_set The fd set where the socket descriptor will be added.
 */
axl_bool  __vortex_io_waiting_epoll_add_to (int                fds, 
					    VortexConnection * connection,
					    axlPointer         __fd_set)
{
	VortexEPoll *        epoll  = (VortexEPoll *) __fd_set;
	VortexCtx   *        ctx    = epoll->ctx;
	int                  max;
	struct epoll_event   ev;

	/* check if max size reached */
	if (epoll->length == epoll->max) {
		/* support up to 4096 connections */
		if (! vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &max)) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get current max hard sock limit, closing socket");
			return axl_false;
		} /* end if */

		if (epoll->max >= max) {
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to accept more sockets, max poll set reached (%d).", epoll->max);
			return axl_false;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "max amount of file descriptors reached, expanding");

		/* limit reached */
		epoll->max          = max;
		epoll->events       = axl_realloc (epoll->events, sizeof (struct epoll_event) * epoll->max);
	} /* end if */

	/* clear data */
	memset (&ev, 0, sizeof (struct epoll_event));

	/* configure the kind of polling */
	if (VORTEX_IO_IS(epoll->wait_to, READ_OPERATIONS)) {
		ev.events = EPOLLIN | EPOLLPRI;
	} /* end if */

	if (VORTEX_IO_IS(epoll->wait_to, WRITE_OPERATIONS)) {
		ev.events = EPOLLOUT;
	} /* end if */

	/* configure the file descriptor */
	ev.data.ptr = connection;
	if (epoll_ctl(epoll->set, EPOLL_CTL_ADD, fds, &ev) != 0 && errno != EEXIST) {
		
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "failed to add to the epoll fd=%d, epoll_ctl system call have failed: %s",
			    fds, vortex_errno_get_last_error ());
		return axl_false;
	} /* end if */

	/* update length */
	epoll->length++;

	return axl_true;
}

/** 
 * @internal
 *
 * Perform a wait operation over the object supporting epoll(2)
 * interface.
 */
int __vortex_io_waiting_epoll_wait_on (axlPointer __fd_group, int max_fds, VortexIoWaitingFor wait_to)
{
	int           result  = -1;
	VortexEPoll * epoll   = (VortexEPoll *) __fd_group;

	/* clear events reported */
	/* memset (epoll->events, 0, sizeof (struct epoll_event) * epoll->max); */

	/* perform the select operation according to the
	 * <b>wait_to</b> value. */
	if (VORTEX_IO_IS (wait_to, READ_OPERATIONS)) {
		result = epoll_wait (epoll->set, epoll->events, epoll->length, 500);
	} else 	if (VORTEX_IO_IS (wait_to, WRITE_OPERATIONS)) {
		result = epoll_wait (epoll->set, epoll->events, epoll->length, 1000);
	} /* end if */

	/* check result */
	if ((result == VORTEX_SOCKET_ERROR) && (errno == VORTEX_EINTR))
		return -1;
	
	return result;
}

/** 
 * @internal Notify that we have dispatch support.
 */
axl_bool      __vortex_io_waiting_epoll_have_dispatch (axlPointer fd_group)
{
	return axl_true;
}

/** 
 * @internal
 *
 * Poll implementation for the automatic dispatch.
 * 
 * @param fds The socket descriptor to be checked to be active on the
 * given fd group.
 *
 * @param fd_set The fd set where the socket descriptor will be checked.
 */
void     __vortex_io_waiting_epoll_dispatch (axlPointer           fd_group, 
					     VortexIoDispatchFunc dispatch_func,
					     int                  changed,
					     axlPointer           user_data)
{
	VortexEPoll      * epoll    = (VortexEPoll *) fd_group;
	VortexConnection * connection;
	int                iterator = 0;

	/* for all sockets polled */
	while ((iterator < changed) && (iterator < epoll->length)) {
		
		/* item found now check the event */
		if (VORTEX_IO_IS (epoll->wait_to, READ_OPERATIONS)) {
			
			if ((epoll->events[iterator].events & EPOLLIN) == EPOLLIN ||
			    (epoll->events[iterator].events & EPOLLPRI) == EPOLLPRI) {

				/* get the connection */
				connection = (VortexConnection *) epoll->events[iterator].data.ptr;
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					vortex_connection_get_socket (connection),
					/* purpose for the waiting set */
					epoll->wait_to, 
					/* connection associated */
					connection,
					/* dispatch user data */
					user_data);
			} /* end if */

		} /* end if */

		/* item found now check the event */
		if (VORTEX_IO_IS(epoll->wait_to, WRITE_OPERATIONS)) {
			if ((epoll->events[iterator].events & EPOLLOUT) == EPOLLOUT) {

				/* get the connection */
				connection = (VortexConnection *) epoll->events[iterator].data.ptr;
				
				/* found read event, dispatch */
				dispatch_func (
					/* socket found */
					vortex_connection_get_socket (connection),
					/* purpose for the waiting set */
					epoll->wait_to, 
					/* connection associated */
					connection,
					/* dispatch user data */
					user_data);
			} /* end if */
		} /* end if */
		
		/* go to the next */
		iterator++;

	} /* end if */
	
	return;
}
#endif /* VORTEX_HAVE_EPOLL */


/** 
 * @brief Allows to configure the default io waiting mechanism to be
 * used by the library.
 * 
 * To get current mechanism selected you must use \ref
 * vortex_io_waiting_get_current. During the compilation process,
 * vortex selects the default mechanism, choosing the best available. 
 *
 * This function is designed to support changing the I/O API at
 * runtime so, you can call to this function during normal vortex
 * operations.
 *
 * Here is an example of code on how to change the default mechanism
 * to use system call poll(2):
 *
 * \code
 * // check the mechanism is available
 * if (vortex_io_waiting_is_available (VORTEX_IO_WAIT_POLL)) {
 *     // configure the mechanism
 *     vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_POLL);
 * }
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param type The I/O waiting mechanism to be used by the library
 * (especially at the vortex reader module).
 * 
 * @return axl_true if the mechanism was activated, otherwise axl_false
 * is returned.
 */
axl_bool                  vortex_io_waiting_use (VortexCtx * ctx, VortexIoWaitingType type)
{
	/* get current context */
	axl_bool       result = axl_false;
	axl_bool       do_notify;
#if defined(ENABLE_VORTEX_LOG)
	const char   * mech = "";
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "about to change I/O waiting API");

	/* check if the type is available */
	if (! vortex_io_waiting_is_available (type)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "I/O waiting API not available");
		return axl_false;
	}

	/* check if the user is requesting to change to the same IO
	 * API */
	if (type == ctx->waiting_type) {
		vortex_log (VORTEX_LEVEL_DEBUG, "I/O waiting API requested currently installed");
		return axl_true;
	}

	/* notify the reader to stop processing and dealloc resources
	 * with current IO api installed. */
	vortex_log (VORTEX_LEVEL_DEBUG, "requesting vortex reader to change its I/O API");
	do_notify = vortex_reader_notify_change_io_api (ctx);

	vortex_log (VORTEX_LEVEL_DEBUG, "done, now vortex reader is blocked until we finish");

	switch (type) {
	case VORTEX_IO_WAIT_SELECT:
		/* use select mechanism */
		ctx->waiting_create        = __vortex_io_waiting_default_create;
		ctx->waiting_destroy       = __vortex_io_waiting_default_destroy;
		ctx->waiting_clear         = __vortex_io_waiting_default_clear;
		ctx->waiting_wait_on       = __vortex_io_waiting_default_wait_on;
		ctx->waiting_add_to        = __vortex_io_waiting_default_add_to;
		ctx->waiting_is_set        = __vortex_io_waiting_default_is_set;
		ctx->waiting_have_dispatch = NULL;
		ctx->waiting_dispatch      = NULL;
		ctx->waiting_type          = VORTEX_IO_WAIT_SELECT;
#if defined(ENABLE_VORTEX_LOG)
		mech                       = "select(2) system call";
#endif

		/* ok */
		result = axl_true;
		break;
	case VORTEX_IO_WAIT_POLL:
		/* use poll mechanism */
#if defined (VORTEX_HAVE_POLL)
		/* use select mechanism */
		ctx->waiting_create        = __vortex_io_waiting_poll_create;
		ctx->waiting_destroy       = __vortex_io_waiting_poll_destroy;
		ctx->waiting_clear         = __vortex_io_waiting_poll_clear;
		ctx->waiting_wait_on       = __vortex_io_waiting_poll_wait_on;
		ctx->waiting_add_to        = __vortex_io_waiting_poll_add_to;
		/* no is_set support but automatic dispatch */
		ctx->waiting_is_set        = NULL;
		ctx->waiting_have_dispatch = __vortex_io_waiting_poll_have_dispatch;
		ctx->waiting_dispatch      = __vortex_io_waiting_poll_dispatch;
		ctx->waiting_type          = VORTEX_IO_WAIT_POLL;
#if defined(ENABLE_VORTEX_LOG)
		mech                       = "poll(2) system call";
#endif
	       
		/* ok */
		result = axl_true;
#else 
		result = axl_false;
#endif
		/* important, leave the break outside the mech
		 * definition */
		break;
	case VORTEX_IO_WAIT_EPOLL:
		/* use poll mechanism */
#if defined (VORTEX_HAVE_EPOLL)
		/* use select mechanism */
		ctx->waiting_create        = __vortex_io_waiting_epoll_create;
		ctx->waiting_destroy       = __vortex_io_waiting_epoll_destroy;
		ctx->waiting_clear         = __vortex_io_waiting_epoll_clear;
		ctx->waiting_wait_on       = __vortex_io_waiting_epoll_wait_on;
		ctx->waiting_add_to        = __vortex_io_waiting_epoll_add_to;
		/* no is_set support but automatic dispatch */
		ctx->waiting_is_set        = NULL;
		ctx->waiting_have_dispatch = __vortex_io_waiting_epoll_have_dispatch;
		ctx->waiting_dispatch      = __vortex_io_waiting_epoll_dispatch;
		ctx->waiting_type          = VORTEX_IO_WAIT_EPOLL;
#if defined(ENABLE_VORTEX_LOG)
		mech                       = "linux epoll(2) system call";
#endif
	       
		/* ok */
		result = axl_true;
#else 
		result = axl_false;
#endif
		/* important, leave the break outside the mech
		 * definition */
		break;
	} /* end switch */

	vortex_log (VORTEX_LEVEL_DEBUG, "I/O API changed with result=%s%s%s", 
		    result ? "ok" : "fail",
		    (strlen (mech) > 0) ? ", now using: " :"",
		    (strlen (mech) > 0) ? mech            : "");

	/* notify that the API change process have finished */
	if (do_notify) {
		vortex_reader_notify_change_done_io_api (ctx);
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "vortex reader notified");

	/* fail */
	return result;
}

/** 
 * @brief Allows to check if a particular mechanism is available to be
 * installed.
 * 
 * @param type The mechanism to check.
 * 
 * @return axl_true if available, otherwise axl_false is returned.
 */
axl_bool                  vortex_io_waiting_is_available (VortexIoWaitingType type)
{
	switch (type) {
	case VORTEX_IO_WAIT_SELECT:
		/* ok */
		return axl_true;
	case VORTEX_IO_WAIT_POLL:
		/* use poll mechanism */
#if defined (VORTEX_HAVE_POLL)
		/* ok */
		return axl_true;
#else
		/* not available */
		return axl_false;
#endif
	case VORTEX_IO_WAIT_EPOLL:
		/* use poll mechanism */
#if defined (VORTEX_HAVE_EPOLL)
		/* ok */
		return axl_true;
#else
		/* not available */
		return axl_false;
#endif
	} /* end switch */

	/* fail */
	return axl_false;
}

/** 
 * @brief Allows to check current I/O waiting mechanism being used at
 * the providec context.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return The I/O mechanism used by the core.
 */
VortexIoWaitingType  vortex_io_waiting_get_current  (VortexCtx * ctx)
{
	if (ctx == NULL)
		return 0;

	/* return current implementation */
	return ctx->waiting_type;
}

/** 
 * @brief Allows to set current handler that allows to create a fd
 * group that will be used by Vortex internals to perform IO blocking.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param create The handler to be executed by the Vortex Library to
 * create a new IO fd set. The function will fail to set the default
 * create handler is a NULL handler is provided. No handler will be
 * modified is the create handler is not provided.
 */
void                 vortex_io_waiting_set_create_fd_group (VortexCtx * ctx, 
							    VortexIoCreateFdGroup create)
{
	/* check that the handler provided is not null */
	if (ctx == NULL || create == NULL)
		return;

	/* set default create handler */
	ctx->waiting_create = create;
	
	return;
}

/** 
 * @internal
 *
 * @brief Perform an invocation to create a fd group using current
 * create handler defined.
 * 
 * This function is for internal Vortex Library purposes and should
 * not be used by API consumers.
 * 
 * @return Returns a reference to a new allocated fd set.
 */
axlPointer           vortex_io_waiting_invoke_create_fd_group (VortexCtx           * ctx,
							       VortexIoWaitingFor    wait_to)
{
	/* check current create IO handler configuration */
	if (ctx == NULL || ctx->waiting_create == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "critical error, found that create fd set handler is not properly set, this is critical malfunctions");
		return NULL;
	}
	
	/* return a newly allocated fd set */
	return ctx->waiting_create (ctx, wait_to);
}


/** 
 * @brief Allows to define the destroy method to be used on the fd_set
 * allocated by calling current configured \ref VortexIoCreateFdGroup
 * "create handler".
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param destroy The handler to deallocate. The function will fail if
 * the provided destroy function is NULL. No operation will be
 * performed in such case.
 */
void                 vortex_io_waiting_set_destroy_fd_group    (VortexCtx              * ctx, 
								VortexIoDestroyFdGroup   destroy)
{
	/* check if the destroy handler is not a NULL reference */
	if (ctx == NULL || destroy == NULL)
		return;

	/* set default destroy handler */
	ctx->waiting_destroy = destroy;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current destroy handler configured to release memory
 * allocate by the fd group created by calling \ref
 * vortex_io_waiting_invoke_create_fd_group.
 *
 * This function is for internal Vortex Library purposes and should
 * not be used by API consumers.
 * 
 * @param fd_group The fd group to destroy. 
 */
void                 vortex_io_waiting_invoke_destroy_fd_group (VortexCtx  * ctx, 
								axlPointer   fd_group)
{
	/* check that the received fd_group is not null */
	if (ctx == NULL || fd_group == NULL)
		return;

	/* check current destroy IO handler configuration */
	if (ctx->waiting_destroy == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "critical error, found that destroy fd set handler is not properly set, this is critical malfunctions");
		return;
	}
	
	/* return a newly allocated fd set */
	ctx->waiting_destroy (fd_group);

	return;
}


/** 
 * @brief Allows to configure the clear handler to be executed while
 * it is required to clear a descriptor set.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param clear The handler to executed once Vortex internal process
 * requires to create a socket descriptor set.
 */
void                 vortex_io_waiting_set_clear_fd_group    (VortexCtx             * ctx, 
							      VortexIoClearFdGroup    clear)
{
	/* check if the clear handler is not a NULL reference */
	if (clear == NULL || ctx == NULL)
		return;

	/* set default clear handler */
	ctx->waiting_clear = clear;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current clear fd group configure handler on the
 * given reference received.
 *
 * This function is internal to vortex library, and it is used at the
 * right place. This function should be be used by API consumers.
 *
 * @param fd_group The fd_group reference to clear.
 */
void                 vortex_io_waiting_invoke_clear_fd_group (VortexCtx * ctx, axlPointer fd_group)
{
	/* check that the received fd_group is not null */
	if (fd_group == NULL || ctx == NULL)
		return;

	/* check current clear IO handler configuration */
	if (ctx->waiting_clear == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "critical error, found that clear fd set handler is not properly set, this is critical malfunctions");
		return;
	}
	
	/* return a newly allocated fd set */
	ctx->waiting_clear (fd_group);

	return;
}

/** 
 * @brief Allows to configure the add socket to fd set operation.
 *
 * The function will fail if the provided handler is NULL.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param add_to The handler to be invoked when it is required to add
 * a socket descriptor into the fd set.
 */
void                 vortex_io_waiting_set_add_to_fd_group     (VortexCtx            * ctx, 
								VortexIoAddToFdGroup   add_to)
{
	/* check for NULL reference handlers */
	if (add_to == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_add_to = add_to;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current add to operation for the given socket
 * descriptor into the given fd set.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fds The socket descriptor to be added. The socket descriptor
 * should be greater than 0.
 *
 * @param on_reading The fd set where the socket descriptor will be
 * added.
 */
axl_bool               vortex_io_waiting_invoke_add_to_fd_group  (VortexCtx        * ctx,
								  VORTEX_SOCKET      fds, 
								  VortexConnection * connection, 
								  axlPointer         fd_group)
{
	if (ctx != NULL && ctx->waiting_add_to != NULL) {
		
		/* invoke add to operation */
		return ctx->waiting_add_to (fds, connection, fd_group);
	} /* end if */

	/* return axl_false if it fails */
	return axl_false;
}

/** 
 * @brief Allows to configure the is set operation for the socket on the fd set.
 *
 * The function will fail if the provided handler is NULL.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param is_set The handler to be invoked when it is required to
 * check if a socket descriptor is set into a given fd set.
 */
void                 vortex_io_waiting_set_is_set_fd_group     (VortexCtx            * ctx, 
								VortexIoIsSetFdGroup   is_set)
{
	/* check for NULL reference handlers */
	if (is_set == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_is_set = is_set;

	return;
}

/** 
 * @brief Allows to configure the "have dispatch" function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param have_dispatch The handler to be executed by the vortex I/O
 * module to check if the current mechanism support automatic
 * dispatch.
 */
void                 vortex_io_waiting_set_have_dispatch       (VortexCtx             * ctx,
								VortexIoHaveDispatch    have_dispatch)
{

	/* check for NULL reference handlers */
	if (have_dispatch == NULL || ctx == NULL)
		return;

	/* set the new handler */
	ctx->waiting_have_dispatch = have_dispatch;
	return;
}

/** 
 * @brief Allows to configure the dispatch function to be used for the
 * automatic dispatch function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param dispatch The dispatch function to be executed on every
 * socket changed.
 */
void                 vortex_io_waiting_set_dispatch            (VortexCtx          * ctx,
								VortexIoDispatch     dispatch)
{
	/* check for NULL reference handlers */
	if (dispatch == NULL || ctx == NULL)
		return;
	
	/* set the new handler */
	ctx->waiting_dispatch = dispatch;
	return;
}

/** 
 * @internal
 *
 * @brief Invokes current is set to operation for the given socket
 * descriptor into the given fd set.
 * 
 * @param fds The socket descriptor to be checked. The socket
 * descriptor should be greater than 0.
 *
 * @param on_reading The fd set where the socket descriptor will be
 * checked for its activation.
 *
 * @param user_data User defined pointer provided to the is set
 * function.
 */
axl_bool                   vortex_io_waiting_invoke_is_set_fd_group  (VortexCtx      * ctx,
								      VORTEX_SOCKET    fds, 
								      axlPointer       fd_group, 
								      axlPointer       user_data)
{
	if (ctx == NULL || fd_group == NULL || fds <= 0)
		return axl_false;
	
	/* check for properly add to handler configuration */
	if (ctx->waiting_is_set == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "there is no \"is set\" operation defined, this will cause critical malfunctions");
		return axl_false;
	}

	/* invoke add to operation */
	return ctx->waiting_is_set (fds, fd_group, user_data);
}

/** 
 * @brief Allows to check if the current I/O mechanism have automatic
 * dispatch support, that is, the I/O mechanism takes care about
 * invoking the appropiate function if the socket is activated for the
 * registered purpose.
 *
 * If the implementation provided support this feature, the function
 * must return axl_true. By returning axl_true it is implied that \ref
 * vortex_io_waiting_invoke_dispatch is also implemented.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fd_group The reference to the object created by the I/O
 * implemetation that is being used.
 * 
 * @return axl_true if the current I/O mechanism support automatic
 * dispatch, otherwise axl_false is returned.
 */
axl_bool                  vortex_io_waiting_invoke_have_dispatch    (VortexCtx  * ctx, 
								     axlPointer   fd_group)
{
	/* check the references received */
	if (ctx == NULL || fd_group == NULL)
		return axl_false;
	
	/* check for properly add to handler configuration */
	if (ctx->waiting_have_dispatch == NULL) {
		return axl_false;
	} /* end if */

	/* invoke have dispatch operation */
	return ctx->waiting_have_dispatch (fd_group);
}

/** 
 * @brief Performs automatic dispath, over all activated resources,
 * invoking the provided handler.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param fd_group The file set group that is being notified to
 * automatically dispatch.
 *
 * @param func The function to execute on each activated socket
 * descriptor.
 *
 * @param changed Number of file descriptors that have been reported
 * changed state.
 *
 * @param user_data Reference to the user defined data to be passed to
 * the dispatch function.
 */
void                 vortex_io_waiting_invoke_dispatch         (VortexCtx            * ctx,
								axlPointer             fd_group, 
								VortexIoDispatchFunc   func,
								int                    changed,
								axlPointer             user_data)
{
	/* check parameters */
	if (ctx == NULL || fd_group == NULL || func == NULL)
		return;

	/* check for properly add to handler configuration */
	if (ctx->waiting_dispatch == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "there is no \"dispatch\" operation defined, this will cause critical malfunctions");
		return;
	} /* end if */

	/* invoke dispatch operation */
	ctx->waiting_dispatch (fd_group, func, changed, user_data);
	return;
}




/** 
 * @brief Allows to configure the wait on fd group handler. 
 * 
 * This handler allows to configure how is implemented a non-consuming
 * resource on a given fd group.
 *
 * The invoked function should return a selected error code according
 * to the following values:
 * 
 * - A timeout operation was received, then <b>-1</b> should be
 * returned.
 *
 * - A error has being received but the call loop should continue
 * without paying attention to it, without checking for the sockets
 * for its status, and to perform as soon as possible the same
 * wait. This is used because current current default implementation,
 * which used the IO "select" call could return because a signal was
 * received. If the IO implemented doesn't require to implement this
 * concept, just ignore it. Returned value should be <b>-2</b>.
 *
 * - A non recoverable error has happened, and vortex reader should
 * exist. In this case <b>-3</b> should be returned. This error code
 * is also used by the internal vortex IO handler invocation to report
 * that a wait handler haven't been defined.
 *
 * - No socket descriptor have change but it is required to perform
 * again the blocking IO wait. Then <b>0</b> should be returned.
 *
 * - For the rest of the cases, the number of sockets could be
 * returned by this function, however, this is not used by the Vortex
 * Library. To report that a socket have changed (or a group of then
 * inside the fd set), and you don't want to return how many sockets
 * have changed, just return a positive value (<b>1</b>).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param wait_on The handler to be used. The function will fail on
 * setting the handler if it is provided a NULL reference.
 */
void                 vortex_io_waiting_set_wait_on_fd_group    (VortexCtx             * ctx, 
								VortexIoWaitOnFdGroup   wait_on)
{
	/* check references */
	if (ctx == NULL || wait_on == NULL)
		return;

	/* set new default handler to be used */
	ctx->waiting_wait_on = wait_on;

	return;
}

/** 
 * @internal
 *
 * @brief Invokes current implementation for IO blocking until the
 * requested operation could be performed of any of the given sockets
 * descriptors inside the descriptor group selected.
 *
 * @param fd_group The socket descriptor group where the request wait
 * operation will be performed.
 *
 * @param wait_to The operation requested.
 * 
 * @return Current error code or the number of sockets that have
 * changed.
 * 
 */
int                  vortex_io_waiting_invoke_wait             (VortexCtx          * ctx,
								        axlPointer           fd_group, 
								        int                  max_fds,
								        VortexIoWaitingFor   wait_to)
{
	/* check reference to the context */
	if (ctx == NULL)
		return -3;

	/* check for NULL reference */
	if (ctx->waiting_wait_on == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "wait handler is not defined, this is a critical error that will produce a great malfunctions");
		return -3;
	}

	/* perform IO blocking operation */
	return ctx->waiting_wait_on (fd_group, max_fds, wait_to);
}

/**
 * @internal Private function to init the context with the proper
 * functions.
 */
void                 vortex_io_init (VortexCtx * ctx)
{
	/*
	 * @internal
	 * 
	 * Following is the set of handlers to be used while performing IO
	 * waiting operation, as defined by the API exported to modify
	 * function to be used.
	 *  
	 * There is one handler for each operation that could be performed. No
	 * mutex and any other blocking operation should be used while
	 * implementing this set of handler.
	 *
	 * Critical section, resource exclusion and any other race condition
	 * is ensure by the Vortex Library to not happen. 
	 *
	 * See default handles implemented to get an idea about how should be
	 * implemented other IO blocking operations using other APIs rather than
	 * select.
	 */
#if defined(DEFAULT_EPOLL)
	
	ctx->waiting_create        = __vortex_io_waiting_epoll_create;
	ctx->waiting_destroy       = __vortex_io_waiting_epoll_destroy;
	ctx->waiting_clear         = __vortex_io_waiting_epoll_clear;
	ctx->waiting_wait_on       = __vortex_io_waiting_epoll_wait_on;
	ctx->waiting_add_to        = __vortex_io_waiting_epoll_add_to;
	ctx->waiting_is_set        = NULL;
	ctx->waiting_have_dispatch = __vortex_io_waiting_epoll_have_dispatch;
	ctx->waiting_dispatch      = __vortex_io_waiting_epoll_dispatch;

	/* current implementation used */
	ctx->waiting_type          = VORTEX_IO_WAIT_EPOLL;

#elif defined(DEFAULT_POLL) /* poll(2) system call support */

	ctx->waiting_create        = __vortex_io_waiting_poll_create;
	ctx->waiting_destroy       = __vortex_io_waiting_poll_destroy;
	ctx->waiting_clear         = __vortex_io_waiting_poll_clear;
	ctx->waiting_wait_on       = __vortex_io_waiting_poll_wait_on;
	ctx->waiting_add_to        = __vortex_io_waiting_poll_add_to;
	ctx->waiting_is_set        = NULL;
	ctx->waiting_have_dispatch = __vortex_io_waiting_poll_have_dispatch;
	ctx->waiting_dispatch      = __vortex_io_waiting_poll_dispatch;

	/* current implementation used */
	ctx->waiting_type          = VORTEX_IO_WAIT_POLL;

#else /* select(2) system call support */

	ctx->waiting_create        = __vortex_io_waiting_default_create;
	ctx->waiting_destroy       = __vortex_io_waiting_default_destroy;
	ctx->waiting_clear         = __vortex_io_waiting_default_clear;
	ctx->waiting_wait_on       = __vortex_io_waiting_default_wait_on;
	ctx->waiting_add_to        = __vortex_io_waiting_default_add_to;
	ctx->waiting_is_set        = __vortex_io_waiting_default_is_set;
	ctx->waiting_have_dispatch = NULL;
	ctx->waiting_dispatch      = NULL;

	/* current implementation used */
	ctx->waiting_type          = VORTEX_IO_WAIT_SELECT;

#endif
	return;
}

/* @} */



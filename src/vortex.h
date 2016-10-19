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

#ifndef __VORTEX_H__
#define __VORTEX_H__

#if defined(__GNUC__) && !defined(_GNU_SOURCE)
/** 
 * Needed for extended pthread API for recursive functions.
 */
#  define _GNU_SOURCE 
#endif

/**
 * \addtogroup vortex
 * @{
 */

/* define default socket pool size for the VORTEX_IO_WAIT_SELECT
 * method. If you change this value, you must change the
 * following. This value must be synchronized with FD_SETSIZE. This
 * has been tested on windows to work properly, but under GNU/Linux,
 * the GNUC library just rejects providing this value, no matter where
 * you place them. The only solutions are:
 *
 * [1] Modify /usr/include/bits/typesizes.h the macro __FD_SETSIZE and
 *     update the following values: FD_SETSIZE and VORTEX_FD_SETSIZE.
 *
 * [2] Use better mechanism like poll or epoll which are also available 
 *     in the platform that is giving problems.
 * 
 * [3] The last soluction could be the one provided by you. Please report
 *     any solution you may find.
 **/
#ifndef VORTEX_FD_SETSIZE
#define VORTEX_FD_SETSIZE 1024
#endif
#ifndef FD_SETSIZE
#define FD_SETSIZE 1024
#endif

/* External header includes */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Axl library headers */
#include <axl.h>

/* Direct portable mapping definitions */
#if defined(AXL_OS_UNIX)

/* Portable definitions while using Vortex Library */
#define VORTEX_EINTR           EINTR
#define VORTEX_EWOULDBLOCK     EWOULDBLOCK
#define VORTEX_EINPROGRESS     EINPROGRESS
#define VORTEX_EAGAIN          EAGAIN
#define VORTEX_SOCKET          int
#define VORTEX_INVALID_SOCKET  -1
#define VORTEX_SOCKET_ERROR    -1
#define vortex_close_socket(s) do {if ( s >= 0) {close (s);}} while (0)
#define vortex_getpid          getpid
#define vortex_sscanf          sscanf
#define vortex_is_disconnected (errno == EPIPE)
#define VORTEX_FILE_SEPARATOR "/"

#endif /* end defined(AXL_OS_UNIX) */

#if defined(AXL_OS_WIN32)

/* additional includes for the windows platform */

/* _WIN32_WINNT note: If the application including the header defines
 * the _WIN32_WINNT, it must include the bit defined by the value
 * 0x501 (formerly 0x400). */
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x501

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <time.h>

#define VORTEX_EINTR           WSAEINTR
#define VORTEX_EWOULDBLOCK     WSAEWOULDBLOCK
#define VORTEX_EINPROGRESS     WSAEINPROGRESS
#define VORTEX_EAGAIN          WSAEWOULDBLOCK
#define SHUT_RDWR              SD_BOTH
#define SHUT_WR                SD_SEND
#define VORTEX_SOCKET          SOCKET
#define VORTEX_INVALID_SOCKET  INVALID_SOCKET
#define VORTEX_SOCKET_ERROR    SOCKET_ERROR
#define vortex_close_socket(s) do {if ( s >= 0) {closesocket (s);}} while (0)
#define vortex_getpid          _getpid
#define vortex_sscanf          sscanf
#define uint16_t               u_short
#define vortex_is_disconnected ((errno == WSAESHUTDOWN) || (errno == WSAECONNABORTED) || (errno == WSAECONNRESET))
#define VORTEX_FILE_SEPARATOR "\\"
#define inet_ntop vortex_win32_inet_ntop
#define EADDRNOTAVAIL 125

/* no link support windows */
#define S_ISLNK(m) (0)

#endif /* end defined(AXL_OS_WINDOWS) */

#if defined(AXL_OS_UNIX)
#include <sys/types.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#endif

/* additional headers for poll support */
#if defined(VORTEX_HAVE_POLL)
#include <sys/poll.h>
#endif

/* additional headers for linux epoll support */
#if defined(VORTEX_HAVE_EPOLL)
#include <sys/epoll.h>
#endif

/* Check gnu extensions, providing an alias to disable its precence
 * when no available. */
#if     __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 8)
#  define GNUC_EXTENSION __extension__
#else
#  define GNUC_EXTENSION
#endif

/* define minimal support for int64 constants */
#ifndef _MSC_VER 
#  define INT64_CONSTANT(val) (GNUC_EXTENSION (val##LL))
#else /* _MSC_VER */
#  define INT64_CONSTANT(val) (val##i64)
#endif

/* check for missing definition for S_ISDIR */
#ifndef S_ISDIR
#  ifdef _S_ISDIR
#    define S_ISDIR(x) _S_ISDIR(x)
#  else
#    ifdef S_IFDIR
#      ifndef S_IFMT
#        ifdef _S_IFMT
#          define S_IFMT _S_IFMT
#        endif
#      endif
#       ifdef S_IFMT
#         define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#       endif
#    endif
#  endif
#endif

/* check for missing definition for S_ISREG */
#if defined(_MSC_VER) && (_MSC_VER >= 1400)
# define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif 

/** 
 * @brief Returns the minimum from two values.
 * @param a First value to compare.
 * @param b Second value to compare.
 */
#define VORTEX_MIN(a,b) ((a) > (b) ? b : a)

/** 
 * @brief Allows to check the reference provided, and returning the
 * return value provided.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 */
#define VORTEX_CHECK_REF(ref, return_value) do { \
	if (ref == NULL) {   		         \
             return return_value;                \
	}                                        \
} while (0)

/** 
 * @brief Allows to check the reference provided, returning the return
 * value provided, also releasing a second reference with a custom
 * free function.
 * @param ref The reference to be checke for NULL.
 * @param return_value The return value to return in case of NULL reference.
 * @param ref2 Second reference to be released
 * @param free2_func Function to be used to release the second reference.
 */
#define VORTEX_CHECK_REF2(ref, return_value, ref2, free2_func) do { \
        if (ref == NULL) {                                          \
               free2_func (ref);                                    \
	       return return_value;                                 \
	}                                                           \
} while (0)

BEGIN_C_DECLS

/* Internal includes and external includes for Vortex API
 * consumers. */
#include <vortex_types.h>
#include <vortex_support.h>
#include <vortex_handlers.h>
#include <vortex_ctx.h>
#include <vortex_thread.h>
#include <vortex_thread_pool.h>
#include <vortex_queue.h>
#include <vortex_hash.h>
#include <vortex_connection.h>
#include <vortex_listener.h>
#include <vortex_frame_factory.h>
#include <vortex_greetings.h>
#include <vortex_profiles.h>
#include <vortex_channel.h>
#include <vortex_channel_pool.h>
#include <vortex_io.h>
#include <vortex_reader.h>
#include <vortex_dtds.h>
#include <vortex_sequencer.h>
#include <vortex_channel_pool.h>
#include <vortex_errno.h>
#include <vortex_payload_feeder.h>

END_C_DECLS

#if defined(AXL_OS_WIN32)
#include <vortex_win32.h>
#endif

#include <errno.h>

#if defined(AXL_OS_WIN32)
/* errno redefinition for windows platform. this declaration must
 * follow the previous include. Please, use -DVORTEX_SKIP_ERRNO_REDEF
 * if this redefinition is causing you problems under windows. */

/* Also, some automatic code is added to detect some cases */
#if defined(_MSC_VER) && (_MSC_VER >= 1700)
/* skip errno redefinition for Visual Studio 11 2012 and above:
   http://stackoverflow.com/questions/70013/how-to-detect-if-im-compiling-code-with-visual-studio-2008
*/
#  ifndef VORTEX_SKIP_ERRNO_REDEF
#    define VORTEX_SKIP_ERRNO_REDEF
#  endif
#endif

#if !defined(VORTEX_SKIP_ERRNO_REDEF)
#  ifdef  errno
#  undef  errno
#  endif
#  define errno (WSAGetLastError())
#  endif
#endif /* !defined(VORTEX_SKIP_ERRNO_REDEF) */

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. If log is
 * stripped from vortex building all instructions are removed.
 */
#if defined(ENABLE_VORTEX_LOG)
# define vortex_log(l, m, ...)   do{_vortex_log  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define vortex_log2(l, m, ...)   do{_vortex_log2  (ctx, __AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define vortex_log _vortex_log
#   define vortex_log2 _vortex_log2
# else
#   define vortex_log(l, m, ...) /* nothing */
#   define vortex_log2(l, m, message, ...) /* nothing */
# endif
#endif

/** 
 * @internal The following definition allows to find printf like wrong
 * argument passing to nopoll_log function. To activate the depuration
 * just add the following header after this comment.
 *
 * #define SHOW_FORMAT_BUGS (1)
 */
#if defined(SHOW_FORMAT_BUGS)
# undef  vortex_log
# define vortex_log(l, m, ...)   do{printf (m, ##__VA_ARGS__);}while(0)
#endif

/** 
 * @internal Allows to check a condition and return if it is not meet.
 * 
 * @param expr The expresion to check.
 */
#define v_return_if_fail(expr) \
if (!(expr)) {return;}

/** 
 * @internal Allows to check a condition and return the given value if it
 * is not meet.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 */
#define v_return_val_if_fail(expr, val) \
if (!(expr)) { return val;}

/** 
 * @internal Allows to check a condition and return if it is not
 * meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_if_fail_msg(expr,msg) \
if (!(expr)) {vortex_log (VORTEX_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return;}

/** 
 * @internal Allows to check a condition and return the given value if
 * it is not meet. It also provides a way to log an error message.
 * 
 * @param expr The expresion to check.
 *
 * @param val The value to return if the expression is not meet.
 *
 * @param msg The message to log in the case a failure is found.
 */
#define v_return_val_if_fail_msg(expr, val, msg) \
if (!(expr)) { vortex_log (VORTEX_LEVEL_CRITICAL, "%s: %s", __AXL_PRETTY_FUNCTION__, msg); return val;}


BEGIN_C_DECLS

axl_bool vortex_init_ctx             (VortexCtx * ctx);

axl_bool vortex_init_check           (VortexCtx * ctx);

void     vortex_exit_ctx             (VortexCtx * ctx, 
				      axl_bool    free_ctx);

axl_bool vortex_is_exiting           (VortexCtx * ctx);

axl_bool vortex_log_is_enabled       (VortexCtx * ctx);

axl_bool vortex_log2_is_enabled      (VortexCtx * ctx);

void     vortex_log_enable           (VortexCtx * ctx, 
				      axl_bool    status);

void     vortex_log2_enable          (VortexCtx * ctx, 
				      axl_bool    status);

axl_bool vortex_color_log_is_enabled (VortexCtx * ctx);

void     vortex_color_log_enable     (VortexCtx * ctx, 
				      axl_bool    status);

axl_bool vortex_log_is_enabled_acquire_mutex (VortexCtx * ctx);

void     vortex_log_acquire_mutex            (VortexCtx * ctx, 
					      axl_bool    status);

void     vortex_log_set_handler      (VortexCtx         * ctx,
				      VortexLogHandler    handler);

void     vortex_log_set_handler_full (VortexCtx             * ctx,
				      VortexLogHandlerFull    handler,
				      axlPointer              user_data);

void     vortex_log_set_prepare_log  (VortexCtx         * ctx,
				      axl_bool            prepare_string);

VortexLogHandler vortex_log_get_handler (VortexCtx      * ctx);

void     vortex_log_filter_level     (VortexCtx * ctx, const char * filter_string);

axl_bool    vortex_log_filter_is_enabled (VortexCtx * ctx);

void     vortex_writer_data_free     (VortexWriterData * writer_data);

/**
 * @brief Allowed items to use for \ref vortex_conf_get.
 */
typedef enum {
	/** 
	 * @brief Gets/sets current soft limit to be used by the library,
	 * regarding the number of connections handled. Soft limit
	 * means it is can be moved to hard limit.
	 *
	 * To configure this value, use the integer parameter at \ref vortex_conf_set. Example:
	 * \code
	 * vortex_conf_set (VORTEX_SOFT_SOCK_LIMIT, 4096, NULL);
	 * \endcode
	 */
	VORTEX_SOFT_SOCK_LIMIT = 1,
	/** 
	 * @brief Gets/sets current hard limit to be used by the
	 * library, regarding the number of connections handled. Hard
	 * limit means it is not possible to exceed it.
	 *
	 * To configure this value, use the integer parameter at \ref vortex_conf_set. Example:
	 * \code
	 * vortex_conf_set (VORTEX_HARD_SOCK_LIMIT, 4096, NULL);
	 * \endcode
	 */
	VORTEX_HARD_SOCK_LIMIT = 2,
	/** 
	 * @brief Gets/sets current backlog configuration for listener
	 * connections.
	 *
	 * Once a listener is activated, the backlog is the number of
	 * complete connections (with the finished tcp three-way
	 * handshake), that are ready to be accepted by the
	 * application. The default value is 5.
	 *
	 * Once a listener is activated, and its backlog is
	 * configured, it can't be changed. In the case you configure
	 * this value, you must set it (\ref vortex_conf_set) after
	 * calling to the family of functions to create vortex
	 * listeners (\ref vortex_listener_new).
	 *
	 * To configure this value, use the integer parameter at \ref vortex_conf_set. Example:
	 * \code
	 * vortex_conf_set (VORTEX_LISTENER_BACKLOG, 64, NULL);
	 * \endcode
	 */
	VORTEX_LISTENER_BACKLOG = 3,
	/** 
	 * @brief By default, vortex will allow the application layer
	 * to request a channel creation using a profile which wasn't
	 * adviced by the remote peer. Though it could be not
	 * required, some BEEP peers may want to hide some profiles
	 * until some condition is meet. 
	 * 
	 * Because a new BEEP &lt;greeting&gt; can't be sent advising new
	 * profiles supported once those conditions are meet, it is
	 * required to allow creating channels under profiles that
	 * aren't adviced by the remote peer at the first &lt;greetings&gt;
	 * exchange.
	 * 
	 * This is mainly used by Turbulence BEEP application server
	 * for its profile path support, which allows to design a policy
	 * to be follow while creating channels, selecting some
	 * profiles under some conditions.
	 *
	 * By default, the value configured is axl_false, that is, allows
	 * to create channels under profiles even not adviced.
	 */
	VORTEX_ENFORCE_PROFILES_SUPPORTED = 4,
	/** 
	 * @brief If configured, makes all messages send via
	 * vortex_channel_send_* to automatically add MIME headers
	 * configured.
	 * 
	 * This means that all messages sent will be configured with a
	 * CR+LF prefix assuming the application level is sending the
	 * MIME body. 
	 *
	 * See \ref vortex_manual_using_mime for a long
	 * explanation. In sort, this function allows to configure if
	 * MIME headers should be added or not automatically on each
	 * message sent using the family of functions
	 * vortex_channel_send_*.
	 *
	 * The set of functions that are affected by this configuration are:
	 * 
	 *  - \ref vortex_channel_send_msg
	 *  - \ref vortex_channel_send_msgv
	 *  - \ref vortex_channel_send_msg_and_wait
	 *  - \ref vortex_channel_send_rpy
	 *  - \ref vortex_channel_send_rpyv
	 *  - \ref vortex_channel_send_err
	 *  - \ref vortex_channel_send_errv
	 *  - \ref vortex_channel_send_ans_rpy
	 *  - \ref vortex_channel_send_ans_rpyv
	 *
	 * Use the following values to configure this feature:
	 * 
	 * - 1: Enable automatic MIME handling for messages send over
	 * any channel that is not configured.
	 *
	 * - 2: Disable automatic MIME handling for channels that
	 * aren't configured.
	 */
	VORTEX_AUTOMATIC_MIME_HANDLING = 5,
	/** 
	 * @brief Allows to skip thread pool waiting on vortex ctx finalization.
	 *
	 * By default, when vortex context is finished by calling \ref
	 * vortex_exit_ctx, the function waits for all threads running
	 * the in thread pool to finish. However, under some
	 * conditions, this may cause a dead-lock problem especially
	 * when blocking operations are triggered from threads inside the
	 * pool at the time the exit operation happens.
	 *
	 * This parameter allows to signal this vortex context to not
	 * wait for threads running in the thread pool.
	 *
	 * To set the value to make vortex ctx exit to not wait for
	 * threads in the pool to finish use:
	 *
	 * \code
	 * vortex_conf_set (ctx, VORTEX_SKIP_THREAD_POOL_WAIT, axl_true, NULL);
	 * \endcode
	 */
	VORTEX_SKIP_THREAD_POOL_WAIT = 6
} VortexConfItem;

axl_bool  vortex_conf_get             (VortexCtx      * ctx,
				       VortexConfItem   item, 
				       int            * value);

axl_bool  vortex_conf_set             (VortexCtx      * ctx,
				       VortexConfItem   item, 
				       int              value, 
				       const char     * str_value);

void     _vortex_log                 (VortexCtx        * ctx,
				      const       char * file,
				      int                line,
				      VortexDebugLevel   level, 
				      const char       * message,
				      ...);

void     _vortex_log2                (VortexCtx        * ctx,
				      const       char * file,
				      int                line,
				      VortexDebugLevel   level, 
				      const char       * message, 
				      ...);

#if defined(__COMPILING_VORTEX__) && defined(__GNUC__)
/* makes gcc happy, by prototyping functions which aren't exported
 * while compiling with -ansi. Really uggly hack, please report
 * any idea to solve this issue. */
int  setenv  (const char *name, const char *value, int overwrite);
#endif

END_C_DECLS

/* @} */
#endif

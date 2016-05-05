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

#define LOG_DOMAIN "vortex-errno"

#if defined(AXL_OS_WIN32)
#define ERRNO_WSAEINTR              "10004 Interrupted function call. A blocking operation was interrupted by a call to WSACancelBlockingCall."
#define ERRNO_WSAEACCES             "10013 Permission denied. An attempt was made to access a socket in a way forbidden by its access permissions. An example is using a broadcast address for sendto without broadcast permission being set using setsockopt(SO_BROADCAST). Another possible reason for the WSAEACCES error is that when the bind function is called (on Windows NT 4 SP4 or later), another application, service, or kernel mode driver is bound to the same address with exclusive access. Such exclusive access is a new feature of Windows NT 4 SP4 and later, and is implemented by using the SO_EXCLUSIVEADDRUSE option."
#define ERRNO_WSAEFAULT             "10014 Bad address. The system detected an invalid pointer address in attempting to use a pointer argument of a call. This error occurs if an application passes an invalid pointer value, or if the length of the buffer is too small. For instance, if the length of an argument, which is a sockaddr structure, is smaller than the sizeof(sockaddr)."
#define ERRNO_WSAEINVAL             "10022 Invalid argument. Some invalid argument was supplied (for example, specifying an invalid level to the setsockopt function). In some instances, it also refers to the current state of the socket for instance, calling accept on a socket that is not listening."
#define ERRNO_WSAEMFILE             "10024 Too many open files. Too many open sockets. Each implementation may have a maximum number of socket handles available, either globally, per process, or per thread."
#define ERRNO_WSAEWOULDBLOCK        "10035 Resource temporarily unavailable. This error is returned from operations on nonblocking sockets that cannot be completed immediately, for example recv when no data is queued to be read from the socket. It is a nonfatal error, and the operation should be retried later. It is normal for WSAEWOULDBLOCK to be reported as the result from calling connect on a nonblocking SOCK_STREAM socket, since some time must elapse for the connection to be established."
#define ERRNO_WSAEINPROGRESS        "10036 Operation now in progress. A blocking operation is currently executing. Windows Sockets only allows a single blocking operation per- task or thread to be outstanding, and if any other function call is made (whether or not it references that or any other socket) the function fails with the WSAEINPROGRESS error."
#define ERRNO_WSAEALREADY           "10037 Operation already in progress. An operation was attempted on a nonblocking socket with an operation already in progress that is, calling connect a second time on a nonblocking socket that is already connecting, or canceling an asynchronous request (WSAAsyncGetXbyY) that has already been canceled or completed."
#define ERRNO_WSAENOTSOCK           "10038 Socket operation on nonsocket. An operation was attempted on something that is not a socket. Either the socket handle parameter did not reference a valid socket, or for select, a member of an fd_set was not valid."
#define ERRNO_WSAEDESTADDRREQ       "10039 Destination address required. A required address was omitted from an operation on a socket. For example, this error is returned if sendto is called with the remote address of ADDR_ANY."
#define ERRNO_WSAEMSGSIZE           "10040 Message too long. A message sent on a datagram socket was larger than the internal message buffer or some other network limit, or the buffer used to receive a datagram was smaller than the datagram itself."
#define ERRNO_WSAEPROTOTYPE         "10041 Protocol wrong type for socket. A protocol was specified in the socket function call that does not support the semantics of the socket type requested. For example, the ARPA Internet UDP protocol cannot be specified with a socket type of SOCK_STREAM."
#define ERRNO_WSAENOPROTOOPT        "10042 Bad protocol option. An unknown, invalid or unsupported option or level was specified in a getsockopt or setsockopt call."
#define ERRNO_WSAEPROTONOSUPPORT    "10043 Protocol not supported. The requested protocol has not been configured into the system, or no implementation for it exists. For example, a socket call requests a SOCK_DGRAM socket, but specifies a stream protocol."
#define ERRNO_WSAESOCKTNOSUPPORT    "10044 Socket type not supported. The support for the specified socket type does not exist in this address family. For example, the optional type SOCK_RAW might be selected in a socket call, and the implementation does not support SOCK_RAW sockets at all."
#define ERRNO_WSAEOPNOTSUPP         "10045 Operation not supported. The attempted operation is not supported for the type of object referenced. Usually this occurs when a socket descriptor to a socket that cannot support this operation is trying to accept a connection on a datagram socket."
#define ERRNO_WSAEPFNOSUPPORT       "10046 Protocol family not supported. The protocol family has not been configured into the system or no implementation for it exists. This message has a slightly different meaning from WSAEAFNOSUPPORT. However, it is interchangeable in most cases, and all Windows Sockets functions that return one of these messages also specify WSAEAFNOSUPPORT."
#define ERRNO_WSAEAFNOSUPPORT       "10047 Address family not supported by protocol family. An address incompatible with the requested protocol was used. All sockets are created with an associated address family (that is, AF_INET for Internet Protocols) and a generic protocol type (that is, SOCK_STREAM). This error is returned if an incorrect protocol is explicitly requested in the socket call, or if an address of the wrong family is used for a socket, for example, in sendto."
#define ERRNO_WSAEADDRINUSE         "10048 Address already in use. Typically, only one usage of each socket address (protocol/IP address/port) is permitted. This error occurs if an application attempts to bind a socket to an IP address/port that has already been used for an existing socket, or a socket that was not closed properly, or one that is still in the process of closing. For server applications that need to bind multiple sockets to the same port number, consider using setsockopt (SO_REUSEADDR). Client applications usually need not call bind at all  connect chooses an unused port automatically. When bind is called with a wildcard address (involving ADDR_ANY), a WSAEADDRINUSE error could be delayed until the specific address is committed. This could happen with a call to another function later, including connect, listen, WSAConnect, or WSAJoinLeaf."
#define ERRNO_WSAEADDRNOTAVAIL      "10049 Cannot assign requested address. The requested address is not valid in its context. This normally results from an attempt to bind to an address that is not valid for the local computer. This can also result from connect, sendto, WSAConnect, WSAJoinLeaf, or WSASendTo when the remote address or port is not valid for a remote computer (for example, address or port 0)."
#define ERRNO_WSAENETDOWN           "10050 Network is down. A socket operation encountered a dead network. This could indicate a serious failure of the network system (that is, the protocol stack that the Windows Sockets DLL runs over), the network interface, or the local network itself."
#define ERRNO_WSAENETUNREACH        "10051 Network is unreachable. A socket operation was attempted to an unreachable network. This usually means the local software knows no route to reach the remote host."
#define ERRNO_WSAENETRESET          "10052 Network dropped connection on reset. The connection has been broken due to keep-alive activity detecting a failure while the operation was in progress. It can also be returned by setsockopt if an attempt is made to set SO_KEEPALIVE on a connection that has already failed."
#define ERRNO_WSAECONNABORTED       "10053 Software caused connection abort. An established connection was aborted by the software in your host computer, possibly due to a data transmission time-out or protocol error."
#define ERRNO_WSAECONNRESET         "10054 Connection reset by peer. An existing connection was forcibly closed by the remote host. This normally results if the peer application on the remote host is suddenly stopped, the host is rebooted, the host or remote network interface is disabled, or the remote host uses a hard close (see setsockopt for more information on the SO_LINGER option on the remote socket). This error may also result if a connection was broken due to keep-alive activity detecting a failure while one or more operations are in progress. Operations that were in progress fail with WSAENETRESET. Subsequent operations fail with WSAECONNRESET."
#define ERRNO_WSAENOBUFS            "10055 No buffer space available. An operation on a socket could not be performed because the system lacked sufficient buffer space or because a queue was full."
#define ERRNO_WSAEISCONN            "10056 Socket is already connected. A connect request was made on an already-connected socket. Some implementations also return this error if sendto is called on a connected SOCK_DGRAM socket (for SOCK_STREAM sockets, the to parameter in sendto is ignored) although other implementations treat this as a legal occurrence."
#define ERRNO_WSAENOTCONN           "10057 Socket is not connected. A request to send or receive data was disallowed because the socket is not connected and (when sending on a datagram socket using sendto) no address was supplied. Any other type of operation might also return this error for example, setsockopt setting SO_KEEPALIVE if the connection has been reset."
#define ERRNO_WSAESHUTDOWN          "10058 Cannot send after socket shutdown. A request to send or receive data was disallowed because the socket had already been shut down in that direction with a previous shutdown call. By calling shutdown a partial close of a socket is requested, which is a signal that sending or receiving, or both have been discontinued."
#define ERRNO_WSAETIMEDOUT          "10060 Connection timed out. A connection attempt failed because the connected party did not properly respond after a period of time, or the established connection failed because the connected host has failed to respond."
#define ERRNO_WSAECONNREFUSED       "10061 Connection refused. No connection could be made because the target computer actively refused it. This usually results from trying to connect to a service that is inactive on the foreign host that is, one with no server application running."
#define ERRNO_WSAEHOSTDOWN          "10064 Host is down. A socket operation failed because the destination host is down. A socket operation encountered a dead host. Networking activity on the local host has not been initiated. These conditions are more likely to be indicated by the error WSAETIMEDOUT."
#define ERRNO_WSAEHOSTUNREACH       "10065 No route to host. A socket operation was attempted to an unreachable host. See WSAENETUNREACH."
#define ERRNO_WSAEPROCLIM           "10067 Too many processes. A Windows Sockets implementation may have a limit on the number of applications that can use it simultaneously.WSAStartup may fail with this error if the limit has been reached."
#define ERRNO_WSASYSNOTREADY        "10091 Network subsystem is unavailable. This error is returned by WSAStartup if the Windows Sockets implementation cannot function at this time because the underlying system it uses to provide network services is currently unavailable. Users should check: That the appropriate Windows Sockets DLL file is in the current path. That they are not trying to use more than one Windows Sockets implementation simultaneously. If there is more than one Winsock DLL on your system, be sure the first one in the path is appropriate for the network subsystem currently loaded. The Windows Sockets implementation documentation to be sure all necessary components are currently installed and configured correctly."
#define ERRNO_WSAVERNOTSUPPORTED    "10092 Winsock.dll version out of range. The current Windows Sockets implementation does not support the Windows Sockets specification version requested by the application. Check that no old Windows Sockets DLL files are being accessed."
#define ERRNO_WSANOTINITIALISED     "10093 Successful WSAStartup not yet performed. Either the application has not called WSAStartup or WSAStartup failed. The application may be accessing a socket that the current active task does not own (that is, trying to share a socket between tasks), or WSACleanup has been called too many times."
#define ERRNO_WSAEDISCON            "10101 Graceful shutdown in progress. Returned by WSARecv and WSARecvFrom to indicate that the remote party has initiated a graceful shutdown sequence."
#define ERRNO_WSATYPE_NOT_FOUND     "10109 Class type not found. The specified class was not found."
#define ERRNO_WSAHOST_NOT_FOUND     "11001 Host not found. No such host is known. The name is not an official host name or alias, or it cannot be found in the database(s) being queried. This error may also be returned for protocol and service queries, and means that the specified name could not be found in the relevant database."
#define ERRNO_WSATRY_AGAIN          "11002 Nonauthoritative host not found. This is usually a temporary error during host name resolution and means that the local server did not receive a response from an authoritative server. A retry at some time later may be successful."
#define ERRNO_WSANO_RECOVERY        "11003 This is a nonrecoverable error. This indicates that some sort of nonrecoverable error occurred during a database lookup. This may be because the database files (for example, BSD-compatible HOSTS, SERVICES, or PROTOCOLS files) could not be found, or a DNS request was returned by the server with a severe error."
#define ERRNO_WSANO_DATA            "11004 Valid name, no data record of requested type. The requested name is valid and was found in the database, but it does not have the correct associated data being resolved for. The usual example for this is a host name-to-address translation attempt (using gethostbyname or WSAAsyncGetHostByName) which uses the DNS (Domain Name Server). An MX record is returned but no A record indicating the host itself exists, but is not directly reachable."
#define ERRNO_WSA_INVALID_HANDLE    "OS dependent. Specified event object handle is invalid. An application attempts to use an event object, but the specified handle is not valid."
#define ERRNO_WSA_INVALID_PARAMETER "OS dependent. One or more parameters are invalid. An application used a Windows Sockets function which directly maps to a Windows function. The Windows function is indicating a problem with one or more parameters."
#define ERRNO_WSA_IO_INCOMPLETE     "OS dependent. Overlapped I/O event object not in signaled state. The application has tried to determine the status of an overlapped operation which is not yet completed. Applications that use WSAGetOverlappedResult (with the fWait flag set to FALSE) in a polling mode to determine when an overlapped operation has completed, get this error code until the operation is complete."
#define ERRNO_WSA_IO_PENDING        "OS dependent. Overlapped operations will complete later. The application has initiated an overlapped operation that cannot be completed immediately. A completion indication will be given later when the operation has been completed."
#define ERRNO_WSA_NOT_ENOUGH_MEMORY "OS dependent. Insufficient memory available. An application used a Windows Sockets function that directly maps to a Windows function. The Windows function is indicating a lack of required memory resources."
#define ERRNO_WSA_OPERATION_ABORTED "OS dependent. Overlapped operation aborted. An overlapped operation was canceled due to the closure of the socket, or the execution of the SIO_FLUSH command in WSAIoctl."
#define ERRNO_WSASYSCALLFAILURE     "OS dependent. System call failure. Generic error code, returned under various conditions. Returned when a system call that should never fail does fail. For example, if a call to WaitForMultipleEvents fails or one of the registry functions fails trying to manipulate the protocol/namespace catalogs. Returned when a provider does not return SUCCESS and does not provide an extended error code. Can indicate a service provider implementation error."
#endif 

void    vortex_errno_show_error        (VortexCtx * ctx, int  __errno)
{

#if defined(AXL_OS_WIN32)
	switch (__errno) {
	case WSAEINTR:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEINTR);
	case WSAEACCES:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEACCES);
	case WSAEFAULT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEFAULT);
	case WSAEINVAL:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEINVAL);
	case WSAEMFILE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEFAULT);
	case WSAEWOULDBLOCK:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEWOULDBLOCK);
	case WSAEINPROGRESS:        
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEINPROGRESS);
	case WSAEALREADY:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEALREADY);           
	case WSAENOTSOCK:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENOTSOCK);
	case WSAEDESTADDRREQ:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEDESTADDRREQ);
	case WSAEMSGSIZE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEMSGSIZE);           
	case WSAEPROTOTYPE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEPROTOTYPE);
	case WSAENOPROTOOPT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENOPROTOOPT);
	case WSAEPROTONOSUPPORT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEPROTONOSUPPORT);
	case WSAESOCKTNOSUPPORT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAESOCKTNOSUPPORT);
	case WSAEOPNOTSUPP:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEOPNOTSUPP);
	case WSAEPFNOSUPPORT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEPFNOSUPPORT);
	case WSAEAFNOSUPPORT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEAFNOSUPPORT);
	case WSAEADDRINUSE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEADDRINUSE);
	case WSAEADDRNOTAVAIL:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEADDRNOTAVAIL);
	case WSAENETDOWN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENETDOWN);
	case WSAENETUNREACH:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENETUNREACH);
	case WSAENETRESET:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENETRESET);
	case WSAECONNABORTED:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAECONNABORTED);
	case WSAECONNRESET:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAECONNRESET);
	case WSAENOBUFS:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENOBUFS);
	case WSAEISCONN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEISCONN);
	case WSAENOTCONN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAENOTCONN);
	case WSAESHUTDOWN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAESHUTDOWN);
	case WSAETIMEDOUT:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAETIMEDOUT);
	case WSAECONNREFUSED:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAECONNREFUSED);
	case WSAEHOSTDOWN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEHOSTDOWN);
	case WSAEHOSTUNREACH:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEHOSTUNREACH);
	case WSAEPROCLIM:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEPROCLIM);
	case WSASYSNOTREADY:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSASYSNOTREADY);
	case WSAVERNOTSUPPORTED:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAVERNOTSUPPORTED);
	case WSANOTINITIALISED:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSANOTINITIALISED);
	case WSAEDISCON:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAEDISCON);
	case WSATYPE_NOT_FOUND:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSATYPE_NOT_FOUND);     
	case WSAHOST_NOT_FOUND:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSAHOST_NOT_FOUND);
	case WSATRY_AGAIN:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSATRY_AGAIN);
	case WSANO_RECOVERY:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSANO_RECOVERY);
	case WSANO_DATA:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSANO_DATA);            
	case WSA_INVALID_HANDLE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_INVALID_HANDLE);
	case WSA_INVALID_PARAMETER:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_INVALID_PARAMETER);
	case WSA_IO_INCOMPLETE:     
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_IO_INCOMPLETE);
	case WSA_IO_PENDING:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_IO_PENDING);
	case WSA_NOT_ENOUGH_MEMORY:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_NOT_ENOUGH_MEMORY);
	case WSA_OPERATION_ABORTED:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSA_OPERATION_ABORTED);
	case WSASYSCALLFAILURE:
		vortex_log (VORTEX_LEVEL_CRITICAL,  ERRNO_WSASYSCALLFAILURE);
	}
#elif defined (AXL_OS_UNIX)
	/* show the last error */
	vortex_log (VORTEX_LEVEL_CRITICAL, strerror (__errno));
#endif
	return;
}

void    vortex_errno_show_last_error   (VortexCtx * ctx)
{
	vortex_errno_show_error (ctx, errno);

	return;
}

char  * vortex_errno_get_error         (int  __errno)
{
#if defined(AXL_OS_WIN32)
	switch (__errno) {
	case WSAEINTR:
		return ERRNO_WSAEINTR;
	case WSAEACCES:
		return ERRNO_WSAEACCES;
	case WSAEFAULT:
		return ERRNO_WSAEFAULT;
	case WSAEINVAL:
		return ERRNO_WSAEINVAL;
	case WSAEMFILE:
		return ERRNO_WSAEFAULT;
	case WSAEWOULDBLOCK:
		return ERRNO_WSAEWOULDBLOCK;
	case WSAEINPROGRESS:        
		return ERRNO_WSAEINPROGRESS;
	case WSAEALREADY:
		return ERRNO_WSAEALREADY;           
	case WSAENOTSOCK:
		return ERRNO_WSAENOTSOCK;
	case WSAEDESTADDRREQ:
		return ERRNO_WSAEDESTADDRREQ;
	case WSAEMSGSIZE:
		return ERRNO_WSAEMSGSIZE;           
	case WSAEPROTOTYPE:
		return ERRNO_WSAEPROTOTYPE;
	case WSAENOPROTOOPT:
		return ERRNO_WSAENOPROTOOPT;
	case WSAEPROTONOSUPPORT:
		return ERRNO_WSAEPROTONOSUPPORT;
	case WSAESOCKTNOSUPPORT:
		return ERRNO_WSAESOCKTNOSUPPORT;
	case WSAEOPNOTSUPP:
		return ERRNO_WSAEOPNOTSUPP;
	case WSAEPFNOSUPPORT:
		return ERRNO_WSAEPFNOSUPPORT;
	case WSAEAFNOSUPPORT:
		return ERRNO_WSAEAFNOSUPPORT;
	case WSAEADDRINUSE:
		return ERRNO_WSAEADDRINUSE;
	case WSAEADDRNOTAVAIL:
		return ERRNO_WSAEADDRNOTAVAIL;
	case WSAENETDOWN:
		return ERRNO_WSAENETDOWN;
	case WSAENETUNREACH:
		return ERRNO_WSAENETUNREACH;
	case WSAENETRESET:
		return ERRNO_WSAENETRESET;
	case WSAECONNABORTED:
		return ERRNO_WSAECONNABORTED;
	case WSAECONNRESET:
		return ERRNO_WSAECONNRESET;
	case WSAENOBUFS:
		return ERRNO_WSAENOBUFS;
	case WSAEISCONN:
		return ERRNO_WSAEISCONN;
	case WSAENOTCONN:
		return ERRNO_WSAENOTCONN;
	case WSAESHUTDOWN:
		return ERRNO_WSAESHUTDOWN;
	case WSAETIMEDOUT:
		return ERRNO_WSAETIMEDOUT;
	case WSAECONNREFUSED:
		return ERRNO_WSAECONNREFUSED;
	case WSAEHOSTDOWN:
		return ERRNO_WSAEHOSTDOWN;
	case WSAEHOSTUNREACH:
		return ERRNO_WSAEHOSTUNREACH;
	case WSAEPROCLIM:
		return ERRNO_WSAEPROCLIM;
	case WSASYSNOTREADY:
		return ERRNO_WSASYSNOTREADY;
	case WSAVERNOTSUPPORTED:
		return ERRNO_WSAVERNOTSUPPORTED;
	case WSANOTINITIALISED:
		return ERRNO_WSANOTINITIALISED;
	case WSAEDISCON:
		return ERRNO_WSAEDISCON;
	case WSATYPE_NOT_FOUND:
		return ERRNO_WSATYPE_NOT_FOUND;     
	case WSAHOST_NOT_FOUND:
		return ERRNO_WSAHOST_NOT_FOUND;
	case WSATRY_AGAIN:
		return ERRNO_WSATRY_AGAIN;
	case WSANO_RECOVERY:
		return ERRNO_WSANO_RECOVERY;
	case WSANO_DATA:
		return ERRNO_WSANO_DATA;            
	case WSA_INVALID_HANDLE:
		return ERRNO_WSA_INVALID_HANDLE;
	case WSA_INVALID_PARAMETER:
		return ERRNO_WSA_INVALID_PARAMETER;
	case WSA_IO_INCOMPLETE:     
		return ERRNO_WSA_IO_INCOMPLETE;
	case WSA_IO_PENDING:
		return ERRNO_WSA_IO_PENDING;
	case WSA_NOT_ENOUGH_MEMORY:
		return ERRNO_WSA_NOT_ENOUGH_MEMORY;
	case WSA_OPERATION_ABORTED:
		return ERRNO_WSA_OPERATION_ABORTED;
	case WSASYSCALLFAILURE:
		return ERRNO_WSASYSCALLFAILURE;
	}
	return NULL;
#elif defined(AXL_OS_UNIX)
	/* return last errno */
	return strerror (__errno);
#else
	return NULL;
#endif

}

char  * vortex_errno_get_last_error    (void)
{
	return (errno == 0) ? NULL : vortex_errno_get_error (errno);
}

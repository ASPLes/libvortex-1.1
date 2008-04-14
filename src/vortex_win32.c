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
#include <vortex.h>

#define LOG_DOMAIN "vortex-win32"

#if defined(AXL_OS_WIN32)

bool vortex_win32_init (VortexCtx * ctx)
{
	WORD wVersionRequested; 
	WSADATA wsaData; 
	int error; 
	
	wVersionRequested = MAKEWORD( 2, 2 ); 
	
	error = WSAStartup( wVersionRequested, &wsaData ); 
	if (error != NO_ERROR) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to init winsock api, exiting..");
		return false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "winsock initialization ok");
	return true;
}

BOOL APIENTRY DllMain (HINSTANCE hInst,
                       DWORD reason,
                       LPVOID reserved)
{
 
    /* always returns true because vortex init is done through
     * vortex_init */
    return true;
}

bool     __vortex_win32_blocking_socket_set (VORTEX_SOCKET socket,
                                             int           status) 
{
        unsigned long enable = status;

	return (ioctlsocket (socket, FIONBIO, &enable) == 0);
}

bool     vortex_win32_nonblocking_enable (VORTEX_SOCKET socket)
{
        return __vortex_win32_blocking_socket_set (socket, 1);
}

bool     vortex_win32_blocking_enable (VORTEX_SOCKET socket)
{
        return __vortex_win32_blocking_socket_set (socket, 0);
}

#if ! defined(HAVE_GETTIMEOFDAY)


/** 
 * @brief The function obtains the current time, expressed as seconds
 * and microseconds since the Epoch, and store it in the timeval
 * structure pointed to by tv. As posix says gettimeoday should return
 * zero and should not reserve any value for error, this function
 * returns zero.
 *
 * The timeval struct have the following members:
 *
 * \code
 * struct timeval {
 *     long tv_sec;
 *     long tv_usec;
 * } timeval;
 * \endcode
 * 
 * @param tv Timeval struct.
 * @param notUsed Not defined.
 * 
 * @return The function allways return 0.
 */
int gettimeofday(struct timeval *tv, axlPointer notUsed)
{
	union {
		long long ns100;
		FILETIME fileTime;
	} now;
	
	GetSystemTimeAsFileTime (&now.fileTime);
	tv->tv_usec = (long) ((now.ns100 / 10LL) % 1000000LL);
	tv->tv_sec = (long) ((now.ns100 - 116444736000000000LL) / 10000000LL);
	return (0);
} /* end gettimeofday */
#endif /* end ! defined(HAVE_GETTIMEOFDAY) */

#endif 

/** 
 *  PyVortex: Vortex Library Python bindings
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
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

#ifndef __PY_VORTEX_H__
#define __PY_VORTEX_H__

/* include python devel headers */
#include <Python.h>

/* include vortex headers */
#include <vortex.h>

#ifndef PyMODINIT_FUNC	/* declarations for DLL import/export */
#define PyMODINIT_FUNC void
#endif

/** 
 * @brief PyVortex macro used to cast objects to PyObject.
 */
#define __PY_OBJECT(o) ((PyObject *)o)

/** 
 * @brief Enum used to configure debug level used by the binding.
 */
typedef enum {
	/** 
	 * @brief Debug and information messages.
	 */
	PY_VORTEX_DEBUG   = 1,
	/** 
	 * @brief Warning messages
	 */
	PY_VORTEX_WARNING = 2,
	/** 
	 * @brief Critical messages.
	 */
	PY_VORTEX_CRITICAL   = 3,
} PyVortexLog;

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. 
 */
#if defined(ENABLE_PY_VORTEX_LOG)
# define py_vortex_log(l, m, ...)    do{_py_vortex_log  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define py_vortex_log2(l, m, ...)   do{_py_vortex_log2  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define py_vortex_log _py_vortex_log
#   define py_vortex_log2 _py_vortex_log2
# else
#   define py_vortex_log(l, m, ...) /* nothing */
#   define py_vortex_log2(l, m, message, ...) /* nothing */
# endif
#endif

void _py_vortex_log (const char          * file,
		     int                   line,
		     PyVortexLog           log_level,
		     const char          * message,
		     ...);

void _py_vortex_log2 (const char          * file,
		      int                   line,
		      PyVortexLog           log_level,
		      const char          * message,
		      ...);

axl_bool py_vortex_log_is_enabled       (void);

axl_bool py_vortex_log2_is_enabled      (void);

axl_bool py_vortex_color_log_is_enabled (void);

/* include other modules */
#include <py_vortex_ctx.h>
#include <py_vortex_connection.h>
#include <py_vortex_channel.h>
#include <py_vortex_async_queue.h>
#include <py_vortex_frame.h>

#endif

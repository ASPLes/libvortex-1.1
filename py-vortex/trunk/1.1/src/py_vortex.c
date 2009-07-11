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
#include <py_vortex.h>

#if defined(ENABLE_PY_VORTEX_LOG)
/** 
 * @brief Variable used to track if environment variables associated
 * to log were checked. 
 */
axl_bool _py_vortex_log_checked = axl_false;

/** 
 * @brief Boolean variable that tracks current console log status. By
 * default it is disabled.
 */
axl_bool _py_vortex_log_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current second level console
 * log status. By default it is disabled.
 */
axl_bool _py_vortex_log2_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current console color log
 * status. By default it is disabled.
 */
axl_bool _py_vortex_color_log_enabled = axl_false;
#endif


/** 
 * @brief Function that implements vortex_channel_queue_reply handler
 * used as frame received handler.
 */
static PyObject * py_vortex_queue_reply (PyVortexChannel * self, PyObject * args)
{
	PyVortexConnection  * conn    = NULL;
	PyVortexChannel     * channel = NULL;
	PyVortexFrame       * frame   = NULL;
	PyVortexAsyncQueue  * data    = NULL;
	
	/* parse and check result */
	if (! PyArg_ParseTuple (args, "OOOO", &conn, &channel, &frame, &data))
		return NULL;

	/* NOTE: do not call to vortex_channel_queue_reply because that
	 * function will store a copy of the frame received in the
	 * queue rather allow to store the PyVortexFrame reference
	 * which is what is required by queue.pop or channel.get_reply
	 * (). */
	
	/* create a frame copy and acquire a reference to it */
	frame = PY_VORTEX_FRAME ( py_vortex_frame_create (py_vortex_frame_get (frame), axl_true) );

	/* now store the frame created into the queue */
	vortex_async_queue_push (py_vortex_async_queue_get (data), frame);

	/* reply work done */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Allows to start a listener running on the address and port
 * specified.
 */
static PyObject * py_vortex_create_listener (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * host          = NULL;
	const char         * port          = NULL;
	PyObject           * py_vortex_ctx = NULL;
	VortexConnection   * listener      = NULL;
	PyObject           * py_listener;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "host", "port", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "Oss", kwlist, &py_vortex_ctx, &host, &port))
		return NULL;

	/* create a listener */
	py_vortex_log (PY_VORTEX_DEBUG, "creating listener using: %s:%s", host, port);
	listener = vortex_listener_new_full (
		/* context */
		py_vortex_ctx_get (py_vortex_ctx),
		/* host and port */
		host, port, NULL, NULL);

	if (vortex_connection_is_ok (listener, axl_false)) {
		py_vortex_log (PY_VORTEX_DEBUG, "created a listener running at: %s:%s (refs: %d, id: %d)", 
			       vortex_connection_get_host (listener),
			       vortex_connection_get_port (listener),
			       vortex_connection_ref_count (listener),
			       vortex_connection_get_id (listener));

		/* create the listener and acquire a reference to the
		 * PyVortexCtx */
		py_listener =  py_vortex_connection_create (
			/* connection reference wrapped */
			listener, 
			/* context */
			py_vortex_ctx,
			/* acquire a reference */
			axl_true,
			/* close ref on variable collect */
			axl_true);

		py_vortex_log (PY_VORTEX_DEBUG, "py_listener running at: %s:%s (refs: %d, id: %d)", 
			       vortex_connection_get_host (listener),
			       vortex_connection_get_port (listener),
			       vortex_connection_ref_count (listener),
			       vortex_connection_get_id (listener));
		
		return py_listener;
	} /* end if */
	
	/* reply work done */
	py_vortex_log (PY_VORTEX_CRITICAL, "failed to create listener, returning None..");
	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef py_vortex_methods[] = { 
	/* queue reply */
	{"queue_reply", (PyCFunction) py_vortex_queue_reply, METH_VARARGS,
	 "Implementation of vortex_channel_queue_reply. The function is used inside the queue reply method that requires this handler to be configured as frame received then to use channel.get_reply."},
	/* create_listener */
	{"create_listener", (PyCFunction) py_vortex_create_listener, METH_VARARGS | METH_KEYWORDS,
	 "Wrapper of the set of functions that allows to create a BEEP listener. The function returns a new vortex.Connection that represents a listener running on the port and address provided."},
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

/** 
 * @internal Function that inits all vortex modules and classes.
 */
PyMODINIT_FUNC initvortex(void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3 ("vortex", py_vortex_methods, 
			   "Example module that creates an extension type.");
	if (module == NULL)
		return;

	/* call to register all vortex modules and types */
	init_vortex_ctx         (module);
	init_vortex_connection  (module);
	init_vortex_channel     (module);
	init_vortex_async_queue (module);
	init_vortex_frame       (module);

	return;
}

/** 
 * @brief Allows to get current log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool py_vortex_log_is_enabled (void)
{
	/* if log is not checked, check environment variables */
	if (! _py_vortex_log_checked) {
		_py_vortex_log_checked = axl_true;

		/* check for PY_VORTEX_DEBUG */
		_py_vortex_log_enabled = vortex_support_getenv_int ("PY_VORTEX_DEBUG") > 0;

		/* check for PY_VORTEX_DEBUG_COLOR */
		_py_vortex_color_log_enabled = vortex_support_getenv_int ("PY_VORTEX_DEBUG_COLOR") > 0;
	} /* end if */

	return _py_vortex_log_enabled;
}

/** 
 * @brief Allows to get current second level log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool py_vortex_log2_is_enabled (void)
{
#if defined(ENABLE_PY_VORTEX_LOG)
	return _py_vortex_log2_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current color log enabled status.
 *
 * @return axl_true if color log is enabled, otherwise axl_false is returned.
 */
axl_bool py_vortex_color_log_is_enabled (void)
{
#if defined(ENABLE_PY_VORTEX_LOG)
	return _py_vortex_color_log_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @internal Internal common log implementation to support several
 * levels of logs.
 * 
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _py_vortex_log_common (const char          * file,
			    int                   line,
			    PyVortexLog           log_level,
			    const char          * message,
			    va_list               args)
{
#if defined(ENABLE_PY_VORTEX_LOG)
	/* if not PY_VORTEX_DEBUG FLAG, do not output anything */
	if (!py_vortex_log_is_enabled ()) 
		return;

#if defined (__GNUC__)
	if (py_vortex_color_log_is_enabled ()) 
		fprintf (stdout, "\e[1;36m(proc %d)\e[0m: ", getpid ());
	else 
#endif /* __GNUC__ */
		fprintf (stdout, "(proc %d): ", getpid ());
		
		
	/* drop a log according to the level */
#if defined (__GNUC__)
	if (py_vortex_color_log_is_enabled ()) {
		switch (log_level) {
		case PY_VORTEX_DEBUG:
			fprintf (stdout, "(\e[1;32mdebug\e[0m) ");
			break;
		case PY_VORTEX_WARNING:
			fprintf (stdout, "(\e[1;33mwarning\e[0m) ");
			break;
		case PY_VORTEX_CRITICAL:
			fprintf (stdout, "(\e[1;31mcritical\e[0m) ");
			break;
		}
	} else {
#endif /* __GNUC__ */
		switch (log_level) {
		case PY_VORTEX_DEBUG:
			fprintf (stdout, "(debug) ");
			break;
		case PY_VORTEX_WARNING:
			fprintf (stdout, "(warning) ");
			break;
		case PY_VORTEX_CRITICAL:
			fprintf (stdout, "(critical) ");
			break;
		}
#if defined (__GNUC__)
	} /* end if */
#endif
	
	/* drop a log according to the domain */
	(file != NULL) ? fprintf (stdout, "%s:%d ", file, line) : fprintf (stdout, ": ");
	
	/* print the message */
	vfprintf (stdout, message, args);
	
	fprintf (stdout, "\n");
	
	/* ensure that the log is dropped to the console */
	fflush (stdout);
#endif /* end ENABLE_PY_VORTEX_LOG */

	/* return */
	return;
}

/** 
 * @internal Log function used by py_vortex to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>py_vortex_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _py_vortex_log (const char          * file,
		     int                   line,
		     PyVortexLog           log_level,
		     const char          * message,
		     ...)
{

#ifndef ENABLE_PY_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_py_vortex_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by py_vortex to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>py_vortex_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _py_vortex_log2 (const char          * file,
		      int                   line,
		      PyVortexLog           log_level,
		      const char          * message,
		      ...)
{

#ifndef ENABLE_PY_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not PY_VORTEX_DEBUG2 FLAG, do not output anything */
	if (!py_vortex_log2_is_enabled ()) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_py_vortex_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 *  PyVortex: Vortex Library Python bindings
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
#include <py_vortex.h>

/* signal handling */
#include <signal.h>

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
	listener = vortex_listener_new_full (
		/* context */
		py_vortex_ctx_get (py_vortex_ctx),
		/* host and port */
		host, port, NULL, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "creating listener using: %s:%s (%p, status: %d)", host, port,
		       listener, __unlocked_vortex_connection_is_ok (listener, axl_false));

	/* do not check if the connection is ok, to return a different
	   value. Rather return a PyVortexConnection in all cases
	   allowing the user to call to .is_ok () */

	/* create the listener and acquire a reference to the
	 * PyVortexCtx */
	py_listener =  py_vortex_connection_create (
			/* connection reference wrapped */
			listener, 
			/* acquire a reference */
			axl_true,
			/* close ref on variable collect */
			axl_true);

	/* handle exception */
	py_vortex_handle_and_clear_exception (py_listener);

	py_vortex_log (PY_VORTEX_DEBUG, "py_listener running at: %s:%s (refs: %d, id: %d)", 
		       vortex_connection_get_host (listener),
		       vortex_connection_get_port (listener),
		       vortex_connection_ref_count (listener),
		       vortex_connection_get_id (listener));
	
	return py_listener;
}

VortexCtx * py_vortex_wait_listeners_ctx = NULL;

void py_vortex_wait_listeners_sig_handler (int _signal)
{
	VortexCtx * ctx = py_vortex_wait_listeners_ctx;

	/* nullify context */
	py_vortex_wait_listeners_ctx = NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received signal %d, unlocking (ctx: %p)", _signal, ctx);
	vortex_listener_unlock (ctx);
	return;
}

/** 
 * @brief Implementation of vortex.wait_listeners which blocks the
 * caller until all vortex library is stopped.
 */
static PyObject * py_vortex_wait_listeners (PyObject * self, PyObject * args, PyObject * kwds)
{
	
	PyObject           * py_vortex_ctx    = NULL;
	axl_bool             unlock_on_signal = axl_false;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "unlock_on_signal", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "O|i", kwlist, &py_vortex_ctx, &unlock_on_signal))
		return NULL;

	if (! py_vortex_ctx_check (py_vortex_ctx))
		return NULL;

	/* received context, increase its reference during the wait
	   operation */
	Py_INCREF (py_vortex_ctx);

	/* configure signal handling to detect signal emision */
	if (unlock_on_signal) {
		/* record ctx */
		py_vortex_wait_listeners_ctx = py_vortex_ctx_get (py_vortex_ctx);

		/* configure handler */
		signal (SIGTERM, py_vortex_wait_listeners_sig_handler);
		signal (SIGQUIT, py_vortex_wait_listeners_sig_handler);
		signal (SIGINT,  py_vortex_wait_listeners_sig_handler);
	} /* end if */

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* call to wait for listeners */
	py_vortex_log (PY_VORTEX_DEBUG, "waiting listeners to finish: %p", py_vortex_ctx_get (py_vortex_ctx));
	vortex_listener_wait (py_vortex_ctx_get (py_vortex_ctx));
	py_vortex_log (PY_VORTEX_DEBUG, "wait for listeners ended, returning");

	/* restore thread state */
	Py_END_ALLOW_THREADS

	Py_DECREF (py_vortex_ctx);
	
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Implementation of vortex.unlock_listeners which blocks the
 * caller until all vortex library is stopped.
 */
static PyObject * py_vortex_unlock_listeners (PyObject * self, PyObject * args, PyObject * kwds)
{
	
	PyObject           * py_vortex_ctx = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "O", kwlist, &py_vortex_ctx))
		return NULL;
	
	if (! py_vortex_ctx_check (py_vortex_ctx))
		return NULL;

	/* unlock the caller */
	py_vortex_log (PY_VORTEX_DEBUG, "unlocking listeners: %p", py_vortex_ctx_get (py_vortex_ctx));
	vortex_listener_unlock (py_vortex_ctx_get (py_vortex_ctx));
	
	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

void py_vortex_decref (PyObject * obj)
{
	Py_XDECREF (obj);
	return;
}

/** 
 * @internal Implementation used by py_vortex_register_profile to
 * bridge into python notifying start request.
 */
axl_bool  py_vortex_profile_start  (int                channel_num,
				    VortexConnection * conn,
				    axlPointer         user_data)
{
	PyGILState_STATE     state;
	PyObject           * py_conn;
	PyObject           * start;
	PyObject           * start_data;
	VortexChannel      * channel;
	PyObject           * args;
	PyObject           * result;
	axl_bool             _result;
	VortexCtx          * ctx = CONN_CTX (conn);

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* get a reference to the actual channel being accepted */
	channel  = vortex_connection_get_channel (conn, channel_num);

	/* get references to handlers */
	start      = py_vortex_ctx_register_get (ctx, "%s_start", vortex_channel_get_profile (channel));
	start_data = py_vortex_ctx_register_get (ctx, "%s_start_data", vortex_channel_get_profile (channel));

	/* provide a default value */
	if (start_data == NULL)
		start_data = Py_None;

	/* create a PyVortexConnection instance */
	py_conn  = py_vortex_connection_find_reference (conn);

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);

	/* the following function PyTuple_SetItem "steals" a reference
	 * which is the python way to say that we are transfering the
	 * ownership of the reference to that function, making it
	 * responsible of calling to Py_DECREF when required. */
	PyTuple_SetItem (args, 0, Py_BuildValue ("i", channel_num));
	PyTuple_SetItem (args, 1, py_conn);

	/* increment reference counting because the tuple will
	 * decrement the reference passed when he thinks it is no
	 * longer used. */
	Py_INCREF (start_data);
	PyTuple_SetItem (args, 2, start_data);

	/* record handler */
	START_HANDLER (start);

	/* now invoke */
	result = PyObject_Call (start, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (start);
	
	py_vortex_log (PY_VORTEX_DEBUG, "channel start notification finished, checking for exceptions..");
	py_vortex_handle_and_clear_exception (py_conn);

	/* translate result */
	_result = axl_false;
	if (result) {
		PyArg_Parse (result, "i", &_result);
		py_vortex_log (PY_VORTEX_DEBUG, "channel start notification result: %d..", _result);
	} /* end if */

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* release the GIL */
	PyGILState_Release(state);
	
	return _result;
}

/** 
 * @internal Implementation used by py_vortex_register_profile to
 * bridge into python notifying close request.
 */
axl_bool py_vortex_profile_close (int                channel_num,
				  VortexConnection * connection,
				  axlPointer         user_data)
{
	return axl_true;
}

/** 
 * @internal Implementation used by py_vortex_register_profile to
 * bridge into python notifying frame received.
 */
void py_vortex_profile_frame_received (VortexChannel    * channel,
				       VortexConnection * conn,
				       VortexFrame      * frame,
				       axlPointer         user_data)
{
	PyGILState_STATE     state;
	PyObject           * py_frame;
	PyObject           * py_channel;
	PyObject           * py_conn;
	PyObject           * frame_received;
	PyObject           * frame_received_data;
	PyObject           * args;
	PyObject           * result;
	VortexCtx          * ctx = CONN_CTX (conn);

	/* acquire the GIL */
	state    = PyGILState_Ensure();

	/* create a PyVortexFrame instance */
	py_frame = py_vortex_frame_create (frame, axl_true);

	/* get references to handlers */
	frame_received      = py_vortex_ctx_register_get (ctx, "%s_frame_received", vortex_channel_get_profile (channel));
	frame_received_data = py_vortex_ctx_register_get (ctx, "%s_frame_received_data", vortex_channel_get_profile (channel));
	py_vortex_log (PY_VORTEX_DEBUG, "frame received handler %p, data %p", frame_received, frame_received_data);

	/* set to none rather than NULL */
	if (frame_received_data == NULL)
		frame_received_data = Py_None;

	/* create a connection, acquire_ref=axl_true, close_ref=axl_false */
	py_conn  = py_vortex_connection_create (conn, axl_true, axl_false);

	/* create the channel */
	py_channel = py_vortex_channel_create (channel);
	py_vortex_log (PY_VORTEX_DEBUG, "notifying frame received over channel: %d", vortex_channel_get_number (channel));

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);

	/* the following function PyTuple_SetItem "steals" a reference
	 * which is the python way to say that we are transfering the
	 * ownership of the reference to that function, making it
	 * responsible of calling to Py_DECREF when required. */
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, py_channel);
	PyTuple_SetItem (args, 2, py_frame);

	/* increment reference counting because the tuple will
	 * decrement the reference passed when he thinks it is no
	 * longer used. */
	Py_INCREF (frame_received_data);
	PyTuple_SetItem (args, 3, frame_received_data);

	/* record handler */
	START_HANDLER (frame_received);

	/* now invoke */
	result = PyObject_Call (frame_received, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (frame_received);
	
	py_vortex_log (PY_VORTEX_DEBUG, "frame notification finished, checking for exceptions..");
	py_vortex_handle_and_clear_exception (py_conn);

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* release the GIL */
	PyGILState_Release(state);

	return;
}

/** 
 * @brief Allows to register a profile and its associated handlers
 * that will be used for incoming requests.
 */
static PyObject * py_vortex_register_profile (PyObject * self, PyObject * args, PyObject * kwds)
{
	const char         * uri           = NULL;
	PyObject           * py_vortex_ctx = NULL;

	PyObject           * start         = NULL;
	PyObject           * start_data    = NULL;

	PyObject           * close         = NULL;
	PyObject           * close_data    = NULL;

	PyObject           * frame_received       = NULL;
	PyObject           * frame_received_data  = NULL;
	VortexCtx          * ctx = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "uri", "start", "start_data", "close", "close_data", "frame_received", "frame_received_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "Os|OOOOOO", kwlist, 
					   &py_vortex_ctx, &uri, 
					   &start, &start_data, 
					   &close, &close_data, 
					   &frame_received, &frame_received_data))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to register profile %s", uri);
	/* check object received is a py_vortex_ctx */
	if (! py_vortex_ctx_check (py_vortex_ctx)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Expected to receive vortex.Ctx object but found something else..");
		return NULL;
	}

	/* get the VortexCtx */
	ctx = py_vortex_ctx_get (py_vortex_ctx);

	/* check handlers defined */
	if (start != NULL && ! PyCallable_Check (start)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	if (close != NULL && ! PyCallable_Check (close)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	if (frame_received != NULL && ! PyCallable_Check (frame_received)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "calling to register %s, frame_received=%p, frame_received_data=%p", uri,
		       frame_received, frame_received_data);

	/* acquire a reference to the register content */
	py_vortex_ctx_register (ctx, start, "%s_start", uri);
	py_vortex_ctx_register (ctx, start_data, "%s_start_data", uri);

	py_vortex_ctx_register (ctx, close, "%s_close", uri);
	py_vortex_ctx_register (ctx, close_data, "%s_close_data", uri);

	py_vortex_ctx_register (ctx, frame_received, "%s_frame_received", uri);
	py_vortex_ctx_register (ctx, frame_received_data, "%s_frame_received_data", uri);
	
	/* call to register */
	if (! vortex_profiles_register (ctx,
					uri,
					/* start */
					start ? py_vortex_profile_start : NULL, 
					NULL,
					/* close */
					close ? py_vortex_profile_close : NULL, 
					NULL,
					/* frame_received */
					frame_received ? py_vortex_profile_frame_received : NULL, 
					NULL)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failure found while registering %s at vortex_profiles_register", uri);
		return NULL;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "acquiring references to handlers and objects..");

	/* reply work done */
	py_vortex_log (PY_VORTEX_DEBUG, "registered beep uri: %s", uri);
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
	/* wait_listeners */
	{"wait_listeners", (PyCFunction) py_vortex_wait_listeners, METH_VARARGS | METH_KEYWORDS,
	 "Direct wrapper for vortex_listener_wait. This function is optional and it is used at the listener side to make the main thread to not finish after all vortex initialization."},
	{"unlock_listeners", (PyCFunction) py_vortex_unlock_listeners, METH_VARARGS | METH_KEYWORDS,
	 "Direct wrapper for vortex_listener_unlock. This function allows to unlock the thread that is blocked at vortex.wait_listeners."},
	
	/* register_profile */
	{"register_profile", (PyCFunction) py_vortex_register_profile, METH_VARARGS | METH_KEYWORDS,
	 "Function that allows to register a profile with its associated handlers (frame received, channel start and channel close)."},
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

/** 
 * @internal Function that inits all vortex modules and classes.
 */ 
PyMODINIT_FUNC  initlibpy_vortex_11 (void)
{
	PyObject * module;

	/** 
	 * NOTE: it seems the previous call is not the appropriate way
	 * but there are relevant people that do not think so:
	 *
	 * http://fedoraproject.org/wiki/Features/PythonEncodingUsesSystemLocale
	 *
	 * Our appreciation is that python should take care of the
	 * current system locale to translate unicode content into
	 * const char strings, for those Py_ParseTuple and Py_BuildArg
	 * using s and z, rather forcing people to get into these
	 * hacks which are problematic. 
	 */
	PyUnicode_SetDefaultEncoding ("UTF-8");

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3 ("libpy_vortex_11", py_vortex_methods, 
				 "Base module that include core BEEP elements implemented by the Vortex base library");
	if (module == NULL) 
		return;

	/* call to register all vortex modules and types */
	init_vortex_ctx          (module);
	init_vortex_connection   (module);
	init_vortex_channel      (module);
	init_vortex_async_queue  (module);
	init_vortex_frame        (module);
	init_vortex_channel_pool (module);
	init_vortex_handle       (module);

	return;
}

/** 
 * @brief Allows to get current log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool py_vortex_log_is_enabled (void)
{
#if defined(ENABLE_PY_VORTEX_LOG)
	/* if log is not checked, check environment variables */
	if (! _py_vortex_log_checked) {
		_py_vortex_log_checked = axl_true;

		/* check for PY_VORTEX_DEBUG */
		_py_vortex_log_enabled = vortex_support_getenv_int ("PY_VORTEX_DEBUG") > 0;

		/* check for PY_VORTEX_DEBUG_COLOR */
		_py_vortex_color_log_enabled = vortex_support_getenv_int ("PY_VORTEX_DEBUG_COLOR") > 0;
	} /* end if */

	return _py_vortex_log_enabled;
#else
	return axl_false;
#endif
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

/** 
 * @internal Handler used to notify exception catched and handled by
 * py_vortex_handle_and_clear_exception.
 */
PyVortexExceptionHandler py_vortex_exception_handler = NULL;

/** 
 * @brief Allows to check, handle and clear exception state.
 */ 
axl_bool py_vortex_handle_and_clear_exception (PyObject * py_conn)
{
	PyObject * ptype      = NULL;
	PyObject * pvalue     = NULL;
	PyObject * ptraceback = NULL;
	PyObject * list;
	PyObject * string;
	PyObject * mod;
	int        iterator;
	char     * str;
	char     * str_aux;
	axl_bool   found_error = axl_false;


	/* check exception */
	if (PyErr_Occurred()) {
		/* notify error found */
		found_error = axl_true;

		py_vortex_log (PY_VORTEX_CRITICAL, "found exception...handling..");

		/* fetch exception state */
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		/* import traceback module */
		mod = PyImport_ImportModule("traceback");
		if (! mod) {
			py_vortex_log (PY_VORTEX_CRITICAL, "failed to import traceback module, printing error to console");
			/* print exception */
			PyErr_Print ();
			goto clean_up;
		} /* end if */

		/* list of backtrace items */
		py_vortex_log (PY_VORTEX_CRITICAL, "formating exception: ptype:%p  pvalue:%p  ptraceback:%p",
			       ptype, pvalue, ptraceback);

		/* check ptraceback */
		if (ptraceback == NULL) {
			ptraceback = Py_None;
			Py_INCREF (Py_None);
		} /* end if */

		/* check pvalue */
		if (pvalue == NULL) {
			pvalue = Py_None;
			Py_INCREF (Py_None);
		} /* end if */

		list     = PyObject_CallMethod (mod, "format_exception", "OOO", ptype,  pvalue, ptraceback);
		iterator = 0;
		if (py_vortex_exception_handler)
			str      = axl_strdup ("");
		else
			str      = axl_strdup ("PyVortex found exception inside: \n");
		while (iterator < PyList_Size (list)) {
			/* get the string */
			string  = PyList_GetItem (list, iterator);

			str_aux = str;
			str     = axl_strdup_printf ("%s%s", str_aux, PyString_AsString (string));
			axl_free (str_aux);

			/* next iterator */
			iterator++;
		}

		/* drop a log */
		if (py_vortex_exception_handler) {
			/* remove trailing \n */
			str[strlen (str) - 1] = 0;

			/* clear all % to avoid printf problems to the
			   handler */
			iterator = 0;
			while (str[iterator])  {
				if (str[iterator] == '%')
					str[iterator] = '#';
				iterator++;
			} /* end while */

			/* allow other threads to enter into the python space */
			Py_BEGIN_ALLOW_THREADS

			py_vortex_exception_handler (str);

			/* restore thread state */
			Py_END_ALLOW_THREADS
			

#if defined(ENABLE_PY_VORTEX_LOG)
		} else if (_py_vortex_log_enabled) {
			str[strlen (str) - 1] = 0;
			py_vortex_log (PY_VORTEX_CRITICAL, str);
#endif
		} else {
			fprintf (stdout, "%s", str);
		}
		/* free message */
		axl_free (str);

		/* create an empty string \n */
		Py_XDECREF (list);
		Py_DECREF (mod);


	clean_up:
		/* call to finish retrieved vars .. */
		Py_XDECREF (ptype);
		Py_XDECREF (pvalue);
		Py_XDECREF (ptraceback);

		/* check connection reference and its role to close it
		 * in the case we are working at the listener side */
		if (py_conn && (vortex_connection_get_role (py_vortex_connection_get (py_conn)) != VortexRoleInitiator)) {
			/* shutdown connection due to unhandled exception found */
			py_vortex_log (PY_VORTEX_CRITICAL, "shutting down connection due to unhandled exception found");
			Py_DECREF ( py_vortex_connection_shutdown (PY_VORTEX_CONNECTION (py_conn)) );
		}
		

	} /* end if */

	/* clear exception */
	PyErr_Clear ();

	return found_error;
}

/** 
 * @brief Allows to configure a handler that will receive the
 * exception message created. This is useful inside the context of
 * PyVortex and Turbulence because exception propagated from python
 * into the c space are handled by \ref
 * py_vortex_handle_and_clear_exception which support notifying the
 * entire exception message on the handler here configured.
 *
 * @param handler The handler to configure to receive all exception
 * handling.
 */
void     py_vortex_set_exception_handler (PyVortexExceptionHandler handler)
{
	/* configure the handler */
	py_vortex_exception_handler = handler;

	return;
}

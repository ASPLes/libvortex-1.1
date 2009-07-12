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

/** 
 * @brief Implementation of vortex.wait_listeners which blocks the
 * caller until all vortex library is stopped.
 */
static PyObject * py_vortex_wait_listeners (PyObject * self, PyObject * args)
{
	
	PyObject           * py_vortex_ctx = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_vortex_ctx))
		return NULL;

	/* received context, increase its reference during the wait
	   operation */
	Py_INCREF (py_vortex_ctx);

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* call to wait for listeners */
	py_vortex_log (PY_VORTEX_DEBUG, "waiting listeners to finish");
	vortex_listener_wait (py_vortex_ctx_get (py_vortex_ctx));
	py_vortex_log (PY_VORTEX_DEBUG, "wait for listeners ended, returning");

	/* restore thread state */
	Py_END_ALLOW_THREADS

	Py_DECREF (py_vortex_ctx);
	
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
axl_bool py_vortex_profile_start (int                channel_num,
				  VortexConnection * conn,
				  axlPointer         user_data)
{
	return axl_true;
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

#define PY_VORTEX_REGISTER_PROFILE_ITEM(s, o) do {                                       \
	if (o) {                                                                         \
	        Py_INCREF (o);                                                           \
		vortex_ctx_set_data_full (py_vortex_ctx_get (py_vortex_ctx),             \
					  /* key and value */                            \
					  axl_strdup_printf (s, uri), o,                 \
					  /* destroy functions */                        \
					  axl_free, (axlDestroyFunc) py_vortex_decref);  \
	}                                                                                \
} while (0);

#define PY_VORTEX_REGISTER_PROFILE_ITEM_GET(obj, s, channel, py_ctx) do {                \
         tmp_str = axl_strdup_printf (s, vortex_channel_get_profile (channel));          \
         obj     = vortex_ctx_get_data (py_vortex_ctx_get (py_ctx), tmp_str);	         \
         axl_free (tmp_str);                                                             \
} while (0);

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
	PyObject           * py_ctx;
	PyObject           * py_conn;
	PyObject           * frame_received;
	PyObject           * frame_received_data;
	PyObject           * args;
	char               * tmp_str;
	PyObject           * result;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a PyVortexFrame instance */
	py_frame = py_vortex_frame_create (frame, axl_true);
	
	/* create a PyVortexConnection instance */
        py_ctx   = py_vortex_ctx_create (vortex_connection_get_ctx (conn));

	/* get references to handlers */
	PY_VORTEX_REGISTER_PROFILE_ITEM_GET (frame_received,      "%s_frame_received", channel, py_ctx);
	PY_VORTEX_REGISTER_PROFILE_ITEM_GET (frame_received_data, "%s_frame_received_data", channel, py_ctx);

	/* set to none rather than NULL */
	if (frame_received_data == NULL)
		frame_received_data = Py_None;

	py_conn  = py_vortex_connection_create (
		/* connection to wrap */
		conn, 
		/* context: create a copy */
		py_ctx,
		/* acquire a reference to the connection */
		axl_true,  
		/* do not close the connection when the reference is collected, close_ref=axl_false */
		axl_false);

	/* create the channel */
	py_channel = py_vortex_channel_create (channel, py_conn);

	/* decrement py_ctx reference since it is now owned by py_conn */
	Py_DECREF (py_ctx);

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

	/* now invoke */
	result = PyObject_Call (frame_received, args, NULL);
	
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

	/* check handlers defined */
	if (start != NULL && ! PyCallable_Check (start)) {
		py_vortex_log (PY_VORTEX_DEBUG, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	if (close != NULL && ! PyCallable_Check (close)) {
		py_vortex_log (PY_VORTEX_DEBUG, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	if (frame_received != NULL && ! PyCallable_Check (frame_received)) {
		py_vortex_log (PY_VORTEX_DEBUG, "defined start handler but received a non callable object, unable to register %s", uri);
		return NULL;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "calling to register %s", uri);
	
	/* call to register */
	if (! vortex_profiles_register (py_vortex_ctx_get (py_vortex_ctx),
					uri,
					/* start */
					start ? py_vortex_profile_start : NULL, 
					start ? start_data : NULL,
					/* close */
					close ? py_vortex_profile_close : NULL, 
					close ? close_data : NULL,
					/* frame_received */
					frame_received ? py_vortex_profile_frame_received : NULL, 
					frame_received ? frame_received_data : NULL)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failure found while registering %s at vortex_profiles_register", uri);
		return NULL;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "acquiring references to handlers and objects..");

	/* acquire a reference to the register content */
	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_start", start);
	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_start_data", start_data);

	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_close", close);
	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_close_data", close_data);

	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_frame_received", frame_received);
	PY_VORTEX_REGISTER_PROFILE_ITEM ("%s_frame_received_data", frame_received_data);
	
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
	{"wait_listeners", (PyCFunction) py_vortex_wait_listeners, METH_VARARGS,
	 "Direct wrapper for vortex_listener_wait. This function is optional and it is used at the listener side to make the main thread to not finish after all vortex initialization."},
	/* register_profile */
	{"register_profile", (PyCFunction) py_vortex_register_profile, METH_VARARGS | METH_KEYWORDS,
	 "Function that allows to register a profile with its associated handlers (frame received, channel start and channel close)."},
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

/** 
 * @brief Allows to check, handle and clear exception state.
 */ 
void py_vortex_handle_and_clear_exception (PyObject * py_conn)
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


	/* check exception */
	if (PyErr_Occurred()) {
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
		list     = PyObject_CallMethod (mod, "format_exception", "OOO", ptype,  pvalue, ptraceback);
		iterator = 0;
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
		py_vortex_log (PY_VORTEX_CRITICAL, str);
		axl_free (str);

		/* create an empty string \n */
		Py_DECREF (list);
		Py_DECREF (mod);


	clean_up:
		/* call to finish retrieved vars .. */
		Py_XDECREF (ptype);
		Py_XDECREF (pvalue);
		Py_XDECREF (ptraceback);

		if (py_conn) {
			/* shutdown connection due to unhandled exception found */
			py_vortex_log (PY_VORTEX_CRITICAL, "shutting down connection due to unhandled exception found");
			Py_DECREF ( py_vortex_connection_shutdown (PY_VORTEX_CONNECTION (py_conn)) );
		}
		

	} /* end if */

	/* clear exception */
	PyErr_Clear ();
	return;
}

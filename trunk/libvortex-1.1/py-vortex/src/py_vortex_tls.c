/** 
 *  PyVortex: Vortex Library Python bindings
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

/* include base library */
#include <py_vortex_tls.h>

/* include vortex tls support */
#include <vortex_tls.h>

/** 
 * @brief Allows to init vortex.tls module.
 */
static PyObject * py_vortex_tls_init (PyObject * self, PyObject * args)
{
	PyObject * py_ctx = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_ctx))
		return NULL;

	/* check connection object */
	if (! py_vortex_ctx_check (py_ctx)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Ctx object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("i", vortex_tls_init (py_vortex_ctx_get (py_ctx)));
}

typedef struct _PyVortexTlsDoNotify {
	PyObject * tls_notify;
	PyObject * tls_notify_data;
	PyObject * py_conn;
} PyVortexTlsDoNotify;

void py_vortex_tls_do_notify           (VortexConnection * connection,
					VortexStatus       status,
					char             * status_message,
					axlPointer         user_data)
{
	PyVortexTlsDoNotify    * data = user_data;
	PyObject               * args;
	PyObject               * _result;
	PyGILState_STATE         state;
	PyObject               * py_conn;
	PyObject               * py_ctx;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to notify TLS termination status");

	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create the connection wrapper */
	py_ctx  = py_vortex_connection_get_ctx (data->py_conn);
	py_conn = py_vortex_connection_create (
		/* connection */
		connection,
		/* the context */
		py_ctx, 
		/* acquire ref */
		axl_true,
		/* close the connection when finished */
		axl_true);

	/* remove reference added by previous call because the
	 * connection is already owned by py_conn */
	vortex_connection_unref (connection, "py_vortex_tls_do_notify");

	/* nullify internal reference of the old connection */
	py_vortex_connection_nullify (data->py_conn);
	Py_DECREF (data->py_conn);

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("i", status));
	PyTuple_SetItem (args, 2, Py_BuildValue ("s", status_message));
	PyTuple_SetItem (args, 3, data->tls_notify_data);
	
	/* perform call */
	_result = PyObject_Call (data->tls_notify, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "TLS termination status finished, checking for exceptions, _result: %p..", _result);
	py_vortex_handle_and_clear_exception (py_conn);

	Py_XDECREF (_result);
	Py_DECREF (args);

	/* terminate PyVortexTlsDoNotify: do not terminate py_conn or
	 * tls_notify_data, this is already done by previous tuple
	 * deallocation */
	Py_DECREF (data->tls_notify);
	axl_free (data);

	/* release the GIL */
	PyGILState_Release(state);

	return;
}

static PyObject * py_vortex_tls_start_tls (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject            * py_conn             = NULL;
	PyObject            * tls_notify          = NULL;
	PyObject            * tls_notify_data     = Py_None;
	const char          * serverName          = NULL;
	PyVortexTlsDoNotify * tls_data            = NULL;
	VortexConnection    * conn;
	VortexStatus          status              = VortexError;
	char                * status_msg          = NULL;
	int                   conn_id;
	PyObject            * result;

	/* now parse arguments */
	static char *kwlist[] = {"conn", "serverName", "tls_notify", "tls_notify_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|sOO", kwlist, 
					  &py_conn, &serverName, &tls_notify, &tls_notify_data))
		return NULL;

	/* check connection object */
	if (! py_vortex_connection_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Connection object but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (tls_notify != NULL && ! PyCallable_Check (tls_notify)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for tls_notify handler, but received something different.");
		return NULL;
	} /* end if */

	/* ok, now begin auth setting a notification handler if the
	 * user defined it */
	if (tls_notify != NULL) {
		tls_data = axl_new (PyVortexTlsDoNotify, 1);
		
		/* set auth_notify */
		tls_data->tls_notify = tls_notify;
		Py_INCREF (tls_notify);

		/* set auth_notify_data */
		tls_data->tls_notify_data = tls_notify_data;
		Py_INCREF (tls_notify_data);

		/* set py_conn */
		tls_data->py_conn = py_conn;
		Py_INCREF (py_conn);

		/* call to start tls process */
		vortex_tls_start_negotiation (py_vortex_connection_get (py_conn), serverName, py_vortex_tls_do_notify, tls_data);
		
		/* finished */
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* get connection id */
	conn_id = vortex_connection_get_id (py_vortex_connection_get (py_conn));
	
	/* nullify connection content to make the reference only owned
	 * by the current code */
	conn   = py_vortex_connection_get (py_conn);
	py_vortex_connection_nullify (py_conn);

	/* call to authenticate in a blocking manner */
	conn    = vortex_tls_start_negotiation_sync (conn, serverName, &status, &status_msg);

	/* create the connection or increase reference in the case the
	 * same connection is returned */
	if (conn_id == vortex_connection_get_id (conn)) {
		Py_INCREF (py_conn);
	} else {
		py_conn = py_vortex_connection_create (conn, 
						       py_vortex_connection_get_ctx (py_conn),
						       /* acquire reference */
						       axl_true,
						       /* close connection on deallocation */
						       axl_true);

		/* decrease internal reference counting */
		vortex_connection_unref (conn, "py_vortex_tls_start_tls");
	} /* end if */
	
	
	/* create a tuple to contain arguments */
	result = PyTuple_New (3);
	PyTuple_SetItem (result, 0, py_conn);
	PyTuple_SetItem (result, 1, Py_BuildValue ("i", status));
	PyTuple_SetItem (result, 2, Py_BuildValue ("s", status_msg));
	
	return result;
}

#define PY_VORTEX_TLS_DATA "py:vo:tl:da"

typedef struct _PyVortexTlsAcceptData {
	PyObject     * ctx;
	PyObject     * accept_handler;
	PyObject     * accept_handler_data;
	PyObject     * cert_handler;
	PyObject     * cert_handler_data;
	PyObject     * key_handler;
	PyObject     * key_handler_data;
} PyVortexTlsAcceptData;

void py_vortex_tls_data_free (PyVortexTlsAcceptData * data)
{
	if (data == NULL)
		return;

	/* unref handlers (if defined) */
	Py_XDECREF (data->accept_handler);
	Py_XDECREF (data->accept_handler_data);

	Py_XDECREF (data->cert_handler);
	Py_XDECREF (data->cert_handler_data);

	Py_XDECREF (data->key_handler);
	Py_XDECREF (data->key_handler_data);

	/* free the node itself */
	axl_free (data);
	return;
}

axl_bool  py_vortex_tls_accept_handler_bridge (VortexConnection * connection,
					       const char       * serverName)
{
	VortexCtx              * ctx;
	PyVortexTlsAcceptData  * data;
	PyObject               * py_conn;
	PyObject               * args;
	PyObject               * _result;
	axl_bool                 result = axl_false;
	PyGILState_STATE         state;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to handle tls query accept for serverName=%s", 
		       serverName ? serverName : "none");

	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* get data pointer */
	ctx  = vortex_connection_get_ctx (connection);
	data = vortex_ctx_get_data (ctx, PY_VORTEX_TLS_DATA);

	/* create the connection */
	py_conn = py_vortex_connection_find_reference (
		/* the connection */
		connection,
		/* the context */
		data->ctx);

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("s", serverName));
	PyTuple_SetItem (args, 2, data->accept_handler_data);
	Py_INCREF (data->accept_handler_data);
	
	/* perform call */
	_result = PyObject_Call (data->accept_handler, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "tls query accept handling finished, checking for exceptions, _result: %p..", _result);
	py_vortex_handle_and_clear_exception (py_conn);

	/* get result to return */
	if (_result != NULL && ! PyArg_Parse (_result, "i", &result)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to parse result get from tls accept handler");
		py_vortex_handle_and_clear_exception (py_conn);
		return axl_false;
	}

	Py_XDECREF (_result);
	Py_DECREF (args);

	/* release the GIL */
	PyGILState_Release(state);

	py_vortex_log (PY_VORTEX_DEBUG, "tls accept query result: %d", result);

	return result;
}

char * py_vortex_tls_cert_handler_bridge (VortexConnection * connection,
					  const char       * serverName)
{
	VortexCtx              * ctx;
	PyVortexTlsAcceptData  * data;
	PyObject               * py_conn;
	PyObject               * args;
	PyObject               * _result;
	char                   * result = NULL;
	PyGILState_STATE         state;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to return certificate for serverName=%s", 
		       serverName ? serverName : "none");

	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* get data pointer */
	ctx  = vortex_connection_get_ctx (connection);
	data = vortex_ctx_get_data (ctx, PY_VORTEX_TLS_DATA);

	/* create the connection */
	py_conn = py_vortex_connection_find_reference (
		/* the connection */
		connection,
		/* the context */
		data->ctx);

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("s", serverName));
	PyTuple_SetItem (args, 2, data->cert_handler_data);
	Py_INCREF (data->cert_handler_data);
	
	/* perform call */
	_result = PyObject_Call (data->cert_handler, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "cert handling finished, checking for exceptions, _result: %p..", _result);
	py_vortex_handle_and_clear_exception (py_conn);

	/* get result to return */
	if (_result != NULL && ! PyArg_Parse (_result, "z", &result)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to parse result get from tls accept handler");
		py_vortex_handle_and_clear_exception (py_conn);
		return NULL;
	}

	Py_XDECREF (_result);
	Py_DECREF (args);

	/* release the GIL */
	PyGILState_Release(state);

	py_vortex_log (PY_VORTEX_DEBUG, "tls cert query result: %s", result);

	return axl_strdup (result);
}

char * py_vortex_tls_key_handler_bridge (VortexConnection * connection,
					 const char       * serverName)
{
	VortexCtx              * ctx;
	PyVortexTlsAcceptData  * data;
	PyObject               * py_conn;
	PyObject               * args;
	PyObject               * _result;
	char                   * result = NULL;
	PyGILState_STATE         state;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to return keyificate for serverName=%s", 
		       serverName ? serverName : "none");

	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* get data pointer */
	ctx  = vortex_connection_get_ctx (connection);
	data = vortex_ctx_get_data (ctx, PY_VORTEX_TLS_DATA);

	/* create the connection */
	py_conn = py_vortex_connection_find_reference (
		/* the connection */
		connection,
		/* the context */
		data->ctx);

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("s", serverName));
	PyTuple_SetItem (args, 2, data->key_handler_data);
	Py_INCREF (data->key_handler_data);
	
	/* perform call */
	_result = PyObject_Call (data->key_handler, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "key handling finished, checking for exceptions, _result: %p..", _result);
	py_vortex_handle_and_clear_exception (py_conn);

	/* get result to return */
	if (_result != NULL && ! PyArg_Parse (_result, "z", &result)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to parse result get from tls accept handler");
		py_vortex_handle_and_clear_exception (py_conn);
		return NULL;
	}

	Py_XDECREF (_result);
	Py_DECREF (args);

	/* release the GIL */
	PyGILState_Release(state);

	py_vortex_log (PY_VORTEX_DEBUG, "tls key query result: %s", result);

	return axl_strdup (result);
}

static PyObject * py_vortex_tls_accept_tls (PyObject * self, 
					    PyObject * args, 
					    PyObject * kwds)
{
	PyObject               * py_ctx              = NULL;
	PyObject               * accept_handler      = NULL;
	PyObject               * accept_handler_data = Py_None;
	PyObject               * cert_handler        = NULL;
	PyObject               * cert_handler_data   = Py_None;
	PyObject               * key_handler         = NULL;
	PyObject               * key_handler_data    = Py_None;
	PyVortexTlsAcceptData  * data;
	axl_bool                 result;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "accept_handler", "accept_handler_data", "cert_handler", "cert_handler_data", "key_handler", "key_handler_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|OOOOOO", kwlist, 
					  &py_ctx,
					  &accept_handler, &accept_handler_data, 
					  &cert_handler, &cert_handler_data, 
					  &key_handler, &key_handler_data))
		return NULL;

	/* check context object received */
	if (! py_vortex_ctx_check (py_ctx)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Ctx object but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (accept_handler != NULL && ! PyCallable_Check (accept_handler)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for accept_handler handler, but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (cert_handler != NULL && ! PyCallable_Check (cert_handler)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for cert_handler handler, but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (key_handler != NULL && ! PyCallable_Check (key_handler)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for key_handler handler, but received something different.");
		return NULL;
	} /* end if */

	data  = axl_new (PyVortexTlsAcceptData, 1);
	/* set accept_handler */
	data->accept_handler = accept_handler;
	if (data->accept_handler)
		Py_INCREF (data->accept_handler);
	data->accept_handler_data = accept_handler_data;
	Py_INCREF (data->accept_handler_data);

	/* set cert_handler */
	data->cert_handler = cert_handler;
	if (data->cert_handler)
		Py_INCREF (data->cert_handler);
	data->cert_handler_data = cert_handler_data;
	Py_INCREF (data->cert_handler_data);

	/* set key_handler */
	data->key_handler = key_handler;
	if (data->key_handler)
		Py_INCREF (data->key_handler);
	data->key_handler_data = key_handler_data;
	Py_INCREF (data->key_handler_data);

	/* set py_ctx */
	data->ctx      = py_ctx;

	/* set the data into the current context */
	vortex_ctx_set_data_full (
		py_vortex_ctx_get (py_ctx),
		/* key and value */
		PY_VORTEX_TLS_DATA, data,
		/* destroy functions */
		NULL, (axlDestroyFunc) py_vortex_tls_data_free);

	/* call to accept incoming TLS requests */
	result = vortex_tls_accept_negotiation (
		/* the context */
		py_vortex_ctx_get (py_ctx), 
		/* accept handler if defined python one */
		data->accept_handler ? py_vortex_tls_accept_handler_bridge : NULL,
		/* cert handler if defined python one */
		data->cert_handler ? py_vortex_tls_cert_handler_bridge : NULL,
		/* key handler if defined python one */
		data->key_handler ? py_vortex_tls_key_handler_bridge : NULL);
	
	/* configure bridge handlers */
	return Py_BuildValue ("i", result);
}

static PyMethodDef py_vortex_tls_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_vortex_tls_init, METH_VARARGS,
	 "Inits vortex.tls module (a wrapper to vortex_tls_init function)."},
	{"start_tls", (PyCFunction) py_vortex_tls_start_tls, METH_VARARGS | METH_KEYWORDS,
	 "Starts TLS process on the provided connection (vortex_tls_start_negotiation wrapper)."},
	{"accept_tls", (PyCFunction) py_vortex_tls_accept_tls, METH_VARARGS | METH_KEYWORDS,
	 "Enables accepting incoming TLS requests. This is used by server side BEEP peers to accept incoming TLS requests."},
	/* is_authenticated */
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

PyMODINIT_FUNC initlibpy_vortex_tls_11 (void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3 ("libpy_vortex_tls_11", py_vortex_tls_methods, 
				 "TLS binding support for vortex library TLS profile");
	if (module == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to create tls module");
		return;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "Tls module started");

	return;
}



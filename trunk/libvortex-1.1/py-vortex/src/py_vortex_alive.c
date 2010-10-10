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
 * @brief Allows to init vortex.alive module.
 */
static PyObject * py_vortex_alive_init (PyObject * self, PyObject * args)
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
	return Py_BuildValue ("i", vortex_alive_init (py_vortex_ctx_get (py_ctx)));
}

static PyObject * py_vortex_alive_enable_check (PyObject * self, 
						PyObject * args, 
						PyObject * kwds)
{
	PyObject               * py_conn         = NULL;
	PyObject               * failure_handler = NULL;
	long                     check_period    = 0;
	int                      unreply_count   = 0;
	axl_bool                 result;

	/* now parse arguments */
	static char *kwlist[] = {"conn", "check_period", "unreply_count", "failure_handler", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oii|O", kwlist, 
					  &py_conn,
					  
					  &accept_handler, &accept_handler_data, 
					  &cert_handler, &cert_handler_data, 
					  &key_handler, &key_handler_data))
		return NULL;

	/* check context object received */
	if (! py_vortex_connection_check (py_conn)) {
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

static PyMethodDef py_vortex_alive_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_vortex_alive_init, METH_VARARGS,
	 "Inits vortex.alive module (a wrapper to vortex_alive_init function)."},
	{"enable_check", (PyCFunction) py_vortex_alive_enable_check, METH_VARARGS | METH_KEYWORDS,
	 "Starts alive check support on the provided connection"},
	/* is_authenticated */
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

PyMODINIT_FUNC initlibpy_vortex_alive_11 (void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3 ("libpy_vortex_alive_11", py_vortex_alive_methods, 
				 "ALIVE binding support for vortex library ALIVE profile");
	if (module == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to create alive module");
		return;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "Alive module started");

	return;
}



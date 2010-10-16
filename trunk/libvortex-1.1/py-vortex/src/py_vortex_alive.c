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
#include <py_vortex_alive.h>

/* include vortex alive support */
#include <vortex_alive.h>

#define PY_VORTEX_ALIVE_FAILURE_HANDLER "py:vo:al:fail"

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

void py_vortex_alive_failure_handler (VortexConnection * conn, 
				      long               check_period,
				      int                unreply_count)
{
	PyObject          * py_ctx;
	PyObject          * py_conn;
	PyGILState_STATE    state;
	PyObject          * _result;
	PyObject          * args;
	PyObject          * failure_handler;

	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* build references */
	py_ctx  = py_vortex_ctx_create (CONN_CTX (conn));
	py_conn = py_vortex_connection_find_reference (conn, py_ctx);
	Py_DECREF (py_ctx);

	/* now implementuser code notification */
	args = PyTuple_New (3);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("i", (int) check_period));
	PyTuple_SetItem (args, 2, Py_BuildValue ("i", unreply_count));

	/* perform call */
	failure_handler = vortex_connection_get_data (conn, PY_VORTEX_ALIVE_FAILURE_HANDLER);
	_result = PyObject_Call (failure_handler, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "ALIVE failure handler finished , checking for exceptions, _result: %p..", _result);
	py_vortex_handle_and_clear_exception (py_conn);

	Py_XDECREF (_result);
	Py_DECREF (args);

	/* release the GIL */
	PyGILState_Release(state);
	
	
	return;
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
					  &check_period, &unreply_count, &failure_handler))
		return NULL;

	/* check context object received */
	if (! py_vortex_connection_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Ctx object but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (failure_handler != NULL && ! PyCallable_Check (failure_handler)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for accept_handler handler, but received something different.");
		return NULL;
	} /* end if */

	/* acquire a reference to the failure handler if defined */
	if (failure_handler) {
		Py_INCREF (failure_handler);
		vortex_connection_set_data_full (py_vortex_connection_get (py_conn), PY_VORTEX_ALIVE_FAILURE_HANDLER, 
						 failure_handler, NULL, (axlDestroyFunc) py_vortex_decref);
	} /* end if */

	/* call to enable check */
	result = vortex_alive_enable_check (py_vortex_connection_get (py_conn),
					    check_period, unreply_count, failure_handler ? py_vortex_alive_failure_handler : NULL);

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



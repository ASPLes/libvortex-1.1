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

/* include header */
#include <py_vortex_sasl.h>

/* include sasl headers */
#include <vortex_sasl.h>

/** 
 * @brief Allows to init vortex.sasl module.
 */
static PyObject * py_vortex_sasl_init (PyObject * self, PyObject * args)
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
	return Py_BuildValue ("i", vortex_sasl_init (py_vortex_ctx_get (py_ctx)));
}

static PyObject * py_vortex_sasl_is_authenticated (PyObject * self, PyObject * args)
{
	PyObject * py_conn = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_conn))
		return NULL;

	/* check connection object */
	if (! py_vortex_connection_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Connection object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("i", vortex_sasl_is_authenticated (py_vortex_connection_get (py_conn)));
}

/** 
 * @internal Allows to check and configure the string as the propertie provided.
 */
#define	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, string, prop) do {                                                 \
	if (string != NULL) {                                                                                         \
               /* set the property */                                                                                 \
               vortex_sasl_set_propertie (py_vortex_connection_get (py_conn), prop, axl_strdup (string), axl_free);   \
        }                                                                                                             \
} while (0);

typedef struct _PyVortexSaslDoNotify {
	PyObject * auth_notify;
	PyObject * auth_notify_data;
	PyObject * py_conn;
} PyVortexSaslDoNotify;

void py_vortex_sasl_do_notify (VortexConnection * connection,
			       VortexStatus       status,
			       char             * status_message,
			       axlPointer         user_data)
{
	PyVortexSaslDoNotify * auth_data = user_data;
	PyGILState_STATE       state;
	PyObject             * result;
	PyObject             * args;
	PyObject             * py_conn   = auth_data->py_conn;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	py_vortex_log (PY_VORTEX_DEBUG, "received sasl auth notification, status=%d, status_msg=%s", 
		       status, status_message);

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);
	/* py_conn */
	PyTuple_SetItem (args, 0, py_conn);
	/* status */
	PyTuple_SetItem (args, 1, Py_BuildValue ("i", status));
	/* status_msg */
	PyTuple_SetItem (args, 2, Py_BuildValue ("s", status_message));
	PyTuple_SetItem (args, 3, auth_data->auth_notify_data);
	
	/* do notification */
	result = PyObject_Call (auth_data->auth_notify, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "finished sasl auth notification, checking exceptions..");

	/* finished notification handle exceptions */
	py_vortex_handle_and_clear_exception (py_conn);

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* terminate here PyVortexSaslDoNotify: do not terminate
	 * auth_notify_data and py_conn references inside because they
	 * are terminated by previous tuple deallocation */
	Py_DECREF (auth_data->auth_notify);
	axl_free (auth_data);

	/* release the GIL */
	PyGILState_Release(state);
	
	return;
}

const char * py_vortex_sasl_normalize_profile_name (const char * profile)
{
	py_vortex_log (PY_VORTEX_DEBUG, "normalizing profile: %s", profile);

	/* check for profiles not started with http://iana.org/beep/SASL/ */
	if (axl_memcmp (profile, "http://iana.org/beep/SASL", 25))
		return profile;

	/* check for the appropriate profile */
	if (axl_casecmp (VORTEX_SASL_PLAIN + 26, profile))
		return VORTEX_SASL_PLAIN;

	if (axl_casecmp (VORTEX_SASL_ANONYMOUS + 26, profile))
		return VORTEX_SASL_ANONYMOUS;

	if (axl_casecmp (VORTEX_SASL_EXTERNAL + 26, profile))
		return VORTEX_SASL_EXTERNAL;

	if (axl_casecmp (VORTEX_SASL_CRAM_MD5 + 26, profile))
		return VORTEX_SASL_CRAM_MD5;

	if (axl_casecmp (VORTEX_SASL_DIGEST_MD5 + 26, profile))
		return VORTEX_SASL_DIGEST_MD5;

	if (axl_casecmp (VORTEX_SASL_GSSAPI + 26, profile))
		return VORTEX_SASL_GSSAPI;

	if (axl_casecmp (VORTEX_SASL_KERBEROS_V4 + 26, "kerberos_v4"))
		return VORTEX_SASL_KERBEROS_V4;
	
	return NULL;
}

static PyObject * py_vortex_sasl_start_auth (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject             * py_conn          = NULL;
	const char           * profile          = NULL;
	PyObject             * auth_notify      = NULL;
	PyObject             * auth_notify_data = NULL;
	const char           * auth_id          = NULL;
	const char           * authorization_id = NULL;
	const char           * password         = NULL;
	const char           * realm            = NULL;
	const char           * anonymous_token  = NULL;
	PyVortexSaslDoNotify * auth_data;
	VortexStatus           status           = -1;
	char                 * status_msg       = NULL;
	PyObject             * result;
	const char           * _profile;

	/* now parse arguments */
	static char *kwlist[] = {"conn", "profile", "auth_notify", "auth_notify_data", "auth_id", "authorization_id", "password", "realm", "anonymous_token", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "Os|OOsssss", kwlist, 
					  &py_conn, &profile, &auth_notify, &auth_notify_data, 
					  /* optional arguments */
					  &auth_id, &authorization_id, &password, &realm, &anonymous_token))
		return NULL;

	/* check connection object */
	if (! py_vortex_connection_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Connection object but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (auth_notify != NULL && ! PyCallable_Check (auth_notify)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for auth_notify handler, but received something different.");
		return NULL;
	} /* end if */

	/* normilize profile */
	_profile = py_vortex_sasl_normalize_profile_name (profile);
	if (_profile == NULL) {
		PyErr_Format (PyExc_ValueError, "Unable to process profile received '%s', failed to start SASL auth", profile);
		return NULL;
	}

	/* log profile used */
	py_vortex_log (PY_VORTEX_DEBUG, "preparing SASL auth using profile %s", _profile); 

	/* set a default value for associated data */
	if (auth_notify != NULL && auth_notify_data == NULL)
		auth_notify_data = Py_None;

	/* now check each parameter to configure it */
	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, auth_id, VORTEX_SASL_AUTH_ID);
	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, authorization_id, VORTEX_SASL_AUTHORIZATION_ID);
	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, password, VORTEX_SASL_PASSWORD);
	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, realm, VORTEX_SASL_REALM);
	PY_VOTEX_SASL_CHECK_AND_CONFIGURE(py_conn, anonymous_token, VORTEX_SASL_ANONYMOUS_TOKEN);

	/* ok, now begin auth setting a notification handler if the
	 * user defined it */
	if (auth_notify != NULL) {
		auth_data = axl_new (PyVortexSaslDoNotify, 1);

		/* set auth_notify */
		auth_data->auth_notify = auth_notify;
		Py_INCREF (auth_notify);

		/* set auth_notify_data */
		auth_data->auth_notify_data = auth_notify_data;
		Py_INCREF (auth_notify_data);

		/* set py_conn */
		auth_data->py_conn = py_conn;
		Py_INCREF (py_conn);

		vortex_sasl_start_auth (py_vortex_connection_get (py_conn), _profile, py_vortex_sasl_do_notify,  auth_data);
		
		/* finished */
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */
	
	/* call to authenticate in a blocking manner */
	vortex_sasl_start_auth_sync (py_vortex_connection_get (py_conn), _profile, &status, &status_msg);
	
	/* create a tuple to contain arguments */
	result = PyTuple_New (2);
	PyTuple_SetItem (result, 0, Py_BuildValue ("i", status));
	PyTuple_SetItem (result, 1, Py_BuildValue ("s", status_msg));
	
	return result;
}

/** 
 * @brief Allows to init vortex.sasl module.
 */
static PyObject * py_vortex_sasl_method_used (PyObject * self, PyObject * args)
{
	PyObject * py_conn = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &py_conn))
		return NULL;

	/* check connection object */
	if (! py_vortex_connection_check (py_conn)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Connection object but received something different");
		return NULL;
	} /* end if */
	
	/* call to return value */
	return Py_BuildValue ("s", vortex_sasl_auth_method_used (py_vortex_connection_get (py_conn)));
}

typedef struct _PyVortexSaslAcceptMech {
	PyObject * auth_handler;
	PyObject * auth_handler_data;
	PyObject * py_ctx;
} PyVortexSaslAcceptMech;

void py_vortex_sasl_accept_mech_free (PyVortexSaslAcceptMech * data)
{
	Py_DECREF (data->auth_handler);
	Py_DECREF (data->auth_handler_data);
	memset (data, 0, sizeof (PyVortexSaslAcceptMech));
	axl_free (data);
	return;
}


/** 
 * @internal Helper macro used by py_vortex_sasl_auth_handler_bridge
 * to build the hash that contains all auth items required by
 * particular profile mechs.
 */
#define AUTH_HANDLER_SET_ITEM(string, value)                                          \
	if (value) {                                                                  \
	       PyDict_SetItemString (auth_items, string, Py_BuildValue ("s", value)); \
        } else {                                                                      \
	       Py_INCREF (Py_None);					              \
	       PyDict_SetItemString (auth_items, string, Py_None);                    \
        }                

axlPointer py_vortex_sasl_auth_handler_bridge (VortexConnection * conn,
					       VortexSaslProps  * props,
					       axlPointer         user_data)
{
	PyVortexSaslAcceptMech * data = user_data;
	PyGILState_STATE         state;
	PyObject               * result;
	PyObject               * args;
	PyObject               * py_conn;
	PyObject               * auth_items;
	char                   * password = NULL;
	axl_bool                 status   = axl_false;
	

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a PyVortexConnection */
	py_conn  = py_vortex_connection_find_reference (
		/* connection to wrap */
		conn, 
		/* context: create a copy */
		data->py_ctx);

	/* create auth_items */
	auth_items = PyDict_New ();

	/* configure auth_items hash */
	AUTH_HANDLER_SET_ITEM ("mech",             props->mech);
	AUTH_HANDLER_SET_ITEM ("anonymous_token",  props->anonymous_token);
	AUTH_HANDLER_SET_ITEM ("auth_id",          props->auth_id);
	AUTH_HANDLER_SET_ITEM ("authorization_id", props->authorization_id);
	AUTH_HANDLER_SET_ITEM ("password",         props->password);
	AUTH_HANDLER_SET_ITEM ("realm",            props->realm);
	AUTH_HANDLER_SET_ITEM ("return_password",  NULL);

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);

	/* the following function PyTuple_SetItem "steals" a reference
	 * which is the python way to say that we are transfering the
	 * ownership of the reference to that function, making it
	 * responsible of calling to Py_DECREF when required. */
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, auth_items);

	Py_INCREF (data->auth_handler_data);
	PyTuple_SetItem (args, 2, data->auth_handler_data);

	/* now invoke */
	result = PyObject_Call (data->auth_handler, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "auth notification finished status, checking for exceptions..");
	py_vortex_handle_and_clear_exception (py_conn);
	
	/* check auth_items hash */
	PyArg_Parse (PyDict_GetItemString (auth_items, "return_password"), "i", &status);
	if (status) {
		/* parse a password from result */
		PyArg_Parse (result, "s", &password);
		
		/* notify caller we are returning a password */
		props->return_password = axl_true;
	} else {
		/* parse boolean status */
		if (result)
			PyArg_Parse (result, "i", &status);
		py_vortex_log (PY_VORTEX_DEBUG, "auth operation status: %d..\n", status);
	}

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* release the GIL */
	PyGILState_Release(state);

	/* return a password or status */
	if (props->return_password)
		return password;
	return INT_TO_PTR (status);
}

/** 
 * @internal Allows to get the authentication auth_id that was used to
 * complete a connection.
 */
static PyObject * py_vortex_sasl_auth_id (PyObject * self, PyObject * args)
{
	PyObject * object = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &object))
		return NULL;

	/* check connection object */
	if (py_vortex_connection_check (object)) {
		/* received a connection object */
		return Py_BuildValue ("z", AUTH_ID_FROM_CONN(py_vortex_connection_get (object)));
	} else if (py_vortex_channel_check (object)) {
		/* received a channel object */
		return Py_BuildValue ("z", AUTH_ID_FROM_CHANNEL(py_vortex_channel_get (object)));
	} /* end if */
	
	/* call to return value */
	PyErr_SetString (PyExc_TypeError, "Expected to receive either a vortex.Connection or a vortex.Channel instance but received something different.");
	return NULL;
}

static PyObject * py_vortex_sasl_accept_mech (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject               * py_ctx            = NULL;
	const char             * profile           = NULL;
	const char             * _profile          = NULL;
	PyObject               * auth_handler      = NULL;
	PyObject               * auth_handler_data = Py_None;
	PyVortexSaslAcceptMech * data;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "profile", "auth_handler", "auth_handler_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "OsO|O", kwlist, 
					  &py_ctx, &profile, &auth_handler, &auth_handler_data))
		return NULL;

	/* check ctx object */
	if (! py_vortex_ctx_check (py_ctx)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive a vortex.Ctx object but received something different.");
		return NULL;
	} /* end if */

	/* check callable object */
	if (auth_handler != NULL && ! PyCallable_Check (auth_handler)) {
		/* set exception */
		PyErr_SetString (PyExc_TypeError, "Expected to receive callable object for auth_handler handler, but received something different.");
		return NULL;
	} /* end if */

	/* normilize profile */
	_profile = py_vortex_sasl_normalize_profile_name (profile);
	if (_profile == NULL) {
		PyErr_Format (PyExc_ValueError, "Unable to process profile received '%s', failed to start SASL auth", profile);
		return NULL;
	}

	/* log profile used */
	py_vortex_log (PY_VORTEX_DEBUG, "preparing SASL auth using profile %s", _profile);

	/* prepare data */
	data                    = axl_new (PyVortexSaslAcceptMech, 1);
	/* set auth_handler */
	data->auth_handler      = auth_handler;
	Py_INCREF (auth_handler);
	/* set auth_handler_data */
	data->auth_handler_data = auth_handler_data;
	Py_INCREF (auth_handler_data);
	/* set py_ctx */
	data->py_ctx            = py_ctx;

	/* associate data to the context */
	vortex_ctx_set_data_full (
		/* context */
		py_vortex_ctx_get (py_ctx), 
		/* key and value */
		axl_strdup_printf ("%p", data), data, 
		/* key destroy and value destroy */
		axl_free, (axlDestroyFunc) py_vortex_sasl_accept_mech_free);
	
	/* enable auth for the profile provided */
	return Py_BuildValue ("i", vortex_sasl_accept_negotiation_common (py_vortex_ctx_get (py_ctx), _profile, py_vortex_sasl_auth_handler_bridge, data));
}


static PyMethodDef py_vortex_sasl_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_vortex_sasl_init, METH_VARARGS,
	 "Inits vortex.sasl module (a wrapper to vortex_sasl_init function)."},
	/* is_authenticated */
	{"is_authenticated", (PyCFunction) py_vortex_sasl_is_authenticated, METH_VARARGS,
	 "Allows to check if a connection is authenticated"},
	{"start_auth", (PyCFunction) py_vortex_sasl_start_auth, METH_VARARGS | METH_KEYWORDS,
	 "Starts SASL authentication process using provided profile and required data."},
	{"method_used", (PyCFunction) py_vortex_sasl_method_used, METH_VARARGS,
	 "Allows to check method used to authenticate the conection."},
	{"auth_id", (PyCFunction) py_vortex_sasl_auth_id, METH_VARARGS,
	 "Allows to get the authentication id used given a connection or a channel. "},
	{"accept_mech", (PyCFunction) py_vortex_sasl_accept_mech, METH_VARARGS | METH_KEYWORDS,
	 "Listener side implementation that allows to handle incoming SASL authentication."},
	{NULL, NULL, 0, NULL}   /* sentinel */
}; 

PyMODINIT_FUNC initlibpy_vortex_sasl_11(void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3 ("libpy_vortex_sasl_11", py_vortex_sasl_methods, 
				 "SASL binding support for vortex library SASL profiles");
	if (module == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to create sasl module");
		return;
	} /* end if */

	py_vortex_log (PY_VORTEX_DEBUG, "Sasl module started");

	return;
}


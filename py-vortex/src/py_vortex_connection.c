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

#include <py_vortex_connection.h>

struct _PyVortexConnection {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the VortexConnection object */
	VortexConnection * conn;

	/** 
	 * @internal variable used to signal the type to close the
	 * connection wrapped (VortexConnection) when the reference
	 * PyVortexConnection is garbage collected.
	 */
	axl_bool           close_ref;

	/** 
	 * @brief Allows to skip the automatic connection close when
	 * python object is collected.
	 */
	axl_bool         skip_conn_close;
};

#define PY_VORTEX_CONNECTION_CHECK_NOT_ROLE(py_conn, role, method)                                                \
do {                                                                                                              \
	if (vortex_connection_get_role (((PyVortexConnection *)py_conn)->conn) == role) {                         \
	         py_vortex_log (PY_VORTEX_CRITICAL,                                                               \
                                "trying to run a method %s not supported by the role %d, connection id: %d",      \
				method, role, vortex_connection_get_id (((PyVortexConnection *)py_conn)->conn));  \
	         Py_INCREF(Py_None);                                                                              \
		 return Py_None;                                                                                  \
	}                                                                                                         \
} while(0);

/** 
 * @internal function that maps connection roles to string values.
 */
const char * __py_vortex_connection_stringify_role (VortexConnection * conn)
{
	/* check known roles to return its appropriate string */
	switch (vortex_connection_get_role (conn)) {
	case VortexRoleInitiator:
		return "initiator";
	case VortexRoleListener:
		return "listener";
	case VortexRoleMasterListener:
		return "master-listener";
	default:
		break;
	}

	/* return unknown string */
	return "unknown";
}
		 

/** 
 * @brief Allows to get the VortexConnection reference inside the
 * PyVortexConnection.
 *
 * @param py_conn The reference that holds the connection inside.
 *
 * @return A reference to the VortexConnection inside or NULL if it fails.
 */
VortexConnection * py_vortex_connection_get  (PyObject * py_conn)
{
	PyVortexConnection * _py_conn = (PyVortexConnection *) py_conn;

	/* return NULL reference */
	if (_py_conn == NULL)
		return NULL;
	/* return py connection */
	return _py_conn->conn;
}

static int py_vortex_connection_init_type (PyVortexConnection *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.Connection
 */
static PyObject * py_vortex_connection_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexConnection * self;
	const char         * host       = NULL;
	const char         * port       = NULL;
	const char         * serverName = NULL;
	PyObject           * py_vortex_ctx = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "host", "port", "serverName", NULL};

	/* create the object */
	self = (PyVortexConnection *)type->tp_alloc(type, 0);

	/* check args */
	if (args != NULL) {
		/* parse and check result */
		if (! PyArg_ParseTupleAndKeywords(args, kwds, "|Ossz", kwlist, &py_vortex_ctx, &host, &port, &serverName)) 
			return NULL;

		/* check for empty creation */
		if (py_vortex_ctx == NULL) {
			py_vortex_log (PY_VORTEX_DEBUG, "found empty request to create a PyVortexConnection ref..");
			return (PyObject *) self;
		}

		/* check that py_vortex_ctx is indeed a vortex ctx */
		if (! py_vortex_ctx_check (py_vortex_ctx)) {
			PyErr_Format (PyExc_ValueError, "Expected to receive a vortex.Ctx object but received something different");
			return NULL;
		}

		/* set default timeout */
		vortex_connection_connect_timeout (py_vortex_ctx_get (py_vortex_ctx), 60000000);

		/* allow threads */
		Py_BEGIN_ALLOW_THREADS

		/* create the vortex connection in a blocking manner */
		if (serverName)
			self->conn = vortex_connection_new_full (py_vortex_ctx_get (py_vortex_ctx),
								 host, port,
								 CONN_OPTS (VORTEX_SERVERNAME_FEATURE, serverName, VORTEX_OPTS_END),
								 NULL, NULL);
		else
			self->conn = vortex_connection_new (py_vortex_ctx_get (py_vortex_ctx), host, port, NULL, NULL);

		/* end threads */
		Py_END_ALLOW_THREADS

		/* signal this instance as a master copy to be closed
		 * if the reference is collected and the connection is
		 * working */
		self->close_ref = axl_true;

		if (vortex_connection_is_ok (self->conn, axl_false)) {
			py_vortex_log (PY_VORTEX_DEBUG, "created connection id %d, with %s:%s (self: %p, conn: %p)",
				       vortex_connection_get_id (self->conn), 
				       vortex_connection_get_host (self->conn),
				       vortex_connection_get_port (self->conn),
				       self, self->conn);
		} else {
			py_vortex_log (PY_VORTEX_CRITICAL, "failed to connect with %s:%s, connection id: %d",
				       vortex_connection_get_host (self->conn),
				       vortex_connection_get_port (self->conn),
				       vortex_connection_get_id (self->conn));
		} /* end if */
	} /* end if */

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.Connection
 */
static void py_vortex_connection_dealloc (PyVortexConnection* self)
{
#if defined(ENABLE_PY_VORTEX_LOG)
	int conn_id = vortex_connection_get_id (self->conn);
#endif
	int ref_count;

	py_vortex_log (PY_VORTEX_DEBUG, "finishing PyVortexConnection id: %d (%p, VortexConnection %p, role: %s, close-ref: %d)", 
		       conn_id, self, self->conn, __py_vortex_connection_stringify_role (self->conn), self->close_ref);

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS

	/* finish the connection in the case it is no longer referenced */
	if (vortex_connection_is_ok (self->conn, axl_false) && self->close_ref) {
		py_vortex_log (PY_VORTEX_DEBUG, "shutting down BEEP session associated at connection finalize id: %d (connection is ok, and close_ref is activated, refs: %d)", 
			       vortex_connection_get_id (self->conn),
			       vortex_connection_ref_count (self->conn));

		/* shutdown connection if itsn't flagged that way */
		if (! self->skip_conn_close) {
			vortex_connection_shutdown (self->conn);
		}

		ref_count = vortex_connection_ref_count (self->conn);
		vortex_connection_unref (self->conn, "py_vortex_connection_dealloc when is ok");
		py_vortex_log (PY_VORTEX_DEBUG, "ref count after close: %d", ref_count - 1);
	} else {
		py_vortex_log (PY_VORTEX_DEBUG, "unref the connection id: %d", vortex_connection_get_id (self->conn));
		/* only unref the connection */
		vortex_connection_unref (self->conn, "py_vortex_connection_dealloc");
	} /* end if */

	/* end threads */
	Py_END_ALLOW_THREADS

	/* nullify */
	self->conn = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	py_vortex_log (PY_VORTEX_DEBUG, "terminated PyVortexConnection dealloc with id: %d (self: %p)", conn_id, self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_connection_is_ok function. 
 */
static PyObject * py_vortex_connection_is_ok (PyVortexConnection* self)
{
	PyObject *_result;

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("i", __unlocked_vortex_connection_is_ok (self->conn, axl_false));
	
	return _result;
}

static PyObject * py_vortex_connection_pop_channel_error (PyVortexConnection * self)
{
	/* create a tuple to contain arguments */
	PyObject * result;
	int        code = 0;
	char     * msg  = NULL;

	/* check if this is a listener connection that cannot provide
	   this service */
	PY_VORTEX_CONNECTION_CHECK_NOT_ROLE(self, VortexRoleMasterListener, "pop_channel_error");

	/* check for channel errors */
	if (vortex_connection_pop_channel_error (self->conn, &code, &msg)) {
		/* found error message */
		result = PyTuple_New (2);
		PyTuple_SetItem (result, 0, Py_BuildValue ("i", code));
		PyTuple_SetItem (result, 1, Py_BuildValue ("s", msg));

		py_vortex_log (PY_VORTEX_DEBUG, "poping channel error code: %d, msg: %s",
			       code, msg);
		
		/* release msg */
		axl_free (msg);

		/* return tuple */
		return result;
	} /* end if */

	/* no error is found, return None */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Direct wrapper for the vortex_connection_close function. 
 */
static PyObject * py_vortex_connection_close (PyVortexConnection* self)
{
	PyObject       * _result;
	axl_bool         result;
	VortexPeerRole   role     = VortexRoleUnknown;
	const char     * str_role = NULL;

	if (self->conn) {
		py_vortex_log (PY_VORTEX_DEBUG, "closing connection id: %d (%s, refs: %d)",
			       vortex_connection_get_id (self->conn), 
			       __py_vortex_connection_stringify_role (self->conn), 
			       vortex_connection_ref_count (self->conn));
		/* get peer role to avoid race conditions */
		role     = vortex_connection_get_role (self->conn);
		str_role = __py_vortex_connection_stringify_role (self->conn);
	} /* end if */

	/* according to the connection role and status, do a shutdown
	 * or a close */
	if (role == VortexRoleMasterListener && __unlocked_vortex_connection_is_ok (self->conn, axl_false)) {
		py_vortex_log (PY_VORTEX_DEBUG, "shutting down working master listener connection id=%d", 
			       vortex_connection_get_id (self->conn));
		result = axl_true;
		/* allow threads */
		Py_BEGIN_ALLOW_THREADS
		vortex_connection_shutdown (self->conn);
		/* end threads */
		Py_END_ALLOW_THREADS
	} else  {
		py_vortex_log (PY_VORTEX_DEBUG, "closing connection id=%d (role: %s)", 
			       vortex_connection_get_id (self->conn), str_role);
		/* allow threads */
		Py_BEGIN_ALLOW_THREADS
		result  = vortex_connection_close (self->conn);
		/* end threads */
		Py_END_ALLOW_THREADS
	} /* end if */
	_result = Py_BuildValue ("i", result);

	/* check to nullify connection reference in the case the connection is closed */
	if (result) {
		/* check if we have to unref the connection in the
		 * case of a master listener */
		if (role == VortexRoleMasterListener) {
			/* allow threads */
			Py_BEGIN_ALLOW_THREADS
				
			vortex_connection_unref (self->conn, "py_vortex_connection_close (master-listener)");

			/* end threads */
			Py_END_ALLOW_THREADS
		}

		py_vortex_log (PY_VORTEX_DEBUG, "close ok, nullifying..");
		self->conn = NULL; 
	}

	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_connection_shutdown function. 
 */
PyObject * py_vortex_connection_shutdown (PyVortexConnection* self)
{
	py_vortex_log (PY_VORTEX_DEBUG, "calling to shutdown connection id: %d, self: %p",
		       vortex_connection_get_id (self->conn), self);

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS
	/* shut down the connection */
	vortex_connection_shutdown (self->conn);
	/* end threads */
	Py_END_ALLOW_THREADS

	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Increment reference counting
 */
static PyObject * py_vortex_connection_incref (PyVortexConnection* self)
{
	/* close the connection */
	Py_INCREF (__PY_OBJECT (self));
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Decrement reference counting.
 */
static PyObject * py_vortex_connection_decref (PyVortexConnection* self)
{
	/* close the connection */
	Py_DECREF (__PY_OBJECT (self));
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Allows to nullify the internal reference to the
 * VortexConnection object.
 */ 
void                 py_vortex_connection_nullify  (PyObject           * py_conn)
{
	PyVortexConnection * _py_conn = (PyVortexConnection *) py_conn;
	if (py_conn == NULL)
		return;
	
	/* nullify the connection to make it available to other owner
	 * or process */
	_py_conn->conn = NULL;
	return;
}

/** 
 * @brief Direct wrapper for the vortex_connection_status function. 
 */
PyObject * py_vortex_connection_status (PyVortexConnection* self)
{
	PyObject *_result;

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("i", vortex_connection_get_status (self->conn));
	
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_connection_get_message function. 
 */
PyObject * py_vortex_connection_error_msg (PyVortexConnection* self)
{
	PyObject *_result;
	/* printf ("Received request to return status message: %s\n", vortex_connection_get_message (self->conn)); */

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("z", vortex_connection_get_message (self->conn));
	
	return _result;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_connection_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyVortexConnection * self = (PyVortexConnection *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "error_msg")) {
		/* found error_msg attribute */
		return Py_BuildValue ("z", vortex_connection_get_message (self->conn));
	} else if (axl_cmp (attr, "status")) {
		/* found status attribute */
		return Py_BuildValue ("i", vortex_connection_get_status (self->conn));
	} else if (axl_cmp (attr, "server_name")) {
		/* found server_name */
		return Py_BuildValue ("z", vortex_connection_get_server_name (self->conn));
	} else if (axl_cmp (attr, "host")) {
		/* found host attribute */
		return Py_BuildValue ("z", vortex_connection_get_host (self->conn));
	} else if (axl_cmp (attr, "host_ip")) {
		/* found host attribute */
		return Py_BuildValue ("z", vortex_connection_get_host_ip (self->conn));
	} else if (axl_cmp (attr, "port")) {
		/* found port attribute */
		return Py_BuildValue ("z", vortex_connection_get_port (self->conn));
	} else if (axl_cmp (attr, "local_addr")) {
		/* found local_addr attribute */
		return Py_BuildValue ("z", vortex_connection_get_local_addr (self->conn));
	} else if (axl_cmp (attr, "local_port")) {
		/* found local_port attribute */
		return Py_BuildValue ("z", vortex_connection_get_local_port (self->conn));
	} else if (axl_cmp (attr, "num_channels")) {
		/* found num_channels attribute */
		return Py_BuildValue ("i", vortex_connection_channels_count (self->conn));
	} else if (axl_cmp (attr, "role")) {
		/* found role attribute */
		switch (vortex_connection_get_role (self->conn)) {
		case VortexRoleInitiator:
			return Py_BuildValue ("s", "initiator");
		case VortexRoleListener:
			return Py_BuildValue ("s", "listener");
		case VortexRoleMasterListener:
			return Py_BuildValue ("s", "master-listener");
		default:
			break;
		}
		return Py_BuildValue ("s", "unknown");
	} else if (axl_cmp (attr, "ctx")) {
		/* found ctx attribute */
		return py_vortex_ctx_create (CONN_CTX (self->conn));
	} else if (axl_cmp (attr, "id")) {
		/* return integer value */
		return Py_BuildValue ("i", vortex_connection_get_id (self->conn));
	} else if (axl_cmp (attr, "ref_count")) {
		/* return integer value */
		return Py_BuildValue ("i", vortex_connection_ref_count (self->conn));
	} /* end if */

	/* printf ("Attribute not found: '%s'..\n", attr); */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}


static PyObject * py_vortex_connection_open_channel (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject           * py_channel;
	VortexChannel      * channel;
	int                  number;
	const char         * profile                 = NULL;
	PyObject           * frame_received          = NULL;
	PyObject           * frame_received_data     = NULL;
	PyObject           * on_channel              = NULL;
	PyObject           * on_channel_data         = NULL;
	const char         * profile_content         = NULL;
	int                  profile_content_size    = 0;
	const char         * serverName              = NULL;
	VortexEncoding       encoding                = EncodingUnknown;
	
	/* now parse arguments */
	static char *kwlist[] = {"number", "profile", "serverName", 
				 "frame_received", "frame_received_data", 
				 "encoding", "profile_content", 
				 "on_channel", "on_channel_data", NULL};

	/* check if this is a listener connection that cannot provide
	   this service */
	PY_VORTEX_CONNECTION_CHECK_NOT_ROLE(self, VortexRoleMasterListener, "open_channel");

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "is|sOOisOO", kwlist, &number, &profile, 
					  /* optional parameters */
					  &serverName, 
					  &frame_received, &frame_received_data, 
					  &encoding, &profile_content, 
					  &on_channel, &on_channel_data)) {
		return NULL;
	}

	/* check for frame received configuration */
	if (frame_received && ! PyCallable_Check (frame_received)) {
		PyErr_Format (PyExc_ValueError, "frame_received handler is not a callable reference, unable to create channel");
		return NULL;
	}

	/* check for frame received configuration */
	if (on_channel && ! PyCallable_Check (on_channel)) {
		PyErr_Format (PyExc_ValueError, "on_channel handler is not a callable reference, unable to create channel");
		return NULL;
	}

	/* create an empty channel reference */
	py_channel = py_vortex_channel_create_empty ();

	/* set on channel created handler if defined */
	if (on_channel && 
	    ! py_vortex_channel_set_on_channel (py_channel, on_channel, on_channel_data)) {
		PyErr_Format (PyExc_ValueError, "failed to configure on channel created handler, unable to create channel");
		
		/* decrease py ref created */
		Py_DECREF (py_channel);
		return NULL;
	}

	/* update profile content size if provided by the user */
	if (profile_content != NULL)
		profile_content_size = strlen (profile_content);

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS

	/* now try to create the channel */
	channel = vortex_channel_new_full (
		/* pass the BEEP connection */
		((PyVortexConnection *)self)->conn, 
		/* channel number, serverName and profile */
		number, serverName, profile,
		/* profile content to be send on first request */
		encoding, profile_content, profile_content_size,
		/* close handler */
		NULL, NULL, 
		/* frame received handler */
		frame_received ? py_vortex_channel_received : NULL, NULL,
		/* on channel created */
		on_channel ? py_vortex_channel_create_notify : NULL, py_channel);

	/* reacquire again thread execution */
	Py_END_ALLOW_THREADS;

	/* call to set the handler */
	if (frame_received && channel &&
	    ! py_vortex_channel_configure_frame_received (channel, 
							  frame_received, 
							  frame_received_data)) {
		PyErr_Format (PyExc_ValueError, "failed to configure frame received handler, unable to create channel");
		
		/* decrease py ref created */
		Py_DECREF (py_channel);
		return NULL;
	} /* end if */

	/* check for async channel creati on notification */
	if (on_channel != NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */
	
	/* check for error found */
	if (channel == NULL) {
		/* release python channel reference */
		Py_DECREF (py_channel);
		Py_INCREF (Py_None);
		return Py_None;
	}

	/* set the channel */
	py_vortex_channel_set ((PyVortexChannel *) py_channel, channel);

	/* return reference created */
	return py_channel;
}

/** 
 * @internal Function that implements vortex.Connection.channel_pool_new
 */
static PyObject * py_vortex_connection_channel_pool_new (PyObject * self, PyObject *args, PyObject *kwds)
{
	const char          * profile                   = NULL;
	int                   init_num                  = -1;
	PyObject            * create_channel            = NULL;
	PyObject            * create_channel_user_data  = NULL;
	PyObject            * close                     = NULL;
	PyObject            * close_user_data           = NULL;
	PyObject            * received                  = NULL;
	PyObject            * received_user_data        = NULL;
	PyObject            * on_channel_pool_created   = NULL;
	PyObject            * user_data                 = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"profile", "init_num", 
				 "create_channel", "create_channel_data", 
				 "received", "received_data",
				 "close", "close_data", 
				 "on_created", "user_data",
				 NULL};

	/* check if this is a listener connection that cannot provide
	   this service */
	PY_VORTEX_CONNECTION_CHECK_NOT_ROLE(self, VortexRoleMasterListener, "open_channel");

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "si|OOOOOOOO", kwlist, 
					  &profile, &init_num,
					  &create_channel, &create_channel_user_data,
					  &received, &received_user_data,
					  &close, &close_user_data,
					  &on_channel_pool_created, &user_data))
		return NULL;

	
	/* create an empty pool */
	return py_vortex_channel_pool_create (self,
					      profile,
					      init_num,
					      create_channel,
					      create_channel_user_data,
					      close, close_user_data,
					      received,
					      received_user_data,
					      on_channel_pool_created,
					      user_data);
}

/** 
 * @internal Function that implements vortex.Connection.pool
 */
static PyObject * py_vortex_connection_get_pool (PyObject * self, PyObject *args, PyObject *kwds)
{
	int                   pool_id                   = 1;
	VortexChannelPool   * pool;

	/* now parse arguments */
	static char *kwlist[] = {"pool_id", NULL};

	/* check if this is a listener connection that cannot provide
	   this service */
	PY_VORTEX_CONNECTION_CHECK_NOT_ROLE(self, VortexRoleMasterListener, "pool");

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|i", kwlist, &pool_id))
		return NULL;

	/* get the pool with the provided id */
	pool = vortex_connection_get_channel_pool (py_vortex_connection_get (self), pool_id);
	if (pool == NULL) {
		/* no pool found */
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */
	
	/* return wrapped reference */
	return py_vortex_channel_pool_find_reference (pool);
}

/** 
 * @internal Unlocked version that allows threading to enter while
 * this call (vortex_connection_is_ok) is happening to avoid having
 * deadlocks betheen python gil and conn internal mutexes.
 */
axl_bool             __unlocked_vortex_connection_is_ok (VortexConnection * conn,
							 axl_bool           free_on_fail)
{
	axl_bool status;
	
	/* allow threads */
	Py_BEGIN_ALLOW_THREADS

	/* get connection status */
	status = vortex_connection_is_ok (conn, free_on_fail);
		
	/* end threads */
	Py_END_ALLOW_THREADS

	/* return results */
	return status;
}

/** 
 * @internal The following is an auxiliar structure used to bridge
 * set_on_close_full call into python. Because Vortex version allows
 * configuring several handlers at the same time, it is required to
 * track a different object with full state to properly bridge the
 * notification received into python without mixing notifications. See
 * py_vortex_connection_set_on_close to know how this is used.
 */
typedef struct _PyVortexConnectionSetOnCloseData {
	PyObject           * py_conn;
	PyObject           * on_close;
	PyObject           * on_close_data;
} PyVortexConnectionSetOnCloseData;

void py_vortex_connection_set_on_close_handler (VortexConnection * conn, 
						axlPointer         _on_close_obj)
{
	PyVortexConnectionSetOnCloseData * on_close_obj = _on_close_obj;
	PyGILState_STATE                   state;
	PyObject                         * args;
	PyObject                         * result;
	VortexCtx                        * ctx = CONN_CTX(conn);

	/* notify on close notification received */
	py_vortex_log (PY_VORTEX_DEBUG, "found on close notification for connection id=%d, (internal: %p)", 
		       vortex_connection_get_id (conn), _on_close_obj);
	
	/*** bridge into python ***/
	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (2);

	Py_INCREF (on_close_obj->py_conn);
	PyTuple_SetItem (args, 0, __PY_OBJECT (on_close_obj->py_conn));
	Py_INCREF (on_close_obj->on_close_data);
	PyTuple_SetItem (args, 1, on_close_obj->on_close_data);

	/* record handler */
	START_HANDLER (on_close_obj->on_close);

	/* now invoke */
	result = PyObject_Call (on_close_obj->on_close, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (on_close_obj->on_close);

	py_vortex_log (PY_VORTEX_DEBUG, "connection on close notification finished, checking for exceptions..");
	py_vortex_handle_and_clear_exception (__PY_OBJECT (on_close_obj->py_conn));

	Py_XDECREF (result);
	Py_DECREF (args);

	/* now release the rest of data */
	Py_DECREF (on_close_obj->py_conn);
	Py_DECREF (on_close_obj->on_close);
	Py_DECREF (on_close_obj->on_close_data);

	/* unref the node itself */
	axl_free  (on_close_obj);

	/* release the GIL */
	PyGILState_Release(state);

	return;
}

PyObject * py_vortex_connection_set_on_close (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject                         * on_close      = NULL;
	PyObject                         * on_close_data = Py_None;
	PyVortexConnectionSetOnCloseData * on_close_obj;
	
	/* now parse arguments */
	static char *kwlist[] = {"on_close", "on_close_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &on_close, &on_close_data)) 
		return NULL;

	/* check handler received */
	if (on_close == NULL || ! PyCallable_Check (on_close)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "received on_close handler which is not a callable object");
		return NULL;
	} /* end if */

	/* configure an on close handler to bridge into python */
	on_close_obj = axl_new (PyVortexConnectionSetOnCloseData, 1);

	/* acquire a reference to the connection */
	on_close_obj->py_conn = self;
	Py_INCREF (self);

	/* configure on_close handler */
	on_close_obj->on_close = on_close;
	Py_INCREF (on_close);

	/* configure on_close_data handler data */
	if (on_close_data == NULL)
		on_close_data = Py_None;
	on_close_obj->on_close_data = on_close_data;
	Py_INCREF (on_close_data);

	/* configure on_close_full */
	vortex_connection_set_on_close_full (
               /* the connection with on close */
	       py_vortex_connection_get (on_close_obj->py_conn),
	       /* the handler */
	       py_vortex_connection_set_on_close_handler, 
	       /* the object with all references */
	       on_close_obj);

	/* create a handle that allows to remove this particular
	   handler. This handler can be used to remove the on close
	   handler */
	return py_vortex_handle_create (on_close_obj, NULL);
}

static PyObject * py_vortex_connection_remove_on_close (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject                         * handle      = NULL;
	PyVortexConnectionSetOnCloseData * on_close_obj;
	axl_bool                           result;
	
	/* now parse arguments */
	static char *kwlist[] = {"handle", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &handle)) 
		return NULL;

	/* if None is received, just return */
	if (! py_vortex_handle_check (handle)) {
		Py_INCREF (Py_False);
		return Py_False;
	} /* end if */
	
	/* get on_close object */
	on_close_obj = py_vortex_handle_get (handle);
	py_vortex_handle_nullify (handle);

	/* call to remove close handler */
	result = vortex_connection_remove_on_close_full (py_vortex_connection_get (self), 
							 py_vortex_connection_set_on_close_handler,
							 on_close_obj);

	/* finish close object */
	if (on_close_obj) {
		py_vortex_log (PY_VORTEX_DEBUG, "finishing on close object reference (%p)", on_close_obj);
		Py_DECREF (on_close_obj->py_conn);
		Py_DECREF (on_close_obj->on_close);
		Py_DECREF (on_close_obj->on_close_data);
		axl_free (on_close_obj);
	} /* end if */

	if (result) {
		/* handler removed */
		Py_INCREF (Py_True);
		return Py_True;
	} /* end if */

	Py_INCREF (Py_False);
	return Py_False;
}

typedef struct _PyVortexConnectionSelectChannels {
	const char * profile;
	PyObject   * list;
	PyObject   * conn;
} PyVortexConnectionSelectChannels;

axl_bool  py_vortex_connection_find_by_uri_select_channels (VortexChannel * channel, axlPointer user_data)
{
	PyVortexConnectionSelectChannels * data = (PyVortexConnectionSelectChannels *) user_data;
	PyObject                         * py_channel;
	
	/* check channel to run the profile selected */
	if (vortex_channel_is_running_profile (channel, data->profile)) {
		py_vortex_log (PY_VORTEX_DEBUG, "found channel=%s running the profile, adding to the result", 
			       data->profile);
		/* found, add it to the list */
		py_channel = py_vortex_channel_create (channel);
		if (PyList_Append (data->list, py_channel) == -1) {
			py_vortex_log (PY_VORTEX_CRITICAL, "failed to add channel %p into the list");
			py_vortex_handle_and_clear_exception (data->conn);
			return axl_true;
		}
		Py_XDECREF (py_channel);
	}

	/* always returns axl_false to check all channels */
	return axl_false;
}

static PyObject * py_vortex_connection_find_by_uri (PyObject * self, PyObject * args)
{
	const char                       * profile = NULL; 
	PyObject                         * result;
	PyVortexConnectionSelectChannels * data = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "z", &profile)) 
		return NULL;

	/* init the result */
	result = PyList_New (0);

	/* find all channels with the provided profile */
	data          = axl_new (PyVortexConnectionSelectChannels, 1);
	data->profile = profile;
	data->list    = result;
	data->conn    = self;
	vortex_connection_get_channel_by_func (PY_VORTEX_CONNECTION (self)->conn, 
					       py_vortex_connection_find_by_uri_select_channels,
					       data);

	/* free memory used */
	axl_free (data);
	return result;
}

static PyObject * py_vortex_connection_set_data (PyVortexConnection * self, PyObject * args)
{
	const char  * key = NULL; 
	PyObject    * obj = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "sO", &key, &obj)) 
		return NULL;
	
	/* check key to not be NULL or empty */
	if (key == NULL || strlen (key) == 0) {
		PyErr_Format (PyExc_ValueError, "Key data index is NULL or empty, this is not allowed.");
		return NULL;
	} /* end if */
	
	/* increment object ref count */
	Py_INCREF (obj);

	/* store in the connection */
	vortex_connection_set_data_full (self->conn, axl_strdup (key), obj, axl_free, (axlDestroyFunc) py_vortex_decref);

	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_vortex_connection_skip_conn_close (PyVortexConnection * self, PyObject * args)
{
	axl_bool skip_conn_close = axl_true;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "|i", &skip_conn_close))
		return NULL;

	/* set value received */
	self->skip_conn_close = skip_conn_close;
	
	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_vortex_connection_block (PyVortexConnection * self, PyObject * args)
{
	axl_bool block = axl_true;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "|i", &block))
		return NULL;

	/* call to block connection */
	vortex_connection_block (self->conn, block);

	/* done, return ok */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_vortex_connection_is_blocked (PyVortexConnection * self, PyObject * args)
{
	axl_bool is_blocked;

	/* call to block connection */
	is_blocked = vortex_connection_is_blocked (self->conn);

	/* done, return ok */
	if (is_blocked) {
		Py_INCREF (Py_True);
		return Py_True;
	}
	Py_INCREF (Py_False);
	return Py_False;
}

static PyObject * py_vortex_connection_get_data (PyVortexConnection * self, PyObject * args)
{
	const char  * key = NULL; 
	PyObject    * obj = NULL;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "s", &key)) 
		return NULL;
	
	/* check key to not be NULL or empty */
	if (key == NULL || strlen (key) == 0) {
		PyErr_Format (PyExc_ValueError, "Key data index is NULL or empty, this is not allowed.");
		return NULL;
	} /* end if */
	
	/* get the reference */
	obj = vortex_connection_get_data (self->conn, key);
	if (obj == NULL)
		obj = Py_None;

	/* return the object reference */
	Py_INCREF (obj);
	return obj;
}

static PyMethodDef py_vortex_connection_methods[] = { 
	/* is_ok */
	{"is_ok", (PyCFunction) py_vortex_connection_is_ok, METH_NOARGS,
	 "Allows to check current vortex.Connection status. In the case False is returned the connection is no longer operative. "},
	/* open_channel */
	{"open_channel", (PyCFunction) py_vortex_connection_open_channel, METH_VARARGS | METH_KEYWORDS,
	 "Allows to open a channel on the provided connection (BEEP session)."},
	/* pop_channel_error */
	{"pop_channel_error", (PyCFunction) py_vortex_connection_pop_channel_error, METH_NOARGS,
	 "API wrapper for vortex_connection_pop_channel_error. Each time this method is called, a tulple (code, msg) is returned containing the error code and the error message. One tuple is returned for each channel error found. In the case no error is stored on the connection None is returned."},
	/* set_on_close */
	{"set_on_close", (PyCFunction) py_vortex_connection_set_on_close, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for vortex_connection_set_on_close_full. This method allows to configure a handler which will be called in case the connection is closed. This is useful to detect client or server broken connection."},
	/* remove_on_close */
	{"remove_on_close", (PyCFunction) py_vortex_connection_remove_on_close, METH_VARARGS | METH_KEYWORDS,
	 "API wrapper for vortex_connection_remove_on_close_full. This method allows to remove a particular on close handler installed by the method .set_on_close."},
	/* find_by_uri */
	{"find_by_uri", (PyCFunction) py_vortex_connection_find_by_uri, METH_VARARGS,
	 "Allows to get a reference to all channels opened on the conection using a particular profile."},
	/* channel_pool_new */
	{"channel_pool_new", (PyCFunction) py_vortex_connection_channel_pool_new, METH_VARARGS | METH_KEYWORDS,
	 "Allows to create a new channel pool (vortex.ChannelPool) on the provided connection. Channel pools are a Vortex abstraction that allows managing, grouping and reusing channel references in an efficient manner."},
	/* pool */
	{"pool", (PyCFunction) py_vortex_connection_get_pool, METH_VARARGS | METH_KEYWORDS,
	 "Allows to get a reference to the default pool (no parameters) or to a selected pool with a particular id."},
	/* close */
	{"close", (PyCFunction) py_vortex_connection_close, METH_NOARGS,
	 "Allows to close a the BEEP session (vortex.Connection) following all BEEP close negotation phase. The method returns True in the case the connection was cleanly closed, otherwise False is returned. If this operation finishes properly, the reference should not be used."},
	/* shutdown */
	{"shutdown", (PyCFunction) py_vortex_connection_shutdown, METH_NOARGS,
	 "Allows to shutdown the BEEP session. This operation closes the underlaying transport without going into the full BEEP close process. It is still required to call to .close method to fully finish the connection. After the shutdown the caller can still use the reference and check its status. After a close operation the connection cannot be used again."},
	/* incref */
	{"incref", (PyCFunction) py_vortex_connection_incref, METH_NOARGS,
	 "Allows to increment reference counting of the python object (vortex.Connection) holding the connection."},
	/* decref */
	{"decref", (PyCFunction) py_vortex_connection_decref, METH_NOARGS,
	 "Allows to decrement reference counting of the python object (vortex.Connection) holding the connection."},
	/* set_data */
	{"set_data", (PyCFunction) py_vortex_connection_set_data, METH_VARARGS,
	 "Allows to sent arbitary object references index by a name associated to the connection. This API is set on top vortex_connection_set_data_full."},
	/* get_data */
	{"get_data", (PyCFunction) py_vortex_connection_get_data, METH_VARARGS,
	 "Allows to retrieve arbitary object references index by a name associated to the connection that were configured with Connection.set_data. This API is set on top vortex_connection_set_data_full."},
	/* skip_conn_close */
	{"skip_conn_close", (PyCFunction) py_vortex_connection_skip_conn_close, METH_VARARGS,
	 "Allows to configure this vortex.Connection object to not close automatically its associated reference when the object is collected. Calling without arguments activates skip connection close on dealloction, otherwise pass skip=False to leave default behaviour."},
	/* block */
	{"block", (PyCFunction) py_vortex_connection_block, METH_VARARGS,
	 "Allows to receiving any content from the provided connection. This method uses vortex_connection_block."},
	/* is_blocked */
	{"is_blocked", (PyCFunction) py_vortex_connection_is_blocked, METH_VARARGS,
	 "Allows to check blocked status applied by vortex.Connection.block method."},
 	{NULL}  
}; 


static PyTypeObject PyVortexConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Connection",       /* tp_name*/
    sizeof(PyVortexConnection),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_connection_dealloc, /* tp_dealloc*/
    0,                         /* tp_print*/
    0,                         /* tp_getattr*/
    0,                         /* tp_setattr*/
    0,                         /* tp_compare*/
    0,                         /* tp_repr*/
    0,                         /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                         /* tp_as_mapping*/
    0,                         /* tp_hash */
    0,                         /* tp_call*/
    0,                         /* tp_str*/
    py_vortex_connection_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.Connection, the object used to represent a connected BEEP session.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_connection_methods,     /* tp_methods */
    0, /* py_vortex_connection_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_connection_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_connection_new,  /* tp_new */

};

/** 
 * @brief Allows to create a new PyVortexConnection instance using the
 * reference received.
 *
 * NOTE: At server side notification use
 * py_vortex_connection_find_reference to avoid creating/finishing
 * references for each notification.
 *
 * @param conn The connection to use as reference to wrap
 *
 * @param acquire_ref Allows to configure if py_conn reference must
 * acquire a reference to the connection.
 *
 * @param close_ref Allows to signal the object created to close or
 * not the connection when the reference is garbage collected.
 *
 * @return A newly created PyVortexConnection reference.
 */
PyObject * py_vortex_connection_create   (VortexConnection * conn, 
					  axl_bool           acquire_ref,
					  axl_bool           close_ref)
{
	/* return a new instance */
	PyVortexConnection * obj = (PyVortexConnection *) PyObject_CallObject ((PyObject *) &PyVortexConnectionType, NULL); 

	/* check ref created */
	if (obj == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to create PyVortexConnection object, returning NULL");
		return NULL;
	} /* end if */

	/* configure close_ref */
	obj->close_ref = close_ref;

	/* set channel reference received */
	if (obj && conn) {
		/* check to acquire a ref */
		if (acquire_ref) {
			py_vortex_log (PY_VORTEX_DEBUG, "acquiring a reference to conn: %p (role: %s)",
				       conn, __py_vortex_connection_stringify_role (conn));
			/* check ref */
			if (! vortex_connection_ref_internal (conn, "py_vortex_connection_create", axl_false)) {
				py_vortex_log (PY_VORTEX_CRITICAL, "failed to acquire reference, unable to create connection");
				Py_DECREF (obj);
				return NULL;
			}
		} /* end if */

		/* configure the reference */
		obj->conn = conn;
	} /* end if */

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @internal Function used to reuse PyVortexConnection references
 * rather creating and finishing them especially at server side async
 * notification.
 *
 * This function is designed to avoid using
 * py_vortex_connection_create providing a way to reuse references
 * that, not only saves memory, but are available after finishing the
 * python context that created the particular connection reference.
 *
 * @param conn The connection for which its reference will be looked up.
 *
 * @param py_ctx The vortex.Ctx object where to lookup for an already
 * created vortex.Connection reference.
 */
PyObject * py_vortex_connection_find_reference (VortexConnection * conn)
{
	return py_vortex_connection_create (
		/* connection to wrap */
		conn, 
		/* acquire a reference to the connection */
		axl_true,  
		/* do not close the connection when the reference is collected, close_ref=axl_false */
		axl_false);
}

/** 
 * @brief Allows to store a python object into the provided
 * vortex.Connection object, incrementing the reference count. The
 * object is automatically removed when the vortex.Connection
 * reference is collected.
 */
void        py_vortex_connection_register (PyObject   * py_conn, 
					   PyObject   * data,
					   const char * key,
					   ...)
{
	va_list    args;
	char     * full_key;

	/* check data received */
	if (key == NULL || py_conn == NULL)
		return;

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);

	/* check to remove */
	if (data == NULL) {
		vortex_connection_set_data (((PyVortexConnection *)py_conn)->conn, full_key, NULL);
		axl_free (full_key);
		return;
	} /* end if */
	
	/* now register the data received into the key created */
	py_vortex_log (PY_VORTEX_DEBUG, "registering key %s = %p on vortex.Connection %p",
		       full_key, data, py_conn);
	Py_INCREF (data);
	vortex_connection_set_data_full (((PyVortexConnection *)py_conn)->conn, full_key, data, axl_free, (axlDestroyFunc) py_vortex_decref);
	return;
}


/** 
 * @brief Allows to get the object associated to the key provided. The
 * reference returned is still owned by the internal hash. Use
 * Py_INCREF in the case a new reference must owned by the caller.
 */
PyObject  * py_vortex_connection_register_get (PyObject * py_conn,
					       const char * key,
					       ...)
{
	va_list    args;
	char     * full_key;
	PyObject * data;

	/* check data received */
	if (key == NULL || py_conn == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to register data, key %p or vortex.Connection %p reference is null",
			       key, py_conn);
		return NULL;
	} /* end if */

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);
	
	/* now register the data received into the key created */
	data = __PY_OBJECT (vortex_connection_get_data (((PyVortexConnection *)py_conn)->conn, full_key));
	py_vortex_log (PY_VORTEX_DEBUG, "returning key %s = %p on vortex.Connection %p",
		       full_key, data, py_conn);
	axl_free (full_key);
	return data;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyVortexConnection reference.
 */
axl_bool             py_vortex_connection_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexConnectionType);
}

/** 
 * @brief Inits the vortex connection module. It is implemented as a type.
 */
void init_vortex_connection (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexConnectionType) < 0)
		return;
	
	Py_INCREF (&PyVortexConnectionType);
	PyModule_AddObject(module, "Connection", (PyObject *)&PyVortexConnectionType);

	return;
}



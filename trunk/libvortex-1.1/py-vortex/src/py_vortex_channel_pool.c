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

#include <py_vortex_channel_pool.h>

typedef struct _PyVortexChannelPoolData {
	int              pool_id;
	PyObject       * create_channel;
	PyObject       * create_channel_user_data;
	PyObject       * close;
	PyObject       * close_user_data;
	PyObject       * received;
	PyObject       * received_data;
	PyObject       * on_channel_pool_created;
	PyObject       * user_data;

} PyVortexChannelPoolData;

struct _PyVortexChannelPool {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the VortexChannelPool object */
	VortexChannelPool * pool;

	/** 
	 * @brief Reference to the python connection.
	 */
	PyObject       * py_conn;

	/** 
	 * @internal Reference to data handler pointers.
	 */
	PyVortexChannelPoolData * data;
};

void py_vortex_channel_pool_release_data (PyVortexChannelPoolData * data)
{
	if (data == NULL)
		return;

	py_vortex_log (PY_VORTEX_DEBUG, "Finishing PyVortexChannelPoolData %p, pool id: %d", data, data->pool_id);

	Py_DECREF (data->create_channel);
	data->create_channel = NULL;

	Py_DECREF (data->create_channel_user_data);
	data->create_channel_user_data = NULL;

	Py_DECREF (data->close);
	data->close = NULL;

	Py_DECREF (data->close_user_data);
	data->close_user_data = NULL;

	Py_DECREF (data->received);
	data->received = NULL;

	Py_DECREF (data->received_data);
	data->received_data = NULL;

	Py_DECREF (data->on_channel_pool_created);
	data->on_channel_pool_created = NULL;

	Py_DECREF (data->user_data);
	data->user_data = NULL;

	axl_free (data);

	return;
}

PyVortexChannelPoolData * py_vortex_channel_pool_get_data (PyVortexChannelPool * py_pool)
{
	char                    * key;
	PyVortexChannelPoolData * data;

	/* return direct reference */
	if (py_pool->data)
		return py_pool->data;

	/* if not find it */
	key  = axl_strdup_printf ("py:vo:ch:pool-data:%d", vortex_channel_pool_get_id (py_pool->pool));
	data = vortex_connection_get_data (py_vortex_connection_get (py_pool->py_conn), key);
	axl_free (key);
	return data;
}

void py_vortex_channel_pool_set_data (PyVortexChannelPool * py_pool)
{
	/* store data */
	vortex_connection_set_data_full (py_vortex_connection_get (py_pool->py_conn),
					 /* key */
					 axl_strdup_printf ("py:vo:ch:pool-data:%d", vortex_channel_pool_get_id (py_pool->pool)),
					 /* data */
					 py_pool->data, 
					 axl_free, (axlDestroyFunc) py_vortex_channel_pool_release_data);

	/* update internal pool data id */
	py_pool->data->pool_id = vortex_channel_pool_get_id (py_pool->pool);
	return;
}

/** 
 * @brief Allows to get the VortexChannelPool reference inside the
 * PyVortexChannelPool.
 *
 * @param py_conn The reference that holds the channel_pool inside.
 *
 * @return A reference to the VortexChannelPool inside or NULL if it fails.
 */
VortexChannelPool * py_vortex_channel_pool_get  (PyObject * py_pool)
{
	PyVortexChannelPool * _py_pool = (PyVortexChannelPool *) py_pool;

	/* return NULL reference */
	if (_py_pool == NULL)
		return NULL;
	/* return py channel_pool */
	return _py_pool->pool;
}

static int py_vortex_channel_pool_init_type (PyVortexChannelPool *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.ChannelPool
 */
static PyObject * py_vortex_channel_pool_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexChannelPool * self;
	
	/* create the object */
	self = (PyVortexChannelPool *)type->tp_alloc(type, 0);
	
	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.ChannelPool
 */
static void py_vortex_channel_pool_dealloc (PyVortexChannelPool* self)
{
	int pool_id = vortex_channel_pool_get_id (self->pool);

	py_vortex_log (PY_VORTEX_DEBUG, "finishing PyVortexChannelPool id: %d (%p)", 
		       pool_id, self);

	/* finish connection */
	Py_DECREF (self->py_conn);

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_channel_pool_get_attr (PyObject *o, PyObject *attr_name) {
	const char          * attr = NULL;
	PyObject            * result;
	PyVortexChannelPool * self = (PyVortexChannelPool *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	if (axl_cmp (attr, "id")) {
		/* found error_msg attribute */
		return Py_BuildValue ("i", vortex_channel_pool_get_id (self->pool));
	} else if (axl_cmp (attr, "ctx")) {
		/* found ctx attribute */
		result = py_vortex_connection_get_ctx (self->py_conn);
		Py_XINCREF (result);
		return result;
	} else if (axl_cmp (attr, "conn")) {
		/* found conn attribute */
		Py_XINCREF (self->py_conn);
		return self->py_conn;
	} else if (axl_cmp (attr, "channel_count")) {
		return Py_BuildValue ("i", vortex_channel_pool_get_num (self->pool));
	} else if (axl_cmp (attr, "channel_available")) {
		return Py_BuildValue ("i", vortex_channel_pool_get_available_num (self->pool));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static PyObject * py_vortex_channel_pool_next_ready (PyVortexChannelPool * self, PyObject * args, PyObject * kwds)
{
	axl_bool                  auto_inc  = axl_false;
	PyObject                * user_data = NULL;
	VortexChannel           * channel;
	PyObject                * py_channel;
	PyVortexChannelPoolData * data;
	
	/* now parse arguments */
	static char *kwlist[] = {"auto_inc", "user_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iO", kwlist, &auto_inc, &user_data)) 
		return NULL;

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS;

	/* call to return the channel */
	channel = vortex_channel_pool_get_next_ready_full (self->pool, auto_inc, user_data);

	/* end threads */
	Py_END_ALLOW_THREADS;

	if (channel == NULL) {
		/* set configured */
		Py_INCREF (Py_None);
	} /* end if */

	/* create the python channel reference */
	py_channel = py_vortex_channel_create (channel, (PyObject *) self->py_conn);

	/* get data */
	data = py_vortex_channel_pool_get_data (self);

	/* now configure frame received */
	if (data->received) {
		py_vortex_log (PY_VORTEX_DEBUG, "setting frame received handler for channel=%d (%p) returned by next_ready()",
			       vortex_channel_get_number (channel), py_channel);
		py_vortex_channel_configure_frame_received ((PyVortexChannel *) py_channel, data->received, data->received_data);
	} else {
		py_vortex_log (PY_VORTEX_DEBUG, "not setting frame received handler for channel=%d (%p) returned by next_ready()",
			       vortex_channel_get_number (channel), py_channel);
	} /* end if */
	
	/* return reference created */
	return py_channel;
}

static PyObject * py_vortex_channel_pool_release (PyVortexChannelPool * self, PyObject * args, PyObject * kwds)
{
	PyObject      * channel = NULL;
	
	/* now parse arguments */
	static char *kwlist[] = {"channel", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &channel))
		return NULL;

	/* call to release the channel */
	vortex_channel_pool_release_channel (self->pool, py_vortex_channel_get (channel));

	/* create the python channel reference */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef py_vortex_channel_pool_methods[] = { 
	/* next_ready */
	{"next_ready", (PyCFunction) py_vortex_channel_pool_next_ready, METH_VARARGS | METH_KEYWORDS,
	 "Allows to get next channel ready on the pool. "},
	/* release */
	{"release", (PyCFunction) py_vortex_channel_pool_release, METH_VARARGS | METH_KEYWORDS,
	 "Allows release a channel used from the pool that was obtained via .next_ready() method. "},
 	{NULL}  
}; 


static PyTypeObject PyVortexChannelPoolType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.ChannelPool",       /* tp_name*/
    sizeof(PyVortexChannelPool),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_channel_pool_dealloc, /* tp_dealloc*/
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
    py_vortex_channel_pool_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.ChannelPool, the object used to represent a pool of channels (VortexChannelPool).",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_channel_pool_methods,     /* tp_methods */
    0, /* py_vortex_channel_pool_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_channel_pool_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_channel_pool_new,  /* tp_new */

};

VortexChannel * py_vortex_channel_pool_create_channel (VortexConnection      * connection, 
						       int                     channel_num, 
						       const char            * profile, 
						       VortexOnCloseChannel    on_close, 
						       axlPointer              on_close_user_data, 
						       VortexOnFrameReceived   on_received, 
						       axlPointer              on_received_user_data, 
						       axlPointer              user_data,
						       axlPointer              get_next_data)
{
	/* reference to the python channel */
	PyVortexChannelPool     * pool       = user_data;
	PyVortexChannelPoolData * data       = py_vortex_channel_pool_get_data (pool);
	PyObject                * _next_data = get_next_data;
	PyObject                * py_conn    = pool->py_conn;
	PyObject                * args;
	PyGILState_STATE          state;
	PyObject                * result;
	VortexChannel           * channel    = NULL;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (9);

	/* the following function PyTuple_SetItem "steals" a reference
	 * which is the python way to say that we are transfering the
	 * ownership of the reference to that function, making it
	 * responsible of calling to Py_DECREF when required. */
	py_vortex_log (PY_VORTEX_DEBUG, "requesting user space to create channel num %d, with profile %s",
		       channel_num, profile);
	Py_INCREF (py_conn);
	PyTuple_SetItem (args, 0, py_conn);
	PyTuple_SetItem (args, 1, Py_BuildValue ("i", channel_num));
	PyTuple_SetItem (args, 2, Py_BuildValue ("s", profile));

	Py_INCREF (data->received);
	PyTuple_SetItem (args, 3, data->received);

	Py_INCREF (data->received_data);
	PyTuple_SetItem (args, 4, data->received_data);
	
	Py_INCREF (data->close);
	PyTuple_SetItem (args, 5, data->close);

	Py_INCREF (data->close_user_data);
	PyTuple_SetItem (args, 6, data->close_user_data);

	Py_INCREF (data->create_channel_user_data);
	PyTuple_SetItem (args, 7, data->create_channel_user_data);

	if (_next_data == NULL)
		_next_data = Py_None;
	Py_INCREF (_next_data);
	PyTuple_SetItem (args, 8, _next_data);

	/* now invoke */
	result = PyObject_Call (data->create_channel, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "channel pool create handler notification finished, checking for exceptions..");
	py_vortex_handle_and_clear_exception (py_conn);

	/* get channel reference */
	if (py_vortex_channel_check (result)) {
		channel = py_vortex_channel_get (result);
		py_vortex_log (PY_VORTEX_DEBUG, "Received channel reference %p, channel num: %d",
			       channel, vortex_channel_get_number (channel));
	}

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args); 
	Py_XDECREF (result);

	/* release the GIL */
	PyGILState_Release(state);

	return channel;
}

axl_bool py_vortex_channel_pool_close_channel (int                channel_num, 
					       VortexConnection * connection, 
					       axlPointer         user_data)
{
	return axl_true;
}


void py_vortex_channel_pool_on_created (VortexChannelPool * _pool, 
					axlPointer          user_data)
{
	/* reference to the python channel */
	PyVortexChannelPool     * py_pool    = user_data;
	PyVortexChannelPoolData * data       = py_vortex_channel_pool_get_data (py_pool);
	PyObject                * args;
	PyObject                * result;
	PyGILState_STATE          state;

	/* set internal pool reference */
	py_pool->pool = _pool;
	py_vortex_log (PY_VORTEX_DEBUG, "on pool created notification: PyVortexChanenlPool ref: %p, VortexChannelPool: %p",
		       py_pool, py_pool->pool);

	/* set hook on on the pool */
	py_vortex_channel_pool_set_data (py_pool);

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (2);

	/* The following py_pool reference is not Py_INCREF'ed because
	   we are transfering total ownership of the reference to the
	   on created handler */
	PyTuple_SetItem (args, 0, (PyObject*) py_pool);
	Py_INCREF (data->user_data);
	PyTuple_SetItem (args, 1, data->user_data);

	/* now invoke */
	result = PyObject_Call (data->on_channel_pool_created, args, NULL);

	py_vortex_log (PY_VORTEX_DEBUG, "on channel pool created notification finished, checking for exceptions %p %p (conn id: %d)..",
		       py_pool, py_pool->py_conn, vortex_connection_get_id (py_vortex_connection_get (py_pool->py_conn)));
	
	py_vortex_handle_and_clear_exception (NULL); 

	/* release tuple and result returned (which may be null) */
	Py_DECREF  (args);
	Py_XDECREF (result);

	/* release the GIL */
	PyGILState_Release (state);

	return;
}

/** 
 * @brief Creates an empty channel pool instance and set the internal
 * reference it represents.
 */
PyObject * py_vortex_channel_pool_create   (PyObject           * py_conn,
					    PyObject           * ctx,
					    const char         * profile,
					    int                  init_num,
					    PyObject           * create_channel,
					    PyObject           * create_channel_user_data,
					    PyObject           * close,
					    PyObject           * close_user_data,
					    PyObject           * received,
					    PyObject           * received_user_data,
					    PyObject           * on_channel_pool_created,
					    PyObject           * user_data)
{
	/* return a new instance */
	PyVortexChannelPool     * obj;
	

	/* check callable handlers */
	PY_VORTEX_IS_CALLABLE (create_channel, "create_channel handler is not a callable reference, unable to create channel pool");
	PY_VORTEX_IS_CALLABLE (close, "close handler is not a callable reference, unable to create channel pool");
	PY_VORTEX_IS_CALLABLE (received, "received handler is not a callable reference, unable to create channel pool");
	PY_VORTEX_IS_CALLABLE (on_channel_pool_created, "create_channel handler is not a callable reference, unable to create channel pool");

	/* check init num value received */
	if (init_num <= 0) {
		PyErr_Format (PyExc_ValueError, "Expected to receive a init_num > 0 but found 0 or a lower value");
		return NULL;
	} /* end if */

	/* create the empty object here */
	obj = (PyVortexChannelPool *) py_vortex_channel_pool_empty (py_conn, ctx);

	/* create data structure */
	obj->data = axl_new (PyVortexChannelPoolData, 1);

	/* set all handlers */
	PY_VORTEX_SET_REF (obj->data->create_channel, create_channel);
	PY_VORTEX_SET_REF (obj->data->create_channel_user_data, create_channel_user_data);
	PY_VORTEX_SET_REF (obj->data->close, close);
	PY_VORTEX_SET_REF (obj->data->close_user_data, close_user_data);
	PY_VORTEX_SET_REF (obj->data->received, received);
	PY_VORTEX_SET_REF (obj->data->received_data, received_user_data);
	PY_VORTEX_SET_REF (obj->data->on_channel_pool_created, on_channel_pool_created);
	PY_VORTEX_SET_REF (obj->data->user_data, user_data);

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS;

	/* create the pool */
	obj->pool = vortex_channel_pool_new_full (py_vortex_connection_get (py_conn),
						  profile,
						  init_num,
						  /* create channel */
						  create_channel ? py_vortex_channel_pool_create_channel : NULL,
						  create_channel ? obj : NULL,
						  /* close channel handler */
						  close ? py_vortex_channel_pool_close_channel : NULL,
						  close ? obj : NULL,
						  /* received handler */
						  NULL, NULL,
						  /* on created pool */
						  on_channel_pool_created ? py_vortex_channel_pool_on_created : NULL,
						  on_channel_pool_created ? obj : NULL);

	/* end threads */
	Py_END_ALLOW_THREADS;
				       
	/* return None in the case it was requested to notify the pool
	   on the handler */
	if (on_channel_pool_created) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* set pool internal data here */
	py_vortex_channel_pool_set_data (obj);

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Allows to create an empty vortex.ChannelPool object (still
 * without internal VortexChannelPool reference).
 */
PyObject            * py_vortex_channel_pool_empty    (PyObject           * py_conn,
						       PyObject           * ctx)
{
	/* return a new instance */
	PyVortexChannelPool * obj = (PyVortexChannelPool *) PyObject_CallObject ((PyObject *) &PyVortexChannelPoolType, NULL); 

	/* check ref created */
	if (obj == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to create PyVortexChannelPool object, returning NULL");
		return NULL;
	} /* end if */

	/* set context reference */
	obj->py_conn       = py_conn;
	Py_INCREF (py_conn);

	/* return object */
	return (PyObject *) obj;
}

PyObject            * py_vortex_channel_pool_find_reference (VortexChannelPool * pool,
							     PyObject          * _py_conn,
							     PyObject          * py_ctx)
{
	PyVortexChannelPool * py_pool = (PyVortexChannelPool *) py_vortex_channel_pool_empty (_py_conn, py_ctx);

	py_pool->pool = pool;
	py_pool->data = py_vortex_channel_pool_get_data (py_pool);
	
	return __PY_OBJECT (py_pool);
}

/** 
 * @brief Allows to get a reference to the PyVortexCtx reference used
 * by the provided PyVortexChannelPool.
 * 
 * @param py_conn A reference to PyVortexChannelPool object.
 */
PyObject           * py_vortex_channel_pool_get_ctx  (PyObject         * py_pool)
{
	PyVortexChannelPool * _py_pool = (PyVortexChannelPool *) py_pool;

	/* check object received */
	if (! py_vortex_channel_pool_check (py_pool))
		return NULL;

	/* return context */
	return py_vortex_connection_get_ctx (_py_pool->py_conn);
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyVortexChannelPool reference.
 */
axl_bool             py_vortex_channel_pool_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexChannelPoolType);
}


/** 
 * @brief Inits the vortex channel pool module. It is implemented as a type.
 */
void init_vortex_channel_pool (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexChannelPoolType) < 0)
		return;
	
	Py_INCREF (&PyVortexChannelPoolType);
	PyModule_AddObject(module, "ChannelPool", (PyObject *)&PyVortexChannelPoolType);

	return;
}



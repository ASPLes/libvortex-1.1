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

#include <py_vortex_channel.h>

struct _PyVortexChannel {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD
	/* pointer to the VortexChannel object */
	VortexChannel * channel;
	/* frame received handler and data */
	PyObject      * frame_received;
	PyObject      * frame_received_data;
};

static int py_vortex_channel_init_type (PyVortexChannel *self, PyObject *args, PyObject *kwds)
{
    return 0;
}


/** 
 * @brief Function used to allocate memory required by the object vortex.Channel
 */
static PyObject * py_vortex_channel_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexChannel    * self;

	/* create the object */
	self = (PyVortexChannel *)type->tp_alloc(type, 0);

	/* and do nothing more */
	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.Channel
 */
static void py_vortex_channel_dealloc (PyVortexChannel* self)
{
	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_channel_close function. 
 */
static PyObject * py_vortex_channel_close (PyVortexChannel* self)
{
	PyObject *_result;
	axl_bool  result;

	/* close the channel */
	result  = vortex_channel_close (self->channel, NULL);
	_result = Py_BuildValue ("i", result);

	/* check to nullify channel reference in the case the channel is closed */
	if (result) 
		self->channel = NULL;
	
	return _result;
}


/** 
 * @internal Function used to implement general frame received
 * handling.
 */
void     py_vortex_channel_received     (VortexChannel    * channel,
					 VortexConnection * connection,
					 VortexFrame      * frame,
					 axlPointer         user_data)
{
	/* reference to the python channel */
	PyVortexChannel    * py_channel = user_data; 
	PyVortexConnection * py_conn;
	PyVortexFrame      * py_frame;
	PyObject           * args;
	PyGILState_STATE     state;

	printf ("py_vortex_channel_received: Frame received, preparing invocation..\n");

	/* acquire the GIL */
	state = PyGILState_Ensure();

	printf ("py_vortex_channel_received: Ok..proceeding..\n");

	/* create a PyVortexFrame instance */
	py_frame = py_vortex_frame_create (frame);
	
	/* create a PyVortexConnection instance */
	py_conn  = py_vortex_connection_create (connection);

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);

	Py_INCREF ((PyObject *) py_conn);
	PyTuple_SetItem (args, 0, (PyObject *) py_conn);

	Py_INCREF ((PyObject *) py_channel);
	PyTuple_SetItem (args, 1, (PyObject *) py_channel);

	Py_INCREF ((PyObject *) py_frame);
	PyTuple_SetItem (args, 2, (PyObject *) py_frame);

	Py_INCREF ((PyObject *) py_channel->frame_received_data);
	PyTuple_SetItem (args, 3, (PyObject *) py_channel->frame_received_data);

	printf ("py_vortex_channel_received: Frame received, dictionary created..\n");

	/* now invoke */
	printf ("py_vortex_channel_frame_received Channel: %p\n", py_channel);
	printf ("py_vortex_channel_frame_received frame received: %p\n", py_channel->frame_received);
	printf ("py_vortex_channel_frame_received args: %p\n", args);
	
	PyObject_Call (py_channel->frame_received, args, NULL);

	printf ("py_vortex_channel_frame_received: Frame received, call complete..\n");
	
	/* release tuple and dict */
	Py_DECREF (args);

	printf ("py_vortex_channel_frame_received: Frame received, memory released..\n");

	/* release memory used by py_frame, py_conn, py_channel ? */

	/* release the GIL */
	PyGILState_Release(state);

	printf ("py_vortex_channel_frame_received: GIL released!..\n");

	return;
}

static PyObject * py_vortex_channel_set_frame_received (PyVortexChannel * self, PyObject * args, PyObject * kwds)
{
	PyObject * handler = NULL;
	PyObject * data    = NULL;

	/* parse args received */
	static char *kwlist[] = {"handler", "data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "O|O", kwlist, &handler, &data))
		return NULL;

	/* check values received */
	if (! PyCallable_Check (handler)) 
		return NULL;

	/* unref prevous handler and data associated if defined */
	if (self->frame_received) {
		Py_DECREF (self->frame_received);
	} /* end if */
	self->frame_received = handler;

	if (self->frame_received_data) {
		Py_DECREF (self->frame_received_data);
	} /* end if */
	self->frame_received_data = data;

	/* in the case no frame received configured, set None value */
	if (! self->frame_received_data) {
		/* set None */
		self->frame_received_data = Py_BuildValue ("");
	}

	/* increment reference counting */
	if (self->frame_received)
		Py_INCREF (self->frame_received);
	if (self->frame_received_data)
		Py_INCREF (self->frame_received_data);

	printf ("Configured frame received..\n");
	printf ("Channel: %p\n", self);
	printf ("frame received: %p\n", self->frame_received);

	/* reconfigure the frame received for used for all channels */
	vortex_channel_set_received_handler (self->channel, py_vortex_channel_received, self);

	/* return none */
	return Py_BuildValue ("");
}

static PyObject * py_vortex_channel_send_msg (PyVortexChannel * self, PyObject * args)
{
	const char * content = NULL;
	int          size    = 0;
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "si", &content, &size))
		return NULL;

	/* call to send the message */
	result = vortex_channel_send_msg (self->channel, content, size, &msg_no);

	/* return none in the case of failure */
	if (! result)
		return Py_BuildValue ("");

	/* and return the message number used in the case of proper
	 * send operation */
	return Py_BuildValue ("i", msg_no);
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_channel_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyVortexChannel * self = (PyVortexChannel *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "number")) {
		/* found error_msg attribute */
		return Py_BuildValue ("i", vortex_channel_get_number (self->channel));
	} else if (axl_cmp (attr, "profile")) {
		/* found error_msg attribute */
		return Py_BuildValue ("s", vortex_channel_get_profile (self->channel));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static PyMethodDef py_vortex_channel_methods[] = { 
	{"send_msg", (PyCFunction) py_vortex_channel_send_msg, METH_VARARGS,
	 "Allows to send the message with type MSG."},
	{"set_frame_received", (PyCFunction) py_vortex_channel_set_frame_received, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure the frame received handler."},
	{"close", (PyCFunction) py_vortex_channel_close, METH_NOARGS,
	 "Allows to close the selected channel and to remove it from the BEEP session."},
 	{NULL}  
}; 


static PyTypeObject PyVortexChannelType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Channel",       /* tp_name*/
    sizeof(PyVortexChannel),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_channel_dealloc, /* tp_dealloc*/
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
    py_vortex_channel_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.Channel, the object used to represent a BEEP channel running a particular profile.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_channel_methods,     /* tp_methods */
    0, /* py_vortex_channel_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_channel_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_channel_new,  /* tp_new */
};

/** 
 * @brief Creates new empty channel instance.
 */
PyVortexChannel * py_vortex_channel_create (VortexChannel * channel)
{
	/* return a new instance */
	PyVortexChannel * obj = (PyVortexChannel *) PyObject_CallObject ((PyObject *) &PyVortexChannelType, NULL);

	/* set channel reference received */
	if (obj)
		obj->channel = channel;

	/* return object */
	return obj;
}

/** 
 * @brief Inits the vortex channel module. It is implemented as a type.
 */
void init_vortex_channel (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexChannelType) < 0)
		return;
	
	Py_INCREF (&PyVortexChannelType);
	PyModule_AddObject(module, "Channel", (PyObject *)&PyVortexChannelType);

	return;
}



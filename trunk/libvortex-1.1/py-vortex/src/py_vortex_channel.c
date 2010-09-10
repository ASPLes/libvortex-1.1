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

#include <py_vortex_channel.h>

struct _PyVortexChannel {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the VortexChannel object */
	VortexChannel      * channel;
	
	/* reference to the connection */
	PyObject           * py_conn;

	/* frame received handler and data */
	PyObject           * frame_received;
	PyObject           * frame_received_data;

	/* @internal Reference to on channel created notify handler. */
	PyObject       * on_channel;
	PyObject       * on_channel_data;
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

	/* release reference associated */
	py_vortex_log  (PY_VORTEX_DEBUG, "Calling to unref the channel: %d (self: %p, self->channel: %p, refs: %d)..", 
			vortex_channel_get_number (self->channel), self, self->channel, vortex_channel_ref_count (self->channel));
	vortex_channel_unref (self->channel);
	self->channel = NULL; 

	/* decrement reference counting on py_conn */
	py_vortex_log (PY_VORTEX_DEBUG, "  Finishing reference to connection..");
	Py_XDECREF (__PY_OBJECT (self->py_conn));
	self->py_conn = NULL;

	/* finish frame received and associated data */
	py_vortex_log (PY_VORTEX_DEBUG, "  Finishing reference to frame received handler and associated data..");
	Py_CLEAR (self->frame_received);
	self->frame_received = NULL;
	Py_CLEAR (self->frame_received_data);
	self->frame_received_data = NULL;

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
	py_vortex_log (PY_VORTEX_DEBUG, "closing channel %d..", vortex_channel_get_number (self->channel));

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS
	result  = vortex_channel_close (self->channel, NULL);
	/* end threads */
	Py_END_ALLOW_THREADS

	_result = Py_BuildValue ("i", result);
	
	return _result;
}

/** 
 * @brief Increment reference counting
 */
static PyObject * py_vortex_channel_incref (PyVortexChannel* self)
{
	/* close the channel */
	Py_INCREF (__PY_OBJECT (self));
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Decrement reference counting.
 */
static PyObject * py_vortex_channel_decref (PyVortexChannel* self)
{
	/* close the channel */
	Py_DECREF (__PY_OBJECT (self));
	Py_INCREF (Py_None);
	return Py_None;
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
	PyObject           * py_conn;
	PyObject           * py_frame;
	PyObject           * args;
	PyGILState_STATE     state;
	PyObject           * result;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a PyVortexFrame instance */
	py_frame = py_vortex_frame_create (frame, axl_true);
	
	/* create a PyVortexConnection instance */
        py_conn  = py_channel->py_conn;
	Py_INCREF (py_conn);

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);

	/* the following function PyTuple_SetItem "steals" a reference
	 * which is the python way to say that we are transfering the
	 * ownership of the reference to that function, making it
	 * responsible of calling to Py_DECREF when required. */
	PyTuple_SetItem (args, 0, py_conn);

	Py_INCREF (py_channel);
	PyTuple_SetItem (args, 1, (PyObject *) py_channel);

	PyTuple_SetItem (args, 2, py_frame);

	/* increment reference counting because the tuple will
	 * decrement the reference passed when he thinks it is no
	 * longer used. */
	Py_INCREF (py_channel->frame_received_data);
	PyTuple_SetItem (args, 3, py_channel->frame_received_data);

	/* now invoke */
	result = PyObject_Call (py_channel->frame_received, args, NULL);

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
 * @brief Allows to configure the python frame received handler
 * identified by the handler variable and its associated data (data).
 * 
 * @param self The channel to be configured with a frame received handler.
 *
 * @param handler The handler to be called on each frame received.
 *
 * @param data User defined data to be passed to the handler.
 *
 * @return axl_true if the frame received handler was properly
 * configured otherwise axl_false is returned.
 */
axl_bool  py_vortex_channel_configure_frame_received  (PyVortexChannel * self, PyObject * handler, PyObject * data)
{
	PyObject * old_handler;
	PyObject * old_data;

	/* check values received */
	if (! PyCallable_Check (handler) || self == NULL) 
		return axl_false;
	
	/* store previous references */
	old_handler = self->frame_received;
	old_data    = self->frame_received_data;

	/* set new references */
	self->frame_received = handler;
	self->frame_received_data = data ? data : Py_None;

	/* increment reference counting */
	Py_XINCREF (self->frame_received);
	Py_INCREF  (self->frame_received_data);

	/* now unref previous references (if defined) */
	if (old_handler || old_data) {
		Py_CLEAR (old_handler);
		Py_CLEAR (old_data);
	}

	/* reconfigure the frame received for used for all channels */
	if (self->channel)
		vortex_channel_set_received_handler (self->channel, py_vortex_channel_received, self);

	return axl_true;
}

/** 
 * @internal Allows to configure async channel creation notification
 * on the provided PyVortexChannel reference.
 */
axl_bool        py_vortex_channel_set_on_channel (PyObject * _channel,
						  PyObject * handler,
						  PyObject * data)
{
	PyVortexChannel * py_channel = (PyVortexChannel *) _channel;
	if (handler == NULL)
		return axl_true;

	py_channel->on_channel = handler;
	Py_INCREF (handler);
	
	py_channel->on_channel_data = data;
	if (data == NULL) 
		py_channel->on_channel_data = Py_None;
	Py_INCREF (py_channel->on_channel_data);

	return axl_true;
}


/** 
 * @internal Support function used to implement async channel create
 * notification.
 */
void            py_vortex_channel_create_notify  (int                channel_num,
						  VortexChannel    * channel,
						  VortexConnection * conn,
						  axlPointer         user_data)
{
	PyVortexChannel    * py_channel = user_data;
	PyGILState_STATE     state;
	PyObject           * args;
	PyObject           * result;

	/* acquire the GIL */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (4);

	/* set parameters */
	PyTuple_SetItem (args, 0, Py_BuildValue ("i", channel_num));
	if (channel == NULL) {
		Py_INCREF (Py_None);
		PyTuple_SetItem (args, 1, Py_None);
	} else {
		/* set the channel into the refernce */
		py_vortex_channel_set (py_channel, channel);
		PyTuple_SetItem (args, 1, __PY_OBJECT (py_channel));
	} /* end if */

	Py_INCREF (py_channel->py_conn);
	PyTuple_SetItem (args, 2, py_channel->py_conn);

	/* now setup user defined on channel data */
	PyTuple_SetItem (args, 3, py_channel->on_channel_data);
	py_channel->on_channel_data = NULL;
	
	/* now invoke */
	result = PyObject_Call (py_channel->on_channel, args, NULL);
	Py_DECREF (py_channel->on_channel);

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* check check to finish py channel ref in case of failure */
	if (channel == NULL) {
		Py_DECREF (py_channel);
	}

	/* release the GIL */
	PyGILState_Release(state);
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

	/* configure frame received */
	if (! py_vortex_channel_configure_frame_received (self, handler, data))
		return NULL;

	/* return none */
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject * py_vortex_channel_send_msg (PyVortexChannel * self, PyObject * args)
{
	const char * content = NULL;
	int          size    = 0;
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "zi", &content, &size))
		return NULL;

	/* call to send the message */
	result = vortex_channel_send_msg (self->channel, content, size, &msg_no);

	/* return none in the case of failure */
	if (! result) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* and return the message number used in the case of proper
	 * send operation */
	return Py_BuildValue ("i", msg_no);
}

PyObject * py_vortex_channel_send_rpy (PyVortexChannel * self, PyObject * args)
{
	const char * content = NULL;
	int          size    = 0;
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "zii", &content, &size, &msg_no))
		return NULL;

	/* remove the following log since it is not secure */
	py_vortex_log (PY_VORTEX_DEBUG, "received request to send rpy, channel: %p (id: %d), content: %s, size: %d, msg_no: %d",
		       self->channel, vortex_channel_get_number (self->channel), content, size, msg_no);

	/* call to send reply */
	result = vortex_channel_send_rpy (self->channel, content, size, msg_no);

	py_vortex_log (PY_VORTEX_DEBUG, "after sending RPY frame, status found was: %d", result);

	/* return none in the case of failure */
	if (! result) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* return reply status (1 if it was ok, otherwise use 0) */
	return Py_BuildValue ("i", result);
}

PyObject * py_vortex_channel_send_err (PyVortexChannel * self, PyObject * args)
{
	const char * content = NULL;
	int          size    = 0;
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "zii", &content, &size, &msg_no))
		return NULL;

	/* call to send reply */
	result = vortex_channel_send_err (self->channel, content, size, msg_no);

	/* return none in the case of failure */
	if (! result) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* return reply status (1 if it was ok, otherwise use 0) */
	return Py_BuildValue ("i", result);
}

PyObject * py_vortex_channel_send_ans (PyVortexChannel * self, PyObject * args)
{
	const char * content = NULL;
	int          size    = 0;
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "zii", &content, &size, &msg_no))
		return NULL;

	/* call to send ans reply */
	result = vortex_channel_send_ans_rpy (self->channel, content, size, msg_no);

	/* return none in the case of failure */
	if (! result) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* return reply status (1 if it was ok, otherwise use 0) */
	return Py_BuildValue ("i", result);
}

PyObject * py_vortex_channel_finalize_ans (PyVortexChannel * self, PyObject * args)
{
	axl_bool     result;
	int          msg_no  = 0;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "i", &msg_no))
		return NULL;

	/* call to send ans reply */
	result = vortex_channel_finalize_ans_rpy (self->channel, msg_no);

	/* return none in the case of failure */
	if (! result) {
		Py_INCREF (Py_None);
		return Py_None;
	} /* end if */

	/* return reply status (1 if it was ok, otherwise use 0) */
	return Py_BuildValue ("i", result);
}

static PyObject * py_vortex_channel_get_reply (PyVortexChannel * self, PyObject * args)
{
	PyVortexAsyncQueue * queue = NULL;
	PyVortexFrame      * py_frame;
	VortexFrame        * frame;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "O", &queue))
		return NULL;

	/* first check we don't have a pending piggy to avoid mixing references */
	frame = vortex_channel_get_piggyback (self->channel);
	if (frame != NULL) {
		/* found piggy back, build a py_frame with this reference */
		return py_vortex_frame_create (frame, axl_false);
	} /* end if */

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* get the next item inside the queue. We are returning the
	 * reference stored directly because py_vortex_queue_reply
	 * already stored the frame received as a PyVortexFrame
	 * object */
	py_frame = PY_VORTEX_FRAME ( vortex_channel_get_reply (self->channel, py_vortex_async_queue_get (queue)) );

	/* restore thread state */
	Py_END_ALLOW_THREADS

	if (py_frame == NULL)  {
		/* failed to get next frame */
		Py_INCREF (Py_None);
		return  Py_None;
	} /* end */

	/* return frame found */
	return __PY_OBJECT (py_frame);
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_channel_get_attr (PyObject *o, PyObject *attr_name) {
	const char      * attr = NULL;
	PyObject        * result;
	PyVortexChannel * self = (PyVortexChannel *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to report channel attr name %s (self: %p)",
		       attr, o);

	if (axl_cmp (attr, "number")) {
		/* found error_msg attribute */
		return Py_BuildValue ("i", vortex_channel_get_number (self->channel));
	} else if (axl_cmp (attr, "profile")) {
		/* found error_msg attribute */
		return Py_BuildValue ("s", vortex_channel_get_profile (self->channel));
	} else if (axl_cmp (attr, "is_ready")) {
		/* found is_ready attribute */
		return Py_BuildValue ("i", vortex_channel_is_ready (self->channel));
	} else if (axl_cmp (attr, "conn")) {

		/* return connection associated to the channel */
		Py_INCREF (__PY_OBJECT (self->py_conn));
		return __PY_OBJECT (self->py_conn);
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

/** 
 * @brief Implements attribute set operation.
 */
int py_vortex_channel_set_attr (PyObject *o, PyObject *attr_name, PyObject *v)
{
	const char      * attr = NULL;
	PyVortexChannel * self = (PyVortexChannel *) o;
	axl_bool          boolean_value = axl_false;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return -1;

	if (axl_cmp (attr, "set_serialize")) {
		/* found set serialize operation, get the boolean value */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* call to set serialize on channel */
		vortex_channel_set_serialize (self->channel, boolean_value);

		/* return operation ok */
		return 0;
	} /* end if */

	/* now implement generic setter */
	return PyObject_GenericSetAttr (o, attr_name, v);
}


static PyMethodDef py_vortex_channel_methods[] = { 
	/* send_msg */
	{"send_msg", (PyCFunction) py_vortex_channel_send_msg, METH_VARARGS,
	 "Allows to send the message with type MSG."},
	/* send_rpy */
	{"send_rpy", (PyCFunction) py_vortex_channel_send_rpy, METH_VARARGS,
	 "Allows to reply to a MSG received with a RPY message."},
	/* send_err */
	{"send_err", (PyCFunction) py_vortex_channel_send_err, METH_VARARGS,
	 "Allows to reply to a MSG received with a ERR message."},
	/* send_ans */
	{"send_ans", (PyCFunction) py_vortex_channel_send_ans, METH_VARARGS,
	 "Allows to reply to a MSG received with a ANS message."},
	/* finalize_ans */
	{"finalize_ans", (PyCFunction) py_vortex_channel_finalize_ans, METH_VARARGS,
	 "Allows to terminate a series of ANS replies with the last NUL frame."},
	/* set_frame_received */
	{"set_frame_received", (PyCFunction) py_vortex_channel_set_frame_received, METH_VARARGS | METH_KEYWORDS,
	 "Allows to configure the frame received handler."},
	/* get_reply */
	{"get_reply", (PyCFunction) py_vortex_channel_get_reply, METH_VARARGS,
	 "Python implementation of vortex_channel_get_reply function. This methods is part of the get reply method where it is required to configure as frame_received handler the function vortex.queue_reply, causing all frames received to be queued. These frames are later retrieved by using this method, that is, channel.get_reply (queue). The advantage of this method is that allows to get all frames received (replies, error, messages..) and it also support returning the first piggyback received on the channel."},
	/* close */
	{"close", (PyCFunction) py_vortex_channel_close, METH_NOARGS,
	 "Allows to close the selected channel and to remove it from the BEEP session."},
	/* incref */
	{"incref", (PyCFunction) py_vortex_channel_incref, METH_NOARGS,
	 "Allows to increment reference counting of the python object (vortex.Channel) holding the channel."},
	/* decref */
	{"decref", (PyCFunction) py_vortex_channel_decref, METH_NOARGS,
	 "Allows to decrement reference counting of the python object (vortex.Channel) holding the channel."},
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
    py_vortex_channel_set_attr, /* tp_setattro*/
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
PyObject * py_vortex_channel_create (VortexChannel * channel, PyObject * py_conn)
{
	/* return a new instance */
	PyVortexChannel * obj;

	/* increase reference counting */
	if ((channel == NULL) || (! vortex_channel_ref (channel))) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to create PyChannel reference, received NULL reference or vortex_channel_ref failed");
		return NULL;
	}

	/* create the channel object */
	obj = (PyVortexChannel *) PyObject_CallObject ((PyObject *) &PyVortexChannelType, NULL);

	/* set channel reference received */
	if (channel)  {
		obj->channel = channel; 
	} /* end if */

	/* acquire a reference to the py_conn if defined */
	obj->py_conn = py_conn;
	Py_XINCREF (obj->py_conn);

	py_vortex_log (PY_VORTEX_DEBUG, "created PyChannel reference with number=%d (connection: %d, self: %p, channel: %p, refs: %d)",
		       vortex_channel_get_number (channel), vortex_connection_get_id (py_vortex_connection_get (py_conn)), 
		       obj, obj->channel, vortex_channel_ref_count (channel));

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Returns an empty PyVortexChannel definition.
 *
 * @param py_conn Reference to the python connection.
 *
 * @return A reference to a newly created PyVortexChannel.
 */
PyObject      * py_vortex_channel_create_empty (PyObject * py_conn)
{
	PyVortexChannel * py_channel = (PyVortexChannel *) PyObject_CallObject ((PyObject *) &PyVortexChannelType, NULL);

	/* acquire a reference to the py_conn if defined */
	py_channel->py_conn = py_conn;
	Py_XINCREF (py_channel->py_conn);
	
	return __PY_OBJECT (py_channel);
}

/** 
 * @brief Allows to configure the channel reference into the python
 * channel.
 */
void            py_vortex_channel_set    (PyVortexChannel * py_channel, 
					  VortexChannel   * channel)
{
	/* do nothing if null reference is received */
	if (py_channel == NULL)
		return;

	/* increase reference counting */
	if (! vortex_channel_ref (channel))
		return;

	/* set the channel (even if it is null) */
	py_channel->channel = channel;

	return;
}

/** 
 * @brief Allows to get the VortexChannel reference that is stored
 * inside the python channel reference received.
 *
 * @param channel The python wrapper that contains the channel to be returned.
 *
 * @return A reference to the channel inside the python channel.
 */
VortexChannel * py_vortex_channel_get    (PyObject * channel)
{
	PyVortexChannel * _channel = (PyVortexChannel *) channel;

	/* check null values */
	if (_channel == NULL)
		return NULL;

	/* return the channel reference inside */
	return _channel->channel;
}

/** 
 * @brief Allows to check if the reference received points to a
 * channel.
 */ 
axl_bool        py_vortex_channel_check  (PyObject        * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexChannelType);
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



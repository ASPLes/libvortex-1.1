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

#include <py_vortex_async_queue.h>

struct _PyVortexAsyncQueue {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the vortex context */
	VortexAsyncQueue * async_queue;
};

/** 
 * @brief Allows to get the VortexAsyncQueue type definition found inside the
 * PyVortexAsyncQueue encapsulation.
 *
 * @param py_vortex_async_queue The PyVortexAsyncQueue that holds a reference to the
 * inner VortexAsyncQueue.
 *
 * @return A reference to the inner VortexAsyncQueue.
 */
VortexAsyncQueue * py_vortex_async_queue_get (PyVortexAsyncQueue * py_vortex_async_queue)
{
	if (py_vortex_async_queue == NULL)
		return NULL;
	
	/* return current context created */
	return py_vortex_async_queue->async_queue;
}

static int py_vortex_async_queue_init_type (PyVortexAsyncQueue *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.async_queue
 */
static PyObject * py_vortex_async_queue_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexAsyncQueue *self;

	/* create the object */
	self = (PyVortexAsyncQueue *)type->tp_alloc(type, 0);

	/* create the context */
	self->async_queue = vortex_async_queue_new ();

	py_vortex_log (PY_VORTEX_DEBUG, "created new AsyncQueue: %p, self->queue: %p", self, self->async_queue);

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.async_queue
 */
static void py_vortex_async_queue_dealloc (PyVortexAsyncQueue* self)
{
	/* do a log */
	py_vortex_log (PY_VORTEX_DEBUG, "finishing PyVortxAsyncQueue reference: %p, self->queue: %p", 
		       self, self->async_queue);

	/* free async_queue */
	vortex_async_queue_safe_unref (&(self->async_queue)); 

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_async_queue_push function. 
 */
static PyObject * py_vortex_async_queue_push (PyVortexAsyncQueue* self, PyObject * args)
{
	PyObject * obj;

	/* now implement other attributes */
	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	/* incremenet reference count */
	Py_INCREF (obj); 

	py_vortex_log (PY_VORTEX_DEBUG, "pushing object %p into queue: %p, self->queue: %p",
		       obj, self, self->async_queue);

	/* push the item */
	vortex_async_queue_push (self->async_queue, obj);

	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief Direct wrapper for the vortex_async_queue_pop function.
 */
static PyObject * py_vortex_async_queue_pop (PyVortexAsyncQueue* self)
{
	PyObject           * _result;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	py_vortex_log (PY_VORTEX_DEBUG, "allowed other threads to enter python space, poping next item (queue.pop ())");

	/* get the value */
	_result = vortex_async_queue_pop (self->async_queue);

        py_vortex_log (PY_VORTEX_DEBUG, "Finished wait on queue.pop (), reference returned: %p (queue: %p, self->queue: %p)", 
		       _result, self, self->async_queue);

	/* restore thread state */
	Py_END_ALLOW_THREADS

	/* do not decrement reference counting. It was increased to
	 * provide a reference owned by the caller */
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_init_async_queue function. This method
 * receives a vortex.async_queue object and initializes it calling to
 * vortex_init_async_queue.
 */
static PyObject * py_vortex_async_queue_timedpop (PyVortexAsyncQueue* self, PyObject * args)
{
	PyObject * _result;
	int        microseconds = 0;

	/* now implement other attributes */
	if (! PyArg_Parse (args, "i", &microseconds))
		return NULL;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* get the value */
	_result = vortex_async_queue_timedpop (self->async_queue, microseconds);

	/* restore thread state */
	Py_END_ALLOW_THREADS

	/* do not decrement reference counting. It was increased to
	 * provide a reference owned by the caller */
	return _result;
}

/** 
 * @brief Direct mapping for unref operation vortex_async_queue_unref
 */
/* static PyObject * py_vortex_async_queue_unref (PyVortexAsyncQueue* self)
{
	py_vortex_log (PY_VORTEX_DEBUG, "calling to queue.unref (): %p, self->queue: %p",
		       self, self->async_queue); 
		       */
	/* decrease reference counting */
	/*	vortex_async_queue_safe_unref (&(self->async_queue)); */

	/* and decrement references to the pyobject */
	/*	Py_CLEAR (self); */

	/* return None */
	/*	Py_INCREF (Py_None);
	return Py_None;
	}*/

/** 
 * @brief Direct mapping for the ref operation vortex_async_queue_ref.
 */
PyObject * py_vortex_async_queue_ref (PyVortexAsyncQueue* self)
{
	/* increase reference counting */
	vortex_async_queue_ref (self->async_queue);

	/* and decrement references to the pyobject */
	Py_INCREF (self);

	/* return None */
	Py_INCREF (Py_None);
	return Py_None;
}


static PyMethodDef py_vortex_async_queue_methods[] = { 
	{"push", (PyCFunction) py_vortex_async_queue_push, METH_VARARGS,
	 "Allows to push a new item into the queue. Use pop() and timedpop() to retreive items stored."},
	{"pop", (PyCFunction) py_vortex_async_queue_pop, METH_NOARGS,
	 "Allows to get the next item available on the queue (vortex.AsyncQueue). This method will block the caller. Check items attribute to know if there are pending elements."},
	{"timedpop", (PyCFunction) py_vortex_async_queue_timedpop, METH_VARARGS,
	 "Allows to get the next item available on the queue (vortex.AsyncQueue) limiting the wait period to the value (microseconds) provided. This method will block the caller. Check items attribute to know if there are pending elements."},
/*	{"ref", (PyCFunction) py_vortex_async_queue_ref, METH_NOARGS,
	"Allows to increase reference counting on the provided queue reference. "}, */
/*	{"unref", (PyCFunction) py_vortex_async_queue_unref, METH_NOARGS,
	"Allows to decrease reference counting on the provided queue reference. If reached 0 the queue is finished."}, */
 	{NULL}  
}; 

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_async_queue_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyVortexAsyncQueue * self = (PyVortexAsyncQueue *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "length")) {
		/* found length attribute */
		return Py_BuildValue ("i", vortex_async_queue_length (self->async_queue));
	} else if (axl_cmp (attr, "waiters")) {
		/* found waiters attribute */
		return Py_BuildValue ("i", vortex_async_queue_waiters (self->async_queue));
	} else if (axl_cmp (attr, "items")) {
		/* found items attribute */
		return Py_BuildValue ("i", vortex_async_queue_items (self->async_queue));
	} /* end if */

	/* printf ("Attribute not found: '%s'..\n", attr); */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static PyTypeObject PyVortexAsyncQueueType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.AsyncQueue",              /* tp_name*/
    sizeof(PyVortexAsyncQueue),       /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_async_queue_dealloc, /* tp_dealloc*/
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
    py_vortex_async_queue_get_attr,  /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "Vortex context object required to function with Vortex API",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_async_queue_methods,     /* tp_methods */
    0, /* py_vortex_async_queue_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_async_queue_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_async_queue_new,         /* tp_new */

};

/** 
 * @brief Inits the vortex async_queue module. It is implemented as a type.
 */
void init_vortex_async_queue (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexAsyncQueueType) < 0)
		return;
	
	Py_INCREF (&PyVortexAsyncQueueType);
	PyModule_AddObject(module, "AsyncQueue", (PyObject *)&PyVortexAsyncQueueType);

	return;
}






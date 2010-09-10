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

#include <py_vortex_frame.h>

struct _PyVortexFrame {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the VortexFrame object */
	VortexFrame * frame;
	VortexCtx   * ctx;
};

static int py_vortex_frame_init_type (PyVortexFrame *self, PyObject *args, PyObject *kwds)
{
    return 0;
}


/** 
 * @brief Function used to allocate memory required by the object vortex.Frame
 */
static PyObject * py_vortex_frame_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexFrame    * self;

	/* create the object */
	self = (PyVortexFrame *)type->tp_alloc(type, 0);

	/* and do nothing more */
	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.Frame
 */
static void py_vortex_frame_dealloc (PyVortexFrame* self)
{
	/* unref the frame */
	vortex_frame_unref (self->frame);
	self->frame = NULL;
	vortex_ctx_unref   (&(self->ctx));

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}


/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_frame_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyVortexFrame * self = (PyVortexFrame *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "id")) {
		/* get id attribute */
		return Py_BuildValue ("i", vortex_frame_get_id (self->frame));
	} else if (axl_cmp (attr, "type")) {
		/* return string type */
		switch (vortex_frame_get_type (self->frame)) {
		case VORTEX_FRAME_TYPE_MSG:
			return Py_BuildValue ("s", "MSG");
		case VORTEX_FRAME_TYPE_RPY:
			return Py_BuildValue ("s", "RPY");
		case VORTEX_FRAME_TYPE_ERR:
			return Py_BuildValue ("s", "ERR");
		case VORTEX_FRAME_TYPE_ANS:
			return Py_BuildValue ("s", "ANS");
		case VORTEX_FRAME_TYPE_NUL:
			return Py_BuildValue ("s", "NUL");
		case VORTEX_FRAME_TYPE_SEQ:
			return Py_BuildValue ("s", "SEQ");
		default:
			/* unsupported frame */
			break;
		}

		/* return None */
		return Py_BuildValue ("");
	} else if (axl_cmp (attr, "msgno") || axl_cmp (attr, "msg_no")) {
		/* get msgno attribute */
		return Py_BuildValue ("i", vortex_frame_get_msgno (self->frame));
	} else if (axl_cmp (attr, "seqno") || axl_cmp (attr, "seq_no")) {
		/* get seqno attribute */
		return Py_BuildValue ("i", vortex_frame_get_seqno (self->frame));
	} else if (axl_cmp (attr, "ansno") || axl_cmp (attr, "ans_no")) {
		/* get ansno attribute */
		return Py_BuildValue ("i", vortex_frame_get_ansno (self->frame));
	} else if (axl_cmp (attr, "more_flag")) {
		/* get more_flag attribute */
		return Py_BuildValue ("i", vortex_frame_get_more_flag (self->frame));
	} else if (axl_cmp (attr, "payload_size")) {
		/* get payload_size attribute */
		return Py_BuildValue ("i", vortex_frame_get_payload_size (self->frame));
	} else if (axl_cmp (attr, "content_size")) {
		/* get content_size attribute */
		return Py_BuildValue ("i", vortex_frame_get_content_size (self->frame));
	} else if (axl_cmp (attr, "payload")) {
		/* get payload attribute */
		return Py_BuildValue ("z#", vortex_frame_get_payload (self->frame), vortex_frame_get_payload_size (self->frame));
	} else if (axl_cmp (attr, "content")) {
		/* get payload attribute */
		return Py_BuildValue ("z#", vortex_frame_get_content (self->frame), vortex_frame_get_content_size (self->frame));
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

/* no methods */
/* static PyMethodDef py_vortex_frame_methods[] = { 
 	{NULL}  
}; */


static PyTypeObject PyVortexFrameType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Frame",       /* tp_name*/
    sizeof(PyVortexFrame),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_frame_dealloc, /* tp_dealloc*/
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
    py_vortex_frame_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.Frame, the object used to represent a BEEP frame.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0, /*py_vortex_frame_methods,*/     /* tp_methods */
    0, /* py_vortex_frame_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_frame_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_frame_new,  /* tp_new */
};

/** 
 * @brief Creates new empty frame instance. The function acquire a new
 * reference to the frame (vortex_frame_unref) because it us assumed
 * the caller is inside a frame received handler or a similar handler
 * where it is assumed the frame will be no longer available after
 * handler has finished.
 *
 * In the case you are wrapping a frame and you already own a
 * reference to them, you can set acquire_ref to axl_false.
 *
 * @param frame The frame to wrap creating a PyVortexFrame reference.
 *
 * @param acquire_ref Signal the function to acquire a reference to
 * the vortex_frame_ref making the PyVortexFrame returned to "own" a
 * reference to the frame. In the case acquire_ref is axl_false, the
 * caller is telling the function to "steal" a reference from the
 * frame.
 *
 * @return A newly created PyVortexFrame reference, casted to PyObject.
 */
PyObject * py_vortex_frame_create (VortexFrame * frame, axl_bool acquire_ref)
{
	/* return a new instance */
	PyVortexFrame * obj = (PyVortexFrame *) PyObject_CallObject ((PyObject *) &PyVortexFrameType, NULL);

	/* set frame reference received */
	if (obj && frame) {
		/* increase reference counting */
		vortex_frame_ref (frame);

		/* acquire a reference to the ctx */
		obj->ctx = vortex_frame_get_ctx (frame);
		vortex_ctx_ref (obj->ctx);

		/* acquire reference */
		obj->frame = frame;
	} /* end if */

	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Inits the vortex frame module. It is implemented as a type.
 */
void init_vortex_frame (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexFrameType) < 0)
		return;
	
	Py_INCREF (&PyVortexFrameType);
	PyModule_AddObject(module, "Frame", (PyObject *)&PyVortexFrameType);

	return;
}

/** 
 * @brief Allows to get the VortexFrame reference that is stored
 * inside the python frame reference received.
 *
 * @param frame The python wrapper that contains the frame to be returned.
 *
 * @return A reference to the frame inside the python frame.
 */
VortexFrame * py_vortex_frame_get    (PyVortexFrame * frame)
{
	/* check null values */
	if (frame == NULL)
		return NULL;

	/* return the frame reference inside */
	return frame->frame;
}



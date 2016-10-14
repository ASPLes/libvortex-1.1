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

#include <py_vortex_ctx.h>

struct _PyVortexCtx {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the vortex context */
	VortexCtx * ctx;

	/* flags if the PyVortexCtx is pending to exit */
	axl_bool    exit_pending;
};

axlHash      * py_ctx_refs = NULL;
VortexMutex    py_ctx_refs_mutex;

/** 
 * @brief Allows to get the VortexCtx type definition found inside the
 * PyVortexCtx encapsulation.
 *
 * @param py_vortex_ctx The PyVortexCtx that holds a reference to the
 * inner VortexCtx.
 *
 * @return A reference to the inner VortexCtx.
 */
VortexCtx * py_vortex_ctx_get (PyObject * py_vortex_ctx)
{
	if (py_vortex_ctx == NULL)
		return NULL;
	
	/* return current context created */
	return ((PyVortexCtx *)py_vortex_ctx)->ctx;
}

/* open handler (last handler that is found running) */
#define PY_VORTEX_WATCHER_HANDLER_HASH  "py:vo:wa:ha"
#define PY_VORTEX_WATCHER_NOTIFIER      "py:vo:wa:no"
#define PY_VORTEX_WATCHER_NOTIFIER_DATA "py:vo:wa:nod"

typedef struct _PyVortexHandlerWatcher {
	char    * handler_string;
	int       stamp;
} PyVortexHandlerWatcher;

void py_vortex_ctx_release_handler_watcher (axlPointer ptr)
{
	PyVortexHandlerWatcher * watcher = ptr;

	axl_free (watcher->handler_string);
	axl_free (watcher);
	
	return;
}

/** 
 * @internal Allows to track a handler started
 */
void        py_vortex_ctx_record_start_handler (VortexCtx * ctx, PyObject * handler)
{
	PyObject                * obj;
	struct timeval            stamp;
	char                    * buffer;
	Py_ssize_t                buffer_size;
	PyVortexHandlerWatcher  * watcher;
	VortexHash              * hash = vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_HANDLER_HASH);

	/* no hash no record */
	if (hash == NULL)
		return;

	/* record handler position */
	watcher = axl_new (PyVortexHandlerWatcher, 1);
	if (watcher == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to acquire memory to record handler start, skipping");
		return;
	} /* end if */

	/* build an string representation from the handler */
	obj = PyObject_Str (handler);
	if (obj == NULL) {
		axl_free (watcher);
		return;
	}
	if (PyString_AsStringAndSize (obj, &buffer, &buffer_size) == -1) {
		Py_DECREF (obj);
		axl_free (watcher);
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to record handler started because it wasn't possible to build an string representation");
		return;
	} /* end if */
	Py_DECREF (obj);

	/* record handler started */
	watcher->handler_string = axl_strdup (buffer);

	gettimeofday (&stamp, NULL);
	watcher->stamp = (int) stamp.tv_sec;
	
	/* record value */
	/* py_vortex_log (PY_VORTEX_DEBUG, "Started handler %s and stamp %d", watcher->handler_string, (int) watcher->stamp);  */

	/* record */
	vortex_hash_replace_full (hash, handler, NULL, watcher, py_vortex_ctx_release_handler_watcher);

	return;
}

void        py_vortex_ctx_record_close_handler (VortexCtx * ctx, PyObject * handler)
{
	VortexHash * hash;

	/* allow other threads to enter into the python space */
	Py_BEGIN_ALLOW_THREADS

	/* no hash no record */
	hash = vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_HANDLER_HASH);

	if (hash) {
		/* delete handler */
		vortex_hash_remove (hash, handler);
	} /* end if */

	/* restore thread state */
	Py_END_ALLOW_THREADS

	return;
}

axl_bool py_vortex_ctx_handler_watcher_foreach (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2, axlPointer _ctx)
{
	PyVortexHandlerWatcher * watcher         = data;
	int                      watching_period = PTR_TO_INT (user_data2);
	struct  timeval        * stamp           = user_data;
	PyVortexTooLongNotifier  notifier;
	axlPointer               notifier_data;
	char                   * report_msg;
	VortexCtx              * ctx = _ctx;

	if ((stamp->tv_sec - watcher->stamp) > watching_period) {
		py_vortex_log (PY_VORTEX_WARNING, "handler '%s' is taking too long to finish, it being running %d seconds..",
			       watcher->handler_string, (stamp->tv_sec - watcher->stamp));

		/* get references to notifiers */
		notifier      = vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_NOTIFIER);
		notifier_data = vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_NOTIFIER_DATA);

		if (notifier) {
			/* build message */
			report_msg = axl_strdup_printf ("handler '%s' is taking too long to finish, it being running %d seconds..",
							watcher->handler_string, (stamp->tv_sec - watcher->stamp));
			/* notify caller */
			notifier (report_msg, notifier_data);
			/* release */
			axl_free (report_msg);
		}
	} /* end if */

	return axl_false; /* do not stop */
}

axl_bool py_vortex_ctx_handler_watcher (VortexCtx *ctx, axlPointer user_data, axlPointer user_data2)
{
	int              watching_period  = PTR_TO_INT (user_data);
	VortexHash     * hash             = vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_HANDLER_HASH);
	struct timeval   current;
	
	/* py_vortex_log (PY_VORTEX_WARNING, "checking for long running handlers, currently registered: %d..",
	   vortex_hash_size (hash)); */
	if (vortex_hash_size (hash) == 0)
		return axl_false; /* do not remove handler */

	/* get current stamp */
	gettimeofday (&current, NULL);

	/* foreach all registered handlers */
	vortex_hash_foreach3 (hash, py_vortex_ctx_handler_watcher_foreach, &current, INT_TO_PTR (watching_period), ctx);

	return axl_false; /* do not remove handler */
}

/** 
 * @internal Start a handler watcher to report user when a handler is
 * taking too long. In general, having watching_period equal to 3
 * seconds is a good value. This is because you handler shouldn't take
 * longer than that and, if that might happen, look for a solution:
 * start a task that regularly check the result in the future (python
 * subprocess has a nice poll() method to asynchounously check that).
 *
 * @param watching_period When to check if there are opened handler in seconds.
 */
void        py_vortex_ctx_start_handler_watcher (VortexCtx * ctx, int watching_period,
						 PyVortexTooLongNotifier notifier, axlPointer notifier_data)
{

	VortexHash * hash;

	/* check handler to be previously defined */
	if (vortex_ctx_get_data (ctx, PY_VORTEX_WATCHER_HANDLER_HASH)) 
		return;

	/* init hash */
	hash = vortex_hash_new (axl_hash_int, axl_hash_equal_int);
	
	/* set data full */
	vortex_ctx_set_data_full (ctx, PY_VORTEX_WATCHER_HANDLER_HASH, hash, NULL, (axlDestroyFunc) vortex_hash_unref);

	/* record notifier if defined */
	vortex_ctx_set_data (ctx, PY_VORTEX_WATCHER_NOTIFIER, notifier);
	vortex_ctx_set_data (ctx, PY_VORTEX_WATCHER_NOTIFIER_DATA, notifier_data);
		
	/* start handler */
	vortex_thread_pool_new_event (ctx, (watching_period + 1) * 1000000, 
				      py_vortex_ctx_handler_watcher, INT_TO_PTR (watching_period), NULL);
	return;
}

void py_vortex_ctx_too_long_notifier_to_file (const char * msg_string, axlPointer data)
{
	const char * file = data;
	int          fd;
	time_t       rawtime;
	const char * value;

	py_vortex_log (PY_VORTEX_DEBUG, "Logging into file: [%s], message [%s]", file, msg_string);
	
	/* open the file provided by the user */
	fd = open (file, O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (fd < 0) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to open log located at %s to log %s", file, msg_string);
		return;
	} /* end if */

	time ( &rawtime );
	value = ctime (&rawtime);
	if (write (fd, value, strlen (value) - 1) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	if (write (fd, "  ", 2) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	if (write (fd, msg_string, strlen (msg_string)) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	if (write (fd, "\n", 1) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	close (fd);

	return;
}

/** 
 * @internal Installs a too long notification handler that logs into
 * the provided file, all notifications found for handlers that are
 * taking more than the provided watching period measured in seconds.
 */
axl_bool        py_vortex_ctx_log_too_long_notifications (VortexCtx * ctx, int watching_period,
							  const char * file)
{
	int          fd;
	time_t       rawtime;
	const char * value;

	/*** GIL ALREADY ACQUIRED REACHED THIS POINT ***/

	/* open the file provided by the user */
	fd = open (file, O_CREAT | O_APPEND | O_WRONLY, 0600);
	if (fd < 0) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to open log located at %s, unable to enable long notifications", file);
		return axl_false;
	} /* end if */

	time ( &rawtime );
	value = ctime (&rawtime);
	if (write (fd, value, strlen (value) - 1) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	if (write (fd, "  ", 2) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	if (write (fd, "Too long notification watcher started\n", 38) < 0)
		py_vortex_log (PY_VORTEX_WARNING, "Unable to write info into log, errno: %s", vortex_errno_get_error (errno));
	close (fd);

	/* record file into ctx */
	file = strdup (file);
	vortex_ctx_set_data_full (ctx, file, (axlPointer) file, NULL, axl_free);

	/* start handler watcher here */
	py_vortex_ctx_start_handler_watcher (ctx, watching_period, py_vortex_ctx_too_long_notifier_to_file, (axlPointer) file);

	return axl_true;
}

static int py_vortex_ctx_init_type (PyVortexCtx *self, PyObject *args, PyObject *kwds)
{
    return 0;
}



/** 
 * @brief Function used to allocate memory required by the object vortex.ctx
 */
static PyObject * py_vortex_ctx_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexCtx *self;

	/* create the object */
	self = (PyVortexCtx *)type->tp_alloc(type, 0);

	/* create the context */
	self->ctx = vortex_ctx_new ();
	py_vortex_log (PY_VORTEX_DEBUG, "created PyVortexCtx referece %p (VortexCtx %p)", self, self->ctx);

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.ctx
 */
static void py_vortex_ctx_dealloc (PyVortexCtx* self)
{
	py_vortex_log (PY_VORTEX_DEBUG, "collecting vortex.Ctx ref: %p (self->ctx: %p, count: %d)", self, self->ctx, vortex_ctx_ref_count (self->ctx));

	/* check for pending exit */
	if (self->exit_pending) {
		py_vortex_log (PY_VORTEX_DEBUG, "found vortex.Ctx () exiting pending flag enabled, finishing context..");
		Py_DECREF ( py_vortex_ctx_exit (self) );
	} /* end if */

	/* free ctx */
	vortex_ctx_free (self->ctx);
	self->ctx = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_init_ctx function. This method
 * receives a vortex.ctx object and initializes it calling to
 * vortex_init_ctx.
 */
static PyObject * py_vortex_ctx_init (PyVortexCtx* self)
{
	PyObject *_result;

	/* check to not reinitialize */
	if (self->exit_pending) {
		py_vortex_log (PY_VORTEX_WARNING, "called to initialize a context twice");
		return Py_BuildValue ("i", axl_true);
	} /* end if */

	/* call to init context and build result value */
	self->exit_pending = vortex_init_ctx (self->ctx);
	_result = Py_BuildValue ("i", self->exit_pending);

	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_init_ctx function. This method
 * receives a vortex.ctx object and initializes it calling to
 * vortex_init_ctx.
 */
PyObject * py_vortex_ctx_exit (PyVortexCtx* self)
{
	if (self->exit_pending) {
		py_vortex_log (PY_VORTEX_DEBUG, "finishing vortex.Ctx %p (self->ctx: %p)", self, self->ctx);

		/* allow threads */
		Py_BEGIN_ALLOW_THREADS
		
		/* call to finish context: do not dealloc ->ctx, this is
		   already done by the type deallocator */
		vortex_exit_ctx (self->ctx, axl_false);

		/* end threads */
		Py_END_ALLOW_THREADS

		/* flag as exit done */
		self->exit_pending = axl_false;
	}

	/* return None */
	Py_INCREF (Py_None);
	return Py_None;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_ctx_get_attr (PyObject *o, PyObject *attr_name) {
	const char      * attr = NULL;
	PyObject        * result;
	PyVortexCtx     * self = (PyVortexCtx *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to report channel attr name %s (self: %p)",
		       attr, o);

	if (axl_cmp (attr, "log")) {
		/* log attribute */
		return Py_BuildValue ("i", vortex_log_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "log2")) {
		/* log2 attribute */
		return Py_BuildValue ("i", vortex_log2_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "color_log")) {
		/* color_log attribute */
		return Py_BuildValue ("i", vortex_color_log_is_enabled (self->ctx));
	} else if (axl_cmp (attr, "ref_count")) {
		/* ref counting */
		return Py_BuildValue ("i", vortex_ctx_ref_count (self->ctx));
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
int py_vortex_ctx_set_attr (PyObject *o, PyObject *attr_name, PyObject *v)
{
	const char      * attr = NULL;
	PyVortexCtx     * self = (PyVortexCtx *) o;
	axl_bool          boolean_value = axl_false;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return -1;

	if (axl_cmp (attr, "log")) {
		/* configure log */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		vortex_log_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;
	} else if (axl_cmp (attr, "log2")) {
		/* configure log2 */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		vortex_log2_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;

	} else if (axl_cmp (attr, "color_log")) {
		/* configure color_log */
		if (! PyArg_Parse (v, "i", &boolean_value))
			return -1;
		
		/* configure log */
		vortex_color_log_enable (self->ctx, boolean_value);

		/* return operation ok */
		return 0;

	} /* end if */

	/* now implement generic setter */
	return PyObject_GenericSetAttr (o, attr_name, v);
}

typedef struct _PyVortexEventData {
	int        id;
	PyObject * user_data;
	PyObject * user_data2;
	PyObject * handler;
	PyObject * ctx;
} PyVortexEventData;

void py_vortex_ctx_event_free (PyVortexEventData * data)
{
	Py_DECREF (data->user_data);
	Py_DECREF (data->user_data2);
	Py_DECREF (data->handler);
	Py_DECREF (data->ctx);
	axl_free (data);
	return;
}

axl_bool py_vortex_ctx_bridge_event (VortexCtx * ctx, axlPointer user_data, axlPointer user_data2)
{
	/* reference to the python channel */
	PyObject           * args;
	PyGILState_STATE     state;
	PyObject           * result;
	axl_bool             _result = axl_false;
	PyVortexEventData  * data       = user_data;
	char               * str;

	/* check if vortex engine is existing */
	if (vortex_is_exiting (ctx)) {
		py_vortex_log (PY_VORTEX_DEBUG, "disabled bridged event into python code, vortex exiting=%d..",
			       vortex_is_exiting (ctx));
		return axl_true;
	}

	/* check to skip events */
	if (PTR_TO_INT (vortex_ctx_get_data (ctx, "py:vo:ctx:de"))) {
		py_vortex_log (PY_VORTEX_DEBUG, "disabled bridged event into python code, vortex exiting=%d due to key..",
			       vortex_is_exiting (ctx));
		return axl_true;
	}

	/* acquire the GIL */
	/* py_vortex_log2 (PY_VORTEX_DEBUG, "bridging event id=%d into python code (getting GIL) vortex exiting=%d..", 
	   data->id, vortex_is_exiting (ctx));   */
	state = PyGILState_Ensure();

	/* create a tuple to contain arguments */
	args = PyTuple_New (3);

	/* increase reference counting */
	Py_INCREF (data->ctx);
	PyTuple_SetItem (args, 0, data->ctx);

	Py_INCREF (data->user_data);
	PyTuple_SetItem (args, 1, data->user_data);

	Py_INCREF (data->user_data2);
	PyTuple_SetItem (args, 2, data->user_data2);

	/* record handler */
	START_HANDLER (data->handler);

	/* now invoke */
	result = PyObject_Call (data->handler, args, NULL);

	/* unrecord handler */
	CLOSE_HANDLER (data->handler);

	/* py_vortex_log2 (PY_VORTEX_DEBUG, "event notification finished, checking for exceptions and result.."); */
	if (py_vortex_handle_and_clear_exception (NULL)) {
		/* exception found */
		py_vortex_log (PY_VORTEX_CRITICAL, "removing bridged event %d because an exception was found during its handling",
			       data->id);
		goto remove_event;
	}

	/* now get result value */
	_result = axl_false;
	if (! PyArg_Parse (result, "i", &_result)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to parse result from python event handler, requesting to remove handler");
	} /* end if */

	/* release tuple and result returned (which may be null) */
	Py_DECREF (args);
	Py_XDECREF (result);

	/* in the case the python code signaled to finish the event,
	 * terminate content inside ctx */
	if (_result || vortex_is_exiting (ctx)) {

		py_vortex_log (PY_VORTEX_DEBUG, "removing bridged event %d because vortex exiting=%d or event_result=%d",
			       data->id, vortex_is_exiting (ctx), _result);

	remove_event:

		/* allow threads */
		Py_BEGIN_ALLOW_THREADS

		/* call to remove event before returning */
		vortex_thread_pool_remove_event (ctx, data->id);

		/* end threads: this declaration has to be here so
		   next vortex_ctx_set_data runs with GIL acquired */
		Py_END_ALLOW_THREADS

		/* we have to remove the event, finish all data */
		str = axl_strdup_printf ("py:vo:event:%d", data->id);
		vortex_ctx_set_data (ctx, str, NULL);
		axl_free (str);

	} /* end if */

	/* release the GIL */
	PyGILState_Release(state);

	/* return value from python handler */
	return _result;
}

/** 
 * @brief Allows to register a new callable event.
 */
static PyObject * py_vortex_ctx_new_event (PyObject * self, PyObject * args, PyObject * kwds)
{
	PyObject           * handler      = NULL;
	PyObject           * user_data    = NULL;
	PyObject           * user_data2   = NULL;
	long                 microseconds = 0;         
	PyVortexEventData  * data;

	/* now parse arguments */
	static char *kwlist[] = {"microseconds", "handler", "user_data", "user_data2", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "lO|OO", kwlist, 
					   &microseconds,
					   &handler, 
					   &user_data,
					   &user_data2))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "received request to register new event");

	/* check handlers defined */
	if (handler != NULL && ! PyCallable_Check (handler)) {
		py_vortex_log (PY_VORTEX_CRITICAL, "called to define a new event but provided a handler which is not callable");
		return NULL;
	} /* end if */

	if (microseconds <= 0) {
		py_vortex_log (PY_VORTEX_CRITICAL, "called to define a new event but provided a microseconds value which is less or equal to 0");
		return NULL;
	} /* end if */

	/* create data to hold all objects */
	data = axl_new (PyVortexEventData, 1);
	if (data == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "failed to allocate memory for vortex event..");
		return NULL;
	} /* end if */

	/* set references */
	PY_VORTEX_SET_REF (data->user_data,  user_data);
	PY_VORTEX_SET_REF (data->user_data2, user_data2);
	PY_VORTEX_SET_REF (data->handler,    handler);
	PY_VORTEX_SET_REF (data->ctx,        self);

	/* register the handler and get the id */
	data->id = vortex_thread_pool_new_event (py_vortex_ctx_get (self),
						 microseconds,
						 py_vortex_ctx_bridge_event,
						 data, INT_TO_PTR(data->id));
	vortex_ctx_set_data_full (py_vortex_ctx_get (self), axl_strdup_printf ("py:vo:event:%d", data->id), data, axl_free, (axlDestroyFunc) py_vortex_ctx_event_free);

	/* reply work done */
	py_vortex_log (PY_VORTEX_DEBUG, "event registered with id %d", data->id);
	return Py_BuildValue ("i", data->id);
}

/** 
 * @brief Allows to register a new callable event.
 */
static PyObject * py_vortex_ctx_remove_event (PyObject * self, PyObject * args, PyObject * kwds)
{
	int handle_id;

	/* parse and check result */
	if (! PyArg_ParseTuple (args, "i", &handle_id))
		return NULL;	

	/* call to remove handle */
	return Py_BuildValue ("i", vortex_thread_pool_remove_event (py_vortex_ctx_get (self), handle_id));
}

/** 
 * @brief Too long notification to file
 */
static PyObject * py_vortex_ctx_enable_too_long_notify_to_file (PyObject * _self, PyObject * args, PyObject * kwds)
{
	int            watching_period = 0;
	const char   * file;
	PyVortexCtx  * self = (PyVortexCtx *) _self;
	/* now parse arguments */
	static char *kwlist[] = {"watching_period", "file", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords (args, kwds, "is", kwlist, 
					   &watching_period, &file))
		return NULL;

	py_vortex_log (PY_VORTEX_DEBUG, "Enabling sending too long notifications to file %s with watching period %d", file, watching_period);

	/* call to enable the handle and return the result */
	return Py_BuildValue ("i", py_vortex_ctx_log_too_long_notifications (self->ctx, watching_period, file));
}

static PyMethodDef py_vortex_ctx_methods[] = { 
	/* init */
	{"init", (PyCFunction) py_vortex_ctx_init, METH_NOARGS,
	 "Inits the Vortex context starting all vortex functions associated. This API call is required before using the rest of the Vortex API."},
	/* exit */
	{"exit", (PyCFunction) py_vortex_ctx_exit, METH_NOARGS,
	 "Finish the Vortex context. This call must be the last one Vortex API usage (for this context)."},
	/* new_event */
	{"new_event", (PyCFunction) py_vortex_ctx_new_event, METH_VARARGS | METH_KEYWORDS,
	 "Function that allows to configure an asynchronous event calling to the handler defined, between the intervals defined. This function is the interface to vortex_thread_pool_event_new."},
	/* remove_event */
	{"remove_event", (PyCFunction) py_vortex_ctx_remove_event, METH_VARARGS | METH_KEYWORDS,
	 "Allows to remove an event installed with new_event() method using the handle id returned from that function. This function is the interface to vortex_thread_pool_event_remove."},
	/* enable_too_long_notify_to_file */
	{"enable_too_long_notify_to_file", (PyCFunction) py_vortex_ctx_enable_too_long_notify_to_file, METH_VARARGS | METH_KEYWORDS,
	 "Allows to activate a too long notification handler that will long into the provided file a notification every time is found a handler that is taking too long to finish."},
 	{NULL}  
}; 

static PyTypeObject PyVortexCtxType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Ctx",              /* tp_name*/
    sizeof(PyVortexCtx),       /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_ctx_dealloc, /* tp_dealloc*/
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
    py_vortex_ctx_get_attr,    /* tp_getattro*/
    py_vortex_ctx_set_attr,    /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "Vortex context object required to function with Vortex API",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_ctx_methods,     /* tp_methods */
    0, /* py_vortex_ctx_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_ctx_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_ctx_new,         /* tp_new */

};

/** 
 * @brief Allows to create a PyVortexCtx wrapper for the VortexCtx
 * reference received.
 *
 * @param ctx The VortexCtx reference to wrap.
 *
 * @return A newly created PyVortexCtx reference.
 */
PyObject * py_vortex_ctx_create (VortexCtx * ctx)
{
	/* return a new instance */
	PyVortexCtx * obj = (PyVortexCtx *) PyObject_CallObject ((PyObject *) &PyVortexCtxType, NULL);

	if (obj) {
		/* acquire a reference to the VortexCtx if defined */
		if (ctx) {
			py_vortex_log (PY_VORTEX_DEBUG, "found ctx reference defined, creating PyVortexCtx reusing reference received (deallocating previous one)");
			/* free previous ctx */
			vortex_ctx_free (obj->ctx);

			obj->ctx = ctx;
			vortex_ctx_ref (ctx);
		} 

		/* flag to not exit once the ctx is deallocated */
		obj->exit_pending = axl_false;
		
		py_vortex_log (PY_VORTEX_DEBUG, "created vortex.Ctx (self: %p, self->ctx: %p)", 
			       obj, obj->ctx);
		
		return __PY_OBJECT (obj);
	} /* end if */

	/* failed to create object */
	return NULL;
}

/** 
 * @brief Allows to store a python object into the provided vortex.Ctx
 * object, incrementing the reference count. The object is
 * automatically removed when the vortex.Ctx reference is collected.
 */
void        py_vortex_ctx_register (VortexCtx  * ctx,
				    PyObject   * data,
				    const char * key,
				    ...)
{
	va_list    args;
	char     * full_key;

	/* check data received */
	if (key == NULL || ctx == NULL)
		return;

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);

	/* check to remove */
	if (data == NULL) {
		vortex_ctx_set_data (ctx, full_key, NULL);
		axl_free (full_key);
		return;
	} /* end if */
	
	/* now register the data received into the key created */
	py_vortex_log (PY_VORTEX_DEBUG, "registering key %s = %p on vortex.Ctx %p",
		       full_key, data, ctx);
	Py_INCREF (data);
	vortex_ctx_set_data_full (ctx, full_key, data, axl_free, (axlDestroyFunc) py_vortex_decref);
	return;
}


/** 
 * @brief Allows to get the object associated to the key provided. The
 * reference returned is still owned by the internal hash. Use
 * Py_INCREF in the case a new reference must owned by the caller.
 */
PyObject  * py_vortex_ctx_register_get (VortexCtx  * ctx,
					const char * key,
					...)
{
	va_list    args;
	char     * full_key;
	PyObject * data;

	/* check data received */
	if (key == NULL || ctx == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to register data, key %p or vortex.Ctx %p reference is null",
			       key, ctx);
		return NULL;
	} /* end if */

	va_start (args, key);
	full_key = axl_strdup_printfv (key, args);
	va_end   (args);
	
	/* now register the data received into the key created */
	data = __PY_OBJECT (vortex_ctx_get_data (ctx, full_key));
	py_vortex_log (PY_VORTEX_DEBUG, "returning key %s = %p on vortex.Ctx %p",
		       full_key, data, ctx);
	axl_free (full_key);
	return data;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyVortexCtx reference.
 */
axl_bool             py_vortex_ctx_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexCtxType);
}

/** 
 * @brief Inits the vortex ctx module. It is implemented as a type.
 */
void init_vortex_ctx (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexCtxType) < 0)
		return;
	
	Py_INCREF (&PyVortexCtxType);
	PyModule_AddObject(module, "Ctx", (PyObject *)&PyVortexCtxType);

	return;
}


:mod:`vortex.Ctx` --- PyVortexCtx class: context handling
=========================================================

.. currentmodule:: vortex


API documentation for vortex.Ctx object representing a vortex
independent context. vortex.Ctx object is a fundamental type and
represents a single execution context within a process, that includes
the reader loop, the sequencer loop and a shared context execution for
all connections, listeners and channels created inside it.

In most situations it is only required to create a single context as follows::

   ctx = vortex.Ctx ()
   if not ctx.init ():
        print ("ERROR: Failed to create vortex.Ctx object")

   # now it is possible to create connections and channels
   conn = vortex.Connection (ctx, "localhost", "602")
   if not conn.is_ok ():
        print ("ERROR: connection failed to localhost, error was: " + conn.error_msg)

Due to python automatic collection it is not required to explicitly
close a context because that's is done automatically when the
function/method that created the ctx object finishes. However, to
explicitly terminate a vortex execution context just do::

  ctx.exit ()

After creating a context, you can now either create a connection or a
listener using:

.. toctree::
   :maxdepth: 1

   connection

==========
Module API
==========

.. class:: Ctx

   .. method:: init ()
   
      Allows to init the context created.

      :rtype: Returns True if the context was started and initialized or False if failed.

   .. method:: exit ()
   
      Allows to finish an initialized context (:meth:`Ctx.init`)

   .. method:: new_event (microseconds, handler, [user_data], [user_data2])
   
      Allows to install an event handler, a method that will be called each microseconds, optionally receiving one or two parameters. This method implements python api for vortex_thread_pool_new_event. See also its documentation.

      The following is an example of handler (first parameter is vortex.Ctx, the rest two are the user parameters)::

            def event_handler (ctx, param1, param2):
      	    	# do some stuff
		if something:
	     	   return True # return True to remove the event (no more calls)

	  	return False # make the event to keep on working (more calls to come)

      :rtype: Returns a handle id representing the event installed. This handle id can be used to remove the event from outside the handler. Use :mod:`remove_event` to do so.

   .. method:: remove_event (handle_id)
   
      Allows to remove an event installed using :mod:`new_event` providing the handle_id representing the event. This value is the unique identifier returned by :mod:`new_event` every time a new event is installed.

      :rtype: Returns True if the event was found and removed, otherwise False is returned.

   .. method:: enable_too_long_notify_to_file (watching_period, file)
   
      Allows to enable a too long notify handler that internally logs those notifications into the provided file. The watching_period allows to control what is the period over which a notification is recorded.

      :rtype: Returns True if the notifier was installed

   .. attribute:: log

      (Read/Write attribute) (True/False) returns or set current debug log. See vortex_log_is_enabled.

   .. attribute:: log2

      (Read/Write attribute) (True/False) returns or set current second level debug log which includes more detailed messages suppresed. See vortex_log2_is_enabled.

   .. attribute:: color_log

      (Read only attribute) (True/False) returns or set current debug log colourification. See vortex_color_log_is_enabled.

   .. attribute:: ref_count

      (Read only attribute) (Number) returns current vortex.Ctx reference counting state.


 
      
    

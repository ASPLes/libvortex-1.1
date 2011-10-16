:mod:`vortex.ctx` --- LuaVortexCtx class: context handling
==========================================================

.. currentmodule:: vortex


API documentation for vortex.ctx object representing a vortex
independent context. vortex.ctx object is a fundamental type and
represents a single execution context within a process, that includes
the reader loop, the sequencer loop and a shared context execution for
all connections, listeners and channels created inside it.

In most situations it is only required to create a single context as follows::

    -- create context 
    ctx = vortex.ctx.new ()
   
    -- init context
    if not ctx:init () then
       print ("ERROR: expected to find proper vortex.ctx initialization..")
       return false
    end

Due to lua automatic collection it is not required to explicitly
close a context because that's is done automatically when the
function/method that created the ctx object finishes. 

After creating a context, you can now either create a connection or a
listener using:

.. toctree::
   :maxdepth: 1

   connection

==========
Module API
==========

.. class:: ctx

   .. method:: new ()

      Creates a new uninitialized vortex.ctx object. Then a call to
      :mod:`vortex.ctx.init` is requierd to start the vortex engine.

      :rtype: Returns a new :mod:`vortex.ctx` object.

   .. method:: init (ctx)
   
      Allows to init the context created.   

      :param ctx: An uninitialized :mod:`vortex.ctx` object created with :mod:`vortex.ctx.new`
      :type  ctx: A :mod:`vortex.ctx` object.

      :rtype: Returns true if the context was started and initialized or false if failed.

   .. method:: new_event (ctx, microseconds, handler, [user_data], [user_data2])
   
      Allows to install an event handler, a method that will be called
      each microseconds, optionally receiving one or two
      parameters. This method implements lua api for
      vortex_thread_pool_new_event. See also its documentation.

      The following is an example of handler (first parameter is
      :mod:`vortex.ctx`, the rest two are the user parameters)::

          -- install a handler caller each 10000 microseconds
	  event_id = ctx:new_event (10000, 
	  		 function (ctx, param1, param2) 
			     print ("Handler executed!")
			 end)

      :param ctx: The vortex context where the event will be installed.
      :type  ctx: A :mod:`vortex.ctx` object

      :param microseconds: Period between each handler execution measured in microseconds.
      :type  microseconds: Number 

      :param handler: A user defined handler.
      :type  handler: Handler

      :rtype: Returns the event id unique identifier installer.

   .. method:: check (ctx)

      Allows to check if the provided :mod:`vortex.ctx` is indeed a :mod:`vortex.ctx` object.
   
      :rtype: Returns true in the case it is a :mod:`vortex.ctx` object. Otherwise, false is returned.

   .. attribute:: log

      (Read/Write attribute) (true/false) returns or set current debug log. See vortex_log_is_enabled.

   .. attribute:: log2

      (Read/Write attribute) (true/false) returns or set current second level debug log which includes more detailed messages suppresed. See vortex_log2_is_enabled.

   .. attribute:: color_log

      (Read only attribute) (true/false) returns or set current debug log colourification. See vortex_color_log_is_enabled.

   .. attribute:: ref_count

      (Read only attribute) (Number) returns current :mod:`vortex.ctx` reference counting state.




 
      
    

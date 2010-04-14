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

   .. attribute:: log

      (Read/Write attribute) (True/False) returns or set current debug log. See vortex_log_is_enabled.

   .. attribute:: log2

      (Read/Write attribute) (True/False) returns or set current second level debug log which includes more detailed messages suppresed. See vortex_log2_is_enabled.

   .. attribute:: color_log

      (Read only attribute) (True/False) returns or set current debug log colourification. See vortex_color_log_is_enabled.

   .. attribute:: conn

      (Read only attribute) (vortex.Connection) returns a reference to the connection where the channel is working.

   .. attribute:: set_serialize

      (Write only attribute) (True/False) Allows to configure channel delivery serialization. See also vortex_channel_set_serialize.


 
      
    

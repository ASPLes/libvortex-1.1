:mod:`vortex` --- PyVortexCtx class: context handling
=====================================================

.. currentmodule:: vortex


API documentation for vortex.Ctx object representing a vortex
independent context.

==========
Module API
==========

.. class:: Ctx

   .. method:: init ()
   
      Allows to init the context created.

      :rtype: Returns True if the context was started and initialized or False if failed.

   .. method:: exit ()
   
      Allows to finish an initialized context (:meth:`Ctx.init`)

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


 
      
    

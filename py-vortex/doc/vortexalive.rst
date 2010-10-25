:mod:`vortex.alive` --- PyVortex ALIVE module: ALIVE profile support
====================================================================

.. module:: vortex.alive
   :synopsis: Vortex library ALIVE module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required to implement connection
ALIVE through Vortex Library ALIVE implementation.

See notes from C API explaining how to use ALIVE API. Because this module is a binding for that API, all documentation applies:

http://www.aspl.es/fact/files/af-arch/vortex-1.1/html/starting_to_program.html#vortex_manual_alive_api

==========
Module API
==========

.. function:: init (ctx)

   Allows to init ALIVE module on the provided vortex.Ctx reference. This is only required at the receiving side that is, the peer that will be monitored.

   :param ctx: vortex context where ALIVE module will be initialized
   :type ctx: vortex.Ctx

   :rtype: True it initialization was completed, otherwise False is returned.

.. function:: enable_check (conn, check_period, max_unreply_count, failure_handler)

   Activates the ALIVE connection check on the provided connection. The remote side must accept being monitored by calling to :meth:`init`. In the case a failure_handler is provided it will be cause when the failure is found but the connection will not be closed. In the case the failure handler is not configured and a failure is found, the connection is shutted down.
   
   :param conn: The connection where the ALIVE process will take place.
   :type  conn: vortex.Connection

   :param check_period: The check period. It is defined in microseconds (1 second == 1000000 microseconds). To check every 20ms a* connection pass 20000. It must be > 0, or the function will return False.
   :type  serverName: Int

   :param max_unreply_count: The maximum amount of unreplied messages we accept. It must be > 0 or the function will return False.
   :type  max_unreply_count: Int

   :param failure_handler: Optional handler called when a failure is detected.
   :type  failure_handler: Handler

   :rtype: True it initialization was completed, otherwise False is returned.


   

   


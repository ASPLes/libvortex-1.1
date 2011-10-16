:mod:`vortex.asyncqueue` --- LuaVortexAsyncQueue class: Vortex library thread safe queue
========================================================================================

.. currentmodule:: vortex

API documentation for vortex.asyncqueue object: a thread safe queue
used by Vortex Library.

==========
Module API
==========

.. class:: asyncqueue

   .. method:: push (object)
   
      Allows to push an object into the queue.

      :param object: The object to be pushed.
      :type  object: object

   .. method:: pop ()
   
      Allows to retrieve the next object available in the queue

      :rtype: Returns the object stored in the queue or lock the caller until an object is available.

   .. method:: timedout (microseconds)
   
      Allows to retrieve the next object available in the queue, limiting the wait period to the provided value (microseconds).

      :param microseconds: How long the wait period must hold
      :type  microseconds: Number

      :rtype: Returns the object stored in the queue or lock the caller until an object is available or timeout expires, in that case nil is returned.

   .. method:: check (asyncqueue)

      :param asyncqueue: The asyncqueue.
      :type  asyncqueue: vortex.asyncqueue

      Allows to check if the provided :mod:`vortex.asyncqueue` is indeed a :mod:`vortex.asyncqueue` object.
   
      :rtype: Returns true in the case it is a :mod:`vortex.asyncqueue` object. Otherwise, false is returned.


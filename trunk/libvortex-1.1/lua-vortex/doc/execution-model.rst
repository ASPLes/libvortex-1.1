.. _execution-model-label:

LuaVortex's execution model
===========================

The following page explain how LuaVortex deals with vortex threading,
especially async handler notification.

=================================================
Some concepts before start: mixing Lua and Vortex
=================================================

In simple terms, lua places no restriction about how many interpreters
or threads can be running inside a single process, but, of course, it
requires you to ensure only one thread touches the "lua state" at the
same time.

In essense, you can run several "threads" using lua's coroutine
infrastructure, taking care that only one thread is running at
time. However, lua engine, due to its design and objective, does not
provide a GIL (global interpreter lock) or something similar, like
python does.

And what is this GIL and why it is important? Well, this concept means
the "engine" provides an infrastructure to acquire and release a
special lock that ensures only one thread at time touches sensible
parts of the state.

But it is not just a lock, but a global concept that covers more
elements (including the core interpreter and its libraries). 

For example, python acquires and releases the GIL (the engine itself)
to ensure other threads can execute without needing explicit
collaboration (i.e. it is not needed something similar to lua's
corroutine yield()).

GIL also means many function inside interpreters that implements it,
"takes care" of this, releasing the GIL for long running operations,
making the interpreter and the whole library it comes with to play
well with threaded libraries that uses the GIL.

Because Vortex Library is heavy threaded and all notifications are
done using threads, we need a mechanism to collaborate with the
language (lua) in a way all operations remains non-blocking and the
user can still receive async notifications, despite lua lacks of GIL.

====================================
LuaVortex's client support functions
====================================

So far, LuaVortex provides a set of functions that allows easy lua
integration with Vortex Library design. These functions are the following:

  - vortex.yield ()
  - vortex.event_fd ()
  - vortex.wait_events ()

The interesting thing is that all these functions are only required in
the client side, because, at the server, you directly receive async
notifications which is where Vortex Library shines!...however, at the
client it is required a bit of additional work. 

In essense, when a lua client is running, it owns the execution and
can make all async operations without problems, but the client "must"
:mod:`vortex.yield` the execution to allow vortex engine to do
notifications (like frame received), that is, to let other threads to
run.

This is a natural mechanism for lua users (because it mimic lua
corroutine yield ()). Once vortex.yield () returns, it is said you
reacquires the execution. An example to show the concept would be::

       -- send message 
       if channel:send_msg ("This is a test...", 17) == nil then
          print ("ERROR: expected to find proper send operation..")
          return false
       end

       -- now yield
       print ("Let other threads to enter..")
       vortex.yield (1000000)

       -- if we are here, we own the execution

In many cases, you can base your client application by just calling
vortex.yield enough frequently to let other threads to run. In the
case you want finer control over waiting events, you can use
:mod:`vortex.event_fd` () to get a file descriptor that emit changes when a
thread is asking for permission to run.

This function returns a file descriptor that can be watched (among
others) for changes, so you call vortex.yield when they are
detected. The concept is shown in the following example::


   while true do
       -- wait for events (graphical and vortex events)
       event_fd = vortex.event_fd ()

       -- main loop: watch for events
       engine.main_loop ({event_fd, some_fd, etc_fd})

       -- handle, among other things, events from vortex engine
       if vortex.wait_events (1) then
           -- it is recommended to not yield for ever
           vortex.yield (1)
       end
   end

In the previous example, get the the event file descriptor associated
at the current lua's execution. Futher calls to this function returns
the same file descriptor (AND you should never close it).

Once your wait for changes function returns (in this example
represented by the hipothetical function engine.main_loop), the client
application checks for pending events with :mod:`vortex.wait_events`. If the
function returns true, then a call to :mod:`vortex.yield` () happens, causing
one waiting thread to enter into lua space.

===========================================
Some conclusions: everything has advantages
===========================================

The lack of GIL support by lua it's a problem at the first step
because you don't have a "ready to use" mechanism to play well with
threaded libraries.

However, behind this missing feature, it is found an advantage that
languages with GIL doesn't have (i.e. python), that is, you can run
several lua execution contexts, with a good isolation level, all of
them having its own GIL mechanism.

Not having GIL support has the disadvantage the client side requires
explicit collaboration (that is, yield () or similar), but at the
server side, this collaboration is not required **and** the most
important, you can run several lua interpreters inside the same
process which is something python and any other GIL enabled language
can't (either because it is not technically possible to separate the
interpreter from the GIL or because when possible, they only provides
an illusion of that concept).







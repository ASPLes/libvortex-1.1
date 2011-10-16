LuaVortex's documentation center!
=================================

LuaVortex is a lua (http://www.lua.org) binding for Vortex Library
1.1, maintained and supported by ASPL (http://www.aspl.es), that includes full support to
write client/listener BEEP applications written entirely in lua.

Because quality matters, as with Vortex Library 1.1, LuaVortex
development is being driven and checked with a regression test suite
to ensure each realease is ready for production environment.

LuaVortex execution model for async notifications is really similar to
Vortex Library, borrowing GIL concept from python, extending and
adapting it to lua's way of function. In simple terms, this means
LuaVortex library execution will still use threads but only one thread
at time will be executing inside the context of Lua. See
:ref:`execution-model-label` for details.

Because LuaVortex is a binding, Vortex Library 1.1 documention must be
used while using LuaVortex: http://www.aspl.es/fact/files/af-arch/vortex-1.1/html/index.html

**Manuals and additional documentation available:**

.. toctree::
   :maxdepth: 1

   license
   execution-model
   manual
   Vortex Library 1.1 documentation center <http://www.aspl.es/fact/files/af-arch/vortex-1.1/html/index.html>

**API documentation:**

.. toctree::
   :maxdepth: 1

   vortex
   ctx
   connection
   channel
   frame
   channelpool
   asyncqueue
   vortexsasl
   handlers
   vortextls
   vortexalive

=================
Community support
=================

Community assisted support is provided through Vortex Library mailing list located at: http://lists.aspl.es/cgi-bin/mailman/listinfo/vortex.

============================
Professional ensured support
============================

ASPL provides professional support for LuaVortex inside Vortex Library
Tech Support program. See the following for more information:
http://www.aspl.es/vortex/professional.html



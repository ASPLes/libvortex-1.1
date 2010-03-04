PyVortex's documentation center!
================================

PyVortex is a python binding for the Vortex Library 1.1 that includes
full support to write client/listener BEEP applications
written entirely in python. 

Currently it is supported vortex base module (:mod:`vortex`), vortex TLS and vortex SASL (:mod:`vortex.sasl`)
extensions. As with Vortex Library 1.1, PyVortex development is being
driven and checked with a regression test suite.

PyVortex execution model for async notifications is really similar to
Vortex Library because the binding makes use of the GIL feature
described in: http://www.python.org/dev/peps/pep-0311/ This means
PyVortex library execution will still use threads but only one thread
at time will be executing inside the context of Python. 

Because PyVortex is a binding, Vortex Library 1.1 documention must be
used while using PyVortex: http://www.aspl.es/fact/files/af-arch/vortex-1.1/html/index.html

**Manuals and additional documentation available:**

.. toctree::
   :maxdepth: 1

   license
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
   handlers
   vortexsasl
   vortextls

=================
Community support
=================

Community assisted support is provided through Vortex Library mailing list located at: http://lists.aspl.es/cgi-bin/mailman/listinfo/vortex.

============================
Professional ensured support
============================

ASPL provides professional support for PyVortex inside Vortex Library
Tech Support program. See the following for more information:
http://www.aspl.es/vortex/professional.html



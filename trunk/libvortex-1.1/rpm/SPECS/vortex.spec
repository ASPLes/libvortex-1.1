%define vortex_version %(cat VERSION)

Name:           vortex
Version:        %{vortex_version}
Release:        5%{?dist}
Summary:        BEEP core implemetation written in C
Group:          System Environment/Libraries
License:        LGPLv2+ 
URL:            http://www.aspl.es/vortex
Source:         %{name}-%{version}.tar.gz


%define debug_package %{nil}

%description
Vortex Library is an implementation of the RFC 3080 / RFC 3081
standard definitions, known as the BEEP Core protocol, implemented
on top of the TCP/IP stack, using C language. Additionally, it comes
with a complete XML-RPC over BEEP RFC 3529 support and complete
support for TUNNEL profile, which allows to perform proxy operations
for every BEEP profile developed.

%prep
%setup -q

%build
PKG_CONFIG_PATH=/usr/lib/pkgconfig:/usr/local/lib/pkgconfig %configure --prefix=/usr --sysconfdir=/etc
make clean
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} INSTALL='install -p'
find %{buildroot} -name '*.la' -exec rm -f {} ';'
# %find_lang %{name}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

# %files -f %{name}.lang
%doc AUTHORS COPYING NEWS README THANKS
# %{_libdir}/libaxl.so.*

# %files devel
# %doc COPYING
# %{_includedir}/axl*
# %{_libdir}/libaxl.so
# %{_libdir}/pkgconfig/axl.pc

# libvortex-1.1 package
%package -n libvortex-1.1
Summary: Base library, core implementation
Group: System Environment/Libraries
Requires: libaxl1
%description  -n libvortex-1.1
Base library, core implementation
%files -n libvortex-1.1
   /usr/lib64/libvortex-1.1.a
   /usr/lib64/libvortex-1.1.so
   /usr/lib64/libvortex-1.1.so.0
   /usr/lib64/libvortex-1.1.so.0.0.0

# libvortex-1.1-dev package
%package -n libvortex-1.1-dev
Summary: Development headers for the core library implementation.
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-1.1-dev
Development headers for the core library implementation.
%files -n libvortex-1.1-dev
   /usr/include/vortex-1.1/vortex-channel.dtd.h
   /usr/include/vortex-1.1/vortex-listener-conf.dtd.h
   /usr/include/vortex-1.1/vortex.h
   /usr/include/vortex-1.1/vortex_channel.h
   /usr/include/vortex-1.1/vortex_channel_pool.h
   /usr/include/vortex-1.1/vortex_connection.h
   /usr/include/vortex-1.1/vortex_connection_private.h
   /usr/include/vortex-1.1/vortex_ctx.h
   /usr/include/vortex-1.1/vortex_ctx_private.h
   /usr/include/vortex-1.1/vortex_dtds.h
   /usr/include/vortex-1.1/vortex_errno.h
   /usr/include/vortex-1.1/vortex_frame_factory.h
   /usr/include/vortex-1.1/vortex_greetings.h
   /usr/include/vortex-1.1/vortex_handlers.h
   /usr/include/vortex-1.1/vortex_hash.h
   /usr/include/vortex-1.1/vortex_hash_private.h
   /usr/include/vortex-1.1/vortex_io.h
   /usr/include/vortex-1.1/vortex_listener.h
   /usr/include/vortex-1.1/vortex_payload_feeder.h
   /usr/include/vortex-1.1/vortex_payload_feeder_private.h
   /usr/include/vortex-1.1/vortex_profiles.h
   /usr/include/vortex-1.1/vortex_queue.h
   /usr/include/vortex-1.1/vortex_reader.h
   /usr/include/vortex-1.1/vortex_sequencer.h
   /usr/include/vortex-1.1/vortex_support.h
   /usr/include/vortex-1.1/vortex_thread.h
   /usr/include/vortex-1.1/vortex_thread_pool.h
   /usr/include/vortex-1.1/vortex_types.h
   /usr/include/vortex-1.1/vortex_win32.h
   /usr/lib64/pkgconfig/vortex-1.1.pc
   /usr/share/libvortex-1.1/channel.dtd
   /usr/share/libvortex-1.1/test-certificate.pem
   /usr/share/libvortex-1.1/test-private-key.pem
   /usr/share/libvortex-1.1/vortex-listener-conf.dtd

# libvortex-tls-1.1 package
%package -n libvortex-tls-1.1
Summary: BEEP TLS profile support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: openssl
%description  -n libvortex-tls-1.1
This package contains the extension library to add support for TLS
profile to core Vortex Library.
%files -n libvortex-tls-1.1
   /usr/lib64/libvortex-tls-1.1.a
   /usr/lib64/libvortex-tls-1.1.so
   /usr/lib64/libvortex-tls-1.1.so.0
   /usr/lib64/libvortex-tls-1.1.so.0.0.0

# libvortex-tls-1.1-dev package
%package -n libvortex-tls-1.1-dev
Summary: BEEP TLS profile support for Vortex Library (devel headers).
Group: System Environment/Libraries
Requires: libvortex-tls-1.1
Requires: openssl-devel
%description  -n libvortex-tls-1.1-dev
Development headers for BEEP TLS profile.
%files -n libvortex-tls-1.1-dev
   /usr/include/vortex-1.1/vortex-tls.dtd.h
   /usr/include/vortex-1.1/vortex_tls.h
   /usr/lib64/pkgconfig/vortex-tls-1.1.pc
   /usr/share/libvortex-1.1/tls.dtd

# libvortex-sasl-1.1 package
%package -n libvortex-sasl-1.1
Summary: BEEP SASL profiles support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libgsasl7
%description  -n libvortex-sasl-1.1
This package contains the extension library to add support for SASL
profiles to the core Vortex Library.
%files -n libvortex-sasl-1.1
   /usr/lib64/libvortex-sasl-1.1.a
   /usr/lib64/libvortex-sasl-1.1.so
   /usr/lib64/libvortex-sasl-1.1.so.0
   /usr/lib64/libvortex-sasl-1.1.so.0.0.0
   /usr/share/libvortex-1.1/sasl.dtd

# libvortex-sasl-1.1-dev package
%package -n libvortex-sasl-1.1-dev
Summary: BEEP SASL profiles support for Vortex Library (devel headers)
Group: System Environment/Libraries
Requires: libvortex-sasl-1.1
# Requires: libgsasl (missing package)
%description  -n libvortex-sasl-1.1-dev
Development headers for BEEP SASL profiles for Vortex Library
%files -n libvortex-sasl-1.1-dev
   /usr/include/vortex-1.1/vortex-sasl.dtd.h
   /usr/include/vortex-1.1/vortex_sasl.h
   /usr/lib64/pkgconfig/vortex-sasl-1.1.pc

# libvortex-tunnel-1.1 package
%package -n libvortex-tunnel-1.1
Summary: BEEP TUNNEL profile support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-tunnel-1.1
This package contains the extension library to add support for TUNNEL
profile to the core Vortex Library. TUNNEL profile is the proxy standard
definition for BEEP protocol.
%files -n libvortex-tunnel-1.1
   /usr/lib64/libvortex-tunnel-1.1.a
   /usr/lib64/libvortex-tunnel-1.1.so
   /usr/lib64/libvortex-tunnel-1.1.so.0
   /usr/lib64/libvortex-tunnel-1.1.so.0.0.0

# libvortex-tunnel-1.1-dev package
%package -n libvortex-tunnel-1.1-dev
Summary: BEEP TUNNEL profile support for Vortex Library (devel headers)
Group: System Environment/Libraries
Requires: libvortex-tunnel-1.1
%description  -n libvortex-tunnel-1.1-dev
Development headers for BEEP TUNNEL profile for Vortex Library.
%files -n libvortex-tunnel-1.1-dev
   /usr/include/vortex-1.1/vortex_tunnel.h
   /usr/lib64/pkgconfig/vortex-tunnel-1.1.pc

# libvortex-xml-rpc-1.1 package
%package -n libvortex-xml-rpc-1.1
Summary: BEEP XML-RPC profile support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-xml-rpc-1.1
This package contains the extension library to add support for XML-RPC
over BEEP profile to the core Vortex Library. 
%files -n libvortex-xml-rpc-1.1
   /usr/lib64/libvortex-xml-rpc-1.1.a
   /usr/lib64/libvortex-xml-rpc-1.1.so
   /usr/lib64/libvortex-xml-rpc-1.1.so.0
   /usr/lib64/libvortex-xml-rpc-1.1.so.0.0.0

# libvortex-xml-rpc-1.1-dev package
%package -n libvortex-xml-rpc-1.1-dev
Summary: XML-RPC over BEEP profile support for Vortex Library (devel headers)
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-xml-rpc-1.1-dev
Development headers for XML-RPC over BEEP profile for Vortex Library.
%files -n libvortex-xml-rpc-1.1-dev
   /usr/include/vortex-1.1/vortex_xml_rpc.h
   /usr/include/vortex-1.1/vortex_xml_rpc_types.h
   /usr/lib64/pkgconfig/vortex-xml-rpc-1.1.pc
   /usr/share/libvortex-1.1/xml-rpc-boot.dtd
   /usr/share/libvortex-1.1/xml-rpc.dtd

# libvortex-pull-1.1 package
%package -n libvortex-pull-1.1
Summary: PULL API for single thread programming with Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-pull-1.1
Vortex PULL API is an extension library that provides a single thread 
programming interface that marshalls all async notifications into
pullable events that are handled inside the same thread.
%files -n libvortex-pull-1.1
   /usr/lib64/libvortex-pull-1.1.a
   /usr/lib64/libvortex-pull-1.1.so
   /usr/lib64/libvortex-pull-1.1.so.0
   /usr/lib64/libvortex-pull-1.1.so.0.0.0

# libvortex-pull-1.1-dev package
%package -n libvortex-pull-1.1-dev
Summary: PULL API extension development headers
Group: System Environment/Libraries
Requires: libvortex-pull-1.1
%description  -n libvortex-pull-1.1-dev
Development headers for PULL API extension
%files -n libvortex-pull-1.1-dev
   /usr/include/vortex-1.1/vortex_pull.h
   /usr/lib64/pkgconfig/vortex-pull-1.1.pc

# libvortex-http-1.1 package
%package -n libvortex-http-1.1
Summary: HTTP CONNECT extension library for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-http-1.1
Vortex HTTP CONNECT is an extension library that allows to connect to 
BEEP listener by using a HTTP proxy serve
%files -n libvortex-http-1.1
   /usr/lib64/libvortex-http-1.1.a
   /usr/lib64/libvortex-http-1.1.so
   /usr/lib64/libvortex-http-1.1.so.0
   /usr/lib64/libvortex-http-1.1.so.0.0.0

# libvortex-http-1.1-dev package
%package -n libvortex-http-1.1-dev
Summary: HTTP CONNECT extension library for Vortex Library (headers)
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-http-1.1-dev
Development headers for HTTP CONNECT extension library
%files -n libvortex-http-1.1-dev
   /usr/include/vortex-1.1/vortex_http.h
   /usr/lib64/pkgconfig/vortex-http-1.1.pc

# libvortex-alive-1.1 package
%package -n libvortex-alive-1.1
Summary: BEEP ALIVE support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
%description  -n libvortex-alive-1.1
This package contains ALIVE Vortex Library implementation
%files -n libvortex-alive-1.1
   /usr/lib64/libvortex-alive-1.1.a
   /usr/lib64/libvortex-alive-1.1.so
   /usr/lib64/libvortex-alive-1.1.so.0
   /usr/lib64/libvortex-alive-1.1.so.0.0.0

# libvortex-alive-1.1-dev package
%package -n libvortex-alive-1.1-dev
Summary: BEEP ALIVE support for Vortex Library (devel headers)
Group: System Environment/Libraries
Requires: libvortex-alive-1.1
%description  -n libvortex-alive-1.1-dev
Development headers for BEEP ALIVE profile
%files -n libvortex-alive-1.1-dev
   /usr/include/vortex-1.1/vortex_alive.h
   /usr/lib64/pkgconfig/vortex-alive-1.1.pc

# libvortex-websocket-1.1 package
%package -n libvortex-websocket-1.1
Summary: BEEP over WebSocket support for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libnopoll0
%description  -n libvortex-websocket-1.1
This package contains BEEP over WebSocket support for Vortex Library
%files -n libvortex-websocket-1.1
   /usr/lib64/libvortex-websocket-1.1.a
   /usr/lib64/libvortex-websocket-1.1.so
   /usr/lib64/libvortex-websocket-1.1.so.0
   /usr/lib64/libvortex-websocket-1.1.so.0.0.0

#  libvortex-websocket-1.1-dev package
%package -n  libvortex-websocket-1.1-dev
Summary: BEEP over WebSocket support for Vortex Library (headers)
Group: System Environment/Libraries
Requires: libvortex-websocket-1.1
%description  -n  libvortex-websocket-1.1-dev
This package contains BEEP over WebSocket support for Vortex Library.
%files -n  libvortex-websocket-1.1-dev
   /usr/include/vortex-1.1/vortex_websocket.h
   /usr/lib64/pkgconfig/vortex-websocket-1.1.pc

# vortex-xml-rpc-gen-1.1 package
%package -n  vortex-xml-rpc-gen-1.1
Summary: Tool to create support files to execute XML-RPC services
Group: System Environment/Libraries
Requires: libvortex-websocket-1.1
%description  -n  vortex-xml-rpc-gen-1.1
This package contains the xml-rpc-gen tool which is used to create 
client and server side files to support XML-RPC over BEEP.
%files -n  vortex-xml-rpc-gen-1.1
   /usr/bin/xml-rpc-gen-1.1

# vortex-client-1.1 package
%package -n  vortex-client-1.1
Summary: BEEP core implemetation client
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-tls-1.1
Requires: libvortex-xml-rpc-1.1
%description  -n  vortex-client-1.1
This package contains vortex-client, a command line that allows
to connect to and debug BEEP peers.
%files -n  vortex-client-1.1
  /usr/bin/vortex-client

# python-vortex package
%package -n  python-vortex
Summary: Python bindings for Vortex Library
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: python
%description  -n  python-vortex
Python bindings for Vortex Library (base library)
%files -n  python-vortex
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_11.a
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_11.so
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_11.so.0
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_11.so.0.0.0
   /usr/lib/python2.6/site-packages/vortex/__init__.py
   /usr/lib/python2.6/site-packages/vortex/__init__.pyc
   /usr/lib/python2.6/site-packages/vortex/__init__.pyo


# python-vortex-dev package
%package -n  python-vortex-dev
Summary: Development headers for Axl Python bindings
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: python
Requires: python-vortex
%description  -n  python-vortex-dev
Python Vortex bindings development headers.
%files -n  python-vortex-dev
   /usr/include/py_vortex/py_vortex.h
   /usr/include/py_vortex/py_vortex_async_queue.h
   /usr/include/py_vortex/py_vortex_channel.h
   /usr/include/py_vortex/py_vortex_channel_pool.h
   /usr/include/py_vortex/py_vortex_connection.h
   /usr/include/py_vortex/py_vortex_ctx.h
   /usr/include/py_vortex/py_vortex_frame.h
   /usr/include/py_vortex/py_vortex_handle.h
   /usr/lib64/pkgconfig/py-vortex.pc

# python-vortex-tls package
%package -n  python-vortex-tls
Summary: Python bindings for Vortex Library TLS profile
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-tls-1.1
Requires: python
Requires: python-vortex
%description  -n  python-vortex-tls
Python bindings for Vortex Library TLS profile.
%files -n  python-vortex-tls
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_tls_11.a
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_tls_11.so
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_tls_11.so.0
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_tls_11.so.0.0.0
   /usr/lib/python2.6/site-packages/vortex/tls.py
   /usr/lib/python2.6/site-packages/vortex/tls.pyc
   /usr/lib/python2.6/site-packages/vortex/tls.pyo


# python-vortex-tls-dev package
%package -n  python-vortex-tls-dev
Summary: Development headers for Python Vortex TLS bindings
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-tls-1.1-dev
Requires: python
Requires: python-vortex-tls
%description  -n  python-vortex-tls-dev
Python Vortex TLS  bindings development headers
%files -n  python-vortex-tls-dev
   /usr/include/py_vortex/py_vortex_tls.h
   /usr/lib64/pkgconfig/py-vortex-tls.pc

# python-vortex-sasl package
%package -n  python-vortex-sasl
Summary: Python bindings for Vortex Library SASL profile
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-sasl-1.1
Requires: python
Requires: python-vortex
%description  -n  python-vortex-sasl
Python bindings for Vortex Library SASL profile
%files -n  python-vortex-sasl
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_sasl_11.a
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_sasl_11.so
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_sasl_11.so.0
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_sasl_11.so.0.0.0
   /usr/lib/python2.6/site-packages/vortex/sasl.py
   /usr/lib/python2.6/site-packages/vortex/sasl.pyc
   /usr/lib/python2.6/site-packages/vortex/sasl.pyo


# python-vortex-sasl-dev package
%package -n  python-vortex-sasl-dev
Summary: Development headers for Python Vortex SASL bindings
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-sasl-1.1
Requires: libvortex-sasl-1.1-dev
Requires: python-vortex-sasl
%description  -n  python-vortex-sasl-dev
Python Vortex SASL  bindings development headers
%files -n  python-vortex-sasl-dev
   /usr/include/py_vortex/py_vortex_sasl.h
   /usr/lib64/pkgconfig/py-vortex-sasl.pc

# python-vortex-alive package
%package -n  python-vortex-alive
Summary: Python bindings for Vortex Library ALIVE support
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-alive-1.1
Requires: python
Requires: python-vortex
%description  -n  python-vortex-alive
Python bindings for Vortex Library ALIVE profile
%files  -n  python-vortex-alive
   /usr/lib/python2.6/site-packages/vortex/alive.py
   /usr/lib/python2.6/site-packages/vortex/alive.pyc
   /usr/lib/python2.6/site-packages/vortex/alive.pyo
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_alive_11.a
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_alive_11.so
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_alive_11.so.0
   /usr/lib/python2.6/site-packages/vortex/libpy_vortex_alive_11.so.0.0.0


# python-vortex-alive-dev package
%package -n  python-vortex-alive-dev
Summary: Python bindings for Vortex Library ALIVE support (headers)
Group: System Environment/Libraries
Requires: libvortex-1.1
Requires: libvortex-alive-1.1
Requires: python
Requires: python-vortex
Requires: python-vortex-alive
%description  -n  python-vortex-alive-dev
Python bindings for Vortex Library ALIVE profile (devel headers)
%files -n  python-vortex-alive-dev
   /usr/include/py_vortex/py_vortex_alive.h



%changelog
* Sun Apr 07 2015 Francis Brosnan Blázquez <francis@aspl.es> - %{vortex_version}
- New upstream release


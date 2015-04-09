EXTRA_DIST=__init__.py sasl.py tls.py alive.py

if ENABLE_PY_VORTEX_LOG
INCLUDE_PY_VORTEX_LOG=-DENABLE_PY_VORTEX_LOG
endif

if ENABLE_SASL_SUPPORT
sasl_lib=libpy_vortex_sasl_11.la 
sasl_includes=-I$(top_srcdir)/sasl
endif

if ENABLE_TLS_SUPPORT
tls_lib=libpy_vortex_tls_11.la
tls_includes=-I$(top_srcdir)/tls
endif

INCLUDES = -Wall -g -ansi -I. -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(VORTEX_VERSION)\" \
	   $(AXL_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/alive $(sasl_includes) $(tls_includes) $(PYTHON_CFLAGS) $(INCLUDE_PY_VORTEX_LOG)

lib_LTLIBRARIES    = libpy_vortex_11.la $(sasl_lib) $(tls_lib) libpy_vortex_alive_11.la

# libpy_vortex_11.la
libpy_vortex_11_la_include_HEADERS  = \
	py_vortex.h \
	py_vortex_ctx.h \
	py_vortex_connection.h \
	py_vortex_channel.h  \
	py_vortex_async_queue.h \
	py_vortex_frame.h \
	py_vortex_channel_pool.h \
	py_vortex_handle.h
libpy_vortex_11_la_SOURCES  = \
	py_vortex.c \
	py_vortex_ctx.c \
	py_vortex_connection.c \
	py_vortex_channel.c \
	py_vortex_async_queue.c \
	py_vortex_frame.c \
	py_vortex_channel_pool.c \
	py_vortex_handle.c

libpy_vortex_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(AXL_LIBS)
libpy_vortex_11_la_includedir = $(includedir)/py_vortex

libpy_vortex_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

if ENABLE_SASL_SUPPORT
# libpy_vortex_sasl_11.la
libpy_vortex_sasl_11_la_include_HEADERS = \
	py_vortex_sasl.h
libpy_vortex_sasl_11_la_SOURCES = \
	py_vortex_sasl.c
libpy_vortex_sasl_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/sasl/libvortex-sasl-1.1.la $(AXL_LIBS) libpy_vortex_11.la
libpy_vortex_sasl_11_la_includedir = $(includedir)/py_vortex

libpy_vortex_sasl_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)
endif

if ENABLE_TLS_SUPPORT
# libpy_vortex_tls_11.la
libpy_vortex_tls_11_la_include_HEADERS = \
	py_vortex_tls.h
libpy_vortex_tls_11_la_SOURCES = \
	py_vortex_tls.c
libpy_vortex_tls_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/tls/libvortex-tls-1.1.la $(AXL_LIBS) libpy_vortex_11.la
libpy_vortex_tls_11_la_includedir = $(includedir)/py_vortex

libpy_vortex_tls_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)
endif

# libpy_vortex_alive_11.la
libpy_vortex_alive_11_la_include_HEADERS = \
	py_vortex_alive.h
libpy_vortex_alive_11_la_SOURCES = \
	py_vortex_alive.c
libpy_vortex_alive_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/alive/libvortex-alive-1.1.la $(AXL_LIBS) libpy_vortex_11.la
libpy_vortex_alive_11_la_includedir = $(includedir)/py_vortex

libpy_vortex_alive_11_la_LIBADD = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

libdir = ${pythondir}/vortex

# post-build script
all-local: 
	@echo "Running postbuild tasks.."

clean-local:
	@rm -f vortex$(SHARED_EXTENSION) _sasl$(SHARED_EXTENSION) _tls$(SHARED_EXTENSION) _alive$(SHARED_EXTENSION)

install-exec-hook:
	@echo "site installation ${DESTDIR}${pythondir}/vortex.."
	install -d ${DESTDIR}${pythondir}/vortex
	install -m 644 \
		sasl.py \
		tls.py \
		alive.py \
		__init__.py \
		${DESTDIR}${pythondir}/vortex

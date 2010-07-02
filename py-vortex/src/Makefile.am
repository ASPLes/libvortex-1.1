EXTRA_DIST=__init__.py sasl.py tls.py

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
	   $(AXL_CFLAGS) -I$(top_srcdir)/src $(sasl_includes) $(tls_includes) $(PYTHON_CFLAGS) $(INCLUDE_PY_VORTEX_LOG)

lib_LTLIBRARIES    = libpy_vortex_11.la $(sasl_lib) $(tls_lib)

# libpy_vortex_11.la
libpy_vortex_11_la_include_HEADERS  = \
	py_vortex.h \
	py_vortex_ctx.h \
	py_vortex_connection.h \
	py_vortex_channel.h  \
	py_vortex_async_queue.h \
	py_vortex_frame.h \
	py_vortex_channel_pool.h
libpy_vortex_11_la_SOURCES  = \
	py_vortex.c \
	py_vortex_ctx.c \
	py_vortex_connection.c \
	py_vortex_channel.c \
	py_vortex_async_queue.c \
	py_vortex_frame.c \
	py_vortex_channel_pool.c
libpy_vortex_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(AXL_LIBS)
libpy_vortex_11_la_includedir = $(includedir)/py_vortex

if ENABLE_SASL_SUPPORT
# libpy_vortex_sasl_11.la
libpy_vortex_sasl_11_la_include_HEADERS = \
	py_vortex_sasl.h
libpy_vortex_sasl_11_la_SOURCES = \
	py_vortex_sasl.c
libpy_vortex_sasl_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/sasl/libvortex-sasl-1.1.la $(AXL_LIBS) libpy_vortex_11.la
libpy_vortex_sasl_11_la_includedir = $(includedir)/py_vortex
endif

if ENABLE_TLS_SUPPORT
# libpy_vortex_tls_11.la
libpy_vortex_tls_11_la_include_HEADERS = \
	py_vortex_tls.h
libpy_vortex_tls_11_la_SOURCES = \
	py_vortex_tls.c
libpy_vortex_tls_11_la_LDFLAGS  = $(PYTHON_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/tls/libvortex-tls-1.1.la $(AXL_LIBS) libpy_vortex_11.la
libpy_vortex_tls_11_la_includedir = $(includedir)/py_vortex
endif

libdir = ${pythondir}/vortex

# post-build script
all-local: 
	@echo "Running postbuild tasks.."

clean-local:
	@rm -f vortex$(SHARED_EXTENSION) _sasl$(SHARED_EXTENSION) _tls$(SHARED_EXTENSION)

install-exec-hook:
	@echo "site installation ${DESTDIR}${pythondir}/vortex.."
	install -d ${DESTDIR}${pythondir}/vortex
	install -m 644 \
		sasl.py \
		tls.py \
		__init__.py \
		${DESTDIR}${pythondir}/vortex

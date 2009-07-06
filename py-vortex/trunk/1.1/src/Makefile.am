if ENABLE_PY_VORTEX_LOG
INCLUDE_PY_VORTEX_LOG=-DENABLE_PY_VORTEX_LOG
endif

INCLUDES = -Wall -g -ansi -I. -DCOMPILATION_DATE=`date +%s` \
	   -DVERSION=\"$(PY_VORTEX_VERSION)\" \
	   $(AXL_CFLAGS) $(VORTEX_CFLAGS) $(PYTHON_CFLAGS) $(INCLUDE_PY_VORTEX_LOG)

lib_LTLIBRARIES    = libpython-vortex.la
libpython_vortex_la_SOURCES  = \
	py_vortex.h py_vortex.c \
	py_vortex_ctx.h py_vortex_ctx.c \
	py_vortex_connection.h py_vortex_connection.c \
	py_vortex_channel.h py_vortex_channel.c \
	py_vortex_async_queue.h py_vortex_async_queue.c \
	py_vortex_frame.h py_vortex_frame.c

libpython_vortex_la_LDFLAGS  = $(PYTHON_LIBS) $(VORTEX_LIBS) $(AXL_LIBS)

# post-build script
all-local: libpython-vortex.la
	@echo "Running postbuild tasks.."
	@cp `find ./ -name libpython-vortex.so* -type f` vortex.so



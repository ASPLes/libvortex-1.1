if ENABLE_LUA_VORTEX_LOG
INCLUDE_LUA_VORTEX_LOG=-DENABLE_LUA_VORTEX_LOG
endif

if ENABLE_SASL_SUPPORT
sasl_lib=liblua_vortex_sasl_11.la 
sasl_includes=-I$(top_srcdir)/sasl
endif

if ENABLE_TLS_SUPPORT
tls_lib=liblua_vortex_tls_11.la
tls_includes=-I$(top_srcdir)/tls
endif

INCLUDES = $(compiler_options) $(AXL_CFLAGS) -I$(top_srcdir) -I$(top_srcdir)/src -I$(top_srcdir)/alive $(sasl_includes) $(tls_includes) -DVERSION=\""$(VORTEX_VERSION)"\"  $(VORTEX_CFLAGS) $(LUA_CFLAGS) $(INCLUDE_LUA_VORTEX_LOG)

lib_LTLIBRARIES = liblua_vortex_11.la $(sasl_lib) $(tls_lib) liblua_vortex_alive_11.la

## core library
liblua_vortex_11_includedir = $(includedir)/lua-vortex

liblua_vortex_11_la_SOURCES =	\
	lua-vortex.c \
	lua-vortex-ctx.c \
	lua-vortex-connection.c \
	lua-vortex-asyncqueue.c \
	lua-vortex-object.c \
	lua-vortex-channel.c \
	lua-vortex-frame.c \
	lua-vortex-channelpool.c

liblua_vortex_11_include_HEADERS = \
	lua-vortex.h \
	lua-vortex-ctx.h \
	lua-vortex-connection.h \
	lua-vortex-asyncqueue.h \
	lua-vortex-private.h \
	lua-vortex-object.h \
	lua-vortex-channel.h \
	lua-vortex-frame.h \
	lua-vortex-channelpool.h

liblua_vortex_11_la_LDFLAGS = $(LUA_LIBS) $(top_srcdir)/src/libvortex-1.1.la -no-undefined

## SASL module
if ENABLE_SASL_SUPPORT
liblua_vortex_sasl_11_includedir = $(includedir)/lua-vortex

liblua_vortex_sasl_11_la_SOURCES =	\
	lua-vortex-sasl.c 

liblua_vortex_sasl_11_include_HEADERS = \
	lua-vortex-sasl.h 

liblua_vortex_sasl_11_la_LDFLAGS = $(LUA_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/sasl/libvortex-sasl-1.1.la $(AXL_LIBS) -no-undefined liblua_vortex_11.la
endif

## TLS module
if ENABLE_TLS_SUPPORT
liblua_vortex_tls_11_includedir = $(includedir)/lua-vortex

liblua_vortex_tls_11_la_SOURCES =	\
	lua-vortex-tls.c 

liblua_vortex_tls_11_include_HEADERS = \
	lua-vortex-tls.h 

liblua_vortex_tls_11_la_LDFLAGS = $(LUA_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/tls/libvortex-tls-1.1.la $(AXL_LIBS) -no-undefined liblua_vortex_11.la
endif

## ALIVE module
liblua_vortex_alive_11_includedir = $(includedir)/lua-vortex

liblua_vortex_alive_11_la_SOURCES =	\
	lua-vortex-alive.c 

liblua_vortex_alive_11_include_HEADERS = \
	lua-vortex-alive.h 

liblua_vortex_alive_11_la_LDFLAGS = $(LUA_LIBS) $(top_srcdir)/src/libvortex-1.1.la $(top_srcdir)/alive/libvortex-alive-1.1.la $(AXL_LIBS) $(VORTEX_ALIVE_LIBS) -no-undefined liblua_vortex_11.la

all-local:

install-exec-hook:
	@echo "create site installation ${DESTDIR}${LUA_INSTALL_DIR} if it does not exists"
	test -d ${DESTDIR}${LUA_INSTALL_DIR} || install -d ${DESTDIR}${LUA_INSTALL_DIR}
	@echo "site installation ${DESTDIR}${LUA_INSTALL_DIR}/vortex.so"
	test -e ${DESTDIR}${LUA_INSTALL_DIR}/vortex.so || ln -s ${DESTDIR}${prefix}/lib/liblua_vortex_11.so ${DESTDIR}${LUA_INSTALL_DIR}/vortex.so
	test -d ${DESTDIR}${LUA_INSTALL_DIR}/vortex || mkdir ${DESTDIR}${LUA_INSTALL_DIR}/vortex
	test -e ${DESTDIR}${LUA_INSTALL_DIR}/vortex/sasl.so || ln -s ${DESTDIR}${prefix}/lib/liblua_vortex_sasl_11.so ${DESTDIR}${LUA_INSTALL_DIR}/vortex/sasl.so
	test -e ${DESTDIR}${LUA_INSTALL_DIR}/vortex/tls.so || ln -s ${DESTDIR}${prefix}/lib/liblua_vortex_tls_11.so ${DESTDIR}${LUA_INSTALL_DIR}/vortex/tls.so
	test -e ${DESTDIR}${LUA_INSTALL_DIR}/vortex/alive.so || ln -s ${DESTDIR}${prefix}/lib/liblua_vortex_alive_11.so ${DESTDIR}${LUA_INSTALL_DIR}/vortex/alive.so



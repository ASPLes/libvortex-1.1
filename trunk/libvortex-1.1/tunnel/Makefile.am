EXTRA_DIST = Makefile.win libvortex-tunnel-1.1.def

if ENABLE_TUNNEL_SUPPORT
INCLUDE_TUNNEL_SUPPORT=-DENABLE_TUNNEL_SUPPORT
endif

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

INCLUDES = -I. -I$(top_srcdir)/src -Wall -Wstrict-prototypes -ansi -g -I$(top_srcdir) -D__COMPILING_VORTEX__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(VORTEX_VERSION)"\" $(INCLUDE_TUNNEL_SUPPORT) \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libvortex_tunnel_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-tunnel-1.1.la

libvortex_tunnel_1_1_la_SOURCES = vortex_tunnel.c 
libvortex_tunnel_1_1_include_HEADERS = vortex_tunnel.h

libvortex_tunnel_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(top_builddir)/src/libvortex-1.1.la

libvortex_tunnel_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*'

libvortex-tunnel-1.1.def: update-def

update-def:
	echo "EXPORTS" > libvortex-tunnel-1.1.def
	cat .libs/libvortex-tunnel-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-tunnel-1.1.def
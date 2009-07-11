EXTRA_DIST = Makefile.win libvortex-pull-1.1.def

if ENABLE_PULL_SUPPORT
INCLUDE_PULL_SUPPORT=-DENABLE_PULL_SUPPORT
endif

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

INCLUDES = -I. -I$(top_srcdir)/src -Wall  -Wstrict-prototypes -ansi -g -I$(top_srcdir) -D__COMPILING_VORTEX__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(VORTEX_VERSION)"\" $(INCLUDE_PULL_SUPPORT) \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libvortex_pull_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-pull-1.1.la

libvortex_pull_1_1_la_SOURCES = vortex_pull.c 
libvortex_pull_1_1_include_HEADERS = vortex_pull.h

libvortex_pull_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(top_builddir)/src/libvortex-1.1.la

libvortex_pull_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*'

libvortex-pull-1.1.def: update-def

update-def:
	echo "EXPORTS" > libvortex-pull-1.1.def
	cat .libs/libvortex-pull-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-pull-1.1.def
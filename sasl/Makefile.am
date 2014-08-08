EXTRA_DIST = Makefile.win libvortex-sasl-1.1.def vortex-sasl.dtd.h

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

INCLUDES = $(compiler_options) -I. -I$(top_srcdir)/src -I$(top_srcdir) -D__COMPILING_VORTEX__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) $(SASL_CFLAGS) $(HAVE_GSASL_FREE) \
	-DVERSION=\""$(VORTEX_VERSION)"\" $(INCLUDE_SASL_SUPPORT) \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libvortex_sasl_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-sasl-1.1.la

libvortex_sasl_1_1_la_SOURCES = vortex_sasl.c 
libvortex_sasl_1_1_include_HEADERS = vortex_sasl.h vortex-sasl.dtd.h

libvortex_sasl_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(SASL_LIBS) $(top_builddir)/src/libvortex-1.1.la

libvortex_sasl_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*' 

libvortex-sasl-1.1.def: update-def

update-dtd:
	axl-knife --input ../data/sasl.dtd --dtd-to-c  --output vortex-sasl.dtd.h  --ifnewer

update-def:
	echo "EXPORTS" > libvortex-sasl-1.1.def
	cat .libs/libvortex-sasl-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-sasl-1.1.def
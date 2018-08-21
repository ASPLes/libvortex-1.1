EXTRA_DIST = Makefile.win libvortex-tls-1.1.def

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif
if VORTEX_HAVE_SSLv23_ENABLED
INCLUDE_VORTEX_HAVE_SSLv23_ENABLED=-DVORTEX_HAVE_SSLv23_ENABLED
endif
if VORTEX_HAVE_TLSv10_ENABLED
INCLUDE_VORTEX_HAVE_TLSv10_ENABLED=-DVORTEX_HAVE_TLSv10_ENABLED
endif
if VORTEX_HAVE_TLSv11_ENABLED
INCLUDE_VORTEX_HAVE_TLSv11_ENABLED=-DVORTEX_HAVE_TLSv11_ENABLED
endif
if VORTEX_HAVE_TLSv12_ENABLED
INCLUDE_VORTEX_HAVE_TLSv12_ENABLED=-DVORTEX_HAVE_TLSv12_ENABLED
endif
if VORTEX_HAVE_TLS_FLEXIBLE_ENABLED
INCLUDE_VORTEX_HAVE_TLS_FLEXIBLE_ENABLED=-DVORTEX_HAVE_TLS_FLEXIBLE_ENABLED
endif

bin_PROGRAMS    = vortex-digest-tool

INCLUDES = -I. -I$(top_srcdir)/src $(compiler_options) -I$(top_srcdir) -D__COMPILING_VORTEX__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) $(TLS_CFLAGS) $(INCLUDE_VORTEX_HAVE_SSLv23_ENABLED) $(INCLUDE_VORTEX_HAVE_TLSv10_ENABLED) $(INCLUDE_VORTEX_HAVE_TLSv11_ENABLED) $(INCLUDE_VORTEX_HAVE_TLSv12_ENABLED) $(INCLUDE_VORTEX_HAVE_TLS_FLEXIBLE_ENABLED) \
	-DVERSION=\""$(VORTEX_VERSION)"\" \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libvortex_tls_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-tls-1.1.la

libvortex_tls_1_1_la_SOURCES = vortex_tls.c 
libvortex_tls_1_1_include_HEADERS = vortex_tls.h vortex-tls.dtd.h

libvortex_tls_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(TLS_LIBS) $(top_builddir)/src/libvortex-1.1.la 

libvortex_tls_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*'

vortex_digest_tool_SOURCES          = vortex-digest-tool.c 
vortex_digest_tool_LDADD            = $(top_builddir)/src/libvortex-1.1.la libvortex-tls-1.1.la $(TLS_LIBS) $(AXL_LIBS) $(LIBS) 

libvortex-tls-1.1.def: update-def

update-dtd:
	axl-knife --input ../data/tls.dtd --dtd-to-c  --output vortex-tls.dtd.h  --ifnewer

update-def:
	echo "EXPORTS" > libvortex-tls-1.1.def
	cat .libs/libvortex-tls-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-tls-1.1.def

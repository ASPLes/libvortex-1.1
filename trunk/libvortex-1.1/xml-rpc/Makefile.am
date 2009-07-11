EXTRA_DIST = Makefile.win libvortex-xml-rpc-1.1.def xml-rpc-boot.dtd.h

if ENABLE_XML_RPC_SUPPORT
INCLUDE_XML_RPC_SUPPORT=-DENABLE_XML_RPC_SUPPORT
endif

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

INCLUDES = -I. -I$(top_srcdir)/src -Wall  -Wstrict-prototypes -ansi -g -I$(top_srcdir) -D__COMPILING_VORTEX__ -D__axl_disable_broken_bool_def__  \
	$(AXL_CFLAGS) $(INCLUDE_VORTEX_LOG) $(PTHREAD_CFLAGS) \
	-DVERSION=\""$(VORTEX_VERSION)"\" $(INCLUDE_XML_RPC_SUPPORT) \
	-DPACKAGE_DTD_DIR=\""$(datadir)"\" \
	-DPACKAGE_TOP_DIR=\""$(top_srcdir)"\" 

libvortex_xml_rpc_1_1_includedir = $(includedir)/vortex-1.1

lib_LTLIBRARIES = libvortex-xml-rpc-1.1.la

libvortex_xml_rpc_1_1_la_SOURCES = vortex_xml_rpc.c vortex_xml_rpc_types.c
libvortex_xml_rpc_1_1_include_HEADERS = vortex_xml_rpc.h vortex_xml_rpc_types.h

libvortex_xml_rpc_1_1_la_LIBADD = \
	$(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS) $(top_builddir)/src/libvortex-1.1.la

libvortex_xml_rpc_1_1_la_LDFLAGS = -no-undefined -export-symbols-regex '^(vortex|__vortex|_vortex).*'

libvortex-xml-rpc-1.1.def: update-def

update-dtds:
	axl-knife --input ../data/xml-rpc-boot.dtd --dtd-to-c  --output xml-rpc-boot.dtd.h  --ifnewer

update-def:
	echo "EXPORTS" > libvortex-xml-rpc-1.1.def
	cat .libs/libvortex-xml-rpc-1.1.exp | grep -v io_waiting_poll | grep -v io_waiting_epoll | grep -v __vortex >> libvortex-xml-rpc-1.1.def
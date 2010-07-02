if ENABLE_POLL_SUPPORT
INCLUDE_VORTEX_POLL=-DVORTEX_HAVE_POLL=1
endif

if ENABLE_EPOLL_SUPPORT
INCLUDE_VORTEX_EPOLL=-DVORTEX_HAVE_EPOLL=1
endif

if ENABLE_VORTEX_LOG
INCLUDE_VORTEX_LOG=-DENABLE_VORTEX_LOG
endif

if ENABLE_TUNNEL_SUPPORT
INCLUDE_TUNNEL_SUPPORT = -DENABLE_TUNNEL_SUPPORT
endif

if ENABLE_PULL_SUPPORT
INCLUDE_PULL_SUPPORT = -DENABLE_PULL_SUPPORT
endif

if ENABLE_HTTP_SUPPORT
INCLUDE_HTTP_SUPPORT = -DENABLE_HTTP_SUPPORT
endif

if ENABLE_XML_RPC_SUPPORT
INCLUDE_XML_RPC_SUPPORT = -DENABLE_XML_RPC_SUPPORT
XML_RPC_SUPPORT_LIBS    = $(top_builddir)/xml-rpc/libvortex-xml-rpc-1.1.la 
vortex_regression_listener_xml_rpc_files = service_dispatch.h \
	service_dispatch.c \
	test_sum_int_int.h \
	test_sum_int_int.c \
	test_operate_int_int.h \
	test_operate_int_int.c \
	test_get_the_string.h \
	test_get_the_string.c \
	test_get_the_bool_1.h \
	test_get_the_bool_1.c \
	test_get_the_bool_2.h \
	test_get_the_bool_2.c \
	test_get_double_sum_double_double.h \
	test_get_double_sum_double_double.c \
	test_struct_values_xml_rpc.h \
	test_struct_values_xml_rpc.c \
	test_get_struct_values_values.h \
	test_get_struct_values_values.c \
	test_struct_item_xml_rpc.h \
	test_struct_item_xml_rpc.c \
	test_array_itemarray_xml_rpc.h \
	test_array_itemarray_xml_rpc.c \
	test_get_array.h \
	test_get_array.c \
	test_get_list.h \
	test_get_list.c \
	test_struct_node_xml_rpc.h \
	test_struct_node_xml_rpc.c \
	test_sum2_int_int.h \
	test_sum2_int_int.c

vortex_regression_client_xml_rpc_files = test_types.h \
	test_xml_rpc.h \
	test_xml_rpc.c \
	test_struct_values_xml_rpc.h \
	test_struct_values_xml_rpc.c \
	test_struct_item_xml_rpc.h \
	test_struct_item_xml_rpc.c \
	test_array_itemarray_xml_rpc.h \
	test_array_itemarray_xml_rpc.c \
	test_struct_node_xml_rpc.h \
	test_struct_node_xml_rpc.c
endif

if ENABLE_SASL_SUPPORT
INCLUDE_SASL_SUPPORT = -DENABLE_SASL_SUPPORT -I$(top_srcdir)/sasl
SASL_SUPPORT_LIBS    = $(top_builddir)/sasl/libvortex-sasl-1.1.la 
SASL_TESTS           = vortex-sasl-listener
endif

if ENABLE_TLS_SUPPORT
INCLUDE_TLS_SUPPORT = -DENABLE_TLS_SUPPORT -I$(top_srcdir)/tls
TLS_SUPPORT_LIBS    = $(top_builddir)/tls/libvortex-tls-1.1.la 
TLS_TESTS           = vortex-tls-listener
endif

certfilesdir    = $(datadir)/libvortex-1.1
certfiles_DATA  = test-certificate.pem test-private-key.pem
EXTRA_DIST      = Makefile.win $(certfiles_DATA) vortex-regression-client.vcproj  vortex-regression-listener.vcproj vortex-regression-client.idl \
	mime.example.1.txt  mime.example.body.1.txt  mime.example.2.txt  mime.example.3.txt  mime.example.body.2.txt 
bin_PROGRAMS    = vortex-client 
noinst_PROGRAMS = \
	vortex-listener \
	vortex-simple-listener \
	vortex-simple-client \
	vortex-omr-server $(SASL_TESTS) $(TLS_TESTS) \
	vortex-xml-rpc-listener \
	vortex-client-connections \
	vortex-regression-client \
	vortex-regression-listener \
	vortex-server-without-cbs \
	vortex-client-without-cbs \
	vortex-close-listeners \
	vortex-file-transfer-server vortex-file-transfer-client \
	vortex-pull-listener

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/tunnel -I$(top_srcdir)/pull -I$(top_srcdir)/alive \
	-I$(top_srcdir)/xml-rpc -I$(top_srcdir)/http $(AXL_CFLAGS)  $(PTHREAD_CFLAGS) \
	-I$(READLINE_PATH)/include -Wall  -Wstrict-prototypes -g -ansi -D__axl_disable_broken_bool_def__   \
        -DVERSION=\""$(VORTEX_VERSION)"\" -I$(top_srcdir)/src $(INCLUDE_VORTEX_POLL) $(INCLUDE_VORTEX_EPOLL) $(INCLUDE_VORTEX_LOG) $(INCLUDE_TUNNEL_SUPPORT) $(INCLUDE_PULL_SUPPORT)  $(INCLUDE_XML_RPC_SUPPORT) $(INCLUDE_SASL_SUPPORT) $(INCLUDE_HTTP_SUPPORT) $(INCLUDE_TLS_SUPPORT)

LIBS            = $(AXL_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

bin_SCRIPTS = prepare_files

# copy files required to run regression test if they are not found (this is required to build
# vortex when sources and build directories are diferent).
vortex_test_files = $(top_srcdir)/test/mime.*.txt \
	     $(top_srcdir)/test/vortex-regression-client.c \
	     $(top_srcdir)/test/vortex-client.c \
	     $(top_srcdir)/test/vortex-regression-listener.c \
	     $(top_srcdir)/test/vortex-sasl-listener.c \
	     $(top_srcdir)/test/test-certificate.pem \
	     $(top_srcdir)/test/test-private-key.pem
prepare_files:
	test -e vortex-regression-client.c || cp $(vortex_test_files)  .

vortex_listener_SOURCES        = vortex-listener.c
vortex_listener_LDADD          = $(LIBS) $(top_builddir)/src/libvortex-1.1.la 

vortex_omr_server_SOURCES      = vortex-omr-server.c
vortex_omr_server_LDADD        = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

if ENABLE_TLS_SUPPORT
vortex_tls_listener_SOURCES    = vortex-tls-listener.c
vortex_tls_listener_LDADD      = $(LIBS) $(top_builddir)/src/libvortex-1.1.la $(TLS_SUPPORT_LIBS)
endif

vortex_client_SOURCES          = vortex-client.c 
vortex_client_LDADD            = $(LIBS) $(top_builddir)/src/libvortex-1.1.la -L$(READLINE_PATH)/lib -lreadline $(XML_RPC_SUPPORT_LIBS) $(SASL_SUPPORT_LIBS) $(TLS_SUPPORT_LIBS)

vortex_simple_listener_SOURCES = vortex-simple-listener.c
vortex_simple_listener_LDADD   = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_simple_client_SOURCES   = vortex-simple-client.c
vortex_simple_client_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_client_connections_SOURCES   = vortex-client-connections.c
vortex_client_connections_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la $(top_builddir)/tunnel/libvortex-tunnel-1.1.la 

vortex_regression_client_SOURCES   = \
	vortex-regression-client.c vortex-regression-common.c vortex-regression-common.h $(vortex_regression_client_xml_rpc_files)

vortex_regression_client_LDADD     = \
	$(LIBS) $(top_builddir)/src/libvortex-1.1.la  \
	$(top_builddir)/tunnel/libvortex-tunnel-1.1.la \
	$(top_builddir)/http/libvortex-http-1.1.la \
	$(top_builddir)/alive/libvortex-alive-1.1.la \
	$(top_builddir)/pull/libvortex-pull-1.1.la  $(XML_RPC_SUPPORT_LIBS) $(SASL_SUPPORT_LIBS) $(TLS_SUPPORT_LIBS) 

vortex_regression_listener_SOURCES   = \
	vortex-regression-listener.c vortex-regression-common.c vortex-regression-common.h $(vortex_regression_listener_xml_rpc_files) 

vortex_regression_listener_LDADD     = \
	$(LIBS) $(top_builddir)/src/libvortex-1.1.la \
	$(top_builddir)/tunnel/libvortex-tunnel-1.1.la \
	$(top_builddir)/pull/libvortex-pull-1.1.la \
	$(top_builddir)/alive/libvortex-alive-1.1.la \
	 $(XML_RPC_SUPPORT_LIBS) $(SASL_SUPPORT_LIBS) $(TLS_SUPPORT_LIBS)

vortex_server_without_cbs_SOURCES   = vortex-server-without-cbs.c
vortex_server_without_cbs_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_client_without_cbs_SOURCES   = vortex-client-without-cbs.c
vortex_client_without_cbs_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_close_listeners_SOURCES      = vortex-close-listeners.c
vortex_close_listeners_LDADD        = $(LIBS) $(top_builddir)/src/libvortex-1.1.la 

if ENABLE_SASL_SUPPORT
vortex_sasl_listener_SOURCES   = vortex-sasl-listener.c
vortex_sasl_listener_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la $(SASL_SUPPORT_LIBS) $(TLS_SUPPORT_LIBS)
endif

vortex_xml_rpc_listener_SOURCES   = vortex-xml-rpc-listener.c
vortex_xml_rpc_listener_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la $(XML_RPC_SUPPORT_LIBS)

vortex_file_transfer_server_SOURCES   = vortex-file-transfer-server.c
vortex_file_transfer_server_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_file_transfer_client_SOURCES   = vortex-file-transfer-client.c
vortex_file_transfer_client_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la

vortex_pull_listener_SOURCES   = vortex-pull-listener.c
vortex_pull_listener_LDADD     = $(LIBS) $(top_builddir)/src/libvortex-1.1.la $(top_builddir)/pull/libvortex-pull-1.1.la

# build xml-rpc source files required by the regression test.
test_types.h:
	$(top_builddir)/xml-rpc-gen/xml-rpc-gen-1.1 --out-server-dir . --out-stub-dir .  vortex-regression-client.idl --disable-autoconf --disable-main-file
if ENABLE_AF_ARCH_DOC
VORTEX_DOC_DIR = doc
endif

if ENABLE_READLINE 
VORTEX_TEST_DIR = test
endif

if ENABLE_XML_RPC_GEN_TOOL
VORTEX_XML_RPC_GEN_TOOL_DIR = xml-rpc-gen
endif

if ENABLE_TUNNEL_SUPPORT
TUNNEL_SUPPORT_DIR = tunnel
TUNNEL_PKGCONFIG = vortex-tunnel-1.1.pc
endif

if ENABLE_PULL_SUPPORT
PULL_SUPPORT_DIR = pull
PULL_PKGCONFIG = vortex-pull-1.1.pc
endif

if ENABLE_XML_RPC_SUPPORT
XML_RPC_SUPPORT_DIR = xml-rpc
XML_RPC_PKGCONFIG = vortex-xml-rpc-1.1.pc
endif

if ENABLE_SASL_SUPPORT
SASL_SUPPORT_DIR = sasl
SASL_PKGCONFIG = vortex-sasl-1.1.pc
endif

if ENABLE_TLS_SUPPORT
TLS_SUPPORT_DIR = tls
TLS_PKGCONFIG = vortex-tls-1.1.pc
endif

if ENABLE_HTTP_SUPPORT
HTTP_SUPPORT_DIR = http
HTTP_PKGCONFIG = vortex-http-1.1.pc
endif

if ENABLE_PY_VORTEX_SUPPORT
PY_VORTEX_DIR = py-vortex
endif

if ENABLE_ALIVE_SUPPORT
ALIVE_SUPPORT_DIR = alive
ALIVE_PKGCONFIG = vortex-alive-1.1.pc
endif


SUBDIRS = src $(PULL_SUPPORT_DIR) $(TUNNEL_SUPPORT_DIR) $(XML_RPC_SUPPORT_DIR) $(SASL_SUPPORT_DIR) $(TLS_SUPPORT_DIR) $(HTTP_SUPPORT_DIR) $(ALIVE_SUPPORT_DIR) $(PY_VORTEX_DIR) $(VORTEX_TEST_DIR) $(VORTEX_XML_RPC_GEN_TOOL_DIR) $(VORTEX_DOC_DIR) data logo
EXTRA_DIST = vortex-1.1.pc.in VERSION libvortex.sln get-python-config Makefile.win Makefile.win.in
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vortex-1.1.pc $(PULL_PKGCONFIG) $(TUNNEL_PKGCONFIG) $(XML_RPC_PKGCONFIG) $(SASL_PKGCONFIG) $(TLS_PKGCONFIG) $(HTTP_PKGCONFIG) $(ALIVE_PKGCONFIG)

update-defs:
	cd src; make update-def; cd ..
	cd tls; make update-def; cd ..
	cd sasl; make update-def; cd ..
	cd tunnel; make update-def; cd ..
	cd xml-rpc; make update-def; cd ..
	cd pull; make update-def; cd ..
	cd alive; make update-def; cd ..

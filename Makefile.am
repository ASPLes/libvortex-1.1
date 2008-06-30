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
endif

if ENABLE_XML_RPC_SUPPORT
XML_RPC_SUPPORT_DIR = xml-rpc
endif

if ENABLE_SASL_SUPPORT
SASL_SUPPORT_DIR = sasl
endif


SUBDIRS = src $(TUNNEL_SUPPORT_DIR) $(XML_RPC_SUPPORT_DIR) $(SASL_SUPPORT_DIR) $(VORTEX_TEST_DIR) $(VORTEX_XML_RPC_GEN_TOOL_DIR) $(VORTEX_DOC_DIR) data logo
EXTRA_DIST = vortex-1.1.pc.in VERSION libvortex.sln
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vortex-1.1.pc vortex-tunnel-1.1.pc vortex-xml-rpc-1.1.pc

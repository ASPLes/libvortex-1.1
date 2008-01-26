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


SUBDIRS = src $(TUNNEL_SUPPORT_DIR) $(VORTEX_TEST_DIR) $(VORTEX_XML_RPC_GEN_TOOL_DIR) $(VORTEX_DOC_DIR) data logo
EXTRA_DIST = vortex-1.1.pc.in VERSION libvortex.sln
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vortex-1.1.pc vortex-tunnel-1.1.pc

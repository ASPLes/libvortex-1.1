EXTRA_DIST = vortex-doc.doxygen \
	     vortex-status.txt \
	     vortex-doc-footer.html \
	     vortex-doc-header.html \
	     main-page-logo.png \
	     xml-rpc-invocation.png \
	     vortex.css mime-structure.png af-arch.idl change-resource.idl \
	     body_back.gif body-include.idl body-include-file.idl \
	     vortex-components.png aspl-logo-header.png
bin_SCRIPTS = initial_build_doc

help:
	@echo 
	@echo " Documentation not build!:"
	@echo " ~~~~~~~~~~~~~~~~~~~~~~~~~"
	@echo " --> Doxygen documentation wasn't built because an stamp was detected: doc.stamp"
	@echo " --> To regenerate or force doc building type make build_doc or remove doc.stamp";
	@echo
	@echo " Note:"
	@echo " --> To build xml-rpc-gen-draft document you need xml2rpc and the execute:"
	@echo " --> make xml_rpc_gen_draft"
	@echo

initial_build_doc:
	@echo "checking documenation stamp..";
	@test -e doc.stamp & make help
	@test -e doc.stamp || make build_doc
	@test -e doc.stamp || touch doc.stamp

build_doc:
	@echo "Generating module documentation..";
	(cd $(top_builddir)/xml-rpc-gen; ./xml-rpc-gen-1.1 $(top_srcdir)/xml-rpc-gen/reg-test01.idl;)
	doxygen vortex-doc.doxygen
	cp $(top_srcdir)/doc/main-page-logo.png html
	cp $(top_srcdir)/doc/aspl-logo-header.png html
	cp $(top_srcdir)/doc/vortex.css html
	cp $(top_srcdir)/doc/body_back.gif html

xml_rpc_gen_draft:
	xml2rfc xml-rpc-gen-doc.xml && a2ps -2 xml-rpc-gen-doc.txt -o draft-xdl-brosnan.ps
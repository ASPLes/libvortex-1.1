EXTRA_DIST = vortex-regression-client.py vortex-regression-listener.py test.crt test.key

all: prepare
	@echo "To run regression tests do the following steps:"
	@echo " - ./vortex-regression-listener.py"
	@echo "   <in other terminal>"
	@echo " - ./vortex-regression-client.py"

run-tests: prepare check

prepare:
	@cd ../src; make; cd -
	@test -d vortex || mkdir vortex
        # copy dynamic libraries

	@cp `find ../src -name libpy_vortex_11$(SHARED_EXTENSION) -type l` vortex/
if ENABLE_SASL_SUPPORT
	@cp `find ../src -name libpy_vortex_sasl_11$(SHARED_EXTENSION) -type l` vortex/
endif
if ENABLE_TLS_SUPPORT
	@cp `find ../src -name libpy_vortex_tls_11$(SHARED_EXTENSION) -type l` vortex/
endif
        # copy python imports
	@test -e vortex/__init__.py || cp ../src/__init__.py vortex/
	@diff vortex/__init__.py ../src/__init__.py || cp ../src/__init__.py vortex/
	@test -e vortex/sasl.py     || cp ../src/sasl.py vortex/
	@diff vortex/sasl.py ../src/sasl.py || cp ../src/sasl.py vortex/
	@test -e vortex/tls.py      || cp ../src/tls.py vortex/
	@diff vortex/tls.py ../src/tls.py || cp ../src/tls.py vortex/
check:
	python vortex-regression-client.py


all-local:
        # prepare content from src dir
	cd ..; cd src; make; cd ../test
	test -e vortex.so || ln -s ../src/.libs/liblua_vortex_11.so vortex.so
	test -d vortex || mkdir vortex
	cp ../src/.libs/liblua_vortex_sasl_11.so.0.0.0 vortex/sasl.so
	cp ../src/.libs/liblua_vortex_tls_11.so.0.0.0 vortex/tls.so
	cp ../src/.libs/liblua_vortex_alive_11.so.0.0.0 vortex/alive.so




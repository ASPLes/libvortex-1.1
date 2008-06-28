dtddir = $(datadir)/libvortex-1.1
dtd_DATA =  xml-rpc.dtd

EXTRA_DIST = $(dtd_DATA) \
	\ # .idl files
	reg-test08.idl reg-test02.idl reg-test06.idl reg-test07.idl \
	reg-test01.idl reg-test05.idl reg-test04.idl reg-test03.idl \
	test01.idl \
	\ # .xdl files
	reg-test06.xdl reg-test07.xdl reg-test08.xdl reg-test05.xdl \
	reg-test01.xdl reg-test04.xdl reg-test03.xdl reg-test02.xdl \
	test01.xdl \
	\ # .c files
	reg-test03.c reg-test01a.c reg-test01.c reg-test02.c \
	reg-test04.c reg-test05.c reg-test06.c reg-test07.c  _support.c 

INCLUDES = -g -Wall -ansi -I. -I$(top_srcdir)/libvortex-1.1/src $(AXL_CFLAGS) $(EXARG_CFLAGS) $(PTHREAD_CFLAGS) -DVERSION=\""$(VORTEX_VERSION)"\" -DINSTALL_DIR=\"$(datadir)/libvortex-1.1\" -I../src

LIBS     = $(AXL_LIBS) $(EXARG_LIBS) $(PTHREAD_LIBS) $(ADDITIONAL_LIBS)

bin_PROGRAMS = xml-rpc-gen-1.1

# tests: reg-test01 reg-test02 reg-test03

# reg-test%: _support.o xml-rpc-gen
#	@echo "Bulding Xml data interface for: $@.."
#	./xml-rpc-gen ./$@.xdl
#	(cd out/client-test; ./autogen.sh)
#	(cd out/client-test; make)
#	(cd out/server-test; ./autogen.sh)
#	(cd out/server-test; make)
#	@echo "Compiling: $@.."
#	libtool --mode=compile gcc $(INCLUDES) -Iout/client-test -c -o $@.o $@.c
#	libtool --mode=link gcc $(LIBRARIES) -o $@ $@.o _support.o ../src/libvortex.la out/client-test/libtest_xml_rpc.la
#	@echo "Running $@ server.."
#	@(cd out/server-test; libtool --mode=execute ./server-test &)
#	@echo "Waiting for $@ to start.."
#	@sleep 3
#	@echo "Running client test for: $@.."
#	./$@
#	@echo "Stopping the $@ server.."
#	@killall server-test

_support.o: _support.h _support.c
	@echo "Building test support infrastructure.."
	libtool --mode=compile gcc $(INCLUDES) -I. -c -o _support.o _support.c

clean_tests:
	rm -f _support.o reg-test01

# copy exarg dependency to not force people to download a library with
# two files
exarg.h exarg.c: 
	test -e exarg.h || cp ../../libexarg/src/exarg.h .
	test -e exarg.c || cp ../../libexarg/src/exarg.c .


xml_rpc_gen_1_1_SOURCES = exarg.h exarg.c xml-rpc-gen.c xml-rpc.h \
	xml-rpc-support.h xml-rpc-support.c \
	xml-rpc-c-stub.h xml-rpc-c-stub.c \
	xml-rpc-autoconf.h xml-rpc-autoconf.c \
	xml-rpc-c-server.h xml-rpc-c-server.c \
	xml-rpc-gen-translate.h xml-rpc-gen-translate.c

xml_rpc_gen_1_1_LDADD = ../src/libvortex-1.1.la
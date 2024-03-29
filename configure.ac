dnl  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
dnl  Copyright (C) 2022 Advanced Software Production Line, S.L.
dnl
dnl  This program is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public License
dnl  as published by the Free Software Foundation; either version 2.1 of
dnl  the License, or (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of 
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
dnl  GNU Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with this program; if not, write to the Free
dnl  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl  
dnl  You may find a copy of the license under this software is released
dnl  at COPYING file. This is LGPL software: you are wellcome to
dnl  develop propietary applications using this library withtout any
dnl  royalty or fee but returning back any change, improvement or
dnl  addition in the form of source code, project image, documentation
dnl  patches, etc. 
dnl
dnl  For comercial support on build BEEP enabled solutions contact us:
dnl          
dnl      Postal address:
dnl         Advanced Software Production Line, S.L.
dnl         C/ Antonio Suarez N�10, Edificio Alius A, Despacho 102
dnl         Alcal� de Henares 28802 Madrid
dnl         Spain
dnl
dnl      Email address:
dnl         info@aspl.es - http://www.aspl.es/vortex
dnl

AC_INIT(src/vortex.h)

configure_dir=`dirname "$0"`
VORTEX_VERSION=`cat $configure_dir/VERSION`
AC_SUBST(VORTEX_VERSION)

axl_string_version=axl-`axl-knife --version`
AC_SUBST(axl_string_version)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(vortex, $VORTEX_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

# compiler option
compiler_options=""
STRICT_PROTOTYPES=""
if test "$compiler" = "gcc" ; then
   compiler_options="-Wstrict-prototypes -Wall -Werror -g -fstack-protector-all -Wstack-protector"
   echo "Detected gcc compiler: $compiler, adding options: $compiler_options"
fi
AC_SUBST(compiler_options)
# ansi option
ansi_option=""
if test "$compiler" = "gcc" ; then
    ansi_option="-ansi"
    echo "Detected gcc compiler: $compiler, adding options: $ansi_option"
fi
AC_SUBST(ansi_option)

AC_CHECK_PROG(PKG_CONFIG, pkg-config, "yes", "no")
if test "$PKG_CONFIG" = "no" ; then
  AC_MSG_ERROR([You need to install pkg-config to compile Vortex 1.1 options. See: http://pkgconfig.freedesktop.org/releases/])
fi

dnl
dnl Thread detection support mostly taken from the apache project 2.2.3.
dnl
dnl VORTEX_PTHREADS_TRY_RUN(actions-if-success)
dnl
dnl Try running a program which uses pthreads, executing the
dnl actions-if-success commands on success.
dnl
AC_DEFUN([VORTEX_PTHREADS_TRY_RUN], [
AC_TRY_RUN( [
#include <pthread.h>
#include <stddef.h>

void *thread_routine(void *data) {
    return data;
}

int main() {
    pthread_t thd;
    pthread_mutexattr_t mattr;
    pthread_once_t once_init = PTHREAD_ONCE_INIT;
    int data = 1;
    pthread_mutexattr_init(&mattr);
    return pthread_create(&thd, NULL, thread_routine, &data);
} ], [vortex_p_t_r=yes], [vortex_p_t_r=no], [vortex_p_t_r=no])

if test $vortex_p_t_r = yes; then
  $1
fi

])dnl


dnl
dnl VORTEX_PTHREADS_CHECK()
dnl
dnl Try to find a way to enable POSIX threads.  Sets the 
dnl pthreads_working variable to "yes" on success.
dnl
AC_DEFUN([VORTEX_PTHREADS_CHECK],[

AC_CACHE_CHECK([for CFLAGS needed for pthreads], [vortex_cv_pthreads_cflags],
[vortex_ptc_cflags=$CFLAGS
 for flag in -kthread -pthread -pthreads -mt -mthreads -Kthread -threads; do 
    CFLAGS=$vortex_ptc_cflags
    test "x$flag" != "xnone" && CFLAGS="$CFLAGS $flag"
    VORTEX_PTHREADS_TRY_RUN([
      vortex_cv_pthreads_cflags="$flag"
      break
    ])
 done
 CFLAGS=$vortex_ptc_cflags
])

# The CFLAGS may or may not be sufficient to ensure that libvortex
# depends on the pthreads library: some versions of libtool
# drop -pthread when passed on the link line; some versions of
# gcc ignore -pthread when linking a shared object.  So always
# try and add the relevant library to LIBS too.

AC_CACHE_CHECK([for LIBS needed for pthreads], [vortex_cv_pthreads_lib], [
  vortex_ptc_libs=$LIBS
  for lib in -lpthread -lpthreads -lc_r; do
    LIBS="$vortex_ptc_libs $lib"
    VORTEX_PTHREADS_TRY_RUN([
      vortex_cv_pthreads_lib=$lib
      break
    ])
  done
  LIBS=$vortex_ptc_libs
])

if test "$pthreads_working" = "yes"; then
  threads_result="POSIX Threads found"
else
  threads_result="POSIX Threads not found"
fi
])dnl

dnl call to detect thread activation support
VORTEX_PTHREADS_CHECK

PTHREAD_CFLAGS="$vortex_cv_pthreads_cflags"
PTHREAD_LIBS="$vortex_cv_pthreads_lib"
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

dnl check for poll support
AC_CHECK_HEADER(sys/poll.h, enable_poll=yes, enable_poll=no)
AM_CONDITIONAL(ENABLE_POLL_SUPPORT, test "x$enable_poll" = "xyes")

dnl Check for the Linux epoll interface; epoll* may be available in libc
dnl with Linux kernels 2.6.X
AC_CACHE_CHECK([for epoll(2) support], [enable_cv_epoll],
[AC_TRY_RUN([
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    return epoll_create(5) == -1;
}], [enable_cv_epoll=yes], [enable_cv_epoll=no], [enable_cv_epoll=no])])
AM_CONDITIONAL(ENABLE_EPOLL_SUPPORT, test "x$enable_cv_epoll" = "xyes")

dnl select the best I/O platform
if test x$enable_cv_epoll = xyes ; then
   default_platform="epoll"
elif test x$enable_poll = xyes ; then
   default_platform="poll"
else 
   default_platform="select"
fi
AM_CONDITIONAL(DEFAULT_EPOLL, test "x$default_platform" = "xepoll")
AM_CONDITIONAL(DEFAULT_POLL, test "x$default_platform" = "xpoll")

dnl check for vortex client tool dependencies
AC_ARG_ENABLE(vortex-client, [  --enable-vortex-client    Enable vortex-client tool building [default=yes]], enable_vortex_client="$enableval", enable_vortex_client=yes)
if test x$enable_vortex_client = xyes ; then
AC_CHECK_HEADER(readline/readline.h, readline_found=true, readline_found=false)
     AC_TRY_LINK(,,[	READLINE_DEPLIBS="-lreadline" ],[ 
		LIBS="$LIBS -ltermcap" 
		AC_TRY_LINK(,,[ READLINE_DEPLIBS="-lreadline -ltermcap" ],[
			AC_MSG_ERROR([Cannot figure out how to link with the readline library; see config.log for more information])
		])
	])
fi
AM_CONDITIONAL(ENABLE_READLINE, test "x$readline_found" = "xtrue")

AC_ARG_VAR(READLINE_PATH, [Readline base include path. This can be helpful to set a different location for a readline installation.
This will be used to build a include path as READLINE_PATH/include and a READLINE_PATH/lib.
])

dnl check for doxygen documentation
AC_ARG_ENABLE(vortex-doc, [  --enable-vortex-doc       Enable building Vortex Library documentation (doxygen required) [default=yes]], 
	      enable_vortex_doc="$enableval", 
	      enable_vortex_doc=yes)
if test x$enable_vortex_doc = xyes ; then
        AC_CHECK_PROG(DOXYGEN, doxygen, "yes", "no")
fi
AM_CONDITIONAL(ENABLE_AF_ARCH_DOC, test "x$DOXYGEN" = "xyes")

dnl LibAxl library support.
PKG_CHECK_MODULES(AXL, axl >= 0.7.0, [enable_axl_support=yes], [enable_axl_support=no])
if test "$enable_axl_support" = "no" ; then
   dnl Try to find by header
   AC_CHECK_HEADER(axl/axl.h,enable_axl_support=yes,enable_axl_support=no)
   if test "$enable_axl_support" = "yes" ; then
       AXL_CFLAGS=
       AXL_LIBS=-laxl
   else
       dnl Try to find files at /usr/local/lib/pkgconfig/axl.pc
       if test -f /usr/local/lib/pkgconfig/axl.pc; then
       	  dnl alternative location found, try to tell the system to use this location
	  export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
	  PKG_CHECK_MODULES(AXL, axl >= 0.7.0, [enable_axl_support=yes], [enable_axl_support=no])
       fi
       if test "$enable_axl_support" = "no" ; then
       	  AC_MSG_ERROR([ERROR: LibAxl wasn't found installed on this system. Unable to compile Vortex without it.])
       fi
   fi
fi
AXL_CFLAGS=`echo $AXL_CFLAGS | sed 's@ \/@/@g'`
AXL_LIBS=`echo $AXL_LIBS | sed 's@ \/@/@g'`
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)

dnl check for console log
AC_ARG_ENABLE(vortex-log, [  --enable-vortex-log       Enable building Vortex Library console debug log support [default=yes]], 
	      enable_vortex_log="$enableval", 
	      enable_vortex_log=yes)
AM_CONDITIONAL(ENABLE_VORTEX_LOG, test "x$enable_vortex_log" = "xyes")

dnl check for tls building
AC_ARG_ENABLE(tls-support, [  --disable-tls-support     Makes buidling Vortex Library TLS support (OpenSSL required)], 
	      enable_tls_support="$enableval", 
	      enable_tls_support=yes)
if test x$enable_tls_support = xyes ; then
   dnl check header needed
   AC_CHECK_HEADER(openssl/ssl.h,,enable_tls_support=no)
   if test x$enable_tls_support = xno ; then
      AC_MSG_WARN([Cannot OpenSSL installation, disabling it. This will disable TLS profile support. ]) 
      enable_tls_support=no
   else
      dnl configure additional TLS flags
      TLS_LIBS="-lssl -lcrypto"
      AC_SUBST(TLS_LIBS)
   fi

   # OLD TLS Flex method: (before OpenSSL 1.1.0)
   #
   # https://www.openssl.org/docs/man1.0.2/man3/SSLv23_client_method.html
   #
   # SSLv23_method(), SSLv23_server_method(), SSLv23_client_method()
   # These are the general-purpose version-flexible SSL/TLS
   # methods. The actual protocol version used will be negotiated to
   # the highest version mutually supported by the client and the
   # server. The supported protocols are SSLv2, SSLv3, TLSv1, TLSv1.1
   # and TLSv1.2. Most applications should use these method, and avoid
   # the version specific methods described below.
   AC_CHECK_LIB(ssl,SSLv23_method,  ssl_sslv23_supported=yes, ssl_sslv23_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_SSLv23_ENABLED, test "x$ssl_sslv23_supported" = "xyes")

   # Support for SSLv3 
   AC_CHECK_LIB(ssl,SSLv3_method,  ssl_sslv3_supported=yes, ssl_sslv3_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_SSLv3_ENABLED, test "x$ssl_sslv3_supported" = "xyes")

   # TLSv1.0 as defined in the old days
   AC_CHECK_LIB(ssl,TLSv1_0_method, ssl_tlsv10_supported=yes,  ssl_tlsv10_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_TLSv10_ENABLED, test "x$ssl_tlsv10_supported" = "xyes")
   # TLSv1.0 as defined nowdays
   AC_CHECK_LIB(ssl,TLSv1_method, ssl_tlsv1_supported=yes,  ssl_tlsv1_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_TLSv1_ENABLED, test "x$ssl_tlsv1_supported" = "xyes")

   AC_CHECK_LIB(ssl,TLSv1_1_method, ssl_tlsv11_supported=yes, ssl_tlsv11_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_TLSv11_ENABLED, test "x$ssl_tlsv11_supported" = "xyes")

   AC_CHECK_LIB(ssl,TLSv1_2_method, ssl_tlsv12_supported=yes, ssl_tlsv12_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_TLSv12_ENABLED, test "x$ssl_tlsv12_supported" = "xyes")

   # NEW TLS Flex method:
   #
   # https://www.openssl.org/docs/manmaster/man3/SSL_CTX_new.html
   # 
   # SSLv23_method(), SSLv23_server_method() and
   # SSLv23_client_method() were deprecated and the preferred
   # TLS_method(), TLS_server_method() and TLS_client_method()
   # functions were added in OpenSSL 1.1.0.
   AC_CHECK_LIB(ssl,TLS_client_method, ssl_tls_flexible_supported=yes, ssl_tls_flexible_supported=no)
   AM_CONDITIONAL(VORTEX_HAVE_TLS_FLEXIBLE_ENABLED, test "x$ssl_tls_flexible_supported" = "xyes")

fi
AM_CONDITIONAL(ENABLE_TLS_SUPPORT, test "x$enable_tls_support" = "xyes")

dnl check for sasl building through GSASL
AC_ARG_ENABLE(sasl-support, [  --disable-sasl-support    Makes Vortex Library to be built without SASL support], 
	      enable_sasl_support="$enableval", 
	      enable_sasl_support=yes)
if test "$enable_sasl_support" != "no" ; then
    PKG_CHECK_MODULES(GSASL, libgsasl >= 0.2.21,	[enable_sasl_support=yes], [enable_sasl_support=no])
    if test "$enable_sasl_support" = "no" ; then
        PKG_CHECK_MODULES(GSASL, libgsasl >= 0.2.12,	[enable_sasl_support=yes], [enable_sasl_support=no])
	if test "$enable_sasl_support" = "yes" ; then
	     AC_MSG_WARN([Found old GSASL installation, this could cause problems.])
	     found_old_sasl=yes
        else 
	     dnl try to find headers manually
	     AC_CHECK_HEADER(gsasl.h,enable_sasl_support=yes,enable_sasl_support=no)
	     if test "$enable_sasl_support" = "yes" ; then

	     	# flag that we have gsasl
		HAVE_GSASL_FREE=-DGSASL_HAVE_FREE

		dnl Enable SASL cflags and link headers
		SASL_LIBS=-lgsasl
		SASL_CFLAGS=
		dnl SASL libraries subsitution
		AC_SUBST(SASL_CFLAGS)
		AC_SUBST(SASL_LIBS)
		AC_SUBST(HAVE_GSASL_FREE)
	     	
	     else
	        AC_MSG_WARN([Cannot find GNU SASL installaton, disabling it. This will disable SASL family profile support.])
		enable_sasl_support=no
             fi
        fi
    else
	# flag that we have gsasl
	HAVE_GSASL_FREE=-DGSASL_HAVE_FREE

	dnl Enable SASL cflags and link headers
	SASL_LIBS=`pkg-config --libs libgsasl`
   	SASL_CFLAGS=`pkg-config --cflags libgsasl`

	dnl cleanup declarations (FreeBSD)
	SASL_CFLAGS=`echo $SASL_CFLAGS | sed 's@ \/@/@g'`
	SASL_LIBS=`echo $SASL_LIBS | sed 's@ \/@/@g'`

	dnl SASL libraries subsitution
	AC_SUBST(SASL_CFLAGS)
	AC_SUBST(SASL_LIBS)
	AC_SUBST(HAVE_GSASL_FREE)
    fi
fi

AM_CONDITIONAL(ENABLE_SASL_SUPPORT, test "x$enable_sasl_support" = "xyes")

dnl check for websocket support (through noPoll)
AC_ARG_ENABLE(websocket-support, [  --disable-websocket-support    Makes Vortex Library to be built with WebSocket extension library], 
	      enable_websocket_support="$enableval", 
	      enable_websocket_support=yes)

dnl If websocket is requested/enabled, try to detect it
if test "$enable_websocket_support" != "no" ; then

    PKG_CHECK_MODULES(NOPOLL, nopoll,	[enable_websocket_support=yes], [enable_websocket_support=no])
    if test "$enable_websocket_support" = "no" ; then
       	dnl try to check by header
	AC_CHECK_HEADER([nopoll/nopoll.h],enable_websocket_support=yes,enable_websocket_support=no)
	AC_MSG_RESULT([Headers detected for noPoll: $enable_websocket_support])
	if test "$enable_websocket_support" = "yes" ; then
	   NOPOLL_CFLAGS=
	   NOPOLL_LIBS=-lnopoll
	fi
    fi

    NOPOLL_CFLAGS=`echo $NOPOLL_CFLAGS | sed 's@ \/@/@g'`
    NOPOLL_LIBS=`echo $NOPOLL_LIBS | sed 's@ \/@/@g'`
    AC_SUBST(NOPOLL_CFLAGS)
    AC_SUBST(NOPOLL_LIBS)
fi
AM_CONDITIONAL(ENABLE_WEBSOCKET_SUPPORT, test "x$enable_websocket_support" = "xyes")

dnl check for XML-RPC RFC3529 support
AC_ARG_ENABLE(xml-rpc-support, [  --disable-xml-rpc-support    Makes Vortex Library to be built without XML-RPC (RFC3529) support], 
	      enable_xml_rpc_support="$enableval", 
	      enable_xml_rpc_support=yes)
AM_CONDITIONAL(ENABLE_XML_RPC_SUPPORT, test "x$enable_xml_rpc_support" = "xyes")

dnl check for XML-RPC RFC3529 support
AC_ARG_ENABLE(xml-rpc-support-gen, [  --disable-xml-rpc-gen-tool    Makes Vortex Library to be built without xml-rpc-gen tool], 
	      enable_xml_rpc_gen_tool="$enableval", 
	      enable_xml_rpc_gen_tool=yes)
AM_CONDITIONAL(ENABLE_XML_RPC_GEN_TOOL, test "x$enable_xml_rpc_gen_tool" = "xyes")

dnl check for TUNNEL RFC3620 support (BEEP general application proxy support)
AC_ARG_ENABLE(tunnel-support, [  --disable-tunnel-support    Makes Vortex Library to be built without TUNNEL (RFC3620) profile support], 
	      enable_tunnel_support="$enableval", 
	      enable_tunnel_support=yes)
AM_CONDITIONAL(ENABLE_TUNNEL_SUPPORT, test "x$enable_tunnel_support" = "xyes")

dnl check for PULL API support (Single threaded pull API)
AC_ARG_ENABLE(pull-support, [  --disable-pull-support    Makes Vortex Library to be built without PULL API support], 
	      enable_pull_support="$enableval", 
	      enable_pull_support=yes)
AM_CONDITIONAL(ENABLE_PULL_SUPPORT, test "x$enable_pull_support" = "xyes")

dnl check for HTTP API support (BEEP connection through HTTP/1.1 proxy)
AC_ARG_ENABLE(http-support, [  --disable-http-support    Makes Vortex Library to be built without HTTP CONNECT proxy support], 
	      enable_http_support="$enableval", 
	      enable_http_support=yes)
AM_CONDITIONAL(ENABLE_HTTP_SUPPORT, test "x$enable_http_support" = "xyes")

dnl check for ALIVE API support (BEEP connection check)
AC_ARG_ENABLE(alive-support, [  --disable-alive-support    Makes Vortex Library to be built without ALIVE profile support], 
	      enable_alive_support="$enableval", 
	      enable_alive_support=yes)
AM_CONDITIONAL(ENABLE_ALIVE_SUPPORT, test "x$enable_alive_support" = "xyes")

dnl check for py-vortex support
AC_ARG_ENABLE(py-vortex,   [   --disable-py-vortex       Makes Vortex Library to be build without PyVortex: python bindings support],
              enable_py_vortex_support="$enableval",
	      enable_py_vortex_support=yes)
AM_PATH_PYTHON([2.3],, enable_py_vortex_support="no")
if test "$enable_py_vortex_support" = "yes" ; then

   # find python-config
   if test -f "/bin/bash" ; then
      PYTHON_CONFIG=`./get-python-config`
   else
      bash_location=`which bash`
      echo "Bash location found: $bash_location"
      PYTHON_CONFIG=`${bash_location} ./get-python-config`
   fi 

   echo "Python pythondir is: ${pythondir}"
   echo "Python found: ${PYTHON}"

   # get site_packages
   # AC_SUBST(python_dir_check, m4_esyscmd([./check-python-dir.py $pythondir]))
   if test "${PYTHON}" = "/usr/bin/python" ; then
      python_dir_check=`./check-python-dir.py ${pythondir}`
   else
      if test "${PYTHON}" = "/usr/bin/python2" ; then
         python_dir_check=`/usr/bin/python2 ./check-python-dir.py ${pythondir}`
      else
        if test "${PYTHON}" = "/usr/bin/python2.7" ; then
           python_dir_check=`/usr/bin/python2.7 ./check-python-dir.py ${pythondir}`
        else
           python_dir_check=`python ./check-python-dir.py ${pythondir}`
        fi
      fi
   fi     

   # get python includes 
   PYTHON_INCLUDES=`$PYTHON_CONFIG --includes 2>/dev/null`
   old_CPPFLAGS=$CPPFLAGS
   CPPFLAGS="$PYTHON_INCLUDES"
   echo "Python includes: $PYTHON_INCLUDES"
   AC_SUBST(PYTHON_INCLUDES)

   AC_TRY_COMPILE([#include <Python.h>],
          [Py_InitModule3(NULL, NULL, NULL);],
          enable_py_vortex_support="yes",
          enable_py_vortex_support="no")
   AC_MSG_RESULT([Python development headers status: $enable_py_vortex_support])

   if test "$enable_py_vortex_support" = "yes" ; then

      # get python flags and libs
      PYTHON_CFLAGS=`$PYTHON_CONFIG --cflags 2>/dev/null | sed -r 's/\-arch (\d|\w)+ //g'`
      PYTHON_LIBS=`$PYTHON_CONFIG --ldflags 2>/dev/null`
      AC_SUBST(PYTHON_CFLAGS)
      AC_SUBST(PYTHON_LIBS)

      dnl get site dinamic library extension
      SHARED_EXTENSION=$shrext_cmds
      AC_SUBST(SHARED_EXTENSION)

      # check python_install_dir and pythondir maches
      python_dir_temp=`eval echo ${pythondir}`
      # check python_install_dir and pythondir maches
      if test "$python_dir_check" != "ok" ; then
          AC_MSG_ERROR([ERROR: pythondir variable is poiting to an unusable location: $python_dir_temp. Error detected: $python_dir_check. Fix your autoconf install or disable python building, for that, pass --disable-py-vortex to configure. This usually indicates a problem with automake package which provides the _AM_PATH_PYTHON_ macro.])
      fi
      echo "Python install dir OK: $python_dir_temp"
   fi
fi
AM_CONDITIONAL(ENABLE_PY_VORTEX_SUPPORT, test "x$enable_py_vortex_support" = "xyes")

dnl check for console log
AC_ARG_ENABLE(py-vortex-log, [  --enable-py-vortex-log       Enable building PyVortex console debug log support [default=yes]], 
	      enable_py_vortex_log="$enableval", 
	      enable_py_vortex_log=yes)
AM_CONDITIONAL(ENABLE_PY_VORTEX_LOG, test "x$enable_py_vortex_log" = "xyes")

dnl check for lua-vortex support
AC_ARG_ENABLE(lua-vortex,   [   --disable-lua-vortex       Makes Vortex Library to be build without LuaVortex: lua bindings support],
              enable_lua_vortex_support="$enableval",
	      enable_lua_vortex_support=yes)
if test "$enable_lua_vortex_support" = "yes" ; then
    dnl lua support
    PKG_CHECK_MODULES(LUA, lua5.1, enable_lua_vortex_support="yes", enable_lua_vortex_support=="no")

    if test "$enable_lua_vortex_support" = "yes" ; then
       LUA_CFLAGS=`echo $LUA_CFLAGS | sed 's@ \/@/@g'`
       LUA_LIBS=`echo $LUA_LIBS | sed 's@ \/@/@g'`
       AC_SUBST(LUA_CFLAGS)
       AC_SUBST(LUA_LIBS)

       # installation directory
       LUA_INSTALL_DIR=`pkg-config lua5.1 --variable=INSTALL_CMOD`
       AC_SUBST(LUA_INSTALL_DIR)

       dnl check for console log
       AC_ARG_ENABLE(lua-vortex-log, [  --enable-lua-vortex-log       Enable building LuaVortex console debug log support [default=yes]], 
     	             enable_lua_vortex_log="$enableval", 
	             enable_lua_vortex_log=yes)
    fi
fi
AM_CONDITIONAL(ENABLE_LUA_VORTEX_SUPPORT, test "x$enable_lua_vortex_support" = "xyes")
AM_CONDITIONAL(ENABLE_LUA_VORTEX_LOG, test "x$enable_lua_vortex_log" = "xyes")


dnl general libries subsitution
dnl AC_SUBST(LIBRARIES_CFLAGS)
dnl AC_SUBST(LIBRARIES_LIBS)

dnl get current platform and add especific flags
case $host in
  *-*-beos*)
    echo "Found BEOS platform: $host.."
    ;;
  *-*-cygwin*)
    echo "Found cygwin platform: $host.."
    ;;
  *-*-mingw*)
    echo "Found mingw platform: $host.."
    ADDITIONAL_LIBS=-lws2_32
    ;;
  *)
    echo "Found platform: $host.."
    ;;
esac
AC_SUBST(ADDITIONAL_LIBS)

# check additional flags for exarg
AC_TRY_LINK([#define _GNU_SOURCE
#include <stdio.h>], 
[
  char * result;
  return vasprintf (&result, "This is a test: %d", NULL);
], [have_vasprintf=yes],[have_vasprintf=no])
echo "Checking vasprintf support: $have_vasprintf"

dnl support for vasprintf
case $have_vasprintf in
yes)
	EXARG_FLAGS="-DHAVE_VASPRINTF $EXARG_FLAGS"
	echo "vasprintf support found"
     ;;
esac

dnl check for windows platform
case $host in
*-*-mingw*)
	EXARG_FLAGS="-DOS_WIN32 $EXARG_FLAGS"
	echo "windows platform found"
	;;	
*)
	echo "platform found: $host"
    ;;
esac
AC_SUBST(EXARG_FLAGS)	

AC_OUTPUT([
Makefile
src/Makefile
tunnel/Makefile
pull/Makefile
xml-rpc/Makefile
sasl/Makefile
tls/Makefile
http/Makefile
alive/Makefile
web-socket/Makefile
external/Makefile
py-vortex/Makefile
py-vortex/src/Makefile
py-vortex/test/Makefile
py-vortex/py-vortex.pc
py-vortex/py-vortex-sasl.pc
py-vortex/py-vortex-tls.pc
py-vortex/doc/conf.py
py-vortex/doc/Makefile
lua-vortex/Makefile
lua-vortex/src/Makefile
lua-vortex/test/Makefile
lua-vortex/doc/conf.py
lua-vortex/doc/Makefile
doc/Makefile
doc/vortex-doc.doxygen
test/Makefile
xml-rpc-gen/Makefile
data/Makefile
logo/Makefile
vortex-1.1.pc
vortex-xml-rpc-1.1.pc
vortex-tunnel-1.1.pc
vortex-pull-1.1.pc
vortex-sasl-1.1.pc
vortex-tls-1.1.pc
vortex-http-1.1.pc
vortex-alive-1.1.pc
vortex-websocket-1.1.pc
vortex-external-1.1.pc
])

echo "------------------------------------------"
echo "--     VORTEX LIBRARY 1.1 SETTINGS      --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "      select(2) support:           [yes]"
echo "      poll(2) support:             [$enable_poll]"
echo "      epoll(2) support:            [$enable_cv_epoll]"
echo "      default:                     [$default_platform]"
echo "      debug log support:           [$enable_vortex_log]"
echo "      release prefix:              [$enable_release_prefix]"
echo "   OpenSSL TLS protocol versions detected:"
echo "      flexible methods: "
echo "        SSLv23 (old flex):  $ssl_sslv23_supported"
echo "        TLSflex (new flex): $ssl_tls_flexible_supported"
echo "      fixed ssl/tls protocol: "
echo "        SSLv3:              $ssl_sslv3_supported"
echo "        TLSv1.0:            $ssl_tlsv10_supported"
echo "        TLSv1:              $ssl_tlsv1_supported"
echo "        TLSv1.1:            $ssl_tlsv11_supported"
echo "        TLSv1.2:            $ssl_tlsv12_supported"
echo "      pthread cflags: $PTHREAD_CFLAGS, libs=$PTHREAD_LIBS"
echo "      additional libs: $ADDITIONAL_LIBS"
echo "      axl flags: $AXL_CFLAGS"
echo "      axl libs: $AXL_LIBS"
echo "      nopoll flags: $NOPOLL_CFLAGS"
echo "      nopoll libs: $NOPOLL_LIBS"
echo "      gsasl flags: $GSASL_CFLAGS"
echo "      gsasl libs: $GSASL_LIBS"
echo "      lua flags: $LUA_CFLAGS"
echo "      lua libs: $LUA_LIBS"
if test x$enable_vortex_log = xyes ; then
echo "      NOTE: To disable log reporting use: "
echo "            --disable-vortex-log"               
fi
echo
echo "   Axl installation: "
echo "      cflags: $AXL_CFLAGS"
echo "      libs:   $AXL_LIBS"
echo
if test x$readline_found = xtrue ; then
   echo "   Build vortex-client tool:                 enabled"
else
   echo "   Build vortex-client tool:                 disabled"
   echo 
   echo "     NOTE: To enable vortex-client tool building"
   echo "     it is needed to have readline on this system. "
   echo "     Once installed run again configure process"
   echo "     using --enable-vortex-client option."
fi
if test x$enable_tls_support = xyes ; then
   echo "   Build TLS profile support:                enabled"
   echo "     (libvortex-tls-1.1)"
else
   echo "   Build TLS profile support:                disabled"
   echo 
   echo "     NOTE: To enable TLS profile support you have to"
   echo "     install OpenSSL developement headers. "
   echo "     Once installed run again configure process "
   echo "     using --enable-tls-support option."
fi
if test x$enable_sasl_support = xyes ; then
   echo "   Build SASL profile support:               enabled"
   echo "     (libvortex-sasl-1.1)"
   if test x$found_old_sasl = xyes ; then
      echo "     WARNING: found GSASL installation previous to 0.2.21 release."
      echo "              This could cause unexpected memory errors."
   fi 
else
   echo "   Build SASL profile support:               disabled"
   echo 
   echo "     NOTE: To enable SASL profile support you have to"
   echo "     install GNU SASL developement files. "
   echo "     Once installed run again configure process "
   echo "     using --enable-sasl-support option."
fi
if test x$enable_websocket_support = xyes ; then
   echo "   Build WebSocket extension library:        enabled"
   echo "     (libvortex-websocket-1.1)"
else
   echo "   Build WebSocket extension library:        disabled"
   echo 
   echo "     NOTE: To enable WebSocket support you need to have libnopoll"
   echo "           installed on your system and use --enable-websocket-support option."
fi
if test x$enable_xml_rpc_support = xyes ; then
   echo "   Build XML-RPC profile support:            enabled"
   echo "     (libvortex-xml-rpc-1.1)"
else
   echo "   Build XML-RPC profile support:            disabled"
   echo 
   echo "     NOTE: To enable XML-RPC profile support you have to"
   echo "           use --enable-xml-rpc-support option."
fi
if test x$enable_xml_rpc_gen_tool = xyes ; then
   echo "   Build xml-rpc-gen tool:                   enabled"
else
   echo "   Build xml-rpc-gen tool:                   disabled"
   echo 
   echo "     NOTE: To enable building the xml-rpc-gen tool"
   echo "           use the configure flag: "
   echo "              --enable-xml-rpc-gen-tool"
fi
if test x$enable_tunnel_support = xyes ; then
   echo "   Build TUNNEL profile support:             enabled"
   echo "     (libvortex-tunnel-1.1)"
else
   echo "   Build TUNNEL profile support:             disabled"
   echo 
   echo "     NOTE: To enable TUNNEL profile support you have to"
   echo "           use --enable-tunnel-support option."
fi
if test x$enable_pull_support = xyes ; then
   echo "   Build PULL API support:                   enabled"
   echo "     (libvortex-pull-1.1)"
else
   echo "   Build PULL API support:                   disabled"
   echo 
   echo "     NOTE: To enable PULL API you have to"
   echo "           use --enable-pull-support option."
fi
if test x$enable_http_support = xyes ; then
   echo "   Build HTTP CONNECT proxy support:         enabled"
   echo "     (libvortex-http-1.1)"
else
   echo "   Build HTTP CONNECT proxy support:         disabled"
   echo 
   echo "     NOTE: To enable HTTP CONNECT proxy support you have to"
   echo "           use --enable-http-support option."
fi
if test x$enable_alive_support = xyes ; then
   echo "   Build ALIVE check profile:                enabled"
   echo "     (libvortex-alive-1.1)"
else
   echo "   Build ALIVE check profile:                disabled"
   echo 
   echo "     NOTE: To enable ALIVE profile check you have to"
   echo "           use --enable-alive-support option."
fi
echo
echo "   Bindings: "
echo
if test x$enable_py_vortex_support = xyes ; then
   echo "   Build PyVortex support:                   enabled"
   echo "     python install dir: ${python_install_dir}"
   echo "     python dir: ${pythondir}"
   echo "     python cflags:      ${PYTHON_CFLAGS}"
   echo "     python libs:        ${PYTHON_LIBS}"
else
   echo "   Build PyVortex support:                   disabled"
   echo 
   echo "     NOTE: To enable PyVortex support you have to install python development headers and "
   echo "           use --enable-py-vortex option."
fi
if test x$enable_lua_vortex_support = xyes ; then
   echo "   Build LuaVortex support:                   enabled"
   echo "     lua install dir: ${luathon_install_dir}"
   echo "     lua cflags:      ${LUA_CFLAGS}"
   echo "     lua libs:        ${LUA_LIBS}"
else
   echo "   Build LuaVortex support:                   disabled"
   echo 
   echo "     NOTE: To enable LuaVortex support you have to install liblua5.1-0-dev development headers and "
   echo "           use --enable-lua-vortex option."
fi
if test x$DOXYGEN = xyes ; then
   echo "   Build Vortex Library doc:                 enabled"
else	    
   echo "   Build Vortex Library doc:                 disabled"
   echo  
   echo "     NOTE: To enable Vortex documentation you will"
   echo "     have to install doxygen tool. Then run again"
   echo "     configure process."
fi
echo
echo "   To build without optimizer use: "
echo "   >> make CFLAGS='-g -O0' CXXFLAGS='-g -O0' clean all"
echo
echo "------------------------------------------"
echo "--            LET IT BEEP!              --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

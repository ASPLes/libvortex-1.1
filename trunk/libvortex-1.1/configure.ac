dnl  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
dnl  Copyright (C) 2005 Advanced Software Production Line, S.L.
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
dnl         C/ Dr. Michavila Nº 14
dnl         Coslada 28820 Madrid
dnl         Spain
dnl
dnl      Email address:
dnl         info@aspl.es - http://fact.aspl.es
dnl

AC_INIT(src/vortex.h)

VORTEX_VERSION=`cat VERSION`
AC_SUBST(VORTEX_VERSION)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(vortex, $VORTEX_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL

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
 for flag in none -kthread -pthread -pthreads -mt -mthreads -Kthread -threads; do 
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
AC_CACHE_CHECK([for epoll(2) support], [enable_epoll],
[AC_TRY_RUN([
#include <sys/epoll.h>
#include <unistd.h>

int main()
{
    return epoll_create(5) == -1;
}], [enable_epoll=yes], [enable_epoll=no], [enable_epoll=no])])
AM_CONDITIONAL(ENABLE_EPOLL_SUPPORT, test "x$enable_epoll" = "xyes")

dnl select the best I/O platform
if test x$enable_epoll = xyes ; then
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
PKG_CHECK_MODULES(AXL, axl >= 0.4.15)
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
      TLS_SUPPORT_FLAGS="-lssl -lcrypto"
      AC_SUBST(TLS_SUPPORT_FLAGS)
   fi
fi
AM_CONDITIONAL(ENABLE_TLS_SUPPORT, test "x$enable_tls_support" = "xyes")

dnl check for sasl building through GSASL
AC_ARG_ENABLE(sasl-support, [  --disable-sasl-support    Makes Vortex Library to be built without SASL support], 
	      enable_sasl_support="$enableval", 
	      enable_sasl_support=yes)
if test "$enable_sasl_support" != "no" ; then
    PKG_CHECK_MODULES(GSASL, libgsasl >= 0.2.5,	[enable_sasl_support=yes], [enable_sasl_support=no])
    if test "$enable_sasl_support" != "yes" ; then
        AC_MSG_WARN([Cannot find GNU SASL installaton, disabling it. This will disable SASL family profile support.])
	enable_sasl_support=no
    else
dnl Enable SASL cflags and link headers
	SASL_LIBS=`pkg-config --libs libgsasl`
	SASL_CFLAGS=`pkg-config --cflags libgsasl`
	dnl SASL libraries subsitution
	AC_SUBST(SASL_CFLAGS)
	AC_SUBST(SASL_LIBS)
    fi
fi
AM_CONDITIONAL(ENABLE_SASL_SUPPORT, test "x$enable_sasl_support" = "xyes")

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

dnl general libries subsitution
dnl AC_SUBST(LIBRARIES_CFLAGS)
dnl AC_SUBST(LIBRARIES_LIBS)

AC_OUTPUT([
Makefile
src/Makefile
tunnel/Makefile
doc/Makefile
doc/vortex-doc.doxygen
test/Makefile
xml-rpc-gen/Makefile
data/Makefile
logo/Makefile
vortex-1.1.pc
vortex-tunnel-1.1.pc])

echo "------------------------------------------"
echo "--     VORTEX LIBRARY 1.1 SETTINGS      --"
echo "------------------------------------------"
echo "   Installation prefix:            [$prefix]"
echo "      select(2) support:           [yes]"
echo "      poll(2) support:             [$enable_poll]"
echo "      epoll(2) support:            [$enable_epoll]"
echo "      default:                     [$default_platform]"
echo "      debug log support:           [$enable_vortex_log]"
echo "      pthread cflags=$PTHREAD_CFLAGS, libs=$PTHREAD_LIBS"
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
   echo "   Build vortex-client tool:       enabled"
else
   echo "   Build vortex-client tool:       disabled"
   echo 
   echo "     NOTE: To enable vortex-client tool building"
   echo "     it is needed to have readline on this system. "
   echo "     Once installed run again configure process"
   echo "     using --enable-vortex-client option."
fi
if test x$enable_tls_support = xyes ; then
   echo "   Build TLS profile support:      enabled"
else
   echo "   Build TLS profile support:      disabled"
   echo 
   echo "     NOTE: To enable TLS profile support you have to"
   echo "     install OpenSSL developement headers. "
   echo "     Once installed run again configure process "
   echo "     using --enable-tls-support option."
fi
if test x$enable_sasl_support = xyes ; then
   echo "   Build SASL profile support:     enabled"
else
   echo "   Build SASL profile support:     disabled"
   echo 
   echo "     NOTE: To enable SASL profile support you have to"
   echo "     install GNU SASL developement files. "
   echo "     Once installed run again configure process "
   echo "     using --enable-sasl-support option."
fi
if test x$enable_xml_rpc_support = xyes ; then
   echo "   Build XML-RPC profile support:  enabled"
else
   echo "   Build XML-RPC profile support:  disabled"
   echo 
   echo "     NOTE: To enable XML-RPC profile support you have to"
   echo "           use --enable-xml-rpc-support option."
fi
if test x$enable_xml_rpc_gen_tool = xyes ; then
   echo "   Build xml-rpc-gen tool:         enabled"
else
   echo "   Build xml-rpc-gen tool:         disabled"
   echo 
   echo "     NOTE: To enable building the xml-rpc-gen tool"
   echo "           use the configure flag: "
   echo "              --enable-xml-rpc-gen-tool"
fi
if test x$enable_tunnel_support = xyes ; then
   echo "   Build TUNNEL profile support:   enabled"
   echo "     (libvortex-tunnel-1.1)"
else
   echo "   Build TUNNEL profile support:   disabled"
   echo 
   echo "     NOTE: To enable TUNNEL profile support you have to"
   echo "           use --enable-xml-rpc-support option."
fi
if test x$DOXYGEN = xyes ; then
   echo "   Build Vortex Library doc:       enabled"
else	    
   echo "   Build Vortex Library doc:       disabled"
   echo  
   echo "     NOTE: To enable Vortex documentation you will"
   echo "     have to install doxygen tool. Then run again"
   echo "     configure process."
fi
echo
echo "------------------------------------------"
echo "--            LET IT BEEP!              --"
echo "--                                      --"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

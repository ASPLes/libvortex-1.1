dnl  Copyright (C) 2009 Advanced Software Production Line, S.L.
dnl
dnl  This program is free software; you can redistribute it and/or
dnl  modify it under the terms of the GNU Lesser General Public
dnl  License as published by the Free Software Foundation; either
dnl  version 2.1 of the License, or (at your option) any later
dnl  version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
dnl  Lesser General Public License for more details.
dnl
dnl  You should have received a copy of the GNU Lesser General Public
dnl  License along with this program; if not, write to the Free
dnl  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
dnl  02111-1307 USA
dnl  
dnl  You may find a copy of the license under this software is
dnl  released at COPYING file. This is LGPL software: you are wellcome
dnl  to develop propietary applications using this library withtout
dnl  any royalty or fee but returning back any change, improvement or
dnl  addition in the form of source code, project image, documentation
dnl  patches, etc.
dnl
dnl  For commercial support on build BEEP enabled solutions contact
dnl  us:
dnl          
dnl      Postal address:
dnl         Advanced Software Production Line, S.L.
dnl         Edificio Alius A, Oficina 102,
dnl         C/ Antonio Suarez Nº 10,
dnl         Alcalá de Henares 28802 Madrid
dnl         Spain
dnl
dnl      Email address:
dnl         info@aspl.es - http://www.aspl.es/xml
dnl
AC_INIT(src/py_vortex.h)

configure_dir=`dirname "$0"`
PY_VORTEX_VERSION=`cat $configure_dir/VERSION`
AC_SUBST(PY_VORTEX_VERSION)

AC_CONFIG_AUX_DIR(.)
AM_INIT_AUTOMAKE(py_vortex, $PY_VORTEX_VERSION)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

# get python includes 
PYTHON_INCLUDES=`python-config --includes 2>/dev/null`
old_CPPFLAGS=$CPPFLAGS
CPPFLAGS="-Wall -Werror $PYTHON_INCLUDES"
echo "Python includes: $PYTHON_INCLUDES"
AC_SUBST(PYTHON_INCLUDES)

AC_TRY_COMPILE([#include <Python.h>],
  [Py_InitModule3(NULL, NULL, NULL);],
  python_devel_found=yes,
  python_devel_found=no)
AC_MSG_RESULT([Python development headers status: $python_devel_found])
if test "x$python_devel_found" = "xno"; then
   AC_MSG_ERROR([Cannot find Python.h header or current python development environment do not compile symbols required. On debian try to install python-dev package.])
fi

# get python flags and libs
PYTHON_CFLAGS=`python-config --cflags 2>/dev/null`
PYTHON_LIBS=`python-config --ldflags 2>/dev/null`
AC_SUBST(PYTHON_CFLAGS)
AC_SUBST(PYTHON_LIBS)

dnl check for vortex
PKG_CHECK_MODULES(VORTEX, vortex-1.1)
AC_SUBST(VORTEX_CFLAGS)
AC_SUBST(VORTEX_LIBS)

dnl check for axl
PKG_CHECK_MODULES(AXL, axl)
AC_SUBST(AXL_CFLAGS)
AC_SUBST(AXL_LIBS)

dnl check for console log
AC_ARG_ENABLE(py-vortex-log, [  --enable-py-vortex-log       Enable building PyVortex console debug log support [default=yes]], 
	      enable_py_vortex_log="$enableval", 
	      enable_py_vortex_log=yes)
AM_CONDITIONAL(ENABLE_PY_VORTEX_LOG, test "x$enable_py_vortex_log" = "xyes")

AC_PROG_LIBTOOL

AC_OUTPUT([
Makefile
src/Makefile
test/Makefile])

echo "------------------------------------------"
echo "--       PyVortex EXTENSION SETTINGS    --"
echo "------------------------------------------"
echo "      PyVortex $PY_VORTEX_VERSION"
echo "      PyVortex log: $enable_py_vortex_log "
echo "------------------------------------------"
echo "--     NOW TYPE: make; make install     --"
echo "------------------------------------------"

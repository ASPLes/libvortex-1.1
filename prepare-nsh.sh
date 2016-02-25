#!/bin/bash

set -x

# gen vortex_1_1_product_version.nsh
echo -n "!define PRODUCT_VERSION \"" > vortex_1_1_product_version.nsh
value=`cat VERSION`
echo -n $value >> vortex_1_1_product_version.nsh
echo "\"" >> vortex_1_1_product_version.nsh
echo "!define PLATFORM_BITS \"$1\"" >> vortex_1_1_product_version.nsh
echo "InstallDir \"\$PROGRAMFILES$1\VortexW$1\"" >> vortex_1_1_product_version.nsh

# readline libs
readline_libs=`echo $readline_libs | sed 's@/@\\\@g'`
readline_libs=$(echo $readline_libs)
echo "!define READLINE_LIBS \"$readline_libs\"" >> vortex_1_1_product_version.nsh

# gsasl include dir
gsasl_include_dir=`echo $GSASL_FLAGS | sed 's@-I@@g' | sed 's@-DENABLE_SASL_SUPPORT@@g' | sed 's@/@\\\@g'`
gsasl_include_dir=$(echo $gsasl_include_dir)
echo "!define GSASL_INCLUDE_DIR \"$gsasl_include_dir\"" >> vortex_1_1_product_version.nsh

# check those files that should be included
rm -f vortex_1_1_sasl_optional_files.nsh
touch vortex_1_1_sasl_optional_files.nsh
if [ -f "libvortex-1.1/test/libgcrypt-11.dll" ]; then
   echo '  File "libvortex-1.1\test\libgcrypt-11.dll"' >> vortex_1_1_sasl_optional_files.nsh
fi
if [ -f "libvortex-1.1/test/libgpg-error-0.dll" ]; then
   echo '  File "libvortex-1.1\test\libgpg-error-0.dll"' >> vortex_1_1_sasl_optional_files.nsh
fi


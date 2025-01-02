#!/bin/bash

rm -rf doc/html
rm -rf debian/tmp
rm -rf debian/libvortex-1.1
rm -rf debian/libvortex-1.1-dev
rm -rf debian/libvortex-alive-1.1
rm -rf debian/libvortex-alive-1.1-dev
rm -rf debian/libvortex-external-1.1
rm -rf debian/libvortex-external-1.1-dev
rm -rf debian/libvortex-http-1.1
rm -rf debian/libvortex-http-1.1-dev
rm -rf debian/libvortex-pull-1.1
rm -rf debian/libvortex-pull-1.1-dev
rm -rf debian/libvortex-sasl-1.1
rm -rf debian/libvortex-sasl-1.1-dev
rm -rf debian/libvortex-tls-1.1
rm -rf debian/libvortex-tls-1.1-dev
rm -rf debian/libvortex-tunnel-1.1
rm -rf debian/libvortex-tunnel-1.1-dev
rm -rf debian/libvortex-websocket-1.1
rm -rf debian/libvortex-websocket-1.1-dev
rm -rf debian/libvortex-xml-rpc-1.1
rm -rf debian/libvortex-xml-rpc-1.1-dev
rm -rf debian/python-vortex
rm -rf debian/python-vortex-alive
rm -rf debian/python-vortex-dev
rm -rf debian/python-vortex-sasl
rm -rf debian/python-vortex-sasl-dev
rm -rf debian/python-vortex-tls
rm -rf debian/python-vortex-tls-dev
rm -rf debian/vortex-client-1.1
rm -rf debian/vortex-xml-rpc-gen-1.1


# find all files that have copy right declaration associated to Aspl that don't have 
# the following declaration year
current_year="2025"
LANG=C rgrep "Copyright" alive data debian-files doc external http logo lua-vortex pull py-vortex rpm sasl src test tls tunnel web web-socket xml-rpc xml-rpc-gen configure.ac 2>&1 | grep "Advanced" | grep -v "Permission denied" | grep -v '~:' | grep -v '/\.svn/' | grep -v "${current_year}"

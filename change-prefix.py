#!/usr/bin/python
# coding: utf-8
#  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
#  change-prefix.py : a tool to change library prefix by updating Makefiles before compiling.
#  Copyright (C) 2022 Advanced Software Production Line, S.L.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free
#  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307 USA
#  
#  You may find a copy of the license under this software is released
#  at COPYING file. This is LGPL software: you are wellcome to
#  develop propietary applications using this library withtout any
#  royalty or fee but returning back any change, improvement or
#  addition in the form of source code, project image, documentation
#  patches, etc. 
#
#  You can also contact us to negociate other license term you may be
#  interested different from LGPL. Contact us at:
#          
#        Postal address:
#           Advanced Software Production Line, S.L.
#           C/ Antonio Suarez Nº10, Edificio Alius A, Despacho 102
#           Alcalá de Henares 28802 Madrid
#           Spain
#  
#        Email address:
#           info@aspl.es - http://www.aspl.es/vortex
#

def replace_on_file (file_path, maps):

    # get content
    content = open (file_path).read ()

    # write content
    open ("%s.prefix.bak" % file_path, "w").write (content)
    
    for (o, r) in maps:
        content = content.replace (o, r)

    # save back content
    open (file_path, "w").write (content)
    print "INFO: updating %s.." % file_path
    return

print "change-prefix.py : a tool to change library prefix by updating Makefiles before compiling."
print "Copyright (C) 2022 Advanced Software Production Line, S.L."

prefix = raw_input("Please, provide library prefix to configure (i.e.: 64, 32, lts): ")
if prefix:
    prefix = prefix.strip ()
    
if not prefix:
    print "ERROR: no prefix was provided"
    sys.exit (-1)

print "INFO: this is change library compilation from libvortex-1.1.so.0 -> libvortex%s-1.1.so.0" % prefix
answer = raw_input( "Is this correct? (yes/NO): " )
if answer:
    answer = answer.strip ()
if answer == "no":
    print "ERROR: doing nothing.."
    sys.exit (-1)
if answer != "yes":
    print "ERROR: please, answer yes or no.."
    sys.exit (-1)

# create substitution maps
maps = []
# sasl
maps.append (("libvortex-1.1.def", "libvortex%s-1.1.def" % prefix))
maps.append (("libvortex-1.1.exp", "libvortex%s-1.1.exp" % prefix))
maps.append (("libvortex-1.1.la", "libvortex%s-1.1.la" % prefix))
maps.append (("libvortex-1.1.def", "libvortex%s-1.1.def" % prefix))
maps.append (("libvortex_1_1", "libvortex%s_1_1" % prefix))
# tls
maps.append (("libvortex-tls-1.1.def", "libvortex%s-tls-1.1.def" % prefix))
maps.append (("libvortex-tls-1.1.exp", "libvortex%s-tls-1.1.exp" % prefix))
maps.append (("libvortex-tls-1.1.la", "libvortex%s-tls-1.1.la" % prefix))
maps.append (("libvortex-tls-1.1.def", "libvortex%s-tls-1.1.def" % prefix))
maps.append (("libvortex_tls_1_1", "libvortex_tls_%s_1_1" % prefix))
# sasl
maps.append (("libvortex-sasl-1.1.def", "libvortex%s-sasl-1.1.def" % prefix))
maps.append (("libvortex-sasl-1.1.exp", "libvortex%s-sasl-1.1.exp" % prefix))
maps.append (("libvortex-sasl-1.1.la", "libvortex%s-sasl-1.1.la" % prefix))
maps.append (("libvortex-sasl-1.1.def", "libvortex%s-sasl-1.1.def" % prefix))
maps.append (("libvortex_sasl_1_1", "libvortex_sasl_%s_1_1" % prefix))
# alive
maps.append (("libvortex-alive-1.1.def", "libvortex%s-alive-1.1.def" % prefix))
maps.append (("libvortex-alive-1.1.exp", "libvortex%s-alive-1.1.exp" % prefix))
maps.append (("libvortex-alive-1.1.la", "libvortex%s-alive-1.1.la" % prefix))
maps.append (("libvortex-alive-1.1.def", "libvortex%s-alive-1.1.def" % prefix))
maps.append (("libvortex_alive_1_1", "libvortex_alive_%s_1_1" % prefix))
# http
maps.append (("libvortex-http-1.1.def", "libvortex%s-http-1.1.def" % prefix))
maps.append (("libvortex-http-1.1.exp", "libvortex%s-http-1.1.exp" % prefix))
maps.append (("libvortex-http-1.1.la", "libvortex%s-http-1.1.la" % prefix))
maps.append (("libvortex-http-1.1.def", "libvortex%s-http-1.1.def" % prefix))
maps.append (("libvortex_http_1_1", "libvortex_http_%s_1_1" % prefix))
# tunnel
maps.append (("libvortex-tunnel-1.1.def", "libvortex%s-tunnel-1.1.def" % prefix))
maps.append (("libvortex-tunnel-1.1.exp", "libvortex%s-tunnel-1.1.exp" % prefix))
maps.append (("libvortex-tunnel-1.1.la", "libvortex%s-tunnel-1.1.la" % prefix))
maps.append (("libvortex-tunnel-1.1.def", "libvortex%s-tunnel-1.1.def" % prefix))
maps.append (("libvortex_tunnel_1_1", "libvortex_tunnel_%s_1_1" % prefix))
# pull
maps.append (("libvortex-pull-1.1.def", "libvortex%s-pull-1.1.def" % prefix))
maps.append (("libvortex-pull-1.1.exp", "libvortex%s-pull-1.1.exp" % prefix))
maps.append (("libvortex-pull-1.1.la", "libvortex%s-pull-1.1.la" % prefix))
maps.append (("libvortex-pull-1.1.def", "libvortex%s-pull-1.1.def" % prefix))
maps.append (("libvortex_pull_1_1", "libvortex_pull_%s_1_1" % prefix))
# web-socket
maps.append (("libvortex-websocket-1.1.def", "libvortex%s-websocket-1.1.def" % prefix))
maps.append (("libvortex-websocket-1.1.exp", "libvortex%s-websocket-1.1.exp" % prefix))
maps.append (("libvortex-websocket-1.1.la", "libvortex%s-websocket-1.1.la" % prefix))
maps.append (("libvortex-websocket-1.1.def", "libvortex%s-websocket-1.1.def" % prefix))
maps.append (("libvortex_websocket_1_1", "libvortex_websocket_%s_1_1" % prefix))

# xml-rpc
maps.append (("libvortex-xml-rpc-1.1.def", "libvortex%s-xml-rpc-1.1.def" % prefix))
maps.append (("libvortex-xml-rpc-1.1.exp", "libvortex%s-xml-rpc-1.1.exp" % prefix))
maps.append (("libvortex-xml-rpc-1.1.la", "libvortex%s-xml-rpc-1.1.la" % prefix))
maps.append (("libvortex-xml-rpc-1.1.def", "libvortex%s-xml-rpc-1.1.def" % prefix))
maps.append (("libvortex_xml_rpc_1_1", "libvortex_xml_rpc_%s_1_1" % prefix))

# axl translations
maps.append (("-laxl", "-laxl%s" % prefix))

# lua-plugins
maps.append (("liblua_vortex_sasl_11", "liblua_vortex_sasl_%s_11" % prefix))

replace_on_file ("src/Makefile", maps)
replace_on_file ("tls/Makefile", maps)
replace_on_file ("sasl/Makefile", maps)
replace_on_file ("alive/Makefile", maps)
replace_on_file ("http/Makefile", maps)
replace_on_file ("tunnel/Makefile", maps)
replace_on_file ("pull/Makefile", maps)
replace_on_file ("web-socket/Makefile", maps)
replace_on_file ("xml-rpc/Makefile", maps)
replace_on_file ("xml-rpc-gen/Makefile", maps)
replace_on_file ("lua-vortex/Makefile", maps)
replace_on_file ("lua-vortex/src/Makefile", maps)
replace_on_file ("py-vortex/Makefile", maps)
replace_on_file ("py-vortex/src/Makefile", maps)
replace_on_file ("test/Makefile", maps)







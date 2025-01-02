#!/usr/bin/lua
--  Lua Vortex:  Lua bindings for Vortex Library
--  Copyright (C) 2025 Advanced Software Production Line, S.L.
--
--  This program is free software; you can redistribute it and/or
--  modify it under the terms of the GNU Lesser General Public License
--  as published by the Free Software Foundation; either version 2.1
--  of the License, or (at your option) any later version.
--
--  This program is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--  Lesser General Public License for more details.
--
--  You should have received a copy of the GNU Lesser General Public
--  License along with this program; if not, write to the Free
--  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
--  02111-1307 USA
--  
--  You may find a copy of the license under this software is released
--  at COPYING file. This is LGPL software: you are welcome to develop
--  proprietary applications using this library without any royalty or
--  fee but returning back any change, improvement or addition in the
--  form of source code, project image, documentation patches, etc.
--
--  For commercial support on build BEEP enabled solutions contact us:
--          
--      Postal address:
--         Advanced Software Production Line, S.L.
--         C/ Antonio Suarez Nº 10, 
--         Edificio Alius A, Despacho 102
--         Alcalá de Henares 28802 (Madrid)
--         Spain
--
--      Email address:
--         info@aspl.es - http://www.aspl.es/vortex

require "vortex"
require "vortex.sasl"
require "vortex.tls"
require "vortex.alive"

function error (message) 
   print ("ERROR: " .. message)
end

function info (message) 
   print ("   " .. message)
end

-- uri tests
regression_uri      = "http://iana.org/beep/transient/vortex-regression"
regression_uri_zero = "http://iana.org/beep/transient/vortex-regression/zero"

regression_uri_deny           = "http://iana.org/beep/transient/vortex-regression/deny"
regression_uri_deny_supported = "http://iana.org/beep/transient/vortex-regression/deny_supported"

function default_frame_received (conn, channel, frame, data)
   -- frame received, just reply
   if not channel:send_rpy (frame.payload, frame.payload_size, frame.msgno) then
      print ("ERROR: Failed to send reply, some parts of the regression test are not working..")
   end
end

function deny_supported (channel_num, conn, data)
   print ("Received channel start request, denying..")
   -- always deny 
   return false
end

function sasl_auth_handler (conn, auth_props, user_data)
   
   print ("Received request to complete auth process using profile: " .. auth_props["mech"])
   print ("Auth id: " .. tostring(auth_props["auth_id"]))
   print ("Anonymous token: " .. tostring(auth_props["anonymous_token"]))
   print ("Password: " .. tostring (auth_props["password"]))

   -- check plain
   if auth_props["mech"] == vortex.sasl.PLAIN then
      if auth_props["auth_id"] == "bob" and auth_props["password"] == "secret" then
	 return true
      end
   end

   -- check anonymous
   if auth_props["mech"] == vortex.sasl.ANONYMOUS then
      if auth_props["anonymous_token"] == "test@aspl.es" then
	 return true
      end 
   end

   -- check digest-md5
   if auth_props["mech"] == vortex.sasl.DIGEST_MD5 then
      if auth_props["auth_id"] == "bob" then
	 -- set password notification
	 auth_props["return_password"] = true
	 return "secret"
      end
   end

   -- check cram-md5
   if auth_props["mech"] == vortex.sasl.CRAM_MD5 then
      if auth_props["auth_id"] == "bob" then
	 -- set password notification
	 auth_props["return_password"] = true
	 return "secret"
      end
   end
   
   -- deny if not accepted
   return false
end

function tls_accept_handler(conn, server_name, data)
    print ("Received request to accept TLS for serverName: " .. tostring (server_name))
    return true
end

function tls_cert_handler(conn, server_name, data) 
    return "test.crt"
end

function tls_key_handler(conn, server_name, data)
    return "test.key"
end

-- init context
ctx = vortex.ctx.new ()
if not ctx:init () then
   print ("ERROR: failed to init vortex context..")
   os.exit (-1)
end

-- register some profiles
vortex.register_profile (ctx, regression_uri, default_frame_received)
vortex.register_profile (ctx, regression_uri_zero, default_frame_received)
vortex.register_profile (ctx, regression_uri_deny_supported, 
			 -- frame received 
			 default_frame_received, nil, 
			 -- start handler
			 deny_supported)

-- enable sasl listener support
if not vortex.sasl.accept_mech (ctx, "plain", sasl_auth_handler) then
   print ("ERROR: unable to install SASL plain handler..")
   os.exit (-1)
end
if not vortex.sasl.accept_mech (ctx, "anonymous", sasl_auth_handler) then
   print ("ERROR: unable to install SASL anonymous handler..")
   os.exit (-1)
end
if not vortex.sasl.accept_mech (ctx, "digest-md5", sasl_auth_handler) then
   print ("ERROR: unable to install SASL digest-md5 handler..")
   os.exit (-1)
end
if not vortex.sasl.accept_mech (ctx, "cram-md5", sasl_auth_handler) then
   print ("ERROR: unable to install SASL cram-md5 handler..")
   os.exit (-1)
end

-- enable tls support
vortex.tls.accept_tls (ctx, 
		       -- accept handler
		       tls_accept_handler, "test", 
		       -- cert handler
		       tls_cert_handler, "test 2",
		       -- key handler
		       tls_key_handler, "test 3")

-- enable receiving alive requests
if not vortex.alive.init (ctx) then
   error ("Unable to start listener, vortex.alive.init failed..")
   os.exit (-1)
end

-- create a listener
listener = vortex.create_listener (ctx, "0.0.0.0", "44010")

-- check listener status
if not listener:is_ok () then
   print ("ERROR: failed to start listener, Maybe there is another instance running at 44010?")
   os.exit (-1)
end

-- do wait operation
info ("Listener started, waiting requests..")
vortex.wait_listeners (ctx, true)

#!/usr/bin/python
# -*- coding: utf-8 -*-
#  PyVortex: Vortex Library Python bindings
#  Copyright (C) 2009 Advanced Software Production Line, S.L.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free
#  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307 USA
#  
#  You may find a copy of the license under this software is released
#  at COPYING file. This is LGPL software: you are welcome to develop
#  proprietary applications using this library without any royalty or
#  fee but returning back any change, improvement or addition in the
#  form of source code, project image, documentation patches, etc.
#
#  For commercial support on build BEEP enabled solutions contact us:
#          
#      Postal address:
#         Advanced Software Production Line, S.L.
#         C/ Antonio Suarez Nº 10, 
#         Edificio Alius A, Despacho 102
#         Alcalá de Henares 28802 (Madrid)
#         Spain
#
#      Email address:
#         info@aspl.es - http://www.aspl.es/vortex
#
import vortex
import vortex.sasl
import vortex.tls
import signal
import sys

# import common items for reg test
from regtest_common import *

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def default_frame_received (conn, channel, frame, data):
    # reply to the frame received
    if not channel.send_rpy (frame.payload, frame.payload_size, frame.msgno):
        error ("Failed to send reply, some parts of the regression test are not working")
    
    return

def frame_replies_ans (conn, channel, frame, data):
    if frame.type == "MSG":
        # received request, reply
        print ("Received msg, replying..")
        iterator = 0
        while iterator < 10:
            # send message
            channel.send_ans (TEST_REGRESSION_URI_4_MESSAGE, len (TEST_REGRESSION_URI_4_MESSAGE), frame.msg_no)

            # next message
            print ("Message sent: iterator=" + str (iterator))
            iterator += 1

        # finish transmission
        print ("Sending NUL frame: iterator=" + str (iterator))
        channel.finalize_ans (frame.msg_no)
        return

    # this must not be reached
    print ("ERROR: received request to handle a frame type not expected..shutting down connection\n")
    conn.shutdown ()

    return

def deny_supported (channel_num, conn, data):
    # always deny 
    return False

def close_conn (channel_num, conn, data):
    # close the connection in the middle of the start transaction
    conn.shutdown ()
    return False

recorded_conn = None

def record_conn (channel_num, conn, data):
    # save connection
    global recorded_conn
    recorded_conn = conn
    return True

def close_recorded_conn (channel_num, conn, data):
    # close previously recorded conn
    global recorded_conn
    if recorded_conn:
        recorded_conn.shutdown ()
        recorded_conn = None

    # and return false for channel creation
    return False

def sasl_auth_handler (conn, auth_props, user_data):
    
    print ("Received request to complete auth process using profile: " + auth_props["mech"])
    # check plain
    if auth_props["mech"] == vortex.sasl.PLAIN:
        if auth_props["auth_id"] == "bob" and auth_props["password"] == "secret":
            return True

    # check anonymous
    if auth_props["mech"] == vortex.sasl.ANONYMOUS:
        if auth_props["anonymous_token"] == "test@aspl.es":
            return True

    # check digest-md5
    if auth_props["mech"] == vortex.sasl.DIGEST_MD5:
        if auth_props["auth_id"] == "bob":
            # set password notification
            auth_props["return_password"] = True
            return "secret"

    # check cram-md5
    if auth_props["mech"] == vortex.sasl.CRAM_MD5:
        if auth_props["auth_id"] == "bob":
            # set password notification
            auth_props["return_password"] = True
            return "secret"
    
    # deny if not accepted
    return False

def tls_accept_handler(conn, server_name, data):
    print ("Received request to accept TLS for serverName: " + str (server_name))
    return True

def tls_cert_handler(conn, server_name, data):
    return "test.crt"

def tls_key_handler(conn, server_name, data):
    return "test.key"

def signal_handler (signal, stackframe):
    print ("Received signal: " + str (signal))
    return

if __name__ == '__main__':

    # create a context
    ctx = vortex.Ctx ()

    # init context
    if not ctx.init ():
        error ("Unable to init ctx, failed to start listener")
        sys.exit(-1)

    # configure signal handling
    signal.signal (signal.SIGTERM, signal_handler)
    signal.signal (signal.SIGINT, signal_handler)
    signal.signal (signal.SIGQUIT, signal_handler)

    # register all profiles used 
    vortex.register_profile (ctx, REGRESSION_URI,
                             frame_received=default_frame_received)
    vortex.register_profile (ctx, REGRESSION_URI_ZERO,
                             frame_received=default_frame_received)
    vortex.register_profile (ctx, REGRESSION_URI_DENY_SUPPORTED,
                             start=deny_supported)
    vortex.register_profile (ctx, REGRESSION_URI_ANS,
                             frame_received=frame_replies_ans)
    vortex.register_profile (ctx, REGRESSION_URI_START_CLOSE,
                             start=close_conn)
    vortex.register_profile (ctx, REGRESSION_URI_RECORD_CONN,
                             start=record_conn)
    vortex.register_profile (ctx, REGRESSION_URI_CLOSE_RECORDED_CONN,
                             start=close_recorded_conn)
    
    # enable sasl listener support
    vortex.sasl.accept_mech (ctx, "plain", sasl_auth_handler)
    vortex.sasl.accept_mech (ctx, "anonymous", sasl_auth_handler)
    vortex.sasl.accept_mech (ctx, "digest-md5", sasl_auth_handler)
    vortex.sasl.accept_mech (ctx, "cram-md5", sasl_auth_handler)

    # enable tls support
    vortex.tls.accept_tls (ctx, 
                           # accept handler
                           accept_handler=tls_accept_handler, accept_handler_data="test", 
                           # cert handler
                           cert_handler=tls_cert_handler, cert_handler_data="test 2",
                           # key handler
                           key_handler=tls_key_handler, key_handler_data="test 3")

    # create a listener
    info ("starting listener at 0.0.0.0:44010")
    listener = vortex.create_listener (ctx, "0.0.0.0", "44010")

    # check listener started
    if not listener.is_ok ():
        error ("ERROR: failed to start listener. Maybe there is another instance running at 44010?")
        sys.exit (-1)

    # do a wait operation
    info ("waiting requests..")
    vortex.wait_listeners (ctx, unlock_on_signal=True)

    

        

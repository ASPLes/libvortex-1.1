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

# import sys for command line parsing
import sys

# import python vortex binding
import vortex

####################
# regression tests #
####################

def test_01():
    # call to initilize a context and to finish it 
    ctx = vortex.Ctx ()

    # init context and finish it */
    info ("init context..");
    if not ctx.init ():
        error ("Failed to init Vortex context");
        return False

    # ok, now finish context
    info ("finishing context..");
    ctx.exit ()

    # finish ctx 
    del ctx

    return True;

def test_02():
    # call to initialize a context 
    ctx = vortex.Ctx ();

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context");
        return False;

    # call to create a connection
    conn = vortex.Connection (ctx, host, port);

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg);
        return False;

    info ("BEEP connection created to: " + conn.host + ":" + conn.port); 
    
    # now close the connection
    info ("Now closing the BEEP session..");
    conn.close ();

    return True;

###########################
# intraestructure support #
###########################

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def run_all_tests():
    test_count = 0
    for test in tests:
        # print log
        info ("Test-" + str(test_count) + ": Running " + test[1])
        
        # call test
        if not test[0]():
            error ("detected test failure at: " + test[1])
            return False;
        
        # next test
        test_count += 1
    
    ok ("All tests ok!")
    return True;
        

# declare list of tests available
tests = [
    (test_01, "Check Vortex context initialization"),
    (test_02, "Check Vortex basic BEEP connection")
]

# declare default host and port
host = "localhost"
port = "44010"

if __name__ == '__main__':
    iterator = 0
    for arg in sys.argv:
        # according to the argument position, take the value 
        if iterator == 1:
            host = arg
        elif iterator == 2:
            port = arg
            
        # next iterator
        iterator += 1

    # drop a log
    info ("Running tests against " + host + ":" + port)

    # call to run all tests
    run_all_tests ()





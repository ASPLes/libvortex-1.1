/**
 * C server skel to implement services exported by the XML-RPC
 * component: test.
 *
 * This file was generated by xml-rpc-gen tool, from Vortex Library
 * project. 
 *
 * Vortex Library homepage:           http://vortex.aspl.es
 * Axl Library homepage:              http://xml.aspl.es
 * Advanced Software Production Line: http://www.aspl.es
 */
#ifndef __SERVER_TEST_OPERATE_XML_RPC_H__
#define __SERVER_TEST_OPERATE_XML_RPC_H__

/* include base library */
#include <vortex.h>
/* include xml-rpc library */
#include <vortex_xml_rpc.h>
#include <test_types.h>

int operate_2_int_int (int a, int b, char ** fault_error, int * fault_code, VortexChannel * channel);

XmlRpcMethodResponse * __operate_2_int_int (XmlRpcMethodCall * method_call, VortexChannel * channel);

#endif

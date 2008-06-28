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
/* include base library */
#include <vortex.h>
/* include xml-rpc library */
#include <vortex_xml_rpc.h>
#include <test_types.h>

int operate_2_int_int (int a, int b, char ** fault_error, int * fault_code, VortexChannel * channel)
{
	/* WRITE HERE YOUR CODE */
	REPLY_FAULT ("Service is not implemented yet.", -1, 0);

}

/* This is a support function to invoke 'operate' service , do not modify it!! */
XmlRpcMethodResponse * __operate_2_int_int (XmlRpcMethodCall * method_call, VortexChannel * channel)
{
	/* error support variables */
	char * fault_error = NULL;
	int    fault_code  = -1;
	int    result = -1;

	/* call to the user implementation */
	result = operate_2_int_int (method_call_get_param_value_as_int (method_call, 0), method_call_get_param_value_as_int (method_call, 1),  &fault_error, &fault_code, channel);

	/* check error reply looking at the fault_error */
	if (fault_error != NULL) {
		/* we have a error reply */
		return CREATE_FAULT_REPLY (fault_code, fault_error);
	}

	/* return reply generated */
	return CREATE_OK_REPLY (XML_RPC_INT_VALUE, INT_TO_PTR (result));
}

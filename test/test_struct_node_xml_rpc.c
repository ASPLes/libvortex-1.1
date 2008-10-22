/**
 * C client stub to invoke services exported by the XML-RPC component: test.
 *
 * This file was generated by xml-rpc-gen tool, from Vortex Library
 * project.
 *
 * Vortex Library homepage: http://vortex.aspl.es
 * Axl Library homepage: http://xml.aspl.es
 * Advanced Software Production Line: http://www.aspl.es
 */
#include <test_types.h>

/* (un)marshaller support functions  */
XmlRpcStruct * test_node_marshall (Node * ref, int  dealloc)
{
	XmlRpcStruct       * _result;
	XmlRpcStructMember * _member;

	/* check received reference */
	if (ref == NULL)
		return NULL;
	/* create the struct */
	_result = vortex_xml_rpc_struct_new (2);

	/* position member */
	_member = vortex_xml_rpc_struct_member_new ("position", method_value_new (XML_RPC_INT_VALUE, INT_TO_PTR (ref->position)));
	vortex_xml_rpc_struct_add_member (_result, _member);

	/* next member */
	_member = vortex_xml_rpc_struct_member_new ("next", method_value_new (XML_RPC_STRUCT_VALUE, test_node_marshall (ref->next, false)));
	vortex_xml_rpc_struct_add_member (_result, _member);

	/* dealloc data source */
	if (dealloc)
		test_node_free (ref);

	/* return result created */
	return _result;
}

Node * test_node_unmarshall (XmlRpcStruct * ref, int  dealloc)
{
	Node * _result;

	/* check received reference */
	if (ref == NULL)
		return NULL;

	/* check the number of items the provided struct */
	v_return_val_if_fail (vortex_xml_rpc_struct_get_member_count (ref) == 2, NULL);
	/* check member names */
	v_return_val_if_fail (vortex_xml_rpc_struct_check_member_names (ref, 2, "position", "next"), NULL);
	/* check member types */
	v_return_val_if_fail (vortex_xml_rpc_struct_check_member_types (ref, 2, "int", "struct"), NULL);

	_result = test_node_new (
		vortex_xml_rpc_struct_get_member_value_as_int (ref, "position"),
		test_node_unmarshall (vortex_xml_rpc_struct_get_member_value_as_struct (ref, "next"), false));

	/* dealloc data source */
	if (dealloc)
		vortex_xml_rpc_struct_free (ref);

	return _result;
}

/* memory (de)allocation functions */
Node * test_node_new (int position, Node * next)
{
	Node * _result = axl_new (Node, 1);

	_result->position = position;
	_result->next = next;
	
	return _result;
}

Node * test_node_copy (Node * ref)
{
	Node * _result = NULL;

	if (ref == NULL)
		return NULL;

	_result = axl_new (Node, 1);
	_result->position = ref->position;
	_result->next = test_node_copy (ref->next);
	
	return _result;
}

void test_node_free (Node * ref)
{
	if (ref == NULL)
		return;
	if (ref->next)
		test_node_free (ref->next);
	if (ref != NULL)
		axl_free (ref);
	return;
}


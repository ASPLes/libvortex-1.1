/* include base library */
#include <vortex.h>

/* listener context */
VortexCtx * ctx = NULL;

#if defined(ENABLE_XML_RPC_SUPPORT)
/* include xml-rpc library */
#include <vortex_xml_rpc.h>

/** 
 * @brief Validation resource function. 
 * 
 * This function is called to validate the resource
 * requested. Resources are a way to organize remote method to be
 * invoked, grouped under a common resource name.
 *
 * Default resource used is "/". 
 * 
 * @param connection The connection where the validation request was
 * received.
 *
 * @param channel_number The channel number that is being requested.
 *
 * @param serverName The serverName requested while validating
 *
 * @param resource_path Resource path received. This could be any
 * value, including path.
 * 
 * @return 
 */
int      validate_resource (VortexConnection * connection, 
			    int                channel_number,
			    const char       * serverName,
			    const char       * resource_path,
			    axlPointer         user_data)
{

	printf ("Received request to validate resource: '%s' (virtual host: serverName='%s')\n", 
		 resource_path,
		 serverName != NULL ? serverName : "no serverName value received");
	return axl_true;
}

/** 
 * @brief Final invocation function, that perform the sum operation.
 * 
 * @param a First parameter.
 *
 * @param b Second parameter.
 *
 * @param fault_error Optional fault error to be filled in the case an
 * error is found.
 *
 * @param fault_code Optinonal fault error code to be filled in the
 * case an error is found.
 * 
 * @return Return a int value.
 */
int  sum_2_int_int (int  a, int  b, char  ** fault_error, int  * fault_code)
{
	if (a == 2 && b == 7) {
		/* the function couldn't perform a sum operation for
		 * the 2 and 7 values. */
		REPLY_FAULT ("Current implementation is not allowed to sum the 2 and 7 values", -1, 0);
	}
	
	/* for the rest of cases, just sum the two incoming values */
	return a + b;
}

/** 
 * @brief Method invocation for the sum method, having 2 parameter
 * that are int values.
 * 
 * @param method_call The method call received, representing the sum
 * method.
 * 
 * @return A newly allocated \ref XmlRpcMethodResponse, having current reply.
 */
XmlRpcMethodResponse * __sum_2_int_int (XmlRpcMethodCall * method_call)
{
	/* reply value */
	int                 result;
	char              * fault_error = NULL;
	int                 fault_code  = -1;

	/* get parameters */
	int  a = method_call_get_param_value_as_int (method_call, 0);
	int  b = method_call_get_param_value_as_int (method_call, 1);

	/* perform invocation */
	result = sum_2_int_int (a, b, &fault_error, &fault_code);
	
	/* check error reply looking at the fault_error */
	if (fault_error != NULL) {
		/* we have a error reply */
		return CREATE_FAULT_REPLY (fault_code, fault_error);
	}

	/* return reply generated */
	return CREATE_OK_REPLY (ctx, XML_RPC_INT_VALUE, INT_TO_PTR (result));
	
}

/** 
 * @brief Process method call invocation, without unrefereing the \ref
 * XmlRpcMethodCall object.
 * 
 * @param channel The channel where the method invocation was received.
 * @param method_call The method call received.
 * @param user_data User space data.
 */
XmlRpcMethodResponse *  service_dispatch (VortexChannel * channel, XmlRpcMethodCall * method_call, axlPointer user_data) 
{
	
	printf ("*** channel %d, method call received %s (params %d)***\n",
		 vortex_channel_get_number (channel),
		 method_call_get_name (method_call), 
		 method_call_get_num_params (method_call));

	/* check if the incoming method call is called sum, and has
	 * two arguments */
	if (method_call_is (method_call, "sum", 2, XML_RPC_INT_VALUE, XML_RPC_INT_VALUE, -1)) {
		printf ("*** sum method found... ***\n");
		return __sum_2_int_int (method_call);
		
	}
	       
	/* return that the method to be invoked, is not supported */
	return CREATE_FAULT_REPLY (-1, "Method call received couldn't be dispatched because it not supported by this server");
}
#endif


int  main (int  argc, char ** argv) 
{

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

#if defined(ENABLE_XML_RPC_SUPPORT)
	/* enable XML-RPC profile */
	vortex_xml_rpc_accept_negotiation (ctx, 
					   validate_resource,
					   /* no user space data for
					    * the validation resource
					    * function. */
					   NULL, 
					   service_dispatch,
					   /* no user space data for
					    * the dispatch function. */
					   NULL);
#else
	printf ("Found no support for XML-RPC built, failed to run listener...\n");
	return -1;
#endif

	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);

	/* also listener on the following port */
	vortex_listener_new (ctx, "0.0.0.0", "42000", NULL, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);
	
	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}


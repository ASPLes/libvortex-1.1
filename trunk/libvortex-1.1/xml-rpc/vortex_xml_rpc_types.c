/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

/* include local main header */
#include <vortex_xml_rpc.h>

#define LOG_DOMAIN "vortex-xml-rpc"

/**
 * \defgroup vortex_xml_rpc_types Vortex XML-RPC types: XML-RPC data abstraction layer, supporting types used.
 */

/* \addtogroup vortex_xml_rpc_types */
/* @{ */

struct _XmlRpcMethodCall {
	/**
	 * @internal
	 * @brief Method name this MethodCall represents.
	 */
	char                * methodName;
	/** 
	 * @internal
	 * @brief How many parameters does this method call object have.
	 */
	int                   count;
	/** 
	 * @internal
	 *
	 * @brief An internal value to check how many parameters are
	 * added used as a sanity check to avoid setting a method
	 * invocator object more values than the ones set after.
	 */
	int                   added_count;

	/**
	 * @internal Context where the method call was created.
	 */
	VortexCtx           * ctx;

	/**
	 * @internal
	 * @brief A list of parameters for the method being invoked.
	 */
	XmlRpcMethodValue  ** params;

	/** 
	 * @internal
	 *
	 * @brief Allows to configure if the given object reference to
	 * be deallocated once performed the invocation. By default,
	 * all invocators are released, however, for serveral, and
	 * continuous invocations, this could be configured.
	 */
 	axl_bool              release_after_invoke;

	/** 
	 * @internal
	 *
	 * Internal channel number where the method call was
	 * received. This is used by the Vortex Library to perform
	 * deferred method invocation.
	 */
	VortexChannel      *  channel;

	/** 
	 * @internal
	 * 
	 * Internal message number this method call must reply to, to
	 * correlate the method reply to be generated.
	 */
	int                   msg_no;
};

struct _XmlRpcMethodResponse {
	/**
	 * @internal
	 *
	 * @brief Current method response status. In case the response
	 * is not positive the the member response contains a pointer
	 * to the XmlRpcMethodResponseFail. 
	 * 
	 * Otherwise a pointer to a unique XmlRpcMethodValue is stored
	 * on the same member.
	 */
	XmlRpcResponseStatus   status;
	/**
	 * @internal
	 * @brief Stores current method response value.
	 */
	XmlRpcMethodValue    * value;

	/** 
	 * @internal
	 * @brief Current string representation for the value stored.
	 */
	char                 * string_value;
	
	/**
	 * @internal
	 * @brief Contains the reply error found on the method response.
	 */
	int                    fault_code;
	/**
	 * @internal
	 * @brief Contains a textual diagnostic for the error found.
	 */
	char                 * fault_string;

};


struct _XmlRpcMethodValue {
	/**
	 * @internal
	 * @brief Context under which the method value was created.
	 */
	VortexCtx            * ctx;

	/**
	 * @internal
	 * @brief Represents which values is actually hold by the this param
	 */
	XmlRpcParamType type;
	/**
	 * @internal
	 *
	 * @brief An union having the right value. Remember to leave
	 * this value as the last value.
	 */
	union {
		/* with this attribute is modeled the int and the
		 * boolean value. */
		int     int_value; 
		/* with this attribute not only is modeled the string
		 * value but also the the base64 string. */
		char  * string_value;
		double  double_value;
		XmlRpcStruct * rpc_struct;
		XmlRpcArray  * rpc_array;
	} value;
};

struct _XmlRpcStruct {
	/**
	 * @internal reference counting.
	 */
	int                   refcount;

	/**
	 * @internal How many member does this struct have.
	 */
	int                   count;

	/** 
	 * @internal
	 * 
	 * An internal count for elements added at a particular
	 * moment. Used to check when new members are added and to
	 * check that the struct contains all members that are
	 * expected.
	 */
	int                   added_count;

	/**
	 * @internal
	 * @brief An array of members stored on this struct.
	 */
	XmlRpcStructMember ** members;
};

struct _XmlRpcStructMember {
	/**
	 * @internal
	 * @brief The name of the member stored inside a struct 
	 */
	char                * name;
	/**
	 * @internal
	 * @brief The value of the member stored, that is a new
	 * XmlRpcMethodValue. This allows to have not only simple
	 * values but also more struct or arrays.
	 */
	XmlRpcMethodValue   * value;
};

struct _XmlRpcArray {
	/**
	 * @internal reference counting.
	 */
	int                   refcount;

	/**
	 * @internal
	 * @brief How many array elements this array have.
	 */
	int  count;

	/** 
	 * @internal
	 * How many items were added to the array.
	 */
	int  added_count;
	/**
	 * @internal
	 * @brief An array of XmlRpcMethodValue values stored.
	 */
	XmlRpcMethodValue ** values;
};

/** 
 * @brief Creates a new method call object, representing a remote
 * procedure invocation.
 *
 * The object created by this function could be understood as a object
 * invocator, or a closure, that is, an high level object that
 * represents an method to be invoked. 
 *
 * Once the object is created by this function, it is needed (if
 * necessary) to add parameter values to it. This is done by using the
 * following functions:
 *
 *  - \ref vortex_xml_rpc_method_call_add_value
 *  - \ref vortex_xml_rpc_method_call_set_value
 *  - \ref vortex_xml_rpc_method_call_create_value
 *
 * Once the method object is completely built, then a call to the
 * following function is required to actually perform the invocation:
 * 
 *  - \ref vortex_xml_rpc_invoke
 *  - \ref vortex_xml_rpc_invoke_sync
 * 
 * Once the \ref XmlRpcMethodCall is created and used with the
 * invocation API (\ref vortex_xml_rpc_invoke_sync and \ref
 * vortex_xml_rpc_invoke), it will be deallocated automatically. This
 * is done to avoid the problem caused to deallocate this object mainly
 * for asynchronous invocation that makes really difficult to know
 * when to done this operation. 
 *
 * However, this behaviour could be configured. See \ref
 * vortex_xml_rpc_method_call_must_release. Keep in mind that the
 * invocator created by this function could be reused several times,
 * maybe to implement a kind of cache. But, if configured the
 * automatic release mechanism to not happen, you will responsible for
 * calling to \ref vortex_xml_rpc_method_call_free, when no longer
 * needed the reference.
 *
 * @param ctx Context where the method call was created.
 *
 * @param methodName The method name to invoke. This value must be not
 * NULL. The function will perform a local copy, so methodName passed
 * in value could be unrefered once this function finish.
 *
 * @param parameters The number of parameters this method invocator
 * object will contain. This value must be greater or equal to 0.
 * 
 * @return A new \ref XmlRpcMethodCall object, or NULL if it
 * fails. The function could only fail if it receive wrong parameters
 * values according to parameter description.
 */
XmlRpcMethodCall * vortex_xml_rpc_method_call_new (VortexCtx   * ctx,
						   const char  * methodName, 
						   int           parameters)
{
	XmlRpcMethodCall * result;

	/* first check environment conditions */
	v_return_val_if_fail_msg (methodName, NULL, "failed to create method call, methodName value is NULL");
	v_return_val_if_fail_msg (parameters >= 0, NULL, "failed to create mthod call, parameters have a non positive value");

	result                       = axl_new (XmlRpcMethodCall, 1);
	result->methodName           = axl_strdup (methodName);
	result->release_after_invoke = axl_true;
	result->ctx                  = ctx;

	/* set the number of parameters this method name will support
	 * in the case the value is higher than 0 */
	result->count          = parameters;
	if (result->count > 0) {
		/* only allocate memory enough to hold pointers to */
		result->params = axl_new (XmlRpcMethodValue *, result->count);
	}
	
	/* that's all men */
	return result;
}

/** 
 * @brief Allows to create a new \ref XmlRpcMethodValue object from
 * the given <i>type</i> and the given <i>value</i>.
 * 
 * You can use this function to create parameters to be added to
 * method objects (\ref XmlRpcMethodCall) using:
 *
 *  - \ref vortex_xml_rpc_method_call_add_value
 *  - \ref vortex_xml_rpc_method_call_set_value
 *
 * You can also skip this function and go directly to add the value
 * being created by this function to the method call using:
 * 
 *  - \ref vortex_xml_rpc_method_call_create_value
 *  - \ref vortex_xml_rpc_method_call_create_value_from_string
 *
 * This function returns a new \ref XmlRpcMethodValue reference which
 * should be unrefered using \ref vortex_xml_rpc_method_value_free. 
 *
 * This function will not perform a local copy for the given
 * <i>value</i> unless \ref XML_RPC_STRING_VALUE is provided. In that
 * case a local copy is made to enable the caller to unref the value
 * provided just returing from this function.
 *
 * This particular behavior is explained from the fact the other types
 * like bool or int doesn't required to perform a local copy, and
 * array and struct values are allocated through the provided
 * interface creating a copy.
 * 
 * But, there is no way to figure out if the string value received was
 * allocated on the heap or it is an stack activated value or simple
 * an static string.
 * 
 * Here are some examples:
 * \code
 * XmlRpcMethodValue * value;
 *
 * // Create a method value representing an integer value
 * value = vortex_xml_rpc_method_value_new (XML_RPC_INT_VALUE, 
 *                                          PTR_TO_INT(10));
 * // free the value created 
 * vortex_xml_rpc_method_value_free (value);
 *
 * // Create a method value representing an string 
 * value = vortex_xml_rpc_method_value_new (XML_RPC_STRING_VALUE,
 *                                          "test");
 * // free the value created 
 * vortex_xml_rpc_method_value_free (value);
 * \endcode
 * 
 * @param ctx The context where the method value is being created.
 * @param type The type to be created.
 * @param value The value associated with the type being created
 * 
 * @return A new reference to \ref XmlRpcMethodValue object or NULL if
 * it fails. The function could only fail if the given value is NULL
 * or the type provided is not a valid value found in the enumeration,
 * not including \ref XML_RPC_UNKNOWN_VALUE.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new         (VortexCtx         * ctx,
							     XmlRpcParamType     type,
							     axlPointer          value)
{
	XmlRpcMethodValue * _value;

	/* perform some environment conditions */
	v_return_val_if_fail_msg (XML_RPC_UNKNOWN_VALUE < type && type <= XML_RPC_NUM_SUPPORTED_VALUES,
				  NULL,
				  "Failed to create method value, type received is an unsupported value");
	/* check received value to not be null for pointer values */
	switch (type) {
	case XML_RPC_STRING_VALUE:
	case XML_RPC_BASE64_VALUE:
		/* check and set empty string for NULL values */
		if (value == NULL)
			value = XML_RPC_EMPTY_STR;
		break;
	case XML_RPC_STRING_REF_VALUE:
	case XML_RPC_BASE64_REF_VALUE:
		/* check and set empty string for NULL values */
		if (value == NULL)
			value = axl_strdup (XML_RPC_EMPTY_STR);
		break;
	case XML_RPC_DATE_VALUE:
		v_return_val_if_fail_msg (value, NULL, "Failed to create method value, received a NULL pointer for date value");
		break;
	default:
		/* nothing to be done for the rest of cases */
		break;
	} /* end switch */

	_value       = axl_new (XmlRpcMethodValue, 1);
	_value->type = type;
	_value->ctx  = ctx;
	
	/* save the value provided checking if we have received and string. */
	switch (type) {
	case XML_RPC_STRING_VALUE:
	case XML_RPC_BASE64_VALUE:
		_value->value.string_value = axl_strdup (value);
		break;
	case XML_RPC_STRING_REF_VALUE:
	case XML_RPC_BASE64_REF_VALUE:
		/* own the reference received, do not allocate
		 * again */
		_value->value.string_value = value;

		/* update the enumerator value */
		if (type == XML_RPC_STRING_REF_VALUE)
			_value->type = XML_RPC_STRING_VALUE;
		if (type == XML_RPC_BASE64_REF_VALUE)
			_value->type = XML_RPC_BASE64_VALUE;
		break;
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		_value->value.int_value = PTR_TO_INT (value);
		break;
	case XML_RPC_DOUBLE_VALUE:
		if (value != NULL) 
			_value->value.double_value = (double ) (* (double  *)value);
		else
			_value->value.double_value = 0;
		break;
	case XML_RPC_STRUCT_VALUE:
		/* check for null value to produce a none indicator */
		if (value == NULL) {
			/* set node value */
			_value->type  = XML_RPC_NONE_VALUE;

			/* do not set anyting more */
		}else {
			/* received a reference to the XmlRpcStruct
			 * pointer */
			_value->value.rpc_struct = value;
		}
		break;
	case XML_RPC_ARRAY_VALUE:
		/* check for null value to produce a none indicator */
		if (value == NULL) {
			/* set node value */
			_value->type  = XML_RPC_NONE_VALUE;

			/* do not set anyting more */
		}else {
			/* received a reference to the XmlRpcArray
			 * pointer */
			_value->value.rpc_array = value;
		}
		break;
	case XML_RPC_NONE_VALUE:
		/* nothing */
		break;
	default:
		/* nothing to do */
		break;
	}
	/* return the value */
	return _value;
}

/** 
 * @brief Convenience interface to \ref
 * vortex_xml_rpc_method_value_new which allows to create a new
 * XmlRpcMethodValue storing an integer value.
 *
 * @param ctx The context where the method value is being created.
 * 
 * @param value The integer value to store inside the \ref
 * XmlRpcMethodValue.
 * 
 * @return A reference to the \ref XmlRpcMethodValue holding an
 * integer value.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new_int         (VortexCtx * ctx, int   value)
{
	/* create and return the xml rpc method value */
	return vortex_xml_rpc_method_value_new (ctx, XML_RPC_INT_VALUE, INT_TO_PTR (value));
}

/** 
 * @brief Convenience interface to \ref
 * vortex_xml_rpc_method_value_new which allows to create a new
 * XmlRpcMethodValue storing a double value.
 *
 * @param ctx The context where the method value is being created.
 * 
 * @param value The double value to store inside the \ref
 * XmlRpcMethodValue.
 * 
 * @return A reference to the \ref XmlRpcMethodValue holding a double
 * value.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new_double      (VortexCtx         * ctx, 
								 double              value)
{
	/* create and return the xml method value */
	return vortex_xml_rpc_method_value_new (ctx, XML_RPC_DOUBLE_VALUE, &value);
}

/** 
 * @brief Convenience interface to \ref
 * vortex_xml_rpc_method_value_new which allows to create a new
 * XmlRpcMethodValue storing a bool value.
 *
 * @param ctx The context where the method value is being created.
 * 
 * @param value The double value to store inside the \ref
 * XmlRpcMethodValue.
 * 
 * @return A reference to the \ref XmlRpcMethodValue holding a bool
 * value.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new_bool        (VortexCtx         * ctx, axl_bool                 value)
{
	/* create and return the xml method value */
	return vortex_xml_rpc_method_value_new (ctx, XML_RPC_BOOLEAN_VALUE, INT_TO_PTR (value ? 1 : 0));
}

/** 
 * @brief Allows to perform a copy from the provided method value.
 * 
 * @param value The method value to copy (\ref XmlRpcMethodValue).
 * 
 * @return A newly allocated method value or NULL if it fails.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_copy (XmlRpcMethodValue * value)
{
	axl_return_val_if_fail (value, NULL);

	switch (value->type) {
	case XML_RPC_STRING_VALUE:
	case XML_RPC_BASE64_VALUE:
	case XML_RPC_STRING_REF_VALUE:
	case XML_RPC_BASE64_REF_VALUE:
		/* string types */
		return method_value_new (value->ctx, value->type, value->value.string_value);
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		/* int types */
		return method_value_new (value->ctx, value->type, INT_TO_PTR (value->value.int_value));
	case XML_RPC_DOUBLE_VALUE:
		return method_value_new (value->ctx, value->type, (axlPointer) &value->value.double_value);
	case XML_RPC_STRUCT_VALUE:
		/* update reference counting */
		vortex_xml_rpc_struct_ref (value->value.rpc_struct);
		return method_value_new (value->ctx, value->type, value->value.rpc_struct);
	case XML_RPC_ARRAY_VALUE:
		/* update reference counting */
		vortex_xml_rpc_array_ref (value->value.rpc_array);
		return method_value_new (value->ctx, value->type, value->value.rpc_array);
	case XML_RPC_NONE_VALUE:
		return method_value_new (value->ctx, value->type, NULL);
	default:
		/* nothing to do */
		break;
	}

	/* return null for unhandled type */
	return NULL;
}

/** 
 * @brief Allows to convert the provided \ref XmlRpcMethodValue into a
 * string representation.
 *
 * The function doesn't support complex values: struct and array. It
 * only support converting method values from \ref XML_RPC_INT_VALUE,
 * \ref XML_RPC_BOOLEAN_VALUE, \ref XML_RPC_DOUBLE_VALUE, and \ref
 * XML_RPC_STRING_VALUE string basic types.
 * 
 * @param value The method value to convert into a string representing
 * the type.
 * 
 * @return The string representing the value or NULL if it fails. The
 * returned value must be deallocated.
 */
char  * vortex_xml_rpc_method_value_stringify (XmlRpcMethodValue * value)
{
	/* check method value received */
	v_return_val_if_fail (value, NULL);

	switch (method_value_get_type (value)) {
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		return axl_strdup_printf ("%d", value->value.int_value);

	case XML_RPC_DOUBLE_VALUE:
		return axl_strdup_printf ("%g", value->value.double_value);

	case XML_RPC_DATE_VALUE:
		/* not supported yet */
		break;
	case XML_RPC_STRING_VALUE:
		return axl_strdup (value->value.string_value);

	case XML_RPC_BASE64_VALUE:
		return axl_strdup (value->value.string_value);

	case XML_RPC_STRUCT_VALUE:
		/* not supported yet */
		break;
	case XML_RPC_ARRAY_VALUE:
		/* not supported yet */
		break;
	default:
		/* nothing to do */
		break;
	}
	
	return NULL;
}

/** 
 * @brief Allows to nullify provided method value, in the case an
 * internal reference is contained to avoid deallocating it once the
 * \ref vortex_xml_rpc_method_value_free is called.
 * 
 * @param _value The method value to nullify.
 */
void                vortex_xml_rpc_method_value_nullify         (XmlRpcMethodValue * _value)
{
	/* check value received */
	v_return_if_fail (_value);

	/* save the value provided checking if we have received and string. */
	switch (_value->type) {
	case XML_RPC_STRING_VALUE:
	case XML_RPC_BASE64_VALUE:
		/* nullify the value */
		_value->value.string_value = NULL;
		break;
	case XML_RPC_STRING_REF_VALUE:
	case XML_RPC_BASE64_REF_VALUE:
		/* nullify the value */
		_value->value.string_value = NULL;
		break;
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		/* nothing to do in this case */
		break;
	case XML_RPC_DOUBLE_VALUE:
		/* nothing to do in this case */
		break;
	case XML_RPC_STRUCT_VALUE:
		/* nullify the rpc struct */
		_value->value.rpc_struct = NULL;
		break;
	case XML_RPC_ARRAY_VALUE:
		/* nullify the rpc struct */
		_value->value.rpc_array = NULL;
		break;
	default:
		/* nothing to do */
		break;
	}
	/* return the value */
	return;
}

/** 
 * @brief Creates a new \ref XmlRpcMethodValue object from the given
 * type, using as value the string provided.
 *
 * The string provided must contain a value according to the type
 * provided. See also \ref
 * vortex_xml_rpc_method_value_new_from_string2, which could create a
 * method value by providing all its values as string references.
 *
 * Currently, the following types are supported to be translated from
 * the string form into the native type representation:
 * 
 * - \ref XML_RPC_INT_VALUE
 * - \ref XML_RPC_BOOLEAN_VALUE
 * - \ref XML_RPC_DOUBLE_VALUE
 * - \ref XML_RPC_STRING_VALUE
 *
 * @param ctx The context where the method value is being created.
 * 
 * @param type The type for the new instance.
 *
 * @param string_value A string value, representing the type configured.
 * 
 * @return A newly created \ref XmlRpcMethodValue or NULL if fails.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new_from_string (VortexCtx         * ctx,
								 XmlRpcParamType     type,
								 const char        * string_value)
{
	double  double_value;
	int     int_value;
	char  * string_aux = NULL;

	switch (type) {
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		int_value = (int) strtol (string_value, &string_aux, 10);
		if ((string_aux != NULL) && (* string_aux))
			return NULL;
		return method_value_new (ctx, type, INT_TO_PTR (int_value));
	case XML_RPC_DOUBLE_VALUE:
		if ((string_aux != NULL) && (* string_aux))
			return NULL;
		double_value = vortex_support_strtod ((char *) string_value, &string_aux);
		return method_value_new (ctx, type, (axlPointer) &double_value);
	case XML_RPC_STRING_VALUE:
		return method_value_new (ctx, type, (axlPointer) string_value);
	case XML_RPC_DATE_VALUE:
		/* not supported yet */
		break;
	case XML_RPC_BASE64_VALUE:
		return method_value_new (ctx, type, (axlPointer) string_value);
	case XML_RPC_ARRAY_VALUE:
		/* not supported yet: to support this case, an string
		 * format must be defined to allow this */
		break;
	case XML_RPC_STRUCT_VALUE:
		/* not supported yet: the same applies to this case as
		 * the array case. */
		break;
	case XML_RPC_NONE_VALUE:
		/* create the none value */
		return method_value_new (ctx, type, NULL);
	default:
		/* nothing to do */
		break;
	}

	return NULL;
}

/** 
 * @brief Allows to create a new method value (\ref XmlRpcMethodValue)
 * by providing two strings which are the type and the value
 * representation.
 *
 * The type string which have the following values according to the
 * desired type value:
 *
 * <table>
 *   <tr>
 *     <td>Type </td><td>Enum type value</td><td>Value to be used</td>
 *   </tr>
 *   <tr>
 *     <td>Integer value </td><td>\ref XML_RPC_INT_VALUE</td><td>"int"</td>
 *   </tr>
 *   <tr>
 *     <td>Boolean value </td><td>\ref XML_RPC_BOOLEAN_VALUE</td><td>"boolean"</td>
 *   </tr>
 *   <tr>
 *     <td>Double value </td><td>\ref XML_RPC_DOUBLE_VALUE</td><td>"boolean"</td>
 *   </tr>
 *   <tr>
 *     <td>String value </td><td>\ref XML_RPC_STRING_VALUE</td><td>"string"</td>
 *   </tr>
 *   <tr>
 *     <td>Base64 string value </td><td>\ref XML_RPC_BASE64_VALUE</td><td>"base64"</td>
 *   </tr>
 *   <tr>
 *     <td>Struct value </td><td>\ref XML_RPC_STRUCT_VALUE</td><td>Not supported</td>
 *   </tr>
 *   <tr>
 *     <td>Array value </td><td>\ref XML_RPC_ARRAY_VALUE</td><td>Not supported</td>
 *   </tr>
 * </table>
 *
 * @param ctx The context where the method value is being created.
 * 
 * @param type An string which specifies the method value type. See the table above.
 *
 * @param string_value The string value associated to the method
 * value, which is a proper string representation of the string value.
 * 
 * @return A newly created method value, that must be deallocated by
 * using \ref vortex_xml_rpc_method_value_free. This is not required
 * if the value is set as a method parameter (for a \ref
 * XmlRpcMethodCall) or a method response (\ref
 * XmlRpcMethodResponse). The function will return NULL if some
 * parameter received is NULL or the type specification is not
 * properly formated.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new_from_string2 (VortexCtx   * ctx,
								  const char  * type,
								  const char  * string_value)
{
	/* perform some environment checks */
	v_return_val_if_fail (type, NULL);
	v_return_val_if_fail (string_value, NULL);

	if (axl_cmp ("int", type))
		return vortex_xml_rpc_method_value_new_from_string (ctx, XML_RPC_INT_VALUE, string_value);

	if (axl_cmp ("boolean", type))
		return vortex_xml_rpc_method_value_new_from_string (ctx, XML_RPC_BOOLEAN_VALUE, string_value);

	if (axl_cmp ("string", type))
		return vortex_xml_rpc_method_value_new_from_string (ctx, XML_RPC_STRING_VALUE, string_value);

	return NULL;
}

/** 
 * @brief Allows to add the given \ref XmlRpcMethodValue in the next
 * parameter position available.
 *
 * This function allows to add created \ref XmlRpcMethodValue into the
 * given \ref XmlRpcMethodCall following the position of the call
 * ordering.
 * 
 * Here is an example where a method with two parameters is created
 * and then these two parameter values are added sequentially:
 *
 * \code
 * XmlRpcMethodCall  * method_call;
 * XmlRpcMethodValue * value;
 * 
 * // create a method call object setting that it will have only 1 
 * // parameter.
 * method_call = method_call_new ("examples.getStateName", 1);
 *
 * // create the value and add it
 * value       = method_value_new (XML_RPC_INT_VALUE, 
 *                                 PTR_TO_INT(41));
 *
 * // add the value into the invocator object
 * method_call_add_value (method_call, value);
 *
 * // create the next value
 * value      = method_value_new (XML_RPC_STRING_VALUE, "test");
 *
 * // add the value into the invocation object
 * method_call_add_value (method_call, value);
 *
 * \endcode
 *
 * As we have observed from the example, the parameters are added as
 * they are created, then this function keeps tracking about the
 * position where the next element added should be stored. Moreover,
 * the function checks you don't add more parameters than the value
 * configured while creating the \ref XmlRpcMethodCall object, at \ref method_call_new.
 *
 * The function will fail if some of the parameters received are NULL
 * or the caller is trying to add more parameter than the \ref
 * XmlRpcMethodCall could accept. The function will not perform any
 * copy from data received.
 * 
 * @param method_call The \ref XmlRpcMethodCall object where the value will be added.
 * @param value       The \ref XmlRpcMethodValue to be added.
 * 
 * @return axl_true if the operation was completed, otherwise axl_false is
 * returned. 
 */
int                 vortex_xml_rpc_method_call_add_value    (XmlRpcMethodCall  * method_call,
							     XmlRpcMethodValue * value)
{
	/* perform a sanity check */
	v_return_val_if_fail (method_call, axl_false);
	v_return_val_if_fail (value, axl_false);
	v_return_val_if_fail (method_call->count != method_call->added_count, axl_false);
	
	/* add the object */
	method_call->params[method_call->added_count] = value;

	/* increase added count */
	method_call->added_count++;
	
	return axl_true;
}

/** 
 * @brief Allows to set a particular value at a particular position
 * into the parameter invocation order.
 *
 * Because \ref vortex_xml_rpc_method_call_add_value works in a
 * sequential mode, it could be inconvenient under some
 * circumstances. This function allows to skip over the sequential
 * adding feature to implement an index-based parameter configuration.
 *
 * Keep in mind that added in a index-based manner you'll have to pay
 * attention about number of parameters, its order, etc. The function
 * will not modify values used to perform sequential adding. This mean
 * that sequential and index-based adding operation should not be
 * mixed on the same method call object.
 *
 * Position values should be taken to range from 0 up to number of
 * parameters - 1. Function with 2 parameters have as available and
 * valid positions [0,1].
 *
 * Once a value is added, previous stored value, if were defined, is
 * unrefered. 
 *
 * The function will not perform any operation if received values are
 * NULL or the position provided is wrong. 
 *
 * @param method_call The method call where the index-based adding operation will be performed.
 * @param position    The position where operate
 * @param value       The value to be set.
 *
 * @return axl_true if the operation was completed, otherwise axl_false is
 * returned. The function can return axl_false if the some parameter
 * received is NULL or the position configured is not compatible with
 * the method call configuration.
 */
axl_bool            vortex_xml_rpc_method_call_set_value    (XmlRpcMethodCall  * method_call,
							     int                 position,
							     XmlRpcMethodValue * value)
{
	XmlRpcMethodValue * __previous;

	/* perform some environment condition checks */
	v_return_val_if_fail (method_call, axl_false);
	v_return_val_if_fail (value, axl_false);
	v_return_val_if_fail (0 <= position && position < method_call->count, axl_false);
	
	/* get previous value to perform a deallocation operation */
	__previous = method_call->params[position];

	/* set new value */
	method_call->params[position] = value;

	/* check and deallocate if necessary */
	if (__previous != NULL)
		vortex_xml_rpc_method_value_free (__previous);

	/* peter: hey jack, this function you have writtn' looks
	 *        pretty simple, you are a great programmer! */
	/* jack: i won't let you play with my PSP! */
	return axl_true;
}

/** 
 * @brief Convenience function to avoid XML-RPC API consumers to write to much code.
 *
 * This function will create a \ref XmlRpcMethodValue using the given
 * value and then will add it using the \ref vortex_xml_rpc_method_call_add_value.
 *
 * The function will not perform any operation if the received value
 * are NULL or the type provided doesn't meet the criteria stated at
 * \ref vortex_xml_rpc_method_value_new.
 *
 * @param method_call The method call object where the operation will be performed.
 * @param type        The type value for the method value object to be created.
 * @param value       The value associated to the method value object to be created.
 *
 * @return axl_true if the operation was completed, otherwise axl_false is
 * returned.
 */
axl_bool            vortex_xml_rpc_method_call_create_value (XmlRpcMethodCall  * method_call,
							     XmlRpcParamType     type,
							     axlPointer          value)
{
	XmlRpcMethodValue * _value;
	VortexCtx         * ctx = METHOD_CALL_CTX(method_call);

	v_return_val_if_fail_msg (method_call, axl_false, "failed to create value, method call reference received is NULL");
	
	/* create the value to be added and check if it was succesful */
	_value = vortex_xml_rpc_method_value_new (ctx, type, value);
	v_return_val_if_fail_msg (_value, axl_false, "failed to create value, call to vortex_xml_rpc_method_value_new have failed");

	/* add the value created into the method call object */
	return vortex_xml_rpc_method_call_add_value (method_call, _value);
}

/** 
 * @brief Convenience function to avoid XML-RPC API consumers to write to much code.
 * 
 * This function will create a \ref XmlRpcMethodValue using the given
 * value (through \ref vortex_xml_rpc_method_value_new_from_string)
 * and then will add it using the \ref vortex_xml_rpc_method_call_add_value.
 *
 * 
 *
 * @param method_call The method call object where the operation will be performed.
 * @param type        The type value for the method value object to be created.
 * @param string_value  The value associated to the method value object to be created, in a string form.
 *
 * @return axl_true if the operation was completed, otherwise axl_false is
 * returned.
 */
axl_bool                 vortex_xml_rpc_method_call_create_value_from_string (XmlRpcMethodCall * method_call,
									      XmlRpcParamType    type,
									      const char       * string_value)
{
	XmlRpcMethodValue * _value;
	VortexCtx         * ctx = METHOD_CALL_CTX(method_call);

	v_return_val_if_fail_msg (method_call, axl_false, "Failed to create method value from string because a method call NULL reference was received");
	v_return_val_if_fail_msg (string_value, axl_false, "Failed to create metho dvalue from string because the string received is NULL");
	
	/* create the value to be added and check if it was succesful */
	_value = vortex_xml_rpc_method_value_new_from_string (ctx, type, string_value);
	v_return_val_if_fail_msg (_value, axl_false, "Call to vortex_xml_rpc_method_value_new_from_string have failed, unable to create method value from string");

	/* add the value created into the method call object */
	return vortex_xml_rpc_method_call_add_value (method_call, _value);
}


/** 
 * @brief Allows to get current method name for the given \ref XmlRpcMethodCall object.
 *
 * The value returned by this function is the method name that will be
 * invoked. You must not unref the value returned by this function,
 * because it is an internal copy to the \ref XmlRpcMethodCall object.
 * 
 * @param method_call The method call to get its name.
 * 
 * @return An string reference representing the method name or NULL if
 * it fails. The function only fails if the method call object
 * reference provided is NULL.
 */
char              * vortex_xml_rpc_method_call_get_name           (XmlRpcMethodCall  * method_call)
{
	v_return_val_if_fail (method_call, NULL);

	return method_call->methodName;
}

/** 
 * @brief Allows to get which is the number of parameters that the
 * given \ref XmlRpcMethodCall object has.
 *
 * The parameter number returned by this function is the one specified
 * while calling to \ref vortex_xml_rpc_method_call_new. It is not the
 * number of parameter that are already added.
 *
 * You should use \ref
 * vortex_xml_rpc_method_call_get_current_num_params to get which is
 * the number of parameter that are added. A properly formated \ref
 * XmlRpcMethodCall should return the same value while calling to both
 * function.
 *
 * @param method_call The method call object to get its parameter numbers.
 * 
 * @return The function return which is the number of parameter that
 * has the object or -1 if it fails. The function only fails if the
 * given method call is NULL.
 */
int             vortex_xml_rpc_method_call_get_num_params     (XmlRpcMethodCall  * method_call)
{
	v_return_val_if_fail (method_call, -1);

	return method_call->count;
}

/** 
 * @brief Allows to get current number of parameters already added to
 * the given \ref XmlRpcMethodCall object.
 * 
 * @param method_call The method call object to get current parameter
 * number.
 * 
 * @return The number of parameter already added or -1 if fails.
 */
int             vortex_xml_rpc_method_call_get_current_num_params (XmlRpcMethodCall * method_call)
{
	v_return_val_if_fail (method_call, -1);

	return method_call->added_count;
}

/** 
 * @brief Allows to get the param value (\ref XmlRpcMethodValue) from
 * the given \ref XmlRpcMethodCall at the selected position.
 *
 * 
 *
 * @param method_call The method call object where the param value
 * will be retrieved.
 *
 * @param position The param value to get. Valid positions ranges from
 * 0 up to param number -1.
 * 
 * @return An referece to the \ref XmlRpcMethodValue or NULL if
 * fails. Returned value should not be deallocated. The function will
 * check that the given method call object is not NULL and the
 * position is in the range of {0..param number - 1}. 
 */
XmlRpcMethodValue * vortex_xml_rpc_method_call_get_param_value (XmlRpcMethodCall * method_call, 
								int  position) 
{
	v_return_val_if_fail (method_call, NULL);
	v_return_val_if_fail ((position >= 0) && (position < method_call->count), NULL);
	v_return_val_if_fail (method_call, NULL);
	
	return method_call->params[position];
}

/** 
 * @brief Allows to get the message number, unique identifier inside
 * the channel, where the xml-rpc method call was received.
 * 
 * @param method_call The method call that is required to returns is
 * msg-no value.
 * 
 * @return Current msg-no value or undefined if the method call wasn't
 * sent (the value is updated once the \ref XmlRpcMethodCall is used
 * to perform an invocation: \ref vortex_xml_rpc_invoke and \ref
 * vortex_xml_rpc_invoke_sync).
 */
int                 vortex_xml_rpc_method_call_get_msgno                 (XmlRpcMethodCall * method_call)
{
	v_return_val_if_fail (method_call, -1);

	/* return current value */
	return method_call->msg_no;
}

/** 
 * @brief Helper function that allows to get the &lt;integer> value from
 * the given position for the arguments received inside the given \ref
 * XmlRpcMethodCall object.
 *
 * This function is a short-cut to use: 
 *   - \ref vortex_xml_rpc_method_call_get_param_value 
 * and then calling to:
 *   - \ref vortex_xml_rpc_method_value_get_as_int
 *
 *
 * @param method_call The method call object where the value will be
 * extracted.
 *
 * @param position The position to check.
 * 
 * @return The function will return the value associated on the given
 * possition or -1 if fails. Keep in mind that a \ref
 * XmlRpcMethodValue with \ref XML_RPC_INT_VALUE could have as a value
 * a <b>-1</b>. So you have to ensure you are not getting a value that
 * is not an &lt;integer> or, the parameter defined by the given position
 * exists.
 */
int                 vortex_xml_rpc_method_call_get_param_value_as_int    (XmlRpcMethodCall * method_call,
									  int  position)
{
	XmlRpcMethodValue * value;

	
	/* get xml rpc method value */
	value = vortex_xml_rpc_method_call_get_param_value (method_call, position);

	/* check environment conditions */
	v_return_val_if_fail (value, -1);
	if (value->type != XML_RPC_INT_VALUE && value->type == XML_RPC_DOUBLE_VALUE) {
		return -1;
	}

	/* return an int value */
	return value->value.int_value;
}

/** 
 * @brief Allow to get current double value associated to the giveh
 * \ref XmlRpcMethodCall, at the given position for the parameter
 * value.
 *
 * If you are looking for a function to get the double inside a
 * particular \ref XmlRpcMethodValue, you can check \ref
 * vortex_xml_rpc_method_value_get_as_double.
 * 
 * @param method_call The method call where the double value will be
 * retrieved.
 *
 * @param position The parameter position to get.
 * 
 * @return A double value or 0.0 if fails. There is no easy way to get
 * that the returned 0.0 value is because an error or due to that
 * indeed, the parameter accessed have that value.
 */
double             vortex_xml_rpc_method_call_get_param_value_as_double (XmlRpcMethodCall * method_call,
									 int  position)
{
	XmlRpcMethodValue * value;
	
	/* get xml rpc method value */
	value = vortex_xml_rpc_method_call_get_param_value (method_call, position);

	/* return the value inside */
	return vortex_xml_rpc_method_value_get_as_double (value);
}

/** 
 * @brief Convenience function to the the string associated to the
 * given parameter, indentified by the position, inside the given \ref
 * XmlRpcMethodCall object.
 *
 * If you are looking for a function that returns the string inside a
 * particular \ref XmlRpcMethodValue, check \ref vortex_xml_rpc_method_value_get_as_string.
 * 
 * @param method_call The method call object.
 *
 * @param position The position of the parameter, being
 * XML_RPC_STRING_VALUE, where the value will be received.
 * 
 * @return A string associated or NULL if fails. The string returned
 * must not be deallocated. 
 */
char  *             vortex_xml_rpc_method_call_get_param_value_as_string (XmlRpcMethodCall * method_call,
									  int  position)
{
	XmlRpcMethodValue * value;
	
	/* get xml rpc method value */
	value = vortex_xml_rpc_method_call_get_param_value (method_call, position);

	/* return the string value inside, if it is possible */
	return vortex_xml_rpc_method_value_get_as_string (value);
}

/** 
 * @brief Allows to get the struct provided at the given position.
 * 
 * @param method_call The method call where the struct is being retrieved.
 *
 * @param position The position to look up.
 * 
 * @return A reference to the struct or NULL if it fails.
 */
XmlRpcStruct      * vortex_xml_rpc_method_call_get_param_value_as_struct (XmlRpcMethodCall * method_call,
									  int  position)
{
	XmlRpcMethodValue * value;
	
	/* get xml rpc method value */
	value = vortex_xml_rpc_method_call_get_param_value (method_call, position);

	/* return the string value inside, if it is possible */
	return vortex_xml_rpc_method_value_get_as_struct (value);
}

/** 
 * @brief Allows to get the XmlRpcArray inside the provided method call.
 * 
 * @param method_call The method call where the XmlRpcArray is being
 * requested.
 *
 * @param position The position to look up.
 * 
 * @return A reference to the XmlRpcArray or NULL if it fails.
 */
XmlRpcArray       * vortex_xml_rpc_method_call_get_param_value_as_array  (XmlRpcMethodCall * method_call,
									  int  position)
{
	XmlRpcMethodValue * value;
	
	/* get xml rpc method value */
	value = vortex_xml_rpc_method_call_get_param_value (method_call, position);

	/* return the string value inside, if it is possible */
	return vortex_xml_rpc_method_value_get_as_array (value);	
}

/* function prototype */
char  * vortex_xml_rpc_marshall_method_value (XmlRpcMethodValue * value);

/** 
 * @internal
 * 
 * Marshall the provided struct representation into an xml
 * representation.
 * 
 * @param _struct The struct to marshall
 * 
 * @return A memory allocated representation for the provided
 * structure.
 */
char  * vortex_xml_rpc_types_marshall_struct (XmlRpcStruct * _struct)
{
	char  * stream_aux;
	char  * stream_aux2;
	char  * stream_result;
	int     iterator;
	

	/* initial struct header */
	stream_result = axl_strdup ("<struct>");
	iterator      = 0;

	while (iterator < _struct->added_count) {

		/* marshall the method member */
		stream_aux2 = vortex_xml_rpc_marshall_method_value (_struct->members[iterator]->value);
		
		/* get the member representation */
		stream_aux  = axl_strdup_printf ("%s<member><name>%s</name>%s</member>",
						 stream_result,
						 _struct->members[iterator]->name,
						 stream_aux2);

		/* release the member method value allocated */
		axl_free (stream_aux2);
		axl_free (stream_result);

		/* update the new stream result */
		stream_result = stream_aux;

		/* update iterator count */
		iterator++;
	}
	
	/* add final representation for the struct */
	stream_aux   = stream_result;
	stream_result = axl_strdup_printf ("%s</struct>", stream_result);
	axl_free (stream_aux);
	
	return stream_result;
}


/** 
 * @internal
 *
 * Internal function which allows to marshal the provided XmlRpcArray
 * array reference into the Xml RPC presentation.
 * 
 * @param array The array to marshall.
 * 
 * @return Returns an string containing the array marshalled.
 */
char  * vortex_xml_rpc_types_marshall_array (XmlRpcArray * array)
{
	char  * stream_aux;
	char  * stream_aux2;
	char  * stream_result;
	int     iterator;

	/* initial struct header */
	stream_result = axl_strdup ("<array><data>");
	iterator      = 0;

	while (iterator < array->added_count) {

		/* marshall the method member */
		stream_aux2 = vortex_xml_rpc_marshall_method_value (array->values[iterator]);
		
		/* get the member representation */
		stream_aux  = axl_strdup_printf ("%s%s",
						 stream_result,
						 stream_aux2);

		/* release the member method value allocated */
		axl_free (stream_aux2);
		axl_free (stream_result);

		/* update the new stream result */
		stream_result = stream_aux;

		/* update iterator count */
		iterator++;
	}
	
	/* add final representation for the struct */
	stream_aux    = stream_result;
	stream_result = axl_strdup_printf ("%s</data></array>", stream_result);
	axl_free (stream_aux);
	
	return stream_result;
}

/** 
 * @internal Marshall the provided method value, returning its string
 * representation.
 * 
 * @param value The method value to marshall.
 * 
 * @return An allocated string representing the method value
 * marshalled.
 */
char  * vortex_xml_rpc_marshall_method_value (XmlRpcMethodValue * value)
{
	char  * stream_result;
	char  * stream_aux;

	switch (method_value_get_type (value)) {
	case XML_RPC_INT_VALUE:
		return axl_strdup_printf ("<value><i4>%d</i4></value>", 
					  method_value_get_as_int (value));
		break;
	case XML_RPC_BOOLEAN_VALUE:
		return axl_strdup_printf ("<value><boolean>%d</boolean></value>", 
					  method_value_get_as_int (value));
	case XML_RPC_DOUBLE_VALUE:
		return axl_strdup_printf ("<value><double>%g</double></value>", 
					  method_value_get_as_double (value));
		break;
	case XML_RPC_STRING_VALUE:
		return axl_strdup_printf ("<value><string><![CDATA[%s]]></string></value>",
					  (char  *) value->value.string_value);
		break;
	case XML_RPC_DATE_VALUE:
		/* not implemented yet */
		break;

	case XML_RPC_BASE64_VALUE:
		/* not implemented yet */
		break;
	case XML_RPC_STRUCT_VALUE:
		/* marshall the value */
		stream_aux    = vortex_xml_rpc_types_marshall_struct (value->value.rpc_struct);
		
		/* use the value marshalled */
		stream_result = axl_strdup_printf ("<value>%s</value>",
						   stream_aux);
		/* free the marshalled value */
		axl_free (stream_aux);

		return stream_result;
	case XML_RPC_ARRAY_VALUE:
		/* marshall the value */
		stream_aux    = vortex_xml_rpc_types_marshall_array (value->value.rpc_array);
		
		/* use the value marshalled */
		stream_result = axl_strdup_printf ("<value>%s</value>",
						   stream_aux);
		/* free the marshalled value */
		axl_free (stream_aux);

		return stream_result;
		break;
	case XML_RPC_NONE_VALUE:
		/* return the none value */
		return axl_strdup_printf ("<value><none /></value>");
	default:
		/* nothing to do */
		return NULL;
	}
	
	/* this point is never reached */
	return NULL;
}

/**
 * @brief Allows to get the \ref VortexCtx associated to the method
 * call object provided.
 *
 * @param method_call The method call to get the context from.
 *
 * @return A reference to the vortex context or NULL if it fails.
 */
VortexCtx         * vortex_xml_rpc_method_call_get_ctx      (XmlRpcMethodCall  * method_call)
{
	/* check and return NULL */
	if (method_call == NULL)
		return NULL;

	/* return reference currently configured */
	return method_call->ctx;
}


/** 
 * @brief Perform a marshalling from the invocator provided
 * (<b>method_call</b>) into the appropiate XML-RPC representation.
 *
 * This function allows to convert the given method call into the
 * XML-RPC representation. The function return the xml stream
 * representing the invocator in a dynamically allocated value. It
 * should be deallocated once no longer needed.
 *
 * Additionally, the function allows to set a pointer to a integer
 * (<b>size parameter</b>) so the function could return the xml stream
 * size generated. This method is preferred over getting the xml
 * stream and then using something similar to <b>strlen</b>.
 *
 * @param method_call The method invocator to marshall into the xml
 * representation.
 *
 * @param size A pointer to a integer where the size for the given
 * stream will be returned. This is an optional parameter.
 * 
 * @return The function return the xml representation or NULL if it
 * fails. The function will the that the method call object received
 * is not NULL and all parameters required were added. This means that
 * the method parameter count and parameters added count should be
 * equal.
 */
char              * vortex_xml_rpc_method_call_marshall     (XmlRpcMethodCall  * method_call,
							     int               * size)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx         * ctx = METHOD_CALL_CTX(method_call);
#endif
	XmlRpcMethodValue * value;
	char              * stream_result;
	char              * stream_aux;
	char              * stream_aux2;

	int                 iterator;

	v_return_val_if_fail_msg (method_call, NULL, 
				  "failed to marshall method call, received null reference for method call");
	v_return_val_if_fail_msg (method_call->count == method_call->added_count, NULL, 
				  "failed to marshall method call, number of parameters added do not match (added count != count)");

	/* get initial method header */
	stream_result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodCall><methodName>%s</methodName>",
					   method_call->methodName);

	/* if the method have at least one parameter, add them */
	if (method_call->count > 0) {
		/* get a refence to the previous message, add initial
		 * <params> header and release no longer used
		 * memory */
		stream_aux    = stream_result;
		stream_result = axl_strdup_printf ("%s<params>", stream_aux);
		axl_free (stream_aux);

		/* iterate over all param values */
		for (iterator = 0; iterator < method_call->count; iterator++) {
			/* get a reference to the value */
			value       = method_call_get_param_value (method_call, iterator);
			
			/* marshall the param value according to its type */
			stream_aux  = vortex_xml_rpc_marshall_method_value (value);
			
			/* get the concatenated result */
			stream_aux2 = axl_strdup_printf ("%s<param>%s</param>", stream_result, stream_aux);
			
			/* free previous data */
			axl_free (stream_aux);
			axl_free (stream_result);

			/* set the new stream result */
			stream_result = stream_aux2;

		}

		/* finally close the param values section for the
		 * invocation xml message */
		stream_aux    = stream_result;
		stream_result = axl_strdup_printf ("%s</params>", 
						   (stream_aux != NULL) ? stream_aux : NULL);
		axl_free (stream_aux);
	}

	/* close the message and return the value */
	stream_aux    = stream_result;
	stream_result = axl_strdup_printf ("%s</methodCall>", stream_aux);
	axl_free (stream_aux);
	
	/* return current result, setting current size. The following
	 * operation is based on strlen operation. This is ok because
	 * XML-RPC standard doesn't support raw binary formats. To
	 * include binary data, it should be encoded into a base64
	 * format.
	 */
	if (size != NULL) 
		(* size ) = strlen (stream_result);
	return stream_result;
}


/** 
 * @brief Unref all memory allocated by the method call object and its
 * childs, including method call values.
 *
 * This function will perform a deep unref operation on the given
 * method call object. 
 * 
 * @param method_call The method call to be deallocated.
 */
void                vortex_xml_rpc_method_call_free         (XmlRpcMethodCall  * method_call)
{
	int  iterator;
	v_return_if_fail (method_call);

	/* for each method value object, perform an unref operation over it */
	for (iterator = 0; iterator < method_call->added_count; iterator ++) {
		vortex_xml_rpc_method_value_free (method_call->params[iterator]);
	}

	/* unref params container */
	axl_free (method_call->params);
	
	/* unref methodName value */
	axl_free (method_call->methodName);
	
	/* unref the object itself */
	axl_free (method_call);
	
	return;
}

/** 
 * @brief Allows to configure how the XML-RPC invocation mechanism
 * manages invocator objects that have performed its invocation.
 *
 * By default, every \ref XmlRpcMethodCall object created to perform
 * an invocation is deallocated automatically once the invocation is
 * performed. However, if it is planned to perform the same invocation
 * several times or to implement a kind of invocation cache, then this
 * function should be used to notify Vortex Library to not release the
 * object once performed its invocation.
 *
 * @param method_call The \ref XmlRpcMethodCall object to configure.
 *
 * @param release axl_true to release the object after perform the
 * invocation, axl_false to not deallocate it.
 */
void                vortex_xml_rpc_method_call_release_after_invoke (XmlRpcMethodCall * method_call, 
								     axl_bool           release)
{
	v_return_if_fail (method_call);

	/* store the value */
	method_call->release_after_invoke = release;
	
	return;
}

/** 
 * @brief Returns current status for releasing the given object after
 * performing an invoke.
 * 
 * @param method_call The method call to check for its configuration.
 * 
 * @return axl_true if the given method call object should be deallocated
 * after the invoke is performed. Otherwise axl_false is returned.
 */
axl_bool                 vortex_xml_rpc_method_call_must_release (XmlRpcMethodCall * method_call)
{
	v_return_val_if_fail (method_call, axl_false);

	return method_call->release_after_invoke;
}

/** 
 * @brief Allows to check if the given method call meets the requirements provided.
 * 
 * Before executing a method call, this function could be used to
 * peform some basic method call recognition, based on the method
 * name, the parameter number, and tye expected type for the incoming
 * data.
 * 
 * Here is an example to validate the method called "sum" with 2
 * integer parameters:
 *
 * \code
 * if (vortex_xml_rpc_method_call_is (method_call, "sum", 2, 
 *                                    XML_RPC_INT_VALUE,
 *                                    XML_RPC_INT_VALUE, -1) {
 *     printf ("Great, this is the method we where looking for");  
 * }
 * \endcode
 *
 * Argument type validation must always finish with a <b>-1</b>
 * value. If it is only required to perform a method call recognition
 * based on the method name and its argument number without taking
 * into account the argument type validation, then <b>-1</b> must be
 * used as the last value.
 * 
 * @param method_call The method call to check. 
 *
 * @param method_name The method name to check. This value is
 * optional. If it is not provided, the method name will not be
 * checked.
 *
 * @param param_num The method parameter number to check. To disable
 * method parameter number check, -1 must be used.
 *
 * 
 * @return axl_true if method match, otherwise axl_false is returned.
 */
axl_bool            vortex_xml_rpc_method_call_is           (XmlRpcMethodCall * method_call, 
							     const char       * method_name,
							     int                param_num,
							     ...)
{
	va_list             args;
	XmlRpcParamType     type;
	int                 iterator = 0;
	XmlRpcMethodValue * value;

	v_return_val_if_fail (method_call, axl_false);
	
	/* check method name */
	if (method_name != NULL) {
		if (!axl_cmp (method_call->methodName, method_name))
			return axl_false;
	}

	/* check method parameter number */
	if (param_num != -1) {
		if (param_num != method_call->count)
			return axl_false;
	}

	/* open std arg list */
	va_start (args, param_num);

	/* now check method call type signature */
	while ((type = va_arg (args, XmlRpcParamType)) != -1) {

		/* get the value from the given position */
		value = method_call_get_param_value (method_call, iterator);
		if (value == NULL) {
			/* uhmm...it seems that it was requested to
			 * check for an argument that thet the
			 * function doesn't have. */
			va_end (args);
			return axl_false;
		}
		
		/* check type */
		if (method_value_get_type (value) != type) {
			/* method value type doesn't match */
			va_end (args);
			return axl_false;
		}
		
		/* update iterator to check */
		iterator++;
	}

	/* close the standard arg */
	va_end (args);

	return axl_true;
}

/** 
 * @internal
 *
 * @brief Allows to configure reply data to the given \ref
 * XmlRpcMethodCall object.
 */
void                __vortex_xml_rpc_method_call_set_reply_data (XmlRpcMethodCall * method_call,
								 VortexChannel    * channel,
								 int                msg_no)
{
	v_return_if_fail (method_call);
	
	method_call->channel = channel;
	method_call->msg_no  = msg_no;
	
	return;
}

/** 
 * @internal
 *
 * @brief Allows to get reply data configured in the given \ref
 * XmlRpcMethodCall object.
 */
void                __vortex_xml_rpc_method_call_get_reply_data (XmlRpcMethodCall *  method_call,
								 VortexChannel    ** channel,
								 int               * msg_no)
{
	v_return_if_fail (method_call);

	if (channel != NULL)
		(* channel ) = method_call->channel;
	if (msg_no != NULL)
		(* msg_no ) = method_call->msg_no;
	return;
}

/** 
 * @brief Perform a memory deallocation operation over the given value.
 *
 * This function will perform a deep unref operation over the given
 * value, including string stored, array, structs and all values
 * inside those holder structures.
 * 
 * @param value The method value to be destroyed.
 */
void                vortex_xml_rpc_method_value_free        (XmlRpcMethodValue * value)
{
	v_return_if_fail (value);

	/* unref internal value according to the type. */
	switch (value->type) {
	case XML_RPC_INT_VALUE:
	case XML_RPC_BOOLEAN_VALUE:
		/* do nothing */
		break;
	case XML_RPC_STRING_VALUE:
	case XML_RPC_BASE64_VALUE:
		/* perform a simple deallocation operation */
		if (value->value.string_value != NULL)
			axl_free (value->value.string_value);
		break;
	case XML_RPC_STRUCT_VALUE:
		/* struct case */
		if (value->value.rpc_struct != NULL)
			vortex_xml_rpc_struct_free (value->value.rpc_struct);
		break;
	case XML_RPC_ARRAY_VALUE:
		/* array case */
		if (value->value.rpc_array != NULL)
			vortex_xml_rpc_array_free (value->value.rpc_array);
		break;
	case XML_RPC_DATE_VALUE:
		/* date case */
		break;
	default:
		/* do nothing */
		break;
	}
	
	/* unref the node itself */
	axl_free (value);

	return;
}

/** 
 * @brief Allows to get current type for the given method value.
 *
 * @param value The \ref XmlRpcMethodValue object to get its type.
 * 
 * @return Current type the given value has or \ref
 * XML_RPC_UNKNOWN_VALUE if it fails.
 */
XmlRpcParamType     vortex_xml_rpc_method_value_get_type    (XmlRpcMethodValue * value)
{
	v_return_val_if_fail (value, XML_RPC_UNKNOWN_VALUE);

	return value->type;
}

/** 
 * @brief Allows to get current integer value from the given method
 * value object (\ref XmlRpcMethodValue).
 *
 * This function not only is used to get param values for the type
 * \ref XML_RPC_INT_VALUE. Because the XML-RPC definition says that a
 * boolean value is represented with the integer values 0 and 1,
 * corresponding to axl_false and axl_true, this function is also used to get
 * the value for a \ref XML_RPC_BOOLEAN_VALUE.
 *
 * @param value The method value to get its value as an integer.
 * 
 * @return The Integer value or -1 if it fails. Keep in mind that, the
 * stored value could be also -1. Check that you are not passing a
 * method value that is NULL or it doesn't hold an integer.
 */
int                 vortex_xml_rpc_method_value_get_as_int  (XmlRpcMethodValue * value)
{
	v_return_val_if_fail (value, -1);
	v_return_val_if_fail (value->type == XML_RPC_INT_VALUE || 
			      value->type == XML_RPC_BOOLEAN_VALUE, -1);

	/* return current internal value as an integer */
	return value->value.int_value;
}

/** 
 * @brief Allows to get the string inside the method value, supposing
 * the method value represents an \ref XML_RPC_STRING_VALUE.
 * 
 * @param value The method value where the value will be returned.
 * 
 * @return An internal reference to the string inside the method value
 * or NULL if it values. The value returned must not be deallocated.
 */
char  *             vortex_xml_rpc_method_value_get_as_string (XmlRpcMethodValue * value)
{
	/* check environment conditions */
	v_return_val_if_fail (value, NULL);
	v_return_val_if_fail (value->type == XML_RPC_STRING_VALUE, NULL);

	/* return an int value */
	return value->value.string_value;
}

/** 
 * @brief Allows to get the string inside the method value, supposing
 * the method value represents an \ref XML_RPC_STRING_VALUE.
 * 
 * @param value The method value where the value will be returned.
 * 
 * @return A newly allocated internal reference to the string inside
 * the method value or NULL if it fails. The value returned must be
 * deallocted by the caller (using axl_free) as it is supposed to be a
 * reference owed by the caller.
 */
char  *             vortex_xml_rpc_method_value_get_as_string_alloc (XmlRpcMethodValue * value)
{
	/* get the string value */
	char * result = vortex_xml_rpc_method_value_get_as_string (value);
	
	/* check, alloc and return */
	if (result != NULL)
		return axl_strdup (result);
	return NULL;
}

/** 
 * @brief Allows to get the double value inside the provided \ref XmlRpcMethodValue.
 * 
 * @param value The method value where the operation is being
 * requested.
 * 
 * @return The double inside the method value.
 */
double              vortex_xml_rpc_method_value_get_as_double   (XmlRpcMethodValue * value)
{
	/* check environment conditions */
	v_return_val_if_fail (value, 0.0);
	v_return_val_if_fail (value->type == XML_RPC_DOUBLE_VALUE, 0.0);

	/* return an int value */
	return value->value.double_value;
}

/** 
 * @brief Allows to get the struct (\ref XmlRpcStruct)  inside the provided \ref XmlRpcMethodValue.
 * 
 * @param value The method value which is expected to have an struct
 * inside (\ref XmlRpcStruct).
 * 
 * @return An internal reference to the struct inside the given \ref
 * XmlRpcMethodValue or NULL if it fails. The returned reference must
 * not be deallocated.
 */
XmlRpcStruct     * vortex_xml_rpc_method_value_get_as_struct   (XmlRpcMethodValue * value)
{
	/* check environment conditions */
	v_return_val_if_fail (value, NULL);
	v_return_val_if_fail (value->type == XML_RPC_STRUCT_VALUE, NULL);

	/* return an int value */
	return value->value.rpc_struct;
}

/** 
 * @brief Allows to get the array (\ref XmlRpcArray)  inside the provided \ref XmlRpcMethodValue.
 * 
 * @param value The method value which is expected to have an array
 * inside (\ref XmlRpcArray).
 * 
 * @return An internal reference to the array inside the given \ref
 * XmlRpcMethodValue or NULL if it fails. The returned reference must
 * not be deallocated.
 */
XmlRpcArray       * vortex_xml_rpc_method_value_get_as_array    (XmlRpcMethodValue * value)
{
	/* check environment conditions */
	v_return_val_if_fail (value, NULL);
	v_return_val_if_fail (value->type == XML_RPC_ARRAY_VALUE, NULL);

	/* return an int value */
	return value->value.rpc_array;
}

/** 
 * @brief Allows to create a new \ref XmlRpcMethodResponse object,
 * representing a request reply having either a \ref XmlRpcMethodValue
 * or a error code with a fault string.
 * 
 * Usually this function is not called directly by API consumers. This
 * function is used by the Vortex XML-RPC implementation to create
 * \ref XmlRpcMethodResponse object from data received as a reply or
 * due to a failure to report.
 * 
 * @param status The status code to be used for this method
 * response. If the the status value used is \ref XML_RPC_OK, then the
 * object returned will be a positive reply containing the given \ref
 * XmlRpcMethodValue reference received. In the case the status is not
 * \ref XML_RPC_OK, then the object generated will be a negative
 * reply containing an status code with an error message.
 *
 * @param fault_code The status code to be used according to the
 * value of the <i>status</i> value received.
 *
 * @param fault_string The status message to be set according to the
 * value of the <i>status</i> value received. This value, in the case
 * it is provided, will be copied. Caller could deallocate the string
 * provided just after returning this function.
 *
 * @param value The \ref XmlRpcMethodValue to be set if the given
 * <i>status</i> value is \ref XML_RPC_OK. The reference provided to
 * this function will not be duplicated and the \ref
 * XmlRpcMethodResponse object should be the unique owner of the
 * reference received. Keep in mind that the value received will be
 * deallocated once called to \ref
 * vortex_xml_rpc_method_response_free.
 * 
 * @return A new \ref XmlRpcMethodResponse object that must be
 * deallocated by using \ref vortex_xml_rpc_method_response_free. The
 * function could fail, returning NULL if either a \ref XML_RPC_OK
 * status is used and the value reference is null or the status value
 * is not \ref XML_RPC_OK and the fault_string is null.
 */
XmlRpcMethodResponse * vortex_xml_rpc_method_response_new  (XmlRpcResponseStatus  status,
							    int                   fault_code,
							    const char          * fault_string,
							    XmlRpcMethodValue   * value)
{
	XmlRpcMethodResponse * response;
	
	if (status == XML_RPC_OK) {
		v_return_val_if_fail (value != NULL, NULL);
	} /* end if */

	if (status != XML_RPC_OK) {
		v_return_val_if_fail (fault_string != NULL, NULL);
	} /* end if */
	
	response         = axl_new (XmlRpcMethodResponse, 1);
	response->status = status;

	/* returning a ok object */
	if (status == XML_RPC_OK)
		response->value = value;

	/* returning a failure object */
	if (status != XML_RPC_OK) {
		response->fault_code   = fault_code;
		response->fault_string = axl_strdup (fault_string);
	}
	/* return object created */
	return response;
}

/** 
 * @brief Creates a new positive \ref XmlRpcMethodResponse object,
 * inserting a \ref XmlRpcMethodValue instance created by providing
 * the <b>type</b> and <b>value</b> this function receives.
 * 
 * @param ctx The context where the response is being created.
 *
 * @param type The value type.
 *
 * @param value The pointer type.
 * 
 * @return A newly allocated \ref XmlRpcMethodResponse object,
 * containing a positive reply, with the a \ref XmlRpcMethodValue
 * inside it. NULL is returned if fail.
 */
XmlRpcMethodResponse * vortex_xml_rpc_method_response_create (VortexCtx       * ctx,
							      XmlRpcParamType   type,
							      axlPointer        value)
{

	XmlRpcMethodValue    * _value;
	XmlRpcMethodResponse * response;
	

	/* creates a new method value from received values */
	_value    = method_value_new (ctx, type, value);
	v_return_val_if_fail (_value, NULL);

	/* returns current method response value */
	response = method_response_new (XML_RPC_OK, -1, NULL, _value);
	if (response == NULL)
		method_value_free (_value);
	
	/* return current status */
	return response;
	
}

/** 
 * @brief Deallocates the given \ref XmlRpcMethodResponse object.
 *
 * @param response The object to deallocate.
 */
void vortex_xml_rpc_method_response_free (XmlRpcMethodResponse * response)
{
	v_return_if_fail (response);

	/* deallocates internal data */
	if (response->value != NULL)
		vortex_xml_rpc_method_value_free (response->value);

	if (response->string_value != NULL)
		axl_free (response->string_value);

	if (response->fault_string != NULL)
		axl_free (response->fault_string);
	
	/* deallocates the node itself */
	axl_free (response);
	
	return;
}

/** 
 * @brief Nullifies the internal \ref XmlRpcMethodValue reference hold
 * by the \ref XmlRpcMethodResponse reference.
 *
 * This function is useful if the method value is required to be
 * extracted from the \ref XmlRpcMethodResponse, so this variable can
 * be deallocated. Here is an example:
 *
 * \code
 * XmlRpcMethodValue * value;
 *  
 * // get method value inside 
 * value = vortex_xml_rpc_method_response_get_value (response);
 * 
 * // nullify internal reference 
 * vortex_xml_rpc_method_response_nullify (response);
 *
 * // release response without releasing method value 
 * vortex_xml_rpc_method_response_free (response);
 * \endcode
 * 
 * @param response The XmlRpcMethodResponse instance to nullify its
 * reference to the \ref XmlRpcMethodValue.
 */
void                   vortex_xml_rpc_method_response_nullify          (XmlRpcMethodResponse * response)
{
	/* check values */
	v_return_if_fail (response);
	
	/* nullify  */
	response->value = NULL;

	return;
}

/** 
 * @brief Allows to perform a marshalling operation into the XML
 * representation for the given \ref XmlRpcMethodResponse object.
 * 
 * @param response The object to marshall into the a XML representation.
 * @param size If the param is not NULL, it will be filled with the message size.
 * 
 * @return A newly allocated string containig the XML representation
 * for the given \ref XmlRpcMethodResponse.
 */
char                 * vortex_xml_rpc_method_response_marshall         (XmlRpcMethodResponse * response,
									int                  * size)
{
	char  * result     = NULL;
	char  * string_aux = NULL;
	
	/* check received method response */
	v_return_val_if_fail (response, NULL);

	switch (response->status) {
	case XML_RPC_OK:
		/* check that the XmlRpcMethodValue is defined */
		v_return_val_if_fail (response->value, NULL);
		switch (response->value->type) {
		case XML_RPC_INT_VALUE:
			/* implement the int value */
			result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value><i4>%d</i4></value></param></params></methodResponse>",
						    response->value->value.int_value);
			break;
		case XML_RPC_BOOLEAN_VALUE:
			/* implement the boolean value */
			result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value><boolean>%d</boolean></value></param></params></methodResponse>",
						    response->value->value.int_value);
			
			break;
		case XML_RPC_DOUBLE_VALUE:
			/* implement the double value */
			result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value><double>%g</double></value></param></params></methodResponse>",
						    response->value->value.double_value);
			break;
		case XML_RPC_STRING_VALUE:
		case XML_RPC_STRING_REF_VALUE:
			/* implement the string value */
			result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value><string><![CDATA[%s]]></string></value></param></params></methodResponse>",
						    response->value->value.string_value);
			break;
		case XML_RPC_DATE_VALUE:
			/* the already not implemented */
			break;
		case XML_RPC_BASE64_VALUE:
		case XML_RPC_BASE64_REF_VALUE:
			/* implemented as string, by performing the
			 * translation automatically */
			result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value><base64>%s</base64></value></param></params></methodResponse>",
						    response->value->value.string_value);
			break;
		case XML_RPC_STRUCT_VALUE:
			/* marshall the struct */
			string_aux = vortex_xml_rpc_types_marshall_struct (response->value->value.rpc_struct);
			result     = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value>%s</value></param></params></methodResponse>",
							string_aux);
			axl_free (string_aux);
			break;
		case XML_RPC_ARRAY_VALUE:
			/* marshall the array received */
			string_aux = vortex_xml_rpc_types_marshall_array (response->value->value.rpc_array);
			result     = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><params><param><value>%s</value></param></params></methodResponse>",
						      string_aux);
			axl_free (string_aux);
			break;
		default:
			/* nothing to do, seems an error */
			return NULL;
			   
		}
		/* reply generated */
		break;
	default:
		/* generate a error reply */
		result = axl_strdup_printf ("<?xml version=\"1.0\"?><methodResponse><fault><value><struct><member><name>faultCode</name><value><int>%d</int></value></member><member><name>faultString</name><value><string><![CDATA[%s]]></string></value></member></struct></value></fault></methodResponse>",
					    response->fault_code, response->fault_string);
		/* negative replies */
		break;
	}
	
	/* get size for the xml content generated */
	if (result != NULL && size != NULL)
		(* size ) = strlen (result);
	
	/* return what we have generated */
	return result;
}

/** 
 * @brief Returns current status for the invocation performed which
 * has return the given \ref XmlRpcMethodResponse.
 * 
 * @param response The method response to get its status.
 * 
 * @return Current Status or \ref XML_RPC_UNKNOWN_ERROR if the given
 * response is NULL.
 */
XmlRpcResponseStatus vortex_xml_rpc_method_response_get_status (XmlRpcMethodResponse * response)
{
	v_return_val_if_fail (response, XML_RPC_UNKNOWN_ERROR);

	return response->status;
}

/** 
 * @brief Allows to get a string representation for the \ref
 * XmlRpcMethodValue inside the given \ref XmlRpcMethodResponse.
 *
 * The string returned must be not deallocated. This is already done
 * while calling to \ref vortex_xml_rpc_method_response_free.
 *
 * This function can only be called if the method response object
 * status is \ref XML_RPC_OK.
 * 
 * @param response The method response to get its internal value as an string.
 * 
 * @return The string internal representation or NULL if the incoming
 * \ref XmlRpcMethodResponse status is not \ref XML_RPC_OK.
 */
char  * vortex_xml_rpc_method_response_stringify (XmlRpcMethodResponse * response)
{
	v_return_val_if_fail (response, NULL);
	v_return_val_if_fail (response->status == XML_RPC_OK, NULL);

	if (response->string_value != NULL)
		return response->string_value;

	/* get method value representation */
	response->string_value = vortex_xml_rpc_method_value_stringify (response->value);

	/* return current internal string value representation */
	return response->string_value;
}

/** 
 * @brief Allows to get the \ref XmlRpcMethodValue inside the provided \ref XmlRpcMethodResponse.
 *
 * @param response The method response where the value will be
 * extracted. The method response must be a positive reply (\ref
 * XML_RPC_OK).
 * 
 * @return An internal reference to the method value inside (\ref
 * XmlRpcMethodValue) or NULL if it fails. The returned reference must
 * not be deallocated.
 */
XmlRpcMethodValue    * vortex_xml_rpc_method_response_get_value        (XmlRpcMethodResponse * response)
{
	v_return_val_if_fail (response, NULL);
	v_return_val_if_fail (response->status == XML_RPC_OK, NULL);

	/* return method value inside */
	return response->value;
}

/** 
 * @brief Get current fault code from the given \ref XmlRpcMethodResponse object.
 *
 * This function must only be used when the \ref XmlRpcMethodResponse
 * object status is not \ref XML_RPC_OK.
 * 
 * @param response The method response object to get fault code from.
 * 
 * @return The fault code or -1 if fail. 
 */
int     vortex_xml_rpc_method_response_get_fault_code (XmlRpcMethodResponse * response)
{
	v_return_val_if_fail (response, -1);
	v_return_val_if_fail (response->status != XML_RPC_OK, -1);

	return response->fault_code;
}

/** 
 * @brief Get current fault string from the given \ref XmlRpcMethodResponse object.
 *
 * This function must only be used when the \ref XmlRpcMethodResponse
 * object status is not \ref XML_RPC_OK.
 *
 * @param response The method response object to get fault string from.
 * 
 * @return The fault string. String value returned from this function
 * must not be deallocated.
 */
char  * vortex_xml_rpc_method_response_get_fault_string (XmlRpcMethodResponse * response)
{
	v_return_val_if_fail (response, NULL);
	v_return_val_if_fail (response->status != XML_RPC_OK, NULL);

	return response->fault_string;
}

/** 
 * @brief Allows to create an empty xml rpc struct.
 *
 * Once created the struct object with this function, a call to the
 * next function will be required to fill it with struct members:
 *
 *  - \ref vortex_xml_rpc_struct_add_member
 *
 * The previous function will require a reference to a \ref
 * XmlRpcStructMember, which could be created using:
 *
 *  - \ref vortex_xml_rpc_struct_member_new
 *
 * An XML-RPC struct could support holding members of any type,
 * including nesting new structs or arrays. 
 *
 * @param count The number of members that the struct will
 * contain. This value must range from 1 up to the desired value. 
 * 
 * @return A newly allocated struct reference that must be deallocated
 * once no longer needed using \ref vortex_xml_rpc_struct_free. 
 */
XmlRpcStruct         * vortex_xml_rpc_struct_new                       (int  count)
{
	XmlRpcStruct * value;

	/* check environment conditions */
	v_return_val_if_fail (count > 0, NULL);

	/* create a new reference */
	value          = axl_new (XmlRpcStruct, 1);
	value->count   = count;
	value->members = axl_new (XmlRpcStructMember *, count);

	/* reference counting */
	value->refcount = 1;

	return value;
}

/**
 * @brief Allows to update reference counting on the provided xml rpc
 * struct. To decrease a reference updated by this function call to
 * \ref vortex_xml_rpc_struct_free.
 *
 * @param _struct The XmlRpcStruct to update by one its reference
 * counting.
 */
axl_bool                     vortex_xml_rpc_struct_ref                       (XmlRpcStruct * _struct)
{
	v_return_val_if_fail (_struct, axl_false);
	
	/* update reference counting */
	_struct->refcount++;

	return axl_true;
}

/** 
 * @brief Allows to get the number of members added to the provided \ref XmlRpcStruct.
 * 
 * @param _struct The struct that is required to returns its member
 * count.
 * 
 * @return The number of members the provided struct has or -1 if it
 * fails.
 */
int                    vortex_xml_rpc_struct_get_member_count          (XmlRpcStruct * _struct)
{
	v_return_val_if_fail (_struct, -1);

	/* return the number of member items */
	return _struct->count;
}

/** 
 * @brief Checks the provided strings, using as upper limit the
 * provided count, as members for the provided struct.
 * 
 * @param _struct The struct reference to check.
 * @param member_count The number of members to check.
 * 
 * @return axl_true if the provided names are equal on its values and its
 * order for the ones found on the provided struct. Otherwise, axl_false
 * if it fails.
 */
axl_bool               vortex_xml_rpc_struct_check_member_names        (XmlRpcStruct * _struct,
									int            member_count,
									...)
{
	char  * member_name;
	int     iterator = 0;
	va_list args;

	v_return_val_if_fail (_struct, axl_false);
	v_return_val_if_fail (_struct->count == member_count, axl_false);

	/* open std arg list */
	va_start (args, member_count);

	/* now check method call type signature */
	while (iterator < member_count) {

		/* get the member name */
		member_name = va_arg (args, char  *);
		
		/* check the member name */
		if (! axl_cmp (_struct->members[iterator]->name, member_name)) {
			/* close the standard arg */
			va_end (args);

			/* seems that the provided member is not equal */
			return axl_false;
		}
		
		/* next member */
		iterator++;
	}
	
	/* close the standard arg */
	va_end (args);

	return axl_true;
}

/** 
 * @brief Allows to check the member types, for the provided
 * structure, including the order.
 *
 * @param _struct The struct to check its members.
 * @param member_count The member count to check.
 * 
 * @return axl_true if all types received matched with members
 * stored. Otherwise axl_false is returned.
 */
axl_bool                    vortex_xml_rpc_struct_check_member_types        (XmlRpcStruct * _struct,
									     int            member_count,
									     ...)
{
	char      * member_type;
	int         iterator = 0;
	int         error = axl_false;
	va_list     args;

	v_return_val_if_fail (_struct, axl_false);
	v_return_val_if_fail (_struct->count == member_count, axl_false);

	/* open std arg list */
	va_start (args, member_count);

	/* now check method call type signature */
	while (iterator < member_count) {

		/* get the member name */
		member_type = va_arg (args, char  *);

		/* check the member type */
		if (axl_cmp (member_type, "int") && _struct->members[iterator]->value->type != XML_RPC_INT_VALUE)
			error = axl_true;

		if (axl_cmp (member_type, "boolean") && _struct->members[iterator]->value->type != XML_RPC_BOOLEAN_VALUE) {
			error = axl_true;
		}

		if (axl_cmp (member_type, "string") && _struct->members[iterator]->value->type != XML_RPC_STRING_VALUE)
			error = axl_true;

		if (axl_cmp (member_type, "double") && _struct->members[iterator]->value->type != XML_RPC_DOUBLE_VALUE)
			error = axl_true;

		if (axl_cmp (member_type, "base64") && _struct->members[iterator]->value->type != XML_RPC_BASE64_VALUE)
			error = axl_true;

		if (axl_cmp (member_type, "date") && _struct->members[iterator]->value->type != XML_RPC_DATE_VALUE)
			error = axl_true;

		if (axl_cmp (member_type, "struct")                                 && 
		    _struct->members[iterator]->value->type != XML_RPC_STRUCT_VALUE &&
		    _struct->members[iterator]->value->type != XML_RPC_NONE_VALUE )
			error = axl_true;

		if (axl_cmp (member_type, "array")                                  && 
		    _struct->members[iterator]->value->type != XML_RPC_ARRAY_VALUE  && 
		    _struct->members[iterator]->value->type != XML_RPC_NONE_VALUE )
			error = axl_true;

		/* check for error found and return */
		if (error) {
			/* close the standard arg */
			va_end (args);

			/* seems that the provided member is not equal */
			return axl_false;
		}

		/* next member */
		iterator++;
	}
	
	/* close the standard arg */
	va_end (args);

	return axl_true;
}

/** 
 * @brief Allows to create a new instance that represents an struct member.
 *
 * The function creates a new struct member which is named by the
 * given name, and holds an \ref XmlRpcMethodValue reference, which
 * could contain any method value supported.
 *
 * The reference created could be deallocated by calling to \ref
 * vortex_xml_rpc_struct_free which not only releases memory allocated
 * by the \ref XmlRpcStruct instance but also for all of its members
 * represented by \ref XmlRpcStructMember.
 *
 * Additionally you can also use \ref vortex_xml_rpc_struct_member_free.
 * 
 * @param name The member name. The function will perform a local copy
 * for the name provided.
 *
 * @param value The \ref XmlRpcMethodValue that will contain the meber
 * being created. The function will create a reference to a newly
 * created \ref XmlRpcStructMember that will own the \ref
 * XmlRpcMethodValue reference provided. No local copy is done for
 * this value.
 * 
 * @return A newly allocated reference to a \ref XmlRpcStructMember
 * that must be deallocated by using \ref
 * vortex_xml_rpc_struct_member_free or let it to be deallocated by
 * calling to \ref vortex_xml_rpc_struct_free, once the result is
 * added as a member to the struct released. The function will return NULL if some parameter received is NULL.
 */
XmlRpcStructMember   * vortex_xml_rpc_struct_member_new                (const char  * name, 
									XmlRpcMethodValue * value)
{
	XmlRpcStructMember * result;

	v_return_val_if_fail (name, NULL);
	v_return_val_if_fail (value, NULL);

	/* create a result reference */
	result        = axl_new (XmlRpcStructMember, 1);
	result->name  = axl_strdup (name);
	result->value = value;
	
	/* return the result created */
	return result;
}


/** 
 * @brief Adds a \ref XmlRpcMethodValue created to the provided \ref XmlRpcStruct instance as a new member.
 *
 * Because an struct (\ref XmlRpcStruct) is only a container for named
 * values (\ref XmlRpcStructMember) called members, this function
 * allows to add more values.
 *
 * Keep in mind that the value is added on the next free position, but
 * this is not important since the XML-RPC standard states that there
 * is no order inside a struct, that is, two structs are equivalent if
 * the have the same members no matter how they are ordered.
 *
 * The function will also check if the member added could be supported
 * by the struct. The struct, at time creation (\ref
 * vortex_xml_rpc_struct_new), is configured to have a particular
 * number of members.
 * 
 * @param _struct The struct (\ref XmlRpcStruct) where the member (\ref XmlRpcStructMember) will be added.
 * 
 * @param member The member to be added.
 */
void                   vortex_xml_rpc_struct_add_member                (XmlRpcStruct * _struct, 
									XmlRpcStructMember * member)
{
	/* some environment checks */
	v_return_if_fail (_struct);
	v_return_if_fail (member);
	v_return_if_fail (_struct->added_count < _struct->count);

	/* add the member */
	_struct->members [_struct->added_count] = member;

	/* increase the cound */
	_struct->added_count++;
	
	/* job done */
	return;
}

/** 
 * @internal
 * 
 * Internal function to get the struct member inside, looking by its name.
 */
XmlRpcStructMember * __vortex_xml_rpc_struct_get_member_by_name (XmlRpcStruct * _struct,
								 const char   * member_name)
{
	int iterator = 0;
	XmlRpcStructMember * member;

	/* perform some environment checks */
	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (member_name, NULL);

	/* lookup for the member */
	while (iterator < _struct->added_count) {
		
		/* get a reference to the member */
		member = _struct->members[iterator];

		/* check the member name */
		if (axl_cmp (member->name, member_name))
			return member;

		/* update the iterator */
		iterator++;
	}

	/* return that no member wasn't found */
	return NULL;	
}

/** 
 * @brief Allows to get the method value at the provided member for
 * the given struct reference.
 * 
 * @param _struct The struct where the member value identified by
 * member_name will be returned.
 *
 * @param member_name The member name to return.
 * 
 * @return A reference to the method value at the member or null if it
 * fails.
 */
XmlRpcMethodValue    * vortex_xml_rpc_struct_get_member_value          (XmlRpcStruct       * _struct,
									const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (member_name, NULL);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, NULL);
	
	/* get the integer inside */
	return member->value;
}

/** 
 * @brief Allows to get the member at the given position (ranging from
 * 0 up to n -1) inside the given struct.
 * 
 * @param _struct The struct where the member is requested.
 *
 * @param position The position where to look for a member.
 * 
 * @return The method value, at the provided position, or NULL if
 * fail.
 */
XmlRpcMethodValue    * vortex_xml_rpc_struct_get_member_value_at       (XmlRpcStruct       * _struct,
									int                  position)
{
	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (position >= 0 && position < _struct->count, NULL);

	/* get the method value, at the provided position */
	return _struct->members[position]->value;
}

/** 
 * @brief Allows to get the nember name for the member inside the
 * struct located at the given position.
 * 
 * @param _struct The struct that is requested to return the member
 * name at the provided position.
 *
 * @param position The position that is requested (value range from 0
 * up to n - 1).
 * 
 * @return The member name reference or NULL if it fails. The value
 * returned must not be deallocated because it is an internal
 * reference.
 */
char      * vortex_xml_rpc_struct_get_member_name_at        (XmlRpcStruct       * _struct,
							     int                  position)
{
	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (position >= 0 && position < _struct->count, NULL);

	/* get the method value, at the provided position */
	return _struct->members[position]->name;
}

/** 
 * @brief Allows to get the integer value associated to the provided
 * member name, inside the provided struct.
 *
 * See also:
 * - \ref vortex_xml_rpc_struct_get_member_value_as_string
 * - \ref vortex_xml_rpc_struct_get_member_value_as_double
 * - \ref vortex_xml_rpc_struct_get_member_value_as_struct
 * - \ref vortex_xml_rpc_struct_get_member_value_as_array
 *
 * @param _struct The struct where the member value is being required.
 *
 * @param member_name The member name to get its value.
 * 
 * @return The integer value requested. The function will always
 * return -1 if some parameter provided is NULL. There is no way to
 * check that a -1 returned is due to an error or due to be the value
 * which is actually stored inside the struct at the provided
 * member. Check that all parameters provided are not NULL to get a
 * properly function.
 */
int                    vortex_xml_rpc_struct_get_member_value_as_int   (XmlRpcStruct       * _struct,
									const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, -1);
	v_return_val_if_fail (member_name, -1);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, -1);
	
	/* get the integer inside */
	return vortex_xml_rpc_method_value_get_as_int (member->value);
	
}

/** 
 * @brief Allows to get the string value stored in a selected member at the provided structure.
 * 
 * See also:
 *
 * - \ref vortex_xml_rpc_struct_get_member_value_as_double
 * - \ref vortex_xml_rpc_struct_get_member_value_as_struct
 * - \ref vortex_xml_rpc_struct_get_member_value_as_array
 * - \ref vortex_xml_rpc_struct_get_member_value_as_int
 * 
 * @param _struct The struct where the operation will be performed.
 * @param member_name The member name to look up for the string
 * inside.
 * 
 * @return An internal reference to the string stored inside the
 * member selected. Returned result must not be deallocated. The
 * function will return NULL if the provided parameters are NULL or
 * the member doesn't exist.
 */
char  *                vortex_xml_rpc_struct_get_member_value_as_string   (XmlRpcStruct       * _struct,
									   const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (member_name, NULL);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, NULL);
	
	/* get the integer inside */
	return vortex_xml_rpc_method_value_get_as_string (member->value);
}

/** 
 * @brief Return the double value inside the provided struct at the selected member.
 *
 * See also:
 *
 * - \ref vortex_xml_rpc_struct_get_member_value_as_string
 * - \ref vortex_xml_rpc_struct_get_member_value_as_struct
 * - \ref vortex_xml_rpc_struct_get_member_value_as_array
 * - \ref vortex_xml_rpc_struct_get_member_value_as_int
 * 
 *
 * @param _struct The struct where the operation will be performed.
 * @param member_name The member name to look up for the double value
 * inside.
 * 
 * @return Returns the double inside the struct or 0.0 if it fails. 
 */
double                     vortex_xml_rpc_struct_get_member_value_as_double   (XmlRpcStruct       * _struct,
									       const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, -1);
	v_return_val_if_fail (member_name, -1);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, -1);
	
	/* get the integer inside */
	return vortex_xml_rpc_method_value_get_as_double (member->value);
}

/** 
 * @brief Allows to get the struct inside the provided member on the provided struct.
 * 
 * See also:
 * - \ref vortex_xml_rpc_struct_get_member_value_as_int
 * - \ref vortex_xml_rpc_struct_get_member_value_as_string
 * - \ref vortex_xml_rpc_struct_get_member_value_as_double
 * - \ref vortex_xml_rpc_struct_get_member_value_as_array
 * 
 * @param _struct The struct where the operation will be performed.
 * @param member_name The member name to look up for the struct
 * inside.
 * 
 * @return An internal reference to the struct stored inside the
 * provided member or NULL if fails. 
 */
XmlRpcStruct              * vortex_xml_rpc_struct_get_member_value_as_struct   (XmlRpcStruct       * _struct,
										const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (member_name, NULL);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, NULL);

	/* check for none value */
	if (method_value_get_type (member->value) == XML_RPC_NONE_VALUE)
		return NULL;
	
	/* get the integer inside */
	return vortex_xml_rpc_method_value_get_as_struct (member->value);
}

/** 
 * @brief Allows to get the array inside the provided structure, at the select member.
 * 
 * See also:
 *
 * - \ref vortex_xml_rpc_struct_get_member_value_as_int
 * - \ref vortex_xml_rpc_struct_get_member_value_as_string
 * - \ref vortex_xml_rpc_struct_get_member_value_as_struct
 * - \ref vortex_xml_rpc_struct_get_member_value_as_double
 * 
 * @param _struct The struct where the operation will be performed.
 * @param member_name The member name to look up for the array
 * inside.
 * 
 * @return Returns an internal reference to the array stored inside
 * the member or NULL if it fails. The result must not be deallocated.
 */
XmlRpcArray              * vortex_xml_rpc_struct_get_member_value_as_array   (XmlRpcStruct       * _struct,
									      const char         * member_name)
{
	XmlRpcStructMember * member;

	v_return_val_if_fail (_struct, NULL);
	v_return_val_if_fail (member_name, NULL);
	
	/* get the member */
	member = __vortex_xml_rpc_struct_get_member_by_name (_struct, member_name);
	v_return_val_if_fail (member, NULL);
	
	/* get the integer inside */
	return vortex_xml_rpc_method_value_get_as_array (member->value);
}


/** 
 * @brief Allows to deallocate the provided struct reference (\ref
 * XmlRpcStruct), including all of its members.
 * 
 * @param _struct The struct to be deallocated.
 */
void                    vortex_xml_rpc_struct_free                      (XmlRpcStruct * _struct)
{
	int iterator = 0;

	/* do nothing if a NULL reference is received */
	if (_struct == NULL)
		return;

	/* decrease reference counting and check if 0 is reached */
	_struct->refcount--;
	if (_struct->refcount != 0)
		return;

	/* release items added */
	while (iterator < _struct->added_count) {

		/* release the member pointed by the iterator
		 * position */
		vortex_xml_rpc_struct_member_free (_struct->members[iterator]);

		/* upgrade the iterator */
		iterator++;
	}

	/* free the members array */
	axl_free (_struct->members);
	
	/* release the memory hold by the struct itself */
	axl_free (_struct);
	
	return;
}

/** 
 * @brief Allows to deallocate the member (\ref XmlRpcStructMember)
 * provided.
 * 
 * @param member The member to release. The function will also release
 * the \ref XmlRpcMethodValue inside using \ref
 * vortex_xml_rpc_method_value_free.
 */
void                    vortex_xml_rpc_struct_member_free               (XmlRpcStructMember * member)
{
	/* do nothing if a NULL reference is received */
	if (member == NULL)
		return;

	/* free the member name */
	axl_free (member->name);

	/* free the value inside */
	vortex_xml_rpc_method_value_free (member->value);

	/* free the node itself */
	axl_free (member);

	return;
}

/** 
 * @brief Allows to create a new \ref XmlRpcArray reference that will
 * contain the provided number of item.
 *
 * The \ref XmlRpcArray is created to support \ref XmlRpcMethodValue
 * items inside. Several functions are provided to get access to the
 * content and configure it:
 *
 *  - \ref vortex_xml_rpc_array_count
 *  - \ref vortex_xml_rpc_array_max_count
 *  - \ref vortex_xml_rpc_array_get
 *  - \ref vortex_xml_rpc_array_set
 *
 * Once the \ref XmlRpcArray is not required, it must be deallocated
 * by using:
 *
 * - \ref vortex_xml_rpc_array_free
 * 
 * @param count The maximum number of items that the XmlRpcArray will
 * contain. This value is not optional and must be greater (or equal
 * to) 0.
 * 
 * @return A newly allocated reference to a \ref XmlRpcArray.
 */
XmlRpcArray          * vortex_xml_rpc_array_new                        (int  count)
{
	XmlRpcArray * result;

	/* check conditions */
	v_return_val_if_fail (count >= 0, NULL);

	/* create the reference */
	result           = axl_new (XmlRpcArray, 1);
	result->count    = count;
	result->refcount = 1;

	/* allocate memory to hold method values */
	if (count > 0) 
		result->values = axl_new (XmlRpcMethodValue *, count);

	/* return the result */
	return result;
}

/**
 * @brief Allows to update reference counting on the provided xml rpc
 * array. To decrease a reference updated by this function call to
 * \ref vortex_xml_rpc_array_free.
 *
 * @param array The XmlRpcArray to update by one its reference
 * counting.
 *
 * @return axl_true if the function was able to update the reference,
 * otherwise axl_false is returned.
 */
axl_bool                     vortex_xml_rpc_array_ref                       (XmlRpcArray * array)
{
	v_return_val_if_fail (array, axl_false);
	
	/* update reference counting */
	array->refcount++;

	return axl_true;
}


/** 
 * @brief Deallocates the provided \ref XmlRpcArray reference.
 *
 * @param array The \ref XmlRpcArray array to destroy.
 */
void                   vortex_xml_rpc_array_free                       (XmlRpcArray * array)
{
	int  iterator;

	v_return_if_fail (array);

	/* decrease reference counting and check if 0 is reached */
	array->refcount--;
	if (array->refcount != 0)
		return;

	/* release all method values inside */
	for (iterator = 0; iterator < array->added_count; iterator++) {
		
		/* release the method value */
		vortex_xml_rpc_method_value_free (array->values[iterator]);
	}
	
	/* release the array */
	axl_free (array->values);
	axl_free (array);
	
	return;
}

/** 
 * @brief Returns the number of items inside the provided \ref XmlRpcArray.
 *
 * This function will return the actual number of items the array
 * have. This is different from the maximum number of items added to
 * the array. To get the maximum number of items check \ref vortex_xml_rpc_array_max_count).
 * 
 * @param array The array where the number of items will be returned.
 * 
 * @return The nuber of items inside the array. The function will
 * return -1 if it fails.
 */
int                    vortex_xml_rpc_array_count                      (XmlRpcArray * array)
{
	/* check conditions */
	v_return_val_if_fail (array, -1);

	/* return the result required */
	return array->added_count;

}

/** 
 * @brief Returns the maximum number of items inside the provided
 * array, that is, the number of items that could hold the provided
 * array (the value configured at \ref vortex_xml_rpc_array_new).
 * 
 * @param array The array that is being requested for the maximum
 * count.
 * 
 * @return The number of items that the array could hold. The function
 * will return -1 if it fails.
 */
int                    vortex_xml_rpc_array_max_count (XmlRpcArray * array)
{
	/* check conditions */
	v_return_val_if_fail (array, -1);

	/* return the result required */
	return array->count;
}

/** 
 * @brief Returns the reference of the \ref XmlRpcMethodValue inside
 * the provided \ref XmlRpcArray, at the provided position.
 * 
 * @param array The array where the data inside, at the given position
 * will be returned.
 *
 * @param index The index being requested. It must be from 0 up to N -
 * 1, where N is the result get from \ref vortex_xml_rpc_array_count.
 * 
 * @return A reference to the \ref XmlRpcMethodValue or NULL if it
 * fails. The reference returned must not be deallocated. It will be
 * deallocated once the \ref vortex_xml_rpc_array_free is called.
 */
XmlRpcMethodValue    * vortex_xml_rpc_array_get                        (XmlRpcArray * array,
									int  index)
{
	/* check the index */
	v_return_val_if_fail (index >= 0 && index < array->added_count, NULL);
	
	/* return the method value */
	return array->values[index];
}

/** 
 * @brief Configures the provided \ref XmlRpcMethodValue at the
 * provided position, for the selected \ref XmlRpcArray.
 *
 * If a method value where previously set, it will be replaced by the
 * new reference provided. In the same context, the old reference will
 * be deallocated.
 *
 * @param array The array to be configured.
 * @param index The index where the value will be placed.
 * @param value The method value to set.
 *
 */
void                   vortex_xml_rpc_array_set                        (XmlRpcArray * array,
									int  index,
									XmlRpcMethodValue * value)
{
	/* check the index */
	v_return_if_fail (index >= 0 && index < array->count);
	
	/* set the method value */
	array->values[index] = value;
	
	/* return */
	return;
}

/** 
 * @brief Addes the next item (\ref XmlRpcMethodValue) to the provided
 * \ref XmlRpcArray.
 *
 * The function could fail if more items that the maximum items
 * (vortex_xml_rpc_array_new) configured are added.
 * 
 * @param array The \ref XmlRpcArray where the item will be added.
 *
 * @param value The \ref XmlRpcMethodValue that will be added. The
 * reference received will be owned by the \ref XmlRpcArray, so it
 * could be deallocated once called \ref vortex_xml_rpc_array_free.
 *
 */
void                   vortex_xml_rpc_array_add                        (XmlRpcArray * array,
									XmlRpcMethodValue * value)
{
	/* check conditions */
	v_return_if_fail (array->added_count < array->count);

	/* add the item and increase the count */
	array->values [array->added_count] = value;
	array->added_count ++;

	/* nothing more */
	return;
}

/* @} */

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
#ifndef __VORTEX_XML_RPC_TYPES_H__
#define __VORTEX_XML_RPC_TYPES_H__

/**
 * \addtogroup vortex_xml_rpc_types
 * @{ 
 */

/** 
 * @brief Values supported to send as parameters for a method
 * invocation and a method response.
 *
 * These are the values supported by the XML-RPC protocol while
 * sending parameters for a method and receiving a result from them
 * inside a response. 
 * 
 * Some of them are plain values like \ref XML_RPC_INT_VALUE and some
 * of them allow to hold more plain values such as \ref
 * XML_RPC_STRUCT_VALUE.
 */
typedef enum {
	/** 
	 * @brief Allows to represent the unknown value to report
	 * error conditions. User application shouldn't use this value
	 * directly.
	 */
	XML_RPC_UNKNOWN_VALUE = 1,
	/** 
	 * @brief Represents a int value, four bype signed integer. An
	 * example could be: -12.
	 */
	XML_RPC_INT_VALUE     = 2,
	/** 
	 * @brief Represents a boolean value, 0 (axl_false), 1 (axl_true). 
	 */
	XML_RPC_BOOLEAN_VALUE = 3,
	/** 
	 * @brief Represents a double-precision signed floating. An
	 * example could be: -12.214.
	 */
	XML_RPC_DOUBLE_VALUE  = 4,
	/** 
	 * @brief Representing an string value. An example could be
	 * "Hello world!!".
	 */
	XML_RPC_STRING_VALUE  = 5,
	/** 
	 * @brief Represents a date time, ISO8601 value. An example
	 * could be: 19980717T14:08:55.
	 */
	XML_RPC_DATE_VALUE    = 6,
	/** 
	 * @brief Represents an binary base64 encoded string. Because
	 * XML doesn't allow all characters available this type could
	 * be used to encode such characters set not supported and
	 * sent it as a base64, xml supported, character set.
	 */
	XML_RPC_BASE64_VALUE  = 7,
	/** 
	 * @brief Represents an structure of named values called
	 * members. Each member is composed by a name and a value
	 * which in turn could be another struct, an array or plain
	 * values such int, bool or string.
	 */
	XML_RPC_STRUCT_VALUE  = 8,
	/** 
	 * @brief Represents an array holding non-named values with an
	 * index-based access semantic. An array could contain not
	 * only plain values such as int, bool or string but also
	 * contains structs or more arrays. Array are not enforced to
	 * contain the same values, you could store a int, a boolean
	 * and an array value inside the same array.
	 */
	XML_RPC_ARRAY_VALUE   = 9,
	/** 
	 * @brief This type represents an string that is already
	 * allocated, avoiding to perform double allocations (and to
	 * support static strings, see \ref XML_RPC_STRING_VALUE).
	 *
	 * This method value enumeration is mainly used by \ref
	 * vortex_xml_rpc_method_value_new to provide aw way to notify
	 * the function to not allocate the value again but to just
	 * use the reference received.
	 */
	XML_RPC_STRING_REF_VALUE = 10,
	/** 
	 * @brief This type represents a base64 string that is already
	 * allocated, avoiding to perform double allocations (and to
	 * support static string).
	 */
	XML_RPC_BASE64_REF_VALUE = 11,
	/** 
	 * @brief This type represents the NULL reference for pointer types.
	 * 
	 * This value allows to represent the null reference (the
	 * undefined value) for pointer types. The following are
	 * considered pointer types:
	 * 
	 *  - \ref XML_RPC_STRING_VALUE
	 *  - \ref XML_RPC_BASE64_VALUE
	 *  - \ref XML_RPC_STRUCT_VALUE
	 *  - \ref XML_RPC_ARRAY_VALUE
	 *
	 * Because services returning (or receiving) this values could
	 * have not defined this values, this type allows to notify
	 * the peer side that a NULL reference was required to be
	 * marshalled. 
	 * 
	 * <b>NOTE: </b> This type is not inside the XML-RPC standard
	 * and falls outside its definition.
	 */
	XML_RPC_NONE_VALUE = 12,
	/** 
	 * @internal
	 *
	 * @brief This value is only for internal Vortex Library
	 * XML-RPC profile implementation purposes. It is used to
	 * detect buggy parameters that are not found inside the range
	 * defined by this enumeration. If a new type is added, that
	 * should not happen until XML-RPC is updated, this enum value
	 * must be the last one.
	 */
	XML_RPC_NUM_SUPPORTED_VALUES
} XmlRpcParamType;

/** 
 * @brief Represents current XmlRpc status operation.
 */
typedef enum {
	/** 
	 * @brief Unknown error while performing XML-RPC
	 * operations. This error code is used to signal bad
	 * parameters situation and wrong environment conditions that
	 * don't fall into the other categories.
	 */
	XML_RPC_UNKNOWN_ERROR = -1,
	/** 
	 * @brief The XmlRpc operation have finished properly.
	 */
	XML_RPC_OK = 1,
	/** 
	 * @brief The XmlRpc operation have failed.
	 */
	XML_RPC_FAIL = -2,
	/** 
	 * @brief An invocation over a channel that is not in the boot
	 * status was performed.
	 */
	XML_RPC_CHANNEL_NOT_READY = -3,
	/** 
	 * @brief An invocation has been detected over a channel that
	 * is not running the XML-RPC profile.
	 */
	XML_RPC_NOT_XML_RPC_CHANNEL = -4,
	/** 
	 * @brief An XML-RPC invocation was detected over a channel
	 * that is actually waiting for a previous invocation.
	 */
	XML_RPC_WAITING_PREVIOUS = -5,
	/** 
	 * @brief An error have happened while sending the XML-RPC request message.
	 */
	XML_RPC_INVOCATION_FAILURE = -6,
	/** 
	 * @brief Invocation timeout, timeout have been reached while waiting for method response.
	 */
	XML_RPC_TIMEOUT_ERROR = -7,
	/** 
	 * @brief This error is generated once a XML-RPC was performed
	 * but  the reply received  is badly  formated, or  it doesn't
	 * follow the XML-RPC.com standard. 
	 */
	XML_RPC_BAD_REPLY_RECEIVED = -8,
	/** 
	 * @brief Error generated by the &lt;fault> reply supported by the XML-RPC invocation.
	 *
	 * This error means that the invocation request was properly
	 * sent, and a reply was properly received, but containing a
	 * negative reply, which contains an struct with two fields:
	 * an error reply (the faultCode) and an an error string (the
	 * faultString).
	 */
	XML_RPC_FAULT_REPLY = -9
} XmlRpcResponseStatus;


/** 
 * @brief Represents an abstraction of a remote procedure invocation.
 * 
 * This object represents the method name to be invoke, and the
 * parameters it requires in a way that it is passed to the xml-rpc
 * invoke API, enabling it to marshall its values into the XML-RPC
 * reference format to be sent to the remote peer.
 *
 * To create an object of this type you have to use the following:
 *  - \ref vortex_xml_rpc_method_call_new (or its alias \ref method_call_new)
 *
 * Once created, you have to add parameter values, only if
 * required. Method could have no parameters.
 *
 * Parameters are represented by \ref XmlRpcMethodValue
 * instances. There are several function to create method values, here
 * are some:
 * 
 *   - \ref vortex_xml_rpc_method_value_new
 *   - \ref vortex_xml_rpc_method_value_new_from_string
 *
 * Once the method parameter (\ref XmlRpcMethodValue) is created, it
 * has to be added to the method call object (\ref XmlRpcMethodCall)
 * by using:
 * 
 *  - \ref vortex_xml_rpc_method_call_add_value
 *
 * Once the method call is complete, the invocation could be done by
 * using:
 * 
 *  - \ref vortex_xml_rpc_invoke
 *  - \ref vortex_xml_rpc_invoke_sync
 *
 * Once the object is not required, it must be deallocated:
 * 
 *  - \ref vortex_xml_rpc_method_call_free
 *
 * Every method value (\ref XmlRpcMethodValue) added to the method
 * call is not required to be deallocated, this is already done by the
 * previous step.
 */
typedef struct _XmlRpcMethodCall      XmlRpcMethodCall;

/** 
 * @brief Abstraction of a value stored inside a method call or a method response.
 *
 * There are two main structures inside the XML-RPC model: The method
 * invocator (\ref XmlRpcMethodCall) and the method response (\ref
 * XmlRpcMethodResponse).
 *
 * Both have values inside, the first represents method parameters,
 * while for the second, represents a value response.
 * 
 * The way method values and method responses are represented is \ref
 * XmlRpcMethodValue.
 *
 * It is an abstraction to hold a value, that could be manipulated by
 * the same API.
 * 
 * There are several ways to create a method value. Here are some:
 * 
 *  - \ref vortex_xml_rpc_method_value_new
 *  - \ref vortex_xml_rpc_method_value_new_from_string
 *  - \ref vortex_xml_rpc_method_value_new_from_string2
 *
 * Once the method value is not required, a call to \ref
 * vortex_xml_rpc_method_value_free could be done. 
 * 
 * Because method values are usually added to method responses or
 * method calls, a deallocation operation is not required because this
 * is already done once the deallocation operation is perfomed on the
 * structure that holds the value.
 *
 * There are several function to get the value actually stored inside
 * the method value:
 * 
 *  - \ref vortex_xml_rpc_method_value_get_as_int
 *  - \ref vortex_xml_rpc_method_value_get_as_string (also used for base64)
 *  - \ref vortex_xml_rpc_method_value_get_as_double
 *  - \ref vortex_xml_rpc_method_value_get_as_struct
 *  - \ref vortex_xml_rpc_method_value_get_as_array
 *
 * Finally, you can get the type of the value stored inside by calling
 * to:
 *
 *  - \ref vortex_xml_rpc_method_value_get_type
 */
typedef struct _XmlRpcMethodValue     XmlRpcMethodValue;

/** 
 * @brief Abstraction to represent a method response object.
 * 
 * Once a XML-RPC method is invoked, a reply is done by using this
 * type definition. 
 *
 * Method response is a combination of a type that support error
 * reporting, including a error integer code and a textual error
 * dianogstic, and a type that includes inside a single \ref XmlRpcMethodValue.
 *
 * Because a method could be executed returning a proper execution or
 * reporting a error value, a call to \ref
 * vortex_xml_rpc_method_response_get_status is required to check if
 * the method response is \ref XML_RPC_OK or anything else.
 *
 * For the case a \ref XML_RPC_OK is received, a call to:
 * 
 *  - \ref vortex_xml_rpc_method_response_get_value
 *  
 * If method response is a negative reply, that is, anything else than
 * \ref XML_RPC_OK, the following functions are provided to get the
 * faultCode and the faultString, representing the error reported by
 * the remote peer side:
 * 
 *   - \ref vortex_xml_rpc_method_response_get_fault_code
 *   - \ref vortex_xml_rpc_method_response_get_fault_string
 *
 * Finally, while programming at the listener side, and to enable
 * programmers to report the fault code and the fault string, a
 * macro is provided: \ref REPLY_FAULT.
 */
typedef struct _XmlRpcMethodResponse  XmlRpcMethodResponse;

/** 
 * @brief Abstraction of a special value representing an struct.
 *
 * An opaque type representation used to model struct type defined
 * inside the XML-RPC standard. 
 *
 * An struct type is a familiar construction for C, C++ and C#
 * programmers which consist of a set of named members \ref
 * XmlRpcStructMember which holds a value (\ref XmlRpcMethodValue).
 *
 * To create a struct the following function is used:
 *  - \ref vortex_xml_rpc_struct_new
 * 
 * Then new struct members are added using: 
 *  - \ref vortex_xml_rpc_struct_add_member
 *
 * Some functions are provided to get the value inside the members
 * (\ref XmlRpcStructMember) stored inside a provided struct (\ref
 * XmlRpcStruct):
 * 
 *  - \ref vortex_xml_rpc_struct_get_member_value_as_int
 *  - \ref vortex_xml_rpc_struct_get_member_value_as_string
 *  - \ref vortex_xml_rpc_struct_get_member_value_as_double
 *  - \ref vortex_xml_rpc_struct_get_member_value_as_struct
 *  - \ref vortex_xml_rpc_struct_get_member_value_as_array
 *
 * Once the struct is no longer needed, a call to \ref
 * vortex_xml_rpc_struct_free could be done. 
 *
 * Remember that this is not required if the struct is added to a
 * method value (\ref XmlRpcMethodValue).
 */
typedef struct _XmlRpcStruct          XmlRpcStruct;

/** 
 * @brief Abstraction of a single member inside a struct.
 *
 * See \ref XmlRpcStruct for more information. This opaque type
 * represents a single member to be stored inside an \ref
 * XmlRpcStruct.
 *
 * It is composed by a name (the member name) and the value associated
 * to the member (\ref XmlRpcMethodValue). 
 *
 * To create a member a call to the following function is required:
 * 
 *  - \ref vortex_xml_rpc_struct_member_new 
 *
 * Then, to add the member to a selected struct a call to the
 * following function is required: \ref
 * vortex_xml_rpc_struct_add_member. 
 *
 * Keep in mind not adding the same reference twice, inside the same
 * or different struct instances.
 */
typedef struct _XmlRpcStructMember    XmlRpcStructMember;

/** 
 * @brief Abstraction of an array of values.
 *
 * This type definition allows to hold \ref XmlRpcMethodValue
 * inside. The following functions are using the create and deallocate
 * an \ref XmlRpcArray reference:
 *
 * - \ref vortex_xml_rpc_array_new
 * - \ref vortex_xml_rpc_array_free
 *
 * The next functions are used to check the maximum item count that
 * the array could hold and the items that are actually added:
 * 
 * - \ref vortex_xml_rpc_array_count
 * - \ref vortex_xml_rpc_array_max_count
 *
 * Finally, the following function are used to add items, or to
 * get/set items at a selected position:
 *
 * - \ref vortex_xml_rpc_array_add
 * - \ref vortex_xml_rpc_array_set
 * - \ref vortex_xml_rpc_array_get
 */
typedef struct _XmlRpcArray           XmlRpcArray;

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_new
 */
#define method_call_new       vortex_xml_rpc_method_call_new

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_new.
 */
#define method_value_new      vortex_xml_rpc_method_value_new

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_copy
 */
#define method_value_copy       vortex_xml_rpc_method_value_copy

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_add_value.
 */
#define method_call_add_value vortex_xml_rpc_method_call_add_value

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_set_value.
 */
#define method_call_set_value vortex_xml_rpc_method_call_set_value

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_create_value.
 */
#define method_call_create_value vortex_xml_rpc_method_call_create_value

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_create_value_from_string.
 */
#define method_call_create_value_from_string vortex_xml_rpc_method_call_create_value_from_string

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_name.
 */
#define method_call_get_name vortex_xml_rpc_method_call_get_name

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_num_params.
 */
#define method_call_get_num_params vortex_xml_rpc_method_call_get_num_params

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_current_num_params.
 */
#define method_call_get_current_num_params vortex_xml_rpc_method_call_get_current_num_params

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value.
 */
#define method_call_get_param_value vortex_xml_rpc_method_call_get_param_value

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_msgno.
 */
#define method_call_get_msgno       vortex_xml_rpc_method_call_get_msgno

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_marshall.
 */
#define method_call_marshall vortex_xml_rpc_method_call_marshall

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_free.
 */
#define method_call_free vortex_xml_rpc_method_call_free

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_is.
 */
#define method_call_is vortex_xml_rpc_method_call_is

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value_as_int.
 */
#define method_call_get_param_value_as_int vortex_xml_rpc_method_call_get_param_value_as_int

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value_as_double.
 */
#define method_call_get_param_value_as_double vortex_xml_rpc_method_call_get_param_value_as_double

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value_as_string.
 */
#define method_call_get_param_value_as_string vortex_xml_rpc_method_call_get_param_value_as_string

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value_as_struct.
 */
#define method_call_get_param_value_as_struct vortex_xml_rpc_method_call_get_param_value_as_struct

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_call_get_param_value_as_array.
 */
#define method_call_get_param_value_as_array vortex_xml_rpc_method_call_get_param_value_as_array

/** 
 * @brief XML-RPC protocol have a design that do not allow to send
 * NULL values or "none". Although, several relevant languages such
 * SQL or python allows to configure a "none" value for every type,
 * that is, the absence of any value, inside XML-RPC it is required to
 * provide a value for all members or variables exchanged.
 *
 * This macro allows to provide an empty string which is used when it
 * is required to send "empty" values (or not significant). The macro
 * is just a simple redifinition of "", but allows to clearly state
 * that the operation is invoking or returning a non-significant
 * value.
 */
#define XML_RPC_EMPTY_STR ""

/*
 * The following header definitions comes to allow operations over
 * \ref XmlRpcMethodCall object.
 */
XmlRpcMethodCall  * vortex_xml_rpc_method_call_new          (VortexCtx         * ctx,
							     const char        * methodName,
							     int                 parameters);

axl_bool            vortex_xml_rpc_method_call_add_value    (XmlRpcMethodCall  * method_call,
							     XmlRpcMethodValue * value);

axl_bool            vortex_xml_rpc_method_call_set_value    (XmlRpcMethodCall  * method_call,
							     int                 position,
							     XmlRpcMethodValue * value);

axl_bool            vortex_xml_rpc_method_call_create_value (XmlRpcMethodCall  * method_call,
							     XmlRpcParamType     type,
							     axlPointer            value);

axl_bool            vortex_xml_rpc_method_call_create_value_from_string (XmlRpcMethodCall * method_call,
									 XmlRpcParamType    type,
									 const char       * string_value);

char              * vortex_xml_rpc_method_call_get_name                  (XmlRpcMethodCall  * method_call);

int                 vortex_xml_rpc_method_call_get_num_params            (XmlRpcMethodCall  * method_call);

int                 vortex_xml_rpc_method_call_get_current_num_params    (XmlRpcMethodCall  * method_call);

XmlRpcMethodValue * vortex_xml_rpc_method_call_get_param_value           (XmlRpcMethodCall * method_call, 
									  int  position);

int                 vortex_xml_rpc_method_call_get_msgno                 (XmlRpcMethodCall * method_call);

int                 vortex_xml_rpc_method_call_get_param_value_as_int    (XmlRpcMethodCall * method_call,
									  int  position);

double              vortex_xml_rpc_method_call_get_param_value_as_double (XmlRpcMethodCall * method_call,
									  int  position);

char              * vortex_xml_rpc_method_call_get_param_value_as_string (XmlRpcMethodCall * method_call,
									  int  position);

XmlRpcStruct      * vortex_xml_rpc_method_call_get_param_value_as_struct (XmlRpcMethodCall * method_call,
									  int  position);

XmlRpcArray       * vortex_xml_rpc_method_call_get_param_value_as_array  (XmlRpcMethodCall * method_call,
									  int  position);

/**
 * @brief Allows to get the \ref VortexCtx associated to the method
 * call object provided.
 *
 * @param mc The method call to get the context from.
 *
 * @return A reference to the vortex context or NULL if it fails.
 */
#define METHOD_CALL_CTX(mc) (vortex_xml_rpc_method_call_get_ctx (mc))

VortexCtx         * vortex_xml_rpc_method_call_get_ctx      (XmlRpcMethodCall  * method_call);

char              * vortex_xml_rpc_method_call_marshall     (XmlRpcMethodCall  * method_call,
							     int               * size);

void                vortex_xml_rpc_method_call_free         (XmlRpcMethodCall  * method_call);

void                vortex_xml_rpc_method_call_release_after_invoke (XmlRpcMethodCall * method_call, 
								     axl_bool           release);

axl_bool            vortex_xml_rpc_method_call_must_release (XmlRpcMethodCall * method_call);

axl_bool            vortex_xml_rpc_method_call_is           (XmlRpcMethodCall * method_call, 
							     const char       * method_name,
							     int                param_num,
							     ...);

/** 
 * @internal
 *
 * @brief The following two function are for internal Vortex Library
 * method invocation deferring model. The first one allows to set the
 * channel number and the message number to be use while replying. The
 * second one is used to get that data to actually do reply.
 */
void                __vortex_xml_rpc_method_call_set_reply_data (XmlRpcMethodCall * method_call,
								 VortexChannel    * channel,
								 int                msg_no);

void                __vortex_xml_rpc_method_call_get_reply_data (XmlRpcMethodCall * method_call,
								 VortexChannel    ** channel,
								 int              * msg_no);

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_free.
 */
#define method_value_free vortex_xml_rpc_method_value_free

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_type.
 */
#define method_value_get_type vortex_xml_rpc_method_value_get_type

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_as_int.
 */
#define method_value_get_as_int vortex_xml_rpc_method_value_get_as_int

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_as_string.
 */
#define method_value_get_as_string vortex_xml_rpc_method_value_get_as_string

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_as_double.
 */
#define method_value_get_as_double vortex_xml_rpc_method_value_get_as_double

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_as_struct.
 */
#define method_value_get_as_struct vortex_xml_rpc_method_value_get_as_struct

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_get_as_array.
 */
#define method_value_get_as_array vortex_xml_rpc_method_value_get_as_array

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_value_new_from_string.
 */
#define method_value_new_from_string vortex_xml_rpc_method_value_new_from_string

/*
 * The fllowing header definitions comes to allow operations over \ref
 * XmlRpcMethodValue objects.
 */
XmlRpcMethodValue * vortex_xml_rpc_method_value_new             (VortexCtx         * ctx,
								 XmlRpcParamType     type,
								 axlPointer          value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_new_int         (VortexCtx         * ctx,
								 int                 value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_new_double      (VortexCtx         * ctx,
								 double              value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_new_bool        (VortexCtx         * ctx,
								 axl_bool            value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_copy            (XmlRpcMethodValue * value);

char              * vortex_xml_rpc_method_value_stringify       (XmlRpcMethodValue * value);

void                vortex_xml_rpc_method_value_nullify         (XmlRpcMethodValue * _value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_new_from_string (VortexCtx         * ctx,
								 XmlRpcParamType     type,
								 const char        * string_value);

XmlRpcMethodValue * vortex_xml_rpc_method_value_new_from_string2 (VortexCtx   * ctx,
								  const char  * type,
								  const char  * string_value);

XmlRpcParamType     vortex_xml_rpc_method_value_get_type        (XmlRpcMethodValue * value);

int                 vortex_xml_rpc_method_value_get_as_int      (XmlRpcMethodValue * value);

char  *             vortex_xml_rpc_method_value_get_as_string   (XmlRpcMethodValue * value);

char  *             vortex_xml_rpc_method_value_get_as_string_alloc (XmlRpcMethodValue * value);

double              vortex_xml_rpc_method_value_get_as_double   (XmlRpcMethodValue * value);

XmlRpcStruct      * vortex_xml_rpc_method_value_get_as_struct   (XmlRpcMethodValue * value);

XmlRpcArray       * vortex_xml_rpc_method_value_get_as_array    (XmlRpcMethodValue * value);

void                vortex_xml_rpc_method_value_free            (XmlRpcMethodValue * value);


/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_new
 */
#define method_response_new vortex_xml_rpc_method_response_new

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_free
 */
#define method_response_free vortex_xml_rpc_method_response_free

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_get_status
 */
#define method_response_get_status vortex_xml_rpc_method_response_get_status

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_stringify
 */
#define method_response_stringify vortex_xml_rpc_method_response_stringify

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_get_fault_code
 */
#define method_response_get_fault_code vortex_xml_rpc_method_response_get_fault_code

/** 
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_get_fault_string
 */
#define method_response_get_fault_string vortex_xml_rpc_method_response_get_fault_string

/**
 * @brief Alias definition for \ref vortex_xml_rpc_method_response_get_value
 */
#define method_response_get_value vortex_xml_rpc_method_response_get_value


/** 
 * @brief Helper function to create positive \ref XmlRpcMethodResponse
 * objects by providing a reference to the \ref XmlRpcMethodValue.
 *
 * Because an \ref XmlRpcMethodResponse could contain a positive or a
 * negative reply, this macro definition is provided to allow creating
 * positive replies that only needs to be defined the \ref
 * XmlRpcMethodValue to be replied.
 *
 * Here is an example:
 * \code
 * XmlRpcMethodValue    * value;
 * XmlRpcMethodResponse * response;
 * 
 * // create a positive reply using direct API (supposing we receive 
 * // an XmlRpcMethodValue reference.
 * value    = method_value_new (XML_RPC_INT_VALUE, INT_TO_PTR(1));
 * response = method_response_new (XML_RPC_OK, -1, NULL, value);
 * 
 * // but you can write previous code as follows
 * response = CREATE_OK_REPLY (XML_RPC_INT_VALUE, 1);
 * \endcode
 *
 * See also \ref CREATE_FAULT_REPLY to create a new \ref
 * XmlRpcMethodResponse but containing a negative reply.
 *
 * @param ctx The context where the method reply is being created.
 *
 * @param type The \ref XmlRpcMethodValue type to be used while
 * creating the XmlRpcMethodResponse reply.
 *
 * @param value The \ref XmlRpcMethodValue instance to be used for the
 * \ref XmlRpcMethodResponse to be created.
 * 
 * 
 * 
 * @return A newly created \ref XmlRpcMethodResponse
 */
#define CREATE_OK_REPLY(ctx, type, value) vortex_xml_rpc_method_response_create (ctx, type, value);


/** 
 * @brief Creates a negative \ref XmlRpcMethodResponse containing the
 * provided fault code and fault string.
 * 
 * This function works the same as \ref CREATE_OK_REPLY but creating a
 * negative reply.
 *
 * The macro requires two values, a fault code and a fault
 * string. They are used to fill the \ref XmlRpcMethodResponse
 * created.
 */
#define CREATE_FAULT_REPLY(fault_code, fault_string) vortex_xml_rpc_method_response_new (XML_RPC_FAIL, fault_code, fault_string, NULL);

/*
 * The following header definitions comes to allow operations over
 * \ref XmlRpcMethodResponse object.
 */
XmlRpcMethodResponse * vortex_xml_rpc_method_response_new              (XmlRpcResponseStatus  status,
									int                   fault_code,
									const char          * fault_string,
									XmlRpcMethodValue   * value);

XmlRpcMethodResponse * vortex_xml_rpc_method_response_create           (VortexCtx           * ctx,
									XmlRpcParamType       type,
									axlPointer            value);

void                   vortex_xml_rpc_method_response_free             (XmlRpcMethodResponse * response);

void                   vortex_xml_rpc_method_response_nullify          (XmlRpcMethodResponse * response);

char                 * vortex_xml_rpc_method_response_marshall         (XmlRpcMethodResponse * response,
									int                  * size);

XmlRpcResponseStatus   vortex_xml_rpc_method_response_get_status       (XmlRpcMethodResponse * response);

char                 * vortex_xml_rpc_method_response_stringify        (XmlRpcMethodResponse * response);

XmlRpcMethodValue    * vortex_xml_rpc_method_response_get_value        (XmlRpcMethodResponse * response);

int                    vortex_xml_rpc_method_response_get_fault_code   (XmlRpcMethodResponse * response);

char                 * vortex_xml_rpc_method_response_get_fault_string (XmlRpcMethodResponse * response);

/* 
 * The following header definitions comes to allow operations over the
 * \ref XmlRpcStruct and XmlRpcStructMember definition.
 */
XmlRpcStruct         * vortex_xml_rpc_struct_new                       (int  count);

axl_bool               vortex_xml_rpc_struct_ref                       (XmlRpcStruct * _struct);

int                    vortex_xml_rpc_struct_get_member_count          (XmlRpcStruct * _struct);

axl_bool               vortex_xml_rpc_struct_check_member_names        (XmlRpcStruct * _struct,
									int            member_count,
									...);

axl_bool               vortex_xml_rpc_struct_check_member_types        (XmlRpcStruct * _struct,
									int            member_count,
									...);

XmlRpcStructMember   * vortex_xml_rpc_struct_member_new                (const char        * name, 
									XmlRpcMethodValue * value);

void                   vortex_xml_rpc_struct_add_member                (XmlRpcStruct       * _struct,
									XmlRpcStructMember * member);

XmlRpcMethodValue    * vortex_xml_rpc_struct_get_member_value          (XmlRpcStruct       * _struct,
									const char         * member_name);

XmlRpcMethodValue    * vortex_xml_rpc_struct_get_member_value_at       (XmlRpcStruct       * _struct,
									int                  position);

char                 * vortex_xml_rpc_struct_get_member_name_at        (XmlRpcStruct       * _struct,
									int                  position);

int                    vortex_xml_rpc_struct_get_member_value_as_int   (XmlRpcStruct       * _struct,
									const char         * member_name);

char  *                vortex_xml_rpc_struct_get_member_value_as_string   (XmlRpcStruct       * _struct,
									   const char         * member_name);

double                 vortex_xml_rpc_struct_get_member_value_as_double   (XmlRpcStruct       * _struct,
									   const char         * member_name);

XmlRpcStruct         * vortex_xml_rpc_struct_get_member_value_as_struct   (XmlRpcStruct       * _struct,
									   const char         * member_name);

XmlRpcArray          * vortex_xml_rpc_struct_get_member_value_as_array   (XmlRpcStruct       * _struct,
									  const char         * member_name);

void                   vortex_xml_rpc_struct_free                      (XmlRpcStruct * _struct);

void                   vortex_xml_rpc_struct_member_free               (XmlRpcStructMember * member);

/*
 * The following header definitions comes to allow operations over the
 * \ref XmlRpcArray.
 */
XmlRpcArray          * vortex_xml_rpc_array_new                        (int  count);

axl_bool               vortex_xml_rpc_array_ref                        (XmlRpcArray * _struct);

void                   vortex_xml_rpc_array_free                       (XmlRpcArray * array);

int                    vortex_xml_rpc_array_count                      (XmlRpcArray * array);

int                    vortex_xml_rpc_array_max_count                  (XmlRpcArray * array);

XmlRpcMethodValue    * vortex_xml_rpc_array_get                        (XmlRpcArray * array, 
									int  index);

void                   vortex_xml_rpc_array_set                        (XmlRpcArray * array,
									int  index,
									XmlRpcMethodValue * value);

void                   vortex_xml_rpc_array_add                        (XmlRpcArray * array,
									XmlRpcMethodValue * value);

/** 
 * @brief Perform an error reply inside a XML-RPC server stub
 * implementation.
 * 
 * The macro fills the fault code and the fault string and then
 * perform a function return using the error value provided. This
 * error value will be ignored but it is necessary to allow the
 * function to compile knowing that it must return something.
 *
 * This macro must be used only while replying an error, as a
 * invocation error of parameter failure. Its usual function could be
 * the following example:
 *
 * \code
 * // ... server side XML-RPC service ...
 * if (/some errror found) {
 *     // just call to the macro (without doing a return)
 *     REPLY_FAULT ("Unable to complete XML-RPC service", -1, NULL);
 * }
 * \endcode
 * 
 * This macro must not be confused with \ref CREATE_FAULT_REPLY. The
 * later is used to create a fault reply represented by a \ref
 * XmlRpcMethodResponse and it is usually used while building raw
 * XML-RPC servers (without using the xml-rpc-gen tool).
 * 
 * @param string The fault string to be reported.
 * @param code The fault code to be reported.
 * @param error The error code to be returned by this function.
 */
#define REPLY_FAULT(string, code, error) do{\
(* fault_error ) = string; \
(* fault_code ) = code; \
return error; } while(0);

/** 
 * @brief Async reply notification for XML-RPC services that returns
 * as a result an integer value.
 *
 * The same handler is used for boolean values that are represented
 * with 0 (axl_false) for false values, and 1 (axl_true) for true ones.
 *
 * @param result The result received from the service invocation.
 *
 * @param status The operation status.
 *
 * @param fault_code User space code returned from the service, in the
 * case an error was produced.
 *
 * @param fault_string User space fault string returned from the
 * service, in the case an error was produced.
 * 
 */
typedef void (*XmlRpcProcessInt) (int result, XmlRpcResponseStatus status, int fault_code, char * fault_string);

/** 
 * @brief Async reply notification for XML-RPC services that returns
 * as a result a string value.
 *
 * This handler is also used to for services that returns a base64
 * string.
 *
 * @param result The result received from the service invocation.
 *
 * @param status The operation status.
 *
 * @param fault_code User space code returned from the service, in the
 * case an error was produced.
 *
 * @param fault_string User space fault string returned from the
 * service, in the case an error was produced.
 * 
 */
typedef void (*XmlRpcProcessString) (char * result, XmlRpcResponseStatus status, int fault_code, char * fault_string);

/** 
 * @brief Async reply notification for XML-RPC services that returns
 * as a result a double value.
 *
 *
 * @param result The result received from the service invocation.
 *
 * @param status The operation status.
 *
 * @param fault_code User space code returned from the service, in the
 * case an error was produced.
 *
 * @param fault_string User space fault string returned from the
 * service, in the case an error was produced.
 * 
 */
typedef void (*XmlRpcProcessDouble) (double result, XmlRpcResponseStatus status, int fault_code, char * fault_string);

/** 
 * @brief Async reply notification for XML-RPC services that returns
 * as a result a structure value.
 *
 * @param result The result received from the service invocation.
 *
 * @param status The operation status.
 *
 * @param fault_code User space code returned from the service, in the
 * case an error was produced.
 *
 * @param fault_string User space fault string returned from the
 * service, in the case an error was produced.
 * 
 */
typedef void (*XmlRpcProcessStruct) (XmlRpcStruct * _struct, XmlRpcResponseStatus status, int fault_code, char * fault_string);

/** 
 * @brief Async reply notification for XML-RPC services that returns
 * as a result an array value.
 *
 * @param result The result received from the service invocation.
 *
 * @param status The operation status.
 *
 * @param fault_code User space code returned from the service, in the
 * case an error was produced.
 *
 * @param fault_string User space fault string returned from the
 * service, in the case an error was produced.
 * 
 */
typedef void (*XmlRpcProcessArray) (XmlRpcArray * array, XmlRpcResponseStatus status, int fault_code, char * fault_string);

/** 
 * @brief Unmarshaller function that receives a XmlRpcArray reference
 * and translates it to a native representation.
 * 
 * @param array The XmlRpcArray to translate.
 * 
 * @param dealloc Allows to configure if the \ref XmlRpcArray must be
 * deallocated.
 * 
 * @return A newly allocated reference representing the native type or
 * NULL if it fails.
 */
typedef axlPointer (*XmlRpcArrayUnMarshaller) (XmlRpcArray * array, axl_bool  dealloc);

/** 
 * @brief Unmarshaller function that receives a XmlRpcStruct reference
 * and translates it to a native representation.
 * 
 * @param _struct The XmlRpcStruct to translate.
 * 
 * @param dealloc Allows to configure if the \ref XmlRpcStruct must be
 * deallcoted.
 * 
 * @return A newly allocated reference representing the native type or
 * NULL if it fails.
 */
typedef axlPointer (*XmlRpcStructUnMarshaller) (XmlRpcStruct * _struct, axl_bool  dealloc);



#endif

/* @} */

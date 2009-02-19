/**
 * C inline representation for DTD xml-rpc.dtd, created by axl-knife
 */
#ifndef __XML_RPC_DTD_H__
#define __XML_RPC_DTD_H__
#define XML_RPC_DTD "\n\
<!--                                                                                   \
    Document root declaration,                                                         \
    complex types, resources and services.                                             \
 -->                                                                                   \
<!ELEMENT xml-rpc-interface (name, (struct | array | resource | service)+)>            \
                                                                                       \
<!--                                                                                   \
    XML-RPC struct declaration, including                                              \
    its members.                                                                       \
  -->                                                                                  \
<!ELEMENT struct (name, member+)>                                                      \
                                                                                       \
<!ELEMENT member (name, type)>                                                         \
                                                                                       \
<!ELEMENT name (#PCDATA)>                                                              \
                                                                                       \
<!ELEMENT value (#PCDATA)>                                                             \
                                                                                       \
<!--                                                                                   \
    XML-RPC array declaration.                                                         \
  -->                                                                                  \
<!ELEMENT array (name, type, size)>                                                    \
                                                                                       \
<!ELEMENT type (#PCDATA)>                                                              \
                                                                                       \
<!ELEMENT size (#PCDATA)>                                                              \
                                                                                       \
<!--                                                                                   \
    XML-RPC service declaration.                                                       \
  -->                                                                                  \
<!ELEMENT service (name, returns, resource?, params, code?, method_name?, options?)>   \
                                                                                       \
<!ELEMENT returns (#PCDATA)>                                                           \
                                                                                       \
<!ELEMENT resource (#PCDATA)>                                                          \
                                                                                       \
<!ELEMENT params (param)*>                                                             \
                                                                                       \
<!ELEMENT param (name, type)>                                                          \
                                                                                       \
<!ELEMENT code (content, language?)>                                                   \
                                                                                       \
<!ELEMENT content (#PCDATA)>                                                           \
                                                                                       \
<!ELEMENT language (#PCDATA)>                                                          \
                                                                                       \
<!ELEMENT method_name (#PCDATA)>                                                       \
                                                                                       \
<!ELEMENT options (header?, body?)>                                                    \
                                                                                       \
<!ELEMENT header (#PCDATA)>                                                            \
                                                                                       \
<!ELEMENT body (#PCDATA)>                                                              \
                                                                                       \
                                                                                       \
                                                                                       \
                                                                                       \
\n"
#endif

/**
 * C inline representation for DTD sasl.dtd, created by axl-knife
 */
#ifndef __SASL_DTD_H__
#define __SASL_DTD_H__
#define SASL_DTD "\n\
<!--                                                                  \
     DTD for the SASL Family of Profiles, as of 2000-09-04            \
                                                                      \
                                                                      \
     Refer to this DTD as:                                            \
                                                                      \
       <!ENTITY % SASL PUBLIC '-//IETF//DTD SASL//EN'                 \
                  'http://xml.resource.org/profiles/sasl/sasl.dtd'>   \
       %SASL;                                                         \
                                                                      \
     SASL messages, exchanged as application/beep+xml                 \
                                                                      \
        role       MSG         RPY         ERR                        \
       ======      ===         ===         ===                        \
       I or L      blob        blob        error                      \
     -->                                                              \
                                                                      \
                                                                      \
<!ELEMENT blob        (#PCDATA)>                                      \
<!ATTLIST blob                                                        \
          xml:space   (default|preserve)        'preserve'            \
          status      (abort|complete|continue) 'continue'>           \
                                                                      \
\n"
#endif

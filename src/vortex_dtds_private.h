#ifndef __VORTEX_DTDS_PRIVATE_H__
#define __VORTEX_DTDS_PRIVATE_H__

/* Inline definition from channel.dtd. */
#define CHANNEL_DTD "\
<!-- channel management DTD definition from RFC 3080-->  \
<!ENTITY % CHAN       \"CDATA\">                         \
<!ENTITY % URI        \"CDATA\">                         \
<!ENTITY % FTRS       \"NMTOKENS\">                      \
<!ENTITY % LANG       \"NMTOKEN\">                       \
<!ENTITY % LOCS       \"NMTOKEN\">                       \
<!ENTITY % XYZ        \"CDATA\">                         \
                                                         \
<!ELEMENT greeting    (profile)*>                        \
<!ATTLIST greeting                                       \
          features    %FTRS;            #IMPLIED         \
          localize    %LOCS;            \"i-default\">   \
                                                         \
<!ELEMENT start       (profile)+>                        \
<!ATTLIST start                                          \
          number      %CHAN;             #REQUIRED       \
          serverName  CDATA              #IMPLIED>       \
                                                         \
<!ELEMENT profile     (#PCDATA)>                         \
<!ATTLIST profile                                        \
          uri         %URI;              #REQUIRED       \
          encoding    (none|base64)      \"none\">       \
                                                         \
<!ELEMENT close       (#PCDATA)>                         \
<!ATTLIST close                                          \
          number      %CHAN;             \"0\"           \
          code        %XYZ;              #REQUIRED       \
          xml:lang    %LANG;             #IMPLIED>       \
                                                         \
<!ELEMENT ok          EMPTY>                             \
                                                         \
<!ELEMENT error       (#PCDATA)>                         \
<!ATTLIST error                                          \
          code        %XYZ;              #REQUIRED       \
          xml:lang    %LANG;             #IMPLIED>       "

#endif /* __VORTEX_DTDS_PRIVATE_H__  */


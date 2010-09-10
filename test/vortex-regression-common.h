/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
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

#ifndef __VORTEX_REGRESSION_COMMOH_H__
#define __VORTEX_REGRESSION_COMMON_H__

/** 
 * Profile use to identify the regression test server.
 */
#define REGRESSION_URI "http://iana.org/beep/transient/vortex-regression"

/** 
 * A profile to check default close channel action.
 */
#define REGRESSION_URI_2 "http://iana.org/beep/transient/vortex-regression/2"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_3 "http://iana.org/beep/transient/vortex-regression/3"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_4 "http://iana.org/beep/transient/vortex-regression/4"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_5 "http://iana.org/beep/transient/vortex-regression/5"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_6 "http://iana.org/beep/transient/vortex-regression/6"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_6bis "http://iana.org/beep/transient/vortex-regression/6bis"

/** 
 * A profile to check close in transit support.
 */ 
#define CLOSE_IN_TRANSIT_URI "http://iana.org/beep/transient/close-in-transit"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_ZERO "http://iana.org/beep/transient/vortex-regression/zero"

/** 
 * A profile to check connection timeout against unresponsive
 * listeners.
 */
#define REGRESSION_URI_LISTENERS "http://iana.org/beep/transient/vortex-regression/fake-listener"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_ZERO "http://iana.org/beep/transient/vortex-regression/zero"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_BLOCK_TLS "http://iana.org/beep/transient/vortex-regression/block-tls"

/** 
 * A regression test profile that allows to check if the listener can
 * send data just after accepting the channel to be created.
 */
#define REGRESSION_URI_FAST_SEND "http://iana.org/beep/transient/vortex-regression/fast-send"

/** 
 * A regression test profile to check channel deny operations. This
 * profile must not be supported by the listener side.
 */
#define REGRESSION_URI_DENY "http://iana.org/beep/transient/vortex-regression/deny"

/** 
 * A regression test profile to check channel deny operations. This
 * profile is supported by the remote listener.
 */
#define REGRESSION_URI_DENY_SUPPORTED "http://iana.org/beep/transient/vortex-regression/deny_supported"

/** 
 * A regression test profile to check channel deny operations. This
 * profile is supported by the remote listener.
 */
#define REGRESSION_URI_CLOSE_AFTER_LARGE_REPLY "http://iana.org/beep/transient/vortex-regression/close-after-large-reply"

/** 
 * Profile use to identify the regression test client and server mime
 * support.
 */
#define REGRESSION_URI_MIME "http://iana.org/beep/transient/vortex-regression/mime"

/** 
 * Profile use to identify the regression test client and server mime
 * support.
 */
#define REGRESSION_URI_ORDERED_DELIVERY "http://iana.org/beep/transient/vortex-regression/ordered-delivery"

/** 
 * Profile use to identify the regression test client and server mime
 * support.
 */
#define REGRESSION_URI_SUDDENTLY_CLOSE "http://iana.org/beep/transient/vortex-regression/suddently-close"

/** 
 * Profile use to identify the regression test to check replies mixed
 * (ANS..NUL with RPY).
 */
#define REGRESSION_URI_MIXING_REPLIES "http://iana.org/beep/transient/vortex-regression/mixing-replies"

/** 
 * Simple profile that replies with 10 ANS messages ended by NUL frame
 * echoing the content received.
 */
#define REGRESSION_URI_SIMPLE_ANS_NUL "http://iana.org/beep/transient/vortex-regression/simple-ans-nul"

/** 
 * Profile to check ready state for channels having pending replies
 * with ans/nul.
 */
#define REGRESSION_URI_ANS_NUL_WAIT   "http://iana.org/beep/transient/vortex-regression/ans-nul-wait"

/** 
 * Profile used to identify the regression test to check connection
 * close after ans/nul reply.
 */
#define REGRESSION_URI_ANS_NUL_REPLY_CLOSE "http://iana.org/beep/transient/vortex-regression/ans-nul-reply-close"

/** 
 * Profile used to identify the regression test to check connection
 * close after ans/nul reply.
 */
#define REGRESSION_URI_CLOSE_AFTER_ANS_NUL_REPLIES "http://iana.org/beep/transient/vortex-regression/close-after-ans-nul-replies"

/** 
 * Profile that does nothing.
 */
#define REGRESSION_URI_NOTHING "http://iana.org/beep/transient/vortex-regression/nothing"

/** 
 * Profile that allows to check seqno limits.
 */
#define REGRESSION_URI_SEQNO_EXCEEDED "http://iana.org/beep/transient/vortex-regression/seqno-exceeded"

/** 
 * Profile to check new unified SASL API
 */
#define REGRESSION_URI_SASL_UNIFIED "http://iana.org/beep/transient/vortex-regression/unified-sasl"

#include <vortex.h>

char * vortex_regression_common_read_file (const char * file, int * size);

/* message size: 4096 */
#define TEST_REGRESION_URI_4_MESSAGE "This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary ."


#endif /* __VORTEX_REGRESSION_COMMON_H__ */

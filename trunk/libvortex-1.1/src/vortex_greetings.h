/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */
#ifndef __VORTEX_GREETINGS_H__
#define __VORTEX_GREETINGS_H__

#include <vortex.h>

bool           vortex_greetings_send           (VortexConnection * connection);

bool           vortex_greetings_is_reply_ok    (VortexFrame      * frame,
						VortexConnection * connection);

VortexFrame *  vortex_greetings_process        (VortexConnection * connection);

bool           vortex_greetings_client_send    (VortexConnection * connection);

VortexFrame *  vortex_greetings_client_process (VortexConnection * connection);

void           vortex_greetings_set_features   (const char * feature);

const char  *  vortex_greetings_get_features   ();

void           vortex_greetings_set_localize   (const char * localize);

const char  *  vortex_greetings_get_localize   ();

void           vortex_greetings_cleanup        (VortexCtx * ctx);

#endif

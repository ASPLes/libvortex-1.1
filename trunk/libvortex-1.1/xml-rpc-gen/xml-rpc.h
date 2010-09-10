/* Hey emacs, show me this like 'c': -*- c -*-
 *
 * xml-rpc-gen: a protocol compiler for the XDL definition language
 * Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifndef __XML_RPC_H__
#define __XML_RPC_H__

/* system include for: chmod */
#include <sys/types.h>
#include <sys/stat.h>

/* axl includes */
#include <axl.h>

/* exarg includes */
#include <exarg.h>

/* include vortex support header */
#include <vortex.h>

/* system includes */
#include <stdio.h>

#if defined(AXL_OS_WIN32)
/* use errno definition from windows header */
#undef errno
#include <errno.h>
/* no link defined on windows, so always false */
#define S_ISLNK(m) (0)
#endif

/* local xml-rpc includes */
#include <xml-rpc-support.h>
#include <xml-rpc-c-stub.h>
#include <xml-rpc-c-server.h>
#include <xml-rpc-autoconf.h>
#include <xml-rpc-gen-translate.h>

#endif

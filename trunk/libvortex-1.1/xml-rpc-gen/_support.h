/* Hey emacs, show me this like 'c': -*- c -*-
 *
 * Support functions for test performed.
 * Copyright (C) 2006 Advanced Software Production Line, S.L.
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
#ifndef ___SUPPORT_H__
#define ___SUPPORT_H__

/* include vortex headers */
#include <vortex.h>

VortexChannel * start_and_create_channel ();

void            close_channel_and_stop (VortexChannel * channel);

#endif

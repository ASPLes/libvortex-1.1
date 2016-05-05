/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __VORTEX_SEQUENCER_H__
#define __VORTEX_SEQUENCER_H__

#include <vortex.h>

axl_bool vortex_sequencer_queue_data               (VortexCtx           * ctx,
						    VortexSequencerData * data);

int      vortex_sequencer_channels_pending_ops     (VortexCtx           * ctx);

axl_bool vortex_sequencer_run                      (VortexCtx * ctx);

void     vortex_sequencer_stop                     (VortexCtx * ctx);

axl_bool vortex_sequencer_direct_send              (VortexConnection * connection,
						    VortexChannel    * channel,
						    VortexWriterData * packet);

void     vortex_sequencer_signal_update            (VortexChannel       * channel, 
						    VortexConnection    * connection);

void     vortex_sequencer_remove_channel           (VortexCtx        * ctx,
						    VortexChannel    * channel);

#endif



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

#ifndef __VORTEX_PAYLOAD_FEEDER_PRIVATE_H__
#define __VORTEX_PAYLOAD_FEEDER_PRIVATE_H__
struct _VortexPayloadFeeder {
	/** 
	 * @internal The context used by the payload feeder.
	 */
	VortexCtx                  * ctx;
	/** 
	 * @internal The handler that defines how this feeder behaves.
	 */
	VortexPayloadFeederHandler   handler;
	/** 
	 * @internal User defiend pointer that is passed to the previous handler.
	 */
	axlPointer                   user_data;
	/** 
	 * @internal Mutex and reference counting used to track owned references.
	 */
	VortexMutex                  mutex;
	int                          ref_count;

	/** 
	 * @internal Status variable used to signal if the feeder can
	 * continue (0), or if it should be paused (-1) or if it should
	 * cancelled (-2).
	 */
	int                          status;

	/** 
	 * @internal Flag to track how close current ongoing transfer
	 * when they are cancelled or paused.
	 */
	axl_bool                     close_transfer;

	/** 
	 * @internal In the case of a pause tranfer without closing
	 * the transfer, this field records the msg_no that should be
	 * used to continue.
	 */
	int                          msg_no;

	/** 
	 * @internal Bytes transferred by this feeder so far.
	 */
	long                         bytes_transferred;

	/** 
	 * @internal Handler used to notify that the transfer has
	 * finished.
	 */
	VortexPayloadFeederFinishedHandler finish_handler;
	axlPointer                         finish_user_data;
	axl_bool                           notification_done;

	/** 
	 * @brief Pointer to the channel that is used to transfer content..
	 */
	VortexChannel              * channel;
};
#endif

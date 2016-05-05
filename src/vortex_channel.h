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

#ifndef __VORTEX_CHANNEL_H__
#define __VORTEX_CHANNEL_H__

#include <vortex.h>

/** 
 * \addtogroup vortex_channel
 * @{
 */

/**
 * @brief Allows to get context associated to the channel provided.
 * @param channel The channel to get the context from.
 * @return A reference to the context.
 */
#define CHANNEL_CTX(channel) (vortex_channel_get_ctx (channel))

VortexChannel     * vortex_channel_new                         (VortexConnection      * connection, 
								int                     channel_num, 
								const char            * profile,
								VortexOnCloseChannel    close,
								axlPointer              close_user_data,
								VortexOnFrameReceived   received,
								axlPointer              received_user_data,
								VortexOnChannelCreated  on_channel_created,
								axlPointer user_data);

VortexChannel     * vortex_channel_new_full                    (VortexConnection      * connection,
								int                     channel_num, 
								const char            * serverName,
								const char            * profile,
								VortexEncoding          encoding,
								const char            * profile_content,
								int                     profile_content_size,
								VortexOnCloseChannel    close,
								axlPointer              close_user_data,
								VortexOnFrameReceived   received,
								axlPointer              received_user_data,
								VortexOnChannelCreated  on_channel_created, axlPointer user_data);

VortexChannel     * vortex_channel_new_fullv                   (VortexConnection      * connection,
								int                     channel_num, 
								const char            * serverName,
								const char            * profile,
								VortexEncoding          encoding,
								VortexOnCloseChannel    close,
								axlPointer              close_user_data,
								VortexOnFrameReceived   received,
								axlPointer              received_user_data,
								VortexOnChannelCreated  on_channel_created, axlPointer user_data,
								const char            * profile_content_format, 
								...);


axl_bool           vortex_channel_close_full                   (VortexChannel * channel, 
								VortexOnClosedNotificationFull on_closed, 
								axlPointer user_data);

axl_bool           vortex_channel_close                        (VortexChannel * channel,
								VortexOnClosedNotification on_closed);

VortexChannel    * vortex_channel_empty_new                    (int                channel_num,
								const char       * profile,
								VortexConnection * connection);

void               vortex_channel_set_close_handler            (VortexChannel * channel,
								VortexOnCloseChannel close,
								axlPointer user_data);

void               vortex_channel_set_closed_handler           (VortexChannel          * channel,
								VortexOnClosedChannel    closed,
								axlPointer               user_data);

void               vortex_channel_invoke_closed                (VortexChannel          * channel);

void               vortex_channel_set_close_notify_handler     (VortexChannel              * channel,
								VortexOnNotifyCloseChannel   close_notify,
								axlPointer                   user_data);

void               vortex_channel_set_received_handler         (VortexChannel * channel, 
								VortexOnFrameReceived received,
								axlPointer user_data);

void               vortex_channel_set_complete_flag            (VortexChannel * channel,
								axl_bool        value);

void               vortex_channel_set_complete_frame_limit     (VortexChannel * channel,
								int             max_payload_size);

axl_bool           vortex_channel_have_previous_frame          (VortexChannel * channel);

VortexFrame      * vortex_channel_get_previous_frame           (VortexChannel * channel);

void               vortex_channel_store_previous_frame         (VortexCtx     * ctx,
								VortexChannel * channel, 
								VortexFrame   * new_frame);

VortexFrame      * vortex_channel_build_single_pending_frame   (VortexChannel * channel);

axl_bool           vortex_channel_have_complete_flag           (VortexChannel * channel);

void               vortex_channel_update_status                (VortexChannel * channel, 
								unsigned int    frame_size,
								int             msg_no,
								WhatUpdate update);

void               vortex_channel_update_status_received       (VortexChannel * channel, 
								unsigned int    frame_size,
								int             msg_no,
								WhatUpdate      update);

int                vortex_channel_get_next_msg_no              (VortexChannel * channel);

void               vortex_channel_set_next_seq_no              (VortexChannel * channel, 
								unsigned int    next_seq_no);

int                vortex_channel_get_last_msg_no_received     (VortexChannel * channel);

int                vortex_channel_get_number                   (VortexChannel * channel);

unsigned int       vortex_channel_get_next_seq_no              (VortexChannel * channel);

unsigned int       vortex_channel_get_next_expected_seq_no     (VortexChannel * channel);

unsigned int       vortex_channel_get_next_ans_no              (VortexChannel * channel);

unsigned int       vortex_channel_get_next_expected_ans_no     (VortexChannel * channel);

int                vortex_channel_get_next_reply_no            (VortexChannel * channel);

int                vortex_channel_get_next_expected_reply_no   (VortexChannel * channel);

int                vortex_channel_get_window_size              (VortexChannel * channel);

void               vortex_channel_set_window_size              (VortexChannel * channel,
                                                                int             desired_size);

const char       * vortex_channel_get_mime_type                (VortexChannel * channel);

const char       * vortex_channel_get_transfer_encoding        (VortexChannel * channel);

void               vortex_channel_set_automatic_mime           (VortexChannel  * channel,
								int              value);

int                vortex_channel_get_automatic_mime           (VortexChannel * channel);

unsigned int       vortex_channel_get_max_seq_no_remote_accepted (VortexChannel * channel);

int                vortex_channel_get_next_frame_size         (VortexChannel * channel,
							       unsigned int    next_seq_no,
							       int             message_size,
							       unsigned int    max_seq_no);

VortexChannelFrameSize  vortex_channel_set_next_frame_size_handler (VortexChannel          * channel,
								    VortexChannelFrameSize   next_frame_size,
								    axlPointer               user_data);

void               vortex_channel_update_remote_incoming_buffer (VortexChannel * channel, 
								 unsigned        ackno,
								 unsigned        window);

unsigned int       vortex_channel_get_max_seq_no_accepted       (VortexChannel * channel);

void               vortex_channel_set_max_seq_no_accepted       (VortexChannel * channel, 
								 unsigned int    seq_no,
								 int             window_size);

axl_bool           vortex_channel_are_equal                     (VortexChannel * channelA,
								 VortexChannel * channelB);

axl_bool           vortex_channel_update_incoming_buffer       (VortexChannel * channel, 
								VortexFrame   * frame,
								unsigned int  * ackno,
								int           * window);

void               vortex_channel_queue_pending_message        (VortexChannel * channel,
								axlPointer      message);

axl_bool           vortex_channel_is_empty_pending_message     (VortexChannel * channel);

axlPointer         vortex_channel_next_pending_message         (VortexChannel * channel);

int                vortex_channel_pending_messages             (VortexChannel * channel);

axlPointer         vortex_channel_remove_pending_message       (VortexChannel * channel);

const char       * vortex_channel_get_profile                  (VortexChannel * channel);

axl_bool           vortex_channel_is_running_profile           (VortexChannel * channel,
								const char    * profile);

VortexConnection * vortex_channel_get_connection               (VortexChannel * channel);

axl_bool           vortex_channel_queue_frame                     (VortexChannel * channel, 
								   VortexWriterData * data);

VortexWriterData * vortex_channel_queue_next_msg                  (VortexChannel * channel);

void               vortex_channel_set_serialize                   (VortexChannel * channel,
								   axl_bool        serialize);

void               vortex_channel_set_data                        (VortexChannel * channel,
								   axlPointer key,
								   axlPointer value);

void               vortex_channel_set_data_full                   (VortexChannel    * channel, 
								   axlPointer         key, 
								   axlPointer         value, 
								   axlDestroyFunc     key_destroy,
								   axlDestroyFunc     value_destroy);

void               vortex_channel_delete_data                     (VortexChannel    * channel,
								   axlPointer         key);

axlPointer         vortex_channel_get_data                        (VortexChannel * channel,
								   axlPointer key);

axl_bool           vortex_channel_ref2                            (VortexChannel * channel, const char * label);

void               vortex_channel_unref2                          (VortexChannel * channel, const char * label);

axl_bool           vortex_channel_ref                             (VortexChannel * channel);

void               vortex_channel_unref                           (VortexChannel * channel);

int                vortex_channel_ref_count                       (VortexChannel * channel);


axl_bool           vortex_channel_send_msg                        (VortexChannel    * channel,
								   const void       * message,
								   size_t             message_size,
								   int              * msg_no);

axl_bool           vortex_channel_send_msgv                       (VortexChannel * channel,
								   int           * msg_no,
								   const char    * format,
								   ...);

axl_bool           vortex_channel_send_msg_and_wait               (VortexChannel     * channel,
								   const void        * message,
								   size_t              message_size,
								   int               * msg_no,
								   WaitReplyData     * wait_reply);

axl_bool           vortex_channel_send_msg_and_waitv              (VortexChannel   * channel,
								   int             * msg_no,
								   WaitReplyData   * wait_reply,
								   const char      * format,
								   ...);

axl_bool           vortex_channel_send_msg_common                 (VortexChannel       * channel,
								   const void          * message,
								   size_t                message_size,
								   int                   proposed_msg_no, 
								   int                 * msg_no,
								   WaitReplyData       * wait_reply,
								   VortexPayloadFeeder * feeder,
								   axl_bool              fixed_more);

axl_bool           vortex_channel_send_msg_from_feeder            (VortexChannel       * channel,
								   VortexPayloadFeeder * feeder);

axl_bool           vortex_channel_send_msg_more                   (VortexChannel    * channel,
								   const void       * message,
								   size_t             message_size,
								   int              * msg_no);

axl_bool           vortex_channel_send_rpy                        (VortexChannel    * channel,  
								   const void       * message,
								   size_t             message_size,
								   int                msg_no_rpy);

axl_bool           vortex_channel_send_rpyv                       (VortexChannel * channel,
								   int             msg_no_rpy,
								   const   char  * format,
								   ...);

axl_bool           vortex_channel_send_rpy_from_feeder            (VortexChannel       * channel,
								   VortexPayloadFeeder * feeder,
								   int                   msg_no_rpy);

axl_bool           vortex_channel_send_rpy_more                   (VortexChannel    * channel,  
								   const void       * message,
								   size_t             message_size,
								   int                msg_no_rpy);

axl_bool           vortex_channel_send_ans_rpy                    (VortexChannel    * channel,
								   const void       * message,
								   size_t             message_size,
								   int                msg_no_rpy);

axl_bool           vortex_channel_send_ans_rpy_from_feeder        (VortexChannel       * channel,
								   VortexPayloadFeeder * feeder,
								   int                   msg_no_rpy);

axl_bool           vortex_channel_send_ans_rpyv                   (VortexChannel * channel,
								   int             msg_no_rpy,
								   const char    * format,
								   ...);

axl_bool           vortex_channel_finalize_ans_rpy                (VortexChannel * channel,
								   int             msg_no_rpy);

axl_bool           vortex_channel_send_err                       (VortexChannel    * channel,  
								  const void       * message,
								  size_t             message_size,
								  int                msg_no_err);

axl_bool           vortex_channel_send_errv                      (VortexChannel * channel,
								  int             msg_no_err,
								  const   char  * format, 
								  ...); 

axl_bool           vortex_channel_send_err_more                  (VortexChannel    * channel,  
								  const void       * message,
								  size_t             message_size,
								  int                msg_no_err);
						
axl_bool           vortex_channel_is_opened                      (VortexChannel * channel);

axl_bool           vortex_channel_is_being_closed                (VortexChannel * channel);


void               vortex_channel_free                           (VortexChannel * channel);

axl_bool           vortex_channel_is_defined_received_handler    (VortexChannel * channel);      

axl_bool           vortex_channel_invoke_received_handler        (VortexConnection * connection,
								  VortexChannel    * channel, 
								  VortexFrame      * frame);

axl_bool           vortex_channel_is_defined_close_handler       (VortexChannel * channel);

axl_bool           vortex_channel_invoke_close_handler           (VortexChannel  * channel);

VortexFrame      * vortex_channel_wait_reply                     (VortexChannel * channel, 
								  int  msg_no,
								  WaitReplyData * wait_reply);

WaitReplyData *    vortex_channel_create_wait_reply              (void);

axl_bool           vortex_channel_wait_reply_ref                 (WaitReplyData * wait_reply);

void               vortex_channel_free_wait_reply                (WaitReplyData * wait_reply);

axl_bool           vortex_channel_is_ready                       (VortexChannel * channel);

void               vortex_channel_queue_reply                    (VortexChannel    * channel,
								  VortexConnection * connection,
								  VortexFrame      * frame,
								  axlPointer         user_data);

VortexFrame      * vortex_channel_get_reply                      (VortexChannel    * channel,
								  VortexAsyncQueue * queue);

VortexFrame      * vortex_channel_get_piggyback                  (VortexChannel    * channel);

axl_bool           vortex_channel_have_piggyback                 (VortexChannel    * channel);

void               vortex_channel_set_piggyback                  (VortexChannel    * channel,
								  const char       * profile_content);

void               vortex_channel_set_outstanding_limit          (VortexChannel    * channel, 
								  int                pending_messages,
								  axl_bool           fail_on_limit);

int                vortex_channel_get_outstanding_messages       (VortexChannel    * channel,
								  int              * outstanding_limit);

void               vortex_channel_defer_start                    (VortexChannel    * channel);

axl_bool           vortex_channel_notify_start                   (VortexChannel    * new_channel,
								  const char       * profile_content_reply,
								  axl_bool           action);

void               vortex_channel_notify_close                   (VortexChannel    * channel,
								  int                msg_no,
								  axl_bool           action);

/* message validation */
axl_bool           vortex_channel_validate_err                   (VortexFrame * frame, 			  
								  char  ** code, char  **msg);

/* internal vortex function */
void               vortex_channel_lock_to_update_received        (VortexChannel * channel);

void               vortex_channel_unlock_to_update_received      (VortexChannel * channel);

void               vortex_channel_lock_to_receive                (VortexChannel * channel);

void               vortex_channel_unlock_to_receive              (VortexChannel * channel);

void               vortex_channel_0_frame_received               (VortexChannel    * channel,
								  VortexConnection * connection,
								  VortexFrame      * frame,
								  axlPointer         user_data);

void           	   vortex_channel_signal_on_close_blocked        (VortexChannel    * channel);

void               vortex_channel_flag_reply_processed           (VortexChannel * channel, 
								  axl_bool        flag);

axl_bool           vortex_channel_install_waiter                 (VortexChannel * channel,
								  int             rpy);

void               vortex_channel_wait_until_sent                (VortexChannel * channel,
								  int             rpy);

void               vortex_channel_signal_rpy_sent               (VortexChannel * channel,
		 						 int             rpy);

void               vortex_channel_signal_reply_sent_on_close_blocked (VortexChannel * channel);

void                vortex_channel_set_pool                       (VortexChannel * channel,
								   VortexChannelPool * pool);

VortexChannelPool * vortex_channel_get_pool                       (VortexChannel * channel);

VortexCtx         * vortex_channel_get_ctx                        (VortexChannel * channel);

void                vortex_channel_init                           (VortexCtx * ctx);

void                vortex_channel_cleanup                        (VortexCtx * ctx);

axl_bool            vortex_channel_block_until_replies_are_sent   (VortexChannel * channel, 
								   long            microseconds_to_wait);

axl_bool            vortex_channel_check_serialize                (VortexCtx        * ctx,
								   VortexConnection * connection, 
								   VortexChannel    * channel, 
								   VortexFrame      * frame);

axl_bool            vortex_channel_check_serialize_pending         (VortexCtx          * ctx,
								    VortexConnection   * conn,
								    VortexChannel      * channel, 
								    VortexFrame       ** caller_frame);

axl_bool            vortex_channel_check_msg_no                    (VortexChannel  * channel, 
 								    VortexFrame    * frame);

axl_bool            vortex_channel_check_incoming_seqno            (VortexChannel  * channel,
								    VortexFrame    * frame);

axl_bool            vortex_channel_is_stalled                      (VortexChannel  * channel);

axl_bool            vortex_channel_remove_first_pending_msg_no     (VortexChannel * channel, 
 								    int             msg_no_rpy);

axl_bool            vortex_channel_remove_first_outstanding_msg_no (VortexChannel * channel, 
 								    int             msg_no_rpy);

void               __vortex_channel_nullify_conn                   (VortexChannel  * channel);

axl_bool           vortex_channel_0_handle_start_msg_reply         (VortexCtx        * ctx, 
								    VortexConnection * connection,
								    int                channel_num,
								    const char       * profile,
								    const char       * profile_content,
								    VortexEncoding     encoding,
								    const char       * serverName,
								    VortexFrame      * frame);

void              __vortex_channel_set_state                       (VortexChannel    * channel,
					                            int                next_reply_no,
								    int                last_seq_no,
								    int                last_seq_no_expected,
								    int                last_reply_received);

#endif

/* @} */

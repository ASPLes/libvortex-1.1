/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
#ifndef __VORTEX_CTX_PRIVATE_H__
#define __VORTEX_CTX_PRIVATE_H__

/* global included */
#include <axl.h>
#include <vortex.h>

typedef struct _VortexSequencerState {
	axlHash       * ready;
	axlHashCursor * ready_cursor;

	VortexMutex     mutex;
	VortexCond      cond;

	axl_bool        exit;
} VortexSequencerState;

struct _VortexCtx {

	VortexMutex          ref_mutex;
	int                  ref_count;

	/* global hash to store arbitrary data */
	VortexHash         * data;

	/* @internal Allows to check if the vortex library is in exit
	 * transit.
	 */
	axl_bool             vortex_exit;
	/* @internal Allows to check if the provided vortex context is initialized
	 */
	axl_bool             vortex_initialized;

	/* global mutex */
	VortexMutex          frame_id_mutex;
	VortexMutex          connection_id_mutex;
	VortexMutex          search_path_mutex;
	VortexMutex          exit_mutex;
	VortexMutex          listener_mutex;
	VortexMutex          listener_unlock;
	VortexMutex          inet_ntoa_mutex;
	VortexMutex          log_mutex;
	axl_bool             use_log_mutex;
	axl_bool             prepare_log_string;

	/* serverName acquire */
	axl_bool             serverName_acquire;

	/* external cleanup functions */
	axlList            * cleanups;

	/* default configurations */
	int                  backlog;
	axl_bool             enforce_profiles_supported;
	int                  automatic_mime;

	/* allows to control if we should wait to finish threads
	 * inside the pool */
	axl_bool             skip_thread_pool_wait;
	
	/* local log variables */
	axl_bool             debug_checked;
	axl_bool             debug;
	
	axl_bool             debug2_checked;
	axl_bool             debug2;
	
	axl_bool             debug_color_checked;
	axl_bool             debug_color;

	VortexLogHandler     debug_handler;
	VortexLogHandlerFull debug_handler2;
	axlPointer           debug_handler2_user_data;

	int                  debug_filter;
	axl_bool             debug_filter_checked;
	axl_bool             debug_filter_is_enabled;

	/*** global handlers */
	/* @internal Finish handler */
	VortexOnFinishHandler             finish_handler;
	axlPointer                        finish_handler_data;

	/* @internal Handler used to implement global frame received.
	 */
	VortexOnFrameReceived             global_frame_received;
	axlPointer                        global_frame_received_data;

	/* @internal Handler used to implement global close channel
	 * request received. */
	VortexOnNotifyCloseChannel        global_notify_close;
	axlPointer                        global_notify_close_data;

	/* @internal Handler used to implement global channel added
	 * event */
	VortexConnectionOnChannelUpdate   global_channel_added;
	axlPointer                        global_channel_added_data;

	/* @internal Handler used to implement global channel removed
	 * event */
	VortexConnectionOnChannelUpdate   global_channel_removed;
	axlPointer                        global_channel_removed_data;

	/* @internal Handler used to implement global channel start
	 * event */
	VortexOnStartChannelExtended      global_channel_start_extended;
	axlPointer                        global_channel_start_extended_data;

	/* @internal Handler used to implement global idle
	   notification */
	VortexIdleHandler                 global_idle_handler;
	long                              max_idle_period;
	axlPointer                        global_idle_handler_data;
	axlPointer                        global_idle_handler_data2;

#if defined(AXL_OS_WIN32)
	/**
	 * Temporal hack to support sock limits under windows until we
	 * find a viable alternative to getrlimit and setrlimit.
	 */
	int                  __vortex_conf_hard_sock_limit;
	int                  __vortex_conf_soft_sock_limit;
#endif

	/**** vortex connection module state *****/
	/** 
	 * @internal
	 * @brief A connection identifier (for internal vortex use).
	 */
	long                 connection_id;
	axl_bool             connection_enable_sanity_check;

	/**
	 * @internal xml caching functions:
	 */ 
	axlHash            * connection_xml_cache;
	axlHash            * connection_hostname;
	VortexMutex          connection_xml_cache_mutex;
	VortexMutex          connection_hostname_mutex;
	
	/**
	 * @internal Vortex connection creation status reporting
	 */
	VortexMutex          connection_actions_mutex;
	axlList           *  connection_actions;

	/** 
	 * @internal Default timeout used by vortex connection operations.
	 */
	long                 connection_std_timeout;
	axl_bool             connection_timeout_checked;
	char              *  connection_timeout_str;
	long                 connection_connect_std_timeout;
	axl_bool             connection_connect_timeout_checked;
	char              *  connection_connect_timeout_str;

	/**** vortex channel module state ****/
	VortexMutex          channel_start_reply_cache_mutex;
	axlHash           *  channel_start_reply_cache;

	/**** vortex frame module state ****/
	/** 
	 * @internal
	 *
	 * Internal variables (frame_id and frame_id_mutex) to make frame
	 * identification for the on going process. This allows to check if
	 * two frames are equal or to track which frames are not properly
	 * released by the Vortex Library.
	 *
	 * Frames are generated calling to __vortex_frame_get_next_id. Thus,
	 * every frame created while the running process is alive have a
	 * different frame unique identifier. 
	 */
	long                frame_id;

	/**** vortex profiles module state ****/
	VortexHash        * registered_profiles;
	axlList           * profiles_list;
	VortexMutex         profiles_list_mutex;

	/**** vortex io waiting module state ****/
	VortexIoCreateFdGroup  waiting_create;
	VortexIoDestroyFdGroup waiting_destroy;
	VortexIoClearFdGroup   waiting_clear;
	VortexIoWaitOnFdGroup  waiting_wait_on;
	VortexIoAddToFdGroup   waiting_add_to;
	VortexIoIsSetFdGroup   waiting_is_set;
	VortexIoHaveDispatch   waiting_have_dispatch;
	VortexIoDispatch       waiting_dispatch;
	VortexIoWaitingType    waiting_type;

	/**** vortex dtd module state ****/
	axlDtd               * channel_dtd;
	axlDtd               * xml_rpc_boot_dtd;

	/**** vortex reader module state ****/
	VortexAsyncQueue        * reader_queue;
	VortexAsyncQueue        * reader_stopped;
	axlPointer                on_reading;
	axlList                 * conn_list;
	axlList                 * srv_list;
	axlListCursor           * conn_cursor;
	axlListCursor           * srv_cursor;
	/* the following flag is used to detecte vortex
	   reinitialization escenarios where it is required to release
	   memory but without perform all release operatios like mutex
	   locks */
	axl_bool                  reader_cleanup;
	
	/** 
	 * @internal Reference to the thread created for the reader loop.
	 */
	VortexThread              reader_thread;

	/* @internal Buffer used to produce the SEQ frame generated by
	 * the vortex reader */
	char                      reader_seq_frame[50];

	/**** vortex pull module ****/
	VortexAsyncQueue        * pull_pending_events;

	/**** vortex support module state ****/
	axlList                 * support_search_path;

	/**** vortex sequender module state ****/
	VortexSequencerState    * sequencer_state;
	/* @internal Definition for the thread created for the sequencer.
	 */
	VortexThread              sequencer_thread;

	/* @internal buffer used by the sequencer to build frames to
	 * be sent (and its associated size). */
	char                    * sequencer_send_buffer;
	int                       sequencer_send_buffer_size;
	char                    * sequencer_feeder_buffer;
	int                       sequencer_feeder_buffer_size;

	/**** vortex thread pool module state ****/
	/** 
	 * @internal Reference to the thread pool.
	 */
	axl_bool                  thread_pool_exclusive;
	VortexThreadPool *        thread_pool;
	axl_bool                  thread_pool_being_stopped;

	/**** vortex greetings module state ****/
	char                    * greetings_features;
	char                    * greetings_localize;

	/**** vortex listener module state ****/
	VortexAsyncQueue        * listener_wait_lock;

	/* configuration handler for OnAccepted signal */
	axlList                 * listener_on_accept_handlers;

	/* default realm configuration for all connections at the
	 * server side */
	char                    * listener_default_realm;

	/** 
	 * @internal Handler used to decide how to split frames.
	 */
	VortexChannelFrameSize  next_frame_size;
	
	/** 
	 * @internal Reference to user defined pointer to be provided
	 * to the function once executed.
	 */
	axlPointer              next_frame_size_data;

	/** 
	 * @internal References to client connection created handler.
	 */
	VortexClientConnCreated conn_created;
	axlPointer              conn_created_data;
	
	axlList               * port_share_handlers;
	VortexMutex             port_share_mutex;

	/* write timeout control */
	int                     conn_close_on_write_timeout;
	axl_bool                disable_conn_close_on_write_timeout;
};

#endif /* __VORTEX_CTX_PRIVATE_H__ */


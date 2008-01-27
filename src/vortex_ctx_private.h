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
#ifndef __VORTEX_CTX_PRIVATE_H__
#define __VORTEX_CTX_PRIVATE_H__

/* global included */
#include <axl.h>
#include <vortex.h>

struct _VortexCtx {

	/* global hash to store arbitrary data */
	VortexHash         * data;

	/* @internal Allows to check if the vortex library is in exit
	 * transit.
	 */
	bool                 vortex_exit;

	/* global mutex */
	VortexMutex          frame_id_mutex;
	VortexMutex          connection_id_mutex;
	VortexMutex          search_path_mutex;
	VortexMutex          exit_mutex;
	VortexMutex          tls_init_mutex;
	VortexMutex          listener_mutex;
	VortexMutex          listener_unlock;
	VortexMutex          log_mutex;
	bool                 use_log_mutex;

	/* default configurations */
	int                  backlog;
	bool                 enforce_profiles_supported;
	
	/* local log variables */
	bool                 debug_checked;
	bool                 debug;
	
	bool                 debug2_checked;
	bool                 debug2;
	
	bool                 debug_color_checked;
	bool                 debug_color;

	VortexLogHandler     debug_handler;

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
	long int             connection_id;
	bool                 connection_enable_sanity_check;

	/** 
	 * @internal
	 * @brief Auto TLS profile negotiation internal support variables.
	 */
	bool                 connection_auto_tls;
	bool     	     connection_auto_tls_allow_failures;
	char  *              connection_auto_tls_server_name;
	
	/**
	 * @internal xml caching functions:
	 */ 
	axlHash            * connection_xml_cache;
	axlHash            * connection_hostname;
	VortexMutex          connection_xml_cache_mutex;
	VortexMutex          connection_hostname_mutex;
	
	/**
	 * @internal Vortex reporting.
	 */
	VortexMutex          connection_new_notify_mutex;
	axlList           *  connection_new_notify_list;

	/** 
	 * @internal Default timeout used by vortex connection operations.
	 */
	long int             connection_std_timeout;
	bool                 connection_timeout_checked;
	char              *  connection_timeout_str;
	long int             connection_connect_std_timeout;
	bool                 connection_connect_timeout_checked;
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
	long int             frame_id;

	/**** vortex profiles module state ****/
	VortexHash        * registered_profiles;
	axlList           * profiles_list;

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
	axlDtd               * sasl_dtd;
	axlDtd               * xml_rpc_boot_dtd;

	/**** vortex reader module state ****/
	VortexAsyncQueue        * reader_queue;
	VortexAsyncQueue        * reader_stopped;
	/** 
	 * @internal Reference to the thread created for the reader loop.
	 */
	VortexThread              reader_thread;
	/* @internal value that allows the vortex to accept incoming
	 * messages with msgno=1 rather than the value that should be
	 * used 0.  See \ref vortex_reader_allow_msgno_starting_from_1.
	 */
	bool                      reader_accept_msgno_startig_from_1;

	/**** vortex support module state ****/
	axlList                 * support_search_path;

	/**** vortex sequender module state ****/
	VortexAsyncQueue        * sequencer_queue;
	VortexAsyncQueue        * sequencer_stoped;
	/* @internal Definition for the thread created for the sequencer.
	 */
	VortexThread              sequencer_thread;

	/**** vortex thread pool module state ****/
	/** 
	 * @internal Reference to the thread pool.
	 */
	bool                      thread_pool_exclusive;
	VortexThreadPool *        thread_pool;
	bool                      thread_pool_being_stoped;

	/**** vortex xml rpc module state ****/
	/* @internal List of dispatch handlers and its associated
	 * mutex to handle it. */
	axlList                 * xml_rpc_service_dispatch;

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

	/**** vortex tls module state ****/
#if defined(ENABLE_TLS_SUPPORT)
	/* @internal Internal default handlers used to define the TLS
	 * profile support. */
	VortexTlsAcceptQuery               tls_accept_handler;
	VortexTlsCertificateFileLocator    tls_certificate_handler;
	VortexTlsPrivateKeyFileLocator     tls_private_key_handler;

	/* default ctx creation */
	VortexTlsCtxCreation               tls_default_ctx_creation;
	axlPointer                         tls_default_ctx_creation_user_data;

	/* default post check */
	VortexTlsPostCheck                 tls_default_post_check;
	axlPointer                         tls_default_post_check_user_data;
	bool                               tls_already_init;
#endif /* end ENABLE_TLS_SUPPORT */

	/**** vortex sasl module state ****/
#if defined(ENABLE_SASL_SUPPORT)
	/* @internal ANONYMOUS validation handler. This is
	 * actually invoked by the library to notify user space that a
	 * ANONYMOUS auth request was received. */
	VortexSaslAuthAnonymous            sasl_anonymous_auth_handler;
	
	/* @internal PLAIN validation handler. This is actually
	 * invoked by the library to notify user space that a PLAIN
	 * auth request was received. */
	VortexSaslAuthPlain                sasl_plain_auth_handler;

	/** 
	 * @internal
	 * @brief CRAM-MD5 validation handler. This is actually invoked by
	 * the library to notify user space that a CRAM-MD5 auth request was
	 * received.
	 */
	VortexSaslAuthCramMd5              sasl_cram_md5_auth_handler;

	/** 
	 * @internal
	 * @brief DIGEST-MD5 validation handler. This is actually invoked by
	 * the library to notify user space that a DIGEST-MD5 auth request was
	 * received.
	 */
	VortexSaslAuthDigestMd5            sasl_digest_md5_auth_handler;

	/** 
	 * @internal
	 * @brief EXTERNAL validation handler. This is actually invoked by
	 * the library to notify user space that a EXTERNAL auth request was
	 * received.
	 */
	VortexSaslAuthExternal             sasl_external_auth_handler;

	/** 
	 * @internal
	 * @brief ANONYMOUS validation handler. This is actually invoked by
	 * the library to notify user space that a ANONYMOUS auth request was
	 * received. Passes the user-defined pointer.
	 */
	VortexSaslAuthAnonymousFull        sasl_anonymous_auth_handler_full;

	/** 
	 * @internal
	 * @brief PLAIN validation handler. This is actually invoked by
	 * the library to notify user space that a PLAIN auth request was
	 * received. Passes the user-defined pointer.
	 */
	VortexSaslAuthPlainFull            sasl_plain_auth_handler_full;

	/** 
	 * @internal
	 * @brief CRAM-MD5 validation handler. This is actually invoked by
	 * the library to notify user space that a CRAM-MD5 auth request was
	 * received. Passes the user-defined pointer.
	 */
	VortexSaslAuthCramMd5Full          sasl_cram_md5_auth_handler_full;
	
	/** 
	 * @internal
	 * @brief DIGEST-MD5 validation handler. This is actually invoked by
	 * the library to notify user space that a DIGEST-MD5 auth request was
	 * received. Passes the user-defined pointer.
	 */
	VortexSaslAuthDigestMd5Full        sasl_digest_md5_auth_handler_full;
	
	/** 
	 * @internal
	 * @brief EXTERNAL validation handler. This is actually invoked by
	 * the library to notify user space that a EXTERNAL auth request was
	 * received. Passes the user-defined pointer.
	 */
	VortexSaslAuthExternalFull         sasl_external_auth_handler_full;
#endif	/* end ENABLE_SASL_SUPPORT */

};

#endif /* __VORTEX_CTX_PRIVATE_H__ */


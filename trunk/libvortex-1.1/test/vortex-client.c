/** 
 *  LibVortex:  A BEEP implementation for af-arch
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* include base library */
#include <vortex.h>

/* include xml-rpc implementation */
#include <vortex_xml_rpc.h>

#if defined(ENABLE_SASL_SUPPORT)
/* include sasl implementation */
#include <vortex_sasl.h>
#endif

#if defined(ENABLE_TLS_SUPPORT)
/* include tls implementation */
#include <vortex_tls.h>
#endif

/* additional libraries */
#include <readline/readline.h>
#include <readline/history.h>

/* context */
VortexCtx * ctx = NULL;

/** 
 * @brief Connect used across the vortex client.
 */
VortexConnection * connection;

/** 
 * @brief Next auto tls value to be used
 */
axl_bool           auto_tls_profile = axl_true;


void vortex_client_show_help (void) {
	printf ("Vortex-client cmd help:\n");
	printf (" -- general commands --\n");
	printf ("  help              - Show this help.\n");
	printf ("  quit              - Quit from tool.\n");
	printf ("  log               - Enable/Disable vortex console log.\n");
	printf ("  color log         - Enable/Disable vortex console log using ansi terminal colors.\n");
	printf ("\n -- connect commands --\n");
	printf ("  connect           - Connects to a remove vortex (BEEP enabled) server\n");
	printf ("  open              - The same as connect.\n");
	printf ("  enable tls        - Try to negociate underlaying TLS profile.\n");
	printf ("  auto tls          - Enable/Disable automatic TLS profile negotiation for new connections\n");
	printf ("  begin auth        - Start SASL authentication process\n");
	printf ("  close             - Close connection opened.\n");
	printf ("\n -- debuging peers --\n");
	printf ("  show profiles     - Show remote peer profiles, features and localize\n");
	printf ("                      values.\n");
	printf ("  channel status    - Returns actual channel status.\n");
	printf ("  connection status - Returns actual connection status.\n");
	printf ("\n -- raw frame support --\n");
	printf ("  write frame       - Write and sends a frame.\n");
	printf ("  write fragment    - Write and sends a frame but splitting the message\n");
	printf ("                      into several pieces to test remote frame fragment\n");
	printf ("                      reading support.\n");
	printf ("  write start msg   - Sends a raw start channel message and returns\n");
	printf ("                      the remote peer response to stdout.\n");
	printf ("  write close msg   - Sends a raw close channel message and returns\n");
	printf ("                      the remote peer response to stdout.\n");
	printf ("\n -- interacting with remote peers --\n");
	printf ("  new channel       - Creates a new channel using the vortex API.\n");
	printf ("  close channel     - Close a channel using the vortex API.\n");
	printf ("  send message      - Send a message using the vortex API.\n\n");
#if defined(ENABLE_XML_RPC_SUPPORT)
	printf ("\n -- xml-rpc (RFC3529) invocation --\n");
	printf ("  new invoke        - Creates and launch an XML-RPC invocation\n");
	printf ("                      against the BEEP peer already connected.\n");
#endif

}

void vortex_show_initial_greetings (void) 
{
	printf ("Vortex-client v.%s: a cmd tool to test vortex (and BEEP-enabled) peers\n", VERSION);
	printf ("Copyright (c) 2005 Advanced Software Production Line, S.L.\n");
}

char  *  get_and_check_cancel (void) {
	char  * response = NULL;

	printf ("  0) cancel process\n");
	response = readline ("you chose: ");
	if (axl_memcmp (response, "0", 1)) {
		axl_free (response);
		response = NULL;
		printf ("canceling process..\n");
	}
	return response;
}

VortexFrameType check_frame_type (char  * response) {

	v_return_val_if_fail (response && (* response), VORTEX_FRAME_TYPE_UNKNOWN);

	if (axl_memcmp (response, "1", 1)) {
		return VORTEX_FRAME_TYPE_MSG;
	}
	if (axl_memcmp (response, "2", 1)) {
		return VORTEX_FRAME_TYPE_RPY;
	}
	if (axl_memcmp (response, "3", 1)) {
		return VORTEX_FRAME_TYPE_ERR;
	}
	if (axl_memcmp (response, "4", 1)) {
		return VORTEX_FRAME_TYPE_ANS;
	}
	if (axl_memcmp (response, "5", 1)) {
		return VORTEX_FRAME_TYPE_NUL;
	}
	return VORTEX_FRAME_TYPE_UNKNOWN;
}
	

int  get_number_int (char  * ans, int limit) {
	char  * response;
	int    num;
 label:
	printf ("%s", ans);
        response  = readline ("you chose: ");
        num       = atoi (response);
	if (num < 0 || num > limit ) {
                axl_free (response);
		printf ("You must provide a valid number from 0 up to %d\n", limit);
		goto label;
	}
        axl_free (response);
        response = NULL;
	return num;
}

unsigned int get_number_long (char  * ans, long limit) {
	char         * response;
	unsigned int   num;

 label:
	printf ("%s", ans);
	response  = readline ("you chose: ");
	num       = strtod (response, NULL);
	if (num < 0 || num > limit ) {
                axl_free (response);
		printf ("You must provide a valid number from 0 up to %ld\n", limit);
		goto label;
	}
        axl_free (response);
        response = NULL;
	return num;
}

void vortex_client_write_frame (VortexConnection * connection, axl_bool      fragment)
{
	char             * response          = NULL;
	VortexFrameType    type              = VORTEX_FRAME_TYPE_UNKNOWN;
	int                channel_num       = 0;
	int                message_num       = 0;
	long               sequence_num      = 0;
	int                payload_size      = 0;
	int                ansno_num         = 0;
	char             * payload           = NULL;
	char             * str               = NULL;
	char             * the_whole_payload = NULL;
	axl_bool           continuator;
	char             * the_frame;
	int                iterator;
	int                length;
	
	printf ("What type of frame do you want to send?\n");
	printf ("  1) MSG\n");
	printf ("  2) RPY\n");
	printf ("  3) ERR\n");
	printf ("  4) ANS\n");
	printf ("  5) NUL\n");
	response = get_and_check_cancel ();
	if (response == NULL)
		return;

	/* get frame type */
	type = check_frame_type (response);
	axl_free (response);

	channel_num = get_number_int ("What is the channel to be used to send frame?\n",
				      MAX_CHANNEL_NO);
	
	message_num = get_number_int ("What is the message number to set?\n",
				      MAX_CHANNEL_NO);
	
	printf ("What message continuator to set?\n");
	printf ("  1) . (no more message indicator)\n");
	printf ("  2) * (more messages to come indicator)\n");
	response = get_and_check_cancel ();
	if (response == NULL)
		return;

	if (axl_memcmp (response, "1", 1))
		continuator = axl_false;
	else
		continuator = axl_true;
	axl_free (response);

	sequence_num = get_number_long ("What is the sequence number to be used?\n", MAX_SEQ_NO);

	ansno_num    = get_number_int  ("What is the ansno number value ?\n", 214783647);

	payload_size = get_number_int  ("What is the payload size?\n", 214783647);

	printf ("Write the frame payload to send (control-D to finish on last line):\n");
	printf ("NOTE: every line you introduce, is completed with a \\r\\n (aka CR LF) terminator\n");
	while ((payload = readline (NULL)) != NULL) {
		if (the_whole_payload == NULL) {
			the_whole_payload = axl_strdup_printf ("%s\r\n", payload);
			axl_free (payload);
			continue;
		}
		str = the_whole_payload;
		the_whole_payload = axl_strdup_printf ("%s%s\r\n", the_whole_payload, payload);
		axl_free (str);
		axl_free (payload);
	}

	printf ("Building frame..\n");
	the_frame = vortex_frame_build_up_from_params (type,
						       channel_num,
						       message_num,
						       continuator,
						       sequence_num,
						       payload_size,
						       ansno_num,
						       the_whole_payload ? the_whole_payload : "");
	if (the_whole_payload != NULL)
		axl_free (the_whole_payload);
	printf ("frame to send:\n%s\nDo you want to send this frame?\n", the_frame);
	printf ("  1) yes\n");
	response = get_and_check_cancel ();
	if (response == NULL) 
		return;
	
	printf ("Sending frame..\n");
	if (fragment) {
		/* send a raw frame (splitting it) */
		iterator = 0;
		length   = strlen (the_frame) / 3;
		while (iterator < length) {
			printf ("sending from %d to %d bytes..\n", iterator, length);
			if (!vortex_frame_send_raw (connection, (the_frame + iterator), 
						    length)) {
				printf ("unable to send frame in raw mode (fragments): %s\n", 
					 vortex_connection_get_message (connection));
			}
			
			/* update indexes */
			iterator +=length;
			if ((strlen (the_frame + iterator)) < length)
				length = strlen (the_frame + iterator);

		}
		

	} else {
		/* send a raw frame (complete one)  */
		if (!vortex_frame_send_raw (connection, the_frame, strlen (the_frame))) {
			printf ("unable to send frame in raw mode: %s\n", 
				 vortex_connection_get_message (connection));
		}
	}
	axl_free (the_frame);

	return;
}

void vortex_client_get_channel_status (VortexConnection * connection)
{
	char          * response;
	int             channel_num;
	VortexChannel * channel;

	response    = readline ("Channel number to get status: ");
	channel_num = atoi (response);
	axl_free (response);

	channel = vortex_connection_get_channel (connection, channel_num);
	if (channel == NULL) {
		printf ("No channel %d is actually opened, leaving status operation..\n", channel_num);
		return;
	}
	
	printf ("Channel %d status is: \n", channel_num);
	printf ("  Profile definition: \n");
	printf ("     %s\n", vortex_channel_get_profile (channel));
	printf ("  Synchronization: \n");
	if (vortex_channel_get_next_msg_no (channel) == 0) 
		printf ("     Last msqno sent:          no message sent yet\n");
	else
		printf ("     Last msqno sent:          %d\n",  vortex_channel_get_next_msg_no (channel) - 1);
	
	printf ("     Next msqno to use:        %d\n",  vortex_channel_get_next_msg_no (channel));
	if (vortex_channel_get_last_msg_no_received (channel) == -1) 
		printf ("     Last msgno received:      no message received yet\n");
	else
		printf ("     Last msgno received:      %d\n",  vortex_channel_get_last_msg_no_received (channel));
	printf ("     Next reply to sent:       %d\n",  vortex_channel_get_next_reply_no (channel));
	printf ("     Next reply exp. to recv:  %d\n",  vortex_channel_get_next_expected_reply_no (channel));
	printf ("     Next seqno to sent:       %u\n",  vortex_channel_get_next_seq_no (channel));
	printf ("     Next seqno to receive:    %u\n",  vortex_channel_get_next_expected_seq_no (channel));
	
	return;
}

axl_bool       check_connected (char  * error, VortexConnection * connection) {
	
	if ((connection == NULL) || (!vortex_connection_is_ok (connection, axl_false))) {
		printf ("%s, connect first\n", error);
		return axl_false;
	}
	return axl_true;

}

void vortex_client_write_start_msg (VortexConnection * connection)
{
	axlList         * profiles;
	char            * response;
	char            * the_frame;
	char            * the_msg;
	int               profile;
	int               channel_num;
	int               iterator = 1;
	VortexChannel   * channel0;
	VortexFrame     * frame;
	WaitReplyData   * wait_reply;
	int               msg_no;

	/* get profile to use */
	printf ("This procedure will send an start msg to remote peer. This actually\n");
	printf ("bypass vortex library channel creation API. This is used for debuging\n");
	printf ("beep enabled peer responses and its behaviour.\n\n");
		 
	printf ("Select what profile to use to create for the new channel?\n");
	profiles = vortex_connection_get_remote_profiles (connection);
	printf ("profiles for this peer: %d\n", axl_list_length (profiles));
	iterator = 0;
	while (iterator < axl_list_length (profiles)) {
		/* print */
		printf ("  %d) %s\n", iterator, (char *) axl_list_get_nth (profiles, iterator));
	       
		/* get the next profile */
		iterator++;
	}
	response = get_and_check_cancel ();
	if (response == NULL) {
		printf ("canceling process..\n");
		return;
	}
	profile  = atoi (response);
	axl_free (response);

	/* get channel num to use */
	channel_num = get_number_int ("What channel number to create: ", 2147483647);
	
	/* get management channel */
	channel0    = vortex_connection_get_channel (connection, 0);

	/* create the start message */
	the_msg     = axl_strdup_printf ("Content-Type: application/beep+xml\x0D\x0A\x0D\x0A<start number='%d'>\x0D\x0A   <profile uri='%s' />\x0D\x0A</start>\x0D\x0A",
					 channel_num, 
					 (char  *) axl_list_get_nth (profiles, profile - 1));
	/* free the list */
	axl_list_free (profiles);
	
	the_frame   = vortex_frame_build_up_from_params (VORTEX_FRAME_TYPE_MSG,
							 0,
							 vortex_channel_get_next_msg_no (channel0),
							 axl_false,
							 vortex_channel_get_next_seq_no (channel0),
							 strlen (the_msg),
							 0,
							 the_msg);

	/* send the message */
	wait_reply = vortex_channel_create_wait_reply ();
	if (!vortex_channel_send_msg_and_wait (channel0, the_msg, strlen (the_msg), &msg_no, wait_reply)) {
		vortex_channel_free_wait_reply (wait_reply);
		printf ("unable to send start message");
		return;
	}
	
	/* free no longer needed data */
	axl_free (the_msg);
	axl_free (the_frame);

	/* wait for response */
	printf ("start message sent, waiting reply..");
	frame = vortex_channel_wait_reply (channel0, msg_no, 
					   wait_reply);

	/* check frame replied */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_RPY) {
		printf ("channel start ok, message (size: %d)\n%s", 
			vortex_frame_get_payload_size (frame),
			(char *) vortex_frame_get_payload (frame));
	}else {
		printf ("channel start failed, message (size: %d)\n%s",
			 vortex_frame_get_payload_size (frame),
			(char *) vortex_frame_get_payload (frame));
	}
	vortex_frame_free (frame);
	return;
}

axl_bool      vortex_client_on_close_received (int                channel_num, 
					       VortexConnection * connection, 
					       axlPointer         user_data) {

	printf ("\n*** Close notification received: ***\n");
	printf ("*** A close notification for the channel: %d over the connection %d) from %s:%s\n",
		 channel_num, vortex_connection_get_id (connection),
		 vortex_connection_get_host (connection),
		 vortex_connection_get_port (connection));
	
	return axl_true;
}

void vortex_client_on_frame_received     (VortexChannel    * channel, 
					  VortexConnection * connection, 
					  VortexFrame      * frame, 
					  axlPointer user_data)
{
	printf ("*** A frame has been received over the channel: %d inside the connection %d) from %s:%s\n",
		 vortex_channel_get_number (channel), vortex_connection_get_id (connection),
		 vortex_connection_get_host (connection),
		 vortex_connection_get_port (connection));
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ANS) {
		printf ("*** Ansno number received: %d\n", vortex_frame_get_ansno (frame));
	}

	printf ("*** its content is:    \n%s",
		(char*) vortex_frame_get_payload (frame));

	return;
}
  	




void vortex_client_new_channel (VortexConnection * connection)
{
	axlList       * profiles;
	char          * response;
	int             profile;
	int             channel_num;
	int             iterator = 1;
	VortexChannel * channel;

	/* get profile to use */
	printf ("This procedure will request to create a new channel using Vortex API.\n");
	printf ("NOTE: this procedure will create a channel but, without having implemented.\n");
	printf ("      the profile selected for the channel, you can only be notified about\n");
	printf ("      the content of the \"close channel\" and \"frame receive\" events.\n\n");

		 
	printf ("Select what profile to use to create for the new channel?\n");
	profiles = vortex_connection_get_remote_profiles (connection);
	printf ("profiles advised for this peer: %d\n", axl_list_length (profiles));
	printf ("  1) provide one profile: \n");
	
	iterator = 2;
	while ((iterator - 2)  < axl_list_length (profiles)) {
		/* print */
		printf ("  %d) %s\n", iterator, (char *) axl_list_get_nth (profiles, iterator - 2));
	       
		/* get the next profile */
		iterator++;
	} /* end while */

	response = get_and_check_cancel ();
	if (response == NULL) {
		return;
	}
	profile  = atoi (response);
	axl_free (response);

	/* get the manual profile if selected */
	if (profile == 1) {
		response    = readline ("URI: ");
	} else {
		response    = axl_strdup ((char  *) axl_list_get_nth (profiles, profile - 2));
	}

	/* get channel num to use */
	channel_num = get_number_int ("What channel number to create: ", 2147483647);

	printf ("creating new channel..");
	channel     = vortex_channel_new (connection, 
					  channel_num, 
					  response,
					  vortex_client_on_close_received, NULL, 
					  vortex_client_on_frame_received, NULL, 
					  NULL, NULL);
	/* free profile list */
	axl_free (response);
	axl_list_free (profiles);
	if (channel == NULL) {
		printf ("failed\n");
		return;
	}
	printf ("ok, channel created: %d\n", vortex_channel_get_number (channel));

	return;
}

void vortex_client_write_close_msg (VortexConnection * connection)
{
	printf ("no implemented yet..\n");
}

void vortex_client_close_channel (VortexConnection * connection)
{
	VortexChannel * channel;
	int             channel_num;

	printf ("This procedure will close a channel using the vortex API.\n");
	/* get channel num to use */
	channel_num = get_number_int ("What channel number to close: ", 2147483647);
	
	if (channel_num == 0) {
		printf ("You can't close channel 0. It will be closed when session (or connection) is closed\n");
		return;
	}

	/* get a channel reference and also check if this channel is
	 * registered by the vortex connetion */
	channel = vortex_connection_get_channel (connection, channel_num);
	if (channel == NULL) {
		printf ("no channel %d is opened, finishing..\n", channel_num);
		return;
	}

	/* close the channel */
	if (vortex_channel_close (channel, NULL)) 
		printf ("Channel close: ok\n");
	else
		printf ("Channel close: failed\n");
	
	return;
}

void vortex_client_send_message (VortexConnection * connection)
		
{
	int               channel_num;
	VortexChannel   * channel;
	VortexFrame     * frame;
	WaitReplyData   * wait_reply   = NULL;
	char            * msg_to_send;
	char            * wait;
	axl_bool          do_a_wait;
	axl_bool          result;
	int               msg_no;

	printf ("This procedure will send a message using the vortex API.\n");
	
	/* get the channel number to use and check this message exists */
	channel_num = get_number_int  ("What channel do you want to use to send the message? ",
				       MAX_CHANNEL_NO);
	
	if (!vortex_connection_channel_exists (connection, channel_num)) {
		printf ("This channel doesn't exists..\n");
		return;
	}

	/* get the message to send */
	msg_to_send = readline ("Type the message to send:\n");
	if (msg_to_send == NULL) {
		printf ("no message to send..\n");
		return;
	}

	/* get if user wants to wait for the message */
	printf ("Do you want to wait for message reply? (Y/n) ");
	wait  = readline (NULL);
	do_a_wait = ((wait != NULL) && ((strlen (wait) == 0) || axl_cmp ("Y", wait) || axl_cmp ("y", wait)));
	if (wait != NULL)
		axl_free (wait);

	/* send the message */
	channel             = vortex_connection_get_channel (connection, channel_num);
	if (do_a_wait) {
		wait_reply  = vortex_channel_create_wait_reply ();
		result      = vortex_channel_send_msg_and_wait (channel, msg_to_send, 
								strlen (msg_to_send), &msg_no, wait_reply);
	}else {
		result      = vortex_channel_send_msg (channel, msg_to_send, strlen (msg_to_send), &msg_no);
	}
	
	/* check result */
	if (!result) {
		printf ("Unable to send the message\n");
		axl_free (msg_to_send);
		if (do_a_wait)
			vortex_channel_free_wait_reply (wait_reply);
		return;
	}
	axl_free (msg_to_send);
	printf ("Message number %d was sent..\n", msg_no);

	/* do a wait if posible */
	if (do_a_wait) {
		printf ("waiting for reply...");
		frame = vortex_channel_wait_reply (channel, msg_no, wait_reply);
		printf ("reply received: \n%s\n", (char*) vortex_frame_get_payload (frame));
		vortex_frame_free (frame);
		return;
	}
	printf ("leaving without waiting..\n");
	return;
}

axl_bool  __vortex_client_connection_status (axlPointer key, axlPointer value, axlPointer user_data)
{
	VortexChannel * channel = value;

	printf ("  channel: %d, profile: %s\n", 
		 vortex_channel_get_number (channel), 
		 vortex_channel_get_profile (channel));

	return axl_false;
}

void vortex_client_connection_status (VortexConnection * connection)
{
#if defined(ENABLE_SASL_SUPPORT)
	char  * authid = vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID);

	printf ("Connection status:\n   Id=%d\n   TLS status=%s\n   SASL auth status=%s, auth id=%s\n", 
		 vortex_connection_get_id (connection),
		 vortex_connection_is_tlsficated (connection) ? "On" : "Off",
		 vortex_sasl_is_authenticated    (connection) ? "On" : "Off",
		 authid ? authid : "none");

	printf ("Created channel over this session:\n");

	vortex_connection_foreach_channel (connection, __vortex_client_connection_status, NULL);
#endif
	
	return;
}

void vortex_client_begin_auth (void) {
#if defined(ENABLE_SASL_SUPPORT)	
	int       profile;
	char    * profile_selected = NULL;
	char    * response;

	/* SASL status notification */
	VortexStatus       status;
	char             * status_message;

	/* check for connection status */
	if (!check_connected ("can't start SASL auth if not connected first", connection))
		return;

	/* check and init SASL state on the provided context */
	if (! vortex_sasl_init (ctx)) {
		printf ("Unable to begin SASL negotiation. Current Vortex Library doesn't support SASL");
		return;
	}

	printf ("Choose a profile to negociate: \n");
	printf ("1) %s\n", VORTEX_SASL_ANONYMOUS);
	printf ("2) %s\n", VORTEX_SASL_EXTERNAL);
	printf ("3) %s\n", VORTEX_SASL_PLAIN);
	printf ("4) %s\n", VORTEX_SASL_CRAM_MD5);
	printf ("5) %s\n", VORTEX_SASL_DIGEST_MD5);
	printf ("6) %s\n", VORTEX_SASL_GSSAPI);
	printf ("7) %s\n", VORTEX_SASL_KERBEROS_V4);
	printf ("0) Cancel\n");
	profile = get_number_int ("Which profile should I use to authenticate?\n", 7);
	switch (profile) {
	case 0:
		/* do nothing */
		break;
	case 1:
		/* anonymous mech */
		profile_selected = VORTEX_SASL_ANONYMOUS;
		response = readline ("Which is the anonymous token to be used (example: anonymous@foobar.com): ");
		vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN,
					   response, axl_free);
		break;
	case 2:
		/* external mech */
		profile_selected = VORTEX_SASL_EXTERNAL;
		response = readline ("Which is the auth id to be used (example: bob): ");
		vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
					   response, axl_free);
		break;
	case 3:
		/* plain mech */
		profile_selected = VORTEX_SASL_PLAIN;

	get_user_and_password_data:

		/* get auth id to be used */
		response = readline ("Which is the auth id to be used (example: bob): ");
		vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
					   response, axl_free);

		/* get proxy auth id to be used */
		if (axl_cmp (profile_selected, VORTEX_SASL_PLAIN) ||
		    axl_cmp (profile_selected, VORTEX_SASL_DIGEST_MD5)) {
			response = readline ("Which is the auth id proxy to be used (example: alice): ");
			vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
						   response, axl_free);
		}

		/* get password to be used */
		response = readline ("Which password to be used (example: secret): ");
		vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
					   response, axl_free);

		/* get realm to be used */
		if (axl_cmp (profile_selected, VORTEX_SASL_DIGEST_MD5)) {
			response = readline ("Which is the realm to be used (example: aspl.es): ");
			vortex_sasl_set_propertie (connection, VORTEX_SASL_REALM, response, axl_free);
		}
		
		break;
	case 4:
		/* cram md5 mech (same as plain) */
		profile_selected = VORTEX_SASL_CRAM_MD5;
		goto get_user_and_password_data;
		break;
	case 5:
		/* digest md5 mech */
		profile_selected = VORTEX_SASL_DIGEST_MD5;
		goto get_user_and_password_data;
		break;
	case 6:
		/* gssapi mech */
		profile_selected = VORTEX_SASL_GSSAPI;
		printf ("No supported yet!\n");
		return;
	case 7:
		/* kerberos v4 mech */
		profile_selected = VORTEX_SASL_KERBEROS_V4;
		printf ("No supported yet!\n");
		return;
	}

	/* begin SASL negotiation */
	vortex_sasl_start_auth_sync (connection, profile_selected, &status, &status_message);

	/* print error */
	switch (status) {
	case VortexOk:
		printf ("OK: SASL successfully negociated, message reported: %s\n", 
			 status_message);
		break;
	default:
	case VortexError:
		printf ("FAIL: There was an error for SASL negotiation, message reported: %s\n",
			 status_message);
		break;

	}

	return;
#else
	printf ("vortex-client does not have SASL support.\n");
	return;
#endif
}

#if defined(ENABLE_XML_RPC_SUPPORT)
/** 
 * @brief Perform an XML-RPC invocation on the given connection.
 * 
 * @param connection The connection where the XML-RPC invocation will
 * take place.
 */
void vortex_client_begin_xml_rpc_invocation (VortexConnection * connection)
{
	XmlRpcMethodCall     * invocator;
	XmlRpcMethodResponse * response;
	XmlRpcMethodValue    * value;
	VortexChannel        * channel;
	VortexStatus           status;
	char                 * message;

	/* method data */
	char                 * method_name;
	char                 * method_count;
	char                 * param_type;
	char                 * param_value;

	int                    count;
	int                    iterator;

	/* create the XML-RPC channel in a synchronous way */
	channel = vortex_xml_rpc_boot_channel_sync (connection, NULL, NULL, &status, &message);

	/* show message returned */
	if (status == VortexError) {
		printf ("Unable to create XML-RPC channel, message was: \n   %s\n", message);
		return;
	}else {
		printf ("XML-RPC channel created, message was: \n   %s\n", message);
	}


	
	/* create invocator object */
	method_name  = readline ("method name to invoke: ");
	method_count = readline ("method count to invoke: ");
	count        = atoi (method_count);
	invocator    = method_call_new (ctx, method_name, count);
	
	axl_free (method_name);
	axl_free (method_count);

	/* configure method parameters */
	iterator = 0;
	while (iterator < count) {
		
		/* get the method type */
		param_type  = readline ("method parameter type (int, string, boolean): ");
		param_value = readline ("method parameter value: ");
		
		/* create the method value */
		value = vortex_xml_rpc_method_value_new_from_string2 (ctx, param_type, param_value);
		
		/* release the values allocated */
		axl_free (param_type);
		axl_free (param_value);

		if (value == NULL) {
			printf ("an error was detected while building the parameter\n");
			continue;
		}

		/* add the method value to the invocator */
		method_call_add_value (invocator, value);

		/* updte the iterator */
		iterator++;
	}				       

	/* peform a synchronous method */
	printf ("   Performing XML-RPC invocation..\n");
	response = vortex_xml_rpc_invoke_sync (channel, invocator);

	switch (method_response_get_status (response)) {
	case XML_RPC_OK:
		printf ("Reply received ok, result is: %s\n", method_response_stringify (response));
		break;
	default:
		printf ("RPC invocation have failed: (code: %d) : %s\n",
			 method_response_get_fault_code (response),
			 method_response_get_fault_string (response));
		break;
	}

	/* free the response received */
	method_response_free (response);

	return;
}
#endif


/** 
 * @brief Support function to __vortex_connection_has_uri_from that
 * finally extract uri information into the provided values.
 */
axl_bool      __vortex_connection_has_uri_from_aux (char  * uri, char  ** host, char  ** port, axl_bool      * activate_tls)
{
	char     ** pieces     = NULL;
	char     ** host_part  = NULL;
	int         result     = axl_false;

	/* first check if we have an uri from using :// */
	pieces = axl_stream_split (uri, 1, "://");
	if (pieces == NULL || pieces [1] == NULL) {
		/* we don't have a :// so, the uri received is the form host[:port] */
		host_part = axl_stream_split (uri, 1, ":");
		if (host_part == NULL || host_part [1] == NULL) {
			/* no host part */
			(* host ) = axl_strdup (uri);
		}else {
			/* host and port part */
			(* host)  = axl_strdup (host_part[0]);
			(* port)  = axl_strdup (host_part[1]);
		}

		/* free and flag that the values where read */
                if (host_part != NULL)
		        axl_stream_freev (host_part);
		result = axl_true;
	}else {
		/* we have a :// specification, use profile
		 * information to figure out which is the port to
		 * use */
	} 

        /* free pieces memory only if allocated */
        if (pieces != NULL)
		axl_stream_freev (pieces);

	return result;
}

/** 
 * @brief Helper function to the main loop that allows to check if the
 * given command containis a URI value specifying host and port to
 * connect.
 *
 * URI values allows are the following:
 *  
 *  - host
 *  - host:port
 *  - beep://host:port
 *  - beeps://host:port
 *  - xmlrpc.beep://host:port
 *  - xmlrpc.beeps://host:port
 * 
 * All previous URI specifications are resumed into the following:
 * 
 *  - [profile.] [beeps:// | beep://] host [:port]
 * 
 * That is, an optional profile implementation, an optional beep
 * protocol specification, that is required is the profile part is
 * used, followed by the compulsory host name and ended by the
 * optional port value.
 *  
 * @param line The command line where the URI could be found.
 * 
 * @param host Optional reference where the host information will be
 * stored if a URI is found.
 *
 * @param port Optional reference where the port information will be
 * stored if a URI is found.
 *
 * @param activate_tls Optional reference where the TLS activation is
 * required to be negociated at connection time.
 *
 * 
 * @return axl_true if the URI is activated or axl_false if the data must be
 * required.
 */
axl_bool      __vortex_connection_has_uri_from (char       * line, 
						char      ** host, 
						char      ** port, 
						axl_bool   * activate_tls)
{
	char  ** line_pieces = NULL;
	int      iterator    = 0;
	axl_bool result      = axl_false;

        /* set default values for the host and the port */
        (* host ) = NULL;
        (* port ) = NULL;

	/* get current pieces */
	line_pieces  = axl_stream_split (line, 1, " ");
	
	/* show all pieces found */
        if (line_pieces != NULL) {
		for (iterator = 0; line_pieces[iterator]; iterator++) {
			/* skip empty content */
			if ((line_pieces[iterator] == NULL) || (strlen (line_pieces[iterator]) == 0))
				continue;

			/* skip connect keyword */
			if (axl_cmp ("connect", line_pieces[iterator]))
				continue;

			/* use the information found to get URI
			 * information */
			result = __vortex_connection_has_uri_from_aux (line_pieces[iterator], host, port, activate_tls);
		}
        }

	/* release memory */
	axl_stream_freev (line_pieces);

	/* report that no URI was found */
	return result;
}

int main (int argc, char *argv[])
{
        char             * host         = NULL;
        char             * port         = NULL;
        axl_bool           activate_tls = axl_false;
        axlList          * profiles     = NULL;
	int                iterator;
	char             * line         = NULL;
	
	/* TLS status notification */
	VortexStatus       status;
	char             * status_message;

	/* some resets to make gcc happy if tls is not activated */
	status         = 0;
	status_message = NULL;

	/* init the context */
	ctx = vortex_ctx_new ();
	
	/* init vortex library */
	if (!vortex_init_ctx (ctx)) {
		printf ("ERROR: unable to initialize vortex library");
		exit (-1);
	}

	vortex_show_initial_greetings ();

	while (axl_true) {

		/* unref previous command */
		if (line != NULL)
			axl_free (line);
		line = NULL;
		
		if ((vortex_connection_is_ok (connection, axl_false)))
			line = readline ("[===] vortex-client > ");
		else
			line = readline ("[=|=] vortex-client > ");

		if (line == NULL) {
			continue;
		}
		
		/* add the command read to the history */
		add_history (line);

		if (axl_memcmp ("help", line, 4)) {
			vortex_client_show_help ();
			continue;
		}

		if (axl_memcmp ("log", line, 3)) {
			vortex_log_enable (ctx, ! vortex_log_is_enabled (ctx) );
			printf (" vortex log is: %s\n", vortex_log_is_enabled (ctx) ? "enabled" : "disabled");
			continue;
		}

		if (axl_memcmp ("color log", line, 9)) {
			vortex_color_log_enable (ctx, ! vortex_color_log_is_enabled (ctx));

			printf (" vortex color log is: %s\n", vortex_color_log_is_enabled (ctx) ? "enabled" : "disabled");
			continue;
		}

		if (axl_memcmp ("show profiles", line, 13)) {
			if (!check_connected ("can't show remote peer profiles", connection))
				continue;

			/* check for supported profiles */
			printf ("Supported remote peer profiles:\n");
			profiles = vortex_connection_get_remote_profiles (connection);
			iterator = 0;
			while (iterator < axl_list_length (profiles)) {
				/* print */
				printf ("  %s\n", (char *) axl_list_get_nth (profiles, iterator));
	       
				/* get the next profile */
				iterator++;
			} /* end if */
			axl_list_free (profiles);

			/* check for features requested by remote peer */
			if (vortex_connection_get_features (connection)) {
				printf ("Features:\n  %s\n", vortex_connection_get_features (connection));
			}

			/* check for localize requested by remote peer */
			if (vortex_connection_get_features (connection)) {
				printf ("Localize:\n  %s\n", vortex_connection_get_localize (connection));
			}
			
			continue;
		}

		if (axl_memcmp ("connection status", line, 17)) {
			if (!check_connected ("can't show connection status", connection))
				continue;
			
			vortex_client_connection_status (connection);
			
			continue;
		}

		if (axl_memcmp ("auto tls", line, 8)) {
#if defined(ENABLE_TLS_SUPPORT)
			/* enable auto tls profile negotiation not allowing TLS failures */
			vortex_tls_set_auto_tls (ctx, auto_tls_profile, axl_false, NULL);
			printf ("Auto TLS profile negotiation is: %s\n", auto_tls_profile ? "ON" : "OFF");
			auto_tls_profile = !auto_tls_profile;
			continue;
#else
			printf ("Current build does not have TLS support.\n");
			continue;
#endif
		}

		if (axl_memcmp ("enable tls", line, 10)) {
			if (!check_connected ("can't enable TLS if not connected first", connection))
				continue;

#if defined(ENABLE_TLS_SUPPORT)
			/* initialize and check if current vortex library supports TLS */
			if (! vortex_tls_init (ctx)) {
				printf ("Unable to activate TLS, Vortex Library is not prepared\n");
				continue;
			}

			/* enable TLS negotiation */
			connection = vortex_tls_start_negotiation_sync (connection, NULL, 
									&status,
									&status_message);

			printf ("\nTLS Negociation status: %s, message=%s\n",
				 (status == VortexOk) ? "Ok" : "Error",
				 (status_message != NULL) ? status_message : "none");
#else
			printf ("Current build does not have TLS support.\n");
			continue;
#endif

			
			continue;
		}

		if (axl_memcmp ("begin auth", line, 10)) {
			vortex_client_begin_auth ();
			continue;
		}

		if (axl_memcmp ("open", line, 4) ||
		    axl_memcmp ("connect", line, 7)) {

			/* close previous  */
			if (vortex_connection_is_ok (connection, axl_false))
				vortex_connection_close (connection);
			
			/* check if the user has specified a host:port
			 * value */
			if (!__vortex_connection_has_uri_from (line, &host, &port, &activate_tls)) {
				
				/* get user data and connect to vortex server */
				host = readline ("server to connect to: ");
				if (host == NULL) {
					printf ("Wrong value received for host, connection cancelled");
					continue;
				}
                        }

			/* if the port is not properly set, ask for it */
			if (port == NULL) {
				port = readline ("port to connect to: ");
                                if (port == NULL)
                                        port = axl_strdup ("44000");
			}
			
			printf ("connecting to %s%s%s..", host, (port != NULL) ? ":" : "", (port != NULL) ? port : "");

			/* create a vortex session */
			connection = vortex_connection_new (ctx, host, port, NULL, NULL);

			/* check if connection is ok */
			if (!vortex_connection_is_ok (connection, axl_false)) {
				
				/* check for printing a message */
				if ((host != NULL) && (port != NULL)) {
					printf ("Unable to connect to vortex server on: %s:%s vortex message was: %s\n",
							 host, port, vortex_connection_get_message (connection));
				} else {
					printf ("Unable to connect using given host and port data.\n");

				}
				/* not needed resources */
				vortex_support_free (2, host, axl_free, port, axl_free);
				vortex_connection_close (connection);
				connection = NULL;
				continue;
			}
			/* not needed resources */
			vortex_support_free (2, host, axl_free, port, axl_free);

			printf ("ok: vortex message: %s\n", vortex_connection_get_message (connection));
			continue;
		}

		if (axl_memcmp ("close channel", line, 13)) {
			if (!check_connected ("can't close channel", connection))
				continue;
			printf ("closing the channel..\n");
			vortex_client_close_channel (connection);
			continue;
		}

		if (axl_memcmp ("quit", line, 4)) {
			if (vortex_connection_is_ok (connection, axl_false)) {
				if (!vortex_connection_close (connection)) {
					printf ("there was an error while closing the connection..");
				}
			}
			
			/* free the line */
			axl_free (line);
			
			/* readline clean up */
			clear_history ();	

			/* finish vortex client */
			vortex_exit_ctx (ctx, axl_false);

			/* free context */
			vortex_ctx_free (ctx);

			/* exit from this program */
			return 0;
		}

		if (axl_memcmp ("close", line, 5)) {
			if (!check_connected ("can't close, not connected", connection))
				continue;
			
			if (!vortex_connection_close (connection)) {
				printf ("Unable to close the channel");
			}else {
				connection = NULL;
			}
			continue;
		}

		if (axl_memcmp ("write frame", line, 11)) {
			if (!check_connected ("can't write and send a frame", connection))
				continue;

			vortex_client_write_frame (connection, axl_false);
			continue;
		}

		if (axl_memcmp ("channel status", line, 14)) {
			if (!check_connected ("can't get channel status", connection))
				continue;

			vortex_client_get_channel_status (connection);
			continue;
		}

		if (axl_memcmp ("write start msg", line, 15)) {
			if (!check_connected ("can't send start msg", connection))
				continue;

			vortex_client_write_start_msg (connection);
			continue;
		}

		if (axl_memcmp ("new channel", line, 11)) {
			if (!check_connected ("can't create new channel", connection))
                                continue;

			vortex_client_new_channel (connection);
			continue;
		}

		if (axl_memcmp ("write close msg", line, 15)) {
			if (!check_connected ("can't send close msg", connection))
				continue;

			vortex_client_write_close_msg (connection);
			continue;
		}

		if (axl_memcmp ("send message", line, 12)) {
			if (!check_connected ("can't send a message", connection))
				continue;
			
			vortex_client_send_message (connection);
			continue;
		}

		if (axl_memcmp ("write fragment", line, 14)) {
			if (!check_connected ("can't send a message", connection))
				continue;
			
			vortex_client_write_frame (connection, axl_true);
			continue;
		}
#if defined(ENABLE_XML_RPC_SUPPORT)
		if (axl_memcmp ("new invoke", line, 10)) {
			if (!check_connected ("can't perform a XML-RPC invocation if not connected first", connection))
				continue;

			vortex_client_begin_xml_rpc_invocation (connection);
			continue;
		}
#endif

		/* show help if a command was not recognized */
		if (line != NULL && strlen (line) >= 1) {
			printf ("Command '%s' not recognized..\n", line);
			vortex_client_show_help ();
		}
	}

	/* finaly free connection */
	if (vortex_connection_is_ok (connection, axl_false))
		vortex_connection_close (connection);
	
	return (0);
}


#include <vortex.h>

#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	printf ("A frame received on channl: %d\n",     vortex_channel_get_number (channel));
	printf ("Data received: '%s'\n",                (char*) vortex_frame_get_payload (frame));

	/* reply the peer client with the same content */
	vortex_channel_send_rpyv (channel,
				  vortex_frame_get_msgno (frame),
				  "Received Ok: %s",
				  vortex_frame_get_payload (frame));
				
	printf ("VORTEX_LISTENER: end task (pid: %d)\n", getpid ());


	return;
}

bool     start_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing starting a new
	 * channel */

	/* to return false denies channel creation to return true
	 * allows create the channel */
	return true;
}

bool     close_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing to closeing a
	 * the channel to return FALSE denies channel closing to
	 * return true allows to close the channel */
	return true;
}

bool     on_accepted (VortexConnection * connection, axlPointer data)
{
	printf ("New connection accepted from: %s:%s\n", 
		 vortex_connection_get_host (connection),
		 vortex_connection_get_port (connection));

	/* return true to accept the connection to be created */
	return true;
}

int  main (int  argc, char ** argv) 
{

	/* init vortex library */
	vortex_init ();

	/* register a profile */
	vortex_profiles_register (PLAIN_PROFILE, 
				  start_channel, NULL, 
				  close_channel, NULL,
				  frame_received, NULL);

	/* create a vortex server */
	vortex_listener_new ("0.0.0.0", "44000", NULL, NULL);

	/* configure connection notification */
	vortex_listener_set_on_connection_accepted (on_accepted, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait ();
	
	/* end vortex function */
	vortex_exit ();

	return 0;
}


#include <test_xml_rpc.h>

int  main (int  argc, char ** argv)
{
	VortexConnection * connection;
	VortexChannel    * channel;

	/* initialize the vortex */
	vortex_init ();

	/* create a connection to a local server */
	connection = vortex_connection_new ("localhost", "44000", NULL, NULL);

	/* create the xml-rpc channel */
	channel = BOOT_CHANNEL (connection, NULL);

	/* perform the invocation */
	if (7 != test_sum_int_int_s (3, 4, channel, NULL, NULL, NULL)) {
		fprintf (stderr, "An error was found while invoking..\n");
		return -1;
	}

	/* close the channel */
	vortex_channel_close (channel, NULL);	

	/* close the connection */
	vortex_connection_close (connection);

	/* terminate vortex */
	vortex_exit ();	

	return 0;
}

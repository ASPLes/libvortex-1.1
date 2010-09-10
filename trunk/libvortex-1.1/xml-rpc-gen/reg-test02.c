/* Hey emacs, show me this like 'c': -*- c -*-
 *
 * Test files for the xml-rpc-gen tool.
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

#include <test_xml_rpc.h>
#include <_support.h>

int  main (int  argc, char ** argv)
{

	VortexChannel * channel;
	char          * result;
	int             iterator;

	/* starts the vortex engine and create the XML-RPC channel,
	 * checking the result */
	channel = start_and_create_channel ();
	v_return_val_if_fail (channel, -1);

	/* get the string from the function */
	result = test_get_the_string_s (channel, NULL, NULL, NULL);
	if (result == NULL) {
		fprintf (stderr, "An error was found while retreiving the result, a NULL value was received when an string was expected\n");
		return -1;
	}
	
	if (! axl_cmp (result, "This is a test")) {
		fprintf (stderr, "An different string value was received than expected '%s' != '%s'..",
			 "This is a test", result);
		return -1;
	}

	/* free the result */
	axl_free (result);

	/* stop the test */
	close_channel_and_stop (channel);

	printf ("02: test ok\n");

	return 0;
}

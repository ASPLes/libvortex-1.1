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

int  main (int  argc, char  ** argv)
{

	VortexChannel * channel;
	Node          * node;
	Node          * result;
	int             iterator;

	/* starts the vortex engine and create the XML-RPC channel,
	 * checking the result */
	channel = start_and_create_channel ();
	v_return_val_if_fail (channel, -1);

	/* get the array content */
	result = test_get_list_s (channel, NULL, NULL, NULL);
	if (result == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return -1;
	}

	/* for each node */
	iterator = 0;
	node     = result;
	for (; iterator < 8; iterator++) {
		if (node == NULL) {
			fprintf (stderr, "Unexpected NULL received\n");
			return -1;
		}

		printf ("Node i=%d, with content position=%d found\n", 
		   iterator, node->position); 
		
		/* check content */
		if (node->position != (iterator + 1)) {
			fprintf (stderr, "Unexpected position received\n");
			return -1;
		}
		
		/* get the next reference */
		node = node->next;
	}

	/* free result */
	test_node_free (result);

	/* stop the test */
	close_channel_and_stop (channel);

	printf ("07: test ok\n");

	return 0;
}

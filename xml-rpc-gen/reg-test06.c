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
	ItemArray     * array;
	Item          * item;
	int             iterator;

	/* starts the vortex engine and create the XML-RPC channel,
	 * checking the result */
	channel = start_and_create_channel ();
	v_return_val_if_fail (channel, -1);

	/* get the array content */
	array = test_get_array_s (channel, NULL, NULL, NULL);
	
	if (array == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return -1;
	}

	/* for each item */
	iterator = 0;
	for (; iterator < 10; iterator++) {
		/* get a reference to the item */
		item = test_itemarray_get (array, iterator);

		/* check null reference */
		if (item == NULL) {
			fprintf (stderr, "Expected to find a struct reference inside..\n");
			return -1;
		}

		/* check the int value inside the struct */
		if (item->position != iterator) {
			fprintf (stderr, "Expected to find an integer value: (%d) != (%d)\n",
				    item->position, iterator);
			return -1;
		}

		/* check the string inside */
		if (! axl_cmp (item->string_position, "test content")) {
			fprintf (stderr, "Expected to find a string value: (%s) != (%s)\n",
				 item->string_position, "test content");
			return -1;
		}
	}
	

	/* free result */
	test_itemarray_free (array);

	/* stop the test */
	close_channel_and_stop (channel);

	printf ("06: test ok\n");

	return 0;
}

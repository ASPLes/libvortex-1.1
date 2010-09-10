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
	Values        * a;
	Values        * b;
	Values        * result;

	/* starts the vortex engine and create the XML-RPC channel,
	 * checking the result */
	channel = start_and_create_channel ();
	v_return_val_if_fail (channel, -1);

	/* get the string from the function */
	a      = test_values_new (3, 10.2, FALSE);
	b      = test_values_new (7, 7.12, TRUE);
	
	/* perform invocation */
	result = test_get_struct_values_values_s (a, b, channel, NULL, NULL, NULL);

	if (result == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n", result);
		return -1;
	}

	if (result->count != 10) {
		fprintf (stderr, "Expected to receive a value not found (count=10 != count=%d\n",
			    result->count);
		return -1;
	}

	if (result->fraction != 17.32) {
		fprintf (stderr, "Expected to receive a value not found (fraction=17.32 != fraction=%g\n",
			    result->fraction);
		return -1;
	}

	if (! result->status) {
		fprintf (stderr, "Expected to receive a true value for status\n");
		return -1;
	}

	/* release memory allocated */
	test_values_free (a);
	test_values_free (b);
	test_values_free (result);

	/* stop the test */
	close_channel_and_stop (channel);

	printf ("05: test ok\n");

	return 0;
}

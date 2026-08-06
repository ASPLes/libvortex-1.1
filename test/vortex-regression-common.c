/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2026 Advanced Software Production Line, S.L.
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
#include <vortex-regression-common.h>

/** 
 * @brief Implements a subsecond wait
 */
void vortex_regression_common_wait (long microseconds)
{
	VortexAsyncQueue * temp;

	/* create the queue */
	temp = vortex_async_queue_new ();

	/* finish queue */
	vortex_async_queue_timedpop (temp, microseconds);
	vortex_async_queue_unref (temp);

	return;
	
}

/** 
 * @brief Reads the content of the file identified the string
 * provided, filling the size in the integer reference received.
 * 
 * @param file The file that is going to be read.
 *
 * @param size The size of the file to be returned.
 * 
 * @return A reference to the content of the file allocated. The
 * caller must unreference by returning axl_free.
 */
char * vortex_regression_common_read_file (const char * file, int * size)
{
	char * result = NULL;
	FILE * handle;
	struct stat status;
	int    requested;

	/* check parameter received */
	if (file == NULL)
		return NULL;

	/* open the file */
#if defined(AXL_OS_WIN32)
	handle = fopen (file, "rb");
#else
	handle = fopen (file, "r");
#endif
	if (handle == NULL)
		return NULL;

	/* get the file size */
	memset (&status, 0, sizeof (struct stat));
	if (stat (file, &status) != 0) {
		/* failed to get file size */
		fprintf (stderr, "Failed to get file size for %s..\n", file);
		fclose (handle);
		return NULL;
	} /* end if */

	result    = axl_new (char, status.st_size + 1);
	requested = fread (result, 1, status.st_size, handle);

	/* disabled because windows could return a different size
	 * reported that the actual size !!!!! */
	if (status.st_size != requested) {
		/* failed to read content */
		fprintf (stdout, "Unable to properly read the file, size expected to read %d (but found %d), wasn't fulfilled\n",
			 (int) status.st_size, requested);
		axl_free (result);
		fclose (handle);
		return NULL;
	} /* end if */

	/* close the file and return the content */
	fclose (handle);

	/* fill the optional size */
	if (size)
		*size = status.st_size;

	return result;
}

/* number of base ports known to the suite */
#define REGRESSION_PORT_COUNT 9

/* offset added to every base port, configured with --offset-port */
static int      regression_offset       = 0;

/* whether the rendered values below are up to date */
static axl_bool regression_ports_ready  = axl_false;

static const int regression_port_bases [REGRESSION_PORT_COUNT] = {
	REGRESSION_PORT_LISTENER,
	REGRESSION_PORT_UNIFIED_SASL,
	REGRESSION_PORT_FAKE_LISTENER,
	REGRESSION_PORT_WEBSOCKET,
	REGRESSION_PORT_WEBSOCKET_TLS,
	REGRESSION_PORT_SHARING,
	REGRESSION_PORT_IPV6,
	REGRESSION_PORT_TUNNEL_PROXY,
	REGRESSION_PORT_TLS
};

/* rendered port values; five digits plus terminator fit comfortably */
static char regression_port_values [REGRESSION_PORT_COUNT][8];

/** 
 * @brief Renders every base port with the offset currently configured.
 *
 * Called from main before any thread starts, so the values it fills are
 * read-only from then on and need no locking.
 */
void regression_port_init (void)
{
	int iterator = 0;

	while (iterator < REGRESSION_PORT_COUNT) {
		sprintf (regression_port_values[iterator], "%d",
			 regression_port_bases[iterator] + regression_offset);
		iterator++;
	} /* end while */

	regression_ports_ready = axl_true;
	return;
}

/** 
 * @brief Configures the offset added to every port the suite binds.
 *
 * @param value The offset as given on the command line, digits only.
 *
 * @return axl_true if the value was accepted, axl_false if it is not a number
 * or falls outside \ref REGRESSION_PORT_OFFSET_MAX.
 */
axl_bool regression_port_offset_configure (const char * value)
{
	int iterator = 0;
	int offset;

	if (value == NULL || value[0] == 0)
		return axl_false;

	while (value[iterator] != 0) {
		if (value[iterator] < '0' || value[iterator] > '9')
			return axl_false;
		iterator++;
	} /* end while */

	offset = atoi (value);
	if (offset < 0 || offset > REGRESSION_PORT_OFFSET_MAX)
		return axl_false;

	regression_offset = offset;
	regression_port_init ();
	return axl_true;
}

/** 
 * @brief Returns the offset currently configured.
 */
int regression_port_offset (void)
{
	return regression_offset;
}

/** 
 * @brief Returns the port to use for the given base, as a string.
 *
 * @param base One of the REGRESSION_PORT_ constants.
 *
 * @return The rendered port, or NULL if the base is not one the suite knows.
 * The returned string must not be freed.
 */
const char * regression_port (int base)
{
	int iterator = 0;

	if (! regression_ports_ready)
		regression_port_init ();

	while (iterator < REGRESSION_PORT_COUNT) {
		if (regression_port_bases[iterator] == base)
			return regression_port_values[iterator];
		iterator++;
	} /* end while */

	return NULL;
}

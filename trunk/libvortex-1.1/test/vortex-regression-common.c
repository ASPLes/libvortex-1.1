/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
	char * result;
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
#if ! defined(AXL_OS_WIN32)
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
#endif
	
	/* close the file and return the content */
	fclose (handle);

	/* fill the optional size */
	if (size)
		*size = status.st_size;

	return result;
}

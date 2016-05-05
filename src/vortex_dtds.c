/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
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

#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

/* include inline dtds */
#include <vortex-channel.dtd.h>

#define LOG_DOMAIN "vortex-dtds"

/** 
 * @internal
 * @brief Loads a DTD file an update xmlDtdPtr value.
 * 
 * @param dtd_pointer  The DTD pointer to update
 * @param file_to_load The file to load
 * 
 * @return axl_true if the DTD file was read, parsed and pointer updated. axl_false if not.
 */
axl_bool      vortex_dtds_load_dtd (VortexCtx * ctx, axlDtd ** dtd_pointer, char * dtd_to_load)
{
	axlError  * error;

	/* load channel DTD */
	(* dtd_pointer) = axl_dtd_parse (dtd_to_load, -1, &error);

	if ((* dtd_pointer) == NULL) {
		/* drop a log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to parse dtd, this can be a problem: %s",
		       axl_error_get (error));
		
		/* release error reported */
		axl_error_free (error);
		return axl_false;
	}
	return axl_true;
}


/** 
 * @internal 
 * @brief Vortex Library internal function to load DTD files
 * 
 * 
 * @return axl_true if all DTD files where loaded.
 */
axl_bool      vortex_dtds_init (VortexCtx * ctx) 
{
	v_return_val_if_fail (ctx, axl_false);

	/* do not load if it was loaded */
	if (ctx->channel_dtd)
		return axl_true;

	/* load BEEP channel management DTD definition */
        if (!vortex_dtds_load_dtd (ctx, &ctx->channel_dtd, CHANNEL_DTD)) {
                fprintf (stderr, "VORTEX_ERROR: unable to load channel.dtd file.\n");
		return axl_false;
        }
	
	return axl_true;
}

/** 
 * @brief Terminates the vortex dtd module state on the provided
 * vortex context.
 * 
 * @param ctx The context to cleanup.
 */
void vortex_dtds_cleanup (VortexCtx * ctx)
{
	v_return_if_fail (ctx);

	axl_dtd_free (ctx->channel_dtd);
	ctx->channel_dtd = NULL;

	return;
}

/** 
 * @internal
 * @brief Returns current BEEP Channel management DTD definition.
 * 
 * @return Current pointer to the DTD definition. This couldn't be
 * NULL, otherwise this Vortex Library instance will not be running
 * (already checked at the Vortex Library startup)
 */
axlDtd * vortex_dtds_get_channel_dtd (VortexCtx * ctx)
{
	if (ctx == NULL)
		return NULL;

	return ctx->channel_dtd;
}



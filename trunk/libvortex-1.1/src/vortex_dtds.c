/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */
#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-dtds"

/** 
 * @internal
 * @brief Loads a DTD file an update xmlDtdPtr value.
 * 
 * @param dtd_pointer  The DTD pointer to update
 * @param file_to_load The file to load
 * 
 * @return true if the DTD file was read, parsed and pointer updated. false if not.
 */
bool     __vortex_dtds_load_dtd (axlDtd ** dtd_pointer, char * file_to_load)
{
	char      * dtd_file_name;
	axlError  * error;

	/* load channel DTD */
	dtd_file_name = vortex_support_find_data_file (file_to_load);
	if (dtd_file_name == NULL)
		return false;

	vortex_log (LOG_DOMAIN, VORTEX_LEVEL_DEBUG, "dtd file definition found at: %s", dtd_file_name);
	(* dtd_pointer) = axl_dtd_parse_from_file (dtd_file_name, &error);
	axl_free (dtd_file_name);

	if ((* dtd_pointer) == NULL) {
		/* drop a log */
		vortex_log (LOG_DOMAIN, VORTEX_LEVEL_CRITICAL, "unable to parse dtd, this can be a problem: %s",
		       axl_error_get (error));
		
		/* release error reported */
		axl_error_free (error);
		return false;
	}
	return true;
}


/** 
 * @internal 
 * @brief Vortex Library internal function to load DTD files
 * 
 * 
 * @return true if all DTD files where loaded.
 */
bool     vortex_dtds_init (VortexCtx * ctx) 
{
	v_return_val_if_fail (ctx, false);

	/* load BEEP channel management DTD definition */
        if (!__vortex_dtds_load_dtd (&ctx->channel_dtd, "channel.dtd")) {
                fprintf (stderr, "VORTEX_ERROR: unable to load channel.dtd file.\n");
		return false;
        }
	
	/* load SASL DTD definition */
	if (!__vortex_dtds_load_dtd (&ctx->sasl_dtd, "sasl.dtd")) {
                fprintf (stderr, "VORTEX_ERROR: unable to load sasl.dtd file.\n");
		return false;
        }

	/* load SASL DTD definition */
	if (!__vortex_dtds_load_dtd (&ctx->xml_rpc_boot_dtd, "xml-rpc-boot.dtd")) {
                fprintf (stderr, "VORTEX_ERROR: unable to load xml-rpc-boot.dtd file.\n");
		return false;
        }

	return true;
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

	axl_dtd_free (ctx->sasl_dtd);
	ctx->sasl_dtd = NULL;

	axl_dtd_free (ctx->xml_rpc_boot_dtd);
	ctx->xml_rpc_boot_dtd = NULL;

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
axlDtd * vortex_dtds_get_channel_dtd ()
{
	/* get current context */
	VortexCtx * ctx = vortex_ctx_get ();

	return ctx->channel_dtd;
}

/** 
 * @internal
 * @brief Returns current SASL DTD definition.
 * 
 * @return Current pointer to the DTD definition for SASL profile.
 */
axlDtd * vortex_dtds_get_sasl_dtd ()
{
	/* get current context */
	VortexCtx * ctx = vortex_ctx_get ();

	return ctx->sasl_dtd;
}

/** 
 * @internal
 * @brief Returns current XML-RPC DTD definition.
 * 
 * @return Current pointer to the DTD definition for XML-RPC profile.
 */
axlDtd * vortex_dtds_get_xml_rpc_boot_dtd ()
{
	/* get current context */
	VortexCtx * ctx = vortex_ctx_get ();

	return ctx->xml_rpc_boot_dtd;
}


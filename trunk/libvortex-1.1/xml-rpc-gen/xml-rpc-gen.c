/* Hey emacs, show me this like 'c': -*- c -*-
 *
 * xml-rpc-gen: a protocol compiler for the XDL definition language
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

/* global include */
#include <xml-rpc.h>

/* local dtd include */
#include <xml-rpc.dtd.h>

#define HELP_HEADER "xml-rpc-gen-1.1: a protocol compiler for the XDL language\n\
Copyright (C) 2010  Advanced Software Production Line, S.L.\n\n"


#define POST_HEADER "\nIf you have question, bugs to report, patches, you can reach us\n\
at <vortex@lists.aspl.es>."

/** 
 * @internal DTD declaration.
 */
axlDtd * dtd = NULL;

/*
 * @internal Vortex library context.
 */
VortexCtx * ctx = NULL;


void show_tutorial ()
{
	printf (HELP_HEADER);
	printf ("To use xml-rpc-gen-1.1:\n\n");
	printf ("1) Define an XDL file (or IDL file), describing your \n");
	printf ("   XML-RPC interface as defined in the documentation. \n");
	printf ("2) Comple the example using the following:\n");
	printf ("   xml-rpc-gen test.xdl\n\n");

	printf ("Try also xml-rpc-gen-1.1 --help\n");
}


void xml_rpc_gen_finish ()
{
	/* terminate exarg library */
	exarg_end ();

	/* release the dtd */
	axl_dtd_free (dtd);

	/* terminate axl library */
	axl_end ();

	/* terminate context */
	vortex_support_cleanup (ctx);
	vortex_ctx_free (ctx);
	
	return;
}

/** 
 * @internal
 * Shows current version.
 */
void show_version ()
{
         printf ("%s", (VERSION != NULL && strlen (VERSION) > 0) ? VERSION : "no-version");
}

/** 
 * @internal
 * @brief Compiles the provided file.
 * 
 */
axl_bool      xml_rpc_gen_compile_selected (const char  * selected)
{
	axlError * error;
	axlDoc   * doc = NULL;

	axlNode  * service;

	char    * out_dir;
	char    * comp_name;
	char    * aux_file;
	int       length;

	/* load the xml document */
	if (! exarg_is_defined ("idl-format"))
		doc = axl_doc_parse_from_file (selected, &error);

	/* check result */
	if (doc == NULL) {
		/* check if the input format was expected to be xml
		   (XDL) */
		if (exarg_is_defined ("xdl-format")) {
			fprintf (stderr, "error: unable to parse file '%s', expected XDL format. Error was:\n%s",
				    selected, axl_error_get (error));
			axl_error_free (error);
			/* stop from parsing */
			return axl_false;
		}
	    
		/* try to load the document as an IDL definition using
		 * the structured format */
		doc = xml_rpc_gen_translate_idl_to_xdl (selected, &error);

		if (exarg_is_defined ("to-xml")) {
			/* get file */
			length = strlen (selected);
			if ((selected[length - 1] == 'l') && 
			    (selected[length - 2] == 'd') && 
			    (selected[length - 3] == 'i')) {
				/* create an aux file replacing ending
				 * idl by xml */
				aux_file = axl_strdup (selected);

				/* translate */
				aux_file [length - 2] = 'm';
				aux_file [length - 3] = 'x';
			}else {
				/* create an aux file replacing
				 * appending the .xml value */
				aux_file = axl_strdup_printf ("%s.xml", selected);
			}

			xml_rpc_report ("translating %s into %s", selected, aux_file);
			if (! axl_doc_dump_pretty_to_file (doc, aux_file, 4)) {
				fprintf (stderr, "error: unable to dump document, error was: %s\n", 
					 axl_error_get (error));
				axl_error_free (error);
			}else {
				xml_rpc_report ("translation ok");
			}

			/* free the aux file */
			axl_free (aux_file);

			/* free the document */
			axl_doc_free (doc);
			
			return axl_true;
		}

		/* seems that the document provided is either not XDL
		 * and IDL. */
		if (doc == NULL) {
			/* report the error */
			fprintf (stderr, "error: unable to parse file '%s', error was:\n%s",
				    selected, axl_error_get (error));
			axl_error_free (error);
			
			/* return that the document wasn't compiled */
			return axl_false;
		}
	}else {
		xml_rpc_report ("detected XDL format definition..");
	}

	xml_rpc_report ("document is well-formed: %s..", selected);

	/* validate file received */
	if (! axl_dtd_validate (doc, dtd, &error)) {
		/* report the error */
		fprintf (stderr, "error: validation failed for '%s', error was:\n%s",
			    selected, axl_error_get (error));
		axl_error_free (error);

		/* return that the document wasn't compiled */
		return axl_false;
	}

	xml_rpc_report ("document is valid: %s..", selected);

	/* get out dir */
	if (exarg_is_defined ("out-dir")) 
		out_dir = exarg_get_string ("out-dir");
	else
		out_dir = "out";

	/* get the name node */
	comp_name = (char*) axl_doc_get_content_at (doc, "/xml-rpc-interface/name", NULL);
	

	/* trim the name */
	axl_stream_trim (comp_name);
	
	xml_rpc_report ("component name: '%s'..", comp_name);

	/* get a service reference */
	service = axl_doc_get (doc, "/xml-rpc-interface/service");

	/* now produce the stub required */
	if (! exarg_is_defined ("only-server")) {
		xml_rpc_c_stub_create (doc, out_dir, comp_name);

		/* write autoconf files for the server component */
		if (! exarg_is_defined ("disable-autoconf"))
			xml_rpc_autoconf_c_stub_create (doc, out_dir, comp_name);
	} else 
		xml_rpc_report ("client stub have been disabled..");
	
	/* now produce the server required */
	if (! exarg_is_defined ("only-client")) {
		xml_rpc_c_server_create (doc, out_dir, comp_name);
		
		/* write autoconf files for the client component */
		if (! exarg_is_defined ("disable-autoconf")) 
			xml_rpc_autoconf_c_server_create (doc, out_dir, comp_name);
	} else
		xml_rpc_report ("server stub have been disabled..");

	/* document parsed */
	axl_doc_free (doc);
	
	xml_rpc_report ("compilation ok");
	
	return axl_true;
}

/** 
 * @internal
 *
 * Compiles the provided document.
 */
void xml_rpc_gen_compile ()
{
	ExArgument * arg;
	const char * file;

	/* get file list */
	arg    = exarg_get_params ();

	/* iterate over all elements */
	while (arg != NULL) {

		/* get the file argument */
		file = exarg_param_get (arg);

		/* check that the file exist and it is regular */
		if (! vortex_support_file_test (file, FILE_IS_REGULAR)) {
			fprintf (stderr, "error: file '%s' doesn't exists or it is not a regular file.\n",
				    file);
			return;
		}

		/* drop a log */
		xml_rpc_report ("compiling: %s..", file);

		/* compile the selected file */
		if (!xml_rpc_gen_compile_selected (file))
			return;

		/* get next param */
		arg = exarg_param_next (arg);
	}
	
	/* nothing more to do */
	return;
}

int main (int argc, char **argv)
{
	axlError   * error;
	char      ** paths;
	int          iterator;

	/* init exarg library */
	exarg_install_arg ("version", "v", EXARG_NONE,
			   "Shows version info");
	exarg_install_arg ("out-dir", "o", EXARG_STRING, 
			   "Configures the output directory to place stub and skeletons generated. Default value, if not provided, is \"out\", starting from the current directory.");
	exarg_install_arg ("disable-autoconf", "a", EXARG_NONE,
			   "Disables autoconf files generation. By default, activated.");
	exarg_install_arg ("only-client", "c", EXARG_NONE,
			   "Only produces the client connector, the stub library used to interface with the XML-RPC component. By default, the client stub is generated.");
	exarg_install_arg ("only-server", "s", EXARG_NONE,
			   "Only produces the server stub, the XML-RPC component that is receiving the invocation. By default, the server stub is generated.");
	exarg_install_arg ("xdl-format", "x", EXARG_NONE,
			   "Instruct the tool to expect and process only XDL formated files. This option is useful because the tool fails into parse IDL format if XDL parsing fails, getting not proper errors if a XDL format was defined. This option is not compatible with --idl-format.");
	exarg_install_arg ("idl-format", "i", EXARG_NONE,
			   "Instruct the tool to expect and process only IDL formated files");
	exarg_install_arg ("out-server-dir", NULL, EXARG_STRING, "Allows to fully configure server output source code. This is not the same as --out-dir which is a directory where is placed both products.");
	exarg_install_arg ("out-stub-dir", NULL, EXARG_STRING, "Allows to fully configure stub output source code. This is not the same as --out-dir which is a directory where is placed both products.");
	exarg_install_arg ("disable-main-file", NULL, EXARG_NONE, "Server side component option. It allows to disable creating a main.c file, allowing to produce a custom one. This option is useful while trying to create XML-RPC components mixed with other profiles");
	exarg_install_arg ("to-xml", NULL, EXARG_NONE, "Makes the IDL input file to be translated into XML");

	exarg_install_arg ("add-search-path", NULL, EXARG_STRING, "Allows to configure a list of directories (separated by ;) that are added to the search path. This allows to locate DTD files required by the tool.");

	
	/* configure headers to be showed */
	exarg_add_usage_header  (HELP_HEADER);
	exarg_add_help_header   (HELP_HEADER);
	exarg_post_help_header  (POST_HEADER);
	exarg_post_usage_header (POST_HEADER);	

	/* init and parse arguments */
	exarg_parse (argc, argv);

	if (exarg_is_defined ("xdl-format") && 
	    exarg_is_defined ("idl-format")) {
		fprintf (stderr, "error: --xdl-format and --idl-format options are incompatible.");

		/* do nothing */
		return -1;
	}

	/* init vortex ctx */
	ctx = vortex_ctx_new ();
	vortex_support_init (ctx);

	/* do not use the add_search_path_ref version to force
	   xml-rpc-gen library to perform a local copy path */
	xml_rpc_support_add_search_path (".");

	/* add additional search path */
	if (exarg_is_defined ("add-search-path")) {
		paths    = axl_stream_split (exarg_get_string ("add-search-path"), 1, ";");
		iterator = 0;
		while (paths[iterator]) {
			/* log and add path */
			xml_rpc_report ("adding search path: %s", paths[iterator]);
			xml_rpc_support_add_search_path (paths[iterator]);
			
			/* update iterator */
			iterator++;
		} /* end while */

		/* free vstring */
		axl_freev (paths);
	} /* end if */

	/* install search paths */
	xml_rpc_support_add_search_path_ref (vortex_support_build_filename (INSTALL_DIR, NULL));

	/* init the axl library and load the DTD */
	axl_init ();
	
	/* load the DTD XML-RPC definition */
	dtd = axl_dtd_parse (XML_RPC_DTD, -1, &error);
	if (dtd == NULL) {
		/* show error found */
		fprintf (stderr, "error: unable to load dtd file '%s', check your installation, \n%s\n",
			 "xml-rpc.dtd", axl_error_get (error));
		axl_error_free (error);

		/* terminate xml-rpc-gen tool */
		xml_rpc_gen_finish ();
		return -1;
	}
	
	/* check and show program version */
	if (exarg_is_defined ("version")) {
		/* show current version */
		show_version ();

		/* terminate xml-rpc-gen tool */
		xml_rpc_gen_finish ();
		return 0;
	}

	/* check the number of parameters */
	if (exarg_get_params_num () > 0) {
		/* a xdl file was received */
		xml_rpc_gen_compile ();

		/* terminate the xml-rpc-gen tool */
		xml_rpc_gen_finish ();
		return 0;
	}

	/* show mini tutorial */
	show_tutorial ();

	/* terminate xml-rpc-gen tool */
	xml_rpc_gen_finish ();
	return 0;
}

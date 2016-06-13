#include <vortex.h>

#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <vortex_tls.h>

void show_help_message (int argc, char ** argv) {
	printf ("%s : tool to get digests that matches vortex_tls_get_peer_ssl_digest () function\n", argv[0]);
	printf ("Usage: \n");
	printf ("  -h|--help : this help \n");
	printf ("  -v|--verbose : enable verbose operation \n");
	printf ("  --md5 : (default) generate digest using MD5 \n");
	printf ("  --sha1 : generate digest using SHA1 \n");
	exit (0);
}

int main (int argc, char ** argv) {
	char           * result;

	VortexCtx      * ctx     = vortex_ctx_new ();
	axl_bool         verbose = axl_false;
	/* axl_bool         md5     = axl_true; */
	axl_bool         sha1    = axl_false;
	
	int              iterator;
	char           * arg_value;
	const char     * file;

	/* init vortex module */
	if (! vortex_init_ctx (ctx)) {
		printf ("ERROR: failed to init Vortex engine, vortex_init_ctx (ctx) failed\n");
		exit (-1);
	} /* end if */

	iterator = 0;
	file     = NULL;
	while (iterator < argc) {
		/* get arg value */
		arg_value = argv[iterator];
		if (arg_value) {
			if (axl_cmp (arg_value, "-v") || axl_cmp (arg_value, "--verbose")) {
				verbose = axl_true;
				
				/* next operation */
				iterator++;
				continue;
			}
			if (axl_cmp (arg_value, "-h") || axl_cmp (arg_value, "--help")) {
				show_help_message (argc, argv);
				
				/* next operation */
				iterator++;
				continue;
			}
			if (axl_cmp (arg_value, "-md5")) {
				/* next operation */
				iterator++;
				continue;
			}

			if (axl_cmp (arg_value, "-sha1")) {
				sha1 = axl_true;
				
				/* next operation */
				iterator++;
				continue;
			}

			
		} /* end if */

		/* get file */
		file = argv[iterator];
		
		/* next operation */
		iterator++;
	}
	
	if (! vortex_tls_init (ctx) ) {
		if (verbose)
			printf ("ERROR: vortex_tls_init (ctx) failed, unable to continue\n");
		exit (-1);
	}

	/* printf ("argc = %d\n", argc);*/
	if (argc < 2) {
		if (verbose)
			printf ("ERROR: Please, provide path to a certificate to get digest.\n");
		exit (-1);
	} /* end if */

	if (verbose)
		printf ("INFO: using certificate %s as source\n", file);


	/* call base implementation */
	result = vortex_tls_get_ssl_digest (file, sha1 ? VORTEX_SHA1 : VORTEX_MD5);

	/* finish vortex */
	vortex_exit_ctx (ctx, axl_true);

	if (result) {
		printf ("%s\n", result);
		axl_free (result);
		return 0;
	} else {
		printf ("ERROR: failed to get certificate from from SSL object..\n");
		exit (-1);
	}

	if (verbose)
		printf ("ERROR, failed to return digest\n");
	return -1;

}

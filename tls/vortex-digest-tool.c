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
	const EVP_MD   * digest_method = NULL;
	SSL_CTX        * sslctx;
	SSL            * ssl;
	X509           * crt;
	
	/* variable declaration */
	unsigned int     message_size;
	unsigned char    message [EVP_MAX_MD_SIZE];
	
	char           * result;

	VortexCtx      * ctx  = vortex_ctx_new ();
	axl_bool         verbose = axl_false;
	/* axl_bool         md5     = axl_true; */
	axl_bool         sha1    = axl_false;
	
	int              iterator;
	char           * arg_value;
	const char     * file;


	
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

	sslctx = SSL_CTX_new (TLSv1_server_method ());
	SSL_CTX_use_certificate_file (sslctx, file, SSL_FILETYPE_PEM);
	ssl    = SSL_new (sslctx);
	crt    = SSL_get_certificate(ssl);	
	
	digest_method = EVP_md5 ();

	if (crt == NULL) {
		if (verbose)
			printf ("ERROR: failed to get certificate from from SSL object..\n");
		exit (-1);
	} /* end if */
	
	/* get the message digest and check */
	if (! X509_digest (crt, digest_method, message, &message_size)) {
		if (verbose)
			printf ("ERROR: failed to get digest out of certificate, X509_digest () failed..\n");
		return -1;
	} /* end if */
	
	/* call base implementation */
	result = vortex_tls_get_digest_sized (sha1 ? VORTEX_SHA1 : VORTEX_MD5, (const char *) message, message_size);
	if (result) {
		printf ("%s\n", result);
		axl_free (result);
		return 0;
	}

	if (verbose)
		printf ("ERROR, failed to return digest\n");
	return -1;

}
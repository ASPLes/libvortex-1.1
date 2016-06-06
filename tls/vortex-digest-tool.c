#include <openssl/x509v3.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include <vortex_tls.h>

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

	
	if (! vortex_tls_init (ctx) ) {
		printf ("ERROR: vortex_tls_init (ctx) failed, unable to continue\n");
		exit (-1);
	}

	/* printf ("argc = %d\n", argc);*/
	if (argc < 2) {
		printf ("ERROR: Please, provide path to a certificate to get digest.\n");
		exit (-1);
	} /* end if */

	printf ("INFO: using certificate %s as source\n", argv[1]);

	sslctx = SSL_CTX_new (TLSv1_server_method ());
	SSL_CTX_use_certificate_file (sslctx, argv[1], SSL_FILETYPE_PEM);
	ssl    = SSL_new (sslctx);
	crt    = SSL_get_certificate(ssl);	
	
	digest_method = EVP_md5 ();

	if (crt == NULL) {
		printf ("ERROR: failed to get certificate from from SSL object..\n");
		exit (-1);
	} /* end if */
	
	/* get the message digest and check */
	if (! X509_digest (crt, digest_method, message, &message_size)) {
		printf ("ERROR: failed to get digest out of certificate, X509_digest () failed..\n");
		return -1;
	} /* end if */
	
	/* call base implementation */
	result = vortex_tls_get_digest_sized (VORTEX_MD5, (const char *) message, message_size);
	if (result) {
		printf ("%s\n", result);
		axl_free (result);
		return 0;
	}

	printf ("ERROR, failed to return digest\n");
	return -1;

}

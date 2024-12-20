/* $OpenBSD: rsa.c,v 1.16 2022/01/14 09:26:41 tb Exp $ */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 *
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 *
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 *
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <openssl/opensslconf.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "apps.h"
#include "progs.h"

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

static struct {
	int check;
	const EVP_CIPHER *enc;
	char *infile;
	int informat;
	int modulus;
	int noout;
	char *outfile;
	int outformat;
	char *passargin;
	char *passargout;
	int pubin;
	int pubout;
	int pvk_encr;
	int text;
} rsa_config;

static int
rsa_opt_cipher(int argc, char **argv, int *argsused)
{
	char *name = argv[0];

	if (*name++ != '-')
		return (1);

	if ((rsa_config.enc = EVP_get_cipherbyname(name)) == NULL) {
		fprintf(stderr, "Invalid cipher '%s'\n", name);
		return (1);
	}

	*argsused = 1;
	return (0);
}

static const struct option rsa_options[] = {
	{
		.name = "check",
		.desc = "Check consistency of RSA private key",
		.type = OPTION_FLAG,
		.opt.flag = &rsa_config.check,
	},
	{
		.name = "in",
		.argname = "file",
		.desc = "Input file (default stdin)",
		.type = OPTION_ARG,
		.opt.arg = &rsa_config.infile,
	},
	{
		.name = "inform",
		.argname = "format",
		.desc = "Input format (DER, NET or PEM (default))",
		.type = OPTION_ARG_FORMAT,
		.opt.value = &rsa_config.informat,
	},
	{
		.name = "modulus",
		.desc = "Print the RSA key modulus",
		.type = OPTION_FLAG,
		.opt.flag = &rsa_config.modulus,
	},
	{
		.name = "noout",
		.desc = "Do not print encoded version of the key",
		.type = OPTION_FLAG,
		.opt.flag = &rsa_config.noout,
	},
	{
		.name = "out",
		.argname = "file",
		.desc = "Output file (default stdout)",
		.type = OPTION_ARG,
		.opt.arg = &rsa_config.outfile,
	},
	{
		.name = "outform",
		.argname = "format",
		.desc = "Output format (DER, NET or PEM (default PEM))",
		.type = OPTION_ARG_FORMAT,
		.opt.value = &rsa_config.outformat,
	},
	{
		.name = "passin",
		.argname = "src",
		.desc = "Input file passphrase source",
		.type = OPTION_ARG,
		.opt.arg = &rsa_config.passargin,
	},
	{
		.name = "passout",
		.argname = "src",
		.desc = "Output file passphrase source",
		.type = OPTION_ARG,
		.opt.arg = &rsa_config.passargout,
	},
	{
		.name = "pubin",
		.desc = "Expect a public key (default private key)",
		.type = OPTION_VALUE,
		.value = 1,
		.opt.value = &rsa_config.pubin,
	},
	{
		.name = "pubout",
		.desc = "Output a public key (default private key)",
		.type = OPTION_VALUE,
		.value = 1,
		.opt.value = &rsa_config.pubout,
	},
	{
		.name = "pvk-none",
		.type = OPTION_VALUE,
		.value = 0,
		.opt.value = &rsa_config.pvk_encr,
	},
	{
		.name = "pvk-strong",
		.type = OPTION_VALUE,
		.value = 2,
		.opt.value = &rsa_config.pvk_encr,
	},
	{
		.name = "pvk-weak",
		.type = OPTION_VALUE,
		.value = 1,
		.opt.value = &rsa_config.pvk_encr,
	},
	{
		.name = "RSAPublicKey_in",
		.type = OPTION_VALUE,
		.value = 2,
		.opt.value = &rsa_config.pubin,
	},
	{
		.name = "RSAPublicKey_out",
		.type = OPTION_VALUE,
		.value = 2,
		.opt.value = &rsa_config.pubout,
	},
	{
		.name = "text",
		.desc = "Print in plain text in addition to encoded",
		.type = OPTION_FLAG,
		.opt.flag = &rsa_config.text,
	},
	{
		.name = NULL,
		.type = OPTION_ARGV_FUNC,
		.opt.argvfunc = rsa_opt_cipher,
	},
	{ NULL }
};

static void
rsa_usage()
{
	int n = 0;

	fprintf(stderr,
	    "usage: rsa [-ciphername] [-check] [-in file] "
	    "[-inform fmt]\n"
	    "    [-modulus] [-noout] [-out file] [-outform fmt] "
	    "[-passin src]\n"
	    "    [-passout src] [-pubin] [-pubout] [-text]\n\n");
	options_usage(rsa_options);
	fprintf(stderr, "\n");

	fprintf(stderr, "Valid ciphername values:\n\n");
	OBJ_NAME_do_all_sorted(OBJ_NAME_TYPE_CIPHER_METH, show_cipher, &n);
	fprintf(stderr, "\n");
}

int
rsa_main(int argc, char **argv)
{
	int ret = 1;
	RSA *rsa = NULL;
	int i;
	BIO *out = NULL;
	char *passin = NULL, *passout = NULL;

	if (single_execution) {
		if (pledge("stdio cpath wpath rpath tty", NULL) == -1) {
			perror("pledge");
			exit(1);
		}
	}

	memset(&rsa_config, 0, sizeof(rsa_config));
	rsa_config.pvk_encr = 2;
	rsa_config.informat = FORMAT_PEM;
	rsa_config.outformat = FORMAT_PEM;

	if (options_parse(argc, argv, rsa_options, NULL, NULL) != 0) {
		rsa_usage();
		goto end;
	}

	if (!app_passwd(bio_err, rsa_config.passargin, rsa_config.passargout,
	    &passin, &passout)) {
		BIO_printf(bio_err, "Error getting passwords\n");
		goto end;
	}
	if (rsa_config.check && rsa_config.pubin) {
		BIO_printf(bio_err, "Only private keys can be checked\n");
		goto end;
	}
	out = BIO_new(BIO_s_file());

	{
		EVP_PKEY *pkey;

		if (rsa_config.pubin) {
			int tmpformat = -1;
			if (rsa_config.pubin == 2) {
				if (rsa_config.informat == FORMAT_PEM)
					tmpformat = FORMAT_PEMRSA;
				else if (rsa_config.informat == FORMAT_ASN1)
					tmpformat = FORMAT_ASN1RSA;
			} else
				tmpformat = rsa_config.informat;

			pkey = load_pubkey(bio_err, rsa_config.infile,
			    tmpformat, 1, passin, "Public Key");
		} else
			pkey = load_key(bio_err, rsa_config.infile,
			    rsa_config.informat, 1, passin, "Private Key");

		if (pkey != NULL)
			rsa = EVP_PKEY_get1_RSA(pkey);
		EVP_PKEY_free(pkey);
	}

	if (rsa == NULL) {
		ERR_print_errors(bio_err);
		goto end;
	}
	if (rsa_config.outfile == NULL) {
		BIO_set_fp(out, stdout, BIO_NOCLOSE);
	} else {
		if (BIO_write_filename(out, rsa_config.outfile) <= 0) {
			perror(rsa_config.outfile);
			goto end;
		}
	}

	if (rsa_config.text)
		if (!RSA_print(out, rsa, 0)) {
			perror(rsa_config.outfile);
			ERR_print_errors(bio_err);
			goto end;
		}
	if (rsa_config.modulus) {
		BIO_printf(out, "Modulus=");
		BN_print(out, RSA_get0_n(rsa));
		BIO_printf(out, "\n");
	}
	if (rsa_config.check) {
		int r = RSA_check_key(rsa);

		if (r == 1)
			BIO_printf(out, "RSA key ok\n");
		else if (r == 0) {
			unsigned long err;

			while ((err = ERR_peek_error()) != 0 &&
			    ERR_GET_LIB(err) == ERR_LIB_RSA &&
			    ERR_GET_FUNC(err) == RSA_F_RSA_CHECK_KEY &&
			    ERR_GET_REASON(err) != ERR_R_MALLOC_FAILURE) {
				BIO_printf(out, "RSA key error: %s\n",
				    ERR_reason_error_string(err));
				ERR_get_error();	/* remove e from error
							 * stack */
			}
		}
		if (r == -1 || ERR_peek_error() != 0) {	/* should happen only if
							 * r == -1 */
			ERR_print_errors(bio_err);
			goto end;
		}
	}
	if (rsa_config.noout) {
		ret = 0;
		goto end;
	}
	BIO_printf(bio_err, "writing RSA key\n");
	if (rsa_config.outformat == FORMAT_ASN1) {
		if (rsa_config.pubout || rsa_config.pubin) {
			if (rsa_config.pubout == 2)
				i = i2d_RSAPublicKey_bio(out, rsa);
			else
				i = i2d_RSA_PUBKEY_bio(out, rsa);
		} else
			i = i2d_RSAPrivateKey_bio(out, rsa);
	} else if (rsa_config.outformat == FORMAT_PEM) {
		if (rsa_config.pubout || rsa_config.pubin) {
			if (rsa_config.pubout == 2)
				i = PEM_write_bio_RSAPublicKey(out, rsa);
			else
				i = PEM_write_bio_RSA_PUBKEY(out, rsa);
		} else
			i = PEM_write_bio_RSAPrivateKey(out, rsa,
			    rsa_config.enc, NULL, 0, NULL, passout);
#if !defined(OPENSSL_NO_DSA) && !defined(OPENSSL_NO_RC4)
	} else if (rsa_config.outformat == FORMAT_MSBLOB ||
	    rsa_config.outformat == FORMAT_PVK) {
		EVP_PKEY *pk;
		pk = EVP_PKEY_new();
		EVP_PKEY_set1_RSA(pk, rsa);
		if (rsa_config.outformat == FORMAT_PVK)
			i = i2b_PVK_bio(out, pk, rsa_config.pvk_encr, 0,
			    passout);
		else if (rsa_config.pubin || rsa_config.pubout)
			i = i2b_PublicKey_bio(out, pk);
		else
			i = i2b_PrivateKey_bio(out, pk);
		EVP_PKEY_free(pk);
#endif
	} else {
		BIO_printf(bio_err,
		    "bad output format specified for outfile\n");
		goto end;
	}
	if (i <= 0) {
		BIO_printf(bio_err, "unable to write key\n");
		ERR_print_errors(bio_err);
	} else
		ret = 0;

 end:
	BIO_free_all(out);
	RSA_free(rsa);
	free(passin);
	free(passout);

	return (ret);
}

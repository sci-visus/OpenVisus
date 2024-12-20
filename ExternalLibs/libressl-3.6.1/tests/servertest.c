/* $OpenBSD: servertest.c,v 1.7 2022/06/10 22:00:15 tb Exp $ */
/*
 * Copyright (c) 2015, 2016, 2017 Joel Sing <jsing@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <openssl/ssl.h>

#include <openssl/err.h>
#include <openssl/dtls1.h>
#include <openssl/ssl3.h>

#include <err.h>
#include <stdio.h>
#include <string.h>

const SSL_METHOD *tls_legacy_method(void);

char *server_ca_file;
char *server_cert_file;
char *server_key_file;

static unsigned char sslv2_client_hello_tls10[] = {
	0x80, 0x6a, 0x01, 0x03, 0x01, 0x00, 0x51, 0x00,
	0x00, 0x00, 0x10, 0x00, 0x00, 0x39, 0x00, 0x00,
	0x38, 0x00, 0x00, 0x35, 0x00, 0x00, 0x16, 0x00,
	0x00, 0x13, 0x00, 0x00, 0x0a, 0x00, 0x00, 0x33,
	0x00, 0x00, 0x32, 0x00, 0x00, 0x2f, 0x00, 0x00,
	0x07, 0x00, 0x00, 0x66, 0x00, 0x00, 0x05, 0x00,
	0x00, 0x04, 0x00, 0x00, 0x63, 0x00, 0x00, 0x62,
	0x00, 0x00, 0x61, 0x00, 0x00, 0x15, 0x00, 0x00,
	0x12, 0x00, 0x00, 0x09, 0x00, 0x00, 0x65, 0x00,
	0x00, 0x64, 0x00, 0x00, 0x60, 0x00, 0x00, 0x14,
	0x00, 0x00, 0x11, 0x00, 0x00, 0x08, 0x00, 0x00,
	0x06, 0x00, 0x00, 0x03, 0xdd, 0xb6, 0x59, 0x26,
	0x46, 0xe6, 0x79, 0x77, 0xf4, 0xec, 0x42, 0x76,
	0xc8, 0x73, 0xad, 0x9c,
};

static unsigned char sslv2_client_hello_tls12[] = {
	0x80, 0xcb, 0x01, 0x03, 0x03, 0x00, 0xa2, 0x00,
	0x00, 0x00, 0x20, 0x00, 0x00, 0xa5, 0x00, 0x00,
	0xa3, 0x00, 0x00, 0xa1, 0x00, 0x00, 0x9f, 0x00,
	0x00, 0x6b, 0x00, 0x00, 0x6a, 0x00, 0x00, 0x69,
	0x00, 0x00, 0x68, 0x00, 0x00, 0x39, 0x00, 0x00,
	0x38, 0x00, 0x00, 0x37, 0x00, 0x00, 0x36, 0x00,
	0x00, 0x88, 0x00, 0x00, 0x87, 0x00, 0x00, 0x86,
	0x00, 0x00, 0x85, 0x00, 0x00, 0x9d, 0x00, 0x00,
	0x3d, 0x00, 0x00, 0x35, 0x00, 0x00, 0x84, 0x00,
	0x00, 0xa4, 0x00, 0x00, 0xa2, 0x00, 0x00, 0xa0,
	0x00, 0x00, 0x9e, 0x00, 0x00, 0x67, 0x00, 0x00,
	0x40, 0x00, 0x00, 0x3f, 0x00, 0x00, 0x3e, 0x00,
	0x00, 0x33, 0x00, 0x00, 0x32, 0x00, 0x00, 0x31,
	0x00, 0x00, 0x30, 0x00, 0x00, 0x9a, 0x00, 0x00,
	0x99, 0x00, 0x00, 0x98, 0x00, 0x00, 0x97, 0x00,
	0x00, 0x45, 0x00, 0x00, 0x44, 0x00, 0x00, 0x43,
	0x00, 0x00, 0x42, 0x00, 0x00, 0x9c, 0x00, 0x00,
	0x3c, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x96, 0x00,
	0x00, 0x41, 0x00, 0x00, 0x07, 0x00, 0x00, 0x05,
	0x00, 0x00, 0x04, 0x00, 0x00, 0x16, 0x00, 0x00,
	0x13, 0x00, 0x00, 0x10, 0x00, 0x00, 0x0d, 0x00,
	0x00, 0x0a, 0x00, 0x00, 0xff, 0x1d, 0xfd, 0x90,
	0x03, 0x61, 0x3c, 0x5a, 0x22, 0x83, 0xed, 0x11,
	0x85, 0xf4, 0xea, 0x36, 0x59, 0xd9, 0x1b, 0x27,
	0x22, 0x01, 0x14, 0x07, 0x66, 0xb2, 0x24, 0xf5,
	0x4e, 0x7d, 0x9d, 0x9c, 0x52,
};

struct server_hello_test {
	const unsigned char *desc;
	unsigned char *client_hello;
	const size_t client_hello_len;
	const SSL_METHOD *(*ssl_method)(void);
	const long ssl_clear_options;
	const long ssl_set_options;
};

static struct server_hello_test server_hello_tests[] = {
	{
		.desc = "TLSv1.0 in SSLv2 record",
		.client_hello = sslv2_client_hello_tls10,
		.client_hello_len = sizeof(sslv2_client_hello_tls10),
		.ssl_method = tls_legacy_method,
		.ssl_clear_options = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1,
		.ssl_set_options = 0,
	},
	{
		.desc = "TLSv1.2 in SSLv2 record",
		.client_hello = sslv2_client_hello_tls12,
		.client_hello_len = sizeof(sslv2_client_hello_tls12),
		.ssl_method = tls_legacy_method,
		.ssl_clear_options = SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1,
		.ssl_set_options = 0,
	},
};

#define N_SERVER_HELLO_TESTS \
    (sizeof(server_hello_tests) / sizeof(*server_hello_tests))

static int
server_hello_test(int testno, struct server_hello_test *sht)
{
	BIO *rbio = NULL, *wbio = NULL;
	SSL_CTX *ssl_ctx = NULL;
	SSL *ssl = NULL;
	int ret = 1;

	fprintf(stderr, "Test %d - %s\n", testno, sht->desc);

	if ((rbio = BIO_new_mem_buf(sht->client_hello,
	    sht->client_hello_len)) == NULL) {
		fprintf(stderr, "Failed to setup rbio\n");
		goto failure;
	}
	if ((wbio = BIO_new(BIO_s_mem())) == NULL) {
		fprintf(stderr, "Failed to setup wbio\n");
		goto failure;
	}

	if ((ssl_ctx = SSL_CTX_new(sht->ssl_method())) == NULL) {
		fprintf(stderr, "SSL_CTX_new() returned NULL\n");
		goto failure;
	}

	if (SSL_CTX_use_certificate_file(ssl_ctx, server_cert_file,
	    SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Failed to load server certificate");
		goto failure;
	}
	if (SSL_CTX_use_PrivateKey_file(ssl_ctx, server_key_file,
	    SSL_FILETYPE_PEM) != 1) {
		fprintf(stderr, "Failed to load server private key");
		goto failure;
	}

	SSL_CTX_set_dh_auto(ssl_ctx, 1);
	SSL_CTX_set_ecdh_auto(ssl_ctx, 1);

	SSL_CTX_clear_options(ssl_ctx, sht->ssl_clear_options);
	SSL_CTX_set_options(ssl_ctx, sht->ssl_set_options);

	if ((ssl = SSL_new(ssl_ctx)) == NULL) {
		fprintf(stderr, "SSL_new() returned NULL\n");
		goto failure;
	}

	BIO_up_ref(rbio);
	BIO_up_ref(wbio);
	SSL_set_bio(ssl, rbio, wbio);

	if (SSL_accept(ssl) != 0) {
		fprintf(stderr, "SSL_accept() returned non-zero\n");
		ERR_print_errors_fp(stderr);
		goto failure;
	}

	ret = 0;

 failure:
	SSL_CTX_free(ssl_ctx);
	SSL_free(ssl);

	BIO_free(rbio);
	BIO_free(wbio);

	return (ret);
}

int
main(int argc, char **argv)
{
	int failed = 0;
	size_t i;

	if (argc != 4) {
		fprintf(stderr, "usage: %s keyfile certfile cafile\n",
		    argv[0]);
		exit(1);
	}

	server_key_file = argv[1];
	server_cert_file = argv[2];
	server_ca_file = argv[3];

	SSL_library_init();
	SSL_load_error_strings();

	for (i = 0; i < N_SERVER_HELLO_TESTS; i++)
		failed |= server_hello_test(i, &server_hello_tests[i]);

	return (failed);
}

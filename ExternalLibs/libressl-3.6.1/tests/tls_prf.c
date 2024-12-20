/* $OpenBSD: tls_prf.c,v 1.7 2022/06/10 22:00:15 tb Exp $ */
/*
 * Copyright (c) 2017 Joel Sing <jsing@openbsd.org>
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

#include <err.h>

#include "ssl_locl.h"

int tls1_PRF(SSL *s, const unsigned char *secret, size_t secret_len,
    const void *seed1, size_t seed1_len, const void *seed2, size_t seed2_len,
    const void *seed3, size_t seed3_len, const void *seed4, size_t seed4_len,
    const void *seed5, size_t seed5_len, unsigned char *out, size_t out_len);

#define TLS_PRF_OUT_LEN 128

struct tls_prf_test {
	const unsigned char *desc;
	const SSL_METHOD *(*ssl_method)(void);
	const uint16_t cipher_value;
	const unsigned char out[TLS_PRF_OUT_LEN];
};

static struct tls_prf_test tls_prf_tests[] = {
	{
		.desc = "MD5+SHA1",
		.ssl_method = TLSv1_method,
		.cipher_value = 0x0033,
		.out = {
			0x03, 0xa1, 0xc1, 0x7d, 0x2c, 0xa5, 0x3d, 0xe8,
			0x9d, 0x59, 0x5e, 0x30, 0xf5, 0x71, 0xbb, 0x96,
			0xde, 0x5c, 0x8e, 0xdc, 0x25, 0x8a, 0x7c, 0x05,
			0x9f, 0x7d, 0x35, 0x29, 0x45, 0xae, 0x56, 0xad,
			0x9f, 0x57, 0x15, 0x5c, 0xdb, 0x83, 0x3a, 0xac,
			0x19, 0xa8, 0x2b, 0x40, 0x72, 0x38, 0x1e, 0xed,
			0xf3, 0x25, 0xde, 0x84, 0x84, 0xd8, 0xd1, 0xfc,
			0x31, 0x85, 0x81, 0x12, 0x55, 0x4d, 0x12, 0xb5,
			0xed, 0x78, 0x5e, 0xba, 0xc8, 0xec, 0x8d, 0x28,
			0xa1, 0x21, 0x1e, 0x6e, 0x07, 0xf1, 0xfc, 0xf5,
			0xbf, 0xe4, 0x8e, 0x8e, 0x97, 0x15, 0x93, 0x85,
			0x75, 0xdd, 0x87, 0x09, 0xd0, 0x4e, 0xe5, 0xd5,
			0x9e, 0x1f, 0xd6, 0x1c, 0x3b, 0xe9, 0xad, 0xba,
			0xe0, 0x16, 0x56, 0x62, 0x90, 0xd6, 0x82, 0x84,
			0xec, 0x8a, 0x22, 0xbe, 0xdc, 0x6a, 0x5e, 0x05,
			0x12, 0x44, 0xec, 0x60, 0x61, 0xd1, 0x8a, 0x66,
		},
	},
	{
		.desc = "GOST94",
		.ssl_method = TLSv1_2_method,
		.cipher_value = 0x0081,
		.out = {
			 0xcc, 0xd4, 0x89, 0x5f, 0x52, 0x08, 0x9b, 0xc7,
			 0xf9, 0xb5, 0x83, 0x58, 0xe8, 0xc7, 0x71, 0x49,
			 0x39, 0x99, 0x1f, 0x14, 0x8f, 0x85, 0xbe, 0x64,
			 0xee, 0x40, 0x5c, 0xe7, 0x5f, 0x68, 0xaf, 0xf2,
			 0xcd, 0x3a, 0x94, 0x52, 0x33, 0x53, 0x46, 0x7d,
			 0xb6, 0xc5, 0xe1, 0xb8, 0xa4, 0x04, 0x69, 0x91,
			 0x0a, 0x9c, 0x88, 0x86, 0xd9, 0x60, 0x63, 0xdd,
			 0xd8, 0xe7, 0x2e, 0xee, 0xce, 0xe2, 0x20, 0xd8,
			 0x9a, 0xfa, 0x9c, 0x63, 0x0c, 0x9c, 0xa1, 0x76,
			 0xed, 0x78, 0x9a, 0x84, 0x70, 0xb4, 0xd1, 0x51,
			 0x1f, 0xde, 0x44, 0xe8, 0x90, 0x21, 0x3f, 0xeb,
			 0x05, 0xf4, 0x77, 0x59, 0xf3, 0xad, 0xdd, 0x34,
			 0x3d, 0x3a, 0x7c, 0xd0, 0x59, 0x40, 0xe1, 0x3f,
			 0x04, 0x4b, 0x8b, 0xd6, 0x95, 0x46, 0xb4, 0x9e,
			 0x4c, 0x2d, 0xf7, 0xee, 0xbd, 0xbc, 0xcb, 0x5c,
			 0x3a, 0x36, 0x0c, 0xd0, 0x27, 0xcb, 0x45, 0x06,
		},
	},
	{
		.desc = "SHA256 (via TLSv1.2)",
		.ssl_method = TLSv1_2_method,
		.cipher_value = 0x0033,
		.out = {
			 0x37, 0xa7, 0x06, 0x71, 0x6e, 0x19, 0x19, 0xda,
			 0x23, 0x8c, 0xcc, 0xb4, 0x2f, 0x31, 0x64, 0x9d,
			 0x05, 0x29, 0x1c, 0x33, 0x7e, 0x09, 0x1b, 0x0c,
			 0x0e, 0x23, 0xc1, 0xb0, 0x40, 0xcc, 0x31, 0xf7,
			 0x55, 0x66, 0x68, 0xd9, 0xa8, 0xae, 0x74, 0x75,
			 0xf3, 0x46, 0xe9, 0x3a, 0x54, 0x9d, 0xe0, 0x8b,
			 0x7e, 0x6c, 0x63, 0x1c, 0xfa, 0x2f, 0xfd, 0xc9,
			 0xd3, 0xf1, 0xd3, 0xfe, 0x7b, 0x9e, 0x14, 0x95,
			 0xb5, 0xd0, 0xad, 0x9b, 0xee, 0x78, 0x8c, 0x83,
			 0x18, 0x58, 0x7e, 0xa2, 0x23, 0xc1, 0x8b, 0x62,
			 0x94, 0x12, 0xcb, 0xb6, 0x60, 0x69, 0x32, 0xfe,
			 0x98, 0x0e, 0x93, 0xb0, 0x8e, 0x5c, 0xfb, 0x6e,
			 0xdb, 0x9a, 0xc2, 0x9f, 0x8c, 0x5c, 0x43, 0x19,
			 0xeb, 0x4a, 0x52, 0xad, 0x62, 0x2b, 0xdd, 0x9f,
			 0xa3, 0x74, 0xa6, 0x96, 0x61, 0x4d, 0x98, 0x40,
			 0x63, 0xa6, 0xd4, 0xbb, 0x17, 0x11, 0x75, 0xed,
		},
	},
	{
		.desc = "SHA384",
		.ssl_method = TLSv1_2_method,
		.cipher_value = 0x009d,
		.out = {
			 0x00, 0x93, 0xc3, 0xfd, 0xa7, 0xbb, 0xdc, 0x5b,
			 0x13, 0x3a, 0xe6, 0x8b, 0x1b, 0xac, 0xf3, 0xfb,
			 0x3c, 0x9a, 0x78, 0xf6, 0x19, 0xf0, 0x13, 0x0f,
			 0x0d, 0x01, 0x9d, 0xdf, 0x0a, 0x28, 0x38, 0xce,
			 0x1a, 0x9b, 0x43, 0xbe, 0x56, 0x12, 0xa7, 0x16,
			 0x58, 0xe1, 0x8a, 0xe4, 0xc5, 0xbb, 0x10, 0x4c,
			 0x3a, 0xf3, 0x7f, 0xd3, 0xdb, 0xe4, 0xe0, 0x3d,
			 0xcc, 0x83, 0xca, 0xf0, 0xf9, 0x69, 0xcc, 0x70,
			 0x83, 0x32, 0xf6, 0xfc, 0x81, 0x80, 0x02, 0xe8,
			 0x31, 0x1e, 0x7c, 0x3b, 0x34, 0xf7, 0x34, 0xd1,
			 0xcf, 0x2a, 0xc4, 0x36, 0x2f, 0xe9, 0xaa, 0x7f,
			 0x6d, 0x1f, 0x5e, 0x0e, 0x39, 0x05, 0x15, 0xe1,
			 0xa2, 0x9a, 0x4d, 0x97, 0x8c, 0x62, 0x46, 0xf1,
			 0x87, 0x65, 0xd8, 0xe9, 0x14, 0x11, 0xa6, 0x48,
			 0xd7, 0x0e, 0x6e, 0x70, 0xad, 0xfb, 0x3f, 0x36,
			 0x05, 0x76, 0x4b, 0xe4, 0x28, 0x50, 0x4a, 0xf2,
		},
	},
	{
		.desc = "STREEBOG256",
		.ssl_method = TLSv1_2_method,
		.cipher_value = 0xff87,
		.out = {
			0x3e, 0x13, 0xb9, 0xeb, 0x85, 0x8c, 0xb4, 0x21,
			0x23, 0x40, 0x9b, 0x73, 0x04, 0x56, 0xe2, 0xff,
			0xce, 0x52, 0x1f, 0x82, 0x7f, 0x17, 0x5b, 0x80,
			0x23, 0x71, 0xca, 0x30, 0xdf, 0xfc, 0xdc, 0x2d,
			0xc0, 0xfc, 0x5d, 0x23, 0x5a, 0x54, 0x7f, 0xae,
			0xf5, 0x7d, 0x52, 0x1e, 0x86, 0x95, 0xe1, 0x2d,
			0x28, 0xe7, 0xbe, 0xd7, 0xd0, 0xbf, 0xa9, 0x96,
			0x13, 0xd0, 0x9c, 0x0c, 0x1c, 0x16, 0x05, 0xbb,
			0x26, 0xd7, 0x30, 0x39, 0xb9, 0x53, 0x28, 0x98,
			0x4f, 0x1b, 0x83, 0xc3, 0xce, 0x1c, 0x7c, 0x34,
			0xa2, 0xc4, 0x7a, 0x54, 0x16, 0xc6, 0xa7, 0x9e,
			0xed, 0x4b, 0x7b, 0x83, 0xa6, 0xae, 0xe2, 0x5b,
			0x96, 0xf5, 0x6c, 0xad, 0x1f, 0xa3, 0x83, 0xb2,
			0x84, 0x32, 0xed, 0xe3, 0x2c, 0xf6, 0xd4, 0x73,
			0x30, 0xef, 0x9d, 0xbe, 0xe7, 0x23, 0x9a, 0xbf,
			0x4d, 0x1c, 0xe7, 0xef, 0x3d, 0xea, 0x46, 0xe2,
		},
	},
};

#define N_TLS_PRF_TESTS \
    (sizeof(tls_prf_tests) / sizeof(*tls_prf_tests))

#define TLS_PRF_SEED1	"tls prf seed 1"
#define TLS_PRF_SEED2	"tls prf seed 2"
#define TLS_PRF_SEED3	"tls prf seed 3"
#define TLS_PRF_SEED4	"tls prf seed 4"
#define TLS_PRF_SEED5	"tls prf seed 5"
#define TLS_PRF_SECRET	"tls prf secretz"

static void
hexdump(const unsigned char *buf, size_t len)
{
	size_t i;

	for (i = 1; i <= len; i++)
		fprintf(stderr, " 0x%02hhx,%s", buf[i - 1], i % 8 ? "" : "\n");

	fprintf(stderr, "\n");
}

static int
do_tls_prf_test(int test_no, struct tls_prf_test *tpt)
{
	unsigned char *out = NULL;
	const SSL_CIPHER *cipher;
	SSL_CTX *ssl_ctx = NULL;
	SSL *ssl = NULL;
	int failure = 1;
	int len;

	fprintf(stderr, "Test %d - %s\n", test_no, tpt->desc);

	if ((out = malloc(TLS_PRF_OUT_LEN)) == NULL)
		errx(1, "failed to allocate out");

	if ((ssl_ctx = SSL_CTX_new(tpt->ssl_method())) == NULL)
		errx(1, "failed to create SSL context");
	if ((ssl = SSL_new(ssl_ctx)) == NULL)
		errx(1, "failed to create SSL context");

	if ((cipher = ssl3_get_cipher_by_value(tpt->cipher_value)) == NULL) {
		fprintf(stderr, "FAIL: no cipher %hx\n", tpt->cipher_value);
		goto failure;
	}

	ssl->s3->hs.cipher = cipher;

	for (len = 1; len <= TLS_PRF_OUT_LEN; len++) {
		memset(out, 'A', TLS_PRF_OUT_LEN);

		if (tls1_PRF(ssl, TLS_PRF_SECRET, sizeof(TLS_PRF_SECRET),
		    TLS_PRF_SEED1, sizeof(TLS_PRF_SEED1), TLS_PRF_SEED2,
		    sizeof(TLS_PRF_SEED2), TLS_PRF_SEED3, sizeof(TLS_PRF_SEED3),
		    TLS_PRF_SEED4, sizeof(TLS_PRF_SEED4), TLS_PRF_SEED5,
		    sizeof(TLS_PRF_SEED5), out, len) != 1) {
			fprintf(stderr, "FAIL: tls_PRF failed for len %d\n",
			    len);
			goto failure;
		}

		if (memcmp(out, tpt->out, len) != 0) {
			fprintf(stderr, "FAIL: tls_PRF output differs for "
			    "len %d\n", len);
			fprintf(stderr, "output:\n");
			hexdump(out, TLS_PRF_OUT_LEN);
			fprintf(stderr, "test data:\n");
			hexdump(tpt->out, TLS_PRF_OUT_LEN);
			fprintf(stderr, "\n");
			goto failure;
		}
	}

	failure = 0;

 failure:
	SSL_free(ssl);
	SSL_CTX_free(ssl_ctx);

	free(out);

	return failure;
}

int
main(int argc, char **argv)
{
	int failed = 0;
	size_t i;

	SSL_library_init();
	SSL_load_error_strings();

	for (i = 0; i < N_TLS_PRF_TESTS; i++)
		failed |= do_tls_prf_test(i, &tls_prf_tests[i]);

	return failed;
}

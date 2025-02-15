/*	$OpenBSD: bntest.c,v 1.25 2022/09/05 21:06:31 tb Exp $	*/
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
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the Eric Young open source
 * license provided above.
 *
 * The binary polynomial arithmetic software is originally written by
 * Sheueling Chang Shantz and Douglas Stebila of Sun Microsystems Laboratories.
 *
 */

/* Until the key-gen callbacks are modified to use newer prototypes, we allow
 * deprecated functions for openssl-internal code */
#ifdef OPENSSL_NO_DEPRECATED
#undef OPENSSL_NO_DEPRECATED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/x509.h>
#include <openssl/err.h>

int BN_mod_exp_ct(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
    const BIGNUM *m, BN_CTX *ctx);
int BN_mod_exp_nonct(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
    const BIGNUM *m, BN_CTX *ctx);
int BN_mod_exp_mont_ct(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
    const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx);
int BN_mod_exp_mont_nonct(BIGNUM *r, const BIGNUM *a, const BIGNUM *p,
    const BIGNUM *m, BN_CTX *ctx, BN_MONT_CTX *m_ctx);

int BN_bntest_rand(BIGNUM *rnd, int bits, int top, int bottom);

const int num0 = 100; /* number of tests */
const int num1 = 50;  /* additional tests for some functions */
const int num2 = 5;   /* number of tests for slow functions */

int test_add(BIO *bp);
int test_sub(BIO *bp);
int test_lshift1(BIO *bp);
int test_lshift(BIO *bp, BN_CTX *ctx, BIGNUM *a_);
int test_rshift1(BIO *bp);
int test_rshift(BIO *bp, BN_CTX *ctx);
int test_div(BIO *bp, BN_CTX *ctx);
int test_div_word(BIO *bp);
int test_div_recp(BIO *bp, BN_CTX *ctx);
int test_mul(BIO *bp);
int test_sqr(BIO *bp, BN_CTX *ctx);
int test_mont(BIO *bp, BN_CTX *ctx);
int test_mod(BIO *bp, BN_CTX *ctx);
int test_mod_mul(BIO *bp, BN_CTX *ctx);
int test_mod_exp(BIO *bp, BN_CTX *ctx);
int test_mod_exp_mont_consttime(BIO *bp, BN_CTX *ctx);
int test_mod_exp_mont5(BIO *bp, BN_CTX *ctx);
int test_mod_exp_sizes(BIO *bp, BN_CTX *ctx);
int test_exp(BIO *bp, BN_CTX *ctx);
int test_gf2m_add(BIO *bp);
int test_gf2m_mod(BIO *bp);
int test_gf2m_mod_mul(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_sqr(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_inv(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_div(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_exp(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_sqrt(BIO *bp, BN_CTX *ctx);
int test_gf2m_mod_solve_quad(BIO *bp, BN_CTX *ctx);
int test_kron(BIO *bp, BN_CTX *ctx);
int test_sqrt(BIO *bp, BN_CTX *ctx);
int rand_neg(void);
static int results = 0;

static unsigned char lst[] =
	"\xC6\x4F\x43\x04\x2A\xEA\xCA\x6E\x58\x36\x80\x5B\xE8\xC9"
	"\x9B\x04\x5D\x48\x36\xC2\xFD\x16\xC9\x64\xF0";

#define PRINT_ERROR printf("Error in %s [%s:%d]\n", __func__, __FILE__,	\
		__LINE__)

#define CHECK_GOTO(a) do {						\
	if (!(a)) {							\
		PRINT_ERROR;						\
		rc = 0;							\
		goto err;						\
	}								\
} while (0)

static void
message(BIO *out, char *m)
{
	fprintf(stderr, "test %s\n", m);
	BIO_puts(out, "print \"test ");
	BIO_puts(out, m);
	BIO_puts(out, "\\n\"\n");
}

int
main(int argc, char *argv[])
{
	BN_CTX *ctx;
	BIO *out;
	char *outfile = NULL;

	results = 0;

	argc--;
	argv++;
	while (argc >= 1) {
		if (strcmp(*argv, "-results") == 0)
			results = 1;
		else if (strcmp(*argv, "-out") == 0) {
			if (--argc < 1)
				break;
			outfile= *(++argv);
		}
		argc--;
		argv++;
	}


	ctx = BN_CTX_new();
	if (ctx == NULL)
		exit(1);

	out = BIO_new(BIO_s_file());
	if (out == NULL)
		exit(1);
	if (outfile == NULL) {
		BIO_set_fp(out, stdout, BIO_NOCLOSE);
	} else {
		if (!BIO_write_filename(out, outfile)) {
			perror(outfile);
			exit(1);
		}
	}

	if (!results)
		BIO_puts(out, "obase=16\nibase=16\n");

	message(out, "BN_add");
	if (!test_add(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_sub");
	if (!test_sub(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_lshift1");
	if (!test_lshift1(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_lshift (fixed)");
	if (!test_lshift(out, ctx, BN_bin2bn(lst, sizeof(lst) - 1, NULL)))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_lshift");
	if (!test_lshift(out, ctx, NULL))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_rshift1");
	if (!test_rshift1(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_rshift");
	if (!test_rshift(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_sqr");
	if (!test_sqr(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mul");
	if (!test_mul(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_div");
	if (!test_div(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_div_word");
	if (!test_div_word(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_div_recp");
	if (!test_div_recp(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod");
	if (!test_mod(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod_mul");
	if (!test_mod_mul(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mont");
	if (!test_mont(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod_exp");
	if (!test_mod_exp(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod_exp_mont_consttime");
	if (!test_mod_exp_mont_consttime(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod_exp_mont5");
	if (!test_mod_exp_mont5(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_exp");
	if (!test_exp(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_kronecker");
	if (!test_kron(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_mod_sqrt");
	if (!test_sqrt(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "Modexp with different sizes");
	if (!test_mod_exp_sizes(out, ctx))
		goto err;
	(void)BIO_flush(out);

#ifndef OPENSSL_NO_EC2M
	message(out, "BN_GF2m_add");
	if (!test_gf2m_add(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod");
	if (!test_gf2m_mod(out))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_mul");
	if (!test_gf2m_mod_mul(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_sqr");
	if (!test_gf2m_mod_sqr(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_inv");
	if (!test_gf2m_mod_inv(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_div");
	if (!test_gf2m_mod_div(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_exp");
	if (!test_gf2m_mod_exp(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_sqrt");
	if (!test_gf2m_mod_sqrt(out, ctx))
		goto err;
	(void)BIO_flush(out);

	message(out, "BN_GF2m_mod_solve_quad");
	if (!test_gf2m_mod_solve_quad(out, ctx))
		goto err;
	(void)BIO_flush(out);
#endif
	BN_CTX_free(ctx);
	BIO_free(out);

	exit(0);
 err:
	BIO_puts(out, "1\n"); /* make sure the Perl script fed by bc notices
	                       * the failure, see test_bn in test/Makefile.ssl*/

	(void)BIO_flush(out);
	ERR_load_crypto_strings();
	ERR_print_errors_fp(stderr);
	exit(1);
}

int
test_add(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(b, 450 + i, 0, 0));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_add(c, a, b));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " + ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		BN_set_negative(a, !BN_is_negative(a));
		BN_set_negative(b, !BN_is_negative(b));
		CHECK_GOTO(BN_add(c, c, b));
		CHECK_GOTO(BN_add(c, c, a));
		if (!BN_is_zero(c)) {
			fprintf(stderr, "Add test failed!\n");
			rc = 0;
			break;
		}
	}

 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);

	return rc;
}

int
test_sub(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	for (i = 0; i < num0 + num1; i++) {
		if (i < num1) {
			CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
			CHECK_GOTO(BN_copy(b, a));
			if (BN_set_bit(a, i) == 0) {
				rc = 0;
				break;
			}
			CHECK_GOTO(BN_add_word(b, i));
		} else {
			CHECK_GOTO(BN_bntest_rand(b, 400 + i - num1, 0, 0));
			BN_set_negative(a, rand_neg());
			BN_set_negative(b, rand_neg());
		}
		CHECK_GOTO(BN_sub(c, a, b));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " - ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_add(c, c, b));
		CHECK_GOTO(BN_sub(c, c, a));
		if (!BN_is_zero(c)) {
			fprintf(stderr, "Subtract test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	return (rc);
}

int
test_div(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_one(a));
	CHECK_GOTO(BN_zero(b));

	if (BN_div(d, c, a, b, ctx)) {
		fprintf(stderr, "Division by zero succeeded!\n");
		return (0);
	}

	for (i = 0; i < num0 + num1; i++) {
		if (i < num1) {
			CHECK_GOTO(BN_bntest_rand(a, 400, 0, 0));
			CHECK_GOTO(BN_copy(b, a));
			CHECK_GOTO(BN_lshift(a, a, i));
			CHECK_GOTO(BN_add_word(a, i));
		} else
			CHECK_GOTO(BN_bntest_rand(b, 50 + 3 * (i - num1), 0, 0));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_div(d, c, a, b, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " / ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");

			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_mul(e, d, b, ctx));
		CHECK_GOTO(BN_add(d, e, c));
		CHECK_GOTO(BN_sub(d, d, a));
		if (!BN_is_zero(d)) {
			fprintf(stderr, "Division test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

static void
print_word(BIO *bp, BN_ULONG w)
{
#ifdef SIXTY_FOUR_BIT
	if (sizeof(w) > sizeof(unsigned long)) {
		unsigned long h = (unsigned long)(w >> 32), l = (unsigned long)(w);

		if (h)
			BIO_printf(bp, "%lX%08lX", h, l);
		else
			BIO_printf(bp, "%lX", l);
		return;
	}
#endif
	BIO_printf(bp, BN_HEX_FMT1, w);
}

int
test_div_word(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL;
	BN_ULONG r, rmod, s = 0;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;

	for (i = 0; i < num0; i++) {
		do {
			if (!BN_bntest_rand(a, 512, -1, 0) ||
			    !BN_bntest_rand(b, BN_BITS2, -1, 0)) {
				rc = 0;
				break;
			}
			s = BN_get_word(b);
		} while (!s);

		if (!BN_copy(b, a)) {
			rc = 0;
			break;
		}

		rmod = BN_mod_word(b, s);
		r = BN_div_word(b, s);

		if (r == (BN_ULONG)-1 || rmod == (BN_ULONG)-1) {
			rc = 0;
			break;
		}

		if (rmod != r) {
			fprintf(stderr, "Mod (word) test failed!\n");
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " / ");
				print_word(bp, s);
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, b));
			BIO_puts(bp, "\n");

			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " % ");
				print_word(bp, s);
				BIO_puts(bp, " - ");
			}
			print_word(bp, r);
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_mul_word(b, s));
		CHECK_GOTO(BN_add_word(b, r));
		CHECK_GOTO(BN_sub(b, a, b));
		if (!BN_is_zero(b)) {
			fprintf(stderr, "Division (word) test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);

	return rc;
}

int
test_div_recp(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	BN_RECP_CTX *recp = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	if ((recp = BN_RECP_CTX_new()) == NULL)
		goto err;

	for (i = 0; i < num0 + num1; i++) {
		if (i < num1) {
			CHECK_GOTO(BN_bntest_rand(a, 400, 0, 0));
			CHECK_GOTO(BN_copy(b, a));
			CHECK_GOTO(BN_lshift(a, a, i));
			CHECK_GOTO(BN_add_word(a, i));
		} else
			CHECK_GOTO(BN_bntest_rand(b, 50 + 3 * (i - num1), 0, 0));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_RECP_CTX_set(recp, b, ctx));
		CHECK_GOTO(BN_div_recp(d, c, a, recp, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " / ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");

			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_mul(e, d, b, ctx));
		CHECK_GOTO(BN_add(d, e, c));
		CHECK_GOTO(BN_sub(d, d, a));
		if (!BN_is_zero(d)) {
			fprintf(stderr, "Reciprocal division test failed!\n");
			fprintf(stderr, "a=");
			CHECK_GOTO(BN_print_fp(stderr, a));
			fprintf(stderr, "\nb=");
			CHECK_GOTO(BN_print_fp(stderr, b));
			fprintf(stderr, "\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_RECP_CTX_free(recp);
	return (rc);
}

int
test_mul(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;
	BN_CTX *ctx;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		exit(1);

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	for (i = 0; i < num0 + num1; i++) {
		if (i <= num1) {
			CHECK_GOTO(BN_bntest_rand(a, 100, 0, 0));
			CHECK_GOTO(BN_bntest_rand(b, 100, 0, 0));
		} else
			CHECK_GOTO(BN_bntest_rand(b, i - num1, 0, 0));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_mul(c, a, b, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " * ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_div(d, e, c, a, ctx));
		CHECK_GOTO(BN_sub(d, d, b));
		if (!BN_is_zero(d) || !BN_is_zero(e)) {
			fprintf(stderr, "Multiplication test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_CTX_free(ctx);
	return (rc);
}

int
test_sqr(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i, rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 40 + i * 10, 0, 0));
		BN_set_negative(a, rand_neg());
		CHECK_GOTO(BN_sqr(c, a, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " * ");
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_div(d, e, c, a, ctx));
		CHECK_GOTO(BN_sub(d, d, a));
		if (!BN_is_zero(d) || !BN_is_zero(e)) {
			fprintf(stderr, "Square test failed!\n");
			goto err;
		}
	}

	/* Regression test for a BN_sqr overflow bug. */
	if (!BN_hex2bn(&a, "80000000000000008000000000000001"
	    "FFFFFFFFFFFFFFFE0000000000000000")) {
		fprintf(stderr, "BN_hex2bn failed\n");
		goto err;
	}
	CHECK_GOTO(BN_sqr(c, a, ctx));
	if (bp != NULL) {
		if (!results) {
			CHECK_GOTO(BN_print(bp, a));
			BIO_puts(bp, " * ");
			CHECK_GOTO(BN_print(bp, a));
			BIO_puts(bp, " - ");
		}
		CHECK_GOTO(BN_print(bp, c));
		BIO_puts(bp, "\n");
	}
	CHECK_GOTO(BN_mul(d, a, a, ctx));
	if (BN_cmp(c, d)) {
		fprintf(stderr,
		    "Square test failed: BN_sqr and BN_mul produce "
		    "different results!\n");
		goto err;
	}

	/* Regression test for a BN_sqr overflow bug. */
	if (!BN_hex2bn(&a, "80000000000000000000000080000001"
	    "FFFFFFFE000000000000000000000000")) {
		fprintf(stderr, "BN_hex2bn failed\n");
		goto err;
	}
	CHECK_GOTO(BN_sqr(c, a, ctx));
	if (bp != NULL) {
		if (!results) {
			CHECK_GOTO(BN_print(bp, a));
			BIO_puts(bp, " * ");
			CHECK_GOTO(BN_print(bp, a));
			BIO_puts(bp, " - ");
		}
		CHECK_GOTO(BN_print(bp, c));
		BIO_puts(bp, "\n");
	}
	CHECK_GOTO(BN_mul(d, a, a, ctx));
	if (BN_cmp(c, d)) {
		fprintf(stderr, "Square test failed: BN_sqr and BN_mul produce "
				"different results!\n");
		goto err;
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return rc;
}

int
test_mont(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *A = NULL, *B = NULL;
	BIGNUM *n = NULL;
	int i;
	int rc = 1;
	BN_MONT_CTX *mont;

	mont = BN_MONT_CTX_new();
	if (mont == NULL)
		return 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((A = BN_new()) == NULL)
		goto err;
	if ((B = BN_new()) == NULL)
		goto err;
	if ((n = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_zero(n));
	if (BN_MONT_CTX_set(mont, n, ctx)) {
		fprintf(stderr, "BN_MONT_CTX_set succeeded for zero modulus!\n");
		return (0);
	}

	CHECK_GOTO(BN_set_word(n, 16));
	if (BN_MONT_CTX_set(mont, n, ctx)) {
		fprintf(stderr, "BN_MONT_CTX_set succeeded for even modulus!\n");
		return (0);
	}

	CHECK_GOTO(BN_bntest_rand(a, 100, 0, 0));
	CHECK_GOTO(BN_bntest_rand(b, 100, 0, 0));
	for (i = 0; i < num2; i++) {
		int bits = (200 * (i + 1)) / num2;

		if (bits == 0)
			continue;
		CHECK_GOTO(BN_bntest_rand(n, bits, 0, 1));
		CHECK_GOTO(BN_MONT_CTX_set(mont, n, ctx));

		CHECK_GOTO(BN_nnmod(a, a, n, ctx));
		CHECK_GOTO(BN_nnmod(b, b, n, ctx));

		CHECK_GOTO(BN_to_montgomery(A, a, mont, ctx));
		CHECK_GOTO(BN_to_montgomery(B, b, mont, ctx));

		CHECK_GOTO(BN_mod_mul_montgomery(c, A, B, mont, ctx));
		CHECK_GOTO(BN_from_montgomery(A, c, mont, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " * ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " % ");
				/* n == &mont->N */
				CHECK_GOTO(BN_print(bp, n));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, A));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_mod_mul(d, a, b, n, ctx));
		CHECK_GOTO(BN_sub(d, d, A));
		if (!BN_is_zero(d)) {
			fprintf(stderr, "Montgomery multiplication test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_MONT_CTX_free(mont);
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(A);
	BN_free(B);
	BN_free(n);
	return (rc);
}

int
test_mod(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_bntest_rand(a, 1024, 0, 0));
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(b, 450 + i * 10, 0, 0));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_mod(c, a, b, ctx));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_div(d, e, a, b, ctx));
		CHECK_GOTO(BN_sub(e, e, c));
		if (!BN_is_zero(e)) {
			fprintf(stderr, "Modulo test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

int
test_mod_mul(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i, j;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_one(a));
	CHECK_GOTO(BN_one(b));
	CHECK_GOTO(BN_zero(c));
	if (BN_mod_mul(e, a, b, c, ctx)) {
		fprintf(stderr, "BN_mod_mul with zero modulus succeeded!\n");
		return (0);
	}

	for (j = 0; j < 3; j++) {
		CHECK_GOTO(BN_bntest_rand(c, 1024, 0, 0));
		for (i = 0; i < num0; i++) {
			CHECK_GOTO(BN_bntest_rand(a, 475 + i * 10, 0, 0));
			CHECK_GOTO(BN_bntest_rand(b, 425 + i * 11, 0, 0));
			BN_set_negative(a, rand_neg());
			BN_set_negative(b, rand_neg());
			if (!BN_mod_mul(e, a, b, c, ctx)) {
				unsigned long l;

				while ((l = ERR_get_error()))
					fprintf(stderr, "ERROR:%s\n",
					    ERR_error_string(l, NULL));
				exit(1);
			}
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " * ");
					CHECK_GOTO(BN_print(bp, b));
					BIO_puts(bp, " % ");
					CHECK_GOTO(BN_print(bp, c));
					if ((BN_is_negative(a) ^ BN_is_negative(b)) &&
					    !BN_is_zero(e)) {
						/* If  (a*b) % c  is negative,  c  must be added
						 * in order to obtain the normalized remainder
						 * (new with OpenSSL 0.9.7, previous versions of
						 * BN_mod_mul could generate negative results)
						 */
						BIO_puts(bp, " + ");
						CHECK_GOTO(BN_print(bp, c));
					}
					BIO_puts(bp, " - ");
				}
				CHECK_GOTO(BN_print(bp, e));
				BIO_puts(bp, "\n");
			}
			CHECK_GOTO(BN_mul(d, a, b, ctx));
			CHECK_GOTO(BN_sub(d, d, e));
			CHECK_GOTO(BN_div(a, b, d, c, ctx));
			if (!BN_is_zero(b)) {
				fprintf(stderr, "Modulo multiply test failed!\n");
				ERR_print_errors_fp(stderr);
				rc = 0;
				goto err;
			}
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

int
test_mod_exp(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_one(a));
	CHECK_GOTO(BN_one(b));
	CHECK_GOTO(BN_zero(c));
	if (BN_mod_exp(d, a, b, c, ctx)) {
		fprintf(stderr, "BN_mod_exp with zero modulus succeeded!\n");
		rc = 0;
		goto err;
	}
	if (BN_mod_exp_ct(d, a, b, c, ctx)) {
		fprintf(stderr, "BN_mod_exp_ct with zero modulus succeeded!\n");
		rc = 0;
		goto err;
	}
	if (BN_mod_exp_nonct(d, a, b, c, ctx)) {
		fprintf(stderr, "BN_mod_exp_nonct with zero modulus succeeded!\n");
		rc = 0;
		goto err;
	}

	CHECK_GOTO(BN_bntest_rand(c, 30, 0, 1)); /* must be odd for montgomery */
	for (i = 0; i < num2; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 20 + i * 5, 0, 0));
		CHECK_GOTO(BN_bntest_rand(b, 2 + i, 0, 0));

		if (!BN_mod_exp(d, a, b, c, ctx)) {
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_exp(e, a, b, ctx));
		CHECK_GOTO(BN_sub(e, e, d));
		CHECK_GOTO(BN_div(a, b, e, c, ctx));
		if (!BN_is_zero(b)) {
			fprintf(stderr, "Modulo exponentiation test failed!\n");
			rc = 0;
			break;
		}
	}

	CHECK_GOTO(BN_bntest_rand(c, 30, 0, 1)); /* must be odd for montgomery */
	for (i = 0; i < num2; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 20 + i * 5, 0, 0));
		CHECK_GOTO(BN_bntest_rand(b, 2 + i, 0, 0));

		if (!BN_mod_exp_ct(d, a, b, c, ctx)) {
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_exp(e, a, b, ctx));
		CHECK_GOTO(BN_sub(e, e, d));
		CHECK_GOTO(BN_div(a, b, e, c, ctx));
		if (!BN_is_zero(b)) {
			fprintf(stderr, "Modulo exponentiation test failed!\n");
			rc = 0;
			break;
		}
	}

	CHECK_GOTO(BN_bntest_rand(c, 30, 0, 1)); /* must be odd for montgomery */
	for (i = 0; i < num2; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 20 + i * 5, 0, 0));
		CHECK_GOTO(BN_bntest_rand(b, 2 + i, 0, 0));

		if (!BN_mod_exp_nonct(d, a, b, c, ctx)) {
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_exp(e, a, b, ctx));
		CHECK_GOTO(BN_sub(e, e, d));
		CHECK_GOTO(BN_div(a, b, e, c, ctx));
		if (!BN_is_zero(b)) {
			fprintf(stderr, "Modulo exponentiation test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

int
test_mod_exp_mont_consttime(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_one(a));
	CHECK_GOTO(BN_one(b));
	CHECK_GOTO(BN_zero(c));
	if (BN_mod_exp_mont_consttime(d, a, b, c, ctx, NULL)) {
		fprintf(stderr, "BN_mod_exp_mont_consttime with zero modulus "
				"succeeded\n");
		rc = 0;
		goto err;
	}

	CHECK_GOTO(BN_set_word(c, 16));
	if (BN_mod_exp_mont_consttime(d, a, b, c, ctx, NULL)) {
		fprintf(stderr, "BN_mod_exp_mont_consttime with even modulus "
				"succeeded\n");
		rc = 0;
		goto err;
	}

	CHECK_GOTO(BN_bntest_rand(c, 30, 0, 1)); /* must be odd for montgomery */
	for (i = 0; i < num2; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 20 + i * 5, 0, 0));
		CHECK_GOTO(BN_bntest_rand(b, 2 + i, 0, 0));

		if (!BN_mod_exp_mont_consttime(d, a, b, c, ctx, NULL)) {
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " % ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_exp(e, a, b, ctx));
		CHECK_GOTO(BN_sub(e, e, d));
		CHECK_GOTO(BN_div(a, b, e, c, ctx));
		if (!BN_is_zero(b)) {
			fprintf(stderr, "Modulo exponentiation test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

/*
 * Test constant-time modular exponentiation with 1024-bit inputs, which on
 * x86_64 cause a different code branch to be taken.
 */
int
test_mod_exp_mont5(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *p = NULL, *m = NULL, *d = NULL, *e = NULL;
	BIGNUM *b = NULL, *n = NULL, *c = NULL;
	BN_MONT_CTX *mont = NULL;
	int len;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((p = BN_new()) == NULL)
		goto err;
	if ((m = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((n = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(mont = BN_MONT_CTX_new());

	CHECK_GOTO(BN_bntest_rand(m, 1024, 0, 1)); /* must be odd for montgomery */
	/* Zero exponent */
	CHECK_GOTO(BN_bntest_rand(a, 1024, 0, 0));
	CHECK_GOTO(BN_zero(p));
	if (!BN_mod_exp_mont_consttime(d, a, p, m, ctx, NULL)) {
		rc = 0;
		goto err;
	}
	if (!BN_is_one(d)) {
		fprintf(stderr, "Modular exponentiation test failed!\n");
		rc = 0;
		goto err;
	}
	/* Regression test for carry bug in mulx4x_mont */
	len = BN_hex2bn(&a,
	    "7878787878787878787878787878787878787878787878787878787878787878"
	    "7878787878787878787878787878787878787878787878787878787878787878"
	    "7878787878787878787878787878787878787878787878787878787878787878"
	    "7878787878787878787878787878787878787878787878787878787878787878");
	CHECK_GOTO(len);
	len = BN_hex2bn(&b,
	    "095D72C08C097BA488C5E439C655A192EAFB6380073D8C2664668EDDB4060744"
	    "E16E57FB4EDB9AE10A0CEFCDC28A894F689A128379DB279D48A2E20849D68593"
	    "9B7803BCF46CEBF5C533FB0DD35B080593DE5472E3FE5DB951B8BFF9B4CB8F03"
	    "9CC638A5EE8CDD703719F8000E6A9F63BEED5F2FCD52FF293EA05A251BB4AB81");
	CHECK_GOTO(len);
	len = BN_hex2bn(&n,
	    "D78AF684E71DB0C39CFF4E64FB9DB567132CB9C50CC98009FEB820B26F2DED9B"
	    "91B9B5E2B83AE0AE4EB4E0523CA726BFBE969B89FD754F674CE99118C3F2D1C5"
	    "D81FDC7C54E02B60262B241D53C040E99E45826ECA37A804668E690E1AFC1CA4"
	    "2C9A15D84D4954425F0B7642FC0BD9D7B24E2618D2DCC9B729D944BADACFDDAF");
	CHECK_GOTO(len);
	CHECK_GOTO(BN_MONT_CTX_set(mont, n, ctx));
	CHECK_GOTO(BN_mod_mul_montgomery(c, a, b, mont, ctx));
	CHECK_GOTO(BN_mod_mul_montgomery(d, b, a, mont, ctx));
	if (BN_cmp(c, d)) {
		fprintf(stderr, "Montgomery multiplication test failed:"
		    " a*b != b*a.\n");
		rc = 0;
		goto err;
	}
	/* Regression test for carry bug in sqr[x]8x_mont */
	len = BN_hex2bn(&n,
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000FFFFFFFF00"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "00000000000000000000000000000000000000000000000000FFFFFFFFFFFFFF");
	CHECK_GOTO(len);
	len = BN_hex2bn(&a,
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"
	    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000000000FFFFFFFF0000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "0000000000000000000000000000000000000000000000000000000000000000"
	    "000000000000000000000000000000000000000000FFFFFFFFFFFFFF00000000");
	CHECK_GOTO(len);
	BN_free(b);
	CHECK_GOTO(b = BN_dup(a));
	CHECK_GOTO(BN_MONT_CTX_set(mont, n, ctx));
	CHECK_GOTO(BN_mod_mul_montgomery(c, a, a, mont, ctx));
	CHECK_GOTO(BN_mod_mul_montgomery(d, a, b, mont, ctx));
	if (BN_cmp(c, d)) {
		fprintf(stderr, "Montgomery multiplication test failed:"
		    " a**2 != a*a.\n");
		rc = 0;
		goto err;
	}
	/* Zero input */
	CHECK_GOTO(BN_bntest_rand(p, 1024, 0, 0));
	CHECK_GOTO(BN_zero(a));
	if (!BN_mod_exp_mont_consttime(d, a, p, m, ctx, NULL)) {
		rc = 0;
		goto err;
	}
	if (!BN_is_zero(d)) {
		fprintf(stderr, "Modular exponentiation test failed!\n");
		rc = 0;
		goto err;
	}
	/*
	 * Craft an input whose Montgomery representation is 1, i.e., shorter
	 * than the modulus m, in order to test the const time precomputation
	 * scattering/gathering.
	 */
	CHECK_GOTO(BN_one(a));
	CHECK_GOTO(BN_MONT_CTX_set(mont, m, ctx));
	if (!BN_from_montgomery(e, a, mont, ctx)) {
		rc = 0;
		goto err;
	}
	if (!BN_mod_exp_mont_consttime(d, e, p, m, ctx, NULL)) {
		rc = 0;
		goto err;
	}
	if (!BN_mod_exp_simple(a, e, p, m, ctx)) {
		rc = 0;
		goto err;
	}
	if (BN_cmp(a, d) != 0) {
		fprintf(stderr, "Modular exponentiation test failed!\n");
		rc = 0;
		goto err;
	}
	/* Finally, some regular test vectors. */
	CHECK_GOTO(BN_bntest_rand(e, 1024, 0, 0));
	if (!BN_mod_exp_mont_consttime(d, e, p, m, ctx, NULL)) {
		rc = 0;
		goto err;
	}
	if (!BN_mod_exp_simple(a, e, p, m, ctx)) {
		rc = 0;
		goto err;
	}
	if (BN_cmp(a, d) != 0) {
		fprintf(stderr, "Modular exponentiation test failed!\n");
		rc = 0;
		goto err;
	}
 err:
	BN_free(a);
	BN_free(p);
	BN_free(m);
	BN_free(d);
	BN_free(e);
	BN_free(b);
	BN_free(n);
	BN_free(c);
	BN_MONT_CTX_free(mont);
	return (rc);
}

int
test_exp(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *d = NULL, *e = NULL, *one = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((one = BN_new()) == NULL)
		goto err;
	CHECK_GOTO(BN_one(one));

	for (i = 0; i < num2; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 20 + i * 5, 0, 0));
		CHECK_GOTO(BN_bntest_rand(b, 2 + i, 0, 0));

		if (BN_exp(d, a, b, ctx) <= 0) {
			rc = 0;
			break;
		}

		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, d));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_one(e));
		for (; !BN_is_zero(b); BN_sub(b, b, one))
			CHECK_GOTO(BN_mul(e, e, a, ctx));
		CHECK_GOTO(BN_sub(e, e, d));
		if (!BN_is_zero(e)) {
			fprintf(stderr, "Exponentiation test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(d);
	BN_free(e);
	BN_free(one);
	return (rc);
}

#ifndef OPENSSL_NO_EC2M
int
test_gf2m_add(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL;
	int i, rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_rand(a, 512, 0, 0));
		CHECK_GOTO(BN_copy(b, BN_value_one()));
		BN_set_negative(a, rand_neg());
		BN_set_negative(b, rand_neg());
		CHECK_GOTO(BN_GF2m_add(c, a, b));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " ^ ");
				CHECK_GOTO(BN_print(bp, b));
				BIO_puts(bp, " = ");
			}
			CHECK_GOTO(BN_print(bp, c));
			BIO_puts(bp, "\n");
		}
#endif
		/* Test that two added values have the correct parity. */
		if ((BN_is_odd(a) && BN_is_odd(c))
		    || (!BN_is_odd(a) && !BN_is_odd(c))) {
			fprintf(stderr, "GF(2^m) addition test (a) failed!\n");
			goto err;
		}
		CHECK_GOTO(BN_GF2m_add(c, c, c));
		/* Test that c + c = 0. */
		if (!BN_is_zero(c)) {
			fprintf(stderr, "GF(2^m) addition test (b) failed!\n");
			goto err;
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	return rc;
}

int
test_gf2m_mod(BIO *bp)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL;
	int i, j;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };
	int rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 1024, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod(c, a, b[j]));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, " - ");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, "\n");
				}
			}
#endif
			CHECK_GOTO(BN_GF2m_add(d, a, c));
			CHECK_GOTO(BN_GF2m_mod(e, d, b[j]));
			/* Test that a + (a mod p) mod p == 0. */
			if (!BN_is_zero(e)) {
				fprintf(stderr, "GF(2^m) modulo test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return rc;
}

int
test_gf2m_mod_mul(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL, *f = NULL;
	BIGNUM *g = NULL, *h = NULL;
	int i, j;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };
	int rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((f = BN_new()) == NULL)
		goto err;
	if ((g = BN_new()) == NULL)
		goto err;
	if ((h = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 1024, 0, 0));
		CHECK_GOTO(BN_bntest_rand(c, 1024, 0, 0));
		CHECK_GOTO(BN_bntest_rand(d, 1024, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod_mul(e, a, c, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " * ");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, " % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, " - ");
					CHECK_GOTO(BN_print(bp, e));
					BIO_puts(bp, "\n");
				}
			}
#endif
			CHECK_GOTO(BN_GF2m_add(f, a, d));
			CHECK_GOTO(BN_GF2m_mod_mul(g, f, c, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_mul(h, d, c, b[j], ctx));
			CHECK_GOTO(BN_GF2m_add(f, e, g));
			CHECK_GOTO(BN_GF2m_add(f, f, h));
			/* Test that (a+d)*c = a*c + d*c. */
			if (!BN_is_zero(f)) {
				fprintf(stderr, "GF(2^m) modular multiplication test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_free(f);
	BN_free(g);
	BN_free(h);
	return rc;
}

int
test_gf2m_mod_sqr(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL;
	int i, j, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 1024, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod_sqr(c, a, b[j], ctx));
			CHECK_GOTO(BN_copy(d, a));
			CHECK_GOTO(BN_GF2m_mod_mul(d, a, d, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " ^ 2 % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, " = ");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, "; a * a = ");
					CHECK_GOTO(BN_print(bp, d));
					BIO_puts(bp, "\n");
				}
			}
#endif
			CHECK_GOTO(BN_GF2m_add(d, c, d));
			/* Test that a*a = a^2. */
			if (!BN_is_zero(d)) {
				fprintf(stderr, "GF(2^m) modular squaring test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	return rc;
}

int
test_gf2m_mod_inv(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL;
	int i, j, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod_inv(c, a, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_mul(d, a, c, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " * ");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, " - 1 % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, "\n");
				}
			}
#endif
			/* Test that ((1/a)*a) = 1. */
			if (!BN_is_one(d)) {
				fprintf(stderr, "GF(2^m) modular inversion test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	return rc;
}

int
test_gf2m_mod_div(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL, *f = NULL;
	int i, j, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((f = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
		CHECK_GOTO(BN_bntest_rand(c, 512, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod_div(d, a, c, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_mul(e, d, c, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_div(f, a, e, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " = ");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, " * ");
					CHECK_GOTO(BN_print(bp, d));
					BIO_puts(bp, " % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, "\n");
				}
			}
#endif
			/* Test that ((a/c)*c)/a = 1. */
			if (!BN_is_one(f)) {
				fprintf(stderr, "GF(2^m) modular division test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_free(f);
	return rc;
}

int
test_gf2m_mod_exp(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL, *f = NULL;
	int i, j, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((f = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
		CHECK_GOTO(BN_bntest_rand(c, 512, 0, 0));
		CHECK_GOTO(BN_bntest_rand(d, 512, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod_exp(e, a, c, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_exp(f, a, d, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_mul(e, e, f, b[j], ctx));
			CHECK_GOTO(BN_add(f, c, d));
			CHECK_GOTO(BN_GF2m_mod_exp(f, a, f, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, " ^ (");
					CHECK_GOTO(BN_print(bp, c));
					BIO_puts(bp, " + ");
					CHECK_GOTO(BN_print(bp, d));
					BIO_puts(bp, ") = ");
					CHECK_GOTO(BN_print(bp, e));
					BIO_puts(bp, "; - ");
					CHECK_GOTO(BN_print(bp, f));
					BIO_puts(bp, " % ");
					CHECK_GOTO(BN_print(bp, b[j]));
					BIO_puts(bp, "\n");
				}
			}
#endif
			CHECK_GOTO(BN_GF2m_add(f, e, f));
			/* Test that a^(c+d)=a^c*a^d. */
			if (!BN_is_zero(f)) {
				fprintf(stderr, "GF(2^m) modular exponentiation test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_free(f);
	return rc;
}

int
test_gf2m_mod_sqrt(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL, *f = NULL;
	int i, j, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	if ((f = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
		for (j = 0; j < 2; j++) {
			CHECK_GOTO(BN_GF2m_mod(c, a, b[j]));
			CHECK_GOTO(BN_GF2m_mod_sqrt(d, a, b[j], ctx));
			CHECK_GOTO(BN_GF2m_mod_sqr(e, d, b[j], ctx));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
			if (bp != NULL) {
				if (!results) {
					CHECK_GOTO(BN_print(bp, d));
					BIO_puts(bp, " ^ 2 - ");
					CHECK_GOTO(BN_print(bp, a));
					BIO_puts(bp, "\n");
				}
			}
#endif
			CHECK_GOTO(BN_GF2m_add(f, c, e));
			/* Test that d^2 = a, where d = sqrt(a). */
			if (!BN_is_zero(f)) {
				fprintf(stderr, "GF(2^m) modular square root test failed!\n");
				goto err;
			}
		}
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	BN_free(f);
	return rc;
}

int
test_gf2m_mod_solve_quad(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b[2] = { 0 }, *c = NULL, *d = NULL, *e = NULL;
	int i, j, s = 0, t, rc = 0;
	int p0[] = { 163, 7, 6, 3, 0, -1 };
	int p1[] = { 193, 15, 0, -1 };

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b[0] = BN_new()) == NULL)
		goto err;
	if ((b[1] = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_GF2m_arr2poly(p0, b[0]));
	CHECK_GOTO(BN_GF2m_arr2poly(p1, b[1]));

	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_bntest_rand(a, 512, 0, 0));
		for (j = 0; j < 2; j++) {
			t = BN_GF2m_mod_solve_quad(c, a, b[j], ctx);
			if (t) {
				s++;
				CHECK_GOTO(BN_GF2m_mod_sqr(d, c, b[j], ctx));
				CHECK_GOTO(BN_GF2m_add(d, c, d));
				CHECK_GOTO(BN_GF2m_mod(e, a, b[j]));
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
				if (bp != NULL) {
					if (!results) {
						CHECK_GOTO(BN_print(bp, c));
						BIO_puts(bp, " is root of z^2 + z = ");
						CHECK_GOTO(BN_print(bp, a));
						BIO_puts(bp, " % ");
						CHECK_GOTO(BN_print(bp, b[j]));
						BIO_puts(bp, "\n");
					}
				}
#endif
				CHECK_GOTO(BN_GF2m_add(e, e, d));
				/* Test that solution of quadratic c satisfies c^2 + c = a. */
				if (!BN_is_zero(e)) {
					fprintf(stderr, "GF(2^m) modular solve quadratic test failed!\n");
					goto err;
				}

			} else {
#if 0 /* make test uses ouput in bc but bc can't handle GF(2^m) arithmetic */
				if (bp != NULL) {
					if (!results) {
						BIO_puts(bp, "There are no roots of z^2 + z = ");
						CHECK_GOTO(BN_print(bp, a));
						BIO_puts(bp, " % ");
						CHECK_GOTO(BN_print(bp, b[j]));
						BIO_puts(bp, "\n");
					}
				}
#endif
			}
		}
	}
	if (s == 0) {
		fprintf(stderr, "All %d tests of GF(2^m) modular solve quadratic resulted in no roots;\n", num0);
		fprintf(stderr, "this is very unlikely and probably indicates an error.\n");
		goto err;
	}
	rc = 1;
 err:
	BN_free(a);
	BN_free(b[0]);
	BN_free(b[1]);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return rc;
}
#endif
static int
genprime_cb(int p, int n, BN_GENCB *arg)
{
	char c = '*';

	if (p == 0)
		c = '.';
	if (p == 1)
		c = '+';
	if (p == 2)
		c = '*';
	if (p == 3)
		c = '\n';
	putc(c, stderr);
	return (1);
}

int
test_kron(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *r = NULL, *t = NULL;
	BN_GENCB *cb = NULL;
	int i;
	int legendre, kronecker;
	int rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((r = BN_new()) == NULL)
		goto err;
	if ((t = BN_new()) == NULL)
		goto err;

	if ((cb = BN_GENCB_new()) == NULL)
		goto err;

	BN_GENCB_set(cb, genprime_cb, NULL);

	/*
	 * We test BN_kronecker(a, b, ctx) just for b odd (Jacobi symbol). In
	 * this case we know that if b is prime, then BN_kronecker(a, b, ctx) is
	 * congruent to $a^{(b-1)/2}$, modulo $b$ (Legendre symbol). So we
	 * generate a random prime b and compare these values for a number of
	 * random a's.  (That is, we run the Solovay-Strassen primality test to
	 * confirm that b is prime, except that we don't want to test whether b
	 * is prime but whether BN_kronecker works.)
	 */

	if (!BN_generate_prime_ex(b, 512, 0, NULL, NULL, cb))
		goto err;
	BN_set_negative(b, rand_neg());
	putc('\n', stderr);

	for (i = 0; i < num0; i++) {
		if (!BN_bntest_rand(a, 512, 0, 0))
			goto err;
		BN_set_negative(a, rand_neg());

		/* t := (|b|-1)/2  (note that b is odd) */
		if (!BN_copy(t, b))
			goto err;
		BN_set_negative(t, 0);
		if (!BN_sub_word(t, 1))
			goto err;
		if (!BN_rshift1(t, t))
			goto err;
		/* r := a^t mod b */
		BN_set_negative(b, 0);

		if (!BN_mod_exp_recp(r, a, t, b, ctx))
			goto err;
		BN_set_negative(b, 1);

		if (BN_is_word(r, 1))
			legendre = 1;
		else if (BN_is_zero(r))
			legendre = 0;
		else {
			if (!BN_add_word(r, 1))
				goto err;
			if (0 != BN_ucmp(r, b)) {
				fprintf(stderr, "Legendre symbol computation failed\n");
				goto err;
			}
			legendre = -1;
		}

		kronecker = BN_kronecker(a, b, ctx);
		if (kronecker < -1)
			goto err;
		/* we actually need BN_kronecker(a, |b|) */
		if (BN_is_negative(a) && BN_is_negative(b))
			kronecker = -kronecker;

		if (legendre != kronecker) {
			fprintf(stderr, "legendre != kronecker; a = ");
			CHECK_GOTO(BN_print_fp(stderr, a));
			fprintf(stderr, ", b = ");
			CHECK_GOTO(BN_print_fp(stderr, b));
			fprintf(stderr, "\n");
			goto err;
		}

		putc('.', stderr);
	}

	putc('\n', stderr);
	rc = 1;

 err:
	BN_GENCB_free(cb);
	BN_free(a);
	BN_free(b);
	BN_free(r);
	BN_free(t);
	return rc;
}

int
test_sqrt(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *p = NULL, *r = NULL;
	BN_GENCB *cb = NULL;
	int i, j;
	int rc = 0;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((p = BN_new()) == NULL)
		goto err;
	if ((r = BN_new()) == NULL)
		goto err;

	if ((cb = BN_GENCB_new()) == NULL)
		goto err;

	BN_GENCB_set(cb, genprime_cb, NULL);

	for (i = 0; i < 16; i++) {
		if (i < 8) {
			unsigned primes[8] = { 2, 3, 5, 7, 11, 13, 17, 19 };

			if (!BN_set_word(p, primes[i]))
				goto err;
		} else {
			if (!BN_set_word(a, 32))
				goto err;
			if (!BN_set_word(r, 2 * i + 1))
				goto err;

			if (!BN_generate_prime_ex(p, 256, 0, a, r, cb))
				goto err;
			putc('\n', stderr);
		}
		BN_set_negative(p, rand_neg());

		for (j = 0; j < num2; j++) {
			/*
			 * construct 'a' such that it is a square modulo p, but in
			 * general not a proper square and not reduced modulo p
			 */
			if (!BN_bntest_rand(r, 256, 0, 3))
				goto err;
			if (!BN_nnmod(r, r, p, ctx))
				goto err;
			if (!BN_mod_sqr(r, r, p, ctx))
				goto err;
			if (!BN_bntest_rand(a, 256, 0, 3))
				goto err;
			if (!BN_nnmod(a, a, p, ctx))
				goto err;
			if (!BN_mod_sqr(a, a, p, ctx))
				goto err;
			if (!BN_mul(a, a, r, ctx))
				goto err;
			if (rand_neg())
				if (!BN_sub(a, a, p))
					goto err;

			if (!BN_mod_sqrt(r, a, p, ctx))
				goto err;
			if (!BN_mod_sqr(r, r, p, ctx))
				goto err;

			if (!BN_nnmod(a, a, p, ctx))
				goto err;

			if (BN_cmp(a, r) != 0) {
				fprintf(stderr, "BN_mod_sqrt failed: a = ");
				CHECK_GOTO(BN_print_fp(stderr, a));
				fprintf(stderr, ", r = ");
				CHECK_GOTO(BN_print_fp(stderr, r));
				fprintf(stderr, ", p = ");
				CHECK_GOTO(BN_print_fp(stderr, p));
				fprintf(stderr, "\n");
				goto err;
			}

			putc('.', stderr);
		}

		putc('\n', stderr);
	}
	rc = 1;

 err:
	BN_GENCB_free(cb);
	BN_free(a);
	BN_free(p);
	BN_free(r);
	return rc;
}

int
test_lshift(BIO *bp, BN_CTX *ctx, BIGNUM *a_)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL;
	int i;
	int rc = 1;

	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	CHECK_GOTO(BN_one(c));

	if (a_)
		a = a_;
	else {
		if ((a = BN_new()) == NULL)
		goto err;
		CHECK_GOTO(BN_bntest_rand(a, 200, 0, 0));
		BN_set_negative(a, rand_neg());
	}
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_lshift(b, a, i + 1));
		CHECK_GOTO(BN_add(c, c, c));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " * ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, b));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_mul(d, a, c, ctx));
		CHECK_GOTO(BN_sub(d, d, b));
		if (!BN_is_zero(d)) {
			fprintf(stderr, "Left shift test failed!\n");
			fprintf(stderr, "a=");
			CHECK_GOTO(BN_print_fp(stderr, a));
			fprintf(stderr, "\nb=");
			CHECK_GOTO(BN_print_fp(stderr, b));
			fprintf(stderr, "\nc=");
			CHECK_GOTO(BN_print_fp(stderr, c));
			fprintf(stderr, "\nd=");
			CHECK_GOTO(BN_print_fp(stderr, d));
			fprintf(stderr, "\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	return (rc);
}

int
test_lshift1(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_bntest_rand(a, 200, 0, 0));
	BN_set_negative(a, rand_neg());
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_lshift1(b, a));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " * 2");
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, b));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_add(c, a, a));
		CHECK_GOTO(BN_sub(a, b, c));
		if (!BN_is_zero(a)) {
			fprintf(stderr, "Left shift one test failed!\n");
			rc = 0;
			break;
		}

		CHECK_GOTO(BN_copy(a, b));
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	return (rc);
}

int
test_rshift(BIO *bp, BN_CTX *ctx)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL, *d = NULL, *e = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;
	if ((d = BN_new()) == NULL)
		goto err;
	if ((e = BN_new()) == NULL)
		goto err;
	CHECK_GOTO(BN_one(c));

	CHECK_GOTO(BN_bntest_rand(a, 200, 0, 0));
	BN_set_negative(a, rand_neg());
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_rshift(b, a, i + 1));
		CHECK_GOTO(BN_add(c, c, c));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " / ");
				CHECK_GOTO(BN_print(bp, c));
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, b));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_div(d, e, a, c, ctx));
		CHECK_GOTO(BN_sub(d, d, b));
		if (!BN_is_zero(d)) {
			fprintf(stderr, "Right shift test failed!\n");
			rc = 0;
			break;
		}
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	BN_free(d);
	BN_free(e);
	return (rc);
}

int
test_rshift1(BIO *bp)
{
	BIGNUM *a = NULL, *b = NULL, *c = NULL;
	int i;
	int rc = 1;

	if ((a = BN_new()) == NULL)
		goto err;
	if ((b = BN_new()) == NULL)
		goto err;
	if ((c = BN_new()) == NULL)
		goto err;

	CHECK_GOTO(BN_bntest_rand(a, 200, 0, 0));
	BN_set_negative(a, rand_neg());
	for (i = 0; i < num0; i++) {
		CHECK_GOTO(BN_rshift1(b, a));
		if (bp != NULL) {
			if (!results) {
				CHECK_GOTO(BN_print(bp, a));
				BIO_puts(bp, " / 2");
				BIO_puts(bp, " - ");
			}
			CHECK_GOTO(BN_print(bp, b));
			BIO_puts(bp, "\n");
		}
		CHECK_GOTO(BN_sub(c, a, b));
		CHECK_GOTO(BN_sub(c, c, b));
		if (!BN_is_zero(c) && !BN_abs_is_word(c, 1)) {
			fprintf(stderr, "Right shift one test failed!\n");
			rc = 0;
			break;
		}
		CHECK_GOTO(BN_copy(a, b));
	}
 err:
	BN_free(a);
	BN_free(b);
	BN_free(c);
	return (rc);
}

int
rand_neg(void)
{
	static unsigned int neg = 0;
	static int sign[8] = { 0, 0, 0, 1, 1, 0, 1, 1 };

	return (sign[(neg++) % 8]);
}

int
test_mod_exp_sizes(BIO *bp, BN_CTX *ctx)
{
	BN_MONT_CTX *mont_ctx = NULL;
	BIGNUM *p = NULL, *x = NULL, *y = NULL, *r = NULL, *r2 = NULL;
	int size;
	int rc = 0;

	BN_CTX_start(ctx);
	CHECK_GOTO(p = BN_CTX_get(ctx));
	CHECK_GOTO(x = BN_CTX_get(ctx));
	CHECK_GOTO(y = BN_CTX_get(ctx));
	CHECK_GOTO(r = BN_CTX_get(ctx));
	CHECK_GOTO(r2 = BN_CTX_get(ctx));
	mont_ctx = BN_MONT_CTX_new();

	if (r2 == NULL || mont_ctx == NULL)
		goto err;

	if (!BN_generate_prime_ex(p, 32, 0, NULL, NULL, NULL) ||
	    !BN_MONT_CTX_set(mont_ctx, p, ctx))
		goto err;

	for (size = 32; size < 1024; size += 8) {
		if (!BN_rand(x, size, -1, 0) ||
		    !BN_rand(y, size, -1, 0) ||
		    !BN_mod_exp_mont_consttime(r, x, y, p, ctx, mont_ctx) ||
		    !BN_mod_exp(r2, x, y, p, ctx))
			goto err;

		if (BN_cmp(r, r2) != 0) {
			char *r_str = NULL;
			char *r2_str = NULL;
			CHECK_GOTO(r_str = BN_bn2hex(r));
			CHECK_GOTO(r2_str = BN_bn2hex(r2));

			printf("Incorrect answer at size %d: %s vs %s\n",
			    size, r_str, r2_str);
			free(r_str);
			free(r2_str);
			goto err;
		}
	}

	rc = 1;

 err:
	BN_MONT_CTX_free(mont_ctx);
	BN_CTX_end(ctx);
	return rc;
}

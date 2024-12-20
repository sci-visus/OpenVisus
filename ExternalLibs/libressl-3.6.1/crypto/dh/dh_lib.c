/* $OpenBSD: dh_lib.c,v 1.37 2022/06/27 12:31:38 tb Exp $ */
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

#include <limits.h>
#include <stdio.h>

#include <openssl/opensslconf.h>

#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/err.h>

#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif

#include "dh_local.h"

static const DH_METHOD *default_DH_method = NULL;

void
DH_set_default_method(const DH_METHOD *meth)
{
	default_DH_method = meth;
}

const DH_METHOD *
DH_get_default_method(void)
{
	if (!default_DH_method)
		default_DH_method = DH_OpenSSL();
	return default_DH_method;
}

int
DH_set_method(DH *dh, const DH_METHOD *meth)
{
	/*
	 * NB: The caller is specifically setting a method, so it's not up to us
	 * to deal with which ENGINE it comes from.
	 */
        const DH_METHOD *mtmp;

        mtmp = dh->meth;
        if (mtmp->finish)
		mtmp->finish(dh);
#ifndef OPENSSL_NO_ENGINE
	ENGINE_finish(dh->engine);
	dh->engine = NULL;
#endif
        dh->meth = meth;
        if (meth->init)
		meth->init(dh);
        return 1;
}

DH *
DH_new(void)
{
	return DH_new_method(NULL);
}

DH *
DH_new_method(ENGINE *engine)
{
	DH *ret;

	ret = malloc(sizeof(DH));
	if (ret == NULL) {
		DHerror(ERR_R_MALLOC_FAILURE);
		return NULL;
	}

	ret->meth = DH_get_default_method();
#ifndef OPENSSL_NO_ENGINE
	if (engine) {
		if (!ENGINE_init(engine)) {
			DHerror(ERR_R_ENGINE_LIB);
			free(ret);
			return NULL;
		}
		ret->engine = engine;
	} else
		ret->engine = ENGINE_get_default_DH();
	if(ret->engine) {
		ret->meth = ENGINE_get_DH(ret->engine);
		if (ret->meth == NULL) {
			DHerror(ERR_R_ENGINE_LIB);
			ENGINE_finish(ret->engine);
			free(ret);
			return NULL;
		}
	}
#endif

	ret->pad = 0;
	ret->version = 0;
	ret->p = NULL;
	ret->g = NULL;
	ret->length = 0;
	ret->pub_key = NULL;
	ret->priv_key = NULL;
	ret->q = NULL;
	ret->j = NULL;
	ret->seed = NULL;
	ret->seedlen = 0;
	ret->counter = NULL;
	ret->method_mont_p=NULL;
	ret->references = 1;
	ret->flags = ret->meth->flags & ~DH_FLAG_NON_FIPS_ALLOW;
	CRYPTO_new_ex_data(CRYPTO_EX_INDEX_DH, ret, &ret->ex_data);
	if (ret->meth->init != NULL && !ret->meth->init(ret)) {
#ifndef OPENSSL_NO_ENGINE
		ENGINE_finish(ret->engine);
#endif
		CRYPTO_free_ex_data(CRYPTO_EX_INDEX_DH, ret, &ret->ex_data);
		free(ret);
		ret = NULL;
	}
	return ret;
}

void
DH_free(DH *r)
{
	int i;

	if (r == NULL)
		return;
	i = CRYPTO_add(&r->references, -1, CRYPTO_LOCK_DH);
	if (i > 0)
		return;

	if (r->meth->finish)
		r->meth->finish(r);
#ifndef OPENSSL_NO_ENGINE
	ENGINE_finish(r->engine);
#endif

	CRYPTO_free_ex_data(CRYPTO_EX_INDEX_DH, r, &r->ex_data);

	BN_clear_free(r->p);
	BN_clear_free(r->g);
	BN_clear_free(r->q);
	BN_clear_free(r->j);
	free(r->seed);
	BN_clear_free(r->counter);
	BN_clear_free(r->pub_key);
	BN_clear_free(r->priv_key);
	free(r);
}

int
DH_up_ref(DH *r)
{
	int i = CRYPTO_add(&r->references, 1, CRYPTO_LOCK_DH);

	return i > 1 ? 1 : 0;
}

int
DH_get_ex_new_index(long argl, void *argp, CRYPTO_EX_new *new_func,
    CRYPTO_EX_dup *dup_func, CRYPTO_EX_free *free_func)
{
	return CRYPTO_get_ex_new_index(CRYPTO_EX_INDEX_DH, argl, argp, new_func,
	    dup_func, free_func);
}

int
DH_set_ex_data(DH *d, int idx, void *arg)
{
	return CRYPTO_set_ex_data(&d->ex_data, idx, arg);
}

void *
DH_get_ex_data(DH *d, int idx)
{
	return CRYPTO_get_ex_data(&d->ex_data, idx);
}

int
DH_size(const DH *dh)
{
	return BN_num_bytes(dh->p);
}

int
DH_bits(const DH *dh)
{
	return BN_num_bits(dh->p);
}

int
DH_security_bits(const DH *dh)
{
	int N = -1;

	if (dh->q != NULL)
		N = BN_num_bits(dh->q);
	else if (dh->length > 0)
		N = dh->length;

	return BN_security_bits(BN_num_bits(dh->p), N);
}

ENGINE *
DH_get0_engine(DH *dh)
{
	return dh->engine;
}

void
DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g)
{
	if (p != NULL)
		*p = dh->p;
	if (q != NULL)
		*q = dh->q;
	if (g != NULL)
		*g = dh->g;
}

int
DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g)
{
	if ((dh->p == NULL && p == NULL) || (dh->g == NULL && g == NULL))
		return 0;

	if (p != NULL) {
		BN_free(dh->p);
		dh->p = p;
	}
	if (q != NULL) {
		BN_free(dh->q);
		dh->q = q;
		dh->length = BN_num_bits(dh->q);
	}
	if (g != NULL) {
		BN_free(dh->g);
		dh->g = g;
	}

	return 1;
}

void
DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key)
{
	if (pub_key != NULL)
		*pub_key = dh->pub_key;
	if (priv_key != NULL)
		*priv_key = dh->priv_key;
}

int
DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key)
{
	if (pub_key != NULL) {
		BN_free(dh->pub_key);
		dh->pub_key = pub_key;
	}
	if (priv_key != NULL) {
		BN_free(dh->priv_key);
		dh->priv_key = priv_key;
	}

	return 1;
}

const BIGNUM *
DH_get0_p(const DH *dh)
{
	return dh->p;
}

const BIGNUM *
DH_get0_q(const DH *dh)
{
	return dh->q;
}

const BIGNUM *
DH_get0_g(const DH *dh)
{
	return dh->g;
}

const BIGNUM *
DH_get0_priv_key(const DH *dh)
{
	return dh->priv_key;
}

const BIGNUM *
DH_get0_pub_key(const DH *dh)
{
	return dh->pub_key;
}

void
DH_clear_flags(DH *dh, int flags)
{
	dh->flags &= ~flags;
}

int
DH_test_flags(const DH *dh, int flags)
{
	return dh->flags & flags;
}

void
DH_set_flags(DH *dh, int flags)
{
	dh->flags |= flags;
}

long
DH_get_length(const DH *dh)
{
	return dh->length;
}

int
DH_set_length(DH *dh, long length)
{
	if (length < 0 || length > INT_MAX)
		return 0;

	dh->length = length;
	return 1;
}

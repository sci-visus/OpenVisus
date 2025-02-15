/* $OpenBSD: ec_lib.c,v 1.45 2022/04/07 17:37:25 tb Exp $ */
/*
 * Originally written by Bodo Moeller for the OpenSSL project.
 */
/* ====================================================================
 * Copyright (c) 1998-2003 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This product includes cryptographic software written by Eric Young
 * (eay@cryptsoft.com).  This product includes software written by Tim
 * Hudson (tjh@cryptsoft.com).
 *
 */
/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 * Binary polynomial ECC support in OpenSSL originally developed by
 * SUN MICROSYSTEMS, INC., and contributed to the OpenSSL project.
 */

#include <string.h>

#include <openssl/opensslconf.h>

#include <openssl/err.h>
#include <openssl/opensslv.h>

#include "bn_lcl.h"
#include "ec_lcl.h"

/* functions for EC_GROUP objects */

EC_GROUP *
EC_GROUP_new(const EC_METHOD * meth)
{
	EC_GROUP *ret;

	if (meth == NULL) {
		ECerror(EC_R_SLOT_FULL);
		return NULL;
	}
	if (meth->group_init == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return NULL;
	}
	ret = malloc(sizeof *ret);
	if (ret == NULL) {
		ECerror(ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	ret->meth = meth;

	ret->extra_data = NULL;

	ret->generator = NULL;
	BN_init(&ret->order);
	BN_init(&ret->cofactor);

	ret->curve_name = 0;
	ret->asn1_flag = OPENSSL_EC_NAMED_CURVE;
	ret->asn1_form = POINT_CONVERSION_UNCOMPRESSED;

	ret->seed = NULL;
	ret->seed_len = 0;

	if (!meth->group_init(ret)) {
		free(ret);
		return NULL;
	}
	return ret;
}


void
EC_GROUP_free(EC_GROUP * group)
{
	if (!group)
		return;

	if (group->meth->group_finish != 0)
		group->meth->group_finish(group);

	EC_EX_DATA_free_all_data(&group->extra_data);

	EC_POINT_free(group->generator);
	BN_free(&group->order);
	BN_free(&group->cofactor);

	free(group->seed);

	free(group);
}


void
EC_GROUP_clear_free(EC_GROUP * group)
{
	if (!group)
		return;

	if (group->meth->group_clear_finish != 0)
		group->meth->group_clear_finish(group);
	else if (group->meth->group_finish != 0)
		group->meth->group_finish(group);

	EC_EX_DATA_clear_free_all_data(&group->extra_data);

	EC_POINT_clear_free(group->generator);
	BN_clear_free(&group->order);
	BN_clear_free(&group->cofactor);

	freezero(group->seed, group->seed_len);
	freezero(group, sizeof *group);
}


int
EC_GROUP_copy(EC_GROUP * dest, const EC_GROUP * src)
{
	EC_EXTRA_DATA *d;

	if (dest->meth->group_copy == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (dest->meth != src->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	if (dest == src)
		return 1;

	EC_EX_DATA_free_all_data(&dest->extra_data);

	for (d = src->extra_data; d != NULL; d = d->next) {
		void *t = d->dup_func(d->data);

		if (t == NULL)
			return 0;
		if (!EC_EX_DATA_set_data(&dest->extra_data, t, d->dup_func,
		    d->free_func, d->clear_free_func))
			return 0;
	}

	if (src->generator != NULL) {
		if (dest->generator == NULL) {
			dest->generator = EC_POINT_new(dest);
			if (dest->generator == NULL)
				return 0;
		}
		if (!EC_POINT_copy(dest->generator, src->generator))
			return 0;
	} else {
		/* src->generator == NULL */
		EC_POINT_clear_free(dest->generator);
		dest->generator = NULL;
	}

	if (!BN_copy(&dest->order, &src->order))
		return 0;
	if (!BN_copy(&dest->cofactor, &src->cofactor))
		return 0;

	dest->curve_name = src->curve_name;
	dest->asn1_flag = src->asn1_flag;
	dest->asn1_form = src->asn1_form;

	if (src->seed) {
		free(dest->seed);
		dest->seed = malloc(src->seed_len);
		if (dest->seed == NULL)
			return 0;
		memcpy(dest->seed, src->seed, src->seed_len);
		dest->seed_len = src->seed_len;
	} else {
		free(dest->seed);
		dest->seed = NULL;
		dest->seed_len = 0;
	}


	return dest->meth->group_copy(dest, src);
}


EC_GROUP *
EC_GROUP_dup(const EC_GROUP * a)
{
	EC_GROUP *t = NULL;

	if ((a != NULL) && ((t = EC_GROUP_new(a->meth)) != NULL) &&
	    (!EC_GROUP_copy(t, a))) {
		EC_GROUP_free(t);
		t = NULL;
	}
	return t;
}


const EC_METHOD *
EC_GROUP_method_of(const EC_GROUP *group)
{
	return group->meth;
}


int
EC_METHOD_get_field_type(const EC_METHOD *meth)
{
	return meth->field_type;
}

/*
 * Try computing the cofactor from generator order n and field cardinality q.
 * This works for all curves of cryptographic interest.
 *
 * Hasse's theorem: | h * n - (q + 1) | <= 2 * sqrt(q)
 *
 * So: h_min = (q + 1 - 2*sqrt(q)) / n and h_max = (q + 1 + 2*sqrt(q)) / n and
 * therefore h_max - h_min = 4*sqrt(q) / n. So if n > 4*sqrt(q) holds, there is
 * only one possible value for h:
 *
 *	h = \lfloor (h_min + h_max)/2 \rceil = \lfloor (q + 1)/n \rceil
 *
 * Otherwise, zero cofactor and return success.
 */
static int
ec_guess_cofactor(EC_GROUP *group)
{
	BN_CTX *ctx = NULL;
	BIGNUM *q = NULL;
	int ret = 0;

	/*
	 * If the cofactor is too large, we cannot guess it and default to zero.
	 * The RHS of below is a strict overestimate of log(4 * sqrt(q)).
	 */
	if (BN_num_bits(&group->order) <=
	    (BN_num_bits(&group->field) + 1) / 2 + 3) {
		BN_zero(&group->cofactor);
		return 1;
	}

	if ((ctx = BN_CTX_new()) == NULL)
		goto err;

	BN_CTX_start(ctx);
	if ((q = BN_CTX_get(ctx)) == NULL)
		goto err;

	/* Set q = 2**m for binary fields; q = p otherwise. */
	if (group->meth->field_type == NID_X9_62_characteristic_two_field) {
		BN_zero(q);
		if (!BN_set_bit(q, BN_num_bits(&group->field) - 1))
			goto err;
	} else {
		if (!BN_copy(q, &group->field))
			goto err;
	}

	/*
	 * Compute
	 *     h = \lfloor (q + 1)/n \rceil = \lfloor (q + 1 + n/2) / n \rfloor.
	 */

	/* h = n/2 */
	if (!BN_rshift1(&group->cofactor, &group->order))
		goto err;
	/* h = 1 + n/2 */
	if (!BN_add(&group->cofactor, &group->cofactor, BN_value_one()))
		goto err;
	/* h = q + 1 + n/2 */
	if (!BN_add(&group->cofactor, &group->cofactor, q))
		goto err;
	/* h = (q + 1 + n/2) / n */
	if (!BN_div_ct(&group->cofactor, NULL, &group->cofactor, &group->order,
	    ctx))
		goto err;

	ret = 1;

 err:
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);

	if (ret != 1)
		BN_zero(&group->cofactor);

	return ret;
}

int
EC_GROUP_set_generator(EC_GROUP *group, const EC_POINT *generator,
    const BIGNUM *order, const BIGNUM *cofactor)
{
	if (generator == NULL) {
		ECerror(ERR_R_PASSED_NULL_PARAMETER);
		return 0;
	}

	/* Require group->field >= 1. */
	if (BN_is_zero(&group->field) || BN_is_negative(&group->field)) {
		ECerror(EC_R_INVALID_FIELD);
		return 0;
	}

	/*
	 * Require order > 1 and enforce an upper bound of at most one bit more
	 * than the field cardinality due to Hasse's theorem.
	 */
	if (order == NULL || BN_cmp(order, BN_value_one()) <= 0 ||
	    BN_num_bits(order) > BN_num_bits(&group->field) + 1) {
		ECerror(EC_R_INVALID_GROUP_ORDER);
		return 0;
	}

	/*
	 * Unfortunately, the cofactor is an optional field in many standards.
	 * Internally, the library uses a 0 cofactor as a marker for "unknown
	 * cofactor".  So accept cofactor == NULL or cofactor >= 0.
	 */
	if (cofactor != NULL && BN_is_negative(cofactor)) {
		ECerror(EC_R_UNKNOWN_COFACTOR);
		return 0;
	}

	if (group->generator == NULL) {
		group->generator = EC_POINT_new(group);
		if (group->generator == NULL)
			return 0;
	}
	if (!EC_POINT_copy(group->generator, generator))
		return 0;

	if (!BN_copy(&group->order, order))
		return 0;

	/* Either take the provided positive cofactor, or try to compute it. */
	if (cofactor != NULL && !BN_is_zero(cofactor)) {
		if (!BN_copy(&group->cofactor, cofactor))
			return 0;
	} else if (!ec_guess_cofactor(group))
		return 0;

	/* Use Hasse's theorem to bound the cofactor. */
	if (BN_num_bits(&group->cofactor) > BN_num_bits(&group->field) + 1) {
		ECerror(EC_R_INVALID_GROUP_ORDER);
		return 0;
	}

	return 1;
}


const EC_POINT *
EC_GROUP_get0_generator(const EC_GROUP *group)
{
	return group->generator;
}


int
EC_GROUP_get_order(const EC_GROUP *group, BIGNUM *order, BN_CTX *ctx)
{
	if (!BN_copy(order, &group->order))
		return 0;

	return !BN_is_zero(order);
}

int
EC_GROUP_order_bits(const EC_GROUP *group)
{
	return group->meth->group_order_bits(group);
}

int
EC_GROUP_get_cofactor(const EC_GROUP *group, BIGNUM *cofactor, BN_CTX *ctx)
{
	if (!BN_copy(cofactor, &group->cofactor))
		return 0;

	return !BN_is_zero(&group->cofactor);
}


void
EC_GROUP_set_curve_name(EC_GROUP * group, int nid)
{
	group->curve_name = nid;
}


int
EC_GROUP_get_curve_name(const EC_GROUP * group)
{
	return group->curve_name;
}


void
EC_GROUP_set_asn1_flag(EC_GROUP * group, int flag)
{
	group->asn1_flag = flag;
}


int
EC_GROUP_get_asn1_flag(const EC_GROUP * group)
{
	return group->asn1_flag;
}


void
EC_GROUP_set_point_conversion_form(EC_GROUP * group,
    point_conversion_form_t form)
{
	group->asn1_form = form;
}


point_conversion_form_t
EC_GROUP_get_point_conversion_form(const EC_GROUP * group)
{
	return group->asn1_form;
}


size_t
EC_GROUP_set_seed(EC_GROUP * group, const unsigned char *p, size_t len)
{
	if (group->seed) {
		free(group->seed);
		group->seed = NULL;
		group->seed_len = 0;
	}
	if (!len || !p)
		return 1;

	if ((group->seed = malloc(len)) == NULL)
		return 0;
	memcpy(group->seed, p, len);
	group->seed_len = len;

	return len;
}


unsigned char *
EC_GROUP_get0_seed(const EC_GROUP * group)
{
	return group->seed;
}


size_t
EC_GROUP_get_seed_len(const EC_GROUP * group)
{
	return group->seed_len;
}

int
EC_GROUP_set_curve(EC_GROUP *group, const BIGNUM *p, const BIGNUM *a,
    const BIGNUM *b, BN_CTX *ctx)
{
	if (group->meth->group_set_curve == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	return group->meth->group_set_curve(group, p, a, b, ctx);
}

int
EC_GROUP_get_curve(const EC_GROUP *group, BIGNUM *p, BIGNUM *a, BIGNUM *b,
    BN_CTX *ctx)
{
	if (group->meth->group_get_curve == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	return group->meth->group_get_curve(group, p, a, b, ctx);
}

int
EC_GROUP_set_curve_GFp(EC_GROUP *group, const BIGNUM *p, const BIGNUM *a,
    const BIGNUM *b, BN_CTX *ctx)
{
	return EC_GROUP_set_curve(group, p, a, b, ctx);
}

int
EC_GROUP_get_curve_GFp(const EC_GROUP *group, BIGNUM *p, BIGNUM *a, BIGNUM *b,
    BN_CTX *ctx)
{
	return EC_GROUP_get_curve(group, p, a, b, ctx);
}

#ifndef OPENSSL_NO_EC2M
int
EC_GROUP_set_curve_GF2m(EC_GROUP *group, const BIGNUM *p, const BIGNUM *a,
    const BIGNUM *b, BN_CTX *ctx)
{
	return EC_GROUP_set_curve(group, p, a, b, ctx);
}

int
EC_GROUP_get_curve_GF2m(const EC_GROUP *group, BIGNUM *p, BIGNUM *a,
    BIGNUM *b, BN_CTX *ctx)
{
	return EC_GROUP_get_curve(group, p, a, b, ctx);
}
#endif

int
EC_GROUP_get_degree(const EC_GROUP * group)
{
	if (group->meth->group_get_degree == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	return group->meth->group_get_degree(group);
}


int
EC_GROUP_check_discriminant(const EC_GROUP * group, BN_CTX * ctx)
{
	if (group->meth->group_check_discriminant == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	return group->meth->group_check_discriminant(group, ctx);
}


int
EC_GROUP_cmp(const EC_GROUP * a, const EC_GROUP * b, BN_CTX * ctx)
{
	int r = 0;
	BIGNUM *a1, *a2, *a3, *b1, *b2, *b3;
	BN_CTX *ctx_new = NULL;

	/* compare the field types */
	if (EC_METHOD_get_field_type(EC_GROUP_method_of(a)) !=
	    EC_METHOD_get_field_type(EC_GROUP_method_of(b)))
		return 1;
	/* compare the curve name (if present in both) */
	if (EC_GROUP_get_curve_name(a) && EC_GROUP_get_curve_name(b) &&
	    EC_GROUP_get_curve_name(a) != EC_GROUP_get_curve_name(b))
		return 1;

	if (!ctx)
		ctx_new = ctx = BN_CTX_new();
	if (!ctx)
		return -1;

	BN_CTX_start(ctx);
	if ((a1 = BN_CTX_get(ctx)) == NULL)
		goto err;
	if ((a2 = BN_CTX_get(ctx)) == NULL)
		goto err;
	if ((a3 = BN_CTX_get(ctx)) == NULL)
		goto err;
	if ((b1 = BN_CTX_get(ctx)) == NULL)
		goto err;
	if ((b2 = BN_CTX_get(ctx)) == NULL)
		goto err;
	if ((b3 = BN_CTX_get(ctx)) == NULL)
		goto err;

	/*
	 * XXX This approach assumes that the external representation of
	 * curves over the same field type is the same.
	 */
	if (!a->meth->group_get_curve(a, a1, a2, a3, ctx) ||
	    !b->meth->group_get_curve(b, b1, b2, b3, ctx))
		r = 1;

	if (r || BN_cmp(a1, b1) || BN_cmp(a2, b2) || BN_cmp(a3, b3))
		r = 1;

	/* XXX EC_POINT_cmp() assumes that the methods are equal */
	if (r || EC_POINT_cmp(a, EC_GROUP_get0_generator(a),
		EC_GROUP_get0_generator(b), ctx))
		r = 1;

	if (!r) {
		/* compare the order and cofactor */
		if (!EC_GROUP_get_order(a, a1, ctx) ||
		    !EC_GROUP_get_order(b, b1, ctx) ||
		    !EC_GROUP_get_cofactor(a, a2, ctx) ||
		    !EC_GROUP_get_cofactor(b, b2, ctx))
			goto err;
		if (BN_cmp(a1, b1) || BN_cmp(a2, b2))
			r = 1;
	}
	BN_CTX_end(ctx);
	if (ctx_new)
		BN_CTX_free(ctx);

	return r;

 err:
	BN_CTX_end(ctx);
	if (ctx_new)
		BN_CTX_free(ctx);
	return -1;
}

/*
 * Coordinate blinding for EC_POINT.
 *
 * The underlying EC_METHOD can optionally implement this function:
 * underlying implementations should return 0 on errors, or 1 on success.
 *
 * This wrapper returns 1 in case the underlying EC_METHOD does not support
 * coordinate blinding.
 */
int
ec_point_blind_coordinates(const EC_GROUP *group, EC_POINT *p, BN_CTX *ctx)
{
	if (group->meth->blind_coordinates == NULL)
		return 1;

	return group->meth->blind_coordinates(group, p, ctx);
}

/* this has 'package' visibility */
int
EC_EX_DATA_set_data(EC_EXTRA_DATA ** ex_data, void *data,
    void *(*dup_func) (void *),
    void (*free_func) (void *),
    void (*clear_free_func) (void *))
{
	EC_EXTRA_DATA *d;

	if (ex_data == NULL)
		return 0;

	for (d = *ex_data; d != NULL; d = d->next) {
		if (d->dup_func == dup_func && d->free_func == free_func &&
		    d->clear_free_func == clear_free_func) {
			ECerror(EC_R_SLOT_FULL);
			return 0;
		}
	}

	if (data == NULL)
		/* no explicit entry needed */
		return 1;

	d = malloc(sizeof *d);
	if (d == NULL)
		return 0;

	d->data = data;
	d->dup_func = dup_func;
	d->free_func = free_func;
	d->clear_free_func = clear_free_func;

	d->next = *ex_data;
	*ex_data = d;

	return 1;
}

/* this has 'package' visibility */
void *
EC_EX_DATA_get_data(const EC_EXTRA_DATA * ex_data,
    void *(*dup_func) (void *),
    void (*free_func) (void *),
    void (*clear_free_func) (void *))
{
	const EC_EXTRA_DATA *d;

	for (d = ex_data; d != NULL; d = d->next) {
		if (d->dup_func == dup_func && d->free_func == free_func && d->clear_free_func == clear_free_func)
			return d->data;
	}

	return NULL;
}

/* this has 'package' visibility */
void
EC_EX_DATA_free_data(EC_EXTRA_DATA ** ex_data,
    void *(*dup_func) (void *),
    void (*free_func) (void *),
    void (*clear_free_func) (void *))
{
	EC_EXTRA_DATA **p;

	if (ex_data == NULL)
		return;

	for (p = ex_data; *p != NULL; p = &((*p)->next)) {
		if ((*p)->dup_func == dup_func &&
		    (*p)->free_func == free_func &&
		    (*p)->clear_free_func == clear_free_func) {
			EC_EXTRA_DATA *next = (*p)->next;

			(*p)->free_func((*p)->data);
			free(*p);

			*p = next;
			return;
		}
	}
}

/* this has 'package' visibility */
void
EC_EX_DATA_clear_free_data(EC_EXTRA_DATA ** ex_data,
    void *(*dup_func) (void *),
    void (*free_func) (void *),
    void (*clear_free_func) (void *))
{
	EC_EXTRA_DATA **p;

	if (ex_data == NULL)
		return;

	for (p = ex_data; *p != NULL; p = &((*p)->next)) {
		if ((*p)->dup_func == dup_func &&
		    (*p)->free_func == free_func &&
		    (*p)->clear_free_func == clear_free_func) {
			EC_EXTRA_DATA *next = (*p)->next;

			(*p)->clear_free_func((*p)->data);
			free(*p);

			*p = next;
			return;
		}
	}
}

/* this has 'package' visibility */
void
EC_EX_DATA_free_all_data(EC_EXTRA_DATA ** ex_data)
{
	EC_EXTRA_DATA *d;

	if (ex_data == NULL)
		return;

	d = *ex_data;
	while (d) {
		EC_EXTRA_DATA *next = d->next;

		d->free_func(d->data);
		free(d);

		d = next;
	}
	*ex_data = NULL;
}

/* this has 'package' visibility */
void
EC_EX_DATA_clear_free_all_data(EC_EXTRA_DATA ** ex_data)
{
	EC_EXTRA_DATA *d;

	if (ex_data == NULL)
		return;

	d = *ex_data;
	while (d) {
		EC_EXTRA_DATA *next = d->next;

		d->clear_free_func(d->data);
		free(d);

		d = next;
	}
	*ex_data = NULL;
}


/* functions for EC_POINT objects */

EC_POINT *
EC_POINT_new(const EC_GROUP * group)
{
	EC_POINT *ret;

	if (group == NULL) {
		ECerror(ERR_R_PASSED_NULL_PARAMETER);
		return NULL;
	}
	if (group->meth->point_init == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return NULL;
	}
	ret = malloc(sizeof *ret);
	if (ret == NULL) {
		ECerror(ERR_R_MALLOC_FAILURE);
		return NULL;
	}
	ret->meth = group->meth;

	if (!ret->meth->point_init(ret)) {
		free(ret);
		return NULL;
	}
	return ret;
}


void
EC_POINT_free(EC_POINT * point)
{
	if (!point)
		return;

	if (point->meth->point_finish != 0)
		point->meth->point_finish(point);
	free(point);
}


void
EC_POINT_clear_free(EC_POINT * point)
{
	if (!point)
		return;

	if (point->meth->point_clear_finish != 0)
		point->meth->point_clear_finish(point);
	else if (point->meth->point_finish != 0)
		point->meth->point_finish(point);
	freezero(point, sizeof *point);
}


int
EC_POINT_copy(EC_POINT * dest, const EC_POINT * src)
{
	if (dest->meth->point_copy == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (dest->meth != src->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	if (dest == src)
		return 1;
	return dest->meth->point_copy(dest, src);
}


EC_POINT *
EC_POINT_dup(const EC_POINT * a, const EC_GROUP * group)
{
	EC_POINT *t;
	int r;

	if (a == NULL)
		return NULL;

	t = EC_POINT_new(group);
	if (t == NULL)
		return (NULL);
	r = EC_POINT_copy(t, a);
	if (!r) {
		EC_POINT_free(t);
		return NULL;
	} else
		return t;
}


const EC_METHOD *
EC_POINT_method_of(const EC_POINT * point)
{
	return point->meth;
}


int
EC_POINT_set_to_infinity(const EC_GROUP * group, EC_POINT * point)
{
	if (group->meth->point_set_to_infinity == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->point_set_to_infinity(group, point);
}

int
EC_POINT_set_Jprojective_coordinates(const EC_GROUP *group, EC_POINT *point,
    const BIGNUM *x, const BIGNUM *y, const BIGNUM *z, BN_CTX *ctx)
{
	if (group->meth->point_set_Jprojective_coordinates == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->point_set_Jprojective_coordinates(group, point,
	    x, y, z, ctx);
}

int
EC_POINT_get_Jprojective_coordinates(const EC_GROUP *group,
    const EC_POINT *point, BIGNUM *x, BIGNUM *y, BIGNUM *z, BN_CTX *ctx)
{
	if (group->meth->point_get_Jprojective_coordinates == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->point_get_Jprojective_coordinates(group, point,
	    x, y, z, ctx);
}

int
EC_POINT_set_Jprojective_coordinates_GFp(const EC_GROUP *group, EC_POINT *point,
    const BIGNUM *x, const BIGNUM *y, const BIGNUM *z, BN_CTX *ctx)
{
	return EC_POINT_set_Jprojective_coordinates(group, point, x, y, z, ctx);
}

int
EC_POINT_get_Jprojective_coordinates_GFp(const EC_GROUP *group,
    const EC_POINT *point, BIGNUM *x, BIGNUM *y, BIGNUM *z, BN_CTX *ctx)
{
	return EC_POINT_get_Jprojective_coordinates(group, point, x, y, z, ctx);
}

int
EC_POINT_set_affine_coordinates(const EC_GROUP *group, EC_POINT *point,
    const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx)
{
	if (group->meth->point_set_affine_coordinates == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	if (!group->meth->point_set_affine_coordinates(group, point, x, y, ctx))
		return 0;
	if (EC_POINT_is_on_curve(group, point, ctx) <= 0) {
		ECerror(EC_R_POINT_IS_NOT_ON_CURVE);
		return 0;
	}
	return 1;
}

int
EC_POINT_set_affine_coordinates_GFp(const EC_GROUP *group, EC_POINT *point,
    const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx)
{
	return EC_POINT_set_affine_coordinates(group, point, x, y, ctx);
}

#ifndef OPENSSL_NO_EC2M
int
EC_POINT_set_affine_coordinates_GF2m(const EC_GROUP *group, EC_POINT *point,
    const BIGNUM *x, const BIGNUM *y, BN_CTX *ctx)
{
	return EC_POINT_set_affine_coordinates(group, point, x, y, ctx);
}
#endif

int
EC_POINT_get_affine_coordinates(const EC_GROUP *group, const EC_POINT *point,
    BIGNUM *x, BIGNUM *y, BN_CTX *ctx)
{
	if (group->meth->point_get_affine_coordinates == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->point_get_affine_coordinates(group, point, x, y, ctx);
}

int
EC_POINT_get_affine_coordinates_GFp(const EC_GROUP *group, const EC_POINT *point,
    BIGNUM *x, BIGNUM *y, BN_CTX *ctx)
{
	return EC_POINT_get_affine_coordinates(group, point, x, y, ctx);
}

#ifndef OPENSSL_NO_EC2M
int
EC_POINT_get_affine_coordinates_GF2m(const EC_GROUP *group, const EC_POINT *point,
    BIGNUM *x, BIGNUM *y, BN_CTX *ctx)
{
	return EC_POINT_get_affine_coordinates(group, point, x, y, ctx);
}
#endif

int
EC_POINT_add(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a,
    const EC_POINT *b, BN_CTX *ctx)
{
	if (group->meth->add == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if ((group->meth != r->meth) || (r->meth != a->meth) || (a->meth != b->meth)) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->add(group, r, a, b, ctx);
}


int
EC_POINT_dbl(const EC_GROUP *group, EC_POINT *r, const EC_POINT *a, BN_CTX *ctx)
{
	if (group->meth->dbl == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if ((group->meth != r->meth) || (r->meth != a->meth)) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->dbl(group, r, a, ctx);
}


int
EC_POINT_invert(const EC_GROUP *group, EC_POINT *a, BN_CTX *ctx)
{
	if (group->meth->invert == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != a->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->invert(group, a, ctx);
}


int
EC_POINT_is_at_infinity(const EC_GROUP *group, const EC_POINT *point)
{
	if (group->meth->is_at_infinity == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->is_at_infinity(group, point);
}


int
EC_POINT_is_on_curve(const EC_GROUP * group, const EC_POINT * point, BN_CTX * ctx)
{
	if (group->meth->is_on_curve == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->is_on_curve(group, point, ctx);
}


int
EC_POINT_cmp(const EC_GROUP *group, const EC_POINT *a, const EC_POINT *b,
    BN_CTX * ctx)
{
	if (group->meth->point_cmp == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return -1;
	}
	if ((group->meth != a->meth) || (a->meth != b->meth)) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return -1;
	}
	return group->meth->point_cmp(group, a, b, ctx);
}


int
EC_POINT_make_affine(const EC_GROUP *group, EC_POINT *point, BN_CTX *ctx)
{
	if (group->meth->make_affine == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (group->meth != point->meth) {
		ECerror(EC_R_INCOMPATIBLE_OBJECTS);
		return 0;
	}
	return group->meth->make_affine(group, point, ctx);
}


int
EC_POINTs_make_affine(const EC_GROUP *group, size_t num, EC_POINT *points[],
    BN_CTX *ctx)
{
	size_t i;

	if (group->meth->points_make_affine == 0) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	for (i = 0; i < num; i++) {
		if (group->meth != points[i]->meth) {
			ECerror(EC_R_INCOMPATIBLE_OBJECTS);
			return 0;
		}
	}
	return group->meth->points_make_affine(group, num, points, ctx);
}


/* Functions for point multiplication */
int
EC_POINTs_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *scalar,
    size_t num, const EC_POINT *points[], const BIGNUM *scalars[], BN_CTX *ctx)
{
	/*
	 * The function pointers must be set, and only support num == 0 and
	 * num == 1.
	 */
	if (group->meth->mul_generator_ct == NULL ||
	    group->meth->mul_single_ct == NULL ||
	    group->meth->mul_double_nonct == NULL ||
	    num > 1) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}

	/* Either bP or aG + bP, this is sane. */
	if (num == 1 && points != NULL && scalars != NULL)
		return EC_POINT_mul(group, r, scalar, points[0], scalars[0],
		    ctx);

	/* aG, this is sane */
	if (scalar != NULL && points == NULL && scalars == NULL)
		return EC_POINT_mul(group, r, scalar, NULL, NULL, ctx);

	/* anything else is an error */
	ECerror(ERR_R_EC_LIB);
	return 0;
}

int
EC_POINT_mul(const EC_GROUP *group, EC_POINT *r, const BIGNUM *g_scalar,
    const EC_POINT *point, const BIGNUM *p_scalar, BN_CTX *ctx)
{
	if (group->meth->mul_generator_ct == NULL ||
	    group->meth->mul_single_ct == NULL ||
	    group->meth->mul_double_nonct == NULL) {
		ECerror(ERR_R_SHOULD_NOT_HAVE_BEEN_CALLED);
		return 0;
	}
	if (g_scalar != NULL && point == NULL && p_scalar == NULL) {
		/*
		 * In this case we want to compute g_scalar * GeneratorPoint:
		 * this codepath is reached most prominently by (ephemeral) key
		 * generation of EC cryptosystems (i.e. ECDSA keygen and sign
		 * setup, ECDH keygen/first half), where the scalar is always
		 * secret. This is why we ignore if BN_FLG_CONSTTIME is actually
		 * set and we always call the constant time version.
		 */
		return group->meth->mul_generator_ct(group, r, g_scalar, ctx);
	}
	if (g_scalar == NULL && point != NULL && p_scalar != NULL) {
		/* In this case we want to compute p_scalar * GenericPoint:
		 * this codepath is reached most prominently by the second half
		 * of ECDH, where the secret scalar is multiplied by the peer's
		 * public point. To protect the secret scalar, we ignore if
		 * BN_FLG_CONSTTIME is actually set and we always call the
		 * constant time version.
		 */
		return group->meth->mul_single_ct(group, r, p_scalar, point,
		    ctx);
	}
	if (g_scalar != NULL && point != NULL && p_scalar != NULL) {
		/*
		 * In this case we want to compute
		 *   g_scalar * GeneratorPoint + p_scalar * GenericPoint:
		 * this codepath is reached most prominently by ECDSA signature
		 * verification. So we call the non-ct version.
		 */
		return group->meth->mul_double_nonct(group, r, g_scalar,
		    p_scalar, point, ctx);
	}

	/* Anything else is an error. */
	ECerror(ERR_R_EC_LIB);
	return 0;
}

int
EC_GROUP_precompute_mult(EC_GROUP * group, BN_CTX * ctx)
{
	if (group->meth->precompute_mult != 0)
		return group->meth->precompute_mult(group, ctx);
	else
		return 1;	/* nothing to do, so report success */
}

int
EC_GROUP_have_precompute_mult(const EC_GROUP * group)
{
	if (group->meth->have_precompute_mult != 0)
		return group->meth->have_precompute_mult(group);
	else
		return 0;	/* cannot tell whether precomputation has
				 * been performed */
}

int
ec_group_simple_order_bits(const EC_GROUP *group)
{
	/* XXX change group->order to a pointer? */
#if 0
	if (group->order == NULL)
		return 0;
#endif
	return BN_num_bits(&group->order);
}

EC_KEY *
ECParameters_dup(EC_KEY *key)
{
	unsigned char *p = NULL;
	EC_KEY *k = NULL;
	int len;

	if (key == NULL)
		return (NULL);

	if ((len = i2d_ECParameters(key, &p)) > 0)
		k = d2i_ECParameters(NULL, (const unsigned char **)&p, len);

	return (k);
}

/* crypto/pqueue/pq_test.c */
/*
 * DTLS implementation written by Nagendra Modadugu
 * (nagendra@cs.stanford.edu) for the OpenSSL project 2005.
 */
/* ====================================================================
 * Copyright (c) 1999-2005 The OpenSSL Project.  All rights reserved.
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
 *    for use in the OpenSSL Toolkit. (http://www.OpenSSL.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@OpenSSL.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.OpenSSL.org/)"
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pqueue.h"

/* remember to change expected.txt if you change these values */
unsigned char prio1[8] = "supercal";
unsigned char prio2[8] = "ifragili";
unsigned char prio3[8] = "sticexpi";

static void
pqueue_print(pqueue pq)
{
	pitem *iter, *item;

	iter = pqueue_iterator(pq);
	for (item = pqueue_next(&iter); item != NULL;
	    item = pqueue_next(&iter)) {
		printf("item\t%02x%02x%02x%02x%02x%02x%02x%02x\n",
		    item->priority[0], item->priority[1],
		    item->priority[2], item->priority[3],
		    item->priority[4], item->priority[5],
		    item->priority[6], item->priority[7]);
	}
}

int
main(void)
{
	pitem *item;
	pqueue pq;

	pq = pqueue_new();

	item = pitem_new(prio3, NULL);
	pqueue_insert(pq, item);

	item = pitem_new(prio1, NULL);
	pqueue_insert(pq, item);

	item = pitem_new(prio2, NULL);
	pqueue_insert(pq, item);

	item = pqueue_find(pq, prio1);
	fprintf(stderr, "found %p\n", item->priority);

	item = pqueue_find(pq, prio2);
	fprintf(stderr, "found %p\n", item->priority);

	item = pqueue_find(pq, prio3);
	fprintf(stderr, "found %p\n", item ? item->priority: 0);

	pqueue_print(pq);

	for (item = pqueue_pop(pq); item != NULL; item = pqueue_pop(pq))
		pitem_free(item);

	pqueue_free(pq);
	return 0;
}

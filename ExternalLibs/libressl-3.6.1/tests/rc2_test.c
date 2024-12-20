/*	$OpenBSD: rc2_test.c,v 1.5 2022/09/12 13:11:36 tb Exp $ */
/*
 * Copyright (c) 2022 Joshua Sing <joshua@hypera.dev>
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

#include <openssl/evp.h>
#include <openssl/rc2.h>

#include <stdint.h>
#include <string.h>

struct rc2_test {
	const int mode;
	const uint8_t key[64];
	const int key_len;
	const int key_bits;
	const uint8_t iv[64];
	const int iv_len;
	const uint8_t in[64];
	const int in_len;
	const uint8_t out[64];
	const int out_len;
	const int padding;
};

static const struct rc2_test rc2_tests[] = {
	/* ECB (Test vectors from RFC 2268) */
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 8,
		.key_bits = 63,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0xeb, 0xb7, 0x73, 0xf9, 0x93, 0x27, 0x8e, 0xff,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		},
		.key_len = 8,
		.key_bits = 64,
		.in = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		},
		.in_len = 8,
		.out = {
			0x27, 0x8b, 0x27, 0xe4, 0x2e, 0x2f, 0x0d, 0x49,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 8,
		.key_bits = 64,
		.in = {
			0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		},
		.in_len = 8,
		.out = {
			0x30, 0x64, 0x9e, 0xdf, 0x9b, 0xe7, 0xd2, 0xc2,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x88,
		},
		.key_len = 1,
		.key_bits = 64,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x61, 0xa8, 0xa2, 0x44, 0xad, 0xac, 0xcc, 0xf0,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x88, 0xbc, 0xa9, 0x0e, 0x90, 0x87, 0x5a,
		},
		.key_len = 7,
		.key_bits = 64,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x6c, 0xcf, 0x43, 0x08, 0x97, 0x4c, 0x26, 0x7f,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x88, 0xbc, 0xa9, 0x0e, 0x90, 0x87, 0x5a, 0x7f,
			0x0f, 0x79, 0xc3, 0x84, 0x62, 0x7b, 0xaf, 0xb2,
		},
		.key_len = 16,
		.key_bits = 64,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x1a, 0x80, 0x7d, 0x27, 0x2b, 0xbe, 0x5d, 0xb1,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x88, 0xbc, 0xa9, 0x0e, 0x90, 0x87, 0x5a, 0x7f,
			0x0f, 0x79, 0xc3, 0x84, 0x62, 0x7b, 0xaf, 0xb2,
		},
		.key_len = 16,
		.key_bits = 128,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x22, 0x69, 0x55, 0x2a, 0xb0, 0xf8, 0x5c, 0xa6,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x88, 0xbc, 0xa9, 0x0e, 0x90, 0x87, 0x5a, 0x7f,
			0x0f, 0x79, 0xc3, 0x84, 0x62, 0x7b, 0xaf, 0xb2,
			0x16, 0xf8, 0x0a, 0x6f, 0x85, 0x92, 0x05, 0x84,
			0xc4, 0x2f, 0xce, 0xb0, 0xbe, 0x25, 0x5d, 0xaf,
			0x1e,
		},
		.key_len = 33,
		.key_bits = 129,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x5b, 0x78, 0xd3, 0xa4, 0x3d, 0xff, 0xf1, 0xf1,
		},
		.out_len = 8,
	},

	/* ECB (Test vectors from http://websites.umich.edu/~x509/ssleay/rrc2.html) */
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 16,
		.key_bits = 1024,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x1c, 0x19, 0x8a, 0x83, 0x8d, 0xf0, 0x28, 0xb7,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
		},
		.key_len = 16,
		.key_bits = 1024,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x21, 0x82, 0x9C, 0x78, 0xA9, 0xF9, 0xC0, 0x74,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 16,
		.key_bits = 1024,
		.in = {
			0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		},
		.in_len = 8,
		.out = {
			0x13, 0xdb, 0x35, 0x17, 0xd3, 0x21, 0x86, 0x9e,
		},
		.out_len = 8,
	},
	{
		.mode = NID_rc2_ecb,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 1024,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 8,
		.out = {
			0x50, 0xdc, 0x01, 0x62, 0xbd, 0x75, 0x7f, 0x31,
		},
		.out_len = 8,
	},

	/* CBC (generated using https://github.com/joshuasing/libressl-test-gen) */
	{
		.mode = NID_rc2_cbc,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 8,
		.key_bits = 64,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0xeb, 0xb7, 0x73, 0xf9, 0x93, 0x27, 0x8e, 0xff,
			0xf0, 0x51, 0x77, 0x8b, 0x65, 0xdb, 0x13, 0x57,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cbc,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0x9c, 0x4b, 0xfe, 0x6d, 0xfe, 0x73, 0x9c, 0x2b,
			0x52, 0x8f, 0xc8, 0x47, 0x2b, 0x66, 0xf9, 0x70,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cbc,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.in_len = 16,
		.out = {
			0x8b, 0x11, 0x08, 0x1c, 0xf0, 0xa0, 0x86, 0xe9,
			0x60, 0x57, 0x69, 0x5d, 0xdd, 0x42, 0x38, 0xe3,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cbc,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		},
		.in_len = 32,
		.out = {
			0x9c, 0x4b, 0xfe, 0x6d, 0xfe, 0x73, 0x9c, 0x2b,
			0x29, 0xf1, 0x7a, 0xd2, 0x16, 0xa0, 0xb2, 0xc6,
			0xd1, 0xa2, 0x31, 0xbe, 0xa3, 0x94, 0xc6, 0xb0,
			0x81, 0x22, 0x27, 0x17, 0x5b, 0xd4, 0x6d, 0x29,
		},
		.out_len = 32,
	},

	/* CFB64 (generated using https://github.com/joshuasing/libressl-test-gen) */
	{
		.mode = NID_rc2_cfb64,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 8,
		.key_bits = 64,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0xeb, 0xb7, 0x73, 0xf9, 0x93, 0x27, 0x8e, 0xff,
			0xf0, 0x51, 0x77, 0x8b, 0x65, 0xdb, 0x13, 0x57,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cfb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0x9c, 0x4b, 0xfe, 0x6d, 0xfe, 0x73, 0x9c, 0x2b,
			0x52, 0x8f, 0xc8, 0x47, 0x2b, 0x66, 0xf9, 0x70,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cfb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.in_len = 16,
		.out = {
			0x9c, 0x4a, 0xfc, 0x6e, 0xfa, 0x76, 0x9a, 0x2c,
			0xeb, 0xdf, 0x25, 0xb0, 0x15, 0x8b, 0x6a, 0x2a,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_cfb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		},
		.in_len = 32,
		.out = {
			0x8b, 0x10, 0x0a, 0x1f, 0xf4, 0xa5, 0x80, 0xee,
			0x94, 0x4d, 0xc3, 0xcd, 0x26, 0x79, 0x81, 0xc0,
			0xe9, 0x3e, 0x20, 0x85, 0x11, 0x71, 0x61, 0x2a,
			0x1d, 0x4c, 0x8a, 0xe2, 0xb7, 0x0a, 0xa8, 0xcf,
		},
		.out_len = 32,
	},

	/* OFB64 (generated using https://github.com/joshuasing/libressl-test-gen) */
	{
		.mode = NID_rc2_ofb64,
		.key = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.key_len = 8,
		.key_bits = 64,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0xeb, 0xb7, 0x73, 0xf9, 0x93, 0x27, 0x8e, 0xff,
			0xf0, 0x51, 0x77, 0x8b, 0x65, 0xdb, 0x13, 0x57,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_ofb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.in_len = 16,
		.out = {
			0x9c, 0x4b, 0xfe, 0x6d, 0xfe, 0x73, 0x9c, 0x2b,
			0x52, 0x8f, 0xc8, 0x47, 0x2b, 0x66, 0xf9, 0x70,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_ofb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.in_len = 16,
		.out = {
			0x9c, 0x4a, 0xfc, 0x6e, 0xfa, 0x76, 0x9a, 0x2c,
			0x5a, 0x86, 0xc2, 0x4c, 0x27, 0x6b, 0xf7, 0x7f,
		},
		.out_len = 16,
	},
	{
		.mode = NID_rc2_ofb64,
		.key = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		},
		.key_len = 16,
		.key_bits = 128,
		.iv = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		},
		.iv_len = 8,
		.in = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		},
		.in_len = 32,
		.out = {
			0x8b, 0x10, 0x0a, 0x1f, 0xf4, 0xa5, 0x80, 0xee,
			0xfa, 0x1d, 0x1a, 0x7c, 0xb2, 0x93, 0x00, 0x9d,
			0x36, 0xa1, 0xff, 0x3a, 0x77, 0x1d, 0x00, 0x9b,
			0x20, 0xde, 0x5f, 0x93, 0xcc, 0x3e, 0x51, 0xaa,
		},
		.out_len = 32,
	},
};

#define N_RC2_TESTS (sizeof(rc2_tests) / sizeof(rc2_tests[0]))

static int
rc2_ecb_test(size_t test_number, const struct rc2_test *rt)
{
	RC2_KEY key;
	uint8_t out[8];

	/* Encryption */
	memset(out, 0, sizeof(out));
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_ecb_encrypt(rt->in, out, &key, 1);

	if (memcmp(rt->out, out, rt->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    SN_rc2_ecb, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_ecb_encrypt(rt->out, out, &key, 0);

	if (memcmp(rt->in, out, rt->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    SN_rc2_ecb, test_number);
		return 0;
	}

	return 1;
}

static int
rc2_cbc_test(size_t test_number, const struct rc2_test *rt)
{
	RC2_KEY key;
	uint8_t out[512];
	uint8_t iv[64];

	/* Encryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_cbc_encrypt(rt->in, out, rt->in_len, &key, iv, 1);

	if (memcmp(rt->out, out, rt->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_cbc_encrypt(rt->out, out, rt->out_len, &key, iv, 0);

	if (memcmp(rt->in, out, rt->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	return 1;
}

static int
rc2_cfb64_test(size_t test_number, const struct rc2_test *rt)
{
	RC2_KEY key;
	uint8_t out[512];
	uint8_t iv[64];
	int remainder = 0;

	/* Encryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_cfb64_encrypt(rt->in, out, rt->in_len * 8, &key, iv, &remainder, 1);

	if (memcmp(rt->out, out, rt->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_cfb64_encrypt(rt->out, out, rt->out_len, &key, iv, &remainder, 0);

	if (memcmp(rt->in, out, rt->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	return 1;
}

static int
rc2_ofb64_test(size_t test_number, const struct rc2_test *rt)
{
	RC2_KEY key;
	uint8_t out[512];
	uint8_t iv[64];
	int remainder = 0;

	/* Encryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_ofb64_encrypt(rt->in, out, rt->in_len, &key, iv, &remainder);

	if (memcmp(rt->out, out, rt->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): encryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	/* Decryption */
	memset(out, 0, sizeof(out));
	memcpy(iv, rt->iv, rt->iv_len);
	RC2_set_key(&key, rt->key_len, rt->key, rt->key_bits);
	RC2_ofb64_encrypt(rt->out, out, rt->out_len, &key, iv, &remainder);

	if (memcmp(rt->in, out, rt->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): decryption mismatch\n",
		    SN_rc2_cbc, test_number);
		return 0;
	}

	return 1;
}

static int
rc2_evp_test(size_t test_number, const struct rc2_test *rt, const char *label,
    const EVP_CIPHER *cipher)
{
	EVP_CIPHER_CTX *ctx;
	uint8_t out[512];
	int in_len, out_len, total_len;
	int i;
	int success = 0;

	if ((ctx = EVP_CIPHER_CTX_new()) == NULL) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_CIPHER_CTX_new failed\n",
		    label, test_number);
		goto failed;
	}

	/* EVP encryption */
	total_len = 0;
	memset(out, 0, sizeof(out));
	if (!EVP_EncryptInit(ctx, cipher, NULL, NULL)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_SET_RC2_KEY_BITS,
	    rt->key_bits, NULL) <= 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_CIPHER_CTX_ctrl failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_key_length(ctx, rt->key_len)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_key_length failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_padding(ctx, rt->padding)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_padding failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_EncryptInit(ctx, NULL, rt->key, rt->iv)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	for (i = 0; i < rt->in_len;) {
		in_len = arc4random_uniform(sizeof(rt->in_len) / 2);
		if (in_len > rt->in_len - i)
			in_len = rt->in_len - i;

		if (!EVP_EncryptUpdate(ctx, out + total_len, &out_len,
		    rt->in + i, in_len)) {
			fprintf(stderr,
			    "FAIL (%s:%zu): EVP_EncryptUpdate failed\n",
			    label, test_number);
			goto failed;
		}

		i += in_len;
		total_len += out_len;
	}

	if (!EVP_EncryptFinal_ex(ctx, out + out_len, &out_len)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_EncryptFinal_ex failed\n",
		    label, test_number);
		goto failed;
	}
	total_len += out_len;

	if (!EVP_CIPHER_CTX_reset(ctx)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_reset failed\n",
		    label, test_number);
		goto failed;
	}

	if (total_len != rt->out_len) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP encryption length mismatch\n",
		    label, test_number);
		goto failed;
	}

	if (memcmp(rt->out, out, rt->out_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP encryption mismatch\n",
		    label, test_number);
		goto failed;
	}

	/* EVP decryption */
	total_len = 0;
	memset(out, 0, sizeof(out));
	if (!EVP_DecryptInit(ctx, cipher, NULL, NULL)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_SET_RC2_KEY_BITS,
	    rt->key_bits, NULL) <= 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_CIPHER_CTX_ctrl failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_key_length(ctx, rt->key_len)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_key_length failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_CIPHER_CTX_set_padding(ctx, rt->padding)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_set_padding failed\n",
		    label, test_number);
		goto failed;
	}

	if (!EVP_DecryptInit(ctx, NULL, rt->key, rt->iv)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptInit failed\n",
		    label, test_number);
		goto failed;
	}

	for (i = 0; i < rt->out_len;) {
		in_len = arc4random_uniform(sizeof(rt->out_len) / 2);
		if (in_len > rt->out_len - i)
			in_len = rt->out_len - i;

		if (!EVP_DecryptUpdate(ctx, out + total_len, &out_len,
		    rt->out + i, in_len)) {
			fprintf(stderr,
			    "FAIL (%s:%zu): EVP_DecryptUpdate failed\n",
			    label, test_number);
			goto failed;
		}

		i += in_len;
		total_len += out_len;
	}

	if (!EVP_DecryptFinal_ex(ctx, out + total_len, &out_len)) {
		fprintf(stderr, "FAIL (%s:%zu): EVP_DecryptFinal_ex failed\n",
		    label, test_number);
		goto failed;
	}
	total_len += out_len;

	if (!EVP_CIPHER_CTX_reset(ctx)) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP_CIPHER_CTX_reset failed\n",
		    label, test_number);
		goto failed;
	}

	if (total_len != rt->in_len) {
		fprintf(stderr,
		    "FAIL (%s:%zu): EVP decryption length mismatch\n",
		    label, test_number);
		goto failed;
	}

	if (memcmp(rt->in, out, rt->in_len) != 0) {
		fprintf(stderr, "FAIL (%s:%zu): EVP decryption mismatch\n",
		    label, test_number);
		goto failed;
	}

	success = 1;

 failed:
	EVP_CIPHER_CTX_free(ctx);
	return success;
}

static int
rc2_test(void)
{
	const struct rc2_test *rt;
	const char *label;
	const EVP_CIPHER *cipher;
	size_t i;
	int failed = 1;

	for (i = 0; i < N_RC2_TESTS; i++) {
		rt = &rc2_tests[i];
		switch (rt->mode) {
		case NID_rc2_ecb:
			label = SN_rc2_ecb;
			cipher = EVP_rc2_ecb();
			if (!rc2_ecb_test(i, rt))
				goto failed;
			break;
		case NID_rc2_cbc:
			label = SN_rc2_cbc;
			cipher = EVP_rc2_cbc();
			if (!rc2_cbc_test(i, rt))
				goto failed;
			break;
		case NID_rc2_cfb64:
			label = SN_rc2_cfb64;
			cipher = EVP_rc2_cfb64();
			if (!rc2_cfb64_test(i, rt))
				goto failed;
			break;
		case NID_rc2_ofb64:
			label = SN_rc2_ofb64;
			cipher = EVP_rc2_ofb();
			if (!rc2_ofb64_test(i, rt))
				goto failed;
			break;
		default:
			fprintf(stderr, "FAIL: unknown mode (%d)\n",
			    rt->mode);
			goto failed;
		}

		if (!rc2_evp_test(i, rt, label, cipher))
			goto failed;
	}

	failed = 0;

 failed:
	return failed;
}

int
main(int argc, char **argv)
{
	int failed = 0;

	failed |= rc2_test();

	return failed;
}

#include "arm_arch.h"

.text
#if defined(__thumb2__) && !defined(__APPLE__)
.syntax	unified
.thumb
#else
.code	32
#undef	__thumb2__
#endif

.align	5
.globl	OPENSSL_atomic_add
.type	OPENSSL_atomic_add,%function
OPENSSL_atomic_add:
#if __ARM_ARCH__>=6
.Ladd:	ldrex	r2,[r0]
	add	r3,r2,r1
	strex	r2,r3,[r0]
	cmp	r2,#0
	bne	.Ladd
	mov	r0,r3
	bx	lr
#else
	stmdb	sp!,{r4,r5,r6,lr}
	ldr	r2,.Lspinlock
	adr	r3,.Lspinlock
	mov	r4,r0
	mov	r5,r1
	add	r6,r3,r2	@ &spinlock
	b	.+8
.Lspin:	bl	sched_yield
	mov	r0,#-1
	swp	r0,r0,[r6]
	cmp	r0,#0
	bne	.Lspin

	ldr	r2,[r4]
	add	r2,r2,r5
	str	r2,[r4]
	str	r0,[r6]		@ release spinlock
	ldmia	sp!,{r4,r5,r6,lr}
	tst	lr,#1
	moveq	pc,lr
.word	0xe12fff1e	@ bx	lr
#endif
.size	OPENSSL_atomic_add,.-OPENSSL_atomic_add

#if __ARM_ARCH__>=7
.arch	armv7-a
.fpu	neon

.align	5
.globl	_armv7_neon_probe
.type	_armv7_neon_probe,%function
_armv7_neon_probe:
	vorr	q0,q0,q0
	bx	lr
.size	_armv7_neon_probe,.-_armv7_neon_probe

.globl	_armv8_aes_probe
.type	_armv8_aes_probe,%function
_armv8_aes_probe:
#if defined(__thumb2__) && !defined(__APPLE__)
.byte	0xb0,0xff,0x00,0x03	@ aese.8	q0,q0
#else
.byte	0x00,0x03,0xb0,0xf3	@ aese.8	q0,q0
#endif
	bx	lr
.size	_armv8_aes_probe,.-_armv8_aes_probe

.globl	_armv8_sha1_probe
.type	_armv8_sha1_probe,%function
_armv8_sha1_probe:
#if defined(__thumb2__) && !defined(__APPLE__)
.byte	0x00,0xef,0x40,0x0c	@ sha1c.32	q0,q0,q0
#else
.byte	0x40,0x0c,0x00,0xf2	@ sha1c.32	q0,q0,q0
#endif
	bx	lr
.size	_armv8_sha1_probe,.-_armv8_sha1_probe

.globl	_armv8_sha256_probe
.type	_armv8_sha256_probe,%function
_armv8_sha256_probe:
#if defined(__thumb2__) && !defined(__APPLE__)
.byte	0x00,0xff,0x40,0x0c	@ sha256h.32	q0,q0,q0
#else
.byte	0x40,0x0c,0x00,0xf3	@ sha256h.32	q0,q0,q0
#endif
	bx	lr
.size	_armv8_sha256_probe,.-_armv8_sha256_probe
.globl	_armv8_pmull_probe
.type	_armv8_pmull_probe,%function
_armv8_pmull_probe:
#if defined(__thumb2__) && !defined(__APPLE__)
.byte	0xa0,0xef,0x00,0x0e	@ vmull.p64	q0,d0,d0
#else
.byte	0x00,0x0e,0xa0,0xf2	@ vmull.p64	q0,d0,d0
#endif
	bx	lr
.size	_armv8_pmull_probe,.-_armv8_pmull_probe
#endif

.globl	OPENSSL_wipe_cpu
.type	OPENSSL_wipe_cpu,%function
OPENSSL_wipe_cpu:
#if __ARM_ARCH__>=7
	ldr	r0,.LOPENSSL_armcap
	adr	r1,.LOPENSSL_armcap
	ldr	r0,[r1,r0]
#ifdef	__APPLE__
	ldr	r0,[r0]
#endif
#endif
	eor	r2,r2,r2
	eor	r3,r3,r3
	eor	ip,ip,ip
#if __ARM_ARCH__>=7
	tst	r0,#1
	beq	.Lwipe_done
	veor	q0, q0, q0
	veor	q1, q1, q1
	veor	q2, q2, q2
	veor	q3, q3, q3
	veor	q8, q8, q8
	veor	q9, q9, q9
	veor	q10, q10, q10
	veor	q11, q11, q11
	veor	q12, q12, q12
	veor	q13, q13, q13
	veor	q14, q14, q14
	veor	q15, q15, q15
.Lwipe_done:
#endif
	mov	r0,sp
#if __ARM_ARCH__>=5
	bx	lr
#else
	tst	lr,#1
	moveq	pc,lr
.word	0xe12fff1e	@ bx	lr
#endif
.size	OPENSSL_wipe_cpu,.-OPENSSL_wipe_cpu

.align	5
#if __ARM_ARCH__>=7
.LOPENSSL_armcap:
.word	OPENSSL_armcap_P-.
#endif
#if __ARM_ARCH__>=6
.align	5
#else
.Lspinlock:
.word	atomic_add_spinlock-.Lspinlock
.align	5

.data
.align	2
atomic_add_spinlock:
.word	0
#endif

.comm	OPENSSL_armcap_P,4,4
.hidden	OPENSSL_armcap_P

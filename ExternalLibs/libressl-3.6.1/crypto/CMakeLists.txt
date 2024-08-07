add_definitions(-DLIBRESSL_CRYPTO_INTERNAL)

if(HOST_ASM_ELF_ARMV4)
	set(
		ASM_ARMV4_ELF_SRC
		aes/aes-elf-armv4.S
		bn/gf2m-elf-armv4.S
		bn/mont-elf-armv4.S
		sha/sha1-elf-armv4.S
		sha/sha512-elf-armv4.S
		sha/sha256-elf-armv4.S
		modes/ghash-elf-armv4.S
		armv4cpuid.S
		armcap.c
	)
	add_definitions(-DAES_ASM)
	add_definitions(-DOPENSSL_BN_ASM_MONT)
	add_definitions(-DOPENSSL_BN_ASM_GF2m)
	add_definitions(-DGHASH_ASM)
	add_definitions(-DSHA1_ASM)
	add_definitions(-DSHA256_ASM)
	add_definitions(-DSHA512_ASM)
	add_definitions(-DOPENSSL_CPUID_OBJ)
	set_property(SOURCE ${ASM_ARMV4_ELF_SRC} PROPERTY LANGUAGE C)
	set(CRYPTO_SRC ${CRYPTO_SRC} ${ASM_ARMV4_ELF_SRC})
endif()

if(HOST_ASM_ELF_X86_64)
	set(
		ASM_X86_64_ELF_SRC
		aes/aes-elf-x86_64.S
		aes/bsaes-elf-x86_64.S
		aes/vpaes-elf-x86_64.S
		aes/aesni-elf-x86_64.S
		aes/aesni-sha1-elf-x86_64.S
		bn/modexp512-elf-x86_64.S
		bn/mont-elf-x86_64.S
		bn/mont5-elf-x86_64.S
		bn/gf2m-elf-x86_64.S
		camellia/cmll-elf-x86_64.S
		md5/md5-elf-x86_64.S
		modes/ghash-elf-x86_64.S
		rc4/rc4-elf-x86_64.S
		rc4/rc4-md5-elf-x86_64.S
		sha/sha1-elf-x86_64.S
		sha/sha256-elf-x86_64.S
		sha/sha512-elf-x86_64.S
		whrlpool/wp-elf-x86_64.S
		cpuid-elf-x86_64.S
	)
	add_definitions(-DAES_ASM)
	add_definitions(-DBSAES_ASM)
	add_definitions(-DVPAES_ASM)
	add_definitions(-DOPENSSL_IA32_SSE2)
	add_definitions(-DOPENSSL_BN_ASM_MONT)
	add_definitions(-DOPENSSL_BN_ASM_MONT5)
	add_definitions(-DOPENSSL_BN_ASM_GF2m)
	add_definitions(-DMD5_ASM)
	add_definitions(-DGHASH_ASM)
	add_definitions(-DRSA_ASM)
	add_definitions(-DSHA1_ASM)
	add_definitions(-DSHA256_ASM)
	add_definitions(-DSHA512_ASM)
	add_definitions(-DWHIRLPOOL_ASM)
	add_definitions(-DOPENSSL_CPUID_OBJ)
	set(CRYPTO_SRC ${CRYPTO_SRC} ${ASM_X86_64_ELF_SRC})
	set_property(SOURCE ${ASM_X86_64_ELF_SRC} PROPERTY LANGUAGE C)
endif()

if(HOST_ASM_MACOSX_X86_64)
	set(
		ASM_X86_64_MACOSX_SRC
		aes/aes-macosx-x86_64.S
		aes/bsaes-macosx-x86_64.S
		aes/vpaes-macosx-x86_64.S
		aes/aesni-macosx-x86_64.S
		aes/aesni-sha1-macosx-x86_64.S
		bn/modexp512-macosx-x86_64.S
		bn/mont-macosx-x86_64.S
		bn/mont5-macosx-x86_64.S
		bn/gf2m-macosx-x86_64.S
		camellia/cmll-macosx-x86_64.S
		md5/md5-macosx-x86_64.S
		modes/ghash-macosx-x86_64.S
		rc4/rc4-macosx-x86_64.S
		rc4/rc4-md5-macosx-x86_64.S
		sha/sha1-macosx-x86_64.S
		sha/sha256-macosx-x86_64.S
		sha/sha512-macosx-x86_64.S
		whrlpool/wp-macosx-x86_64.S
		cpuid-macosx-x86_64.S
	)
	add_definitions(-DAES_ASM)
	add_definitions(-DBSAES_ASM)
	add_definitions(-DVPAES_ASM)
	add_definitions(-DOPENSSL_IA32_SSE2)
	add_definitions(-DOPENSSL_BN_ASM_MONT)
	add_definitions(-DOPENSSL_BN_ASM_MONT5)
	add_definitions(-DOPENSSL_BN_ASM_GF2m)
	add_definitions(-DMD5_ASM)
	add_definitions(-DGHASH_ASM)
	add_definitions(-DRSA_ASM)
	add_definitions(-DSHA1_ASM)
	add_definitions(-DSHA256_ASM)
	add_definitions(-DSHA512_ASM)
	add_definitions(-DWHIRLPOOL_ASM)
	add_definitions(-DOPENSSL_CPUID_OBJ)
	set(CRYPTO_SRC ${CRYPTO_SRC} ${ASM_X86_64_MACOSX_SRC})
	set_property(SOURCE ${ASM_X86_64_MACOSX_SRC} PROPERTY LANGUAGE C)
	set_property(SOURCE ${ASM_X86_64_MACOSX_SRC} PROPERTY XCODE_EXPLICIT_FILE_TYPE "sourcecode.asm")
endif()

if(HOST_ASM_MASM_X86_64)
	set(
		ASM_X86_64_MASM_SRC
		aes/aes-masm-x86_64.S
		aes/bsaes-masm-x86_64.S
		aes/vpaes-masm-x86_64.S
		aes/aesni-masm-x86_64.S
		aes/aesni-sha1-masm-x86_64.S
		#bn/modexp512-masm-x86_64.S
		#bn/mont-masm-x86_64.S
		#bn/mont5-masm-x86_64.S
		#bn/gf2m-masm-x86_64.S
		camellia/cmll-masm-x86_64.S
		md5/md5-masm-x86_64.S
		modes/ghash-masm-x86_64.S
		rc4/rc4-masm-x86_64.S
		rc4/rc4-md5-masm-x86_64.S
		sha/sha1-masm-x86_64.S
		sha/sha256-masm-x86_64.S
		sha/sha512-masm-x86_64.S
		whrlpool/wp-masm-x86_64.S
		cpuid-masm-x86_64.S
	)
	add_definitions(-DAES_ASM)
	add_definitions(-DBSAES_ASM)
	add_definitions(-DVPAES_ASM)
	add_definitions(-DOPENSSL_IA32_SSE2)
	#add_definitions(-DOPENSSL_BN_ASM_MONT)
	#add_definitions(-DOPENSSL_BN_ASM_MONT5)
	#add_definitions(-DOPENSSL_BN_ASM_GF2m)
	add_definitions(-DMD5_ASM)
	add_definitions(-DGHASH_ASM)
	add_definitions(-DRSA_ASM)
	add_definitions(-DSHA1_ASM)
	add_definitions(-DSHA256_ASM)
	add_definitions(-DSHA512_ASM)
	add_definitions(-DWHIRLPOOL_ASM)
	add_definitions(-DOPENSSL_CPUID_OBJ)
	set(CRYPTO_SRC ${CRYPTO_SRC} ${ASM_X86_64_MASM_SRC})
	set_property(SOURCE ${ASM_X86_64_MASM_SRC} PROPERTY LANGUAGE ASM_MASM)
endif()

if(HOST_ASM_MINGW64_X86_64)
	set(
		ASM_X86_64_MINGW64_SRC
		aes/aes-mingw64-x86_64.S
		aes/bsaes-mingw64-x86_64.S
		aes/vpaes-mingw64-x86_64.S
		aes/aesni-mingw64-x86_64.S
		aes/aesni-sha1-mingw64-x86_64.S
		#bn/modexp512-mingw64-x86_64.S
		#bn/mont-mingw64-x86_64.S
		#bn/mont5-mingw64-x86_64.S
		#bn/gf2m-mingw64-x86_64.S
		camellia/cmll-mingw64-x86_64.S
		md5/md5-mingw64-x86_64.S
		modes/ghash-mingw64-x86_64.S
		rc4/rc4-mingw64-x86_64.S
		rc4/rc4-md5-mingw64-x86_64.S
		sha/sha1-mingw64-x86_64.S
		sha/sha256-mingw64-x86_64.S
		sha/sha512-mingw64-x86_64.S
		whrlpool/wp-mingw64-x86_64.S
		cpuid-mingw64-x86_64.S
	)
	add_definitions(-DAES_ASM)
	add_definitions(-DBSAES_ASM)
	add_definitions(-DVPAES_ASM)
	add_definitions(-DOPENSSL_IA32_SSE2)
	#add_definitions(-DOPENSSL_BN_ASM_MONT)
	#add_definitions(-DOPENSSL_BN_ASM_MONT5)
	#add_definitions(-DOPENSSL_BN_ASM_GF2m)
	add_definitions(-DMD5_ASM)
	add_definitions(-DGHASH_ASM)
	add_definitions(-DRSA_ASM)
	add_definitions(-DSHA1_ASM)
	add_definitions(-DSHA256_ASM)
	add_definitions(-DSHA512_ASM)
	add_definitions(-DWHIRLPOOL_ASM)
	add_definitions(-DOPENSSL_CPUID_OBJ)
	set(CRYPTO_SRC ${CRYPTO_SRC} ${ASM_X86_64_MINGW64_SRC})
	set_property(SOURCE ${ASM_X86_64_MINGW64_SRC} PROPERTY LANGUAGE C)
endif()

if((NOT HOST_ASM_ELF_X86_64) AND
	(NOT HOST_ASM_MACOSX_X86_64) AND
	(NOT HOST_ASM_MASM_X86_64) AND
	(NOT HOST_ASM_MINGW64_X86_64) AND
	(NOT HOST_ASM_ELF_ARMV4))
	set(
		CRYPTO_SRC
		${CRYPTO_SRC}
		aes/aes_core.c
	)
endif()

if((NOT HOST_ASM_ELF_X86_64) AND
	(NOT HOST_ASM_MACOSX_X86_64) AND
	(NOT HOST_ASM_MASM_X86_64) AND
	(NOT HOST_ASM_MINGW64_X86_64))
	set(
		CRYPTO_SRC
		${CRYPTO_SRC}
		aes/aes_cbc.c
		camellia/camellia.c
		camellia/cmll_cbc.c
		rc4/rc4_enc.c
		rc4/rc4_skey.c
		whrlpool/wp_block.c
	)
endif()

set(
	CRYPTO_SRC
	${CRYPTO_SRC}
	cpt_err.c
	cryptlib.c
	crypto_init.c
	cversion.c
	ex_data.c
	malloc-wrapper.c
	mem_clr.c
	mem_dbg.c
	o_fips.c
	o_init.c
	o_str.c
	o_time.c
	aes/aes_cfb.c
	aes/aes_ctr.c
	aes/aes_ecb.c
	aes/aes_ige.c
	aes/aes_misc.c
	aes/aes_ofb.c
	aes/aes_wrap.c
	asn1/a_bitstr.c
	asn1/a_enum.c
	asn1/a_int.c
	asn1/a_mbstr.c
	asn1/a_object.c
	asn1/a_octet.c
	asn1/a_pkey.c
	asn1/a_print.c
	asn1/a_pubkey.c
	asn1/a_strex.c
	asn1/a_string.c
	asn1/a_strnid.c
	asn1/a_time.c
	asn1/a_time_tm.c
	asn1/a_type.c
	asn1/a_utf8.c
	asn1/ameth_lib.c
	asn1/asn1_err.c
	asn1/asn1_gen.c
	asn1/asn1_item.c
	asn1/asn1_lib.c
	asn1/asn1_old.c
	asn1/asn1_old_lib.c
	asn1/asn1_par.c
	asn1/asn1_types.c
	asn1/asn_mime.c
	asn1/asn_moid.c
	asn1/bio_asn1.c
	asn1/bio_ndef.c
	asn1/nsseq.c
	asn1/p5_pbe.c
	asn1/p5_pbev2.c
	asn1/p8_pkey.c
	asn1/t_crl.c
	asn1/t_pkey.c
	asn1/t_req.c
	asn1/t_spki.c
	asn1/t_x509.c
	asn1/t_x509a.c
	asn1/tasn_dec.c
	asn1/tasn_enc.c
	asn1/tasn_fre.c
	asn1/tasn_new.c
	asn1/tasn_prn.c
	asn1/tasn_typ.c
	asn1/tasn_utl.c
	asn1/x_algor.c
	asn1/x_attrib.c
	asn1/x_bignum.c
	asn1/x_crl.c
	asn1/x_exten.c
	asn1/x_info.c
	asn1/x_long.c
	asn1/x_name.c
	asn1/x_pkey.c
	asn1/x_pubkey.c
	asn1/x_req.c
	asn1/x_sig.c
	asn1/x_spki.c
	asn1/x_val.c
	asn1/x_x509.c
	asn1/x_x509a.c
	bf/bf_cfb64.c
	bf/bf_ecb.c
	bf/bf_enc.c
	bf/bf_ofb64.c
	bf/bf_skey.c
	bio/b_dump.c
	bio/b_print.c
	bio/b_sock.c
	bio/bf_buff.c
	bio/bf_nbio.c
	bio/bf_null.c
	bio/bio_cb.c
	bio/bio_err.c
	bio/bio_lib.c
	bio/bio_meth.c
	bio/bss_acpt.c
	bio/bss_bio.c
	bio/bss_conn.c
	bio/bss_dgram.c
	bio/bss_fd.c
	bio/bss_file.c
	bio/bss_mem.c
	bio/bss_null.c
	bio/bss_sock.c
	bn/bn_add.c
	bn/bn_asm.c
	bn/bn_blind.c
	bn/bn_bpsw.c
	bn/bn_const.c
	bn/bn_ctx.c
	bn/bn_depr.c
	bn/bn_div.c
	bn/bn_err.c
	bn/bn_exp.c
	bn/bn_exp2.c
	bn/bn_gcd.c
	bn/bn_gf2m.c
	bn/bn_isqrt.c
	bn/bn_kron.c
	bn/bn_lib.c
	bn/bn_mod.c
	bn/bn_mont.c
	bn/bn_mpi.c
	bn/bn_mul.c
	bn/bn_nist.c
	bn/bn_prime.c
	bn/bn_print.c
	bn/bn_rand.c
	bn/bn_recp.c
	bn/bn_shift.c
	bn/bn_sqr.c
	bn/bn_sqrt.c
	bn/bn_word.c
	bn/bn_x931p.c
	buffer/buf_err.c
	buffer/buf_str.c
	buffer/buffer.c
	bytestring/bs_ber.c
	bytestring/bs_cbb.c
	bytestring/bs_cbs.c
	camellia/cmll_cfb.c
	camellia/cmll_ctr.c
	camellia/cmll_ecb.c
	camellia/cmll_misc.c
	camellia/cmll_ofb.c
	cast/c_cfb64.c
	cast/c_ecb.c
	cast/c_enc.c
	cast/c_ofb64.c
	cast/c_skey.c
	chacha/chacha.c
	cmac/cm_ameth.c
	cmac/cm_pmeth.c
	cmac/cmac.c
	cms/cms_asn1.c
	cms/cms_att.c
	cms/cms_cd.c
	cms/cms_dd.c
	cms/cms_enc.c
	cms/cms_env.c
	cms/cms_err.c
	cms/cms_ess.c
	cms/cms_io.c
	cms/cms_kari.c
	cms/cms_lib.c
	cms/cms_pwri.c
	cms/cms_sd.c
	cms/cms_smime.c
	comp/c_rle.c
	comp/c_zlib.c
	comp/comp_err.c
	comp/comp_lib.c
	conf/conf_api.c
	conf/conf_def.c
	conf/conf_err.c
	conf/conf_lib.c
	conf/conf_mall.c
	conf/conf_mod.c
	conf/conf_sap.c
	ct/ct_b64.c
	ct/ct_err.c
	ct/ct_log.c
	ct/ct_oct.c
	ct/ct_policy.c
	ct/ct_prn.c
	ct/ct_sct.c
	ct/ct_sct_ctx.c
	ct/ct_vfy.c
	ct/ct_x509v3.c
	curve25519/curve25519-generic.c
	curve25519/curve25519.c
	des/cbc_cksm.c
	des/cbc_enc.c
	des/cfb64ede.c
	des/cfb64enc.c
	des/cfb_enc.c
	des/des_enc.c
	des/ecb3_enc.c
	des/ecb_enc.c
	des/ede_cbcm_enc.c
	des/enc_read.c
	des/enc_writ.c
	des/fcrypt.c
	des/fcrypt_b.c
	des/ofb64ede.c
	des/ofb64enc.c
	des/ofb_enc.c
	des/pcbc_enc.c
	des/qud_cksm.c
	des/rand_key.c
	des/set_key.c
	des/str2key.c
	des/xcbc_enc.c
	dh/dh_ameth.c
	dh/dh_asn1.c
	dh/dh_check.c
	dh/dh_depr.c
	dh/dh_err.c
	dh/dh_gen.c
	dh/dh_key.c
	dh/dh_lib.c
	dh/dh_pmeth.c
	dh/dh_prn.c
	dsa/dsa_ameth.c
	dsa/dsa_asn1.c
	dsa/dsa_depr.c
	dsa/dsa_err.c
	dsa/dsa_gen.c
	dsa/dsa_key.c
	dsa/dsa_lib.c
	dsa/dsa_meth.c
	dsa/dsa_ossl.c
	dsa/dsa_pmeth.c
	dsa/dsa_prn.c
	dsa/dsa_sign.c
	dsa/dsa_vrf.c
	dso/dso_dlfcn.c
	dso/dso_err.c
	dso/dso_lib.c
	dso/dso_null.c
	dso/dso_openssl.c
	ec/ec2_mult.c
	ec/ec2_oct.c
	ec/ec2_smpl.c
	ec/ec_ameth.c
	ec/ec_asn1.c
	ec/ec_check.c
	ec/ec_curve.c
	ec/ec_cvt.c
	ec/ec_err.c
	ec/ec_key.c
	ec/ec_kmeth.c
	ec/ec_lib.c
	ec/ec_mult.c
	ec/ec_oct.c
	ec/ec_pmeth.c
	ec/ec_print.c
	ec/eck_prn.c
	ec/ecp_mont.c
	ec/ecp_nist.c
	ec/ecp_oct.c
	ec/ecp_smpl.c
	ecdh/ecdh_kdf.c
	ecdh/ech_err.c
	ecdh/ech_key.c
	ecdh/ech_lib.c
	ecdsa/ecs_asn1.c
	ecdsa/ecs_err.c
	ecdsa/ecs_lib.c
	ecdsa/ecs_ossl.c
	ecdsa/ecs_sign.c
	ecdsa/ecs_vrf.c
	engine/eng_all.c
	engine/eng_cnf.c
	engine/eng_ctrl.c
	engine/eng_dyn.c
	engine/eng_err.c
	engine/eng_fat.c
	engine/eng_init.c
	engine/eng_lib.c
	engine/eng_list.c
	engine/eng_openssl.c
	engine/eng_pkey.c
	engine/eng_table.c
	engine/tb_asnmth.c
	engine/tb_cipher.c
	engine/tb_dh.c
	engine/tb_digest.c
	engine/tb_dsa.c
	engine/tb_ecdh.c
	engine/tb_ecdsa.c
	engine/tb_eckey.c
	engine/tb_pkmeth.c
	engine/tb_rand.c
	engine/tb_rsa.c
	engine/tb_store.c
	err/err.c
	err/err_all.c
	err/err_prn.c
	evp/bio_b64.c
	evp/bio_enc.c
	evp/bio_md.c
	evp/c_all.c
	evp/digest.c
	evp/e_aes.c
	evp/e_aes_cbc_hmac_sha1.c
	evp/e_bf.c
	evp/e_camellia.c
	evp/e_cast.c
	evp/e_chacha.c
	evp/e_chacha20poly1305.c
	evp/e_des.c
	evp/e_des3.c
	evp/e_gost2814789.c
	evp/e_idea.c
	evp/e_null.c
	evp/e_old.c
	evp/e_rc2.c
	evp/e_rc4.c
	evp/e_rc4_hmac_md5.c
	evp/e_sm4.c
	evp/e_xcbc_d.c
	evp/encode.c
	evp/evp_aead.c
	evp/evp_enc.c
	evp/evp_err.c
	evp/evp_key.c
	evp/evp_lib.c
	evp/evp_pbe.c
	evp/evp_pkey.c
	evp/m_gost2814789.c
	evp/m_gostr341194.c
	evp/m_md4.c
	evp/m_md5.c
	evp/m_md5_sha1.c
	evp/m_null.c
	evp/m_ripemd.c
	evp/m_sha1.c
	evp/m_sigver.c
	evp/m_streebog.c
	evp/m_sm3.c
	evp/m_wp.c
	evp/names.c
	evp/p5_crpt.c
	evp/p5_crpt2.c
	evp/p_dec.c
	evp/p_enc.c
	evp/p_lib.c
	evp/p_open.c
	evp/p_seal.c
	evp/p_sign.c
	evp/p_verify.c
	evp/pmeth_fn.c
	evp/pmeth_gn.c
	evp/pmeth_lib.c
	gost/gost2814789.c
	gost/gost89_keywrap.c
	gost/gost89_params.c
	gost/gost89imit_ameth.c
	gost/gost89imit_pmeth.c
	gost/gost_asn1.c
	gost/gost_err.c
	gost/gostr341001.c
	gost/gostr341001_ameth.c
	gost/gostr341001_key.c
	gost/gostr341001_params.c
	gost/gostr341001_pmeth.c
	gost/gostr341194.c
	gost/streebog.c
	hkdf/hkdf.c
	hmac/hm_ameth.c
	hmac/hm_pmeth.c
	hmac/hmac.c
	idea/i_cbc.c
	idea/i_cfb64.c
	idea/i_ecb.c
	idea/i_ofb64.c
	idea/i_skey.c
	kdf/hkdf_evp.c
	kdf/kdf_err.c
	lhash/lh_stats.c
	lhash/lhash.c
	md4/md4_dgst.c
	md4/md4_one.c
	md5/md5_dgst.c
	md5/md5_one.c
	modes/cbc128.c
	modes/ccm128.c
	modes/cfb128.c
	modes/ctr128.c
	modes/cts128.c
	modes/gcm128.c
	modes/ofb128.c
	modes/xts128.c
	objects/o_names.c
	objects/obj_dat.c
	objects/obj_err.c
	objects/obj_lib.c
	objects/obj_xref.c
	ocsp/ocsp_asn.c
	ocsp/ocsp_cl.c
	ocsp/ocsp_err.c
	ocsp/ocsp_ext.c
	ocsp/ocsp_ht.c
	ocsp/ocsp_lib.c
	ocsp/ocsp_prn.c
	ocsp/ocsp_srv.c
	ocsp/ocsp_vfy.c
	pem/pem_all.c
	pem/pem_err.c
	pem/pem_info.c
	pem/pem_lib.c
	pem/pem_oth.c
	pem/pem_pk8.c
	pem/pem_pkey.c
	pem/pem_sign.c
	pem/pem_x509.c
	pem/pem_xaux.c
	pem/pvkfmt.c
	pkcs12/p12_add.c
	pkcs12/p12_asn.c
	pkcs12/p12_attr.c
	pkcs12/p12_crpt.c
	pkcs12/p12_crt.c
	pkcs12/p12_decr.c
	pkcs12/p12_init.c
	pkcs12/p12_key.c
	pkcs12/p12_kiss.c
	pkcs12/p12_mutl.c
	pkcs12/p12_npas.c
	pkcs12/p12_p8d.c
	pkcs12/p12_p8e.c
	pkcs12/p12_sbag.c
	pkcs12/p12_utl.c
	pkcs12/pk12err.c
	pkcs7/bio_pk7.c
	pkcs7/pk7_asn1.c
	pkcs7/pk7_attr.c
	pkcs7/pk7_doit.c
	pkcs7/pk7_lib.c
	pkcs7/pk7_mime.c
	pkcs7/pk7_smime.c
	pkcs7/pkcs7err.c
	poly1305/poly1305.c
	rand/rand_err.c
	rand/rand_lib.c
	rand/randfile.c
	rc2/rc2_cbc.c
	rc2/rc2_ecb.c
	rc2/rc2_skey.c
	rc2/rc2cfb64.c
	rc2/rc2ofb64.c
	ripemd/rmd_dgst.c
	ripemd/rmd_one.c
	rsa/rsa_ameth.c
	rsa/rsa_asn1.c
	rsa/rsa_chk.c
	rsa/rsa_crpt.c
	rsa/rsa_depr.c
	rsa/rsa_eay.c
	rsa/rsa_err.c
	rsa/rsa_gen.c
	rsa/rsa_lib.c
	rsa/rsa_meth.c
	rsa/rsa_none.c
	rsa/rsa_oaep.c
	rsa/rsa_pk1.c
	rsa/rsa_pmeth.c
	rsa/rsa_prn.c
	rsa/rsa_pss.c
	rsa/rsa_saos.c
	rsa/rsa_sign.c
	rsa/rsa_x931.c
	sha/sha1_one.c
	sha/sha1dgst.c
	sha/sha256.c
	sha/sha512.c
	sm3/sm3.c
	sm4/sm4.c
	stack/stack.c
	ts/ts_asn1.c
	ts/ts_conf.c
	ts/ts_err.c
	ts/ts_lib.c
	ts/ts_req_print.c
	ts/ts_req_utils.c
	ts/ts_rsp_print.c
	ts/ts_rsp_sign.c
	ts/ts_rsp_utils.c
	ts/ts_rsp_verify.c
	ts/ts_verify_ctx.c
	txt_db/txt_db.c
	ui/ui_err.c
	ui/ui_lib.c
	ui/ui_util.c
	whrlpool/wp_dgst.c
	x509/by_dir.c
	x509/by_file.c
	x509/by_mem.c
	x509/pcy_cache.c
	x509/pcy_data.c
	x509/pcy_lib.c
	x509/pcy_map.c
	x509/pcy_node.c
	x509/pcy_tree.c
	x509/x509_addr.c
	x509/x509_akey.c
	x509/x509_akeya.c
	x509/x509_alt.c
	x509/x509_asid.c
	x509/x509_att.c
	x509/x509_bcons.c
	x509/x509_bitst.c
	x509/x509_cmp.c
	x509/x509_conf.c
	x509/x509_constraints.c
	x509/x509_cpols.c
	x509/x509_crld.c
	x509/x509_d2.c
	x509/x509_def.c
	x509/x509_enum.c
	x509/x509_err.c
	x509/x509_ext.c
	x509/x509_extku.c
	x509/x509_genn.c
	x509/x509_ia5.c
	x509/x509_info.c
	x509/x509_int.c
	x509/x509_issuer_cache.c
	x509/x509_lib.c
	x509/x509_lu.c
	x509/x509_ncons.c
	x509/x509_obj.c
	x509/x509_ocsp.c
	x509/x509_pci.c
	x509/x509_pcia.c
	x509/x509_pcons.c
	x509/x509_pku.c
	x509/x509_pmaps.c
	x509/x509_prn.c
	x509/x509_purp.c
	x509/x509_r2x.c
	x509/x509_req.c
	x509/x509_set.c
	x509/x509_skey.c
	x509/x509_sxnet.c
	x509/x509_trs.c
	x509/x509_txt.c
	x509/x509_utl.c
	x509/x509_v3.c
	x509/x509_verify.c
	x509/x509_vfy.c
	x509/x509_vpm.c
	x509/x509cset.c
	x509/x509name.c
	x509/x509rset.c
	x509/x509spki.c
	x509/x509type.c
	x509/x_all.c
)

if(UNIX)
	set(CRYPTO_SRC ${CRYPTO_SRC} crypto_lock.c)
	set(CRYPTO_SRC ${CRYPTO_SRC} bio/b_posix.c)
	set(CRYPTO_SRC ${CRYPTO_SRC} bio/bss_log.c)
	set(CRYPTO_SRC ${CRYPTO_SRC} ui/ui_openssl.c)
endif()

if(WIN32)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/crypto_lock_win.c)
	set(CRYPTO_SRC ${CRYPTO_SRC} bio/b_win.c)
	set(CRYPTO_UNEXPORT ${CRYPTO_UNEXPORT} BIO_s_log)
	set(CRYPTO_SRC ${CRYPTO_SRC} ui/ui_openssl_win.c)
endif()

if(WIN32)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/posix_win.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} gettimeofday)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} getuid)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_perror)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_fopen)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_fgets)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_open)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_rename)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_connect)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_close)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_read)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_write)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_getsockopt)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} posix_setsockopt)
endif()

if(NOT HAVE_ASPRINTF)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/bsd-asprintf.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} asprintf)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} vasprintf)
endif()

if(NOT HAVE_FREEZERO)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/freezero.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} freezero)
endif()

if(NOT HAVE_GETPAGESIZE)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/getpagesize.c)
endif()

if(NOT HAVE_GETPROGNAME)
	if(WIN32)
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_windows.c)
	elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_linux.c)
	else()
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/getprogname_unimpl.c)
	endif()
endif()

if(NOT HAVE_REALLOCARRAY)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/reallocarray.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} reallocarray)
endif()

if(NOT HAVE_RECALLOCARRAY)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/recallocarray.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} recallocarray)
endif()

if(NOT HAVE_STRCASECMP)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strcasecmp.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strcasecmp)
endif()

if(NOT HAVE_STRLCAT)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strlcat.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strlcat)
endif()

if(NOT HAVE_STRLCPY)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strlcpy.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strlcpy)
endif()

if(NOT HAVE_STRNDUP)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strndup.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strndup)
	if(NOT HAVE_STRNLEN)
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/strnlen.c)
		set(EXTRA_EXPORT ${EXTRA_EXPORT} strnlen)
	endif()
endif()

if(NOT HAVE_STRSEP)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strsep.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strsep)
endif()

if(NOT HAVE_STRTONUM)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/strtonum.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} strtonum)
endif()

if(NOT HAVE_SYSLOG_R)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/syslog_r.c)
endif()

if(NOT HAVE_TIMEGM)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/timegm.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} timegm)
endif()

if(NOT HAVE_EXPLICIT_BZERO)
	if(WIN32)
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/explicit_bzero_win.c)
	else()
		set(CRYPTO_SRC ${CRYPTO_SRC} compat/explicit_bzero.c)
		set_source_files_properties(compat/explicit_bzero.c PROPERTIES COMPILE_FLAGS -O0)
	endif()
	set(EXTRA_EXPORT ${EXTRA_EXPORT} explicit_bzero)
endif()

if(NOT HAVE_ARC4RANDOM_BUF)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/arc4random.c)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/arc4random_uniform.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random_buf)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} arc4random_uniform)

	if(NOT HAVE_GETENTROPY)
		if(WIN32)
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_win.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "AIX")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_aix.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_freebsd.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_hpux.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_linux.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "NetBSD")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_netbsd.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_osx.c)
		elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
			set(CRYPTO_SRC ${CRYPTO_SRC} compat/getentropy_solaris.c)
		endif()
		set(EXTRA_EXPORT ${EXTRA_EXPORT} getentropy)
	endif()
endif()

if(NOT HAVE_TIMINGSAFE_BCMP)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/timingsafe_bcmp.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} timingsafe_bcmp)
endif()

if(NOT HAVE_TIMINGSAFE_MEMCMP)
	set(CRYPTO_SRC ${CRYPTO_SRC} compat/timingsafe_memcmp.c)
	set(EXTRA_EXPORT ${EXTRA_EXPORT} timingsafe_memcmp)
endif()

if(NOT ENABLE_ASM)
	add_definitions(-DOPENSSL_NO_ASM)
else()
	if(MSVC)
		if((NOT "${CMAKE_GENERATOR}" MATCHES "Win64") AND
		   (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "x64"))
			add_definitions(-DOPENSSL_NO_ASM)
		endif()
	elseif(WIN32)
		add_definitions(-DOPENSSL_NO_ASM)
	endif()
endif()

if(NOT "${OPENSSLDIR}" STREQUAL "")
	add_definitions(-DOPENSSLDIR=\"${OPENSSLDIR}\")
else()
	if(WIN32)
		add_definitions(-DOPENSSLDIR=\"C:/Windows/libressl/ssl\")
	else()
		add_definitions(-DOPENSSLDIR=\"${CMAKE_INSTALL_PREFIX}/etc/ssl\")
	endif()
endif()

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/crypto.sym SYMS)
foreach(SYM IN LISTS CRYPTO_UNEXPORT)
	string(REPLACE "${SYM}\n" "" SYMS ${SYMS})
endforeach()
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/crypto_p.sym ${SYMS})
if(EXTRA_EXPORT)
	list(SORT EXTRA_EXPORT)
	foreach(SYM IN LISTS EXTRA_EXPORT)
		file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/crypto_p.sym "${SYM}\n")
	endforeach()
endif()

set(LIBTLS_EXTRA_EXPORT ${EXTRA_EXPORT} PARENT_SCOPE)

add_library(crypto_obj OBJECT ${CRYPTO_SRC})
target_include_directories(crypto_obj
	PRIVATE
		.
		asn1
		bio
		bn
		bytestring
		dh
		dsa
		ec
		ecdh
		ecdsa
		evp
		hmac
		modes
		ocsp
		pkcs12
		rsa
		x509
		../include/compat
	PUBLIC
		../include)

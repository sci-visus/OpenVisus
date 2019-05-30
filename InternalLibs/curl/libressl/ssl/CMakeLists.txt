set(
	SSL_SRC
	bio_ssl.c
	bs_ber.c
	bs_cbb.c
	bs_cbs.c
	d1_both.c
	d1_clnt.c
	d1_enc.c
	d1_lib.c
	d1_pkt.c
	d1_srtp.c
	d1_srvr.c
	pqueue.c
	s3_cbc.c
	s3_lib.c
	ssl_algs.c
	ssl_asn1.c
	ssl_both.c
	ssl_cert.c
	ssl_ciph.c
	ssl_clnt.c
	ssl_err.c
	ssl_init.c
	ssl_lib.c
	ssl_methods.c
	ssl_packet.c
	ssl_pkt.c
	ssl_rsa.c
	ssl_sess.c
	ssl_sigalgs.c
	ssl_srvr.c
	ssl_stat.c
	ssl_tlsext.c
	ssl_txt.c
	ssl_versions.c
	t1_enc.c
	t1_hash.c
	t1_lib.c
	tls13_handshake.c
	tls13_key_schedule.c
)

AddInternalLibrary(ssl ${SSL_SRC})
target_include_directories(ssl PRIVATE . ../include/compat PUBLIC ../include)
target_link_libraries(ssl crypto ${PLATFORM_LIBS})


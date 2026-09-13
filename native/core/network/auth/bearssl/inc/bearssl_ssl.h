

#ifndef BR_BEARSSL_SSL_H__
#define BR_BEARSSL_SSL_H__

#include <stddef.h>
#include <stdint.h>

#include "bearssl_block.h"
#include "bearssl_hash.h"
#include "bearssl_hmac.h"
#include "bearssl_prf.h"
#include "bearssl_rand.h"
#include "bearssl_x509.h"

#ifdef __cplusplus
extern "C" {
#endif




#define BR_SSL_BUFSIZE_INPUT    (16384 + 325)


#define BR_SSL_BUFSIZE_OUTPUT   (16384 + 85)


#define BR_SSL_BUFSIZE_MONO     BR_SSL_BUFSIZE_INPUT


#define BR_SSL_BUFSIZE_BIDI     (BR_SSL_BUFSIZE_INPUT + BR_SSL_BUFSIZE_OUTPUT)




#define BR_SSL30   0x0300

#define BR_TLS10   0x0301

#define BR_TLS11   0x0302

#define BR_TLS12   0x0303




#define BR_ERR_OK                      0


#define BR_ERR_BAD_PARAM               1


#define BR_ERR_BAD_STATE               2


#define BR_ERR_UNSUPPORTED_VERSION     3


#define BR_ERR_BAD_VERSION             4


#define BR_ERR_BAD_LENGTH              5


#define BR_ERR_TOO_LARGE               6


#define BR_ERR_BAD_MAC                 7


#define BR_ERR_NO_RANDOM               8


#define BR_ERR_UNKNOWN_TYPE            9


#define BR_ERR_UNEXPECTED             10


#define BR_ERR_BAD_CCS                12


#define BR_ERR_BAD_ALERT              13


#define BR_ERR_BAD_HANDSHAKE          14


#define BR_ERR_OVERSIZED_ID           15


#define BR_ERR_BAD_CIPHER_SUITE       16


#define BR_ERR_BAD_COMPRESSION        17


#define BR_ERR_BAD_FRAGLEN            18


#define BR_ERR_BAD_SECRENEG           19


#define BR_ERR_EXTRA_EXTENSION        20


#define BR_ERR_BAD_SNI                21


#define BR_ERR_BAD_HELLO_DONE         22


#define BR_ERR_LIMIT_EXCEEDED         23


#define BR_ERR_BAD_FINISHED           24


#define BR_ERR_RESUME_MISMATCH        25


#define BR_ERR_INVALID_ALGORITHM      26


#define BR_ERR_BAD_SIGNATURE          27


#define BR_ERR_WRONG_KEY_USAGE        28


#define BR_ERR_NO_CLIENT_AUTH         29


#define BR_ERR_IO                     31


#define BR_ERR_RECV_FATAL_ALERT      256


#define BR_ERR_SEND_FATAL_ALERT      512




typedef struct br_sslrec_in_class_ br_sslrec_in_class;
struct br_sslrec_in_class_ {
	
	size_t context_size;

	
	int (*check_length)(const br_sslrec_in_class *const *ctx,
		size_t record_len);

	
	unsigned char *(*decrypt)(const br_sslrec_in_class **ctx,
		int record_type, unsigned version,
		void *payload, size_t *len);
};


typedef struct br_sslrec_out_class_ br_sslrec_out_class;
struct br_sslrec_out_class_ {
	
	size_t context_size;

	
	void (*max_plaintext)(const br_sslrec_out_class *const *ctx,
		size_t *start, size_t *end);

	
	unsigned char *(*encrypt)(const br_sslrec_out_class **ctx,
		int record_type, unsigned version,
		void *plaintext, size_t *len);
};


typedef struct {
	
	const br_sslrec_out_class *vtable;
} br_sslrec_out_clear_context;


extern const br_sslrec_out_class br_sslrec_out_clear_vtable;




typedef struct br_sslrec_in_cbc_class_ br_sslrec_in_cbc_class;
struct br_sslrec_in_cbc_class_ {
	
	br_sslrec_in_class inner;

	
	void (*init)(const br_sslrec_in_cbc_class **ctx,
		const br_block_cbcdec_class *bc_impl,
		const void *bc_key, size_t bc_key_len,
		const br_hash_class *dig_impl,
		const void *mac_key, size_t mac_key_len, size_t mac_out_len,
		const void *iv);
};


typedef struct br_sslrec_out_cbc_class_ br_sslrec_out_cbc_class;
struct br_sslrec_out_cbc_class_ {
	
	br_sslrec_out_class inner;

	
	void (*init)(const br_sslrec_out_cbc_class **ctx,
		const br_block_cbcenc_class *bc_impl,
		const void *bc_key, size_t bc_key_len,
		const br_hash_class *dig_impl,
		const void *mac_key, size_t mac_key_len, size_t mac_out_len,
		const void *iv);
};


typedef struct {
	
	const br_sslrec_in_cbc_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	uint64_t seq;
	union {
		const br_block_cbcdec_class *vtable;
		br_aes_gen_cbcdec_keys aes;
		br_des_gen_cbcdec_keys des;
	} bc;
	br_hmac_key_context mac;
	size_t mac_len;
	unsigned char iv[16];
	int explicit_IV;
#endif
} br_sslrec_in_cbc_context;


extern const br_sslrec_in_cbc_class br_sslrec_in_cbc_vtable;


typedef struct {
	
	const br_sslrec_out_cbc_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	uint64_t seq;
	union {
		const br_block_cbcenc_class *vtable;
		br_aes_gen_cbcenc_keys aes;
		br_des_gen_cbcenc_keys des;
	} bc;
	br_hmac_key_context mac;
	size_t mac_len;
	unsigned char iv[16];
	int explicit_IV;
#endif
} br_sslrec_out_cbc_context;


extern const br_sslrec_out_cbc_class br_sslrec_out_cbc_vtable;




typedef struct br_sslrec_in_gcm_class_ br_sslrec_in_gcm_class;
struct br_sslrec_in_gcm_class_ {
	
	br_sslrec_in_class inner;

	
	void (*init)(const br_sslrec_in_gcm_class **ctx,
		const br_block_ctr_class *bc_impl,
		const void *key, size_t key_len,
		br_ghash gh_impl,
		const void *iv);
};


typedef struct br_sslrec_out_gcm_class_ br_sslrec_out_gcm_class;
struct br_sslrec_out_gcm_class_ {
	
	br_sslrec_out_class inner;

	
	void (*init)(const br_sslrec_out_gcm_class **ctx,
		const br_block_ctr_class *bc_impl,
		const void *key, size_t key_len,
		br_ghash gh_impl,
		const void *iv);
};


typedef struct {
	
	union {
		const void *gen;
		const br_sslrec_in_gcm_class *in;
		const br_sslrec_out_gcm_class *out;
	} vtable;
#ifndef BR_DOXYGEN_IGNORE
	uint64_t seq;
	union {
		const br_block_ctr_class *vtable;
		br_aes_gen_ctr_keys aes;
	} bc;
	br_ghash gh;
	unsigned char iv[4];
	unsigned char h[16];
#endif
} br_sslrec_gcm_context;


extern const br_sslrec_in_gcm_class br_sslrec_in_gcm_vtable;


extern const br_sslrec_out_gcm_class br_sslrec_out_gcm_vtable;




typedef struct br_sslrec_in_chapol_class_ br_sslrec_in_chapol_class;
struct br_sslrec_in_chapol_class_ {
	
	br_sslrec_in_class inner;

	
	void (*init)(const br_sslrec_in_chapol_class **ctx,
		br_chacha20_run ichacha,
		br_poly1305_run ipoly,
		const void *key, const void *iv);
};


typedef struct br_sslrec_out_chapol_class_ br_sslrec_out_chapol_class;
struct br_sslrec_out_chapol_class_ {
	
	br_sslrec_out_class inner;

	
	void (*init)(const br_sslrec_out_chapol_class **ctx,
		br_chacha20_run ichacha,
		br_poly1305_run ipoly,
		const void *key, const void *iv);
};


typedef struct {
	
	union {
		const void *gen;
		const br_sslrec_in_chapol_class *in;
		const br_sslrec_out_chapol_class *out;
	} vtable;
#ifndef BR_DOXYGEN_IGNORE
	uint64_t seq;
	unsigned char key[32];
	unsigned char iv[12];
	br_chacha20_run ichacha;
	br_poly1305_run ipoly;
#endif
} br_sslrec_chapol_context;


extern const br_sslrec_in_chapol_class br_sslrec_in_chapol_vtable;


extern const br_sslrec_out_chapol_class br_sslrec_out_chapol_vtable;




typedef struct br_sslrec_in_ccm_class_ br_sslrec_in_ccm_class;
struct br_sslrec_in_ccm_class_ {
	
	br_sslrec_in_class inner;

	
	void (*init)(const br_sslrec_in_ccm_class **ctx,
		const br_block_ctrcbc_class *bc_impl,
		const void *key, size_t key_len,
		const void *iv, size_t tag_len);
};


typedef struct br_sslrec_out_ccm_class_ br_sslrec_out_ccm_class;
struct br_sslrec_out_ccm_class_ {
	
	br_sslrec_out_class inner;

	
	void (*init)(const br_sslrec_out_ccm_class **ctx,
		const br_block_ctrcbc_class *bc_impl,
		const void *key, size_t key_len,
		const void *iv, size_t tag_len);
};


typedef struct {
	
	union {
		const void *gen;
		const br_sslrec_in_ccm_class *in;
		const br_sslrec_out_ccm_class *out;
	} vtable;
#ifndef BR_DOXYGEN_IGNORE
	uint64_t seq;
	union {
		const br_block_ctrcbc_class *vtable;
		br_aes_gen_ctrcbc_keys aes;
	} bc;
	unsigned char iv[4];
	size_t tag_len;
#endif
} br_sslrec_ccm_context;


extern const br_sslrec_in_ccm_class br_sslrec_in_ccm_vtable;


extern const br_sslrec_out_ccm_class br_sslrec_out_ccm_vtable;




typedef struct {
	
	unsigned char session_id[32];
	
	unsigned char session_id_len;
	
	uint16_t version;
	
	uint16_t cipher_suite;
	
	unsigned char master_secret[48];
} br_ssl_session_parameters;

#ifndef BR_DOXYGEN_IGNORE

#define BR_MAX_CIPHER_SUITES   48
#endif


typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	
	int err;

	
	unsigned char *ibuf, *obuf;
	size_t ibuf_len, obuf_len;

	
	uint16_t max_frag_len;
	unsigned char log_max_frag_len;
	unsigned char peer_log_max_frag_len;

	
	size_t ixa, ixb, ixc;
	size_t oxa, oxb, oxc;
	unsigned char iomode;
	unsigned char incrypt;

	
	unsigned char shutdown_recv;

	
	unsigned char record_type_in, record_type_out;

	
	uint16_t version_in;

	
	uint16_t version_out;

	
	union {
		const br_sslrec_in_class *vtable;
		br_sslrec_in_cbc_context cbc;
		br_sslrec_gcm_context gcm;
		br_sslrec_chapol_context chapol;
		br_sslrec_ccm_context ccm;
	} in;
	union {
		const br_sslrec_out_class *vtable;
		br_sslrec_out_clear_context clear;
		br_sslrec_out_cbc_context cbc;
		br_sslrec_gcm_context gcm;
		br_sslrec_chapol_context chapol;
		br_sslrec_ccm_context ccm;
	} out;

	
	unsigned char application_data;

	
	br_hmac_drbg_context rng;
	int rng_init_done;
	int rng_os_rand_done;

	
	uint16_t version_min;
	uint16_t version_max;
	uint16_t suites_buf[BR_MAX_CIPHER_SUITES];
	unsigned char suites_num;

	
	char server_name[256];

	
	unsigned char client_random[32];
	unsigned char server_random[32];
	br_ssl_session_parameters session;

	
	unsigned char ecdhe_curve;
	unsigned char ecdhe_point[133];
	unsigned char ecdhe_point_len;

	
	unsigned char reneg;
	unsigned char saved_finished[24];

	
	uint32_t flags;

	
	struct {
		uint32_t *dp;
		uint32_t *rp;
		const unsigned char *ip;
	} cpu;
	uint32_t dp_stack[32];
	uint32_t rp_stack[32];
	unsigned char pad[512];
	unsigned char *hbuf_in, *hbuf_out, *saved_hbuf_out;
	size_t hlen_in, hlen_out;
	void (*hsrun)(void *ctx);

	
	unsigned char action;

	
	unsigned char alert;

	
	unsigned char close_received;

	
	br_multihash_context mhash;

	
	const br_x509_class **x509ctx;

	
	const br_x509_certificate *chain;
	size_t chain_len;
	const unsigned char *cert_cur;
	size_t cert_len;

	
	const char **protocol_names;
	uint16_t protocol_names_num;
	uint16_t selected_protocol;

	
	br_tls_prf_impl prf10;
	br_tls_prf_impl prf_sha256;
	br_tls_prf_impl prf_sha384;
	const br_block_cbcenc_class *iaes_cbcenc;
	const br_block_cbcdec_class *iaes_cbcdec;
	const br_block_ctr_class *iaes_ctr;
	const br_block_ctrcbc_class *iaes_ctrcbc;
	const br_block_cbcenc_class *ides_cbcenc;
	const br_block_cbcdec_class *ides_cbcdec;
	br_ghash ighash;
	br_chacha20_run ichacha;
	br_poly1305_run ipoly;
	const br_sslrec_in_cbc_class *icbc_in;
	const br_sslrec_out_cbc_class *icbc_out;
	const br_sslrec_in_gcm_class *igcm_in;
	const br_sslrec_out_gcm_class *igcm_out;
	const br_sslrec_in_chapol_class *ichapol_in;
	const br_sslrec_out_chapol_class *ichapol_out;
	const br_sslrec_in_ccm_class *iccm_in;
	const br_sslrec_out_ccm_class *iccm_out;
	const br_ec_impl *iec;
	br_rsa_pkcs1_vrfy irsavrfy;
	br_ecdsa_vrfy iecdsa;
#endif
} br_ssl_engine_context;


static inline uint32_t
br_ssl_engine_get_flags(br_ssl_engine_context *cc)
{
	return cc->flags;
}


static inline void
br_ssl_engine_set_all_flags(br_ssl_engine_context *cc, uint32_t flags)
{
	cc->flags = flags;
}


static inline void
br_ssl_engine_add_flags(br_ssl_engine_context *cc, uint32_t flags)
{
	cc->flags |= flags;
}


static inline void
br_ssl_engine_remove_flags(br_ssl_engine_context *cc, uint32_t flags)
{
	cc->flags &= ~flags;
}


#define BR_OPT_ENFORCE_SERVER_PREFERENCES      ((uint32_t)1 << 0)


#define BR_OPT_NO_RENEGOTIATION                ((uint32_t)1 << 1)


#define BR_OPT_TOLERATE_NO_CLIENT_AUTH         ((uint32_t)1 << 2)


#define BR_OPT_FAIL_ON_ALPN_MISMATCH           ((uint32_t)1 << 3)


static inline void
br_ssl_engine_set_versions(br_ssl_engine_context *cc,
	unsigned version_min, unsigned version_max)
{
	cc->version_min = (uint16_t)version_min;
	cc->version_max = (uint16_t)version_max;
}


void br_ssl_engine_set_suites(br_ssl_engine_context *cc,
	const uint16_t *suites, size_t suites_num);


static inline void
br_ssl_engine_set_x509(br_ssl_engine_context *cc, const br_x509_class **x509ctx)
{
	cc->x509ctx = x509ctx;
}


static inline void
br_ssl_engine_set_protocol_names(br_ssl_engine_context *ctx,
	const char **names, size_t num)
{
	ctx->protocol_names = names;
	ctx->protocol_names_num = (uint16_t)num;
}


static inline const char *
br_ssl_engine_get_selected_protocol(br_ssl_engine_context *ctx)
{
	unsigned k;

	k = ctx->selected_protocol;
	return (k == 0 || k == 0xFFFF) ? NULL : ctx->protocol_names[k - 1];
}


static inline void
br_ssl_engine_set_hash(br_ssl_engine_context *ctx,
	int id, const br_hash_class *impl)
{
	br_multihash_setimpl(&ctx->mhash, id, impl);
}


static inline const br_hash_class *
br_ssl_engine_get_hash(br_ssl_engine_context *ctx, int id)
{
	return br_multihash_getimpl(&ctx->mhash, id);
}


static inline void
br_ssl_engine_set_prf10(br_ssl_engine_context *cc, br_tls_prf_impl impl)
{
	cc->prf10 = impl;
}


static inline void
br_ssl_engine_set_prf_sha256(br_ssl_engine_context *cc, br_tls_prf_impl impl)
{
	cc->prf_sha256 = impl;
}


static inline void
br_ssl_engine_set_prf_sha384(br_ssl_engine_context *cc, br_tls_prf_impl impl)
{
	cc->prf_sha384 = impl;
}


static inline void
br_ssl_engine_set_aes_cbc(br_ssl_engine_context *cc,
	const br_block_cbcenc_class *impl_enc,
	const br_block_cbcdec_class *impl_dec)
{
	cc->iaes_cbcenc = impl_enc;
	cc->iaes_cbcdec = impl_dec;
}


void br_ssl_engine_set_default_aes_cbc(br_ssl_engine_context *cc);


static inline void
br_ssl_engine_set_aes_ctr(br_ssl_engine_context *cc,
	const br_block_ctr_class *impl)
{
	cc->iaes_ctr = impl;
}


void br_ssl_engine_set_default_aes_gcm(br_ssl_engine_context *cc);


static inline void
br_ssl_engine_set_des_cbc(br_ssl_engine_context *cc,
	const br_block_cbcenc_class *impl_enc,
	const br_block_cbcdec_class *impl_dec)
{
	cc->ides_cbcenc = impl_enc;
	cc->ides_cbcdec = impl_dec;
}


void br_ssl_engine_set_default_des_cbc(br_ssl_engine_context *cc);


static inline void
br_ssl_engine_set_ghash(br_ssl_engine_context *cc, br_ghash impl)
{
	cc->ighash = impl;
}


static inline void
br_ssl_engine_set_chacha20(br_ssl_engine_context *cc,
	br_chacha20_run ichacha)
{
	cc->ichacha = ichacha;
}


static inline void
br_ssl_engine_set_poly1305(br_ssl_engine_context *cc,
	br_poly1305_run ipoly)
{
	cc->ipoly = ipoly;
}


void br_ssl_engine_set_default_chapol(br_ssl_engine_context *cc);


static inline void
br_ssl_engine_set_aes_ctrcbc(br_ssl_engine_context *cc,
	const br_block_ctrcbc_class *impl)
{
	cc->iaes_ctrcbc = impl;
}


void br_ssl_engine_set_default_aes_ccm(br_ssl_engine_context *cc);


static inline void
br_ssl_engine_set_cbc(br_ssl_engine_context *cc,
	const br_sslrec_in_cbc_class *impl_in,
	const br_sslrec_out_cbc_class *impl_out)
{
	cc->icbc_in = impl_in;
	cc->icbc_out = impl_out;
}


static inline void
br_ssl_engine_set_gcm(br_ssl_engine_context *cc,
	const br_sslrec_in_gcm_class *impl_in,
	const br_sslrec_out_gcm_class *impl_out)
{
	cc->igcm_in = impl_in;
	cc->igcm_out = impl_out;
}


static inline void
br_ssl_engine_set_ccm(br_ssl_engine_context *cc,
	const br_sslrec_in_ccm_class *impl_in,
	const br_sslrec_out_ccm_class *impl_out)
{
	cc->iccm_in = impl_in;
	cc->iccm_out = impl_out;
}


static inline void
br_ssl_engine_set_chapol(br_ssl_engine_context *cc,
	const br_sslrec_in_chapol_class *impl_in,
	const br_sslrec_out_chapol_class *impl_out)
{
	cc->ichapol_in = impl_in;
	cc->ichapol_out = impl_out;
}


static inline void
br_ssl_engine_set_ec(br_ssl_engine_context *cc, const br_ec_impl *iec)
{
	cc->iec = iec;
}


void br_ssl_engine_set_default_ec(br_ssl_engine_context *cc);


static inline const br_ec_impl *
br_ssl_engine_get_ec(br_ssl_engine_context *cc)
{
	return cc->iec;
}


static inline void
br_ssl_engine_set_rsavrfy(br_ssl_engine_context *cc, br_rsa_pkcs1_vrfy irsavrfy)
{
	cc->irsavrfy = irsavrfy;
}


void br_ssl_engine_set_default_rsavrfy(br_ssl_engine_context *cc);


static inline br_rsa_pkcs1_vrfy
br_ssl_engine_get_rsavrfy(br_ssl_engine_context *cc)
{
	return cc->irsavrfy;
}


static inline void
br_ssl_engine_set_ecdsa(br_ssl_engine_context *cc, br_ecdsa_vrfy iecdsa)
{
	cc->iecdsa = iecdsa;
}


void br_ssl_engine_set_default_ecdsa(br_ssl_engine_context *cc);


static inline br_ecdsa_vrfy
br_ssl_engine_get_ecdsa(br_ssl_engine_context *cc)
{
	return cc->iecdsa;
}


void br_ssl_engine_set_buffer(br_ssl_engine_context *cc,
	void *iobuf, size_t iobuf_len, int bidi);


void br_ssl_engine_set_buffers_bidi(br_ssl_engine_context *cc,
	void *ibuf, size_t ibuf_len, void *obuf, size_t obuf_len);


void br_ssl_engine_inject_entropy(br_ssl_engine_context *cc,
	const void *data, size_t len);


static inline const char *
br_ssl_engine_get_server_name(const br_ssl_engine_context *cc)
{
	return cc->server_name;
}


static inline unsigned
br_ssl_engine_get_version(const br_ssl_engine_context *cc)
{
	return cc->session.version;
}


static inline void
br_ssl_engine_get_session_parameters(const br_ssl_engine_context *cc,
	br_ssl_session_parameters *pp)
{
	memcpy(pp, &cc->session, sizeof *pp);
}


static inline void
br_ssl_engine_set_session_parameters(br_ssl_engine_context *cc,
	const br_ssl_session_parameters *pp)
{
	memcpy(&cc->session, pp, sizeof *pp);
}


static inline int
br_ssl_engine_get_ecdhe_curve(br_ssl_engine_context *cc)
{
	return cc->ecdhe_curve;
}


unsigned br_ssl_engine_current_state(const br_ssl_engine_context *cc);


#define BR_SSL_CLOSED    0x0001

#define BR_SSL_SENDREC   0x0002

#define BR_SSL_RECVREC   0x0004

#define BR_SSL_SENDAPP   0x0008

#define BR_SSL_RECVAPP   0x0010


static inline int
br_ssl_engine_last_error(const br_ssl_engine_context *cc)
{
	return cc->err;
}




unsigned char *br_ssl_engine_sendapp_buf(
	const br_ssl_engine_context *cc, size_t *len);


void br_ssl_engine_sendapp_ack(br_ssl_engine_context *cc, size_t len);


unsigned char *br_ssl_engine_recvapp_buf(
	const br_ssl_engine_context *cc, size_t *len);


void br_ssl_engine_recvapp_ack(br_ssl_engine_context *cc, size_t len);


unsigned char *br_ssl_engine_sendrec_buf(
	const br_ssl_engine_context *cc, size_t *len);


void br_ssl_engine_sendrec_ack(br_ssl_engine_context *cc, size_t len);


unsigned char *br_ssl_engine_recvrec_buf(
	const br_ssl_engine_context *cc, size_t *len);


void br_ssl_engine_recvrec_ack(br_ssl_engine_context *cc, size_t len);


void br_ssl_engine_flush(br_ssl_engine_context *cc, int force);


void br_ssl_engine_close(br_ssl_engine_context *cc);


int br_ssl_engine_renegotiate(br_ssl_engine_context *cc);


int br_ssl_key_export(br_ssl_engine_context *cc,
	void *dst, size_t len, const char *label,
	const void *context, size_t context_len);


typedef struct br_ssl_client_context_ br_ssl_client_context;


typedef struct {
	
	int auth_type;

	
	int hash_id;

	
	const br_x509_certificate *chain;

	
	size_t chain_len;

} br_ssl_client_certificate;




#define BR_AUTH_ECDH    0

#define BR_AUTH_RSA     1

#define BR_AUTH_ECDSA   3


typedef struct br_ssl_client_certificate_class_ br_ssl_client_certificate_class;
struct br_ssl_client_certificate_class_ {
	
	size_t context_size;

	
	void (*start_name_list)(const br_ssl_client_certificate_class **pctx);

	
	void (*start_name)(const br_ssl_client_certificate_class **pctx,
		size_t len);

	
	void (*append_name)(const br_ssl_client_certificate_class **pctx,
		const unsigned char *data, size_t len);

	
	void (*end_name)(const br_ssl_client_certificate_class **pctx);

	
	void (*end_name_list)(const br_ssl_client_certificate_class **pctx);

	
	void (*choose)(const br_ssl_client_certificate_class **pctx,
		const br_ssl_client_context *cc, uint32_t auth_types,
		br_ssl_client_certificate *choices);

	
	uint32_t (*do_keyx)(const br_ssl_client_certificate_class **pctx,
		unsigned char *data, size_t *len);

	
	size_t (*do_sign)(const br_ssl_client_certificate_class **pctx,
		int hash_id, size_t hv_len, unsigned char *data, size_t len);
};


typedef struct {
	
	const br_ssl_client_certificate_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	const br_x509_certificate *chain;
	size_t chain_len;
	const br_rsa_private_key *sk;
	br_rsa_pkcs1_sign irsasign;
#endif
} br_ssl_client_certificate_rsa_context;


typedef struct {
	
	const br_ssl_client_certificate_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	const br_x509_certificate *chain;
	size_t chain_len;
	const br_ec_private_key *sk;
	unsigned allowed_usages;
	unsigned issuer_key_type;
	const br_multihash_context *mhash;
	const br_ec_impl *iec;
	br_ecdsa_sign iecdsa;
#endif
} br_ssl_client_certificate_ec_context;


struct br_ssl_client_context_ {
	
	br_ssl_engine_context eng;

#ifndef BR_DOXYGEN_IGNORE
	
	uint16_t min_clienthello_len;

	
	uint32_t hashes;

	
	int server_curve;

	
	const br_ssl_client_certificate_class **client_auth_vtable;

	
	unsigned char auth_type;

	
	unsigned char hash_id;

	
	union {
		const br_ssl_client_certificate_class *vtable;
		br_ssl_client_certificate_rsa_context single_rsa;
		br_ssl_client_certificate_ec_context single_ec;
	} client_auth;

	
	br_rsa_public irsapub;
#endif
};


static inline uint32_t
br_ssl_client_get_server_hashes(const br_ssl_client_context *cc)
{
	return cc->hashes;
}


static inline int
br_ssl_client_get_server_curve(const br_ssl_client_context *cc)
{
	return cc->server_curve;
}




void br_ssl_client_init_full(br_ssl_client_context *cc,
	br_x509_minimal_context *xc,
	const br_x509_trust_anchor *trust_anchors, size_t trust_anchors_num);


void br_ssl_client_zero(br_ssl_client_context *cc);


static inline void
br_ssl_client_set_client_certificate(br_ssl_client_context *cc,
	const br_ssl_client_certificate_class **pctx)
{
	cc->client_auth_vtable = pctx;
}


static inline void
br_ssl_client_set_rsapub(br_ssl_client_context *cc, br_rsa_public irsapub)
{
	cc->irsapub = irsapub;
}


void br_ssl_client_set_default_rsapub(br_ssl_client_context *cc);


static inline void
br_ssl_client_set_min_clienthello_len(br_ssl_client_context *cc, uint16_t len)
{
	cc->min_clienthello_len = len;
}


int br_ssl_client_reset(br_ssl_client_context *cc,
	const char *server_name, int resume_session);


static inline void
br_ssl_client_forget_session(br_ssl_client_context *cc)
{
	cc->eng.session.session_id_len = 0;
}


void br_ssl_client_set_single_rsa(br_ssl_client_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk, br_rsa_pkcs1_sign irsasign);


void br_ssl_client_set_single_ec(br_ssl_client_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk, unsigned allowed_usages,
	unsigned cert_issuer_key_type,
	const br_ec_impl *iec, br_ecdsa_sign iecdsa);


typedef uint16_t br_suite_translated[2];

#ifndef BR_DOXYGEN_IGNORE


#define BR_SSLKEYX_RSA           0
#define BR_SSLKEYX_ECDHE_RSA     1
#define BR_SSLKEYX_ECDHE_ECDSA   2
#define BR_SSLKEYX_ECDH_RSA      3
#define BR_SSLKEYX_ECDH_ECDSA    4

#define BR_SSLENC_3DES_CBC       0
#define BR_SSLENC_AES128_CBC     1
#define BR_SSLENC_AES256_CBC     2
#define BR_SSLENC_AES128_GCM     3
#define BR_SSLENC_AES256_GCM     4
#define BR_SSLENC_CHACHA20       5

#define BR_SSLMAC_AEAD           0
#define BR_SSLMAC_SHA1           br_sha1_ID
#define BR_SSLMAC_SHA256         br_sha256_ID
#define BR_SSLMAC_SHA384         br_sha384_ID

#define BR_SSLPRF_SHA256         br_sha256_ID
#define BR_SSLPRF_SHA384         br_sha384_ID

#endif


typedef struct br_ssl_server_context_ br_ssl_server_context;


typedef struct {
	
	uint16_t cipher_suite;

	
	unsigned algo_id;

	
	const br_x509_certificate *chain;

	
	size_t chain_len;

} br_ssl_server_choices;


typedef struct br_ssl_server_policy_class_ br_ssl_server_policy_class;
struct br_ssl_server_policy_class_ {
	
	size_t context_size;

	
	int (*choose)(const br_ssl_server_policy_class **pctx,
		const br_ssl_server_context *cc,
		br_ssl_server_choices *choices);

	
	uint32_t (*do_keyx)(const br_ssl_server_policy_class **pctx,
		unsigned char *data, size_t *len);

	
	size_t (*do_sign)(const br_ssl_server_policy_class **pctx,
		unsigned algo_id,
		unsigned char *data, size_t hv_len, size_t len);
};


typedef struct {
	
	const br_ssl_server_policy_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	const br_x509_certificate *chain;
	size_t chain_len;
	const br_rsa_private_key *sk;
	unsigned allowed_usages;
	br_rsa_private irsacore;
	br_rsa_pkcs1_sign irsasign;
#endif
} br_ssl_server_policy_rsa_context;


typedef struct {
	
	const br_ssl_server_policy_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	const br_x509_certificate *chain;
	size_t chain_len;
	const br_ec_private_key *sk;
	unsigned allowed_usages;
	unsigned cert_issuer_key_type;
	const br_multihash_context *mhash;
	const br_ec_impl *iec;
	br_ecdsa_sign iecdsa;
#endif
} br_ssl_server_policy_ec_context;


typedef struct br_ssl_session_cache_class_ br_ssl_session_cache_class;
struct br_ssl_session_cache_class_ {
	
	size_t context_size;

	
	void (*save)(const br_ssl_session_cache_class **ctx,
		br_ssl_server_context *server_ctx,
		const br_ssl_session_parameters *params);

	
	int (*load)(const br_ssl_session_cache_class **ctx,
		br_ssl_server_context *server_ctx,
		br_ssl_session_parameters *params);
};


typedef struct {
	
	const br_ssl_session_cache_class *vtable;
#ifndef BR_DOXYGEN_IGNORE
	unsigned char *store;
	size_t store_len, store_ptr;
	unsigned char index_key[32];
	const br_hash_class *hash;
	int init_done;
	uint32_t head, tail, root;
#endif
} br_ssl_session_cache_lru;


void br_ssl_session_cache_lru_init(br_ssl_session_cache_lru *cc,
	unsigned char *store, size_t store_len);


void br_ssl_session_cache_lru_forget(
	br_ssl_session_cache_lru *cc, const unsigned char *id);


struct br_ssl_server_context_ {
	
	br_ssl_engine_context eng;

#ifndef BR_DOXYGEN_IGNORE
	
	uint16_t client_max_version;

	
	const br_ssl_session_cache_class **cache_vtable;

	
	br_suite_translated client_suites[BR_MAX_CIPHER_SUITES];
	unsigned char client_suites_num;

	
	uint32_t hashes;

	
	uint32_t curves;

	
	const br_ssl_server_policy_class **policy_vtable;
	uint16_t sign_hash_id;

	
	union {
		const br_ssl_server_policy_class *vtable;
		br_ssl_server_policy_rsa_context single_rsa;
		br_ssl_server_policy_ec_context single_ec;
	} chain_handler;

	
	unsigned char ecdhe_key[70];
	size_t ecdhe_key_len;

	
	const br_x500_name *ta_names;
	const br_x509_trust_anchor *tas;
	size_t num_tas;
	size_t cur_dn_index;
	const unsigned char *cur_dn;
	size_t cur_dn_len;

	
	unsigned char hash_CV[64];
	size_t hash_CV_len;
	int hash_CV_id;

	
#endif
};




void br_ssl_server_init_full_rsa(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk);


void br_ssl_server_init_full_ec(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	unsigned cert_issuer_key_type, const br_ec_private_key *sk);


void br_ssl_server_init_minr2g(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk);


void br_ssl_server_init_mine2g(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk);


void br_ssl_server_init_minf2g(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk);


void br_ssl_server_init_minu2g(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk);


void br_ssl_server_init_minv2g(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk);


void br_ssl_server_init_mine2c(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk);


void br_ssl_server_init_minf2c(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk);


static inline const br_suite_translated *
br_ssl_server_get_client_suites(const br_ssl_server_context *cc, size_t *num)
{
	*num = cc->client_suites_num;
	return cc->client_suites;
}


static inline uint32_t
br_ssl_server_get_client_hashes(const br_ssl_server_context *cc)
{
	return cc->hashes;
}


static inline uint32_t
br_ssl_server_get_client_curves(const br_ssl_server_context *cc)
{
	return cc->curves;
}


void br_ssl_server_zero(br_ssl_server_context *cc);


static inline void
br_ssl_server_set_policy(br_ssl_server_context *cc,
	const br_ssl_server_policy_class **pctx)
{
	cc->policy_vtable = pctx;
}


void br_ssl_server_set_single_rsa(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_rsa_private_key *sk, unsigned allowed_usages,
	br_rsa_private irsacore, br_rsa_pkcs1_sign irsasign);


void br_ssl_server_set_single_ec(br_ssl_server_context *cc,
	const br_x509_certificate *chain, size_t chain_len,
	const br_ec_private_key *sk, unsigned allowed_usages,
	unsigned cert_issuer_key_type,
	const br_ec_impl *iec, br_ecdsa_sign iecdsa);


static inline void
br_ssl_server_set_trust_anchor_names(br_ssl_server_context *cc,
	const br_x500_name *ta_names, size_t num)
{
	cc->ta_names = ta_names;
	cc->tas = NULL;
	cc->num_tas = num;
}


static inline void
br_ssl_server_set_trust_anchor_names_alt(br_ssl_server_context *cc,
	const br_x509_trust_anchor *tas, size_t num)
{
	cc->ta_names = NULL;
	cc->tas = tas;
	cc->num_tas = num;
}


static inline void
br_ssl_server_set_cache(br_ssl_server_context *cc,
	const br_ssl_session_cache_class **vtable)
{
	cc->cache_vtable = vtable;
}


int br_ssl_server_reset(br_ssl_server_context *cc);





typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	br_ssl_engine_context *engine;
	int (*low_read)(void *read_context,
		unsigned char *data, size_t len);
	void *read_context;
	int (*low_write)(void *write_context,
		const unsigned char *data, size_t len);
	void *write_context;
#endif
} br_sslio_context;


void br_sslio_init(br_sslio_context *ctx,
	br_ssl_engine_context *engine,
	int (*low_read)(void *read_context,
		unsigned char *data, size_t len),
	void *read_context,
	int (*low_write)(void *write_context,
		const unsigned char *data, size_t len),
	void *write_context);


int br_sslio_read(br_sslio_context *cc, void *dst, size_t len);


int br_sslio_read_all(br_sslio_context *cc, void *dst, size_t len);


int br_sslio_write(br_sslio_context *cc, const void *src, size_t len);


int br_sslio_write_all(br_sslio_context *cc, const void *src, size_t len);


int br_sslio_flush(br_sslio_context *cc);


int br_sslio_close(br_sslio_context *cc);






#define BR_TLS_NULL_WITH_NULL_NULL                   0x0000
#define BR_TLS_RSA_WITH_NULL_MD5                     0x0001
#define BR_TLS_RSA_WITH_NULL_SHA                     0x0002
#define BR_TLS_RSA_WITH_NULL_SHA256                  0x003B
#define BR_TLS_RSA_WITH_RC4_128_MD5                  0x0004
#define BR_TLS_RSA_WITH_RC4_128_SHA                  0x0005
#define BR_TLS_RSA_WITH_3DES_EDE_CBC_SHA             0x000A
#define BR_TLS_RSA_WITH_AES_128_CBC_SHA              0x002F
#define BR_TLS_RSA_WITH_AES_256_CBC_SHA              0x0035
#define BR_TLS_RSA_WITH_AES_128_CBC_SHA256           0x003C
#define BR_TLS_RSA_WITH_AES_256_CBC_SHA256           0x003D
#define BR_TLS_DH_DSS_WITH_3DES_EDE_CBC_SHA          0x000D
#define BR_TLS_DH_RSA_WITH_3DES_EDE_CBC_SHA          0x0010
#define BR_TLS_DHE_DSS_WITH_3DES_EDE_CBC_SHA         0x0013
#define BR_TLS_DHE_RSA_WITH_3DES_EDE_CBC_SHA         0x0016
#define BR_TLS_DH_DSS_WITH_AES_128_CBC_SHA           0x0030
#define BR_TLS_DH_RSA_WITH_AES_128_CBC_SHA           0x0031
#define BR_TLS_DHE_DSS_WITH_AES_128_CBC_SHA          0x0032
#define BR_TLS_DHE_RSA_WITH_AES_128_CBC_SHA          0x0033
#define BR_TLS_DH_DSS_WITH_AES_256_CBC_SHA           0x0036
#define BR_TLS_DH_RSA_WITH_AES_256_CBC_SHA           0x0037
#define BR_TLS_DHE_DSS_WITH_AES_256_CBC_SHA          0x0038
#define BR_TLS_DHE_RSA_WITH_AES_256_CBC_SHA          0x0039
#define BR_TLS_DH_DSS_WITH_AES_128_CBC_SHA256        0x003E
#define BR_TLS_DH_RSA_WITH_AES_128_CBC_SHA256        0x003F
#define BR_TLS_DHE_DSS_WITH_AES_128_CBC_SHA256       0x0040
#define BR_TLS_DHE_RSA_WITH_AES_128_CBC_SHA256       0x0067
#define BR_TLS_DH_DSS_WITH_AES_256_CBC_SHA256        0x0068
#define BR_TLS_DH_RSA_WITH_AES_256_CBC_SHA256        0x0069
#define BR_TLS_DHE_DSS_WITH_AES_256_CBC_SHA256       0x006A
#define BR_TLS_DHE_RSA_WITH_AES_256_CBC_SHA256       0x006B
#define BR_TLS_DH_anon_WITH_RC4_128_MD5              0x0018
#define BR_TLS_DH_anon_WITH_3DES_EDE_CBC_SHA         0x001B
#define BR_TLS_DH_anon_WITH_AES_128_CBC_SHA          0x0034
#define BR_TLS_DH_anon_WITH_AES_256_CBC_SHA          0x003A
#define BR_TLS_DH_anon_WITH_AES_128_CBC_SHA256       0x006C
#define BR_TLS_DH_anon_WITH_AES_256_CBC_SHA256       0x006D


#define BR_TLS_ECDH_ECDSA_WITH_NULL_SHA              0xC001
#define BR_TLS_ECDH_ECDSA_WITH_RC4_128_SHA           0xC002
#define BR_TLS_ECDH_ECDSA_WITH_3DES_EDE_CBC_SHA      0xC003
#define BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA       0xC004
#define BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA       0xC005
#define BR_TLS_ECDHE_ECDSA_WITH_NULL_SHA             0xC006
#define BR_TLS_ECDHE_ECDSA_WITH_RC4_128_SHA          0xC007
#define BR_TLS_ECDHE_ECDSA_WITH_3DES_EDE_CBC_SHA     0xC008
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA      0xC009
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA      0xC00A
#define BR_TLS_ECDH_RSA_WITH_NULL_SHA                0xC00B
#define BR_TLS_ECDH_RSA_WITH_RC4_128_SHA             0xC00C
#define BR_TLS_ECDH_RSA_WITH_3DES_EDE_CBC_SHA        0xC00D
#define BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA         0xC00E
#define BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA         0xC00F
#define BR_TLS_ECDHE_RSA_WITH_NULL_SHA               0xC010
#define BR_TLS_ECDHE_RSA_WITH_RC4_128_SHA            0xC011
#define BR_TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA       0xC012
#define BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA        0xC013
#define BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA        0xC014
#define BR_TLS_ECDH_anon_WITH_NULL_SHA               0xC015
#define BR_TLS_ECDH_anon_WITH_RC4_128_SHA            0xC016
#define BR_TLS_ECDH_anon_WITH_3DES_EDE_CBC_SHA       0xC017
#define BR_TLS_ECDH_anon_WITH_AES_128_CBC_SHA        0xC018
#define BR_TLS_ECDH_anon_WITH_AES_256_CBC_SHA        0xC019


#define BR_TLS_RSA_WITH_AES_128_GCM_SHA256           0x009C
#define BR_TLS_RSA_WITH_AES_256_GCM_SHA384           0x009D
#define BR_TLS_DHE_RSA_WITH_AES_128_GCM_SHA256       0x009E
#define BR_TLS_DHE_RSA_WITH_AES_256_GCM_SHA384       0x009F
#define BR_TLS_DH_RSA_WITH_AES_128_GCM_SHA256        0x00A0
#define BR_TLS_DH_RSA_WITH_AES_256_GCM_SHA384        0x00A1
#define BR_TLS_DHE_DSS_WITH_AES_128_GCM_SHA256       0x00A2
#define BR_TLS_DHE_DSS_WITH_AES_256_GCM_SHA384       0x00A3
#define BR_TLS_DH_DSS_WITH_AES_128_GCM_SHA256        0x00A4
#define BR_TLS_DH_DSS_WITH_AES_256_GCM_SHA384        0x00A5
#define BR_TLS_DH_anon_WITH_AES_128_GCM_SHA256       0x00A6
#define BR_TLS_DH_anon_WITH_AES_256_GCM_SHA384       0x00A7


#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_CBC_SHA256   0xC023
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_CBC_SHA384   0xC024
#define BR_TLS_ECDH_ECDSA_WITH_AES_128_CBC_SHA256    0xC025
#define BR_TLS_ECDH_ECDSA_WITH_AES_256_CBC_SHA384    0xC026
#define BR_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256     0xC027
#define BR_TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384     0xC028
#define BR_TLS_ECDH_RSA_WITH_AES_128_CBC_SHA256      0xC029
#define BR_TLS_ECDH_RSA_WITH_AES_256_CBC_SHA384      0xC02A
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256   0xC02B
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384   0xC02C
#define BR_TLS_ECDH_ECDSA_WITH_AES_128_GCM_SHA256    0xC02D
#define BR_TLS_ECDH_ECDSA_WITH_AES_256_GCM_SHA384    0xC02E
#define BR_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256     0xC02F
#define BR_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384     0xC030
#define BR_TLS_ECDH_RSA_WITH_AES_128_GCM_SHA256      0xC031
#define BR_TLS_ECDH_RSA_WITH_AES_256_GCM_SHA384      0xC032


#define BR_TLS_RSA_WITH_AES_128_CCM                  0xC09C
#define BR_TLS_RSA_WITH_AES_256_CCM                  0xC09D
#define BR_TLS_RSA_WITH_AES_128_CCM_8                0xC0A0
#define BR_TLS_RSA_WITH_AES_256_CCM_8                0xC0A1
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM          0xC0AC
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM          0xC0AD
#define BR_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8        0xC0AE
#define BR_TLS_ECDHE_ECDSA_WITH_AES_256_CCM_8        0xC0AF


#define BR_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256     0xCCA8
#define BR_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256   0xCCA9
#define BR_TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256       0xCCAA
#define BR_TLS_PSK_WITH_CHACHA20_POLY1305_SHA256           0xCCAB
#define BR_TLS_ECDHE_PSK_WITH_CHACHA20_POLY1305_SHA256     0xCCAC
#define BR_TLS_DHE_PSK_WITH_CHACHA20_POLY1305_SHA256       0xCCAD
#define BR_TLS_RSA_PSK_WITH_CHACHA20_POLY1305_SHA256       0xCCAE


#define BR_TLS_FALLBACK_SCSV                         0x5600


#define BR_ALERT_CLOSE_NOTIFY                0
#define BR_ALERT_UNEXPECTED_MESSAGE         10
#define BR_ALERT_BAD_RECORD_MAC             20
#define BR_ALERT_RECORD_OVERFLOW            22
#define BR_ALERT_DECOMPRESSION_FAILURE      30
#define BR_ALERT_HANDSHAKE_FAILURE          40
#define BR_ALERT_BAD_CERTIFICATE            42
#define BR_ALERT_UNSUPPORTED_CERTIFICATE    43
#define BR_ALERT_CERTIFICATE_REVOKED        44
#define BR_ALERT_CERTIFICATE_EXPIRED        45
#define BR_ALERT_CERTIFICATE_UNKNOWN        46
#define BR_ALERT_ILLEGAL_PARAMETER          47
#define BR_ALERT_UNKNOWN_CA                 48
#define BR_ALERT_ACCESS_DENIED              49
#define BR_ALERT_DECODE_ERROR               50
#define BR_ALERT_DECRYPT_ERROR              51
#define BR_ALERT_PROTOCOL_VERSION           70
#define BR_ALERT_INSUFFICIENT_SECURITY      71
#define BR_ALERT_INTERNAL_ERROR             80
#define BR_ALERT_USER_CANCELED              90
#define BR_ALERT_NO_RENEGOTIATION          100
#define BR_ALERT_UNSUPPORTED_EXTENSION     110
#define BR_ALERT_NO_APPLICATION_PROTOCOL   120

#ifdef __cplusplus
}
#endif

#endif

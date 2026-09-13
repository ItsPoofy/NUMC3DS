

#ifndef BR_BEARSSL_EC_H__
#define BR_BEARSSL_EC_H__

#include <stddef.h>
#include <stdint.h>

#include "bearssl_rand.h"

#ifdef __cplusplus
extern "C" {
#endif






#define BR_EC_sect163k1           1


#define BR_EC_sect163r1           2


#define BR_EC_sect163r2           3


#define BR_EC_sect193r1           4


#define BR_EC_sect193r2           5


#define BR_EC_sect233k1           6


#define BR_EC_sect233r1           7


#define BR_EC_sect239k1           8


#define BR_EC_sect283k1           9


#define BR_EC_sect283r1          10


#define BR_EC_sect409k1          11


#define BR_EC_sect409r1          12


#define BR_EC_sect571k1          13


#define BR_EC_sect571r1          14


#define BR_EC_secp160k1          15


#define BR_EC_secp160r1          16


#define BR_EC_secp160r2          17


#define BR_EC_secp192k1          18


#define BR_EC_secp192r1          19


#define BR_EC_secp224k1          20


#define BR_EC_secp224r1          21


#define BR_EC_secp256k1          22


#define BR_EC_secp256r1          23


#define BR_EC_secp384r1          24


#define BR_EC_secp521r1          25


#define BR_EC_brainpoolP256r1    26


#define BR_EC_brainpoolP384r1    27


#define BR_EC_brainpoolP512r1    28


#define BR_EC_curve25519         29


#define BR_EC_curve448           30


typedef struct {
	
	int curve;
	
	unsigned char *q;
	
	size_t qlen;
} br_ec_public_key;


typedef struct {
	
	int curve;
	
	unsigned char *x;
	
	size_t xlen;
} br_ec_private_key;


typedef struct {
	
	uint32_t supported_curves;

	
	const unsigned char *(*generator)(int curve, size_t *len);

	
	const unsigned char *(*order)(int curve, size_t *len);

	
	size_t (*xoff)(int curve, size_t *len);

	
	uint32_t (*mul)(unsigned char *G, size_t Glen,
		const unsigned char *x, size_t xlen, int curve);

	
	size_t (*mulgen)(unsigned char *R,
		const unsigned char *x, size_t xlen, int curve);

	
	uint32_t (*muladd)(unsigned char *A, const unsigned char *B, size_t len,
		const unsigned char *x, size_t xlen,
		const unsigned char *y, size_t ylen, int curve);
} br_ec_impl;


extern const br_ec_impl br_ec_prime_i31;


extern const br_ec_impl br_ec_prime_i15;


extern const br_ec_impl br_ec_p256_m15;


extern const br_ec_impl br_ec_p256_m31;


extern const br_ec_impl br_ec_p256_m62;


const br_ec_impl *br_ec_p256_m62_get(void);


extern const br_ec_impl br_ec_p256_m64;


const br_ec_impl *br_ec_p256_m64_get(void);


extern const br_ec_impl br_ec_c25519_i15;


extern const br_ec_impl br_ec_c25519_i31;


extern const br_ec_impl br_ec_c25519_m15;


extern const br_ec_impl br_ec_c25519_m31;


extern const br_ec_impl br_ec_c25519_m62;


const br_ec_impl *br_ec_c25519_m62_get(void);


extern const br_ec_impl br_ec_c25519_m64;


const br_ec_impl *br_ec_c25519_m64_get(void);


extern const br_ec_impl br_ec_all_m15;


extern const br_ec_impl br_ec_all_m31;


const br_ec_impl *br_ec_get_default(void);


size_t br_ecdsa_raw_to_asn1(void *sig, size_t sig_len);


size_t br_ecdsa_asn1_to_raw(void *sig, size_t sig_len);


typedef size_t (*br_ecdsa_sign)(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig);


typedef uint32_t (*br_ecdsa_vrfy)(const br_ec_impl *impl,
	const void *hash, size_t hash_len,
	const br_ec_public_key *pk, const void *sig, size_t sig_len);


size_t br_ecdsa_i31_sign_asn1(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig);


size_t br_ecdsa_i31_sign_raw(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig);


uint32_t br_ecdsa_i31_vrfy_asn1(const br_ec_impl *impl,
	const void *hash, size_t hash_len,
	const br_ec_public_key *pk, const void *sig, size_t sig_len);


uint32_t br_ecdsa_i31_vrfy_raw(const br_ec_impl *impl,
	const void *hash, size_t hash_len,
	const br_ec_public_key *pk, const void *sig, size_t sig_len);


size_t br_ecdsa_i15_sign_asn1(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig);


size_t br_ecdsa_i15_sign_raw(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig);


uint32_t br_ecdsa_i15_vrfy_asn1(const br_ec_impl *impl,
	const void *hash, size_t hash_len,
	const br_ec_public_key *pk, const void *sig, size_t sig_len);


uint32_t br_ecdsa_i15_vrfy_raw(const br_ec_impl *impl,
	const void *hash, size_t hash_len,
	const br_ec_public_key *pk, const void *sig, size_t sig_len);


br_ecdsa_sign br_ecdsa_sign_asn1_get_default(void);


br_ecdsa_sign br_ecdsa_sign_raw_get_default(void);


br_ecdsa_vrfy br_ecdsa_vrfy_asn1_get_default(void);


br_ecdsa_vrfy br_ecdsa_vrfy_raw_get_default(void);


#define BR_EC_KBUF_PRIV_MAX_SIZE   72


#define BR_EC_KBUF_PUB_MAX_SIZE    145


size_t br_ec_keygen(const br_prng_class **rng_ctx,
	const br_ec_impl *impl, br_ec_private_key *sk,
	void *kbuf, int curve);


size_t br_ec_compute_pub(const br_ec_impl *impl, br_ec_public_key *pk,
	void *kbuf, const br_ec_private_key *sk);

#ifdef __cplusplus
}
#endif

#endif

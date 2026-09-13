

#ifndef BR_BEARSSL_KDF_H__
#define BR_BEARSSL_KDF_H__

#include <stddef.h>
#include <stdint.h>

#include "bearssl_hash.h"
#include "bearssl_hmac.h"

#ifdef __cplusplus
extern "C" {
#endif




typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	union {
		br_hmac_context hmac_ctx;
		br_hmac_key_context prk_ctx;
	} u;
	unsigned char buf[64];
	size_t ptr;
	size_t dig_len;
	unsigned chunk_num;
#endif
} br_hkdf_context;


void br_hkdf_init(br_hkdf_context *hc, const br_hash_class *digest_vtable,
	const void *salt, size_t salt_len);


#define BR_HKDF_NO_SALT   (&br_hkdf_no_salt)

#ifndef BR_DOXYGEN_IGNORE
extern const unsigned char br_hkdf_no_salt;
#endif


void br_hkdf_inject(br_hkdf_context *hc, const void *ikm, size_t ikm_len);


void br_hkdf_flip(br_hkdf_context *hc);


size_t br_hkdf_produce(br_hkdf_context *hc,
	const void *info, size_t info_len, void *out, size_t out_len);


typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	unsigned char dbuf[200];
	size_t dptr;
	size_t rate;
	uint64_t A[25];
#endif
} br_shake_context;


void br_shake_init(br_shake_context *sc, int security_level);


void br_shake_inject(br_shake_context *sc, const void *data, size_t len);


void br_shake_flip(br_shake_context *hc);


void br_shake_produce(br_shake_context *sc, void *out, size_t len);

#ifdef __cplusplus
}
#endif

#endif

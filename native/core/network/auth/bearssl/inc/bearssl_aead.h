

#ifndef BR_BEARSSL_AEAD_H__
#define BR_BEARSSL_AEAD_H__

#include <stddef.h>
#include <stdint.h>

#include "bearssl_block.h"
#include "bearssl_hash.h"

#ifdef __cplusplus
extern "C" {
#endif




typedef struct br_aead_class_ br_aead_class;
struct br_aead_class_ {

	
	size_t tag_size;

	
	void (*reset)(const br_aead_class **cc, const void *iv, size_t len);

	
	void (*aad_inject)(const br_aead_class **cc,
		const void *data, size_t len);

	
	void (*flip)(const br_aead_class **cc);

	
	void (*run)(const br_aead_class **cc, int encrypt,
		void *data, size_t len);

	
	void (*get_tag)(const br_aead_class **cc, void *tag);

	
	uint32_t (*check_tag)(const br_aead_class **cc, const void *tag);

	
	void (*get_tag_trunc)(const br_aead_class **cc, void *tag, size_t len);

	
	uint32_t (*check_tag_trunc)(const br_aead_class **cc,
		const void *tag, size_t len);
};


typedef struct {
	
	const br_aead_class *vtable;

#ifndef BR_DOXYGEN_IGNORE
	const br_block_ctr_class **bctx;
	br_ghash gh;
	unsigned char h[16];
	unsigned char j0_1[12];
	unsigned char buf[16];
	unsigned char y[16];
	uint32_t j0_2, jc;
	uint64_t count_aad, count_ctr;
#endif
} br_gcm_context;


void br_gcm_init(br_gcm_context *ctx,
	const br_block_ctr_class **bctx, br_ghash gh);


void br_gcm_reset(br_gcm_context *ctx, const void *iv, size_t len);


void br_gcm_aad_inject(br_gcm_context *ctx, const void *data, size_t len);


void br_gcm_flip(br_gcm_context *ctx);


void br_gcm_run(br_gcm_context *ctx, int encrypt, void *data, size_t len);


void br_gcm_get_tag(br_gcm_context *ctx, void *tag);


uint32_t br_gcm_check_tag(br_gcm_context *ctx, const void *tag);


void br_gcm_get_tag_trunc(br_gcm_context *ctx, void *tag, size_t len);


uint32_t br_gcm_check_tag_trunc(br_gcm_context *ctx,
	const void *tag, size_t len);


extern const br_aead_class br_gcm_vtable;


typedef struct {
	
	const br_aead_class *vtable;

#ifndef BR_DOXYGEN_IGNORE
	const br_block_ctrcbc_class **bctx;
	unsigned char L2[16];
	unsigned char L4[16];
	unsigned char nonce[16];
	unsigned char head[16];
	unsigned char ctr[16];
	unsigned char cbcmac[16];
	unsigned char buf[16];
	size_t ptr;
#endif
} br_eax_context;


typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	unsigned char st[3][16];
#endif
} br_eax_state;


void br_eax_init(br_eax_context *ctx, const br_block_ctrcbc_class **bctx);


void br_eax_capture(const br_eax_context *ctx, br_eax_state *st);


void br_eax_reset(br_eax_context *ctx, const void *nonce, size_t len);


void br_eax_reset_pre_aad(br_eax_context *ctx, const br_eax_state *st,
	const void *nonce, size_t len);


void br_eax_reset_post_aad(br_eax_context *ctx, const br_eax_state *st,
	const void *nonce, size_t len);


void br_eax_aad_inject(br_eax_context *ctx, const void *data, size_t len);


void br_eax_flip(br_eax_context *ctx);


static inline void
br_eax_get_aad_mac(const br_eax_context *ctx, br_eax_state *st)
{
	memcpy(st->st[1], ctx->head, sizeof ctx->head);
}


void br_eax_run(br_eax_context *ctx, int encrypt, void *data, size_t len);


void br_eax_get_tag(br_eax_context *ctx, void *tag);


uint32_t br_eax_check_tag(br_eax_context *ctx, const void *tag);


void br_eax_get_tag_trunc(br_eax_context *ctx, void *tag, size_t len);


uint32_t br_eax_check_tag_trunc(br_eax_context *ctx,
	const void *tag, size_t len);


extern const br_aead_class br_eax_vtable;


typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	const br_block_ctrcbc_class **bctx;
	unsigned char ctr[16];
	unsigned char cbcmac[16];
	unsigned char tagmask[16];
	unsigned char buf[16];
	size_t ptr;
	size_t tag_len;
#endif
} br_ccm_context;


void br_ccm_init(br_ccm_context *ctx, const br_block_ctrcbc_class **bctx);


int br_ccm_reset(br_ccm_context *ctx, const void *nonce, size_t nonce_len,
	uint64_t aad_len, uint64_t data_len, size_t tag_len);


void br_ccm_aad_inject(br_ccm_context *ctx, const void *data, size_t len);


void br_ccm_flip(br_ccm_context *ctx);


void br_ccm_run(br_ccm_context *ctx, int encrypt, void *data, size_t len);


size_t br_ccm_get_tag(br_ccm_context *ctx, void *tag);


uint32_t br_ccm_check_tag(br_ccm_context *ctx, const void *tag);

#ifdef __cplusplus
}
#endif

#endif

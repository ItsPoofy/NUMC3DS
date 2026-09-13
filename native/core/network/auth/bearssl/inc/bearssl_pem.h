

#ifndef BR_BEARSSL_PEM_H__
#define BR_BEARSSL_PEM_H__

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif




typedef struct {
#ifndef BR_DOXYGEN_IGNORE
	
	struct {
		uint32_t *dp;
		uint32_t *rp;
		const unsigned char *ip;
	} cpu;
	uint32_t dp_stack[32];
	uint32_t rp_stack[32];
	int err;

	const unsigned char *hbuf;
	size_t hlen;

	void (*dest)(void *dest_ctx, const void *src, size_t len);
	void *dest_ctx;

	unsigned char event;
	char name[128];
	unsigned char buf[255];
	size_t ptr;
#endif
} br_pem_decoder_context;


void br_pem_decoder_init(br_pem_decoder_context *ctx);


size_t br_pem_decoder_push(br_pem_decoder_context *ctx,
	const void *data, size_t len);


static inline void
br_pem_decoder_setdest(br_pem_decoder_context *ctx,
	void (*dest)(void *dest_ctx, const void *src, size_t len),
	void *dest_ctx)
{
	ctx->dest = dest;
	ctx->dest_ctx = dest_ctx;
}


int br_pem_decoder_event(br_pem_decoder_context *ctx);


#define BR_PEM_BEGIN_OBJ   1


#define BR_PEM_END_OBJ     2


#define BR_PEM_ERROR       3


static inline const char *
br_pem_decoder_name(br_pem_decoder_context *ctx)
{
	return ctx->name;
}


size_t br_pem_encode(void *dest, const void *data, size_t len,
	const char *banner, unsigned flags);


#define BR_PEM_LINE64   0x0001


#define BR_PEM_CRLF     0x0002

#ifdef __cplusplus
}
#endif

#endif

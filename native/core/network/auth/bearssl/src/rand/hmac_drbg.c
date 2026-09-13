

#include "inner.h"


void
br_hmac_drbg_init(br_hmac_drbg_context *ctx,
	const br_hash_class *digest_class, const void *seed, size_t len)
{
	size_t hlen;

	ctx->vtable = &br_hmac_drbg_vtable;
	hlen = br_digest_size(digest_class);
	memset(ctx->K, 0x00, hlen);
	memset(ctx->V, 0x01, hlen);
	ctx->digest_class = digest_class;
	br_hmac_drbg_update(ctx, seed, len);
}


void
br_hmac_drbg_generate(br_hmac_drbg_context *ctx, void *out, size_t len)
{
	const br_hash_class *dig;
	br_hmac_key_context kc;
	br_hmac_context hc;
	size_t hlen;
	unsigned char *buf;
	unsigned char x;

	dig = ctx->digest_class;
	hlen = br_digest_size(dig);
	br_hmac_key_init(&kc, dig, ctx->K, hlen);
	buf = out;
	while (len > 0) {
		size_t clen;

		br_hmac_init(&hc, &kc, 0);
		br_hmac_update(&hc, ctx->V, hlen);
		br_hmac_out(&hc, ctx->V);
		clen = hlen;
		if (clen > len) {
			clen = len;
		}
		memcpy(buf, ctx->V, clen);
		buf += clen;
		len -= clen;
	}

	
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	x = 0x00;
	br_hmac_update(&hc, &x, 1);
	br_hmac_out(&hc, ctx->K);
	br_hmac_key_init(&kc, dig, ctx->K, hlen);
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	br_hmac_out(&hc, ctx->V);
}


void
br_hmac_drbg_update(br_hmac_drbg_context *ctx, const void *seed, size_t len)
{
	const br_hash_class *dig;
	br_hmac_key_context kc;
	br_hmac_context hc;
	size_t hlen;
	unsigned char x;

	dig = ctx->digest_class;
	hlen = br_digest_size(dig);

	
	br_hmac_key_init(&kc, dig, ctx->K, hlen);
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	x = 0x00;
	br_hmac_update(&hc, &x, 1);
	br_hmac_update(&hc, seed, len);
	br_hmac_out(&hc, ctx->K);
	br_hmac_key_init(&kc, dig, ctx->K, hlen);

	
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	br_hmac_out(&hc, ctx->V);

	
	if (len == 0) {
		return;
	}

	
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	x = 0x01;
	br_hmac_update(&hc, &x, 1);
	br_hmac_update(&hc, seed, len);
	br_hmac_out(&hc, ctx->K);
	br_hmac_key_init(&kc, dig, ctx->K, hlen);

	
	br_hmac_init(&hc, &kc, 0);
	br_hmac_update(&hc, ctx->V, hlen);
	br_hmac_out(&hc, ctx->V);
}


const br_prng_class br_hmac_drbg_vtable = {
	sizeof(br_hmac_drbg_context),
	(void (*)(const br_prng_class **, const void *, const void *, size_t))
		&br_hmac_drbg_init,
	(void (*)(const br_prng_class **, void *, size_t))
		&br_hmac_drbg_generate,
	(void (*)(const br_prng_class **, const void *, size_t))
		&br_hmac_drbg_update
};

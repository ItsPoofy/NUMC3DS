

#include "inner.h"

#define I31_LEN     ((BR_MAX_EC_SIZE + 61) / 31)
#define POINT_LEN   (1 + (((BR_MAX_EC_SIZE + 7) >> 3) << 1))
#define ORDER_LEN   ((BR_MAX_EC_SIZE + 7) >> 3)


size_t
br_ecdsa_i31_sign_raw(const br_ec_impl *impl,
	const br_hash_class *hf, const void *hash_value,
	const br_ec_private_key *sk, void *sig)
{
	
	const br_ec_curve_def *cd;
	uint32_t n[I31_LEN], r[I31_LEN], s[I31_LEN], x[I31_LEN];
	uint32_t m[I31_LEN], k[I31_LEN], t1[I31_LEN], t2[I31_LEN];
	unsigned char tt[ORDER_LEN << 1];
	unsigned char eU[POINT_LEN];
	size_t hash_len, nlen, ulen;
	uint32_t n0i, ctl;
	br_hmac_drbg_context drbg;

	
	if (((impl->supported_curves >> sk->curve) & 1) == 0) {
		return 0;
	}

	
	switch (sk->curve) {
	case BR_EC_secp256r1:
		cd = &br_secp256r1;
		break;
	case BR_EC_secp384r1:
		cd = &br_secp384r1;
		break;
	case BR_EC_secp521r1:
		cd = &br_secp521r1;
		break;
	default:
		return 0;
	}

	
	nlen = cd->order_len;
	br_i31_decode(n, cd->order, nlen);
	n0i = br_i31_ninv31(n[1]);

	
	if (!br_i31_decode_mod(x, sk->x, sk->xlen, n)) {
		return 0;
	}
	if (br_i31_iszero(x)) {
		return 0;
	}

	
	hash_len = (hf->desc >> BR_HASHDESC_OUT_OFF) & BR_HASHDESC_OUT_MASK;

	
	br_ecdsa_i31_bits2int(m, hash_value, hash_len, n[0]);
	br_i31_sub(m, n, br_i31_sub(m, n, 0) ^ 1);

	
	br_i31_encode(tt, nlen, x);
	br_i31_encode(tt + nlen, nlen, m);
	br_hmac_drbg_init(&drbg, hf, tt, nlen << 1);
	for (;;) {
		br_hmac_drbg_generate(&drbg, tt, nlen);
		br_ecdsa_i31_bits2int(k, tt, nlen, n[0]);
		if (br_i31_iszero(k)) {
			continue;
		}
		if (br_i31_sub(k, n, 0)) {
			break;
		}
	}

	
	br_i31_encode(tt, nlen, k);
	ulen = impl->mulgen(eU, tt, nlen, sk->curve);
	br_i31_zero(r, n[0]);
	br_i31_decode(r, &eU[1], ulen >> 1);
	r[0] = n[0];
	br_i31_sub(r, n, br_i31_sub(r, n, 0) ^ 1);

	
	br_i31_from_monty(k, n, n0i);
	br_i31_from_monty(k, n, n0i);
	memcpy(tt, cd->order, nlen);
	tt[nlen - 1] -= 2;
	br_i31_modpow(k, tt, nlen, n, n0i, t1, t2);

	
	br_i31_from_monty(m, n, n0i);
	br_i31_montymul(t1, x, r, n, n0i);
	ctl = br_i31_add(t1, m, 1);
	ctl |= br_i31_sub(t1, n, 0) ^ 1;
	br_i31_sub(t1, n, ctl);
	br_i31_montymul(s, t1, k, n, n0i);

	
	br_i31_encode(sig, nlen, r);
	br_i31_encode((unsigned char *)sig + nlen, nlen, s);
	return nlen << 1;
}

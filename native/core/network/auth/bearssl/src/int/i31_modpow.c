

#include "inner.h"


void
br_i31_modpow(uint32_t *x,
	const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *t1, uint32_t *t2)
{
	size_t mlen;
	uint32_t k;

	
	mlen = ((m[0] + 63) >> 5) * sizeof m[0];

	
	memcpy(t1, x, mlen);
	br_i31_to_monty(t1, m);
	br_i31_zero(x, m[0]);
	x[1] = 1;
	for (k = 0; k < ((uint32_t)elen << 3); k ++) {
		uint32_t ctl;

		ctl = (e[elen - 1 - (k >> 3)] >> (k & 7)) & 1;
		br_i31_montymul(t2, x, t1, m, m0i);
		CCOPY(ctl, x, t2, mlen);
		br_i31_montymul(t2, t1, t1, m, m0i);
		memcpy(t1, t2, mlen);
	}
}

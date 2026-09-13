

#include "inner.h"


void
br_ecdsa_i31_bits2int(uint32_t *x,
	const void *src, size_t len, uint32_t ebitlen)
{
	uint32_t bitlen, hbitlen;
	int sc;

	bitlen = ebitlen - (ebitlen >> 5);
	hbitlen = (uint32_t)len << 3;
	if (hbitlen > bitlen) {
		len = (bitlen + 7) >> 3;
		sc = (int)((hbitlen - bitlen) & 7);
	} else {
		sc = 0;
	}
	br_i31_zero(x, ebitlen);
	br_i31_decode(x, src, len);
	br_i31_rshift(x, sc);
	x[0] = ebitlen;
}

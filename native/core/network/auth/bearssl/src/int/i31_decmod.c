

#include "inner.h"


uint32_t
br_i31_decode_mod(uint32_t *x, const void *src, size_t len, const uint32_t *m)
{
	
	const unsigned char *buf;
	size_t mlen, tlen;
	int pass;
	uint32_t r;

	buf = src;
	mlen = (m[0] + 31) >> 5;
	tlen = (mlen << 2);
	if (tlen < len) {
		tlen = len;
	}
	tlen += 4;
	r = 0;
	for (pass = 0; pass < 2; pass ++) {
		size_t u, v;
		uint32_t acc;
		int acc_len;

		v = 1;
		acc = 0;
		acc_len = 0;
		for (u = 0; u < tlen; u ++) {
			uint32_t b;

			if (u < len) {
				b = buf[len - 1 - u];
			} else {
				b = 0;
			}
			acc |= (b << acc_len);
			acc_len += 8;
			if (acc_len >= 31) {
				uint32_t xw;

				xw = acc & (uint32_t)0x7FFFFFFF;
				acc_len -= 31;
				acc = b >> (8 - acc_len);
				if (v <= mlen) {
					if (pass) {
						x[v] = r & xw;
					} else {
						uint32_t cc;

						cc = (uint32_t)CMP(xw, m[v]);
						r = MUX(EQ(cc, 0), r, cc);
					}
				} else {
					if (!pass) {
						r = MUX(EQ(xw, 0), r, 1);
					}
				}
				v ++;
			}
		}

		
		r >>= 1;
		r |= (r << 1);
	}

	x[0] = m[0];
	return r & (uint32_t)1;
}

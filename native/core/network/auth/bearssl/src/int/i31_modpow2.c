

#include "inner.h"


uint32_t
br_i31_modpow_opt(uint32_t *x,
	const unsigned char *e, size_t elen,
	const uint32_t *m, uint32_t m0i, uint32_t *tmp, size_t twlen)
{
	size_t mlen, mwlen;
	uint32_t *t1, *t2, *base;
	size_t u, v;
	uint32_t acc;
	int acc_len, win_len;

	
	mwlen = (m[0] + 63) >> 5;
	mlen = mwlen * sizeof m[0];
	mwlen += (mwlen & 1);
	t1 = tmp;
	t2 = tmp + mwlen;

	
	if (twlen < (mwlen << 1)) {
		return 0;
	}
	for (win_len = 5; win_len > 1; win_len --) {
		if ((((uint32_t)1 << win_len) + 1) * mwlen <= twlen) {
			break;
		}
	}

	
	br_i31_to_monty(x, m);

	
	if (win_len == 1) {
		memcpy(t2, x, mlen);
	} else {
		memcpy(t2 + mwlen, x, mlen);
		base = t2 + mwlen;
		for (u = 2; u < ((unsigned)1 << win_len); u ++) {
			br_i31_montymul(base + mwlen, base, x, m, m0i);
			base += mwlen;
		}
	}

	
	br_i31_zero(x, m[0]);
	x[(m[0] + 31) >> 5] = 1;
	br_i31_muladd_small(x, 0, m);

	
	acc = 0;
	acc_len = 0;
	while (acc_len > 0 || elen > 0) {
		int i, k;
		uint32_t bits;

		
		k = win_len;
		if (acc_len < win_len) {
			if (elen > 0) {
				acc = (acc << 8) | *e ++;
				elen --;
				acc_len += 8;
			} else {
				k = acc_len;
			}
		}
		bits = (acc >> (acc_len - k)) & (((uint32_t)1 << k) - 1);
		acc_len -= k;

		
		for (i = 0; i < k; i ++) {
			br_i31_montymul(t1, x, x, m, m0i);
			memcpy(x, t1, mlen);
		}

		
		if (win_len > 1) {
			br_i31_zero(t2, m[0]);
			base = t2 + mwlen;
			for (u = 1; u < ((uint32_t)1 << k); u ++) {
				uint32_t mask;

				mask = -EQ(u, bits);
				for (v = 1; v < mwlen; v ++) {
					t2[v] |= mask & base[v];
				}
				base += mwlen;
			}
		}

		
		br_i31_montymul(t1, x, t2, m, m0i);
		CCOPY(NEQ(bits, 0), x, t1, mlen);
	}

	
	br_i31_from_monty(x, m, m0i);
	return 1;
}

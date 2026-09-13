

#include "inner.h"


void
br_i31_decode_reduce(uint32_t *x,
	const void *src, size_t len, const uint32_t *m)
{
	uint32_t m_ebitlen, m_rbitlen;
	size_t mblen, k;
	const unsigned char *buf;
	uint32_t acc;
	int acc_len;

	
	m_ebitlen = m[0];

	
	if (m_ebitlen == 0) {
		x[0] = 0;
		return;
	}

	
	br_i31_zero(x, m_ebitlen);

	
	m_rbitlen = m_ebitlen >> 5;
	m_rbitlen = (m_ebitlen & 31) + (m_rbitlen << 5) - m_rbitlen;
	mblen = (m_rbitlen + 7) >> 3;
	k = mblen - 1;
	if (k >= len) {
		br_i31_decode(x, src, len);
		x[0] = m_ebitlen;
		return;
	}
	buf = src;
	br_i31_decode(x, buf, k);
	x[0] = m_ebitlen;

	
	acc = 0;
	acc_len = 0;
	while (k < len) {
		uint32_t v;

		v = buf[k ++];
		if (acc_len >= 23) {
			acc_len -= 23;
			acc <<= (8 - acc_len);
			acc |= v >> acc_len;
			br_i31_muladd_small(x, acc, m);
			acc = v & (0xFF >> (8 - acc_len));
		} else {
			acc = (acc << 8) | v;
			acc_len += 8;
		}
	}

	
	if (acc_len != 0) {
		acc = (acc | (x[1] << acc_len)) & 0x7FFFFFFF;
		br_i31_rshift(x, 31 - acc_len);
		br_i31_muladd_small(x, acc, m);
	}
}

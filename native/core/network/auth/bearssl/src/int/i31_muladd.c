

#include "inner.h"


void
br_i31_muladd_small(uint32_t *x, uint32_t z, const uint32_t *m)
{
	uint32_t m_bitlen;
	unsigned mblr;
	size_t u, mlen;
	uint32_t a0, a1, b0, hi, g, q, tb;
	uint32_t under, over;
	uint32_t cc;

	
	m_bitlen = m[0];
	if (m_bitlen == 0) {
		return;
	}
	if (m_bitlen <= 31) {
		uint32_t lo;

		hi = x[1] >> 1;
		lo = (x[1] << 31) | z;
		x[1] = br_rem(hi, lo, m[1]);
		return;
	}
	mlen = (m_bitlen + 31) >> 5;
	mblr = (unsigned)m_bitlen & 31;

	
	hi = x[mlen];
	if (mblr == 0) {
		a0 = x[mlen];
		memmove(x + 2, x + 1, (mlen - 1) * sizeof *x);
		x[1] = z;
		a1 = x[mlen];
		b0 = m[mlen];
	} else {
		a0 = ((x[mlen] << (31 - mblr)) | (x[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
		memmove(x + 2, x + 1, (mlen - 1) * sizeof *x);
		x[1] = z;
		a1 = ((x[mlen] << (31 - mblr)) | (x[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
		b0 = ((m[mlen] << (31 - mblr)) | (m[mlen - 1] >> mblr))
			& 0x7FFFFFFF;
	}

	
	g = br_div(a0 >> 1, a1 | (a0 << 31), b0);
	q = MUX(EQ(a0, b0), 0x7FFFFFFF, MUX(EQ(g, 0), 0, g - 1));

	
	cc = 0;
	tb = 1;
	for (u = 1; u <= mlen; u ++) {
		uint32_t mw, zw, xw, nxw;
		uint64_t zl;

		mw = m[u];
		zl = MUL31(mw, q) + cc;
		cc = (uint32_t)(zl >> 31);
		zw = (uint32_t)zl & (uint32_t)0x7FFFFFFF;
		xw = x[u];
		nxw = xw - zw;
		cc += nxw >> 31;
		nxw &= 0x7FFFFFFF;
		x[u] = nxw;
		tb = MUX(EQ(nxw, mw), tb, GT(nxw, mw));
	}

	
	over = GT(cc, hi);
	under = ~over & (tb | LT(cc, hi));
	br_i31_add(x, m, over);
	br_i31_sub(x, m, under);
}



#include "inner.h"


uint32_t
br_divrem(uint32_t hi, uint32_t lo, uint32_t d, uint32_t *r)
{
	
	uint32_t q;
	uint32_t ch, cf;
	int k;

	q = 0;
	ch = EQ(hi, d);
	hi = MUX(ch, 0, hi);
	for (k = 31; k > 0; k --) {
		int j;
		uint32_t w, ctl, hi2, lo2;

		j = 32 - k;
		w = (hi << j) | (lo >> k);
		ctl = GE(w, d) | (hi >> k);
		hi2 = (w - d) >> j;
		lo2 = lo - (d << k);
		hi = MUX(ctl, hi2, hi);
		lo = MUX(ctl, lo2, lo);
		q |= ctl << k;
	}
	cf = GE(lo, d) | hi;
	q |= cf;
	*r = MUX(cf, lo - d, lo);
	return q;
}



#include "inner.h"


void
br_i31_to_monty(uint32_t *x, const uint32_t *m)
{
	uint32_t k;

	for (k = (m[0] + 31) >> 5; k > 0; k --) {
		br_i31_muladd_small(x, 0, m);
	}
}

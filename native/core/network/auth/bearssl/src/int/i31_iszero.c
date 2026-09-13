

#include "inner.h"


uint32_t
br_i31_iszero(const uint32_t *x)
{
	uint32_t z;
	size_t u;

	z = 0;
	for (u = (x[0] + 31) >> 5; u > 0; u --) {
		z |= x[u];
	}
	return ~(z | -z) >> 31;
}

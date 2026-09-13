

#include "inner.h"


void
br_range_enc64be(void *dst, const uint64_t *v, size_t num)
{
	unsigned char *buf;

	buf = dst;
	while (num -- > 0) {
		br_enc64be(buf, *v ++);
		buf += 8;
	}
}

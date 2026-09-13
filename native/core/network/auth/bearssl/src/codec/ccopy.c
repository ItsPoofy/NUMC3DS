

#include "inner.h"


void
br_ccopy(uint32_t ctl, void *dst, const void *src, size_t len)
{
	unsigned char *d;
	const unsigned char *s;

	d = dst;
	s = src;
	while (len -- > 0) {
		uint32_t x, y;

		x = *s ++;
		y = *d;
		*d = MUX(ctl, x, y);
		d ++;
	}
}

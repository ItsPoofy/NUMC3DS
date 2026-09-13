

#include "inner.h"


size_t
br_digest_size_by_ID(int digest_id)
{
	switch (digest_id) {
	case br_md5sha1_ID:
		return br_md5_SIZE + br_sha1_SIZE;
	case br_md5_ID:
		return br_md5_SIZE;
	case br_sha1_ID:
		return br_sha1_SIZE;
	case br_sha224_ID:
		return br_sha224_SIZE;
	case br_sha256_ID:
		return br_sha256_SIZE;
	case br_sha384_ID:
		return br_sha384_SIZE;
	case br_sha512_ID:
		return br_sha512_SIZE;
	default:
		
		return 0;
	}
}



#include "inner.h"



static const uint32_t P256_P[] = {
	0x00000108,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x00000007,
	0x00000000, 0x00000000, 0x00000040, 0x7FFFFF80,
	0x000000FF
};

static const uint32_t P256_R2[] = {
	0x00000108,
	0x00014000, 0x00018000, 0x00000000, 0x7FF40000,
	0x7FEFFFFF, 0x7FF7FFFF, 0x7FAFFFFF, 0x005FFFFF,
	0x00000000
};

static const uint32_t P256_B[] = {
	0x00000108,
	0x6FEE1803, 0x6229C4BD, 0x21B139BE, 0x327150AA,
	0x3567802E, 0x3F7212ED, 0x012E4355, 0x782DD38D,
	0x0000000E
};

static const uint32_t P384_P[] = {
	0x0000018C,
	0x7FFFFFFF, 0x00000001, 0x00000000, 0x7FFFFFF8,
	0x7FFFFFEF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x00000FFF
};

static const uint32_t P384_R2[] = {
	0x0000018C,
	0x00000000, 0x00000080, 0x7FFFFE00, 0x000001FF,
	0x00000800, 0x00000000, 0x7FFFE000, 0x00001FFF,
	0x00008000, 0x00008000, 0x00000000, 0x00000000,
	0x00000000
};

static const uint32_t P384_B[] = {
	0x0000018C,
	0x6E666840, 0x070D0392, 0x5D810231, 0x7651D50C,
	0x17E218D6, 0x1B192002, 0x44EFE441, 0x3A524E2B,
	0x2719BA5F, 0x41F02209, 0x36C5643E, 0x5813EFFE,
	0x000008A5
};

static const uint32_t P521_P[] = {
	0x00000219,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF,
	0x01FFFFFF
};

static const uint32_t P521_R2[] = {
	0x00000219,
	0x00001000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000, 0x00000000, 0x00000000, 0x00000000,
	0x00000000
};

static const uint32_t P521_B[] = {
	0x00000219,
	0x540FC00A, 0x228FEA35, 0x2C34F1EF, 0x67BF107A,
	0x46FC1CD5, 0x1605E9DD, 0x6937B165, 0x272A3D8F,
	0x42785586, 0x44C8C778, 0x15F3B8B4, 0x64B73366,
	0x03BA8B69, 0x0D05B42A, 0x21F929A2, 0x2C31C393,
	0x00654FAE
};

typedef struct {
	const uint32_t *p;
	const uint32_t *b;
	const uint32_t *R2;
	uint32_t p0i;
	size_t point_len;
} curve_params;

static inline const curve_params *
id_to_curve(int curve)
{
	static const curve_params pp[] = {
		{ P256_P, P256_B, P256_R2, 0x00000001,  65 },
		{ P384_P, P384_B, P384_R2, 0x00000001,  97 },
		{ P521_P, P521_B, P521_R2, 0x00000001, 133 }
	};

	return &pp[curve - BR_EC_secp256r1];
}

#define I31_LEN   ((BR_MAX_EC_SIZE + 61) / 31)


typedef struct {
	uint32_t c[3][I31_LEN];
} jacobian;


#define MSET(d, a)      (0x0000 + ((d) << 8) + ((a) << 4))
#define MADD(d, a)      (0x1000 + ((d) << 8) + ((a) << 4))
#define MSUB(d, a)      (0x2000 + ((d) << 8) + ((a) << 4))
#define MMUL(d, a, b)   (0x3000 + ((d) << 8) + ((a) << 4) + (b))
#define MINV(d, a, b)   (0x4000 + ((d) << 8) + ((a) << 4) + (b))
#define MTZ(d)          (0x5000 + ((d) << 8))
#define ENDCODE         0


#define P1x    0
#define P1y    1
#define P1z    2
#define P2x    3
#define P2y    4
#define P2z    5


#define Px     0
#define Py     1
#define Pz     2


#define t1     6
#define t2     7
#define t3     8
#define t4     9
#define t5    10
#define t6    11
#define t7    12


#define t8     3
#define t9     4
#define t10    5


static const uint16_t code_double[] = {
	
	MMUL(t1, Pz, Pz),

	
	MSET(t2, Px),
	MSUB(t2, t1),
	MADD(t1, Px),

	
	MMUL(t3, t1, t2),
	MSET(t1, t3),
	MADD(t1, t3),
	MADD(t1, t3),

	
	MMUL(t3, Py, Py),
	MADD(t3, t3),
	MMUL(t2, Px, t3),
	MADD(t2, t2),

	
	MMUL(Px, t1, t1),
	MSUB(Px, t2),
	MSUB(Px, t2),

	
	MMUL(t4, Py, Pz),
	MSET(Pz, t4),
	MADD(Pz, t4),

	
	MSUB(t2, Px),
	MMUL(Py, t1, t2),
	MMUL(t4, t3, t3),
	MSUB(Py, t4),
	MSUB(Py, t4),

	ENDCODE
};


static const uint16_t code_add[] = {
	
	MMUL(t3, P2z, P2z),
	MMUL(t1, P1x, t3),
	MMUL(t4, P2z, t3),
	MMUL(t3, P1y, t4),

	
	MMUL(t4, P1z, P1z),
	MMUL(t2, P2x, t4),
	MMUL(t5, P1z, t4),
	MMUL(t4, P2y, t5),

	
	MSUB(t2, t1),
	MSUB(t4, t3),

	
	MTZ(t4),

	
	MMUL(t7, t2, t2),
	MMUL(t6, t1, t7),
	MMUL(t5, t7, t2),

	
	MMUL(P1x, t4, t4),
	MSUB(P1x, t5),
	MSUB(P1x, t6),
	MSUB(P1x, t6),

	
	MSUB(t6, P1x),
	MMUL(P1y, t4, t6),
	MMUL(t1, t5, t3),
	MSUB(P1y, t1),

	
	MMUL(t1, P1z, P2z),
	MMUL(P1z, t1, t2),

	ENDCODE
};


static const uint16_t code_check[] = {

	
	MMUL(t1, P1x, P2x),
	MMUL(t2, P1y, P2x),
	MSET(P1x, t1),
	MSET(P1y, t2),

	
	MMUL(t2, P1x, P1x),
	MMUL(t1, P1x, t2),

	
	MSUB(t1, P1x),
	MSUB(t1, P1x),
	MSUB(t1, P1x),

	
	MADD(t1, P2y),

	
	MMUL(t2, P1y, P1y),

	
	MSUB(t1, t2),
	MTZ(t1),

	
	MMUL(P1z, P2x, P2z),

	ENDCODE
};


static const uint16_t code_affine[] = {

	
	MSET(t1, P1z),

	
	MMUL(t2, P1z, P1z),
	MMUL(t3, P1z, t2),
	MMUL(t2, t3, P2z),

	
	MINV(t2, t3, t4),

	
	MSET(t3, P1y),
	MMUL(P1y, t2, t3),

	
	MMUL(t3, t2, t1),

	
	MSET(t2, P1x),
	MMUL(P1x, t2, t3),

	ENDCODE
};

static uint32_t
run_code(jacobian *P1, const jacobian *P2,
	const curve_params *cc, const uint16_t *code)
{
	uint32_t r;
	uint32_t t[13][I31_LEN];
	size_t u;

	r = 1;

	
	memcpy(t[P1x], P1->c, 3 * I31_LEN * sizeof(uint32_t));
	memcpy(t[P2x], P2->c, 3 * I31_LEN * sizeof(uint32_t));

	
	for (u = 0;; u ++) {
		unsigned op, d, a, b;

		op = code[u];
		if (op == 0) {
			break;
		}
		d = (op >> 8) & 0x0F;
		a = (op >> 4) & 0x0F;
		b = op & 0x0F;
		op >>= 12;
		switch (op) {
			uint32_t ctl;
			size_t plen;
			unsigned char tp[(BR_MAX_EC_SIZE + 7) >> 3];

		case 0:
			memcpy(t[d], t[a], I31_LEN * sizeof(uint32_t));
			break;
		case 1:
			ctl = br_i31_add(t[d], t[a], 1);
			ctl |= NOT(br_i31_sub(t[d], cc->p, 0));
			br_i31_sub(t[d], cc->p, ctl);
			break;
		case 2:
			br_i31_add(t[d], cc->p, br_i31_sub(t[d], t[a], 1));
			break;
		case 3:
			br_i31_montymul(t[d], t[a], t[b], cc->p, cc->p0i);
			break;
		case 4:
			plen = (cc->p[0] - (cc->p[0] >> 5) + 7) >> 3;
			br_i31_encode(tp, plen, cc->p);
			tp[plen - 1] -= 2;
			br_i31_modpow(t[d], tp, plen,
				cc->p, cc->p0i, t[a], t[b]);
			break;
		default:
			r &= ~br_i31_iszero(t[d]);
			break;
		}
	}

	
	memcpy(P1->c, t[P1x], 3 * I31_LEN * sizeof(uint32_t));
	return r;
}

static void
set_one(uint32_t *x, const uint32_t *p)
{
	size_t plen;

	plen = (p[0] + 63) >> 5;
	memset(x, 0, plen * sizeof *x);
	x[0] = p[0];
	x[1] = 0x00000001;
}

static void
point_zero(jacobian *P, const curve_params *cc)
{
	memset(P, 0, sizeof *P);
	P->c[0][0] = P->c[1][0] = P->c[2][0] = cc->p[0];
}

static inline void
point_double(jacobian *P, const curve_params *cc)
{
	run_code(P, P, cc, code_double);
}

static inline uint32_t
point_add(jacobian *P1, const jacobian *P2, const curve_params *cc)
{
	return run_code(P1, P2, cc, code_add);
}

static void
point_mul(jacobian *P, const unsigned char *x, size_t xlen,
	const curve_params *cc)
{
	
	uint32_t qz;
	jacobian P2, P3, Q, T, U;

	memcpy(&P2, P, sizeof P2);
	point_double(&P2, cc);
	memcpy(&P3, P, sizeof P3);
	point_add(&P3, &P2, cc);

	point_zero(&Q, cc);
	qz = 1;
	while (xlen -- > 0) {
		int k;

		for (k = 6; k >= 0; k -= 2) {
			uint32_t bits;
			uint32_t bnz;

			point_double(&Q, cc);
			point_double(&Q, cc);
			memcpy(&T, P, sizeof T);
			memcpy(&U, &Q, sizeof U);
			bits = (*x >> k) & (uint32_t)3;
			bnz = NEQ(bits, 0);
			CCOPY(EQ(bits, 2), &T, &P2, sizeof T);
			CCOPY(EQ(bits, 3), &T, &P3, sizeof T);
			point_add(&U, &T, cc);
			CCOPY(bnz & qz, &Q, &T, sizeof Q);
			CCOPY(bnz & ~qz, &Q, &U, sizeof Q);
			qz &= ~bnz;
		}
		x ++;
	}
	memcpy(P, &Q, sizeof Q);
}


static uint32_t
point_decode(jacobian *P, const void *src, size_t len, const curve_params *cc)
{
	
	const unsigned char *buf;
	size_t plen, zlen;
	uint32_t r;
	jacobian Q;

	buf = src;
	point_zero(P, cc);
	plen = (cc->p[0] - (cc->p[0] >> 5) + 7) >> 3;
	if (len != 1 + (plen << 1)) {
		return 0;
	}
	r = br_i31_decode_mod(P->c[0], buf + 1, plen, cc->p);
	r &= br_i31_decode_mod(P->c[1], buf + 1 + plen, plen, cc->p);

	
	r &= EQ(buf[0], 0x04);
	

	
	zlen = ((cc->p[0] + 63) >> 5) * sizeof(uint32_t);
	memcpy(Q.c[0], cc->R2, zlen);
	memcpy(Q.c[1], cc->b, zlen);
	set_one(Q.c[2], cc->p);
	r &= ~run_code(P, &Q, cc, code_check);
	return r;
}


static void
point_encode(void *dst, const jacobian *P, const curve_params *cc)
{
	unsigned char *buf;
	uint32_t xbl;
	size_t plen;
	jacobian Q, T;

	buf = dst;
	xbl = cc->p[0];
	xbl -= (xbl >> 5);
	plen = (xbl + 7) >> 3;
	buf[0] = 0x04;
	memcpy(&Q, P, sizeof *P);
	set_one(T.c[2], cc->p);
	run_code(&Q, &T, cc, code_affine);
	br_i31_encode(buf + 1, plen, Q.c[0]);
	br_i31_encode(buf + 1 + plen, plen, Q.c[1]);
}

static const br_ec_curve_def *
id_to_curve_def(int curve)
{
	switch (curve) {
	case BR_EC_secp256r1:
		return &br_secp256r1;
	case BR_EC_secp384r1:
		return &br_secp384r1;
	case BR_EC_secp521r1:
		return &br_secp521r1;
	}
	return NULL;
}

static const unsigned char *
api_generator(int curve, size_t *len)
{
	const br_ec_curve_def *cd;

	cd = id_to_curve_def(curve);
	*len = cd->generator_len;
	return cd->generator;
}

static const unsigned char *
api_order(int curve, size_t *len)
{
	const br_ec_curve_def *cd;

	cd = id_to_curve_def(curve);
	*len = cd->order_len;
	return cd->order;
}

static size_t
api_xoff(int curve, size_t *len)
{
	api_generator(curve, len);
	*len >>= 1;
	return 1;
}

static uint32_t
api_mul(unsigned char *G, size_t Glen,
	const unsigned char *x, size_t xlen, int curve)
{
	uint32_t r;
	const curve_params *cc;
	jacobian P;

	cc = id_to_curve(curve);
	if (Glen != cc->point_len) {
		return 0;
	}
	r = point_decode(&P, G, Glen, cc);
	point_mul(&P, x, xlen, cc);
	point_encode(G, &P, cc);
	return r;
}

static size_t
api_mulgen(unsigned char *R,
	const unsigned char *x, size_t xlen, int curve)
{
	const unsigned char *G;
	size_t Glen;

	G = api_generator(curve, &Glen);
	memcpy(R, G, Glen);
	api_mul(R, Glen, x, xlen, curve);
	return Glen;
}

static uint32_t
api_muladd(unsigned char *A, const unsigned char *B, size_t len,
	const unsigned char *x, size_t xlen,
	const unsigned char *y, size_t ylen, int curve)
{
	uint32_t r, t, z;
	const curve_params *cc;
	jacobian P, Q;

	

	cc = id_to_curve(curve);
	if (len != cc->point_len) {
		return 0;
	}
	r = point_decode(&P, A, len, cc);
	if (B == NULL) {
		size_t Glen;

		B = api_generator(curve, &Glen);
	}
	r &= point_decode(&Q, B, len, cc);
	point_mul(&P, x, xlen, cc);
	point_mul(&Q, y, ylen, cc);

	
	t = point_add(&P, &Q, cc);
	point_double(&Q, cc);
	z = br_i31_iszero(P.c[2]);

	
	CCOPY(z & ~t, &P, &Q, sizeof Q);
	point_encode(A, &P, cc);
	r &= ~(z & t);

	return r;
}


const br_ec_impl br_ec_prime_i31 = {
	(uint32_t)0x03800000,
	&api_generator,
	&api_order,
	&api_xoff,
	&api_mul,
	&api_mulgen,
	&api_muladd
};

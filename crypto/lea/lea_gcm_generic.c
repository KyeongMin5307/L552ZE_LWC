#include "lea.h"
#include "lea_locl.h"

static void _gcm_ghash(unsigned char *r, const unsigned char *x, unsigned int x_len, const unsigned char h[][16]);
static void _lea_gcm_init(LEA_GCM_CTX *ctx, const unsigned char *mk, int mk_len);

#if (GCM_OPT_LEVEL == 0)

static void gcm_gfmul(unsigned char *r, const unsigned char *x, const unsigned char *y)
{
	unsigned char z[16], v[16];
	int i;

	lea_memset(z, 0, 16);
	lea_memcpy(v, y, 16);

	for (i = 0; i < 128; i++)
	{
		if ((x[i >> 3] >> (7 - (i & 0x7))) & 1)
			XOR8x16(z, z, v);

		if (v[15] & 1)
		{
			RSHIFT8x16_1(v);

			v[0] ^= 0xe1;
		}
		else
			RSHIFT8x16_1(v);
	}

	lea_memcpy(r, z, 16);
}

static void _gcm_ghash(unsigned char *r, const unsigned char *x, int x_len, const unsigned char h[][16])
{
	int i;
	unsigned char y[16] = { 0, };

	lea_memcpy(y, r, 16);

	for (; x_len >= 16; x_len -= 16, x += 16)
	{
		XOR8x16(y, y, x);

		gcm_gfmul(y, y, h[0]);
	}

	if (x_len)
	{
		for (i = 0; i < x_len; i++)
			y[i] = y[i] ^ x[i];

		gcm_gfmul(y, y, h[0]);
	}

	lea_memcpy(r, y, 16);
}

static void _lea_gcm_init(LEA_GCM_CTX *ctx, const unsigned char *mk, int mk_len)
{
	unsigned char zero[16] = { 0, };

	lea_memset(ctx, 0, sizeof(LEA_GCM_CTX));

	lea_set_key(&ctx->key, mk, mk_len);
	lea_encrypt((unsigned char *)ctx->h, zero, &ctx->key);
}

#elif (GCM_OPT_LEVEL == 1)

const unsigned char reduction_4bit[16][2] = {
	{ 0x00, 0x00 }, { 0x1c, 0x20 }, { 0x38, 0x40 }, { 0x24, 0x60 }, { 0x70, 0x80 }, { 0x6c, 0xa0 }, { 0x48, 0xc0 }, { 0x54, 0xe0 },
	{ 0xe1, 0x00 }, { 0xfd, 0x20 }, { 0xd9, 0x40 }, { 0xc5, 0x60 }, { 0x91, 0x80 }, { 0x8d, 0xa0 }, { 0xa9, 0xc0 }, { 0xb5, 0xe0 }
};

static void gcm_init_4bit_table(unsigned char hTable[][16], const unsigned char *h)
{
	unsigned char tmp[16];

	lea_memcpy(tmp, h, 16);

	lea_memcpy(hTable[8], tmp, 16);

	RSHIFT8x16_1(tmp);
	if (hTable[8][15] & 1)
		tmp[0] ^= 0xe1;
	lea_memcpy(hTable[4], tmp, 16);

	RSHIFT8x16_1(tmp);
	if (hTable[4][15] & 1)
		tmp[0] ^= 0xe1;
	lea_memcpy(hTable[2], tmp, 16);


	RSHIFT8x16_1(tmp);
	if (hTable[2][15] & 1)
		tmp[0] ^= 0xe1;
	lea_memcpy(hTable[1], tmp, 16);


	XOR8x16(hTable[3], hTable[2], hTable[1]);
	XOR8x16(hTable[5], hTable[4], hTable[1]);
	XOR8x16(hTable[6], hTable[4], hTable[2]);
	XOR8x16(hTable[7], hTable[4], hTable[3]);
	XOR8x16(hTable[9], hTable[8], hTable[1]);
	XOR8x16(hTable[10], hTable[8], hTable[2]);
	XOR8x16(hTable[11], hTable[8], hTable[3]);
	XOR8x16(hTable[12], hTable[8], hTable[4]);
	XOR8x16(hTable[13], hTable[8], hTable[5]);
	XOR8x16(hTable[14], hTable[8], hTable[6]);
	XOR8x16(hTable[15], hTable[8], hTable[7]);
}

static void gcm_gfmul(unsigned char *r, const unsigned char *x, const unsigned char hTable[][16])
{
	unsigned char z[16], mask;
	int i;

	lea_memset(z, 0, 16);

	for (i = 15; i > 0; i--)
	{
		mask = x[i] & 0xf;
		XOR8x16(z, z, hTable[mask]);

		mask = z[15] & 0xf;
		RSHIFT8x16_4(z);
		z[0] ^= reduction_4bit[mask][0];
		z[1] ^= reduction_4bit[mask][1];

		mask = x[i] >> 4;
		XOR8x16(z, z, hTable[mask]);

		mask = z[15] & 0xf;
		RSHIFT8x16_4(z);
		z[0] ^= reduction_4bit[mask][0];
		z[1] ^= reduction_4bit[mask][1];
	}

	mask = x[i] & 0xf;
	XOR8x16(z, z, hTable[mask]);

	mask = z[15] & 0xf;
	RSHIFT8x16_4(z);
	z[0] ^= reduction_4bit[mask][0];
	z[1] ^= reduction_4bit[mask][1];

	mask = x[i] >> 4;
	XOR8x16(z, z, hTable[mask]);

	lea_memcpy(r, z, 16);
}

static void _gcm_ghash(unsigned char *r, const unsigned char *x, unsigned int x_len, const unsigned char hTable[][16])
{
	int i;
	unsigned char y[16] = { 0, };

	lea_memcpy(y, r, 16);

	for (; x_len >= 16; x_len -= 16, x += 16)
	{
		XOR8x16(y, y, x);

		gcm_gfmul(y, y, hTable);
	}

	if (x_len)
	{
		for (i = 0; i < x_len; i++)
			y[i] = y[i] ^ x[i];

		gcm_gfmul(y, y, hTable);
	}

	lea_memcpy(r, y, 16);
}

static void _lea_gcm_init(LEA_GCM_CTX *ctx, const unsigned char *mk, int mk_len)
{
	unsigned char zero[16] = { 0, }, h[16];

	lea_memset(ctx, 0, sizeof(LEA_GCM_CTX));

	lea_set_key(&ctx->key, mk, mk_len);
	lea_encrypt(zero, zero, &ctx->key);

	gcm_init_4bit_table(ctx->h, zero);
}

#elif (GCM_OPT_LEVEL == 2)

const unsigned char reduction_8bit[256][2] = {
	{ 0x00, 0x00 }, { 0x01, 0xc2 }, { 0x03, 0x84 }, { 0x02, 0x46 }, { 0x07, 0x08 }, { 0x06, 0xca }, { 0x04, 0x8c }, { 0x05, 0x4e },
	{ 0x0e, 0x10 }, { 0x0f, 0xd2 }, { 0x0d, 0x94 }, { 0x0c, 0x56 }, { 0x09, 0x18 }, { 0x08, 0xda }, { 0x0a, 0x9c }, { 0x0b, 0x5e },
	{ 0x1c, 0x20 }, { 0x1d, 0xe2 }, { 0x1f, 0xa4 }, { 0x1e, 0x66 }, { 0x1b, 0x28 }, { 0x1a, 0xea }, { 0x18, 0xac }, { 0x19, 0x6e },
	{ 0x12, 0x30 }, { 0x13, 0xf2 }, { 0x11, 0xb4 }, { 0x10, 0x76 }, { 0x15, 0x38 }, { 0x14, 0xfa }, { 0x16, 0xbc }, { 0x17, 0x7e },
	{ 0x38, 0x40 }, { 0x39, 0x82 }, { 0x3b, 0xc4 }, { 0x3a, 0x06 }, { 0x3f, 0x48 }, { 0x3e, 0x8a }, { 0x3c, 0xcc }, { 0x3d, 0x0e },
	{ 0x36, 0x50 }, { 0x37, 0x92 }, { 0x35, 0xd4 }, { 0x34, 0x16 }, { 0x31, 0x58 }, { 0x30, 0x9a }, { 0x32, 0xdc }, { 0x33, 0x1e },
	{ 0x24, 0x60 }, { 0x25, 0xa2 }, { 0x27, 0xe4 }, { 0x26, 0x26 }, { 0x23, 0x68 }, { 0x22, 0xaa }, { 0x20, 0xec }, { 0x21, 0x2e },
	{ 0x2a, 0x70 }, { 0x2b, 0xb2 }, { 0x29, 0xf4 }, { 0x28, 0x36 }, { 0x2d, 0x78 }, { 0x2c, 0xba }, { 0x2e, 0xfc }, { 0x2f, 0x3e },
	{ 0x70, 0x80 }, { 0x71, 0x42 }, { 0x73, 0x04 }, { 0x72, 0xc6 }, { 0x77, 0x88 }, { 0x76, 0x4a }, { 0x74, 0x0c }, { 0x75, 0xce },
	{ 0x7e, 0x90 }, { 0x7f, 0x52 }, { 0x7d, 0x14 }, { 0x7c, 0xd6 }, { 0x79, 0x98 }, { 0x78, 0x5a }, { 0x7a, 0x1c }, { 0x7b, 0xde },
	{ 0x6c, 0xa0 }, { 0x6d, 0x62 }, { 0x6f, 0x24 }, { 0x6e, 0xe6 }, { 0x6b, 0xa8 }, { 0x6a, 0x6a }, { 0x68, 0x2c }, { 0x69, 0xee },
	{ 0x62, 0xb0 }, { 0x63, 0x72 }, { 0x61, 0x34 }, { 0x60, 0xf6 }, { 0x65, 0xb8 }, { 0x64, 0x7a }, { 0x66, 0x3c }, { 0x67, 0xfe },
	{ 0x48, 0xc0 }, { 0x49, 0x02 }, { 0x4b, 0x44 }, { 0x4a, 0x86 }, { 0x4f, 0xc8 }, { 0x4e, 0x0a }, { 0x4c, 0x4c }, { 0x4d, 0x8e },
	{ 0x46, 0xd0 }, { 0x47, 0x12 }, { 0x45, 0x54 }, { 0x44, 0x96 }, { 0x41, 0xd8 }, { 0x40, 0x1a }, { 0x42, 0x5c }, { 0x43, 0x9e },
	{ 0x54, 0xe0 }, { 0x55, 0x22 }, { 0x57, 0x64 }, { 0x56, 0xa6 }, { 0x53, 0xe8 }, { 0x52, 0x2a }, { 0x50, 0x6c }, { 0x51, 0xae },
	{ 0x5a, 0xf0 }, { 0x5b, 0x32 }, { 0x59, 0x74 }, { 0x58, 0xb6 }, { 0x5d, 0xf8 }, { 0x5c, 0x3a }, { 0x5e, 0x7c }, { 0x5f, 0xbe },
	{ 0xe1, 0x00 }, { 0xe0, 0xc2 }, { 0xe2, 0x84 }, { 0xe3, 0x46 }, { 0xe6, 0x08 }, { 0xe7, 0xca }, { 0xe5, 0x8c }, { 0xe4, 0x4e },
	{ 0xef, 0x10 }, { 0xee, 0xd2 }, { 0xec, 0x94 }, { 0xed, 0x56 }, { 0xe8, 0x18 }, { 0xe9, 0xda }, { 0xeb, 0x9c }, { 0xea, 0x5e },
	{ 0xfd, 0x20 }, { 0xfc, 0xe2 }, { 0xfe, 0xa4 }, { 0xff, 0x66 }, { 0xfa, 0x28 }, { 0xfb, 0xea }, { 0xf9, 0xac }, { 0xf8, 0x6e },
	{ 0xf3, 0x30 }, { 0xf2, 0xf2 }, { 0xf0, 0xb4 }, { 0xf1, 0x76 }, { 0xf4, 0x38 }, { 0xf5, 0xfa }, { 0xf7, 0xbc }, { 0xf6, 0x7e },
	{ 0xd9, 0x40 }, { 0xd8, 0x82 }, { 0xda, 0xc4 }, { 0xdb, 0x06 }, { 0xde, 0x48 }, { 0xdf, 0x8a }, { 0xdd, 0xcc }, { 0xdc, 0x0e },
	{ 0xd7, 0x50 }, { 0xd6, 0x92 }, { 0xd4, 0xd4 }, { 0xd5, 0x16 }, { 0xd0, 0x58 }, { 0xd1, 0x9a }, { 0xd3, 0xdc }, { 0xd2, 0x1e },
	{ 0xc5, 0x60 }, { 0xc4, 0xa2 }, { 0xc6, 0xe4 }, { 0xc7, 0x26 }, { 0xc2, 0x68 }, { 0xc3, 0xaa }, { 0xc1, 0xec }, { 0xc0, 0x2e },
	{ 0xcb, 0x70 }, { 0xca, 0xb2 }, { 0xc8, 0xf4 }, { 0xc9, 0x36 }, { 0xcc, 0x78 }, { 0xcd, 0xba }, { 0xcf, 0xfc }, { 0xce, 0x3e },
	{ 0x91, 0x80 }, { 0x90, 0x42 }, { 0x92, 0x04 }, { 0x93, 0xc6 }, { 0x96, 0x88 }, { 0x97, 0x4a }, { 0x95, 0x0c }, { 0x94, 0xce },
	{ 0x9f, 0x90 }, { 0x9e, 0x52 }, { 0x9c, 0x14 }, { 0x9d, 0xd6 }, { 0x98, 0x98 }, { 0x99, 0x5a }, { 0x9b, 0x1c }, { 0x9a, 0xde },
	{ 0x8d, 0xa0 }, { 0x8c, 0x62 }, { 0x8e, 0x24 }, { 0x8f, 0xe6 }, { 0x8a, 0xa8 }, { 0x8b, 0x6a }, { 0x89, 0x2c }, { 0x88, 0xee },
	{ 0x83, 0xb0 }, { 0x82, 0x72 }, { 0x80, 0x34 }, { 0x81, 0xf6 }, { 0x84, 0xb8 }, { 0x85, 0x7a }, { 0x87, 0x3c }, { 0x86, 0xfe },
	{ 0xa9, 0xc0 }, { 0xa8, 0x02 }, { 0xaa, 0x44 }, { 0xab, 0x86 }, { 0xae, 0xc8 }, { 0xaf, 0x0a }, { 0xad, 0x4c }, { 0xac, 0x8e },
	{ 0xa7, 0xd0 }, { 0xa6, 0x12 }, { 0xa4, 0x54 }, { 0xa5, 0x96 }, { 0xa0, 0xd8 }, { 0xa1, 0x1a }, { 0xa3, 0x5c }, { 0xa2, 0x9e },
	{ 0xb5, 0xe0 }, { 0xb4, 0x22 }, { 0xb6, 0x64 }, { 0xb7, 0xa6 }, { 0xb2, 0xe8 }, { 0xb3, 0x2a }, { 0xb1, 0x6c }, { 0xb0, 0xae },
	{ 0xbb, 0xf0 }, { 0xba, 0x32 }, { 0xb8, 0x74 }, { 0xb9, 0xb6 }, { 0xbc, 0xf8 }, { 0xbd, 0x3a }, { 0xbf, 0x7c }, { 0xbe, 0xbe },
};

static void gcm_init_8bit_table(unsigned char hTable[][16], const unsigned char *h)
{
	unsigned char tmp[16];
	unsigned int i, j;

	lea_memcpy(tmp, h, 16);
	lea_memcpy(hTable[0x80], tmp, 16);

	for (i = 0x40; i >= 1; i >>= 1)
	{
		RSHIFT8x16_1(tmp);
		if (hTable[i << 1][15] & 1)
			tmp[0] ^= 0xe1;
		lea_memcpy(hTable[i], tmp, 16);
	}

	for (i = 2; i < 256; i <<= 1)
	{
		for (j = 1; j < i; j++)
			XOR8x16(hTable[i + j], hTable[i], hTable[j]);
	}
}

static void gcm_gfmul(unsigned char *r, const unsigned char *x, const unsigned char hTable[][16])
{
	unsigned char z[16], mask;
	int i;

	lea_memset(z, 0, 16);

	for (i = 15; i > 0; i--)
	{
		XOR8x16(z, z, hTable[x[i]]);

		mask = z[15];
		RSHIFT8x16_8(z);
		z[0] ^= reduction_8bit[mask][0];
		z[1] ^= reduction_8bit[mask][1];
	}

	XOR8x16(z, z, hTable[x[i]]);

	lea_memcpy(r, z, 16);
}

static void _gcm_ghash(unsigned char *r, const unsigned char *x, unsigned int x_len, const unsigned char hTable[][16])
{
	int i;
	unsigned char y[16] = { 0, };

	lea_memcpy(y, r, 16);

	for (; x_len >= 16; x_len -= 16, x += 16)
	{
		XOR8x16(y, y, x);

		gcm_gfmul(y, y, hTable);
	}

	if (x_len)
	{
		for (i = 0; i < x_len; i++)
			y[i] = y[i] ^ x[i];

		gcm_gfmul(y, y, hTable);
	}

	lea_memcpy(r, y, 16);
}

static void _lea_gcm_init(LEA_GCM_CTX *ctx, const unsigned char *mk, int mk_len)
{
	unsigned char zero[16] = { 0, };

	lea_memset(ctx, 0, sizeof(LEA_GCM_CTX));

	lea_set_key(&ctx->key, mk, mk_len);
	lea_encrypt(zero, zero, &ctx->key);

	gcm_init_8bit_table(ctx->h, zero);
}

#endif

void lea_gcm_init_generic(LEA_GCM_CTX *ctx, const unsigned char *mk, int mk_len){
	_lea_gcm_init(ctx, mk, mk_len);
}

void gcm_ghash_generic(unsigned char *r, const unsigned char *x, unsigned int x_len, const unsigned char hTable[][16]){
	_gcm_ghash(r, x, x_len, hTable);
}

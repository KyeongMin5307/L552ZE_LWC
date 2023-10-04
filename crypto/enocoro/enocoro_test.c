#include <string.h>

#include "enocoro_test.h"
#include "util.h"

// https://www.hitachi.com/rd/yrl/crypto/enocoro/index.html
// See 'Code for the generation of test vectors [ZIP format, 56 kBytes]'
// for tvec/test_main.c

#define TEST_VECTOR_BYTE_SIZE 1024

/* print test-vector */
void print_test_vector_result(const unsigned char *key, int key_len,
		const unsigned char *iv, int iv_len,
		const unsigned char *keystream, int keystream_len)
{
	printf("\n");
	printf("Key        ");
	print_hex(key, key_len);
	printf("IV         ");
	print_hex(iv, iv_len);
	printf("Test Vector ");
	print_hex(keystream, keystream_len);
	printf("\n\n");
}

void enocoro_test() {
	int i;
	/* enocoro-128 */
	uint8_t key[10][ENOCORO128_KEYSIZE / 8] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
		{0x0f, 0x0e, 0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00},
		{0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00,
		 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00},
		{0xf0, 0xe0, 0xd0, 0xc0, 0xb0, 0xa0, 0x90, 0x80,
		 0x70, 0x60, 0x50, 0x40, 0x30, 0x20, 0x10, 0x00},
		{0x12, 0x23, 0x34, 0x45, 0x56, 0x67, 0x78, 0x89,
		 0x9a, 0xab, 0xbc, 0xcd, 0xde, 0xef, 0xf0, 0xf1},
		{0x80, 0x91, 0xa2, 0xb3, 0xc4, 0xd5, 0xe6, 0xf7,
		 0x08, 0x19, 0x2a, 0x3b, 0x4c, 0x5d, 0x6e, 0x7f},
		{0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d, 0x1e, 0x0f,
		 0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87},
		{0x02, 0x04, 0x06, 0x08, 0x0a, 0x0c, 0x0e, 0x00,
		 0x01, 0x03, 0x05, 0x07, 0x09, 0x0b, 0x0d, 0x0f},
		{0x5d, 0x6e, 0x7f, 0x80, 0x91, 0xa2, 0xb3, 0xc4,
		 0xd5, 0xe6, 0xf7, 0x08, 0x19, 0x2a, 0x3b, 0x4c}
	};

	uint8_t iv[10][ENOCORO_IVSIZE / 8] = {
		{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
		{0x00, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70},
		{0x80, 0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0},
		{0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00},
		{0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
		{0xff, 0xee, 0xdd, 0xcc, 0xbb, 0xaa, 0x99, 0x88},
		{0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00},
		{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff},
		{0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10},
		{0x92, 0x83, 0x74, 0x85, 0x96, 0xa7, 0xb8, 0xc9},
	};

	/* clear key-length and IV-size */
	uint32_t keysize = ENOCORO128_KEY_BYTE_SIZE;
	uint32_t ivsize = ENOCORO_IV_BYTE_SIZE;


	/* clear ket-stream area */
	uint8_t keystream[TEST_VECTOR_BYTE_SIZE] = {0};

	/* define context-struct */
	ENOCORO_Ctx ctx; 

	/* --------------------------------------------------------------------- */
	/* --- Test Vectors ---------------------------------------------------- */
	/* --------------------------------------------------------------------- */
	printf("==== ENOCORO TEST VECTORS =============================\n");
	for (i = 0; i < 10; i++) {
		/* clear context */
		memset(&ctx, 0, sizeof (ctx));

		printf("---- TEST%d ---------------\n", i + 1);

		/* === TEST === */
		/* setup init */
		ENOCORO_init(&ctx, key[i], keysize, iv[i], ivsize);

		/* output randum value */
		ENOCORO_keystream(&ctx, keystream, TEST_VECTOR_BYTE_SIZE);

		/* print result */
		print_test_vector_result(key[i], keysize, iv[i], ivsize, keystream,
			TEST_VECTOR_BYTE_SIZE);
	}
}
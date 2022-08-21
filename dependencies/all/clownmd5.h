/*
Copyright (c) 2022 Clownacy

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef CLOWNMD5_GUARD_MISC
#define CLOWNMD5_GUARD_MISC

/* Define 'CLOWNMD5_STATIC' to limit the visibility of public functions. */
/* Alternatively, define 'CLOWNMD5_API' to control the qualifiers applied to the public functions. */
#ifndef CLOWNMD5_API
 #ifdef CLOWNMD5_STATIC
  #define CLOWNMD5_API static
 #else
  #define CLOWNMD5_API
 #endif
#endif

#include <stddef.h>

typedef struct ClownMD5_State
{
	unsigned long A;
	unsigned long B;
	unsigned long C;
	unsigned long D;
	size_t total_bits;
} ClownMD5_State;

#endif /* CLOWNMD5_GUARD_MISC */

#if !defined(CLOWNMD5_STATIC) || defined(CLOWNMD5_IMPLEMENTATION)

#ifndef CLOWNMD5_GUARD_FUNCTION_DECLARATIONS
#define CLOWNMD5_GUARD_FUNCTION_DECLARATIONS

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialises the MD5 hasher's state.
 * 
 * Parameters:
 *   state - A pointer to the ClownMD5_State to inititalise.
 */
CLOWNMD5_API void ClownMD5_Init(ClownMD5_State *state);

/*
 * Processes 512 bits of the to-be-hashed data.
 * 
 * Parameters:
 *   state - A pointer to the ClownMD5_State representing the current hash.
 *   data  - A pointer to an array of 64 chars, containing the 512 bits that are to be processed. Each char should contain 8 bits.
 */
CLOWNMD5_API void ClownMD5_PushData(ClownMD5_State *state, const unsigned char data[16 * 4]);

/*
 * Processes the final bits of the to-be-hashed data and outputs an MD5 hash.
 * 
 * Parameters:
 *   state - A pointer to the ClownMD5_State representing the current hash.
 *   data  - A pointer to an array of chars, containing the bits that are to be processed. Each char should contain 8 bits, except for the final char which should contain 8 or fewer.
 *   bits  - The total number of bits contained in the above array.
 *   hash  - A pointer to an array of 16 chars, which the MD5 hash will be written to.
 */
CLOWNMD5_API void ClownMD5_PushFinalData(ClownMD5_State *state, unsigned char data[16 * 4], unsigned int bits, unsigned char hash[16]);

#ifdef __cplusplus
}
#endif

#endif /* CLOWNMD5_GUARD_FUNCTION_DECLARATIONS */

#endif /* !defined(CLOWNMD5_STATIC) || defined(CLOWNMD5_IMPLEMENTATION) */

#ifdef CLOWNMD5_IMPLEMENTATION

#ifndef CLOWNMD5_GUARD_FUNCTION_DEFINITIONS
#define CLOWNMD5_GUARD_FUNCTION_DEFINITIONS

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* The variables in this file are named to match those described by RFC 1321 */

#define CLOWNMD5_MIN(a, b) ((a) < (b) ? (a) : (b))

#define CLOWNMD5_F_FUNCTION(X, Y, Z) (((X) & (Y)) | (~(X) & (Z)))
#define CLOWNMD5_G_FUNCTION(X, Y, Z) (((X) & (Z)) | ((Y) & ~(Z)))
#define CLOWNMD5_H_FUNCTION(X, Y, Z) ((X) ^ (Y) ^ (Z))
#define CLOWNMD5_I_FUNCTION(X, Y, Z) ((Y) ^ ((X) | ~(Z)))

#define CLOWNMD5_ROTATE_LEFT_32BIT(value, bits_to_rotate_by) (((value) << (bits_to_rotate_by)) | (((value) & 0xFFFFFFFF) >> (32 - (bits_to_rotate_by))))

#define CLOWNMD5_ROUND1(a, b, c, d, k, s, ac) (a) += CLOWNMD5_F_FUNCTION((b), (c), (d)) + X[(k)] + (ac); (a) = (b) + CLOWNMD5_ROTATE_LEFT_32BIT((a), (s))
#define CLOWNMD5_ROUND2(a, b, c, d, k, s, ac) (a) += CLOWNMD5_G_FUNCTION((b), (c), (d)) + X[(k)] + (ac); (a) = (b) + CLOWNMD5_ROTATE_LEFT_32BIT((a), (s))
#define CLOWNMD5_ROUND3(a, b, c, d, k, s, ac) (a) += CLOWNMD5_H_FUNCTION((b), (c), (d)) + X[(k)] + (ac); (a) = (b) + CLOWNMD5_ROTATE_LEFT_32BIT((a), (s))
#define CLOWNMD5_ROUND4(a, b, c, d, k, s, ac) (a) += CLOWNMD5_I_FUNCTION((b), (c), (d)) + X[(k)] + (ac); (a) = (b) + CLOWNMD5_ROTATE_LEFT_32BIT((a), (s))

static void ClownMD5_ProcessBlock(ClownMD5_State *state, const unsigned char block[16 * 4])
{
	/* "Step 4. Process Message in 16-Word Blocks" */
	unsigned int i;

	unsigned long A, B, C, D;

	unsigned long X[16];

	/* Copy block into X, converting it from an array of bytes to an array of 32-bit words. */
	for (i = 0; i < 16; ++i)
	{
		unsigned int j;

		X[i] = 0;

		for (j = 0; j < 4; ++j)
			X[i] |= (unsigned long)block[i * 4 + j] << (8 * j);
	}

	A = state->A;
	B = state->B;
	C = state->C;
	D = state->D;

	/* Round 1 */
	CLOWNMD5_ROUND1(A, B, C, D,  0,  7, 0xD76AA478);
	CLOWNMD5_ROUND1(D, A, B, C,  1, 12, 0xE8C7B756);
	CLOWNMD5_ROUND1(C, D, A, B,  2, 17, 0x242070DB);
	CLOWNMD5_ROUND1(B, C, D, A,  3, 22, 0xC1BDCEEE);

	CLOWNMD5_ROUND1(A, B, C, D,  4,  7, 0xF57C0FAF);
	CLOWNMD5_ROUND1(D, A, B, C,  5, 12, 0x4787C62A);
	CLOWNMD5_ROUND1(C, D, A, B,  6, 17, 0xA8304613);
	CLOWNMD5_ROUND1(B, C, D, A,  7, 22, 0xFD469501);

	CLOWNMD5_ROUND1(A, B, C, D,  8,  7, 0x698098D8);
	CLOWNMD5_ROUND1(D, A, B, C,  9, 12, 0x8B44F7AF);
	CLOWNMD5_ROUND1(C, D, A, B, 10, 17, 0xFFFF5BB1);
	CLOWNMD5_ROUND1(B, C, D, A, 11, 22, 0x895CD7BE);

	CLOWNMD5_ROUND1(A, B, C, D, 12,  7, 0x6B901122);
	CLOWNMD5_ROUND1(D, A, B, C, 13, 12, 0xFD987193);
	CLOWNMD5_ROUND1(C, D, A, B, 14, 17, 0xA679438E);
	CLOWNMD5_ROUND1(B, C, D, A, 15, 22, 0x49B40821);

	/* Round 2 */
	CLOWNMD5_ROUND2(A, B, C, D,  1,  5, 0xF61E2562);
	CLOWNMD5_ROUND2(D, A, B, C,  6,  9, 0xC040B340);
	CLOWNMD5_ROUND2(C, D, A, B, 11, 14, 0x265E5A51);
	CLOWNMD5_ROUND2(B, C, D, A,  0, 20, 0xE9B6C7AA);

	CLOWNMD5_ROUND2(A, B, C, D,  5,  5, 0xD62F105D);
	CLOWNMD5_ROUND2(D, A, B, C, 10,  9, 0x02441453);
	CLOWNMD5_ROUND2(C, D, A, B, 15, 14, 0xD8A1E681);
	CLOWNMD5_ROUND2(B, C, D, A,  4, 20, 0xE7D3FBC8);

	CLOWNMD5_ROUND2(A, B, C, D,  9,  5, 0x21E1CDE6);
	CLOWNMD5_ROUND2(D, A, B, C, 14,  9, 0xC33707D6);
	CLOWNMD5_ROUND2(C, D, A, B,  3, 14, 0xF4D50D87);
	CLOWNMD5_ROUND2(B, C, D, A,  8, 20, 0x455A14ED);

	CLOWNMD5_ROUND2(A, B, C, D, 13,  5, 0xA9E3E905);
	CLOWNMD5_ROUND2(D, A, B, C,  2,  9, 0xFCEFA3F8);
	CLOWNMD5_ROUND2(C, D, A, B,  7, 14, 0x676F02D9);
	CLOWNMD5_ROUND2(B, C, D, A, 12, 20, 0x8D2A4C8A);

	/* Round 3 */
	CLOWNMD5_ROUND3(A, B, C, D,  5,  4, 0xFFFA3942);
	CLOWNMD5_ROUND3(D, A, B, C,  8, 11, 0x8771F681);
	CLOWNMD5_ROUND3(C, D, A, B, 11, 16, 0x6D9D6122);
	CLOWNMD5_ROUND3(B, C, D, A, 14, 23, 0xFDE5380C);

	CLOWNMD5_ROUND3(A, B, C, D,  1,  4, 0xA4BEEA44);
	CLOWNMD5_ROUND3(D, A, B, C,  4, 11, 0x4BDECFA9);
	CLOWNMD5_ROUND3(C, D, A, B,  7, 16, 0xF6BB4B60);
	CLOWNMD5_ROUND3(B, C, D, A, 10, 23, 0xBEBFBC70);

	CLOWNMD5_ROUND3(A, B, C, D, 13,  4, 0x289B7EC6);
	CLOWNMD5_ROUND3(D, A, B, C,  0, 11, 0xEAA127FA);
	CLOWNMD5_ROUND3(C, D, A, B,  3, 16, 0xD4EF3085);
	CLOWNMD5_ROUND3(B, C, D, A,  6, 23, 0x04881D05);

	CLOWNMD5_ROUND3(A, B, C, D,  9,  4, 0xD9D4D039);
	CLOWNMD5_ROUND3(D, A, B, C, 12, 11, 0xE6DB99E5);
	CLOWNMD5_ROUND3(C, D, A, B, 15, 16, 0x1FA27CF8);
	CLOWNMD5_ROUND3(B, C, D, A,  2, 23, 0xC4AC5665);

	/* Round 4 */
	CLOWNMD5_ROUND4(A, B, C, D,  0,  6, 0xF4292244);
	CLOWNMD5_ROUND4(D, A, B, C,  7, 10, 0x432AFF97);
	CLOWNMD5_ROUND4(C, D, A, B, 14, 15, 0xAB9423A7);
	CLOWNMD5_ROUND4(B, C, D, A,  5, 21, 0xFC93A039);

	CLOWNMD5_ROUND4(A, B, C, D, 12,  6, 0x655B59C3);
	CLOWNMD5_ROUND4(D, A, B, C,  3, 10, 0x8F0CCC92);
	CLOWNMD5_ROUND4(C, D, A, B, 10, 15, 0xFFEFF47D);
	CLOWNMD5_ROUND4(B, C, D, A,  1, 21, 0x85845DD1);

	CLOWNMD5_ROUND4(A, B, C, D,  8,  6, 0x6FA87E4F);
	CLOWNMD5_ROUND4(D, A, B, C, 15, 10, 0xFE2CE6E0);
	CLOWNMD5_ROUND4(C, D, A, B,  6, 15, 0xA3014314);
	CLOWNMD5_ROUND4(B, C, D, A, 13, 21, 0x4E0811A1);

	CLOWNMD5_ROUND4(A, B, C, D,  4,  6, 0xF7537E82);
	CLOWNMD5_ROUND4(D, A, B, C, 11, 10, 0xBD3AF235);
	CLOWNMD5_ROUND4(C, D, A, B,  2, 15, 0x2AD7D2BB);
	CLOWNMD5_ROUND4(B, C, D, A,  9, 21, 0xEB86D391);

	state->A += A;
	state->B += B;
	state->C += C;
	state->D += D;
}

CLOWNMD5_API void ClownMD5_Init(ClownMD5_State *state)
{
	/* "Step 3. Initialise MD Buffer" */
	state->A = 0x67452301;
	state->B = 0xEFCDAB89;
	state->C = 0x98BADCFE;
	state->D = 0x10325476;

	state->total_bits = 0;
}

CLOWNMD5_API void ClownMD5_PushData(ClownMD5_State *state, const unsigned char data[16 * 4])
{
	state->total_bits += 16 * 4 * 8;

	ClownMD5_ProcessBlock(state, data);
}

CLOWNMD5_API void ClownMD5_PushFinalData(ClownMD5_State *state, unsigned char data[16 * 4], unsigned int bits, unsigned char hash[16])
{
	unsigned int i, j;

	state->total_bits += bits;

	/* If this is a full block, then process it and start from scratch with an empty block. */
	if (bits == 16 * 4 * 8)
	{
		ClownMD5_ProcessBlock(state, data);
		bits = 0;
	}

	i = bits / 8;

	/* Insert the termination bit at the end of the block's data. */
	/* While we're doing this, pad to the next byte. */
	data[i] &= ~((1 << (7 - (bits & 7) + 1)) - 1); /* Clear the spare bits. */
	data[i] |= 1 << (7 - (bits & 7)); /* Set the first bit after the data. */

	++i;

	/* If we're too close to the end of the block, then complete this one and then start a new one. */
	if (i > 16 * 4 - 8)
	{
		/* Fill the rest of the block with padding. */
		for (; i < 16 * 4; ++i)
			data[i] = 0;

		/* Process the block. */
		ClownMD5_ProcessBlock(state, data);

		/* Start a new block. */
		i = 0;
	}

	/* "Step 1. Append Padding Bits" */
	for (; i < 16 * 4 - 8; ++i)
		data[i] = 0;

	/* "Step 2. Append Length" */
	for (j = 0; j < CLOWNMD5_MIN(sizeof(size_t), 8); ++i, ++j)
		data[i] = (state->total_bits >> (8 * j)) & 0xFF;

	/* Add zeroes where the length didn't fill the remaining space. */
	for (; i < 16 * 4; ++i)
		data[i] = 0;

	/* Process the final block. */
	ClownMD5_ProcessBlock(state, data);

	/* "Step 5. Output" */
	if (hash != NULL)
	{
		for (i = 0; i < 4; ++i)
		{
			hash[4 * 0 + i] = (state->A >> (8 * i)) & 0xFF;
			hash[4 * 1 + i] = (state->B >> (8 * i)) & 0xFF;
			hash[4 * 2 + i] = (state->C >> (8 * i)) & 0xFF;
			hash[4 * 3 + i] = (state->D >> (8 * i)) & 0xFF;
		}
	}
}

#endif /* CLOWNMD5_GUARD_FUNCTION_DEFINITIONS */

#endif /* CLOWNMD5_IMPLEMENTATION */

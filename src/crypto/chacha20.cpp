/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   chacha20.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "crypto/chacha20.hpp"

#include <cstddef>

/*
** ChaCha20 as specified by RFC 8439: a 256-bit key, a 96-bit nonce and a
** 32-bit block counter producing a 64-byte keystream block. It uses no
** lookup tables, so no substitution box or algorithm constant has to be
** embedded in the stub.
*/

namespace
{
enum : uint32_t
{
	BLOCK_SIZE = 64,
	ROUND_PAIRS = 10,
	KEY_WORDS = 8,
	NONCE_WORDS = 3,
	MASK_SIZE = 32
};

uint32_t	rotl32(uint32_t value, int shift)
{
	return ((value << shift) | (value >> (32 - shift)));
}

// "expand 32-byte k" assembled from characters rather than written as a
// contiguous string, so it never appears literally in the stub image.
void	state_constants(uint32_t *state)
{
	state[0] = static_cast<uint32_t>('e')
		| (static_cast<uint32_t>('x') << 8)
		| (static_cast<uint32_t>('p') << 16)
		| (static_cast<uint32_t>('a') << 24);
	state[1] = static_cast<uint32_t>('n')
		| (static_cast<uint32_t>('d') << 8)
		| (static_cast<uint32_t>(' ') << 16)
		| (static_cast<uint32_t>('3') << 24);
	state[2] = static_cast<uint32_t>('2')
		| (static_cast<uint32_t>('-') << 8)
		| (static_cast<uint32_t>('b') << 16)
		| (static_cast<uint32_t>('y') << 24);
	state[3] = static_cast<uint32_t>('t')
		| (static_cast<uint32_t>('e') << 8)
		| (static_cast<uint32_t>(' ') << 16)
		| (static_cast<uint32_t>('k') << 24);
}

uint32_t	load32_le(unsigned char *data)
{
	uint32_t	value;
	uint32_t	index;

	value = 0;
	index = 0;
	while (index < 4)
	{
		value |= static_cast<uint32_t>(data[index]) << (index * 8);
		index++;
	}
	return (value);
}

void	store32_le(unsigned char *data, uint32_t value)
{
	int	index;

	index = 0;
	while (index < 4)
	{
		data[index] = static_cast<unsigned char>((value >> (index * 8)) & 0xFF);
		index++;
	}
}

void	quarter_round(uint32_t *state, int a, int b, int c, int d)
{
	state[a] = state[a] + state[b];
	state[d] = rotl32(state[d] ^ state[a], 16);
	state[c] = state[c] + state[d];
	state[b] = rotl32(state[b] ^ state[c], 12);
	state[a] = state[a] + state[b];
	state[d] = rotl32(state[d] ^ state[a], 8);
	state[c] = state[c] + state[d];
	state[b] = rotl32(state[b] ^ state[c], 7);
}

void	keystream_block(unsigned char *key, unsigned char *nonce, uint32_t counter, unsigned char *output)
{
	uint32_t	initial[16];
	uint32_t	state[16];
	uint32_t	index;

	state_constants(initial);
	index = 0;
	while (index < KEY_WORDS)
	{
		initial[4 + index] = load32_le(key + index * 4);
		index++;
	}
	initial[12] = counter;
	index = 0;
	while (index < NONCE_WORDS)
	{
		initial[13 + index] = load32_le(nonce + index * 4);
		index++;
	}
	index = 0;
	while (index < 16)
	{
		state[index] = initial[index];
		index++;
	}
	index = 0;
	while (index < ROUND_PAIRS)
	{
		quarter_round(state, 0, 4, 8, 12);
		quarter_round(state, 1, 5, 9, 13);
		quarter_round(state, 2, 6, 10, 14);
		quarter_round(state, 3, 7, 11, 15);
		quarter_round(state, 0, 5, 10, 15);
		quarter_round(state, 1, 6, 11, 12);
		quarter_round(state, 2, 7, 8, 13);
		quarter_round(state, 3, 4, 9, 14);
		index++;
	}
	index = 0;
	while (index < 16)
	{
		store32_le(output + index * 4, state[index] + initial[index]);
		index++;
	}
}

void	keystream_xor(std::string &data, unsigned char *key, unsigned char *nonce)
{
	unsigned char	stream[BLOCK_SIZE];
	size_t		offset;
	size_t		chunk;
	size_t		available;
	size_t		index;
	uint32_t	counter;

	offset = 0;
	counter = 0;
	while (offset < data.size())
	{
		keystream_block(key, nonce, counter, stream);
		available = data.size() - offset;
		chunk = available;
		if (chunk > BLOCK_SIZE)
			chunk = BLOCK_SIZE;
		index = 0;
		while (index < chunk)
		{
			data[offset + index] = static_cast<char>(
				static_cast<unsigned char>(data[offset + index]) ^ stream[index]);
			index++;
		}
		offset += chunk;
		counter++;
	}
}
}

bool	crypto::chacha20::encrypt(std::string &data, unsigned char *key, unsigned char *nonce)
{
	keystream_xor(data, key, nonce);
	return (true);
}

bool	crypto::chacha20::decrypt(std::string &data, unsigned char *key, unsigned char *nonce)
{
	keystream_xor(data, key, nonce);
	return (true);
}

// Folds a nonce-derived keystream into the key. The packer stores the key
// masked this way and the stub unmasks it with the nonce it reads back from
// the footer, so the key itself is never a literal in the stub image.
void	crypto::chacha20::mask(unsigned char *key, unsigned char *nonce)
{
	unsigned char	zero_key[KEY_WORDS * 4];
	unsigned char	stream[BLOCK_SIZE];
	uint32_t	index;

	index = 0;
	while (index < KEY_WORDS * 4)
	{
		zero_key[index] = 0;
		index++;
	}
	keystream_block(zero_key, nonce, 0, stream);
	index = 0;
	while (index < MASK_SIZE)
	{
		key[index] = key[index] ^ stream[index];
		index++;
	}
}

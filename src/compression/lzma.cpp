/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lzma.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "compression/lzma.hpp"

#include "LzmaLib.h"

#include <cstddef>

/*
** Stored stream layout: five property bytes, then the LZMA payload. The
** properties describe literal context bits, position bits and dictionary
** size, and are required by the decoder to rebuild its probability model.
*/

namespace
{
enum : uint32_t
{
	PROPS_SIZE = 5,
	LEVEL = 9,
	DICT_SIZE = 1u << 24,
	LC = 3,
	LP = 0,
	PB = 2,
	FB = 64,
	THREADS = 1
};

// Worst case bound: the encoder never grows past the input plus half.
size_t	lzma_bound(size_t input_size)
{
	return (PROPS_SIZE + input_size + input_size / 2 + 4096);
}
}

bool	compression::lzma::compress(std::string input, std::string &output)
{
	size_t	props_size;
	size_t	stored_size;
	int		result;

	output.clear();
	if (input.empty())
		return (true);
	output.resize(lzma_bound(input.size()));
	props_size = LZMA_PROPS_SIZE;
	stored_size = output.size() - LZMA_PROPS_SIZE;
	result = LzmaCompress(
		reinterpret_cast<unsigned char *>(&output[LZMA_PROPS_SIZE]), &stored_size,
		reinterpret_cast<unsigned char *>(&input[0]), input.size(),
		reinterpret_cast<unsigned char *>(&output[0]), &props_size,
		LEVEL, DICT_SIZE, LC, LP, PB, FB, THREADS);
	if (result != SZ_OK)
	{
		output.clear();
		return (false);
	}
	output.resize(LZMA_PROPS_SIZE + stored_size);
	return (true);
}

bool	compression::lzma::decompress(std::string input, uint64_t original_size, std::string &output)
{
	size_t	destination_size;
	SizeT	source_size;
	int		result;

	if (original_size > output.max_size())
		return (false);
	if (input.size() < LZMA_PROPS_SIZE)
		return (false);
	output.resize(static_cast<size_t>(original_size));
	if (original_size == 0)
		return (true);
	destination_size = static_cast<size_t>(original_size);
	source_size = input.size() - LZMA_PROPS_SIZE;
	result = LzmaUncompress(
		reinterpret_cast<unsigned char *>(&output[0]), &destination_size,
		reinterpret_cast<unsigned char *>(&input[LZMA_PROPS_SIZE]), &source_size,
		reinterpret_cast<unsigned char *>(&input[0]), LZMA_PROPS_SIZE);
	if (result != SZ_OK)
		return (false);
	if (destination_size != original_size)
		return (false);
	return (true);
}

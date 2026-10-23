/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compression.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 01:13:57 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 01:13:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "compression/compression.hpp"

#include <windows.h>
#include <compressapi.h>

bool	compression::compress(std::string input, std::string &output)
{
	COMPRESSOR_HANDLE	compressor;
	SIZE_T			compressed_size;

	if (input.empty())
	{
		output.clear();
		return (true);
	}
	compressor = NULL;
	if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &compressor))
		return (false);
	compressed_size = 0;
	if (!Compress(compressor, input.data(), input.size(), NULL, 0, &compressed_size))
	{
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			CloseCompressor(compressor);
			return (false);
		}
	}
	output.resize(compressed_size);
	if (!Compress(compressor, input.data(), input.size(), output.data(), output.size(), &compressed_size))
	{
		CloseCompressor(compressor);
		return (false);
	}
	output.resize(compressed_size);
	if (!CloseCompressor(compressor))
		return (false);
	return (true);
}

bool	compression::decompress(std::string input, uint64_t original_size, std::string &output)
{
	DECOMPRESSOR_HANDLE	decompressor;
	SIZE_T			decompressed_size;

	if (original_size == 0)
	{
		output.clear();
		return (input.empty());
	}
	if (original_size > output.max_size())
		return (false);
	decompressor = NULL;
	if (!CreateDecompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, NULL, &decompressor))
		return (false);
	output.resize(static_cast<size_t>(original_size));
	decompressed_size = 0;
	if (!Decompress(decompressor, input.data(), input.size(), output.data(), output.size(), &decompressed_size))
	{
		CloseDecompressor(decompressor);
		return (false);
	}
	if (!CloseDecompressor(decompressor))
		return (false);
	if (decompressed_size != original_size)
		return (false);
	output.resize(decompressed_size);
	return (true);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 02:08:58 by marvin            #+#    #+#             */
/*   Updated: 2026/10/23 02:08:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include "compression/compression.hpp"
#include "file/file.hpp"

#include <windows.h>
#include <cstdint>
#include <iostream>
#include <string>

uint64_t	read_uint64_le(char *data)
{
	uint64_t	value;
	int		index;

	value = 0;
	index = 0;
	while (index < 8)
	{
		value |= static_cast<uint64_t>(static_cast<unsigned char>(data[index])) << (index * 8);
		index++;
	}
	return (value);
}

HANDLE	open_self(LARGE_INTEGER &self_size)
{
	char	self_path[MAX_PATH];
	DWORD	path_length;
	HANDLE	self_file;

	path_length = GetModuleFileNameA(NULL, self_path, MAX_PATH);
	if (path_length == 0 || path_length >= MAX_PATH)
		return (INVALID_HANDLE_VALUE);
	self_file = CreateFileA(self_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (self_file == INVALID_HANDLE_VALUE)
		return (INVALID_HANDLE_VALUE);
	if (!GetFileSizeEx(self_file, &self_size))
	{
		CloseHandle(self_file);
		return (INVALID_HANDLE_VALUE);
	}
	return (self_file);
}

bool	read_footer(HANDLE self_file, LARGE_INTEGER self_size, uint64_t &compressed_size, uint64_t &original_size)
{
	LARGE_INTEGER	footer_offset;
	char			footer[OBSD_FOOTER_SIZE];
	DWORD			read_size;

	if (self_size.QuadPart < OBSD_FOOTER_SIZE)
		return (false);
	footer_offset.QuadPart = self_size.QuadPart - OBSD_FOOTER_SIZE;
	if (!SetFilePointerEx(self_file, footer_offset, NULL, FILE_BEGIN))
		return (false);
	if (!ReadFile(self_file, footer, sizeof(footer), &read_size, NULL))
		return (false);
	if (read_size != sizeof(footer))
		return (false);
	if (footer[0] != OBSD[0] || footer[1] != OBSD[1] || footer[2] != OBSD[2] || footer[3] != OBSD[3])
		return (false);
	compressed_size = read_uint64_le(footer + 4);
	original_size = read_uint64_le(footer + 12);
	if (compressed_size > static_cast<uint64_t>(self_size.QuadPart - OBSD_FOOTER_SIZE))
		return (false);
	return (true);
}

bool	read_payload(HANDLE self_file, LARGE_INTEGER self_size, uint64_t compressed_size, std::string &payload)
{
	LARGE_INTEGER	payload_offset;
	uint64_t		remaining;
	size_t			position;
	DWORD			read_size;
	DWORD			to_read;

	if (compressed_size > payload.max_size())
		return (false);
	payload_offset.QuadPart = self_size.QuadPart - OBSD_FOOTER_SIZE - static_cast<LONGLONG>(compressed_size);
	if (!SetFilePointerEx(self_file, payload_offset, NULL, FILE_BEGIN))
		return (false);
	payload.resize(static_cast<size_t>(compressed_size));
	remaining = compressed_size;
	position = 0;
	while (remaining > 0)
	{
		to_read = 4096;
		if (remaining < to_read)
			to_read = static_cast<DWORD>(remaining);
		if (!ReadFile(self_file, payload.data() + position, to_read, &read_size, NULL))
			return (false);
		if (read_size != to_read)
			return (false);
		remaining -= read_size;
		position += read_size;
	}
	return (true);
}

int	main(void)
{
	LARGE_INTEGER	self_size;
	HANDLE			self_file;
	std::string	compressed;
	std::string	payload;
	uint64_t		compressed_size;
	uint64_t		original_size;

	self_file = open_self(self_size);
	if (self_file == INVALID_HANDLE_VALUE)
		return (1);
	if (!read_footer(self_file, self_size, compressed_size, original_size))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (!read_payload(self_file, self_size, compressed_size, compressed))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (!CloseHandle(self_file))
		return (1);
	if (!compression::decompress(compressed, original_size, payload))
		return (1);
	if (!file::write("extracted.bin", payload))
		return (1);
	std::cout << "Compressed size: " << compressed_size << " bytes\n";
	std::cout << "Original size: " << original_size << " bytes\n";
	return (0);
}

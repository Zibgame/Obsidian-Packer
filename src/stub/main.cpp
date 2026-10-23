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
#include <windows.h>
#include <cstdint>
#include <iostream>

int	main(void)
{
	LARGE_INTEGER	footer_offset;
	LARGE_INTEGER	payload_offset;
	LARGE_INTEGER	self_size;
	char			buffer[4096];
	char			footer[12];
	char			self_path[MAX_PATH];
	int			index;
	DWORD			path_length;
	DWORD			read_size;
	DWORD			to_read;
	DWORD			written;
	HANDLE			extracted_file;
	HANDLE			self_file;
	uint64_t		payload_size;
	uint64_t		remaining;

	path_length = GetModuleFileNameA(NULL, self_path, MAX_PATH);
	if (path_length == 0 || path_length >= MAX_PATH)
		return (1);
	self_file = CreateFileA(self_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (self_file == INVALID_HANDLE_VALUE)
		return (1);
	if (!GetFileSizeEx(self_file, &self_size))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (self_size.QuadPart < 12)
	{
		CloseHandle(self_file);
		return (1);
	}
	footer_offset.QuadPart = self_size.QuadPart - 12;
	if (!SetFilePointerEx(self_file, footer_offset, NULL, FILE_BEGIN))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (!ReadFile(self_file, footer, sizeof(footer), &read_size, NULL))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (read_size != sizeof(footer))
	{
		CloseHandle(self_file);
		return (1);
	}
	if (footer[0] != OBSD[0] || footer[1] != OBSD[1] || footer[2] != OBSD[2] || footer[3] != OBSD[3])
	{
		CloseHandle(self_file);
		return (1);
	}
	payload_size = 0;
	index = 0;
	while (index < 8)
	{
		payload_size |= static_cast<uint64_t>(static_cast<unsigned char>(footer[index + 4])) << (index * 8);
		index++;
	}
	if (payload_size > static_cast<uint64_t>(self_size.QuadPart - 12))
	{
		CloseHandle(self_file);
		return (1);
	}
	payload_offset.QuadPart = self_size.QuadPart - 12 - static_cast<LONGLONG>(payload_size);
	if (!SetFilePointerEx(self_file, payload_offset, NULL, FILE_BEGIN))
	{
		CloseHandle(self_file);
		return (1);
	}
	extracted_file = CreateFileA("extracted.bin", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (extracted_file == INVALID_HANDLE_VALUE)
	{
		CloseHandle(self_file);
		return (1);
	}
	remaining = payload_size;
	while (remaining > 0)
	{
		to_read = sizeof(buffer);
		if (remaining < sizeof(buffer))
			to_read = static_cast<DWORD>(remaining);
		if (!ReadFile(self_file, buffer, to_read, &read_size, NULL))
		{
			CloseHandle(self_file);
			CloseHandle(extracted_file);
			DeleteFileA("extracted.bin");
			return (1);
		}
		if (read_size != to_read)
		{
			CloseHandle(self_file);
			CloseHandle(extracted_file);
			DeleteFileA("extracted.bin");
			return (1);
		}
		if (!WriteFile(extracted_file, buffer, read_size, &written, NULL))
		{
			CloseHandle(self_file);
			CloseHandle(extracted_file);
			DeleteFileA("extracted.bin");
			return (1);
		}
		if (written != read_size)
		{
			CloseHandle(self_file);
			CloseHandle(extracted_file);
			DeleteFileA("extracted.bin");
			return (1);
		}
		remaining -= read_size;
	}
	if (!CloseHandle(self_file))
	{
		CloseHandle(extracted_file);
		DeleteFileA("extracted.bin");
		return (1);
	}
	if (!CloseHandle(extracted_file))
	{
		DeleteFileA("extracted.bin");
		return (1);
	}
	std::cout << "Payload size: " << payload_size << " bytes\n";
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 02:08:58 by marvin            #+#    #+#             */
/*   Updated: 2026/10/23 23:29:15 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include "compression/compression.hpp"
#include "file/file.hpp"
#include "pe/pe.hpp"

#include <windows.h>
#include <intrin.h>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

uintptr_t get_original_entry_point(char *obsram, uint32_t obsram_size, PEInfo &info)
{
    uint32_t entry_rva;

    entry_rva = info.optional.entry_point_rva;

    if (obsram == nullptr)
        return (0);
    if (entry_rva >= obsram_size)
        return (0);
    if (entry_rva >= info.optional.size_of_image)
        return (0);

    return (reinterpret_cast<uintptr_t>(obsram) + entry_rva);
}

// rebuild a uint32 from 4 raw bytes, little-endian (smallest byte first)
uint32_t	read_uint32_le(char *data)
{
	uint32_t	value;
	int		index;

	value = 0;
	index = 0;
	while (index < 4)
	{
		value |= static_cast<uint32_t>(static_cast<unsigned char>(data[index])) << (index * 8);
		index++;
	}
	return (value);
}

// rebuild a uint16 from 2 raw bytes, little-endian (smallest byte first)
uint16_t	read_uint16_le(char *data)
{
	uint16_t	value;

	value = static_cast<uint16_t>(
		static_cast<uint16_t>(static_cast<unsigned char>(data[0]))
		| (static_cast<uint16_t>(static_cast<unsigned char>(data[1])) << 8));
	return (value);
}

// rebuild a uint64 from 8 raw bytes, little-endian (smallest byte first)
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

// open our own exe on disk so we can read the payload we carry
HANDLE	open_self(LARGE_INTEGER &self_size)
{
	char	self_path[MAX_PATH];
	DWORD	path_length;
	HANDLE	self_file;

	// NULL = current process, gives us the full path of this .exe
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

// cut the last 20 bytes of the file and decode the footer:
// [OBSD][compressed size: u64 LE][original size: u64 LE]
bool	read_footer(HANDLE self_file, LARGE_INTEGER self_size, uint64_t &compressed_size, uint64_t &original_size)
{
	LARGE_INTEGER	footer_offset;
	char			footer[OBSD_FOOTER_SIZE];
	DWORD			read_size;

	// the file must at least contain a footer
	if (self_size.QuadPart < OBSD_FOOTER_SIZE)
		return (false);
	// footer is the last 20 bytes of the file
	footer_offset.QuadPart = self_size.QuadPart - OBSD_FOOTER_SIZE;
	if (!SetFilePointerEx(self_file, footer_offset, NULL, FILE_BEGIN))
		return (false);
	if (!ReadFile(self_file, footer, sizeof(footer), &read_size, NULL))
		return (false);
	if (read_size != sizeof(footer))
		return (false);
	// no valid marker means we are a plain stub with no payload
	if (footer[0] != OBSD[0] || footer[1] != OBSD[1] || footer[2] != OBSD[2] || footer[3] != OBSD[3])
		return (false);
	compressed_size = read_uint64_le(footer + 4);
	original_size = read_uint64_le(footer + 12);
	// refuse sizes that would read outside the file
	if (compressed_size > static_cast<uint64_t>(self_size.QuadPart - OBSD_FOOTER_SIZE))
		return (false);
	return (true);
}

// compressed payload sits just before the footer, read it by 4096-byte chunks
bool	read_payload(HANDLE self_file, LARGE_INTEGER self_size, uint64_t compressed_size, std::string &payload)
{
	LARGE_INTEGER	payload_offset;
	uint64_t		remaining;
	size_t			position;
	DWORD			read_size;
	DWORD			to_read;

	if (compressed_size > payload.max_size())
		return (false);
	// payload start = footer start minus compressed size
	payload_offset.QuadPart = self_size.QuadPart - OBSD_FOOTER_SIZE - static_cast<LONGLONG>(compressed_size);
	if (!SetFilePointerEx(self_file, payload_offset, NULL, FILE_BEGIN))
		return (false);
	payload.resize(static_cast<size_t>(compressed_size));
	remaining = compressed_size;
	position = 0;
	while (remaining > 0)
	{
		// cap each ReadFile at 4096 bytes
		to_read = 4096;
		if (remaining < to_read)
			to_read = static_cast<DWORD>(remaining);
		if (!ReadFile(self_file, payload.data() + position, to_read, &read_size, NULL))
			return (false);
		// a short read means the file changed under us, stop here
		if (read_size != to_read)
			return (false);
		remaining -= read_size;
		position += read_size;
	}
	return (true);
}

// walk our own loaded image's section table and locate the .obsram section
bool	find_obsram(char *&obsram, uint32_t &obsram_size)
{
	char	*base;
	char	*nt_headers;
	uint16_t	section_count;
	uint16_t	optional_size;
	char	*section_table;
	uint16_t	i;

	obsram = NULL;
	obsram_size = 0;
	// HMODULE of the current process == base address of our loaded image
	base = reinterpret_cast<char *>(GetModuleHandleA(NULL));
	if (base == NULL)
		return (false);
	if (base[0] != 'M' || base[1] != 'Z')
		return (false);
	nt_headers = base + read_uint32_le(base + 0x3C);
	if (nt_headers[0] != 'P' || nt_headers[1] != 'E')
		return (false);
	section_count = read_uint16_le(nt_headers + 6);
	optional_size = read_uint16_le(nt_headers + 20);
	section_table = nt_headers + 24 + optional_size;
	i = 0;
	while (i < section_count)
	{
		char	*header;

		header = section_table + static_cast<size_t>(i) * 40;
		// name field is 8 bytes: ".obsram" plus its NUL terminator
		if (std::memcmp(header, ".obsram", 8) == 0)
		{
			obsram_size = read_uint32_le(header + 8);
			obsram = base + read_uint32_le(header + 12);
			if (obsram_size == 0)
				return (false);
			return (true);
		}
		i++;
	}
	return (false);
}

// rebuild the original image in .obsram, file layout to memory layout
bool	restore_sections(std::string &payload, PEInfo &info, char *obsram, uint32_t obsram_size)
{
	size_t	i;

	// .obsram must be able to hold the whole original image layout
	if (obsram_size < info.optional.size_of_image)
		return (false);
	// copy the original headers too, without them the image starts with zeros
	if (info.optional.size_of_headers > payload.size()
		|| info.optional.size_of_headers > obsram_size)
		return (false);
	std::copy_n(payload.data(), info.optional.size_of_headers, obsram);
	i = 0;
	while (i < info.sections.size())
	{
		SectionInfo	&section = info.sections[i];
		size_t	copy_size;
		size_t	virtual_size;

		copy_size = section.raw_size;
		virtual_size = section.virtual_size;
		if (virtual_size == 0)
			virtual_size = copy_size;
		// never copy more than the section claims in memory
		if (copy_size > virtual_size)
			copy_size = virtual_size;
		// section must fit inside .obsram
		if (section.virtual_address > obsram_size)
			return (false);
		if (copy_size > obsram_size - section.virtual_address)
			return (false);
		// source must fit inside the decompressed file
		if (section.raw_offset > payload.size())
			return (false);
		if (copy_size > payload.size() - section.raw_offset)
			return (false);
		if (copy_size != 0)
			std::copy_n(payload.data() + section.raw_offset, copy_size,
				obsram + section.virtual_address);
		i++;
	}
	return (true);
}

// delta = obsram - ImageBase: data was built for ImageBase, lives at obsram
bool	patch_relocations(std::string &payload, PEInfo &info, char *obsram, uint32_t obsram_size, size_t &patched)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	reloc_rva;
	uint32_t	reloc_size;
	size_t	offset;
	size_t	available;
	size_t	position;
	size_t	end;

	patched = 0;
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	if (info.coff.size_of_optional_header < 112)
		return (false);
	if (optional_offset > payload.size()
		|| info.coff.size_of_optional_header > payload.size() - optional_offset)
		return (false);
	directory_count = read_uint32_le(payload.data() + optional_offset + 108);
	if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
		return (false);
	// BASE RELOCATION is data directory index 5
	if (directory_count <= 5)
		return (true);
	reloc_rva = read_uint32_le(payload.data() + optional_offset + 152);
	reloc_size = read_uint32_le(payload.data() + optional_offset + 156);
	if (reloc_rva == 0 && reloc_size == 0)
	{
		// no relocation table: only safe when loaded at the preferred base
		if ((uintptr_t)obsram != info.optional.image_base)
		{
			std::cout << "obsidian: original has no relocations and cannot load at its ImageBase\n";
			return (false);
		}
		return (true);
	}
	if (reloc_rva == 0 || reloc_size == 0)
		return (false);
	if (!rva_to_offset(payload, info, reloc_rva, offset, available))
		return (false);
	if (reloc_size > available)
		return (false);
	position = offset;
	end = position + reloc_size;
	while (end - position >= 8)
	{
		char	*block;
		uint32_t	page_rva;
		uint32_t	block_size;
		size_t	entry_count;
		size_t	entry;

		block = payload.data() + position;
		page_rva = read_uint32_le(block);
		block_size = read_uint32_le(block + 4);
		// block header plus a whole number of 2-byte entries
		if (block_size < 8 || block_size > end - position || (block_size & 1) != 0)
			return (false);
		entry_count = (block_size - 8) / 2;
		entry = 0;
		while (entry < entry_count)
		{
			uint16_t	value;
			uint16_t	type;
			uint16_t	displacement;
			uint64_t	rva;

			value = read_uint16_le(block + 8 + entry * 2);
			type = value >> 12;
			displacement = value & 0x0FFF;
			// ABSOLUTE entries are padding, skip them
			if (type != 0)
			{
				// DIR64 is the only real relocation type in a pure PE32+
				if (type != 10)
				{
					std::cout << "obsidian: unsupported relocation type " << type << "\n";
					return (false);
				}
				rva = static_cast<uint64_t>(page_rva) + displacement;
				// the corrected value must lie inside the restored image
				if (rva + 8 > info.optional.size_of_image || rva + 8 > obsram_size)
					return (false);
				*(uint64_t *)(obsram + rva) += reinterpret_cast<uintptr_t>(obsram)
					- info.optional.image_base;
				patched++;
			}
			entry++;
		}
		position += block_size;
	}
	return (true);
}

// helper: read one uint64 at a file RVA, false if it does not fit
bool	read_u64_at(std::string &payload, PEInfo &info, uint32_t rva, uint64_t &value)
{
	size_t	offset;
	size_t	available;

	if (!rva_to_offset(payload, info, rva, offset, available))
		return (false);
	if (available < 8)
		return (false);
	value = read_uint64_le(payload.data() + offset);
	return (true);
}

// resolve the original imports inside the restored image:
// walk the import descriptors, load every DLL, patch the IAT entries
bool	resolve_imports(std::string &payload, PEInfo &info, char *obsram, uint32_t obsram_size, size_t &dll_count, size_t &func_count)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	import_rva;
	uint32_t	import_size;
	size_t	offset;
	size_t	available;
	size_t	position;
	size_t	end;

	dll_count = 0;
	func_count = 0;
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	if (info.coff.size_of_optional_header < 112)
		return (false);
	if (optional_offset > payload.size()
		|| info.coff.size_of_optional_header > payload.size() - optional_offset)
		return (false);
	directory_count = read_uint32_le(payload.data() + optional_offset + 108);
	if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
		return (false);
	// IMPORT is data directory index 1
	if (directory_count <= 1)
		return (true);
	import_rva = read_uint32_le(payload.data() + optional_offset + 120);
	import_size = read_uint32_le(payload.data() + optional_offset + 124);
	if (import_rva == 0 && import_size == 0)
		return (true);
	if (import_rva == 0 || import_size == 0)
		return (false);
	if (!rva_to_offset(payload, info, import_rva, offset, available))
		return (false);
	if (import_size > available)
		return (false);
	position = offset;
	end = position + import_size;
	while (end - position >= 20)
	{
		char	*descriptor;
		uint32_t	lookup;
		uint32_t	timestamp;
		uint32_t	forwarder;
		uint32_t	name_rva;
		uint32_t	iat;
		char	dll_name[256];
		size_t	name_offset;
		size_t	name_available;
		size_t	name_end;
		HMODULE	module;
		uint32_t	thunk_rva;
		size_t	index;

		descriptor = payload.data() + position;
		lookup = read_uint32_le(descriptor);
		timestamp = read_uint32_le(descriptor + 4);
		forwarder = read_uint32_le(descriptor + 8);
		name_rva = read_uint32_le(descriptor + 12);
		iat = read_uint32_le(descriptor + 16);
		// null descriptor marks the end of the table
		if (lookup == 0 && timestamp == 0 && forwarder == 0 && name_rva == 0 && iat == 0)
			break ;
		// bound imports (non-zero timestamp) are incompatible with our restoration
		if (timestamp != 0)
		{
			std::cout << "obsidian: bound imports are not supported\n";
			return (false);
		}
		if (iat == 0)
			return (false);
		if (!rva_to_offset(payload, info, name_rva, name_offset, name_available))
			return (false);
		if (name_available >= sizeof(dll_name))
			name_available = sizeof(dll_name) - 1;
		name_end = payload.find('\0', name_offset);
		if (name_end == std::string::npos || name_end - name_offset >= name_available)
			return (false);
		std::copy_n(payload.data() + name_offset, name_end - name_offset, dll_name);
		dll_name[name_end - name_offset] = '\0';
		module = LoadLibraryA(dll_name);
		if (module == NULL)
		{
			std::cout << "obsidian: cannot load " << dll_name << " (error " << GetLastError() << ")\n";
			return (false);
		}
		// names live in the OriginalFirstThunk array, fall back to the IAT itself
		thunk_rva = lookup;
		if (thunk_rva == 0)
			thunk_rva = iat;
		// the whole thunk array must lie inside the restored image
		if (thunk_rva >= info.optional.size_of_image)
			return (false);
		index = 0;
		while (true)
		{
			uint64_t	thunk;
			uint64_t	address;
			FARPROC	function;

			// guard against uint32 wraparound on huge thunk tables
			if (static_cast<uint64_t>(thunk_rva) + (index + 1) * 8 > info.optional.size_of_image)
				return (false);
			if (!read_u64_at(payload, info, thunk_rva + static_cast<uint32_t>(index) * 8, thunk))
				return (false);
			// null thunk marks the end of this import array
			if (thunk == 0)
				break ;
			// the IAT slot must lie inside the restored image
			if (static_cast<uint64_t>(iat) + (index + 1) * 8 > info.optional.size_of_image
				|| static_cast<uint64_t>(iat) + (index + 1) * 8 > obsram_size)
				return (false);
			// high bit set means import by ordinal
			if ((thunk >> 63) != 0)
				function = GetProcAddress(module, MAKEINTRESOURCEA(static_cast<uint16_t>(thunk)));
			else
			{
				// low 32 bits are the RVA of a Hint/Name entry: 2-byte hint, then name
				uint32_t	name_rva_entry;
				size_t	func_offset;
				size_t	func_available;
				size_t	func_end;
				char	func_name[256];

				name_rva_entry = static_cast<uint32_t>(thunk & 0xFFFFFFFFULL);
				if (!rva_to_offset(payload, info, name_rva_entry, func_offset, func_available))
					return (false);
				if (func_available < 3)
					return (false);
				func_available -= 2;
				func_offset += 2;
				if (func_available >= sizeof(func_name))
					func_available = sizeof(func_name) - 1;
				func_end = payload.find('\0', func_offset);
				if (func_end == std::string::npos || func_end - func_offset >= func_available)
					return (false);
				std::copy_n(payload.data() + func_offset, func_end - func_offset, func_name);
				func_name[func_end - func_offset] = '\0';
				function = GetProcAddress(module, func_name);
				if (function == NULL)
					std::cout << "obsidian: cannot resolve " << dll_name << "!" << func_name << " (error " << GetLastError() << ")\n";
			}
			if (function == NULL)
				return (false);
			address = reinterpret_cast<uint64_t>(function);
			*(uint64_t *)(obsram + iat + index * 8) = address;
			func_count++;
			index++;
		}
		dll_count++;
		position += 20;
	}
	return (true);
}

// register the x64 exception table (.pdata) of the restored image with Windows:
// pure registration, RtlAddFunctionTable only reads the RUNTIME_FUNCTION array
bool	register_exceptions(std::string &payload, PEInfo &info, char *obsram, uint32_t obsram_size)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	pdata_rva;
	uint32_t	pdata_size;

	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	if (info.coff.size_of_optional_header < 112)
		return (false);
	if (optional_offset > payload.size()
		|| info.coff.size_of_optional_header > payload.size() - optional_offset)
		return (false);
	directory_count = read_uint32_le(payload.data() + optional_offset + 108);
	if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
		return (false);
	// EXCEPTION is data directory index 3
	if (directory_count <= 3)
		return (true);
	pdata_rva = read_uint32_le(payload.data() + optional_offset + 136);
	pdata_size = read_uint32_le(payload.data() + optional_offset + 140);
	if (pdata_rva == 0 && pdata_size == 0)
		return (true);
	if (pdata_rva == 0 || pdata_size == 0)
		return (false);
	// each RUNTIME_FUNCTION is exactly 12 bytes
	if ((pdata_size % 12) != 0)
		return (false);
	if (static_cast<uint64_t>(pdata_rva) + pdata_size > info.optional.size_of_image
		|| static_cast<uint64_t>(pdata_rva) + pdata_size > obsram_size)
		return (false);
	if (!RtlAddFunctionTable(reinterpret_cast<PRUNTIME_FUNCTION>(obsram + pdata_rva),
		pdata_size / 12, reinterpret_cast<DWORD64>(obsram)))
	{
		std::cout << "obsidian: RtlAddFunctionTable failed\n";
		return (false);
	}
	return (true);
}

// count the TLS callbacks of the original: parsed for the audit, never called
bool	audit_tls(std::string &payload, PEInfo &info, size_t &callbacks)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	tls_rva;
	uint32_t	tls_size;
	size_t	offset;
	size_t	available;
	uint64_t	callbacks_rva;

	callbacks = 0;
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	if (info.coff.size_of_optional_header < 112)
		return (false);
	if (optional_offset > payload.size()
		|| info.coff.size_of_optional_header > payload.size() - optional_offset)
		return (false);
	directory_count = read_uint32_le(payload.data() + optional_offset + 108);
	if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
		return (false);
	// TLS is data directory index 9
	if (directory_count <= 9)
		return (true);
	tls_rva = read_uint32_le(payload.data() + optional_offset + 184);
	tls_size = read_uint32_le(payload.data() + optional_offset + 188);
	if (tls_rva == 0 && tls_size == 0)
		return (true);
	if (tls_rva == 0 || tls_size == 0)
		return (false);
	// AddressOfCallBacks sits at offset 24 of the 64-byte TLS structure
	if (!rva_to_offset(payload, info, tls_rva, offset, available) || available < 40)
		return (false);
	callbacks_rva = read_uint64_le(payload.data() + offset + 24);
	if (callbacks_rva == 0)
		return (true);
	// AddressOfCallBacks is an absolute VA, convert it to an RVA first
	if (callbacks_rva < info.optional.image_base
		|| callbacks_rva - info.optional.image_base > info.optional.size_of_image)
		return (false);
	callbacks_rva -= info.optional.image_base;
	while (callbacks < 1024)
	{
		uint64_t	callback;

		if (!read_u64_at(payload, info, static_cast<uint32_t>(callbacks_rva) + static_cast<uint32_t>(callbacks * 8), callback))
			return (false);
		if (callback == 0)
			break ;
		callbacks++;
	}
	if (callbacks >= 1024)
		return (false);
	return (true);
}


// map a permission bitmask (1 read, 2 write, 4 execute) to VirtualProtect
DWORD	perm_to_protect(DWORD perm)
{
	if ((perm & 4) != 0 && (perm & 2) != 0)
		return (PAGE_EXECUTE_READWRITE);
	if ((perm & 4) != 0)
		return (PAGE_EXECUTE_READ);
	if ((perm & 2) != 0)
		return (PAGE_READWRITE);
	return (PAGE_READONLY);
}

// final protections, last step: per-page OR of read/write/execute bits
bool	protect_sections(std::string &payload, PEInfo &info, char *obsram, uint32_t obsram_size)
{
	std::vector<DWORD>	pages;
	size_t	page_count;
	size_t	i;

	(void)payload;
	page_count = (obsram_size + 4095ULL) / 4096ULL;
	pages.assign(page_count, 0);
	i = 0;
	while (i < info.sections.size())
	{
		SectionInfo	&section = info.sections[i];
		uint32_t	flags;
		DWORD	perm;
		uint64_t	size;
		size_t	first_page;
		size_t	last_page;
		size_t	page;

		size = section.virtual_size;
		if (size < section.raw_size)
			size = section.raw_size;
		i++;
		if (size == 0)
			continue ;
		if (static_cast<uint64_t>(section.virtual_address) + size > obsram_size)
			return (false);
		flags = section.characteristics;
		// section characteristics: EXEC 0x20000000, READ 0x40000000, WRITE 0x80000000
		// read is implicit, write and execute are OR-ed per page
		perm = 1;
		if ((flags & 0x20000000U) != 0)
			perm |= 4;
		if ((flags & 0x80000000U) != 0)
			perm |= 2;
		first_page = section.virtual_address / 4096;
		last_page = (section.virtual_address + size - 1) / 4096;
		page = first_page;
		while (page <= last_page)
		{
			pages[page] = pages[page] | perm;
			page++;
		}
	}
	// apply contiguous runs of equal permission with one VirtualProtect call each
	size_t	run_start;
	DWORD	old_value;

	run_start = 0;
	while (run_start < page_count)
	{
		size_t	run_end;
		SIZE_T	byte_count;

		if (pages[run_start] == 0)
		{
			run_start++;
			continue ;
		}
		run_end = run_start + 1;
		while (run_end < page_count && pages[run_end] == pages[run_start])
			run_end++;
		byte_count = (run_end - run_start) * 4096;
		// clamp the final run to the real end of .obsram
		if (run_start * 4096ULL + byte_count > obsram_size)
			byte_count = obsram_size - run_start * 4096;
		if (!VirtualProtect(obsram + run_start * 4096, byte_count,
			perm_to_protect(pages[run_start]), &old_value))
		{
			std::cout << "obsidian: VirtualProtect failed at page " << run_start
				<< " (error " << GetLastError() << ")\n";
			return (false);
		}
		run_start = run_end;
	}
	return (true);
}

// minimal TEB view: ThreadLocalStoragePointer sits at offset 0x58 on x64
// and the TEB self-pointer sits at gs:0x30
struct	TEB_MINI
{
	char	pad[0x58];
	char	**thread_local_storage_pointer;
};

// minimal PEB loader structures: enough to walk the module list
struct	LIST_ENTRY_MINI
{
	char	*flink;
	char	*blink;
};

struct	LDR_ENTRY_MINI
{
	LIST_ENTRY_MINI	in_load_order_links;
	char	pad[0x20];
	char	*dll_base;
};

// next index = one past the highest index the loader actually assigned
size_t	next_tls_index(void)
{
	char	*peb;
	char	*ldr;
	LIST_ENTRY_MINI	*head;
	LIST_ENTRY_MINI	*cur;
	size_t	next;

	peb = reinterpret_cast<char *>(__readgsqword(0x60));
	ldr = *(char **)(peb + 0x18);
	next = 0;
	if (ldr == NULL)
		return (0);
	head = reinterpret_cast<LIST_ENTRY_MINI *>(ldr + 0x10);
	cur = reinterpret_cast<LIST_ENTRY_MINI *>(head->flink);
	while (cur != head && next < 64)
	{
		char	*base;
		uint32_t	e_lfanew;
		uint32_t	size_of_image;

		base = reinterpret_cast<LDR_ENTRY_MINI *>(cur)->dll_base;
		// SizeOfImage sits right after DllBase in the loader entry
		size_of_image = *reinterpret_cast<uint32_t *>(cur + 0x40);
		if (base != NULL && size_of_image != 0
			&& base[0] == 'M' && base[1] == 'Z')
		{
			e_lfanew = read_uint32_le(base + 0x3C);
			// headers must stay in a sane range before we dereference them
			if (e_lfanew >= 0x40 && e_lfanew + 248 < size_of_image
				&& base[e_lfanew] == 'P' && base[e_lfanew + 1] == 'E')
			{
				char	*optional;
				uint32_t	tls_rva;
				uint32_t	tls_size;

				optional = base + e_lfanew + 24;
				if (read_uint16_le(optional) == 0x20B)
				{
					// TLS is data directory index 9: RVA at optional+184
					tls_rva = read_uint32_le(optional + 184);
					tls_size = read_uint32_le(optional + 188);
					if (tls_rva != 0 && tls_size != 0
						&& tls_rva + 64 <= size_of_image)
					{
						uint64_t	index_va;
						uint32_t	assigned;

						index_va = read_uint64_le(base + tls_rva + 16);
						// the assigned index must point inside that module
						if (index_va >= reinterpret_cast<uint64_t>(base)
							&& index_va + 4 <= reinterpret_cast<uint64_t>(base) + size_of_image)
						{
							assigned = read_uint32_le(reinterpret_cast<char *>(index_va));
							if (assigned >= 64)
							{
								std::cout << "obsidian: loader TLS index out of range\n";
								return (64);
							}
							if (assigned + 1 > next)
								next = assigned + 1;
						}
					}
				}
			}
		}
		cur = reinterpret_cast<LIST_ENTRY_MINI *>(cur->flink);
	}
	return (next);
}
// prepare TLS like the loader: extend TEB->ThreadLocalStoragePointer
bool	prepare_tls(std::string &payload, PEInfo &info, char *obsram,
	uint32_t obsram_size, size_t &callbacks, bool &tls_ready)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	tls_rva;
	uint32_t	tls_size;
	char	*tls_dir;
	uint64_t	start_va;
	uint64_t	end_va;
	uint64_t	index_va;
	uint64_t	callbacks_va;
	uint32_t	zero_fill;
	uint64_t	raw_size;
	char	*thread_data;
	char	**tls_array;
	size_t	used;
	char	**new_array;
	TEB_MINI	*teb;

	(void)payload;
	callbacks = 0;
	tls_ready = true;
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	directory_count = read_uint32_le(obsram + optional_offset + 108);
	// TLS is data directory index 9
	if (directory_count <= 9)
		return (true);
	tls_rva = read_uint32_le(obsram + optional_offset + 184);
	tls_size = read_uint32_le(obsram + optional_offset + 188);
	if (tls_rva == 0 && tls_size == 0)
		return (true);
	if (tls_rva == 0 || tls_size == 0)
		return (false);
	// the 64-bit TLS structure is exactly 64 bytes
	if (static_cast<uint64_t>(tls_rva) + 64 > info.optional.size_of_image
		|| static_cast<uint64_t>(tls_rva) + 64 > obsram_size)
		return (false);
	tls_dir = obsram + tls_rva;
	start_va = read_uint64_le(tls_dir);
	end_va = read_uint64_le(tls_dir + 8);
	index_va = read_uint64_le(tls_dir + 16);
	callbacks_va = read_uint64_le(tls_dir + 24);
	zero_fill = read_uint32_le(tls_dir + 32);
	// those VAs were already relocated to their real address inside .obsram
	if (start_va < reinterpret_cast<uint64_t>(obsram)
		|| end_va > reinterpret_cast<uint64_t>(obsram) + obsram_size
		|| end_va < start_va)
	{
		std::cout << "obsidian: TLS raw data outside the restored image\n";
		return (false);
	}
	if (index_va < reinterpret_cast<uint64_t>(obsram)
		|| index_va + 4 > reinterpret_cast<uint64_t>(obsram) + obsram_size)
	{
		std::cout << "obsidian: TLS index outside the restored image\n";
		return (false);
	}
	if (callbacks_va != 0
		&& (callbacks_va < reinterpret_cast<uint64_t>(obsram)
			|| callbacks_va >= reinterpret_cast<uint64_t>(obsram) + obsram_size))
	{
		std::cout << "obsidian: TLS callbacks outside the restored image\n";
		return (false);
	}
	raw_size = end_va - start_va;
	// sanity guard on the data block size
	if (raw_size + zero_fill > 16ULL * 1024 * 1024)
	{
		std::cout << "obsidian: TLS data block too large\n";
		return (false);
	}
	// build the main thread data block: initial raw data then zero fill
	thread_data = static_cast<char *>(HeapAlloc(GetProcessHeap(), 0, raw_size + zero_fill));
	if (thread_data == NULL)
	{
		std::cout << "obsidian: cannot allocate the TLS data block\n";
		return (false);
	}
	if (raw_size != 0)
		std::copy_n(reinterpret_cast<char *>(start_va), raw_size, thread_data);
	std::fill(thread_data + raw_size, thread_data + raw_size + zero_fill, '\0');
	// derive the slot index from loader-written facts (see next_tls_index)
	used = next_tls_index();
	tls_array = reinterpret_cast<char **>(__readgsqword(0x58));
	// consistency: loader array exists iff some module owns a TLS directory
	if (tls_array == NULL && used != 0)
	{
		std::cout << "obsidian: TLS modules exist but the loader array is absent\n";
		return (false);
	}
	if (used >= 64)
	{
		std::cout << "obsidian: TLS directory slots are full\n";
		return (false);
	}
	if (tls_array != NULL)
	{
		// refuse any loader state we did not model
		size_t	check;

		check = 0;
		while (check < used)
		{
			uint64_t	entry;

			entry = reinterpret_cast<uint64_t>(tls_array[check]);
			if (entry < 0x10000ULL || entry > 0x7FFFFFFEFFFFULL || (entry & 7) != 0)
			{
				std::cout << "obsidian: TLS pointer array is in an unexpected state\n";
				return (false);
			}
			check++;
		}
	}
	// extend the array by one slot; the old block is kept, its heap is unknown
	new_array = reinterpret_cast<char **>(HeapAlloc(GetProcessHeap(), 0, (used + 1) * sizeof(char *)));
	if (new_array == NULL)
	{
		std::cout << "obsidian: cannot allocate the TLS slot array\n";
		return (false);
	}
	if (tls_array != NULL && used != 0)
		std::copy_n(tls_array, used, new_array);
	new_array[used] = thread_data;
	teb = reinterpret_cast<TEB_MINI *>(__readgsqword(0x30));
	teb->thread_local_storage_pointer = new_array;
	// read back: the TEB must now expose our array, anything else is fatal
	if (teb->thread_local_storage_pointer != new_array
		|| teb->thread_local_storage_pointer[used] != thread_data)
	{
		std::cout << "obsidian: TLS slot array update did not stick\n";
		return (false);
	}
	*(uint32_t *)index_va = static_cast<uint32_t>(used);
	// count the callbacks, they stay pending, none is executed here
	if (callbacks_va != 0)
	{
		while (callbacks < 1024)
		{
			uint64_t	callback;

			callback = *(uint64_t *)(callbacks_va + callbacks * 8);
			if (callback == 0)
				break ;
			callbacks++;
		}
		if (callbacks >= 1024)
		{
			std::cout << "obsidian: TLS callback array too long\n";
			return (false);
		}
	}
	if (callbacks > 0)
	{
		std::cout << "obsidian: warning, " << callbacks
			<< " TLS callback(s) pending until transfer\n";
		tls_ready = false;
	}
	return (true);
}
// count delay import descriptors: the original resolves them itself at first
// call through its own delay helper, the stub has nothing to patch here
bool	audit_delay_imports(std::string &payload, PEInfo &info, size_t &dlls)
{
	size_t	optional_offset;
	uint32_t	directory_count;
	uint32_t	delay_rva;
	uint32_t	delay_size;
	size_t	offset;
	size_t	available;
	size_t	position;
	size_t	end;

	(void)payload;
	dlls = 0;
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	directory_count = read_uint32_le(payload.data() + optional_offset + 108);
	// DELAY IMPORT is data directory index 13
	if (directory_count <= 13)
		return (true);
	delay_rva = read_uint32_le(payload.data() + optional_offset + 216);
	delay_size = read_uint32_le(payload.data() + optional_offset + 220);
	if (delay_rva == 0 && delay_size == 0)
		return (true);
	if (delay_rva == 0 || delay_size == 0)
		return (false);
	if (!rva_to_offset(payload, info, delay_rva, offset, available))
		return (false);
	if (delay_size > available)
		return (false);
	position = offset;
	end = position + delay_size;
	while (end - position >= 32 && dlls < 4096)
	{
		char	*descriptor;
		uint32_t	gr_attrs;
		uint32_t	name_field;

		descriptor = payload.data() + position;
		gr_attrs = read_uint32_le(descriptor);
		name_field = read_uint32_le(descriptor + 4);
		// null descriptor marks the end of the table
		if (gr_attrs == 0 && name_field == 0)
			break ;
		// grAttrs bit 0 set means the fields are VAs, not RVAs (rare)
		if ((gr_attrs & 1) == 0)
		{
			size_t	name_offset;
			size_t	name_available;

			if (!rva_to_offset(payload, info, name_field, name_offset, name_available))
				return (false);
		}
		dlls++;
		position += 32;
	}
	if (dlls >= 4096)
		return (false);
	return (true);
}
// validation pass: structural validity and readiness checked separately
bool	validate_restored_image(std::string &payload, PEInfo &info, char *obsram,
	uint32_t obsram_size, size_t patched_relocs, bool tls_ready, size_t delay_dlls)
{
	size_t	optional_offset;
	uint32_t	reloc_rva;
	uint32_t	reloc_size;
	uint32_t	import_rva;
	uint32_t	import_size;
	uint32_t	pdata_rva;
	uint32_t	pdata_size;
	uint32_t	e_lfanew;
	size_t	resolved_dlls;
	size_t	resolved_funcs;
	size_t	unwind_count;
	bool	structure_ok;
	std::string	readiness;

	structure_ok = true;
	// the original headers must now be present at the start of .obsram
	if (obsram[0] != 'M' || obsram[1] != 'Z')
	{
		std::cout << "obsidian: validation failed, MZ signature missing in .obsram\n";
		structure_ok = false;
	}
	e_lfanew = read_uint32_le(obsram + 0x3C);
	if (obsram[e_lfanew] != 'P' || obsram[e_lfanew + 1] != 'E')
	{
		std::cout << "obsidian: validation failed, PE signature missing in .obsram\n";
		structure_ok = false;
	}
	// the restored headers must be byte-identical to the decompressed file
	if (info.optional.size_of_headers > payload.size()
		|| info.optional.size_of_headers > obsram_size
		|| std::memcmp(payload.data(), obsram, info.optional.size_of_headers) != 0)
	{
		std::cout << "obsidian: validation failed, restored headers differ from the file\n";
		structure_ok = false;
	}
	// the entry point of the original must be inside the restored image
	if (static_cast<uint64_t>(info.optional.entry_point_rva) + 16 > info.optional.size_of_image
		|| static_cast<uint64_t>(info.optional.entry_point_rva) + 16 > obsram_size)
	{
		std::cout << "obsidian: validation failed, entry point outside the image\n";
		structure_ok = false;
	}
	// the entry point must belong to an executable section of the original
	{
		size_t	index;
		bool	in_exec_section;

		in_exec_section = false;
		index = 0;
		while (index < info.sections.size())
		{
			SectionInfo	&section = info.sections[index];
			uint64_t	section_end;

			section_end = section.virtual_size;
			if (section_end < section.raw_size)
				section_end = section.raw_size;
			section_end += section.virtual_address;
			// EXEC characteristic 0x20000000, entry strictly inside the range
			if (info.optional.entry_point_rva >= section.virtual_address
				&& static_cast<uint64_t>(info.optional.entry_point_rva) < section_end
				&& (section.characteristics & 0x20000000U) != 0)
			{
				in_exec_section = true;
				break ;
			}
			index++;
		}
		if (in_exec_section == false)
		{
			std::cout << "obsidian: validation failed, entry point not in an executable section\n";
			structure_ok = false;
		}
		else
		{
			// the page mapped at the entry address must be executable
			MEMORY_BASIC_INFORMATION	mbi;

			if (VirtualQuery(obsram + info.optional.entry_point_rva, &mbi, sizeof(mbi)) == 0
				|| (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ
					| PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) == 0)
			{
				std::cout << "obsidian: validation failed, entry page is not executable\n";
				structure_ok = false;
			}
		}
	}
	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	reloc_rva = read_uint32_le(obsram + optional_offset + 152);
	reloc_size = read_uint32_le(obsram + optional_offset + 156);
	import_rva = read_uint32_le(obsram + optional_offset + 120);
	import_size = read_uint32_le(obsram + optional_offset + 124);
	pdata_rva = read_uint32_le(obsram + optional_offset + 136);
	pdata_size = read_uint32_le(obsram + optional_offset + 140);
	// a relocation table implies at least one patched entry
	if (reloc_rva != 0 && reloc_size != 0 && patched_relocs == 0)
	{
		std::cout << "obsidian: validation failed, relocations present but none patched\n";
		structure_ok = false;
	}
	// walk the IAT in memory: every slot must hold a resolved address
	resolved_dlls = 0;
	resolved_funcs = 0;
	if (import_rva != 0 && import_size != 0)
	{
		size_t	descriptor_index;

		descriptor_index = 0;
		while (static_cast<uint64_t>(descriptor_index) * 20 < import_size)
		{
			char	*descriptor;
			uint32_t	iat;
			size_t	index;

			descriptor = obsram + import_rva + descriptor_index * 20;
			iat = read_uint32_le(descriptor + 16);
			if (read_uint32_le(descriptor) == 0 && read_uint32_le(descriptor + 4) == 0
				&& read_uint32_le(descriptor + 8) == 0 && read_uint32_le(descriptor + 12) == 0
				&& iat == 0)
				break ;
			if (static_cast<uint64_t>(iat) + 8 > info.optional.size_of_image
				|| static_cast<uint64_t>(iat) + 8 > obsram_size)
			{
				std::cout << "obsidian: validation failed, IAT outside the image\n";
				structure_ok = false;
				break ;
			}
			index = 0;
			while (true)
			{
				uint64_t	slot;

				if (static_cast<uint64_t>(iat) + (index + 1) * 8 > info.optional.size_of_image
					|| static_cast<uint64_t>(iat) + (index + 1) * 8 > obsram_size)
				{
					std::cout << "obsidian: validation failed, IAT grows out of the image\n";
					structure_ok = false;
					break ;
				}
				slot = *(uint64_t *)(obsram + iat + index * 8);
				if (slot == 0)
					break ;
				resolved_funcs++;
				index++;
			}
			resolved_dlls++;
			descriptor_index++;
		}
	}
	// every RUNTIME_FUNCTION must describe a code range inside the image
	unwind_count = 0;
	if (pdata_rva != 0 && pdata_size != 0)
	{
		size_t	index;

		if ((pdata_size % 12) != 0
			|| static_cast<uint64_t>(pdata_rva) + pdata_size > info.optional.size_of_image
			|| static_cast<uint64_t>(pdata_rva) + pdata_size > obsram_size)
		{
			std::cout << "obsidian: validation failed, .pdata out of bounds\n";
			structure_ok = false;
		}
		else
		{
			index = 0;
			while (index < pdata_size / 12)
			{
				uint32_t	begin_rva;
				uint32_t	end_rva;

				begin_rva = read_uint32_le(obsram + pdata_rva + index * 12);
				end_rva = read_uint32_le(obsram + pdata_rva + index * 12 + 4);
				if (begin_rva >= end_rva || end_rva > info.optional.size_of_image)
				{
					std::cout << "obsidian: validation failed, RUNTIME_FUNCTION " << index
						<< " out of bounds\n";
					structure_ok = false;
					break ;
				}
				unwind_count++;
				index++;
			}
		}
	}
	std::string	validity;

	if (structure_ok)
		validity = "valid";
	else
		validity = "INVALID";
	// TLS slot must be reachable and match the decompressed file
	uint32_t	directory_count2 = read_uint32_le(obsram + optional_offset + 108);
	if (directory_count2 > 9)
	{
		uint32_t	tls_rva;
		uint32_t	tls_size;

		tls_rva = read_uint32_le(obsram + optional_offset + 184);
		tls_size = read_uint32_le(obsram + optional_offset + 188);
		if (tls_rva != 0 && tls_size != 0)
		{
			uint64_t	start_va;
			uint64_t	end_va;
			uint64_t	index_va;
			uint64_t	raw_size;
			uint32_t	tls_index;
			char	**tls_array;

			start_va = read_uint64_le(obsram + tls_rva);
			end_va = read_uint64_le(obsram + tls_rva + 8);
			index_va = read_uint64_le(obsram + tls_rva + 16);
			raw_size = end_va - start_va;
			tls_index = read_uint32_le(reinterpret_cast<char *>(index_va));
			tls_array = reinterpret_cast<char **>(__readgsqword(0x58));
			if (tls_array == NULL || tls_array[tls_index] == NULL)
			{
				std::cout << "obsidian: validation failed, TLS slot not reachable\n";
				structure_ok = false;
			}
			else
			{
				size_t	tls_offset;
				size_t	tls_available;

				// in file terms the data lives at start_va - obsram
				if (!rva_to_offset(payload, info, static_cast<uint32_t>(start_va
					- reinterpret_cast<uint64_t>(obsram)), tls_offset, tls_available)
					|| tls_available < raw_size
					|| std::memcmp(tls_array[tls_index],
						payload.data() + tls_offset, raw_size) != 0)
				{
					std::cout << "obsidian: validation failed, TLS data block differs from the file\n";
					structure_ok = false;
				}
			}
		}
	}
	// readiness verdict: separate from structural validity
	if (GetProcessHeap() == NULL || GetCommandLineA() == NULL)
		readiness = "not ready: environment incomplete";
	else if (!tls_ready)
		readiness = "conditional: TLS callbacks pending";
	else if (delay_dlls != 0)
		readiness = "ready: delay imports self-resolve at first call";
	else
		readiness = "ready";
	std::cout << "obsidian: structure " << validity
		<< " (relocs patched=" << patched_relocs
		<< ", dlls=" << resolved_dlls
		<< ", funcs=" << resolved_funcs
		<< ", unwind=" << unwind_count << ")\n";
	std::cout << "obsidian: readiness: " << readiness << "\n";
	if (delay_dlls != 0)
		std::cout << "obsidian: " << delay_dlls << " delay import descriptor(s) found\n";
	return (structure_ok);
}
// run the TLS callbacks with DLL_PROCESS_ATTACH on the restored image
bool	run_tls_callbacks(PEInfo &info, char *obsram, uint32_t obsram_size)
{
	size_t	optional_offset;
	uint32_t	tls_rva;
	uint32_t	tls_size;
	uint64_t	callbacks_va;
	size_t	count;
	size_t	index;

	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	// TLS is data directory index 9
	if (read_uint32_le(obsram + optional_offset + 108) <= 9)
		return (true);
	tls_rva = read_uint32_le(obsram + optional_offset + 184);
	tls_size = read_uint32_le(obsram + optional_offset + 188);
	if (tls_rva == 0 && tls_size == 0)
		return (true);
	if (tls_rva == 0 || tls_size == 0)
		return (false);
	if (static_cast<uint64_t>(tls_rva) + 64 > info.optional.size_of_image
		|| static_cast<uint64_t>(tls_rva) + 64 > obsram_size)
		return (false);
	callbacks_va = read_uint64_le(obsram + tls_rva + 24);
	if (callbacks_va == 0)
		return (true);
	if (callbacks_va < reinterpret_cast<uint64_t>(obsram)
		|| callbacks_va >= reinterpret_cast<uint64_t>(obsram) + obsram_size)
	{
		std::cout << "obsidian: TLS callbacks outside the restored image\n";
		return (false);
	}
	// count and validate every callback before running any of them
	count = 0;
	while (count < 1024)
	{
		uint64_t	callback;

		callback = *(uint64_t *)(callbacks_va + count * 8);
		if (callback == 0)
			break ;
		if (callback < reinterpret_cast<uint64_t>(obsram)
			|| callback >= reinterpret_cast<uint64_t>(obsram) + obsram_size)
		{
			std::cout << "obsidian: TLS callback " << count << " outside the image\n";
			return (false);
		}
		count++;
	}
	if (count >= 1024)
		return (false);
	// run them in order, same as LdrpCallInitRoutines would
	index = 0;
	while (index < count)
	{
		typedef	int (*tls_callback_fn)(void *, DWORD, void *);

		tls_callback_fn	callback;

		callback = reinterpret_cast<tls_callback_fn>(
			*(uint64_t *)(callbacks_va + index * 8));
		callback(reinterpret_cast<void *>(obsram), DLL_PROCESS_ATTACH, NULL);
		index++;
	}
	std::cout << "obsidian: " << count << " TLS callback(s) executed\n";
	return (true);
}

// real transfer: TLS callbacks, then the original entry point
bool	launch_original(char *obsram, uint32_t obsram_size, PEInfo &info)
{
	typedef	int (*entry_fn)(void);
	entry_fn	fn;
	int	return_code;

	if (!run_tls_callbacks(info, obsram, obsram_size))
		return (false);
	// flush our own console output before the original takes over
	std::cout.flush();
	fn = reinterpret_cast<entry_fn>(get_original_entry_point(obsram, obsram_size, info));
	return_code = fn();
	// reached only when the original CRT returns instead of exiting
	ExitProcess(static_cast<UINT>(return_code));
}
int	main(void)
{
	LARGE_INTEGER	self_size;
	HANDLE			self_file;
	std::string	compressed;
	std::string	payload;
	PEInfo	info;
	uint64_t	compressed_size;
	uint64_t	original_size;
	char	*obsram;
	uint32_t	obsram_size;
	size_t	patched_relocs;
	size_t	resolved_dlls;
	size_t	resolved_funcs;
	size_t	tls_callbacks;
	size_t	delay_dlls;
	bool	tls_ready;

	// find ourselves, read footer, read payload, then we can close the file
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
	// decompress the original file into RAM
	if (!compression::decompress(compressed, original_size, payload))
		return (1);
	// recover every PE structure from the restored bytes
	if (!fill_pe_info(payload, info))
		return (1);
	// find the restore area Windows already loaded for us
	if (!find_obsram(obsram, obsram_size))
		return (1);
	// rebuild the original image inside .obsram, nothing is executed yet
	if (!restore_sections(payload, info, obsram, obsram_size))
		return (1);
	// fix absolute addresses for the real location of .obsram
	if (!patch_relocations(payload, info, obsram, obsram_size, patched_relocs))
		return (1);
	// load every imported DLL and fill the original IAT
	if (!resolve_imports(payload, info, obsram, obsram_size, resolved_dlls, resolved_funcs))
		return (1);
	// register the x64 exception table, no code of the original runs here
	if (!register_exceptions(payload, info, obsram, obsram_size))
		return (1);
	// audit only: TLS callbacks are counted, never called
	if (!audit_tls(payload, info, tls_callbacks))
		return (1);
	// delay imports: parsed and counted, the original resolves them itself
	if (!audit_delay_imports(payload, info, delay_dlls))
		return (1);
	// allocate the original TLS slot from loader-written facts, no callback
	if (!prepare_tls(payload, info, obsram, obsram_size, tls_callbacks, tls_ready))
		return (1);
	// final protections, must stay after every patching step
	if (!protect_sections(payload, info, obsram, obsram_size))
		return (1);
	// validation pass over the restored image, nothing is executed
	if (!validate_restored_image(payload, info, obsram, obsram_size,
		patched_relocs, tls_ready, delay_dlls))
		return (1);
	// real transfer: TLS callbacks then the restored entry point
	if (!launch_original(obsram, obsram_size, info))
		return (1);
	return (0);
}

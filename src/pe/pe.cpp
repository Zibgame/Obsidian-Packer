/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pe.cpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 17:42:05 by marvin           #+#    #+#             */
/*   Updated: 2026/10/23 17:42:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "pe/pe.hpp"

#include <iostream>
#include <algorithm>
#include <fstream>

static uint16_t	read_uint16_le(char *data)
{
	uint16_t	value;
	int		index;

	value = 0;
	index = 0;
	while (index < 2)
	{
		value |= static_cast<uint16_t>(static_cast<unsigned char>(data[index])) << (index * 8);
		index++;
	}
	return (value);
}

static uint32_t	read_uint32_le(char *data)
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

static uint64_t	read_uint64_le(char *data)
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

static void	write_uint16_le(char *data, uint16_t value)
{
	int		index;

	index = 0;
	while (index < 2)
	{
		data[index] = static_cast<char>(value >> (index * 8));
		index++;
	}
}

static void	write_uint32_le(char *data, uint32_t value)
{
	int		index;

	index = 0;
	while (index < 4)
	{
		data[index] = static_cast<char>(value >> (index * 8));
		index++;
	}
}

static bool	fill_optional_info(std::string &payload, PEInfo &info)
{
	size_t	optional_offset;

	optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
	if (info.coff.size_of_optional_header < 112 || optional_offset > payload.size() || info.coff.size_of_optional_header > payload.size() - optional_offset)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (optional header out of bounds or too small)\n";
		return (false);
	}
	info.optional.entry_point_rva = read_uint32_le(payload.data() + optional_offset + 16);
	info.optional.image_base = read_uint64_le(payload.data() + optional_offset + 24);
	info.optional.section_alignment = read_uint32_le(payload.data() + optional_offset + 32);
	info.optional.size_of_image = read_uint32_le(payload.data() + optional_offset + 56);
	info.optional.size_of_headers = read_uint32_le(payload.data() + optional_offset + 60);
	info.optional.resource_rva = 0;
	info.optional.resource_size = 0;
	if (read_uint32_le(payload.data() + optional_offset + 108) > 2)
	{
		info.optional.resource_rva = read_uint32_le(payload.data() + optional_offset + 128);
		info.optional.resource_size = read_uint32_le(payload.data() + optional_offset + 132);
	}
	return (true);
}

static bool	fill_sections(std::string &payload, PEInfo &info)
{
	SectionInfo	section;
	char	*data;
	size_t	table_offset;
	int	name_index;
	int	index;

	info.sections.clear();
	table_offset = static_cast<size_t>(info.dos.e_lfanew) + 24 + info.coff.size_of_optional_header;
	if (table_offset > payload.size() || info.coff.number_of_sections == 0 || info.coff.number_of_sections > (payload.size() - table_offset) / 40)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (section table out of bounds)\n";
		return (false);
	}
	index = 0;
	while (index < info.coff.number_of_sections)
	{
		data = payload.data() + table_offset + index * 40;
		name_index = 0;
		while (name_index < 8)
		{
			section.name[name_index] = data[name_index];
			name_index++;
		}
		section.name[8] = '\0';
		section.virtual_size = read_uint32_le(data + 8);
		section.virtual_address = read_uint32_le(data + 12);
		section.raw_size = read_uint32_le(data + 16);
		section.raw_offset = read_uint32_le(data + 20);
		section.characteristics = read_uint32_le(data + 36);
		if (section.raw_size != 0 && (section.raw_offset > payload.size() || section.raw_size > payload.size() - section.raw_offset))
		{
			std::cout << "obsidian: not a PE32+ x86-64 file (section data out of bounds)\n";
			return (false);
		}
		info.sections.push_back(section);
		index++;
	}
	return (true);
}

bool	fill_pe_info(std::string &payload, PEInfo &info)
{
	if (payload.size() < 64)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (too small for a PE header)\n";
		return (false);
	}
	info.dos.signature[0] = payload[0];
	info.dos.signature[1] = payload[1];

	if (info.dos.signature[0] != 'M' || info.dos.signature[1] != 'Z')
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (MZ signature missing)\n";
		return (false);
	}
	info.dos.e_lfanew = read_uint32_le(payload.data() + 0x3C);

	if (info.dos.e_lfanew < 0x40 || info.dos.e_lfanew > payload.size() - 26)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (e_lfanew out of bounds: 0x" << std::hex << info.dos.e_lfanew << ")\n";
		return (false);
	}
	info.pe_signature[0] = payload[info.dos.e_lfanew];
	info.pe_signature[1] = payload[info.dos.e_lfanew + 1];
	info.pe_signature[2] = payload[info.dos.e_lfanew + 2];
	info.pe_signature[3] = payload[info.dos.e_lfanew + 3];

	if (info.pe_signature[0] != 'P' || info.pe_signature[1] != 'E' || info.pe_signature[2] != '\0' || info.pe_signature[3] != '\0')
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (PE signature not found)\n";
		return (false);
	}
	info.coff.machine = read_uint16_le(payload.data() + info.dos.e_lfanew + 4);
	info.coff.number_of_sections = read_uint16_le(payload.data() + info.dos.e_lfanew + 6);
	info.coff.size_of_optional_header = read_uint16_le(payload.data() + info.dos.e_lfanew + 20);
	info.magic = read_uint16_le(payload.data() + info.dos.e_lfanew + 24);

	if (info.coff.machine != 0x8664)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (machine 0x" << std::hex << info.coff.machine << " is not 0x8664)\n";
		return (false);
	}

	if (info.magic != 0x20B)
	{
		std::cout << "obsidian: not a PE32+ x86-64 file (magic 0x" << std::hex << info.magic << " is not 0x20B)\n";
		return (false);
	}
	if (!fill_optional_info(payload, info))
		return (false);
	if (!fill_sections(payload, info))
		return (false);
	return (true);
}

// Return a file offset and the remaining bytes in the same section.
bool rva_to_offset(std::string &payload, PEInfo &info, uint32_t rva, size_t &offset, size_t &available)
{
    if (rva < info.optional.size_of_headers)
    {
        if (rva >= payload.size())
            return (false);
        offset = rva;
        available = std::min(static_cast<size_t>(info.optional.size_of_headers - rva), payload.size() - offset);
        return (true);
    }

    size_t i = 0;

    while (i < info.sections.size())
    {
        SectionInfo &section = info.sections[i];

        if (rva >= section.virtual_address)
        {
            uint64_t delta = static_cast<uint64_t>(rva) - section.virtual_address;

            if (delta < section.raw_size)
            {
                uint64_t position = static_cast<uint64_t>(section.raw_offset) + delta;

                if (position < payload.size())
                {
                    offset = static_cast<size_t>(position);
                    available = std::min(static_cast<size_t>(section.raw_size - delta), payload.size() - offset);
                    return (true);
                }
            }
        }
        i++;
    }

    return (false);
}

bool read_import_dlls(std::string &payload, PEInfo &info, std::vector<std::string> &dlls, std::vector<ImportSymbol> &symbols)
{
    size_t optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;

    dlls.clear();
    symbols.clear();

    if (info.coff.size_of_optional_header < 112)
        return (false);

    if (optional_offset > payload.size()
        || info.coff.size_of_optional_header > payload.size() - optional_offset)
        return (false);

    uint32_t directory_count = read_uint32_le(payload.data() + optional_offset + 108);

	if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
    	return (false);

    if (directory_count <= 1)
        return (true);

    uint32_t import_rva = read_uint32_le(payload.data() + optional_offset + 120);
    uint32_t import_size = read_uint32_le(payload.data() + optional_offset + 124);

    if (import_rva == 0 && import_size == 0)
        return (true);

    if (import_rva == 0 || import_size == 0)
        return (false);

    size_t position;
    size_t available;

    if (!rva_to_offset(payload, info, import_rva, position, available))
        return (false);

    if (import_size > available)
        return (false);

    size_t end = position + import_size;

    while (end - position >= 20)
    {
        char *descriptor = payload.data() + position;

        uint32_t lookup = read_uint32_le(descriptor);
        uint32_t timestamp = read_uint32_le(descriptor + 4);
        uint32_t forwarder = read_uint32_le(descriptor + 8);
        uint32_t name_rva = read_uint32_le(descriptor + 12);
        uint32_t iat = read_uint32_le(descriptor + 16);

        if (lookup == 0 && timestamp == 0 && forwarder == 0
            && name_rva == 0 && iat == 0)
            return (true);

        size_t name_offset;
        size_t name_available;

        if (!rva_to_offset(payload, info, name_rva, name_offset, name_available))
            return (false);

        size_t name_end = payload.find('\0', name_offset);

        if (name_end == std::string::npos
            || name_end - name_offset >= name_available)
            return (false);

        std::string dll_name = payload.substr(name_offset, name_end - name_offset);

        dlls.push_back(dll_name);

        uint32_t thunk_rva = lookup;

        if (thunk_rva == 0)
            thunk_rva = iat;

        size_t index = 0;

        while (thunk_rva != 0)
        {
            uint64_t current_rva = static_cast<uint64_t>(thunk_rva) + static_cast<uint64_t>(index) * 8;

            if (current_rva > UINT32_MAX)
                return (false);

            size_t thunk_offset;
            size_t thunk_available;

            if (!rva_to_offset(payload, info, static_cast<uint32_t>(current_rva), thunk_offset, thunk_available))
                return (false);

            if (thunk_available < 8)
                return (false);

            uint64_t entry = read_uint64_le(payload.data() + thunk_offset);

            if (entry == 0)
                break;

            ImportSymbol symbol;

            symbol.dll = dll_name;
            symbol.function = "";
            symbol.ordinal = 0;
            symbol.by_ordinal = false;

            if ((entry & (1ULL << 63)) != 0)
            {
                if ((entry & 0x7FFFFFFFFFFF0000ULL) != 0)
                    return (false);

                symbol.ordinal = static_cast<uint16_t>(entry & 0xFFFF);
                symbol.by_ordinal = true;
            }
            else
            {
                if (entry > 0x7FFFFFFFULL)
                    return (false);

                size_t function_offset;
                size_t function_available;

                if (!rva_to_offset(payload, info, static_cast<uint32_t>(entry), function_offset, function_available))
                    return (false);

                if (function_available < 3)
                    return (false);

                size_t function_end = payload.find('\0', function_offset + 2);

                if (function_end == std::string::npos
                    || function_end - function_offset >= function_available)
                    return (false);

                symbol.function = payload.substr(function_offset + 2, function_end - function_offset - 2);
            }

            symbols.push_back(symbol);
            index++;
        }

        position += 20;
    }

    return (false);
}

bool read_relocation_info(std::string &payload, PEInfo &info, size_t &blocks, size_t &entries, size_t &dir64)
{
    size_t optional_offset;
    uint32_t directory_count;
    uint32_t reloc_rva;
    uint32_t reloc_size;
    size_t offset;
    size_t available;
    size_t position;
    size_t end;

    blocks = 0;
    entries = 0;
    dir64 = 0;

    optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;

    if (info.coff.size_of_optional_header < 112)
        return (false);

    if (optional_offset > payload.size()
        || info.coff.size_of_optional_header > payload.size() - optional_offset)
        return (false);

    directory_count = read_uint32_le(payload.data() + optional_offset + 108);

    if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
        return (false);

    if (directory_count <= 5)
        return (true);

    reloc_rva = read_uint32_le(payload.data() + optional_offset + 152);
    reloc_size = read_uint32_le(payload.data() + optional_offset + 156);

    if (reloc_rva == 0 && reloc_size == 0)
        return (true);

    if (reloc_rva == 0 || reloc_size == 0)
        return (false);

    if (!rva_to_offset(payload, info, reloc_rva, offset, available))
        return (false);

    if (reloc_size > available)
        return (false);

    position = offset;
    end = offset + reloc_size;

    while (position < end)
    {
        if (end - position < 8)
            return (false);

        uint32_t page_rva = read_uint32_le(payload.data() + position);
        uint32_t block_size = read_uint32_le(payload.data() + position + 4);

        if (block_size < 8 || block_size % 4 != 0)
            return (false);

        if (block_size > end - position)
            return (false);

        size_t entry_position = position + 8;
        size_t block_end = position + block_size;

        while (entry_position < block_end)
        {
            uint16_t entry = read_uint16_le(payload.data() + entry_position);
            uint16_t type = entry >> 12;
            uint16_t displacement = entry & 0x0FFF;

            entries++;

            if (type == 10)
            {
                uint64_t target = static_cast<uint64_t>(page_rva) + displacement;

                if (target + 8 > info.optional.size_of_image)
                    return (false);

                dir64++;
            }
            else if (type != 0)
            {
                return (false);
            }

            entry_position += 2;
        }

        blocks++;
        position = block_end;
    }

    return (true);
}

bool build_pe_image(std::string &payload, PEInfo &info, std::vector<char> &image)
{
    size_t image_size;
    size_t headers_size;
    size_t i;

    image_size = info.optional.size_of_image;
    headers_size = info.optional.size_of_headers;

    if (image_size == 0 || image_size > 512ULL * 1024 * 1024)
        return (false);

    if (headers_size == 0 || headers_size > payload.size())
        return (false);

    if (headers_size > image_size)
        return (false);

    image.assign(image_size, 0);

    std::copy_n(payload.data(), headers_size, image.data());

    i = 0;
    while (i < info.sections.size())
    {
        SectionInfo &section = info.sections[i];

        size_t source = section.raw_offset;
        size_t destination = section.virtual_address;
        size_t copy_size = section.raw_size;
        size_t virtual_size = section.virtual_size;

        if (virtual_size == 0)
            virtual_size = copy_size;

        if (destination > image.size())
            return (false);

        if (virtual_size > image.size() - destination)
            return (false);

        if (copy_size > virtual_size)
            copy_size = virtual_size;

        if (source > payload.size())
            return (false);

        if (copy_size > payload.size() - source)
            return (false);

        if (copy_size > image.size() - destination)
            return (false);

        if (copy_size != 0)
            std::copy_n(payload.data() + source, copy_size, image.data() + destination);

        i++;
    }

    return (true);
}

/*
** Appends a section to the stub image. An empty data buffer produces an
** uninitialized section, which occupies no space in the file.
*/
bool add_section(std::string &stub, std::string name, uint32_t virtual_size, std::string data, uint32_t characteristics, uint32_t &section_rva, size_t &raw_offset)
{
    PEInfo info;

    section_rva = 0;
    raw_offset = 0;
    if (virtual_size == 0 || virtual_size > 512ULL * 1024 * 1024)
        return (false);
    if (name.empty() || name.size() > 8)
        return (false);
    if (!fill_pe_info(stub, info))
        return (false);
    if (info.coff.number_of_sections == 0xFFFF)
        return (false);

    size_t optional_offset = static_cast<size_t>(info.dos.e_lfanew) + 24;
    size_t table_offset = optional_offset + info.coff.size_of_optional_header;
    size_t new_header_offset = table_offset + static_cast<size_t>(info.coff.number_of_sections) * 40;

    if (info.coff.size_of_optional_header < 112)
        return (false);
    if (info.optional.size_of_headers > stub.size())
        return (false);
    if (new_header_offset > info.optional.size_of_headers
        || 40 > info.optional.size_of_headers - new_header_offset)
        return (false);
    if (new_header_offset > stub.size()
        || 40 > stub.size() - new_header_offset)
        return (false);

    uint32_t file_alignment = read_uint32_le(stub.data() + optional_offset + 36);
    uint32_t section_alignment = info.optional.section_alignment;

    if (file_alignment < 512 || file_alignment > 65536
        || (file_alignment & (file_alignment - 1)) != 0)
        return (false);
    if (section_alignment < 4096 || section_alignment < file_alignment
        || (section_alignment & (section_alignment - 1)) != 0)
        return (false);

    uint32_t directory_count = read_uint32_le(stub.data() + optional_offset + 108);

    if (directory_count > (static_cast<uint32_t>(info.coff.size_of_optional_header) - 112U) / 8U)
        return (false);

    // Header changes invalidate an Authenticode signature.
    if (directory_count > 4)
    {
        if (read_uint32_le(stub.data() + optional_offset + 144) != 0
            || read_uint32_le(stub.data() + optional_offset + 148) != 0)
            return (false);
    }

    size_t first_raw_offset = stub.size();
    uint64_t last_end = info.optional.size_of_image;
    size_t i = 0;

    while (i < info.sections.size())
    {
        SectionInfo &section = info.sections[i];

        if (section.raw_size != 0 && section.raw_offset < first_raw_offset)
            first_raw_offset = section.raw_offset;
        if (section.virtual_address % section_alignment != 0)
            return (false);

        uint64_t span = std::max(
            static_cast<uint64_t>(section.virtual_size),
            static_cast<uint64_t>(section.raw_size)
        );
        uint64_t section_end = static_cast<uint64_t>(section.virtual_address) + span;

        if (section_end > last_end)
            last_end = section_end;
        i++;
    }

    if (new_header_offset + 40 > first_raw_offset)
        return (false);

    i = 0;
    while (i < 40)
    {
        if (stub[new_header_offset + i] != '\0')
            return (false);
        i++;
    }

    uint64_t start = ((last_end + section_alignment - 1) / section_alignment) * section_alignment;
    uint64_t end = start + virtual_size;
    uint64_t new_image_size = ((end + section_alignment - 1) / section_alignment) * section_alignment;

    if (start > UINT32_MAX || new_image_size > UINT32_MAX
        || new_image_size > 1024ULL * 1024 * 1024)
        return (false);

    uint32_t uninitialized_size = read_uint32_le(stub.data() + optional_offset + 12);
    uint32_t raw_size = 0;

    if (data.empty())
    {
        if (static_cast<uint64_t>(uninitialized_size) + virtual_size > UINT32_MAX)
            return (false);
    }
    else
    {
        uint64_t padded = ((static_cast<uint64_t>(data.size()) + file_alignment - 1)
            / file_alignment) * file_alignment;

        if (padded > UINT32_MAX)
            return (false);
        raw_size = static_cast<uint32_t>(padded);
    }

    if (!data.empty())
    {
        uint64_t aligned = ((static_cast<uint64_t>(stub.size()) + file_alignment - 1)
            / file_alignment) * file_alignment;

        if (aligned > stub.size())
            stub.append(static_cast<size_t>(aligned - stub.size()), '\0');
        raw_offset = stub.size();
    }

    std::string section_header(40, '\0');

    std::copy_n(name.data(), name.size(), section_header.data());
    write_uint32_le(section_header.data() + 8, virtual_size);
    write_uint32_le(section_header.data() + 12, static_cast<uint32_t>(start));
    if (!data.empty())
    {
        write_uint32_le(section_header.data() + 16, raw_size);
        write_uint32_le(section_header.data() + 20, static_cast<uint32_t>(raw_offset));
    }
    write_uint32_le(section_header.data() + 36, characteristics);

    std::copy(section_header.begin(), section_header.end(), stub.begin() + new_header_offset);

    if (!data.empty())
    {
        stub.append(data);
        stub.append(static_cast<size_t>(raw_size) - data.size(), '\0');
    }

    write_uint16_le(stub.data() + info.dos.e_lfanew + 6, info.coff.number_of_sections + 1);
    if (data.empty())
        write_uint32_le(stub.data() + optional_offset + 12, uninitialized_size + virtual_size);
    write_uint32_le(stub.data() + optional_offset + 56, static_cast<uint32_t>(new_image_size));
    write_uint32_le(stub.data() + optional_offset + 64, 0);

    section_rva = static_cast<uint32_t>(start);
    return (true);
}

bool add_restore_section(std::string &stub, uint32_t original_image_size, uint32_t &restore_rva)
{
    size_t raw_offset;

    return (add_section(stub, ".obsram", original_image_size, std::string(),
        0xC0000080U, restore_rva, raw_offset));
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resource.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 04:15:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 04:15:36 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "packer/resource.hpp"
#include "obsidian.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#ifdef OBSIDIAN_DEBUG
#include <iostream>
#endif

namespace
{
enum : uint32_t
{
	DIRECTORY_HEADER_SIZE = 16,
	DIRECTORY_ENTRY_SIZE = 8,
	DATA_ENTRY_SIZE = 16,
	SUBDIRECTORY_FLAG = 0x80000000U,
	MAX_DEPTH = 4,
	RESOURCE_CHARACTERISTICS = 0x40000040U
};

uint16_t	read_u16(std::string &buffer, size_t offset)
{
	return (static_cast<uint16_t>(static_cast<unsigned char>(buffer[offset]))
		| (static_cast<uint16_t>(static_cast<unsigned char>(buffer[offset + 1])) << 8));
}

uint32_t	read_u32(std::string &buffer, size_t offset)
{
	uint32_t	value;
	int		index;

	value = 0;
	index = 0;
	while (index < 4)
	{
		value |= static_cast<uint32_t>(static_cast<unsigned char>(buffer[offset + index]))
			<< (index * 8);
		index++;
	}
	return (value);
}

void	write_u32(std::string &buffer, size_t offset, uint32_t value)
{
	int	index;

	index = 0;
	while (index < 4)
	{
		buffer[offset + index] = static_cast<char>((value >> (index * 8)) & 0xFF);
		index++;
	}
}

bool	shift_tree(std::string &buffer, size_t base, size_t limit, size_t node,
	uint32_t old_rva, uint32_t delta, int depth)
{
	uint16_t	named;
	uint16_t	ids;
	size_t		count;
	size_t		index;

	if (depth > static_cast<int>(MAX_DEPTH))
		return (false);
	if (node > limit || limit - node < DIRECTORY_HEADER_SIZE)
		return (false);
	named = read_u16(buffer, base + node + 12);
	ids = read_u16(buffer, base + node + 14);
	count = static_cast<size_t>(named) + static_cast<size_t>(ids);
	index = 0;
	while (index < count)
	{
		size_t		entry;
		uint32_t	offset;

		entry = node + DIRECTORY_HEADER_SIZE + index * DIRECTORY_ENTRY_SIZE;
		if (entry > limit || limit - entry < DIRECTORY_ENTRY_SIZE)
			return (false);
		offset = read_u32(buffer, base + entry + 4);
		if ((offset & SUBDIRECTORY_FLAG) != 0)
		{
			if (!shift_tree(buffer, base, limit, offset & ~SUBDIRECTORY_FLAG,
				old_rva, delta, depth + 1))
				return (false);
		}
		else
		{
			uint32_t	rva;
			uint32_t	size;

			if (offset > limit || limit - offset < DATA_ENTRY_SIZE)
				return (false);
			rva = read_u32(buffer, base + offset);
			size = read_u32(buffer, base + offset + 4);
			if (rva < old_rva)
				return (false);
			if (static_cast<uint64_t>(rva - old_rva) + size > limit)
				return (false);
			write_u32(buffer, base + offset, rva + delta);
		}
		index++;
	}
	return (true);
}
}

namespace
{
uint64_t	align_up(uint64_t value, uint64_t alignment)
{
	if (alignment == 0)
		return (value);
	return ((value + alignment - 1) / alignment) * alignment;
}

/*
** Returns the section whose virtual range contains rva. Resource data always
** lives inside a single section, so this lets us copy the whole section rather
** than trusting the data directory size, which does not always cover every
** resource (large icon groups, for instance).
*/
bool	section_of(PEInfo &info, uint32_t rva, size_t &section_index)
{
	size_t	index;

	index = 0;
	while (index < info.sections.size())
	{
		uint64_t	start;
		uint64_t	end;

		start = info.sections[index].virtual_address;
		end = start + std::max(static_cast<uint64_t>(info.sections[index].virtual_size),
			static_cast<uint64_t>(info.sections[index].raw_size));
		if (rva >= start && rva < end)
		{
			section_index = index;
			return (true);
		}
		index++;
	}
	return (false);
}

bool	find_resource_section(PEInfo &stub_info, SectionInfo &section, size_t &section_index)
{
	size_t	index;

	index = 0;
	while (index < stub_info.sections.size())
	{
		if (std::string(stub_info.sections[index].name) == ".rsrc")
		{
			section = stub_info.sections[index];
			section_index = index;
			return (true);
		}
		index++;
	}
	return (false);
}
}

bool	packer::copy_resources(std::string &stub, std::string &payload, PEInfo &info)
{
	std::string	blob;
	std::string	probe;
	PEInfo		stub_info;
	SectionInfo	section;
	size_t		section_index;
	size_t		payload_section;
	size_t		offset;
	size_t		available;
	size_t		file_end;
	size_t		optional_offset;
	size_t		table_offset;
	size_t		header_offset;
	size_t		raw_offset;
	size_t		index;
	size_t		dir_index;
	size_t		dir_entry;
	uint32_t	directory_count;
	uint32_t	dir_rva;
	uint32_t	dir_size;
	uint32_t	file_alignment;
	uint32_t	section_alignment;
	uint64_t	raw_start;
	uint64_t	raw_size;
	uint64_t	cursor;
	uint64_t	new_image_size;
	uint64_t	old_initialized;
	uint64_t	new_initialized;

	if (info.optional.resource_rva == 0 || info.optional.resource_size == 0)
	{
		OBSIDIAN_LOG("obsidian: payload has no resources, keeping the stub's own\n");
		return (true);
	}
	if (!section_of(info, info.optional.resource_rva, payload_section))
	{
		OBSIDIAN_LOG("obsidian: resource directory is not covered by a section\n");
		return (true);
	}
	if (!rva_to_offset(payload, info, info.optional.resource_rva, offset, available))
	{
		OBSIDIAN_LOG("obsidian: resource directory is not covered by a section\n");
		return (true);
	}
	/*
	** Copy up to the end of the containing section instead of trusting the data
	** directory size: the directory size does not always cover every resource,
	** while the section itself does.
	*/
	file_end = static_cast<size_t>(info.sections[payload_section].raw_offset)
		+ info.sections[payload_section].raw_size;
	if (file_end > payload.size() || file_end < offset)
		file_end = payload.size();
	blob = payload.substr(offset, file_end - offset);
	dir_size = info.optional.resource_size;
	if (info.sections[payload_section].virtual_size
		> info.optional.resource_rva - info.sections[payload_section].virtual_address)
		dir_size = info.sections[payload_section].virtual_size
			- (info.optional.resource_rva - info.sections[payload_section].virtual_address);
	probe = blob;
	if (!shift_tree(probe, 0, probe.size(), 0, info.optional.resource_rva, 0, 0))
	{
		OBSIDIAN_LOG("obsidian: resource directory is malformed\n");
		return (true);
	}
	if (!fill_pe_info(stub, stub_info)
		|| !find_resource_section(stub_info, section, section_index))
	{
		OBSIDIAN_LOG("obsidian: stub has no resource section to reuse\n");
		return (true);
	}
	optional_offset = static_cast<size_t>(read_u32(stub, 0x3C)) + 24;
	table_offset = optional_offset + stub_info.coff.size_of_optional_header;
	header_offset = table_offset + section_index * 40;
	file_alignment = read_u32(stub, optional_offset + 36);
	if (file_alignment == 0)
		file_alignment = 512;
	section_alignment = stub_info.optional.section_alignment;
	if (section_alignment == 0)
		section_alignment = file_alignment;

	if (blob.size() <= section.virtual_size
		&& blob.size() <= section.raw_size
		&& section.raw_offset <= stub.size()
		&& section.raw_size <= stub.size() - section.raw_offset)
	{
		std::fill(stub.begin() + section.raw_offset,
			stub.begin() + section.raw_offset + section.raw_size, '\0');
		std::copy(blob.begin(), blob.end(), stub.begin() + section.raw_offset);
		if (!shift_tree(stub, section.raw_offset, blob.size(), 0,
			info.optional.resource_rva,
			section.virtual_address - info.optional.resource_rva, 0))
		{
			OBSIDIAN_LOG("obsidian: could not relocate the resource directory\n");
			return (false);
		}
		write_u32(stub, optional_offset + 128, section.virtual_address);
		write_u32(stub, optional_offset + 132, dir_size);
		OBSIDIAN_LOG("obsidian: payload resources replaced the stub's own ("
			<< blob.size() << " bytes)\n");
		return (true);
	}
	raw_start = align_up(stub.size(), file_alignment);
	raw_size = align_up(blob.size(), file_alignment);
	if (raw_size > UINT32_MAX)
	{
		OBSIDIAN_LOG("obsidian: resource section would not fit in the image\n");
		return (false);
	}
	if (raw_start > stub.size())
		stub.append(static_cast<size_t>(raw_start - stub.size()), '\0');
	raw_offset = stub.size();
	stub.append(blob);
	stub.append(static_cast<size_t>(raw_size - blob.size()), '\0');
	if (section.raw_offset <= stub.size()
		&& section.raw_size <= stub.size() - section.raw_offset)
		std::fill(stub.begin() + section.raw_offset,
			stub.begin() + section.raw_offset + section.raw_size, '\0');
	if (!shift_tree(stub, raw_offset, blob.size(), 0, info.optional.resource_rva,
		section.virtual_address - info.optional.resource_rva, 0))
	{
		OBSIDIAN_LOG("obsidian: could not relocate the resource directory\n");
		return (false);
	}
	write_u32(stub, header_offset + 8, static_cast<uint32_t>(blob.size()));
	write_u32(stub, header_offset + 16, static_cast<uint32_t>(raw_size));
	write_u32(stub, header_offset + 20, static_cast<uint32_t>(raw_offset));
	cursor = align_up(static_cast<uint64_t>(section.virtual_address) + blob.size(),
		section_alignment);
	index = section_index + 1;
	while (index < stub_info.sections.size())
	{
		SectionInfo	&current = stub_info.sections[index];
		uint64_t	span;
		uint64_t	address;

		span = std::max(static_cast<uint64_t>(current.virtual_size),
			static_cast<uint64_t>(current.raw_size));
		address = align_up(cursor, section_alignment);
		if (address > UINT32_MAX)
		{
			OBSIDIAN_LOG("obsidian: resource section would not fit in the image\n");
			return (false);
		}
		write_u32(stub, table_offset + index * 40 + 12, static_cast<uint32_t>(address));
		/*
		** A moved section may be the target of a data directory (the base
		** relocation directory, for instance). Repoint every directory whose
		** RVA falls inside the section's old range, otherwise the loader would
		** read it at a stale address.
		*/
		directory_count = read_u32(stub, optional_offset + 108);
		if (directory_count > 16)
			directory_count = 16;
		dir_index = 0;
		while (dir_index < directory_count)
		{
			dir_entry = optional_offset + 112 + dir_index * 8;
			dir_rva = read_u32(stub, dir_entry);
			if (dir_index != 4 && dir_rva >= current.virtual_address
				&& static_cast<uint64_t>(dir_rva)
					< static_cast<uint64_t>(current.virtual_address) + span)
				write_u32(stub, dir_entry, static_cast<uint32_t>(
					address + (dir_rva - current.virtual_address)));
			dir_index++;
		}
		cursor = address + span;
		index++;
	}
	new_image_size = align_up(cursor, section_alignment);
	if (new_image_size > UINT32_MAX)
	{
		OBSIDIAN_LOG("obsidian: resource section would not fit in the image\n");
		return (false);
	}
	old_initialized = read_u32(stub, optional_offset + 8);
	if (raw_size >= section.raw_size)
		new_initialized = old_initialized + (raw_size - section.raw_size);
	else if (old_initialized >= section.raw_size - raw_size)
		new_initialized = old_initialized - (section.raw_size - raw_size);
	else
		new_initialized = 0;
	if (new_initialized > UINT32_MAX)
		new_initialized = UINT32_MAX;
	write_u32(stub, optional_offset + 8, static_cast<uint32_t>(new_initialized));
	write_u32(stub, optional_offset + 56, static_cast<uint32_t>(new_image_size));
	write_u32(stub, optional_offset + 128, section.virtual_address);
	write_u32(stub, optional_offset + 132, dir_size);
	OBSIDIAN_LOG("obsidian: payload resources grew the .rsrc section ("
		<< blob.size() << " bytes)\n");
	return (true);
}

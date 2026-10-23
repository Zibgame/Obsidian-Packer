/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   pe.hpp                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 17:42:05 by marvin           #+#    #+#             */
/*   Updated: 2026/10/23 17:42:05 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PE_HPP
#define PE_HPP

#include <cstdint>
#include <string>
#include <vector>

struct DOSHeader
{
	char		signature[2];
	uint32_t	e_lfanew;
};

struct COFFHeader
{
	uint16_t	machine;
	uint16_t	number_of_sections;
	uint16_t	size_of_optional_header;
};

struct SectionInfo
{
	char		name[9];
	uint32_t	virtual_size;
	uint32_t	virtual_address;
	uint32_t	raw_size;
	uint32_t	raw_offset;
	uint32_t	characteristics;
};

struct OptionalInfo
{
	uint64_t	image_base;
	uint32_t	size_of_image;
	uint32_t	size_of_headers;
	uint32_t	entry_point_rva;
	uint32_t	section_alignment;

	uint32_t import_rva;
	uint32_t import_size;
	uint32_t reloc_rva;
	uint32_t reloc_size;
};

struct PEInfo
{
	DOSHeader	dos;
	char		pe_signature[4];
	COFFHeader	coff;
	uint16_t	magic;
	OptionalInfo	optional;
	std::vector<SectionInfo>	sections;
};

struct ImportSymbol
{
    std::string dll;
    std::string function;
    uint16_t ordinal;
    bool by_ordinal;
};

bool	fill_pe_info(std::string &payload, PEInfo &info);
bool build_pe_image(std::string &payload, PEInfo &info, std::vector<char> &image);
bool read_import_dlls(std::string &payload,PEInfo &info,std::vector<std::string> &dlls,std::vector<ImportSymbol> &symbols);
bool read_relocation_info(std::string &payload, PEInfo &info, size_t &blocks, size_t &entries, size_t &dir64);
bool restore_original_pe(std::string &payload, PEInfo &info);
bool add_restore_section(std::string &stub, uint32_t original_image_size, uint32_t &restore_rva);
bool rva_to_offset(std::string &payload, PEInfo &info, uint32_t rva, size_t &offset, size_t &available);

#endif
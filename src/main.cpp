/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include "cli/options.hpp"
#include "compression/compression.hpp"
#include "crypto/crypto.hpp"
#include "file/file.hpp"
#include "loader/restore.hpp"
#include "packer/random.hpp"
#include "packer/resource.hpp"
#include "pe/pe.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>

namespace
{
void	write_uint32_le(std::string &buffer, size_t offset, uint32_t value)
{
	size_t	index;

	index = 0;
	while (index < 4)
	{
		buffer[offset + index] = static_cast<char>((value >> (index * 8)) & 0xFF);
		index++;
	}
}

void	write_uint64_le(std::string &buffer, size_t offset, uint64_t value)
{
	size_t	index;

	index = 0;
	while (index < 8)
	{
		buffer[offset + index] = static_cast<char>((value >> (index * 8)) & 0xFF);
		index++;
	}
}

void	write_uint8(std::string &buffer, size_t offset, uint8_t value)
{
	buffer[offset] = static_cast<char>(value);
}

void	generate_section_name(std::string &name)
{
	unsigned char	random_bytes[7];
	size_t		index;

	name = ".";
	if (!packer::random_bytes(random_bytes, 7))
	{
		name = ".data0";
		return;
	}
	index = 0;
	while (index < 7)
	{
		name += "abcdefghijklmnopqrstuvwxyz0123456789"[random_bytes[index] % 36];
		index++;
	}
}

std::string	build_footer(Options &options, std::string &stored, uint64_t original_size,
	unsigned char *nonce, uint32_t key_rva, uint32_t region_size, uint64_t total_size)
{
	std::string	footer;
	size_t		index;

	footer.resize(OBSD_FOOTER_SIZE);
	write_uint32_le(footer, OBSD_OFFSET_TAG, OBSD_TAG);
	write_uint8(footer, OBSD_OFFSET_VERSION, OBSD_VERSION);
	write_uint8(footer, OBSD_OFFSET_COMPRESSION, options.compression->id);
	write_uint8(footer, OBSD_OFFSET_ENCRYPTION, options.encryption->id);
	write_uint8(footer, OBSD_OFFSET_NONCE_SIZE, static_cast<uint8_t>(options.encryption->nonce_size));
	index = 0;
	while (index < options.encryption->nonce_size)
	{
		footer[OBSD_OFFSET_NONCE + index] = static_cast<char>(nonce[index]);
		index++;
	}
	write_uint64_le(footer, OBSD_OFFSET_STORED_SIZE, stored.size());
	write_uint64_le(footer, OBSD_OFFSET_ORIGINAL_SIZE, original_size);
	write_uint8(footer, OBSD_OFFSET_RESTORE, options.restore);
	write_uint32_le(footer, OBSD_OFFSET_KEY_RVA, key_rva);
	write_uint32_le(footer, OBSD_OFFSET_REGION_SIZE, region_size);
	obsidian_footer_cipher(reinterpret_cast<unsigned char *>(&footer[0]), OBSD_FOOTER_SIZE, total_size);
	return (footer);
}

bool	build_output(Options &options)
{
	std::string	compressed;
	std::string	output_path;
	std::string	payload;
	std::string	stub;
	std::string	key_data;
	std::string	section_name;
	unsigned char	key[OBSIDIAN_KEY_SIZE];
	unsigned char	nonce[OBSD_NONCE_MAX];
	PEInfo		info;
	uint32_t	restore_rva;
	uint64_t	total_size;

	output_path = "output.exe";
	if (!file::read(options.payload_path, payload))
		return (false);
	if (!fill_pe_info(payload, info))
		return (false);
	if (!options.compression->compress(payload, compressed))
	{
		OBSIDIAN_LOG("obsidian: compression with " << compression::name_of(options.compression->id) << " failed\n");
		return (false);
	}
	if (!packer::random_bytes(key, OBSIDIAN_KEY_SIZE))
	{
		OBSIDIAN_LOG("obsidian: key generation failed\n");
		return (false);
	}
	if (!packer::random_bytes(nonce, options.encryption->nonce_size))
	{
		OBSIDIAN_LOG("obsidian: nonce generation failed\n");
		return (false);
	}
	if (!options.encryption->encrypt(compressed, key, nonce))
	{
		OBSIDIAN_LOG("obsidian: encryption with " << crypto::name_of(options.encryption->id) << " failed\n");
		return (false);
	}
	if (!file::read(options.stub_path, stub))
		return (false);
	if (stub == payload)
	{
		OBSIDIAN_LOG("obsidian: refusing to pack the stub into itself\n");
		return (false);
	}
	options.encryption->mask(key, nonce);
	if (!packer::copy_resources(stub, payload, info))
		return (false);
	section_name = options.section;
	if (section_name.empty())
		generate_section_name(section_name);
	key_data.assign(reinterpret_cast<char *>(key), OBSIDIAN_KEY_SIZE);
	if (!add_payload_section(stub, section_name, info.optional.size_of_image, key_data, restore_rva))
		return (false);

	std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
	if (!output)
		return (false);
	output.write(stub.data(), static_cast<std::streamsize>(stub.size()));
	output.close();
	if (!output)
		return (false);
	total_size = stub.size() + compressed.size() + OBSD_FOOTER_SIZE;
	if (!file::append(output_path, compressed))
		return (false);
	if (!file::append(output_path, build_footer(options, compressed, payload.size(), nonce,
		restore_rva, info.optional.size_of_image, total_size)))
		return (false);

	std::cout << "Compression: " << compression::name_of(options.compression->id) << "\n";
	std::cout << "Encryption: " << crypto::name_of(options.encryption->id) << "\n";
	std::cout << "Restore mode: " << loader::name_of(options.restore) << "\n";
	std::cout << "Restore section: " << section_name << "\n";
	std::cout << "Original size: " << payload.size() << " bytes\n";
	std::cout << "Compressed size: " << compressed.size() << " bytes\n";
	return (true);
}

// Look for the bundled runtime stub.
bool	find_default_stub(std::string &stub_path)
{
	std::string	candidates[4];
	size_t		index;

	candidates[0] = "build/obsidian-stub.exe";
	candidates[1] = "obsidian-stub.exe";
	candidates[2] = "../build/obsidian-stub.exe";
	candidates[3] = "obsidian-stub";
	index = 0;
	while (index < 4)
	{
		if (file::exists(candidates[index]))
		{
			stub_path = candidates[index];
			return (true);
		}
		index++;
	}
	return (false);
}
}

int	main(int argc, char **argv)
{
	Options		options;
	std::string	error;

	if (!parse_options(argc, argv, options, error))
	{
		OBSIDIAN_LOG("obsidian: " << error << "\n");
		std::cout << usage_text();
		return (1);
	}
	if (options.help)
	{
		std::cout << usage_text();
		return (0);
	}
	if (options.auto_stub)
	{
		if (!find_default_stub(options.stub_path))
		{
			OBSIDIAN_LOG("obsidian: no stub found, expected obsidian-stub.exe"
				" next to the packer, in ./build or ../build\n");
			return (1);
		}
		OBSIDIAN_LOG("obsidian: using stub " << options.stub_path << "\n");
	}
	if (!build_output(options))
	{
		OBSIDIAN_LOG("obsidian: packing failed\n");
		return (1);
	}
	std::cout << "Packed binary created: .\\output.exe\n";
	return (0);
}

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
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

/*
** Footer: marker, format version, algorithm identifiers, nonce, size of the
** stored payload and size of the original payload.
*/
std::string	build_footer(Options &options, std::string &stored, uint64_t original_size, unsigned char *nonce)
{
	std::string	footer;
	size_t		index;

	footer.resize(OBSD_FOOTER_SIZE);
	index = 0;
	while (index < 4)
	{
		footer[index] = OBSD[index];
		index++;
	}
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
	return (footer);
}

/*
** The packer locates the key slot by its marker and overwrites the 32 bytes
** that follow it. The stub reserves the slot in its own data section.
*/
bool	inject_key(std::string &stub, unsigned char *key)
{
	size_t	offset;
	size_t	found;
	size_t	index;
	size_t	matches;

	matches = 0;
	found = 0;
	offset = 0;
	while (offset + OBSIDIAN_KEY_MARKER_SIZE <= stub.size())
	{
		if (stub.compare(offset, OBSIDIAN_KEY_MARKER_SIZE, OBSIDIAN_KEY_MARKER, OBSIDIAN_KEY_MARKER_SIZE) == 0)
		{
			found = offset;
			matches++;
		}
		offset++;
	}
	if (matches == 0)
	{
		std::cout << "obsidian: stub has no key slot, rebuild " << OBSIDIAN_NAME << "-stub\n";
		return (false);
	}
	if (matches > 1)
	{
		std::cout << "obsidian: stub carries " << matches << " key slots, expected one\n";
		return (false);
	}
	index = 0;
	while (index < OBSIDIAN_KEY_SIZE)
	{
		stub[found + OBSIDIAN_KEY_MARKER_SIZE + index] = static_cast<char>(key[index]);
		index++;
	}
	return (true);
}

bool	build_output(Options &options)
{
	std::string	compressed;
	std::string	output_path;
	std::string	payload;
	std::string	stub;
	unsigned char	key[OBSIDIAN_KEY_SIZE];
	unsigned char	nonce[OBSD_NONCE_MAX];
	PEInfo		info;
	uint32_t	restore_rva;

	output_path = "output.exe";
	if (!file::read(options.payload_path, payload))
		return (false);
	if (!fill_pe_info(payload, info))
		return (false);
	if (!options.compression->compress(payload, compressed))
	{
		std::cout << "obsidian: compression with " << options.compression->name << " failed\n";
		return (false);
	}
	if (!packer::random_bytes(key, OBSIDIAN_KEY_SIZE))
	{
		std::cout << "obsidian: key generation failed\n";
		return (false);
	}
	if (!packer::random_bytes(nonce, options.encryption->nonce_size))
	{
		std::cout << "obsidian: nonce generation failed\n";
		return (false);
	}
	if (!options.encryption->encrypt(compressed, key, nonce))
	{
		std::cout << "obsidian: encryption with " << options.encryption->name << " failed\n";
		return (false);
	}
	if (!file::read(options.stub_path, stub))
		return (false);
	// A stub cannot be packed into itself.
	if (stub == payload)
	{
		std::cout << "obsidian: refusing to pack the stub into itself\n";
		return (false);
	}
	options.encryption->mask(key, nonce);
	if (!inject_key(stub, key))
		return (false);
	if (!packer::copy_resources(stub, payload, info))
		return (false);
	if (!add_restore_section(stub, info.optional.size_of_image, restore_rva))
		return (false);

	std::ofstream output(output_path, std::ios::binary | std::ios::trunc);
	if (!output)
		return (false);
	output.write(stub.data(), static_cast<std::streamsize>(stub.size()));
	output.close();
	if (!output)
		return (false);
	if (!file::append(output_path, compressed))
		return (false);
	if (!file::append(output_path, build_footer(options, compressed, payload.size(), nonce)))
		return (false);

	std::cout << "Compression: " << options.compression->name << "\n";
	std::cout << "Encryption: " << options.encryption->name << "\n";
	std::cout << "Restore mode: " << loader::name_of(options.restore) << "\n";
	std::cout << "Original size: " << payload.size() << " bytes\n";
	std::cout << "Compressed size: " << compressed.size() << " bytes\n";
	return (true);
}

// Look for the bundled runtime stub.
bool	find_default_stub(std::string &stub_path)
{
	std::string	candidates[4];
	size_t		index;

	candidates[0] = "obsidian-stub.exe";
	candidates[1] = "build/obsidian-stub.exe";
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
		std::cout << "obsidian: " << error << "\n";
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
			std::cout << "obsidian: no stub found, expected obsidian-stub.exe"
				" next to the packer, in ./build or ../build\n";
			return (1);
		}
		std::cout << "obsidian: using stub " << options.stub_path << "\n";
	}
	if (!build_output(options))
	{
		std::cout << "obsidian: packing failed\n";
		return (1);
	}
	std::cout << "Packed binary created: .\\output.exe\n";
	return (0);
}

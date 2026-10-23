/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/24 00:01:17 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include "compression/compression.hpp"
#include "file/file.hpp"
#include "pe/pe.hpp"

#include <iostream>
#include <fstream>
#include <string>

bool	append_footer(std::string output_path, uint64_t compressed_size, uint64_t original_size)
{
	if (!file::append(output_path, OBSD))
		return (false);
	if (!file::append_uint64_le(output_path, compressed_size))
		return (false);
	if (!file::append_uint64_le(output_path, original_size))
		return (false);
	return (true);
}

bool build_output(std::string stub_path, std::string payload_path)
{
    std::string compressed;
    std::string output_path;
    std::string payload;
    std::string stub;
    PEInfo info;
    uint32_t restore_rva;

    output_path = "output.exe";

    if (!file::read(payload_path, payload))
        return (false);
    if (!fill_pe_info(payload, info))
        return (false);
    if (!compression::compress(payload, compressed))
        return (false);

    if (!file::read(stub_path, stub))
        return (false);
    // A stub cannot be packed into itself.
    if (stub == payload)
    {
        std::cout << "obsidian: refusing to pack the stub into itself\n";
        return (false);
    }
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
    if (!append_footer(output_path, compressed.size(), payload.size()))
        return (false);

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

int	main(int argc, char **argv)
{
	std::string	stub_path;

	if (argc == 2)
	{
		if (!find_default_stub(stub_path))
		{
			std::cout << "obsidian: no stub found, expected obsidian-stub.exe"
				" next to the packer, in ./build or ../build\n";
			return (1);
		}
		std::cout << "obsidian: using stub " << stub_path << "\n";
	}
	else if (argc == 3)
	{
		stub_path = argv[1];
	}
	else
	{
		std::cout << "Usage: " << OBSIDIAN_NAME
			<< " [stub.exe] <payload>\n";
		std::cout << "  without <stub.exe>, the stub is looked up in ./build"
			" and next to the packer\n";
		return (1);
	}
	if (!build_output(stub_path, argv[argc - 1]))
	{
		std::cout << "obsidian: packing failed\n";
		return (1);
	}
	std::cout << "Packed binary created: .\\output.exe\n";
	return (0);
}

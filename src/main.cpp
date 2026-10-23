/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/23 02:57:46 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include "compression/compression.hpp"
#include "file/file.hpp"

#include <iostream>
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

bool	build_output(std::string stub_path, std::string payload_path)
{
	std::string	compressed;
	std::string	output_path;
	std::string	payload;

	output_path = "output.exe";
	if (!file::read(payload_path, payload))
		return (false);
	if (!compression::compress(payload, compressed))
		return (false);
	if (!file::copy(stub_path, output_path))
		return (false);
	if (!file::append(output_path, compressed))
		return (false);
	if (!append_footer(output_path, compressed.size(), payload.size()))
		return (false);
	std::cout << "Original size: " << payload.size() << " bytes\n";
	std::cout << "Compressed size: " << compressed.size() << " bytes\n";
	return (true);
}

int	main(int argc, char **argv)
{
	if (argc != 3)
	{
		std::cout << "Usage: " << OBSIDIAN_NAME << " <stub.exe> <payload>\n";
		return (1);
	}
	if (!build_output(argv[1], argv[2]))
	{
		std::cout << "obsidian: packing failed\n";
		return (1);
	}
	std::cout << "Packed binary created: .\\output.exe\n";
	return (0);
}

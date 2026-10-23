/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/23 02:51:27 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

int	main(int argc, char **argv)
{
	char			buffer[4096];
	char			output_path[] = "obsidian-pack.exe";
	std::ifstream	stub_file;
	std::ofstream	packed_file;
	std::streamsize	read_size;

	if (argc != 2)
	{
		std::cout << "Usage: " << OBSIDIAN_NAME << " <stub.exe>\n";
		return (1);
	}
	stub_file.open(argv[1], std::ios::binary);
	if (!stub_file.is_open())
	{
		std::cout << "obsidian: cannot open stub: " << argv[1] << '\n';
		return (1);
	}

	packed_file.open(output_path, std::ios::binary);
	if (!packed_file.is_open())
	{
		stub_file.close();
		return (1);
	}
	while (stub_file.read(buffer, sizeof(buffer)) || stub_file.gcount() > 0)
	{
		read_size = stub_file.gcount();
		packed_file.write(buffer, read_size);
		if (!packed_file)
		{
			stub_file.close();
			packed_file.close();
			std::filesystem::remove(output_path);
			return (1);
		}
	}
	if (stub_file.bad())
	{
		stub_file.close();
		packed_file.close();
		std::filesystem::remove(output_path);
		return (1);
	}
	packed_file.write("hello", 5);
	if (!packed_file)
	{
		stub_file.close();
		packed_file.close();
		std::filesystem::remove(output_path);
		return (1);
	}
	stub_file.close();
	packed_file.close();
	if (packed_file.fail())
	{
		std::filesystem::remove(output_path);
		return (1);
	}
	std::cout << "Packed binary created: .\\" << output_path << '\n';
	return (0);
}

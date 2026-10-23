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
#include "file/file.hpp"

#include <iostream>
#include <string>

int	main(int argc, char **argv)
{
	char			output_path[] = "obsidian-pack.exe";
	std::string	payload = "hello";

	if (argc != 2)
	{
		std::cout << "Usage: " << OBSIDIAN_NAME << " <stub.exe>\n";
		return (1);
	}
	if (!file::copy(argv[1], output_path))
	{
		std::cout << "obsidian: cannot copy stub: " << argv[1] << '\n';
		return (1);
	}
	if (!file::append(output_path, payload))
	{
		std::cout << "obsidian: cannot append data\n";
		return (1);
	}
	if (!file::append(output_path, OBSD))
	{
		std::cout << "obsidian: cannot append data\n";
		return (1);
	}
	if (!file::append_uint64_le(output_path, payload.size()))
	{
		std::cout << "obsidian: cannot append payload size\n";
		return (1);
	}
	std::cout << "Packed binary created: .\\" << output_path << '\n';
	return (0);
}

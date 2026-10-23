/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   file.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 02:53:36 by marvin            #+#    #+#             */
/*   Updated: 2026/10/23 02:53:39 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "file/file.hpp"

#include <filesystem>
#include <fstream>

bool	file::copy(std::string source, std::string destination)
{
	char			buffer[4096];
	std::ifstream	input;
	std::ofstream	output;
	std::streamsize	read_size;

	if (source == destination)
		return (false);
	input.open(source, std::ios::binary);
	if (!input.is_open())
		return (false);
	output.open(destination, std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		input.close();
		return (false);
	}
	while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
	{
		read_size = input.gcount();
		output.write(buffer, read_size);
		if (!output)
		{
			input.close();
			output.close();
			std::filesystem::remove(destination);
			return (false);
		}
	}
	if (input.bad())
	{
		input.close();
		output.close();
		std::filesystem::remove(destination);
		return (false);
	}
	input.close();
	output.close();
	if (output.fail())
	{
		std::filesystem::remove(destination);
		return (false);
	}
	return (true);
}

bool	file::clear(std::string path)
{
	std::ofstream	output;

	output.open(path, std::ios::binary | std::ios::trunc);
	if (!output.is_open())
		return (false);
	output.close();
	if (output.fail())
		return (false);
	return (true);
}

bool	file::append(std::string path, std::string data)
{
	std::ofstream	output;

	output.open(path, std::ios::binary | std::ios::app);
	if (!output.is_open())
		return (false);
	output.write(data.data(), data.size());
	if (!output)
	{
		output.close();
		return (false);
	}
	output.close();
	if (output.fail())
		return (false);
	return (true);
}

bool	file::append_file(std::string path, std::string source)
{
	char			buffer[4096];
	std::ifstream	input;
	std::ofstream	output;
	std::streamsize	read_size;

	input.open(source, std::ios::binary);
	if (!input.is_open())
		return (false);
	output.open(path, std::ios::binary | std::ios::app);
	if (!output.is_open())
	{
		input.close();
		return (false);
	}
	while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
	{
		read_size = input.gcount();
		output.write(buffer, read_size);
		if (!output)
		{
			input.close();
			output.close();
			return (false);
		}
	}
	if (input.bad())
	{
		input.close();
		output.close();
		return (false);
	}
	input.close();
	output.close();
	if (output.fail())
		return (false);
	return (true);
}

bool	file::append_uint64_le(std::string path, uint64_t value)
{
	char	data[8];
	int	index;

	index = 0;
	while (index < 8)
	{
		data[index] = static_cast<char>(value & 0xff);
		value = value >> 8;
		index++;
	}
	return (file::append(path, std::string(data, 8)));
}

bool	file::write_at(std::string path, std::streamoff offset, std::string data)
{
	std::fstream	output;

	if (offset < 0)
		return (false);
	output.open(path, std::ios::binary | std::ios::in | std::ios::out);
	if (!output.is_open())
		return (false);
	output.seekp(offset);
	if (!output)
	{
		output.close();
		return (false);
	}
	output.write(data.data(), data.size());
	if (!output)
	{
		output.close();
		return (false);
	}
	output.close();
	if (output.fail())
		return (false);
	return (true);
}

std::streamoff	file::size(std::string path)
{
	std::ifstream	input;
	std::streamoff	file_size;

	input.open(path, std::ios::binary | std::ios::ate);
	if (!input.is_open())
		return (-1);
	file_size = input.tellg();
	input.close();
	return (file_size);
}

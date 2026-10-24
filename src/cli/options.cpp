/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   options.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:37:12 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "cli/options.hpp"
#include "obsidian.hpp"

#include <cstring>
#include <vector>

namespace
{
bool	starts_with(std::string text, std::string prefix)
{
	if (text.size() < prefix.size())
		return (false);
	if (text.compare(0, prefix.size(), prefix) != 0)
		return (false);
	return (true);
}

bool	split_assignment(std::string argument, std::string name, std::string &value)
{
	size_t	separator;

	if (!starts_with(argument, name + "="))
		return (false);
	separator = name.size() + 1;
	value = argument.substr(separator);
	return (true);
}

bool	take_value(int argc, char **argv, int &index, std::string argument, std::string flag, std::string &value, std::string &error)
{
	if (split_assignment(argument, flag, value))
	{
		if (value.empty())
		{
			error = flag + " requires a value";
			return (false);
		}
		return (true);
	}
	if (argument != flag)
		return (false);
	index++;
	if (index >= argc)
	{
		error = flag + " requires a value";
		return (false);
	}
	value = argv[index];
	return (true);
}
}

std::string	usage_text()
{
	std::string	usage;

	usage = "Usage: " + std::string(OBSIDIAN_NAME) + " [options] [stub.exe] <payload.exe>\n";
	usage += "\n";
	usage += "Options:\n";
	usage += "  --compression <name>   predefined compression algorithm (default: lzma)\n";
	usage += "  --encryption <name>    predefined encryption algorithm (default: chacha20)\n";
	usage += "  --restore <mode>       restore region, obsram or stomp (default: obsram)\n";
	usage += "  -h, --help             show this message\n";
	usage += "\n";
	usage += "Available compression algorithms: " + compression::names() + "\n";
	usage += "Available encryption algorithms: " + crypto::names() + "\n";
	usage += "Available restore modes: " + loader::names() + "\n";
	usage += "\n";
	usage += "Without <stub.exe>, the stub is looked up next to the packer and in ./build.\n";
	return (usage);
}

bool	parse_options(int argc, char **argv, Options &options, std::string &error)
{
	std::vector<std::string>	positionals;
	std::string			argument;
	std::string			value;
	std::string			flag;
	int				index;

	options.compression = compression::find("lzma");
	options.encryption = crypto::find("chacha20");
	options.restore = RESTORE_OBSRAM;
	options.help = false;
	options.auto_stub = true;
	index = 1;
	while (index < argc)
	{
		argument = argv[index];
		if (argument == "-h" || argument == "--help")
		{
			options.help = true;
			return (true);
		}
		if (argument == "--compression" || starts_with(argument, "--compression="))
		{
			flag = "--compression";
			if (!take_value(argc, argv, index, argument, flag, value, error))
			{
				if (error.empty())
					error = "unknown option: " + argument;
				return (false);
			}
			options.compression = compression::find(value);
			if (options.compression == NULL)
			{
				error = "unknown compression algorithm: " + value + " (available: " + compression::names() + ")";
				return (false);
			}
		}
		else if (argument == "--encryption" || starts_with(argument, "--encryption="))
		{
			flag = "--encryption";
			if (!take_value(argc, argv, index, argument, flag, value, error))
			{
				if (error.empty())
					error = "unknown option: " + argument;
				return (false);
			}
			options.encryption = crypto::find(value);
			if (options.encryption == NULL)
			{
				error = "unknown encryption algorithm: " + value + " (available: " + crypto::names() + ")";
				return (false);
			}
		}
		else if (argument == "--restore" || starts_with(argument, "--restore="))
		{
			flag = "--restore";
			if (!take_value(argc, argv, index, argument, flag, value, error))
			{
				if (error.empty())
					error = "unknown option: " + argument;
				return (false);
			}
			if (!loader::id_of(value, options.restore))
			{
				error = "unknown restore mode: " + value + " (available: " + loader::names() + ")";
				return (false);
			}
		}
		else if (!argument.empty() && argument[0] == '-')
		{
			error = "unknown option: " + argument;
			return (false);
		}
		else
		{
			positionals.push_back(argument);
		}
		index++;
	}
	if (positionals.empty())
	{
		error = "no payload given";
		return (false);
	}
	if (positionals.size() > 2)
	{
		error = "too many arguments";
		return (false);
	}
	options.payload_path = positionals.back();
	options.auto_stub = positionals.size() == 1;
	if (options.auto_stub)
		options.stub_path.clear();
	else
		options.stub_path = positionals.front();
	return (true);
}

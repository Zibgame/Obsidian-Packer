#include "obsidian/version.hpp"

#include <iostream>
#include <string_view>

namespace
{
void print_usage(std::ostream &output)
{
	output << "Usage: " << obsidian::program_name << " [options] <input.exe>\n"
		   << "\n"
		   << "Options:\n"
		   << "  -h, --help       Show this help message\n"
		   << "  -v, --version    Show the current version\n";
}
}

int main(int argc, char **argv)
{
	if (argc == 2)
	{
		const std::string_view argument(argv[1]);

		if (argument == "-h" || argument == "--help")
		{
			print_usage(std::cout);
			return 0;
		}
		if (argument == "-v" || argument == "--version")
		{
			std::cout << obsidian::program_name << ' ' << obsidian::version
					  << '\n';
			return 0;
		}
		std::cerr << "obsidian: packing is not implemented yet\n";
		return 1;
	}
	print_usage(argc == 1 ? std::cout : std::cerr);
	return argc == 1 ? 0 : 1;
}

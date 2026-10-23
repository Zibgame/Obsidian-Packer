#ifndef FILE_HPP
#define FILE_HPP

#include <ios>
#include <string>

namespace file
{
bool			copy(std::string source, std::string destination);
bool			clear(std::string path);
bool			append(std::string path, std::string data);
bool			write_at(std::string path, std::streamoff offset, std::string data);
std::streamoff	size(std::string path);
}

#endif

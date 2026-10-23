#ifndef FILE_HPP
#define FILE_HPP

#include <cstdint>
#include <ios>
#include <string>

namespace file
{
bool			copy(std::string source, std::string destination);
bool			read(std::string path, std::string &data);
bool			write(std::string path, std::string data);
bool			clear(std::string path);
bool			append(std::string path, std::string data);
bool			append_file(std::string path, std::string source);
bool			append_uint64_le(std::string path, uint64_t value);
bool			write_at(std::string path, std::streamoff offset, std::string data);
std::streamoff	size(std::string path);
}

#endif

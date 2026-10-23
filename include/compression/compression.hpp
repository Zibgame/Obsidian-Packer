#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <cstdint>
#include <string>

namespace compression
{
bool	compress(std::string input, std::string &output);
bool	decompress(std::string input, uint64_t original_size, std::string &output);
}

#endif

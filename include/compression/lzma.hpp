/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   lzma.hpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMPRESSION_LZMA_HPP
#define COMPRESSION_LZMA_HPP

#include "compression/compression.hpp"

#include <cstdint>
#include <string>

namespace compression
{
namespace lzma
{
bool	compress(std::string input, std::string &output);
bool	decompress(std::string input, uint64_t original_size, std::string &output);
}
}

#endif

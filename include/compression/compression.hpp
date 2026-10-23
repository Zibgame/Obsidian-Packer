/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compression.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 01:14:08 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 01:14:09 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

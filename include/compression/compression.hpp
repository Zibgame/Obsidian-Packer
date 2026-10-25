/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   compression.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 01:14:08 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef COMPRESSION_HPP
#define COMPRESSION_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace compression
{
struct Algorithm
{
	uint8_t		id;
	bool		(*compress)(std::string input, std::string &output);
	bool		(*decompress)(std::string input, uint64_t original_size, std::string &output);
};

const Algorithm	*find(std::string name);
const Algorithm	*get(uint8_t id);
std::vector<const Algorithm *>	all();
std::string	names();
const char	*name_of(uint8_t id);
}

#endif

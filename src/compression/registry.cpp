/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   registry.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "compression/compression.hpp"

#include "compression/lzma.hpp"

namespace
{
const compression::Algorithm	g_algorithms[] =
{
	{ 1, compression::lzma::compress, compression::lzma::decompress }
};

const size_t	g_algorithm_count = sizeof(g_algorithms) / sizeof(g_algorithms[0]);
}

const compression::Algorithm	*compression::get(uint8_t id)
{
	size_t	index;

	index = 0;
	while (index < g_algorithm_count)
	{
		if (g_algorithms[index].id == id)
			return (&g_algorithms[index]);
		index++;
	}
	return (NULL);
}

std::vector<const compression::Algorithm *>	compression::all()
{
	std::vector<const Algorithm *>	registry;
	size_t				index;

	index = 0;
	while (index < g_algorithm_count)
	{
		registry.push_back(&g_algorithms[index]);
		index++;
	}
	return (registry);
}

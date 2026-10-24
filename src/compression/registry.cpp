/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   registry.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "compression/compression.hpp"

#include "compression/lzma.hpp"

/*
** Adding a backend means adding one entry here and one pair of functions in
** its own translation unit. Identifiers are written into the packed footer
** and must stay stable once assigned.
*/

namespace
{
const compression::Algorithm	g_algorithms[] =
{
	{ "lzma", 1, compression::lzma::compress, compression::lzma::decompress }
};

const size_t	g_algorithm_count = sizeof(g_algorithms) / sizeof(g_algorithms[0]);
}

const compression::Algorithm	*compression::find(std::string name)
{
	size_t	index;

	index = 0;
	while (index < g_algorithm_count)
	{
		if (name == g_algorithms[index].name)
			return (&g_algorithms[index]);
		index++;
	}
	return (NULL);
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

std::string	compression::names()
{
	std::string	joined;
	size_t		index;

	index = 0;
	while (index < g_algorithm_count)
	{
		if (index != 0)
			joined += ", ";
		joined += g_algorithms[index].name;
		index++;
	}
	return (joined);
}

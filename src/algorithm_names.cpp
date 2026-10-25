/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   algorithm_names.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/25 02:37:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "compression/compression.hpp"
#include "crypto/crypto.hpp"

#include <string>
#include <vector>

namespace
{
struct AlgorithmName
{
	const char	*name;
	uint8_t		id;
};

const AlgorithmName	g_crypto_names[] =
{
	{ "chacha20", 1 }
};

const AlgorithmName	g_compression_names[] =
{
	{ "lzma", 1 }
};

const size_t	g_crypto_name_count = sizeof(g_crypto_names) / sizeof(g_crypto_names[0]);
const size_t	g_compression_name_count = sizeof(g_compression_names) / sizeof(g_compression_names[0]);

std::string	join_names(const AlgorithmName *names, size_t count)
{
	std::string	joined;
	size_t		index;

	index = 0;
	while (index < count)
	{
		if (index != 0)
			joined += ", ";
		joined += names[index].name;
		index++;
	}
	return (joined);
}

const char	*lookup_name(const AlgorithmName *names, size_t count, uint8_t id)
{
	size_t	index;

	index = 0;
	while (index < count)
	{
		if (names[index].id == id)
			return (names[index].name);
		index++;
	}
	return ("unknown");
}
}

const char	*crypto::name_of(uint8_t id)
{
	return (lookup_name(g_crypto_names, g_crypto_name_count, id));
}

const crypto::Algorithm	*crypto::find(std::string name)
{
	size_t	index;

	index = 0;
	while (index < g_crypto_name_count)
	{
		if (name == g_crypto_names[index].name)
			return (crypto::get(g_crypto_names[index].id));
		index++;
	}
	return (NULL);
}

std::string	crypto::names()
{
	return (join_names(g_crypto_names, g_crypto_name_count));
}

const char	*compression::name_of(uint8_t id)
{
	return (lookup_name(g_compression_names, g_compression_name_count, id));
}

const compression::Algorithm	*compression::find(std::string name)
{
	size_t	index;

	index = 0;
	while (index < g_compression_name_count)
	{
		if (name == g_compression_names[index].name)
			return (compression::get(g_compression_names[index].id));
		index++;
	}
	return (NULL);
}

std::string	compression::names()
{
	return (join_names(g_compression_names, g_compression_name_count));
}

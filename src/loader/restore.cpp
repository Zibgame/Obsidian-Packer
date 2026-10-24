/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 04:05:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 04:05:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "loader/restore.hpp"

/*
** Adding a restore mode means adding one entry here and one branch in the
** stub. Identifiers are written into the packed footer and must stay stable
** once assigned.
*/

namespace
{
struct RestoreMode
{
	const char	*name;
	uint8_t		id;
};

const RestoreMode	g_modes[] =
{
	{ "obsram", RESTORE_OBSRAM },
	{ "stomp", RESTORE_STOMP }
};

const size_t	g_mode_count = sizeof(g_modes) / sizeof(g_modes[0]);
}

const char	*loader::name_of(uint8_t id)
{
	size_t	index;

	index = 0;
	while (index < g_mode_count)
	{
		if (g_modes[index].id == id)
			return (g_modes[index].name);
		index++;
	}
	return (NULL);
}

bool	loader::id_of(std::string name, uint8_t &id)
{
	size_t	index;

	index = 0;
	while (index < g_mode_count)
	{
		if (name == g_modes[index].name)
		{
			id = g_modes[index].id;
			return (true);
		}
		index++;
	}
	return (false);
}

std::string	loader::names()
{
	std::string	joined;
	size_t		index;

	index = 0;
	while (index < g_mode_count)
	{
		if (index != 0)
			joined += ", ";
		joined += g_modes[index].name;
		index++;
	}
	return (joined);
}

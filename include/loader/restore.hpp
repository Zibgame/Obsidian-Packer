/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   restore.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 04:05:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 04:05:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOADER_RESTORE_HPP
#define LOADER_RESTORE_HPP

#include <cstdint>
#include <string>

/*
** How the stub obtains the memory region that receives the restored sections.
**
**   obsram  the reserved section of the stub image (default)
**   stomp   a legitimately loaded system module, whose pages are overwritten
**
** The chosen mode is written into the packed footer, so a single stub binary
** supports both.
*/

#define RESTORE_OBSRAM 1
#define RESTORE_STOMP 2

namespace loader
{
const char	*name_of(uint8_t id);
bool		id_of(std::string name, uint8_t &id);
std::string	names();
}

#endif

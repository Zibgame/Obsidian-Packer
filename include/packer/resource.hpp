/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   resource.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 04:15:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 04:15:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PACKER_RESOURCE_HPP
#define PACKER_RESOURCE_HPP

#include "pe/pe.hpp"

#include <string>

namespace packer
{
/*
** Copies the payload's resource directory into the stub image, so the packed
** binary presents the payload's version information, manifest and icons
** instead of the stub's own.
*/
bool	copy_resources(std::string &stub, std::string &payload, PEInfo &info);
}

#endif

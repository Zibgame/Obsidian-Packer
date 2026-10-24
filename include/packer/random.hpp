/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   random.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PACKER_RANDOM_HPP
#define PACKER_RANDOM_HPP

#include <cstddef>

namespace packer
{
bool	random_bytes(unsigned char *buffer, size_t size);
}

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   random.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 04:15:51 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "packer/random.hpp"

#include <windows.h>
#include <bcrypt.h>

bool	packer::random_bytes(unsigned char *buffer, size_t size)
{
	NTSTATUS	status;

	if (buffer == NULL || size == 0)
		return (false);
	status = BCryptGenRandom(NULL, buffer, static_cast<ULONG>(size),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (status != 0)
		return (false);
	return (true);
}

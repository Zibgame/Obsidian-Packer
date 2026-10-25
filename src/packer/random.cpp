/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   random.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 03:08:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "packer/random.hpp"

#ifdef _WIN32
#include <windows.h>
#include <bcrypt.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#endif

bool	packer::random_bytes(unsigned char *buffer, size_t size)
{
#ifdef _WIN32
	NTSTATUS	status;

	if (buffer == NULL || size == 0)
		return (false);
	status = BCryptGenRandom(NULL, buffer, static_cast<ULONG>(size),
		BCRYPT_USE_SYSTEM_PREFERRED_RNG);
	if (status != 0)
		return (false);
	return (true);
#else
	int	fd;
	size_t	done;
	ssize_t	chunk;

	if (buffer == NULL || size == 0)
		return (false);
	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		return (false);
	done = 0;
	while (done < size)
	{
		chunk = read(fd, buffer + done, size - done);
		if (chunk <= 0)
		{
			close(fd);
			return (false);
		}
		done += static_cast<size_t>(chunk);
	}
	close(fd);
	return (true);
#endif
}

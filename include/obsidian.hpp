/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   obsidian.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:57:12 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef OBSIDIAN_HPP
#define OBSIDIAN_HPP

#define OBSIDIAN_NAME "obsidian"
#define OBSIDIAN_VERSION "0.1.0-alpha"

#ifdef OBSIDIAN_DEBUG
#define OBSIDIAN_LOG(...) do { std::cout << __VA_ARGS__; } while (0)
#else
#define OBSIDIAN_LOG(...) ((void)0)
#endif

#define OBSD_VERSION 4
#define OBSD_FOOTER_SIZE 48
#define OBSD_TAG 0x6A09E667u

#define OBSD_OFFSET_TAG 0
#define OBSD_OFFSET_VERSION 4
#define OBSD_OFFSET_COMPRESSION 5
#define OBSD_OFFSET_ENCRYPTION 6
#define OBSD_OFFSET_NONCE_SIZE 7
#define OBSD_OFFSET_NONCE 8
#define OBSD_NONCE_MAX 12
#define OBSD_OFFSET_STORED_SIZE 20
#define OBSD_OFFSET_ORIGINAL_SIZE 28
#define OBSD_OFFSET_RESTORE 36
#define OBSD_OFFSET_KEY_RVA 40
#define OBSD_OFFSET_REGION_SIZE 44

#define OBSIDIAN_KEY_SIZE 32

inline unsigned long long obsidian_footer_avalanche(unsigned long long value)
{
	value = value ^ (value << 13);
	value = value ^ (value >> 7);
	value = value ^ (value << 17);
	return (value);
}

inline void obsidian_footer_cipher(unsigned char *data, unsigned int size, unsigned long long seed)
{
	unsigned long long state;
	unsigned int index;

	state = seed ^ 0x9E3779B97F4A7C15ULL;
	index = 0;
	while (index < size)
	{
		state = obsidian_footer_avalanche(state);
		data[index] = data[index] ^ static_cast<unsigned char>(state & 0xFF);
		index++;
	}
}

#endif

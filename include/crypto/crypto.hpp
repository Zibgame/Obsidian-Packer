/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   crypto.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/25 02:37:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace crypto
{
struct Algorithm
{
	uint8_t		id;
	uint32_t	nonce_size;
	bool		(*encrypt)(std::string &data, unsigned char *key, unsigned char *nonce);
	bool		(*decrypt)(std::string &data, unsigned char *key, unsigned char *nonce);
	void		(*mask)(unsigned char *key, unsigned char *nonce);
};

const Algorithm	*find(std::string name);
const Algorithm	*get(uint8_t id);
std::vector<const Algorithm *>	all();
std::string	names();
const char	*name_of(uint8_t id);
}

#endif

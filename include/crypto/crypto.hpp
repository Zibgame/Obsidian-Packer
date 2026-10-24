/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   crypto.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CRYPTO_HPP
#define CRYPTO_HPP

#include <cstdint>
#include <string>
#include <vector>

/*
** A backend encrypts and decrypts in place with a 32-byte key and a nonce of
** its own size. mask() folds a nonce-derived value into the key, so the key
** never sits in the stub image as a literal.
*/

namespace crypto
{
struct Algorithm
{
	const char	*name;
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
}

#endif

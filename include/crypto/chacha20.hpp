/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   chacha20.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CRYPTO_CHACHA20_HPP
#define CRYPTO_CHACHA20_HPP

#include "crypto/crypto.hpp"

#include <cstdint>
#include <string>

namespace crypto
{
namespace chacha20
{
bool	encrypt(std::string &data, unsigned char *key, unsigned char *nonce);
bool	decrypt(std::string &data, unsigned char *key, unsigned char *nonce);
void	mask(unsigned char *key, unsigned char *nonce);
}
}

#endif

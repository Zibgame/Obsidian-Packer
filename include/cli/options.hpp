/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   options.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/24 03:40:00 by marvin            #+#    #+#             */
/*   Updated: 2026/10/24 03:40:00 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLI_OPTIONS_HPP
#define CLI_OPTIONS_HPP

#include "compression/compression.hpp"
#include "crypto/crypto.hpp"
#include "loader/restore.hpp"

#include <string>

struct Options
{
	std::string			payload_path;
	std::string			stub_path;
	std::string			output_path;
	std::string			section;
	bool				auto_stub;
	uint8_t				restore;
	const compression::Algorithm	*compression;
	const crypto::Algorithm		*encryption;
	bool				help;
};

bool		parse_options(int argc, char **argv, Options &options, std::string &error);
std::string	usage_text();

#endif

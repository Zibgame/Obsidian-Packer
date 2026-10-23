/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 01:53:54 by zcadinot          #+#    #+#             */
/*   Updated: 2026/10/23 02:44:19 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"

int	main(int argc, char **argv)
{
	FILE	*stub_file;

	if (argc != 2)
	{
		printf("Usage: %s <stub.exe>\n", OBSIDIAN_NAME);
		return (1);
	}
	stub_file = fopen(argv[1], "rb");
	if (stub_file == NULL)
	{
		printf("obsidian: cannot open stub: %s\n", argv[1]);
		return (1);
	}
	if (fclose(stub_file) != 0)
		return (1);
	printf("Stub selected: %s\n", argv[1]);
	return (0);
}

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
	char	buffer[4096];
	char	output_path[] = "obsidian-pack.exe";
	FILE	*packed_file;
	FILE	*stub_file;
	size_t	read_size;

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
	packed_file = fopen(output_path, "wb");
	if (packed_file == NULL)
	{
		fclose(stub_file);
		return (1);
	}
	while (1)
	{
		read_size = fread(buffer, 1, sizeof(buffer), stub_file);
		if (read_size == 0)
			break ;
		if (fwrite(buffer, 1, read_size, packed_file) != read_size)
		{
			fclose(stub_file);
			fclose(packed_file);
			remove(output_path);
			return (1);
		}
	}
	if (ferror(stub_file))
	{
		fclose(stub_file);
		fclose(packed_file);
		remove(output_path);
		return (1);
	}
	if (fwrite("hello", 1, 5, packed_file) != 5)
	{
		fclose(stub_file);
		fclose(packed_file);
		remove(output_path);
		return (1);
	}
	if (fclose(stub_file) != 0)
	{
		fclose(packed_file);
		remove(output_path);
		return (1);
	}
	if (fclose(packed_file) != 0)
	{
		remove(output_path);
		return (1);
	}
	printf("Packed binary created: .\\%s\n", output_path);
	return (0);
}

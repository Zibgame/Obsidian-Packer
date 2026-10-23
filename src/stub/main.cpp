/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 02:08:58 by marvin            #+#    #+#             */
/*   Updated: 2026/10/23 02:08:58 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "obsidian.hpp"
#include <windows.h>

int	main(void)
{
	LARGE_INTEGER	self_size;
	char		self_path[MAX_PATH];
	char		message[] = "[OBSIDIAN STUB] Execution successful.\r\n";
	DWORD		path_length;
	DWORD		written;
	HANDLE		output;
	HANDLE		self_file;

	output = GetStdHandle(STD_OUTPUT_HANDLE);
	if (output == INVALID_HANDLE_VALUE)
		return (1);
	if (!WriteFile(output, message, sizeof(message) - 1, &written, NULL))
		return (1);

	path_length = GetModuleFileNameA(NULL, self_path, MAX_PATH);
	if (path_length == 0 || path_length == MAX_PATH)
		return (1);

	self_file = CreateFileA(self_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (self_file == INVALID_HANDLE_VALUE)
		return (1);

	if (!GetFileSizeEx(self_file, &self_size))
		return (1);

	printf("%lld\n", self_size.QuadPart);
	if (!CloseHandle(self_file))
		return (1);
	return (0);
}

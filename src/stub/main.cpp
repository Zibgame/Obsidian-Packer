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
	char		message[] = "[OBSIDIAN STUB] Execution successful.\r\n";
	DWORD		written;
	HANDLE		output;

	output = GetStdHandle(STD_OUTPUT_HANDLE);
	if (output == INVALID_HANDLE_VALUE)
		return (1);
	if (!WriteFile(output, message, sizeof(message) - 1, &written, NULL))
		return (1);
	return (0);
}

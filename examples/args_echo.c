/* args_echo: checks argument forwarding through the PEB */
#include <stdio.h>

int	main(int argc, char **argv)
{
	int	index;

	printf("[args_echo] argc = %d\n", argc);
	index = 0;
	while (index < argc)
	{
		printf("[args_echo] argv[%d] = %s\n", index, argv[index]);
		index++;
	}
	return (0);
}
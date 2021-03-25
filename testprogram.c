#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
int main()
{
	char hexnum[] = "9a3f";
	int decimal = 0;
	printf("%li\n", strlen(hexnum));
	for (int i = 0; i < strlen(hexnum); i++)
	{
		if (hexnum[i] == 'a' || hexnum[i] == 'A')
			hexnum[i] = 10;
		else if (hexnum[i] == 'b' || hexnum[i] == 'B')
			hexnum[i] = 11;
		else if (hexnum[i] == 'c' || hexnum[i] == 'C')
			hexnum[i] = 12;
		else if (hexnum[i] == 'd' || hexnum[i] == 'D')
			hexnum[i] = 13;
		else if (hexnum[i] == 'e' || hexnum[i] == 'E')
			hexnum[i] = 14;
		else if (hexnum[i] == 'f' || hexnum[i] == 'F')
			hexnum[i] = 15;
		else
			hexnum[i] -= '0';
		decimal += hexnum[i] * pow(16, (strlen(hexnum) - i - 1));
		printf("%i\n", decimal);
	}
	printf("%i\n", decimal);
	return 0;
}
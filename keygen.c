#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{

	srand(time(NULL));
	char *key = malloc(argc*sizeof(char)+1);
	
	int temp = atoi(argv[1]);

	for (int i = 0; i < temp; i++)
	{
		int aha = rand()%27+1;
	
		if (aha == 27)
		{
			key[i] = ' ';
		}
		else
		{
			key[i] = 65 + rand() / (RAND_MAX / (90-65+1)+1);
		}
	}
	

		printf("%s\n", key);
	
		
	return 0;
}

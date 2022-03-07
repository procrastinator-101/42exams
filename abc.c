#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>


int main()
{
	fork();
	sleep(1);
	return 0;
}
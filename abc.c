#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>


int main(int argc, char **argv, char **envp)
{
	char *args[3] = {"/bin/echo", "hello", 0};
	int pid  = fork();
	if (pid != 0)
		execve(args[0], args, envp);
	return 0;
}
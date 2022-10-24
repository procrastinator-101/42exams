#ifndef MICROSHELL_H
# define MICROSHELL_H

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

# define LRDC	0
# define HRDC	1
# define RRDC	2

/*
	./pipex	file1 cmd1 cmd2 cmd3 ... cmdn file2
			< file1 cmd1 | cmd2 | cmd3 ... | cmdn > file2

	./pipex here_doc LIMITER cmd cmd1 file
			cmd << LIMITER | cmd1 >> file
*/

typedef struct s_scmd
{
	int				fdin;
	int				fdout;
	char			**args;
	struct s_scmd	*next;
}					t_scmd;

#endif
#ifndef MICROSHELL_H
# define MICROSHELL_H

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

# define LRDC	0
# define HRDC	1
# define RRDC	2

typedef struct s_rdc
{
	int				type;
	char			*operand;
	struct s_rdc	*next;
}						t_rdc;

typedef struct s_scmd
{
	int				fdin;
	int				fdout;
	char			**args;
	t_rdc			*rdcs;
	struct s_scmd	*next;
}					t_scmd;

#endif
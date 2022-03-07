#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>



typedef	struct s_scmd
{
	char			op;
	int				fdin;
	int				fdout;
	char			**args;
	struct s_scmd	*next;
}					t_scmd;

void	scmd_display(t_scmd *cmd)
{
	t_scmd	*head;

	head = cmd;
	while (head)
	{
		printf("=================================================================\n");
		printf("cmd   : {%s}\n", head->args[0]);
		printf("args  : {");
		for (int i = 1; head->args[i]; i++)
			printf("%s ", head->args[i]);
		printf("}\n");
		printf("fdin  : %d\n", head->fdin);
		printf("fdout : %d\n", head->fdout);
		printf("op    : %c\n", head->op);
		head = head->next;
	}
	printf("=================================================================\n");
}

t_scmd	*scmd_create(char **argv, int argc, int *idx)
{
	int		size;
	int		start;
	t_scmd	*scmd;

	scmd = malloc(sizeof(t_scmd));
	scmd->fdin = STDIN_FILENO;
	scmd->fdout = STDOUT_FILENO;
	scmd->next = 0;
	size = 1;
	start = *idx;
	for (*idx = start; *idx < argc; (*idx)++)
	{
		if (!strcmp(argv[*idx], "|") || !strcmp(argv[*idx], ";"))
		{
			scmd->op = argv[*idx][0];
			(*idx)++;
			break ;
		}
		size++;
	}
	scmd->args = malloc(size * sizeof(char *));
	for (int i = 0; i < size - 1; i++)
		scmd->args[i] = argv[start + i];
	scmd->args[size - 1] = 0;
	return scmd;
}

void	scmd_addback(t_scmd **tail, t_scmd *node)
{
	t_scmd	*head;

	head = *tail;
	if (!head)
		*tail = node;
	else
	{
		while (head->next)
			head = head->next;
		head->next = node;
	}
}

void	scmd_clear(t_scmd **tail)
{
	t_scmd	*next;
	t_scmd	*head;

	head = *tail;
	while (head)
	{
		next = head->next;
		free(head->args);
		free(head);
		head = next;
	}
	*tail = 0;
}

void	scmd_execute(t_scmd *scmd, t_scmd *previous, int oldpipe[2], int newpipe[2], char **envp)
{
	int	pid;
	int	status;

	if (!strcmp("cd", scmd->args[0]))
	{
		printf("cd\n");
	}
	else
	{
		pid = fork();
		if (pid == 0)
		{
			if (scmd->fdin != STDIN_FILENO)
				dup2(scmd->fdin, STDIN_FILENO);
			if (scmd->fdout != STDOUT_FILENO)
				dup2(scmd->fdout, STDOUT_FILENO);
			execve(scmd->args[0], scmd->args, envp);
			exit(1);
		}
		//
		if (oldpipe[0] != -1)
			close(oldpipe[0]);
		if (scmd->op == '|')
		{
			oldpipe[0] = newpipe[0];
			oldpipe[1] = newpipe[1];
		}
		else
		{
			oldpipe[0] = -1;
			oldpipe[1] = -1;
			close(newpipe[0]);
			close(newpipe[1]);
		}
		//
		waitpid(pid, &status, 0);
	}
}

int main(int argc, char **argv, char **envp)
{
	int		oldpipe[2];
	int		newpipe[2];
	t_scmd	*cmd = 0;
	t_scmd	*head;
	t_scmd	*previous = 0;

	for (int i = 1; i < argc;)
	{
		if (strcmp(argv[i], "|") && strcmp(argv[i], ";"))
		{
			head = scmd_create(argv, argc, &i);
			scmd_addback(&cmd, head);
		}
		else
			i++;
	}
	// scmd_display(cmd);

	head = cmd;
	oldpipe[0] = -1;
	oldpipe[1] = -1;
	while (head)
	{
		if (oldpipe[1] != -1)
			close(oldpipe[1]);
		if (head->op == '|')
		{
			pipe(newpipe);
			printf("in = %d | out = %d\n", newpipe[0], newpipe[1]);
			if (head->next)
				head->fdout = newpipe[1];
		}
		if (previous && previous->op == '|')
			head->fdin = oldpipe[0];
		scmd_execute(head, previous, oldpipe, newpipe, envp);
		
		previous = head;
		head = head->next;
	}
	scmd_display(cmd);
	scmd_clear(&cmd);
	return 0;
}
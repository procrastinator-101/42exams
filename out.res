/bin/ls
README.md
abc.c
grade.sh
microshell
microshell.c
microshell.dSYM
microshell.h
notes.txt
out.res
pipex.c
test

/bin/cat microshell.c
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

int	ft_strlen(char *str)
{
	int	len;

	len = 0;
	while (str[len])
		len++;
	return len;
}

void	ft_putstr_fd(char *str, int fd)
{
	int	len;

	len = ft_strlen(str);
	write(fd, str, len);
}

void	ft_manage_fatal_error()
{
	ft_putstr_fd("error: fatal\n", STDERR_FILENO);
	exit(EXIT_FAILURE);
}

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
	if (!scmd)
		return 0;
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
	if (!scmd->args)
	{
		free(scmd);
		return 0;
	}
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

	if (!tail)
		return;
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

void	ft_update_pipes(int oldpipe[2], int newpipe[2])
{
	if (newpipe[1] != -1)
	{
		close(newpipe[1]);
		newpipe[1] = -1;
	}
	if (oldpipe[0] != -1)
	{
		close(oldpipe[0]);
		oldpipe[0] = -1;
	}
	oldpipe[0] = newpipe[0];
	newpipe[0] = -1;
}

void	scmd_execute(t_scmd *scmd, char **envp, int oldpipe[2], int newpipe[2])
{
	int	pid;
	int	len;
	int	tmpin;
	int	tmpout;
	int	status;

	if (!strcmp("cd", scmd->args[0]))
	{
		len = 0;
		while (scmd->args[len])
			len++;
		if (len != 2)
		{
			ft_putstr_fd("error: cd: bad arguments\n", STDERR_FILENO);
			ft_update_pipes(oldpipe, newpipe);
			return;
		}
		tmpin = dup(STDIN_FILENO);
		tmpout = dup(STDOUT_FILENO);
		dup2(scmd->fdin, STDIN_FILENO);
		dup2(scmd->fdout, STDOUT_FILENO);
		ft_putstr_fd("cd\n", 1);
		status = chdir(scmd->args[1]);
		dup2(tmpin, STDIN_FILENO);
		dup2(tmpout, STDOUT_FILENO);
		close(tmpin);
		close(tmpout);
		ft_update_pipes(oldpipe, newpipe);
		if (status)
		{
			ft_putstr_fd("error: cd: cannot change directory to ", STDERR_FILENO);
			ft_putstr_fd(scmd->args[1], STDERR_FILENO);
			ft_putstr_fd("\n", STDERR_FILENO);
			return;
		}
	}
	else
	{
		pid = fork();
		if (pid == 0)
		{
			if (newpipe[0] != -1)
				close(newpipe[0]);
			// printf("+fdin  : %d\n", scmd->fdin);
			// printf("+fdout : %d\n", scmd->fdout);
			dup2(scmd->fdin, STDIN_FILENO);
			dup2(scmd->fdout, STDOUT_FILENO);
			execve(scmd->args[0], scmd->args, envp);
			ft_putstr_fd("error: cannot execute ", STDERR_FILENO);
			ft_putstr_fd(scmd->args[0], STDERR_FILENO);
			ft_putstr_fd("\n", STDERR_FILENO);
			exit(EXIT_FAILURE);
		}
		else if (pid < 0)
			ft_manage_fatal_error();
		ft_update_pipes(oldpipe, newpipe);
		waitpid(pid, &status, 0);
	}
}

void	ft_cmd_execute(t_scmd *cmd, char **envp)
{
	int		oldpipe[2];
	int		newpipe[2];
	t_scmd	*head;
	t_scmd	*previous;

	head = cmd;
	previous = 0;
	oldpipe[0] = -1;
	oldpipe[1] = -1;
	newpipe[0] = -1;
	newpipe[1] = -1;
	while (head)
	{
		if (head->op == '|' && head->next)
		{
			pipe(newpipe);
			// printf("in = %d | out = %d\n", newpipe[0], newpipe[1]);
			head->fdout = newpipe[1];
		}
		if (previous && previous->op == '|')
			head->fdin = oldpipe[0];
		scmd_execute(head, envp, oldpipe, newpipe);
		previous = head;
		head = head->next;
	}
}

int main(int argc, char **argv, char **envp)
{
	t_scmd	*cmd;
	t_scmd	*tmp;

	cmd = 0;
	for (int i = 1; i < argc;)
	{
		if (strcmp(argv[i], "|") && strcmp(argv[i], ";"))
		{
			tmp = scmd_create(argv, argc, &i);
			if (!tmp)
				ft_manage_fatal_error();
			scmd_addback(&cmd, tmp);
		}
		else
			i++;
	}
	ft_cmd_execute(cmd, envp);
	// scmd_display(cmd);
	scmd_clear(&cmd);
	return 0;
}
/bin/ls microshell.c
microshell.c

/bin/ls salut

;

; ;

; ; /bin/echo OK
OK

; ; /bin/echo OK ;
OK

; ; /bin/echo OK ; ;
OK

; ; /bin/echo OK ; ; ; /bin/echo OK
OK
OK

/bin/ls | /usr/bin/grep microshell
microshell
microshell.c
microshell.dSYM
microshell.h

/bin/ls | /usr/bin/grep microshell | /usr/bin/grep micro
microshell
microshell.c
microshell.dSYM
microshell.h

/bin/ls | /usr/bin/grep microshell | /usr/bin/grep micro | /usr/bin/grep shell | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro
microshell
microshell.c
microshell.dSYM
microshell.h

/bin/ls | /usr/bin/grep microshell | /usr/bin/grep micro | /usr/bin/grep shell | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep micro | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell | /usr/bin/grep shell
microshell
microshell.c
microshell.dSYM
microshell.h

/bin/ls ewqew | /usr/bin/grep micro | /bin/cat -n ; /bin/echo dernier ; /bin/echo
dernier


/bin/ls | /usr/bin/grep micro | /bin/cat -n ; /bin/echo dernier ; /bin/echo ftest ;
     1	microshell
     2	microshell.c
     3	microshell.dSYM
     4	microshell.h
dernier
ftest

/bin/echo ftest ; /bin/echo ftewerwerwerst ; /bin/echo werwerwer ; /bin/echo qweqweqweqew ; /bin/echo qwewqeqrtregrfyukui ;
ftest
ftewerwerwerst
werwerwer
qweqweqweqew
qwewqeqrtregrfyukui

/bin/ls ftest ; /bin/ls ; /bin/ls werwer ; /bin/ls microshell.c ; /bin/ls subject.fr.txt ;
README.md
abc.c
grade.sh
leaks.res
microshell
microshell.c
microshell.dSYM
microshell.h
notes.txt
out.res
pipex.c
test
microshell.c

/bin/ls | /usr/bin/grep micro ; /bin/ls | /usr/bin/grep micro ; /bin/ls | /usr/bin/grep micro ; /bin/ls | /usr/bin/grep micro ;
microshell
microshell.c
microshell.dSYM
microshell.h
microshell
microshell.c
microshell.dSYM
microshell.h
microshell
microshell.c
microshell.dSYM
microshell.h
microshell
microshell.c
microshell.dSYM
microshell.h

/bin/cat subject.fr.txt | /usr/bin/grep a | /usr/bin/grep b ; /bin/cat subject.fr.txt ;

/bin/cat subject.fr.txt | /usr/bin/grep a | /usr/bin/grep w ; /bin/cat subject.fr.txt ;

/bin/cat subject.fr.txt | /usr/bin/grep a | /usr/bin/grep w ; /bin/cat subject.fr.txt

/bin/cat subject.fr.txt ; /bin/cat subject.fr.txt | /usr/bin/grep a | /usr/bin/grep b | /usr/bin/grep z ; /bin/cat subject.fr.txt

; /bin/cat subject.fr.txt ; /bin/cat subject.fr.txt | /usr/bin/grep a | /usr/bin/grep b | /usr/bin/grep z ; /bin/cat subject.fr.txt

blah | /bin/echo OK
OK

blah | /bin/echo OK ;
OK


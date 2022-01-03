#include "microshell.h"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>


//scmd : here_doc LIMITER inputFile cmd outputFile


char	*ft_substr(const char *str, size_t start, size_t size)
{
	size_t	i;
	size_t	len;
	char	*ret;

	if (!str)
		return 0;
	len = 0;
	while (str[len] && len < start + size)
		len++;
	if (len <= start)
		len = 0;
	ret = malloc((len + 1) * sizeof(char));
	if (!ret)
		return 0;
	ret[len] = 0;
	i = -1;
	while (++i < size && start + i < len)
		ret[i] = str[start + i];
	return ret;
}

void	ft_scmd_addback(t_scmd **tail, t_scmd *node)
{
	t_scmd	*head;

	if (!*tail)
		*tail = node;
	else
	{
		head = *tail;
		while (head->next)
			head = head->next;
		head->next = node;
	}
}

t_scmd	*ft_scmd_create(void)
{
	t_scmd	*node;

	node = malloc(sizeof(t_scmd));
	if (!node)
		return 0;
	node->fdin = STDIN_FILENO;
	node->fdout = STDOUT_FILENO;
	node->args = 0;
	node->rdcs = 0;
	node->next = 0;
	return node;
}

t_rdc	*ft_rdc_create(char *operand, int type)
{
	t_rdc	*rdc;

	rdc = malloc(sizeof(t_rdc));
	if (!rdc)
		return 0;
	rdc->operand = strdup(operand);
	rdc->type = type;
	rdc->next = 0;
	return rdc;
}

void	ft_rdc_addback(t_rdc **tail, t_rdc *node)
{
	t_rdc	*head;

	if (!*tail)
		*tail = node;
	else
	{
		head = *tail;
		while (head->next)
			head = head->next;
		head->next = node;
	}
}

void	ft_args_clear(char **args)
{
	int	i;

	i = 0;
	while (args && args[i])
		free(args[i]);
	free(args);
}

char	**ft_addarg(char **args, char *str, int start, int size)
{
	int 	i;
	char	*ptr;
	char	**ret;

	ptr = ft_substr(str, start, size);
	if (!ptr)
	{
		ft_args_clear(args);
		return 0;
	}
	i = 0;
	while (args && args[i])
		i++;
	ret = malloc(sizeof(char *) * (i + 2));
	if (!ret)
	{
		ft_args_clear(args);
		return 0;
	}
	i = 0;
	while (args && args[i])
	{
		ret[i] = args[i];
		i++;
	}
	ret[i] = ptr;
	ret[i + 1] = 0;
	free(args);
	return ret;
}

char	**ft_getargs(char *str)
{
	int		i;
	int		start;
	char	**args;

	i = 0;
	start = 0;
	while (str[i])
	{
		if (str[i] == ' ')
		{
			args = ft_addarg(args, str, start, i - start);
			if (!args)
				return 0;
			start = i + 1;
		}
	}
	return args;
}

t_scmd	*ft_pipeline_parse(int argc, char **argv)
{
	int		i;
	int		iscmd;
	t_rdc	*rdc;
	t_scmd	*scmd;
	t_scmd	*pipeline;

	i = 0;
	iscmd = 0;
	pipeline = 0;
	while (++i < argc)
	{
		if (i == 1 && strcmp(argv[i], "here_doc"))
		{
			scmd = calloc(sizeof(t_scmd), 1);
			ft_scmd_addback(&pipeline, scmd);
			rdc = ft_rdc_create(argv[i], LRDC);
			ft_rdc_addback(&(scmd->rdcs), rdc);
		}
		else if (i == 2 && !strcmp(argv[1], "here_doc"))
		{
			rdc = ft_rdc_create(argv[i], HRDC);
			ft_rdc_addback(&(scmd->rdcs), rdc);
		}
		else if (i == argc - 1)
		{
			rdc = ft_rdc_create(argv[i], RRDC);
			ft_rdc_addback(&(scmd->rdcs), rdc);
		}
		else
		{
			if (iscmd)
			{
				scmd = calloc(sizeof(t_scmd), 1);
				ft_scmd_addback(&pipeline, scmd);
			}
			scmd->args = ft_getargs(argv[i]);
			iscmd = 1;
		}
	}
	return pipeline;
}

void	ft_rdc_del(t_rdc *node)
{
	if (!node)
		return;
	free(node->operand);
	free(node);
}

void	ft_rdc_clear(t_rdc *tail)
{
	t_rdc	*head;
	t_rdc	*next;

	head = tail;
	while (head)
	{
		next = head->next;
		ft_rdc_del(head);
		head = next;
	}
}

void	ft_scmd_del(t_scmd *node)
{
	if (!node)
		return;
	ft_args_clear(node->args);
	ft_rdc_clear(node->rdcs);
	free(node);
}

void	ft_scmd_clear(t_scmd *tail)
{
	t_scmd	*head;
	t_scmd	*next;

	head = tail;
	while (head)
	{
		next = head->next;
		ft_scmd_del(head);
		head = next;
	}
}

void	ft_pipeline_execute(t_scmd *pipeline)
{
	int	nb;
	int	pid;
	int	cpipe[2];
	int	lastpipe[2];
	t_scmd	*scmd;

	nb = 0;
	scmd = pipeline;
	while (scmd)
	{
		pipe(cpipe);
		if (!nb)
			scmd->fdout = cpipe[0];
		else if (!scmd->next)
			scmd->fdin = lastpipe[1];
		else
		{
			scmd->fdout = cpipe[0];
			scmd->fdin = lastpipe[1];
		}
		pid = fork();
		if (pid)
		{
			ft_scmd_execute(scmd);
		}
		else
		{
		
		}
		if (nb)
		{
			close(cpipe[0]);
			close(cpipe[1]);
		}
		nb++;
	}
}

int main(int argc, char **argv, char **envp)
{
	t_scmd	*pipeline;

	pipeline = ft_pipeline_parse(argc, argv);
	if (!pipeline)
		return -1;;
	ft_pipeline_execute(pipeline);
	ft_scmd_clear(pipeline);
	return 0;
}
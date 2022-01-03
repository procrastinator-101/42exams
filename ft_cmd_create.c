#include "microshell.h"

t_cmd   *ft_cmd_create(char *name, char **args, int size)
{
    t_cmd   *cmd;

    cmd = malloc(sizeof(t_cmd));
    if (!cmd)
        return (0);
    cmd->name = ft_strdup(name);
    cmd->args = malloc(sizeof(char *) * (size + 1));
    cmd->args[size] = 0;
    while (size--)
        cmd->args[size] = args[size];
    cmd->next = 0;
    return (cmd);
}
#include <errno.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

//======================================================================================================
// basic methods
//======================================================================================================
void	ft_putstr_fd(char *str, int fd)
{
	write(fd, str, strlen(str));
}

void	ft_manage_fatal_error()
{
	ft_putstr_fd("Fatal error\n", 2);
	exit(1);
}

int	ft_max(int a, int b)
{
	return a > b ? a : b;
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}
//======================================================================================================
// basic methods	End
//======================================================================================================


//======================================================================================================
// Client methods
//======================================================================================================
typedef struct s_client
{
	int 			id;
	int 			fd;
	char			*msg;
	struct s_client	*next;
}					t_client;

t_client	*ft_client_create(int id, int fd)
{
	t_client	*client;

	client = malloc(sizeof(t_client));
	if (!client)
		return 0;
	client->id = id;
	client->fd = fd;
	client->msg = 0;
	client->next = 0;
	return client;
}

void	ft_client_del(t_client *client)
{
	if (!client)
		return ;
	close(client->fd);
	free(client->msg);
	free(client);
}

void	ft_client_addback(t_client **tail, t_client *client)
{
	t_client	*head;

	if (!*tail)
		*tail = client;
	else
	{
		head = *tail;
		while (head->next)
			head = head->next;
		head->next = client;
	}
}

void	ft_client_remove(t_client **tail, t_client *client)
{
	t_client	*head;
	t_client	*prev;

	prev = 0;
	head = *tail;
	while (head && head != client)
	{
		prev = head;
		head = head->next;
	}
	if (!head)
		return ;
	if (prev)
		prev->next = head->next;
	else
		*tail = head->next;
	ft_client_del(client);
}
//======================================================================================================
// Client methods	End
//======================================================================================================


//======================================================================================================
// state
//======================================================================================================
typedef struct s_server
{
	int				fd;
	int				ids;
	int				nfds;
	fd_set			readfds;
	fd_set			writefds;
	fd_set			watchedfds;
	t_client		*clients;
	struct timeval	timeout;
}					t_server;

t_server	server;

void	ft_initialize_server()
{
	server.ids = 0;
	server.nfds = server.fd + 1;
	FD_ZERO(&server.watchedfds);
	FD_SET(server.fd, &server.watchedfds);
	server.clients = 0;
	server.timeout.tv_sec = 0;
	server.timeout.tv_usec = 2000;
}

char	*ft_format_msg(char *fmt, int id, char *str)
{
	char	*buf;
	char	*ret;

	buf = calloc(50, sizeof(char));
	if (!buf)
		ft_manage_fatal_error();
	sprintf(buf, fmt, id);
	ret = str_join(buf, str);
	if (!ret)
		ft_manage_fatal_error();
	return ret;
}

void	ft_send_msg(t_client *client, char *msg)
{
	send(client->fd, msg, strlen(msg), 0);
}

void	ft_broadcast(t_client *client, char *msg)
{
	t_client	*head;

	head = server.clients;
	while (head)
	{
		if (head != client && FD_ISSET(head->fd, &server.writefds))
			ft_send_msg(head, msg);
		head = head->next;
	}
}

void	ft_accept_client()
{
	int					ret;
	char				*msg;
	t_client			*client;
	socklen_t			len;
	struct sockaddr_in	address;

	len = sizeof(struct sockaddr_in);
	ret = accept(server.fd, (struct sockaddr *)&address, &len);
	if (ret < 0)
		return ;
	client = ft_client_create(server.ids++, ret);
	if (!client)
		ft_manage_fatal_error();
	server.nfds = ft_max(server.nfds, ret + 1);
	FD_SET(ret, &server.watchedfds);
	ft_client_addback(&server.clients, client);
	msg = ft_format_msg("server: client %d ", client->id, "just arrived\n");
	ft_broadcast(client, msg);
	free(msg);
}

void	ft_disconnect_client(t_client *client)
{
	char	*msg;

	msg = ft_format_msg("server: client %d ", client->id, "just left\n");
	ft_broadcast(client, msg);
	free(msg);
	FD_CLR(client->fd, &server.watchedfds);
	ft_client_remove(&server.clients, client);
}

void	ft_send_client_msg(t_client *client)
{
	int		ret;
	char	*msg;
	char	*line;

	ret = 1;
	while (ret == 1)
	{
		line = 0;
		ret = extract_message(&client->msg, &line);
		if (ret < 0)
			ft_manage_fatal_error();
		else if (ret)
		{
			msg = ft_format_msg("client %d: ", client->id, line);
			ft_broadcast(client, msg);
			free(msg);
		}
		free(line);
	}
	
}

void	ft_read_from_client(t_client *client)
{
	int		ret;
	char	*buf;

	buf = malloc(1025 * sizeof(char));
	if (!buf)
		ft_manage_fatal_error();
	while (1)
	{
		ret = recv(client->fd, buf, 1024, 0);
		if (ret <= 0)
		{
			ft_disconnect_client(client);
			break ;
		}
		else
		{
			buf[ret] = 0;
			client->msg = str_join(client->msg, buf);
			if (!client->msg)
				ft_manage_fatal_error();
			ft_send_client_msg(client);
			if (ret < 1024)
				break;
		}
	}
	free(buf);
}

void	ft_communicate_with_clients()
{
	t_client	*head;

	head = server.clients;
	while (head)
	{
		if (FD_ISSET(head->fd, &server.readfds))
			ft_read_from_client(head);
		head = head->next;
	}
}

void	ft_server_up()
{
	int	ret;

	ft_initialize_server();
	while (1)
	{
		server.readfds = server.watchedfds;
		server.writefds = server.watchedfds;
		ret = select(server.nfds, &server.readfds, &server.writefds, 0, &server.timeout);
		if (ret <= 0)
			continue;
		if (FD_ISSET(server.fd, &server.readfds))
			ft_accept_client();
		else
			ft_communicate_with_clients();
	}
}


//======================================================================================================
// state End
//======================================================================================================




int main(int argc, char **argv)
{
	int	ret;
	int	port;
	struct sockaddr_in	address;


	if (argc != 2)
	{
		ft_putstr_fd("Wrong number of arguments\n", 2);
		exit(1);
	}

	// socket create and verification 
	server.fd = socket(AF_INET, SOCK_STREAM, 0); 
	if (server.fd == -1)
		ft_manage_fatal_error();

	// assign IP, PORT 
	port = atoi(argv[1]);
	bzero(&address, sizeof(address)); 
	address.sin_family = AF_INET; 
	address.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	address.sin_port = htons(port); 
  
	// Binding newly created socket to given IP and verification 
	ret = bind(server.fd, (const struct sockaddr *)&address, sizeof(address));
	if (ret == -1)
		ft_manage_fatal_error();
	
	ret = listen(server.fd, 10);
	if (ret == -1)
		ft_manage_fatal_error();
	
	ft_server_up();
	return 0;
}
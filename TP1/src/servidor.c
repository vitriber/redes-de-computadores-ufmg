#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>

#include <utils.h>
#include <lista.h>
#include <poll.h>

#define MAX_CON 50
#define MAX_WORKERS 100
#define WAITING_MS 100

pthread_mutex_t list_lock;
struct list students;
struct poll threads;

void close_server()
{
	join_workers(&threads);
	pthread_mutex_destroy(&list_lock);

	delete_poll(&threads);
	delete_list(&students);

	exit(EXIT_SUCCESS);
}

int start_server(struct sockaddr_in *servaddr, int port)
{
	int sockopt_rst, servsock = socket(AF_INET, SOCK_STREAM, 0);
	if (servsock == -1)
		logexit("ERROR socket()");

	sockopt_rst = setsockopt(servsock, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int));
	if (sockopt_rst == -1)
		logexit("ERROR setsockopt(SO_REUSEADDR)");

	servaddr->sin_family = AF_INET;
	servaddr->sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr->sin_port = htons(port);

	int bind_rst = bind(servsock, (struct sockaddr *)servaddr, sizeof(*servaddr));
	if (bind_rst != 0)
		logexit("ERROR bind()");

	int listen_rst = listen(servsock, MAX_CON);
	if (listen_rst != 0)
		logexit("ERROR listen()");

	init_list(&students);
	init_poll(&threads, MAX_WORKERS);

	signal(SIGINT, &close_server);

	int mutex_init_rst = pthread_mutex_init(&list_lock, NULL);
	if (mutex_init_rst != 0)
		logexit("ERROR pthread_mutex_init");

	return servsock;
}

void check(int code)
{
	if (code == TIMEOUT)
		return logmsg("TIMEOUT");
}

void handler_prof(int clisock, struct list *students)
{
	pthread_mutex_lock(&list_lock);
	struct node *it;
	for (it = begin(students); it != end(students); it = next(it))
	{
		char num[20];
		sprintf(num, "%d\n", it->val);
		send_str(clisock, num);
	}
	pthread_mutex_unlock(&list_lock);

	send_msg(clisock, "\0", 1);

	char buff[strlen(OK) + 1];
	int rst_recv = recv_str(clisock, buff, strlen(OK));
	if (rst_recv <= 0)
		return check(rst_recv);
}

void handler_stu(int clisock, struct list *students)
{
	send_str(clisock, OK);
	send_str(clisock, MATRICULA);

	int id;
	int rst_recv = recv_int(clisock, &id);
	if (rst_recv <= 0)
		return check(rst_recv);

	pthread_mutex_lock(&list_lock);
	push(students, id);
	pthread_mutex_unlock(&list_lock);
}

struct handler_args
{
	int clisock;
	char *pass_prof;
	char *pass_stu;
	struct list *students;
};

void *handler(void *args)
{
	struct handler_args *ha = (struct handler_args *)args;

	struct timeval timeout;
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;
	int sockopt_rst = setsockopt(ha->clisock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (sockopt_rst == -1)
	{
		logmsg("ERROR setsockopt(SO_RCVTIMEO)");
		return NULL;
	}

	send_str(ha->clisock, READY);

	char pass[PASS_LEN + 1];
	int rst_recv = recv_str(ha->clisock, pass, PASS_LEN);
	if (rst_recv <= 0)
	{
		check(rst_recv);
		return NULL;
	}

	if (strncmp(pass, ha->pass_prof, PASS_LEN) == 0)
		handler_prof(ha->clisock, ha->students);
	else if (strncmp(pass, ha->pass_stu, PASS_LEN) == 0)
		handler_stu(ha->clisock, ha->students);

	close(ha->clisock);
	free(args);

	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		char *args[1] = {"<port>"};
		help(argv[0], 1, args, NULL);
	}

	srand(time(0));

	char pass_prof[PASS_LEN + 1], pass_stu[PASS_LEN + 1];
	rand_str(pass_prof, PASS_LEN);
	rand_str(pass_stu, PASS_LEN);
	printf("%s\n%s\n", pass_prof, pass_stu);

	struct sockaddr_in servaddr;
	int servsock = start_server(&servaddr, atoi(argv[1]));

	while (1)
	{
		if (is_poll_full(&threads))
		{
			msleep(WAITING_MS);
			continue;
		}

		struct sockaddr_in cliaddr;
		socklen_t cliaddr_len = sizeof(cliaddr);

		int clisock = accept(servsock, (struct sockaddr *)&cliaddr, &cliaddr_len);
		if (clisock == -1)
			continue;

		struct handler_args *ha = malloc(sizeof(struct handler_args));
		ha->clisock = clisock;
		ha->pass_prof = pass_prof;
		ha->pass_stu = pass_stu;
		ha->students = &students;

		add_worker(&threads, handler, (void *)ha);
	}
}
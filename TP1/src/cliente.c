#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>

#include <cliente.h>

void handler(int sockfd, char *pass)
{
	char buff[MAX_LEN];
	int rst_recv;

	rst_recv = recv_str(sockfd, buff, strlen(READY));
	if (rst_recv <= 0)
		return checkexit(rst_recv);

	if (strncmp(buff, READY, strlen(READY)) != 0)
		return;

	send_str(sockfd, pass);

	rst_recv = recv_str(sockfd, buff, strlen(OK));
	if (rst_recv <= 0)
		return checkexit(rst_recv);

	if (strncmp(buff, OK, strlen(OK)) != 0)
		return;

	rst_recv = recv_str(sockfd, buff, strlen(MATRICULA));
	if (rst_recv <= 0)
		return checkexit(rst_recv);

	if (strncmp(buff, MATRICULA, strlen(MATRICULA)) != 0)
		return;

	send_int(sockfd, rand_int());
}

int main(int argc, char *argv[])
{
	check_args(argc, argv);

	struct sockaddr_in servaddr;
	int sockfd = connect_serv(&servaddr, argv[1], argv[2]);

	handler(sockfd, argv[3]);
	close(sockfd);

	return 0;
}
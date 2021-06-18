#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <netinet/in.h>

#define BUFSZ 1024

void usage(int argc, char **argv){
	printf("usage: %s <server IP> <server port>", argv[0]);
	printf("example: %s 127.0.0.1 5151", argv[0]);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if(argc < 3) {
		usage(argc, argv);
	}

	/*Manipulação de endereços IPV4 e IPV6*/
	struct sockaddr_storage storage;
	if(0 != addrparse(argv[1], argv[2], &storage)){
		usage(argc, argv);
	}

	int socketIo;
	socketIo = socket(storage.ss_family, SOCK_STREAM, 0);
	
	if(socketIo == -1) {
		logexit("socket");
	}

	struct sockaddr *addr = (struct sockaddr *)(&storage);

	if(0 != connect(socketIo, addr, sizeof(storage))){
		logexit("connect");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("connect to %s\n", addrstr);

	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);

	/* Lendo a mensagem */

	printf("mensagem > ");
	fgets(buf, BUFSZ - 1, stdin);

	size_t count = send(socketIo, buf, strlen(buf) + 1, 0);
	if( count != strlen(buf) + 1){
		logexit("send");
	}

	memset(buf, 0, BUFSZ);
	unsigned total = 0;

	while (1)
	{
		count = recv(socketIo, buf + total, BUFSZ - total, 0);
		if(count == 0){
			// Conexão finalizada.
			break;
		}
		total += count;
	}
	close(socketIo);

	printf("received %u bytes\n", total);
	printf("%s", buf);

	exit(EXIT_SUCCESS);
}
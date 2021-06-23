#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <netinet/in.h>

#define BUFSZ 500

void usage(int argc, char **argv){
	printf("usage: %s <server IP> <server port>", argv[0]);
	printf("example: %s 127.0.0.1 5151", argv[0]);
	exit(EXIT_FAILURE);
}

//Recebe o endereço IP e a porta do servidor
int main(int argc, char **argv)
{
	// Numero insuficiente de argumentos
	if(argc < 3) {
		usage(argc, argv);
	}

	// Cria storage
	struct sockaddr_storage storage;
	if(0 != addrparse(argv[1], argv[2], &storage)){
		usage(argc, argv);
	}

	// Cria o socket
	int socketIo;
	socketIo = socket(storage.ss_family, SOCK_STREAM, 0);
	if(socketIo == -1) {
		logexit("socket");
	}

	// Conecta no servidor
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if(0 != connect(socketIo, addr, sizeof(storage))){
		logexit("connect");
	}

	// Log de qual servidor esta conectado
	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);
	printf("Conectado ao %s\n", addrstr);

	unsigned total = 0;

	while (1)
	{
		char buf[BUFSZ];
		// Lendo a mensagem
		printf("> ");
		fgets(buf, BUFSZ - 1, stdin);

		buf[strlen(buf) - 1] = 0;

		// Envia a mensagem
		size_t count = send(socketIo, buf, strlen(buf) + 1, 0);
		if( count != strlen(buf) + 1){
			logexit("envio_mensagem");
		}

		memset(buf, 0, BUFSZ);
		count = recv(socketIo, buf, BUFSZ, 0);
		if(count == 0) logexit("recebimento_resposta");

		if(buf[0] == 1) {
			break;
		}

		total += count;
		printf("< %s\n", buf);
	}
	close(socketIo);
	exit(EXIT_SUCCESS);
}
#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#define BUFSZ 1024

Local vaccination_coordenates[50];

double get_distancia(Local _p1, Local _p2) {
	double somaDoQuadradoDosCatetos = pow(abs(_p1.x - _p2.y), 2) + pow(abs(_p1.y - _p2.y), 2);
	return sqrt(somaDoQuadradoDosCatetos);
}

void usage(int argc, char **argv) {
	printf("usage: %s <v4|v6> <server port>", argv[0]);
	printf("example: %s v4 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

int verify_exists(int coordenate_x, int coordenate_y){
	int i = 0;
	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == coordenate_x && vaccination_coordenates[i].y == coordenate_y){
			return i;
		}
	}
	return -1;
}


void add_local_vaccination(int coordenate_x, int coordenate_y, int client_socket){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	char buf[BUFSZ];
	int i = 0;

	if(isExists == -1){
		for(i = 0; i < 50; i++) {
			if(vaccination_coordenates[i].x == 0 && vaccination_coordenates[i].y == 0){
				if(i >= 50){
					printf("limit exceeded");
					return;
				}
				vaccination_coordenates[i].x = coordenate_x;
				vaccination_coordenates[i].y = coordenate_y;
				break;
			}
		}
		sprintf(buf, "%d %d added", coordenate_x, coordenate_y);
	}else{
		sprintf(buf, "%d %d already exists", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf);
}

void remove_local_vaccination(int coordenate_x, int coordenate_y, int client_socket){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	char buf[BUFSZ];

	if(isExists != -1){
		vaccination_coordenates[isExists].x = 0;
		vaccination_coordenates[isExists].y = 0;
		sprintf(buf, "%d %d removed", coordenate_x, coordenate_y);
	}else{
		sprintf(buf, "%d %d does not exist", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf);
}

void query_local_vaccination(int coordenate_x, int coordenate_y){
	// Calcular a distancia de todos os pontos em relação ao ponto passado e salvar em um vetor
	// Escolher a menor distancia
	// Retornar a coordenada que possui a menor distancia
	int i = 0;
	int y = 0;
	int exist_local = 0;

	double distances[50];

	Local point_repassed;
	point_repassed.x = coordenate_x;
	point_repassed.y = coordenate_y;

	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == 0 && vaccination_coordenates[i].y == 0){
			distances[i] = -1;
		}
		exist_local = 1;
		Local point_saved;
		point_saved.x = vaccination_coordenates[i].x;
		point_saved.y = vaccination_coordenates[y].y;

		distances[i] = get_distancia(point_repassed, point_saved);
	}

	double short_distance = distances[0];
	int position = 0;

	for(i = 0; i < 50; i++){
		if(distances[i] == -1 ){
			return;
		}
		if (distances[i] < short_distance){ 
			 short_distance = distances[i];
			 position = i;
		}
	}

	if(exist_local == 0){
		printf("none");
	}else{
		printf("%d %d", vaccination_coordenates[position].x, vaccination_coordenates[position].y);
	}
}

void list_local_vaccination(int socket_client){
	int i = 0;
	int exist_local = 0;

	char buf[BUFSZ];

	char list_coordenates[BUFSZ] = "";
	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == 0 || vaccination_coordenates[i].y == 0){
			continue;
		}
		exist_local = 1;
		char coordenate[15] = "";
		sprintf(coordenate, " %d %d ", vaccination_coordenates[i].x, vaccination_coordenates[i].y);

		strcat(list_coordenates, coordenate);
	}

	sprintf(buf, "%s", list_coordenates);

	if(exist_local == 0){
		sprintf(buf, "none");
	}

	send_message(socket_client, buf);
}

// Recebe a porta para aguardar conexoes do cliente
int main(int argc, char *argv[])
{
	// Numero insuficiente de argumentos
	if(argc < 3) {
		usage(argc, argv);
	}

	//zera as coordenadas de vacinação
	int i;
	for(i = 0; i < 50; i++) {
		vaccination_coordenates[i].x = 0;
		vaccination_coordenates[i].y = 0;
	}

	//inicia o storage
	struct sockaddr_storage storage;
	if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
		usage(argc, argv);
	}

	//cria o socket do servidor
	int socketIo;
	socketIo = socket(storage.ss_family, SOCK_STREAM, 0);
	if(socketIo == -1) {
		logexit("socket");
	}

	//opções do socket
	int enable = 1;
	if(setsockopt(socketIo, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
		logexit("setsockopt");
	}

	// Atribui uma porta ao servidor
	struct sockaddr *addr = (struct sockaddr *)(&storage);
	if(bind(socketIo, addr, sizeof(storage)) != 0){
		logexit("bind");
	}

	// Espera conexões
	if(listen(socketIo, 10) != 0){
		logexit("listen");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("[log] Iniciado no %s, esperando a conexão...\n", addrstr);

	// Loop do funcionamento do servidor
	while(1){
		// Cria um storage e um socket para o cliente
		struct sockaddr_storage client_storage;
		struct sockaddr *caddr = (struct sockaddr *)(&client_storage);
		socklen_t caddrlen = sizeof(client_storage);

		// Aceita conexao
		int csock = accept(socketIo, caddr, &caddrlen);
		if(csock == -1){
			logexit("accept");
		}

		// Log de conexão
		char caddrstr[BUFSZ];
		addrtostr(caddr, caddrstr, BUFSZ);
		printf("[log] Conectado com %s\n", caddrstr);

		while(1){
			// Recebe a entrada
			char buf[BUFSZ];
			memset(buf, 0, 	BUFSZ);
			size_t count = recv(csock, buf, BUFSZ, 0);

			printf("\n[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

			if(strcmp(buf, "kill") == 0){
				close(csock);
				break;
			}
			
			int cont = 0;
			char function[4];
			memset(function, 0, 4);
			int coordenate_x = 0;
			int coordenate_y = 0;  

			char *token = strtok(buf," ");

			while(token != NULL) {        
				if(cont == 0) {        
					strcpy(function, token);
				} else if(cont == 1) {
					coordenate_x = atoi(token);
				} else if(cont == 2) {        
					coordenate_y = atoi(token);
				}
				cont += 1;
				token = strtok(NULL, " ");    
			}

			printf("\n[log]\nComando: %s\nCoordenada X: %d\nCoordenada Y: %d\n", function, coordenate_x, coordenate_y);

			if(strcmp(function, "list") == 0){
				list_local_vaccination(csock);
			}

			if(strcmp(function, "add") == 0){
				add_local_vaccination(coordenate_x, coordenate_y, csock);
			}
			if(strcmp(function, "remove") == 0){
				remove_local_vaccination(coordenate_x, coordenate_y, csock);
			}
			if(strcmp(function, "query") == 0){
				query_local_vaccination(coordenate_x, coordenate_y);
			}
		}
	}

	exit(EXIT_SUCCESS);

}
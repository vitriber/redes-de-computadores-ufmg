#include "common.h"

#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdbool.h>

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

void add_local_vaccination(int coordenate_x, int coordenate_y){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	int i = 0;

	if(isExists != -1){
		for(i = 0; i < 50; i++) {
			if(vaccination_coordenates[i].x != 0 && vaccination_coordenates[i].y != 0){
				if(i >= 50){
					printf("limit exceeded");
					return;
				}
				vaccination_coordenates[i].x = coordenate_x;
				vaccination_coordenates[i].y = coordenate_y;
			}
		}
		printf("%d %d added", coordenate_x, coordenate_y);
	}else{
		printf("%d %d already exists", coordenate_x, coordenate_y);
	}

}

void remove_local_vaccination(int coordenate_x, int coordenate_y){
	int isExists = verify_exists(coordenate_x, coordenate_y);

	if(isExists != -1){
		vaccination_coordenates[isExists].x = 0;
		vaccination_coordenates[isExists].y = 0;
		printf("%d %d removed", coordenate_x, coordenate_y);
	}else{
		printf("%d %d does not exist", coordenate_x, coordenate_y);
	}
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

void list_local_vaccination(){
	int i = 0;
	int exist_local = 0;
	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == 0 || vaccination_coordenates[i].y == 0){
			return;
		}
		exist_local = 1;
		printf("%d %d \n", vaccination_coordenates[i].x, vaccination_coordenates[i].y);
	}

	if(exist_local == 0){
		printf("none");
	}
}


int main(int argc, char *argv[])
{
	if(argc < 3) {
		usage(argc, argv);
	}

	int i;
	for(i = 0; i < 50; i++) {
		vaccination_coordenates[i].x == 0;
		vaccination_coordenates[i].y == 0;
	}

	struct sockaddr_storage storage;
	if(0 != server_sockaddr_init(argv[1], argv[2], &storage)){
		usage(argc, argv);
	}

	int socketIo;
	socketIo = socket(storage.ss_family, SOCK_STREAM, 0);
	
	if(socketIo == -1) {
		logexit("socket");
	}

	int enable = 1;
	if(setsockopt(socketIo, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
		logexit("setsockopt");
	}

	struct sockaddr *addr = (struct sockaddr *)(&storage);

	if(bind(socketIo, addr, sizeof(storage)) != 0){
		logexit("bind");
	}

	if(listen(socketIo, 10) != 0){
		logexit("listen");
	}

	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);

	printf("bound to %s, waiting connection\n", addrstr);

	while(1){
		struct sockaddr_storage client_storage;
		struct sockaddr *caddr = (struct sockaddr *)(&client_storage);

		socklen_t caddrlen = sizeof(client_storage);

		int csock = accept(socketIo, caddr, &caddrlen);
		if(csock == -1){
			logexit("accept");
		}

		char caddrstr[BUFSZ];
		addrtostr(caddr, caddrstr, BUFSZ);
		printf("[log] connection from %s\n", caddrstr);

		char buf[BUFSZ];
		memset(buf, 0, 	BUFSZ);

		size_t count = recv(csock, buf, BUFSZ, 0);
		printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

		char *function = strtok(buf," ");
		if(function == 'list'){
			list_local_vaccination();
		}

		char *coordenate_x = strtok(NULL, " ");
    	char *coordenate_y = strtok(NULL, " ");

		if(function == 'add'){
			add_local_vaccination((int)coordenate_x, (int)coordenate_y);
		}else if(function == 'rm'){

		}
		else if(function == 'query'){

		}

		// sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
		count = send(csock, buf, strlen(buf) + 1, 0);
		if( count != strlen(buf) + 1){
			logexit("send");
		}
		close(csock);

	}

	exit(EXIT_SUCCESS);

}
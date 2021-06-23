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

#define BUFSZ 500

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

int get_position(){
	int i;
	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == -1 && vaccination_coordenates[i].y == -1){
			return i;
		}
	}
	return - 1;
}


void add_local_vaccination(int coordenate_x, int coordenate_y, int client_socket, ssize_t numBytesRcvd){
	char buf[BUFSZ];

	int isExists = verify_exists(coordenate_x, coordenate_y);	
	int index = get_position();

	if(isExists == -1){
		if(index != -1){
			vaccination_coordenates[index].x = coordenate_x;
			vaccination_coordenates[index].y = coordenate_y;
			sprintf(buf, "%d %d added\n", coordenate_x, coordenate_y);
		}else {
			sprintf(buf, "limit exceeded\n");
		}		
	}else{
		sprintf(buf, "%d %d already exists\n", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf, numBytesRcvd);
}

void remove_local_vaccination(int coordenate_x, int coordenate_y, int client_socket, ssize_t numBytesRcvd){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	char buf[BUFSZ];

	if(isExists != -1){
		vaccination_coordenates[isExists].x = -1;
		vaccination_coordenates[isExists].y = -1;
		sprintf(buf, "%d %d removed\n", coordenate_x, coordenate_y);
	}else{
		sprintf(buf, "%d %d does not exist\n", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf, numBytesRcvd);
}

void query_local_vaccination(int coordenate_x, int coordenate_y, int client_socket, ssize_t numBytesRcvd){
	// Calcular a distancia de todos os pontos em relação ao ponto passado e salvar em um vetor
	// Escolher a menor distancia
	// Retornar a coordenada que possui a menor distancia
	int i = 0;
	int y = 0;
	int exist_local = 0;
	char buf[BUFSZ];

	double distances[50];

	Local point_repassed;
	point_repassed.x = coordenate_x;
	point_repassed.y = coordenate_y;

	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == -1 && vaccination_coordenates[i].y == -1){
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
		sprintf(buf, "none\n");
	}else{
		sprintf(buf, "%d %d\n", vaccination_coordenates[position].x, vaccination_coordenates[position].y);
	}

	short_distance = 0;
	position = 0;

	send_message(client_socket, buf, numBytesRcvd);
}

void list_local_vaccination(int socket_client, ssize_t numBytesRcvd){
	int i = 0;
	int exist_local = 0;

	char buf[BUFSZ];

	char list_coordenates[BUFSZ] = "";
	for(i = 0; i < 50; i++) {
		if(vaccination_coordenates[i].x == -1 || vaccination_coordenates[i].y == -1){
			continue;
		}
		exist_local = 1;
		char coordenate[15] = "";
		sprintf(coordenate, "%d %d \n", vaccination_coordenates[i].x, vaccination_coordenates[i].y);

		strcat(list_coordenates, coordenate);
	}

	sprintf(buf, "%s", list_coordenates);

	if(exist_local == 0){
		sprintf(buf, "none\n");
	}

	send_message(socket_client, buf, numBytesRcvd);
}

void clear_array(){
	//zera as coordenadas de vacinação
	int i;
	for(i = 0; i < 50; i++) {
		vaccination_coordenates[i].x = -1;
		vaccination_coordenates[i].y = -1;
	}
}


// Recebe a porta para aguardar conexoes do cliente
int main(int argc, char *argv[])
{
	// Numero insuficiente de argumentos
	if(argc < 3) {
		usage(argc, argv);
	}

	clear_array();

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

	// Log de inicio do servidor
	char addrstr[BUFSZ];
	addrtostr(addr, addrstr, BUFSZ);
	printf("[log] Iniciado no %s, esperando a conexão...\n", addrstr);
	int j = 0;

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

		if(j == 4){
			j = 0;
		}
		
		// Log de conexão com o cliente
		char caddrstr[BUFSZ];
		addrtostr(caddr, caddrstr, BUFSZ);
		printf("\n[log] Conectado com %s, TESTE: %d", caddrstr, j);

		j++;


		while (1) { 
			char buf[BUFSZ];
			memset(buf, 0, 	BUFSZ);

			ssize_t numBytesRcvd = recv(csock, buf, BUFSZ, 0);
			if (numBytesRcvd < 0) logexit("recv() failed");
			printf("\n-------------------------------------------------");
			printf("\n[msg] Mensagem recebida: %s", buf);

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

			// printf("\nFunção: %s Coordenada_X: %d Coordenada_Y: %d\n", function, coordenate_x, coordenate_y);

			int kill = strcmp(function, "kill");
			int list = strcmp(function, "list");
			int add = strcmp(function, "add");
			int remove = strcmp(function, "rm");
			int query = strcmp(function, "query");

			
			if((kill == 0) || (numBytesRcvd > 500) || (kill != 0 && list != 0 && add != 0 && remove != 0 && query != 0)) {
				printf("\nCliente desconectado!!!");
    			printf("\n-------------------------------------------------");
				clear_array();
				close(csock);
				break;
			}


			if( list == 0) list_local_vaccination(csock, numBytesRcvd);
			if( add == 0) add_local_vaccination(coordenate_x, coordenate_y, csock, numBytesRcvd);
			if( remove == 0) remove_local_vaccination(coordenate_x, coordenate_y, csock, numBytesRcvd);
			if( query == 0) query_local_vaccination(coordenate_x, coordenate_y, csock, numBytesRcvd);

			// Vendo se há mais dados para receber
			// numBytesRcvd = recv(csock, buf, BUFSZ, 0);
			// if (numBytesRcvd < 0) logexit("recv() failed");
		}
	}
	exit(EXIT_SUCCESS);
}
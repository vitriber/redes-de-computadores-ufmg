#include "funcoes.h"

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
#define MIN_VALUE_VALID_COORDINATE 0
#define MAX_VALUE_VALID_COORDINATE 9999
#define MAX_LOCATION_QUANTITY 50
#define MAX_BYTES 500
#define OPTION_ADD "add"
#define OPTION_REMOVE "rm"
#define OPTION_LIST "list"
#define OPTION_QUERY "query"
#define ERROR_RETURN "Method not found"

char function[5];
int coordenate_x, coordenate_y;

Local vaccination_coordenates[50];
double euclidean_distances[50];

double get_distancia(Local _p1, Local _p2) {
	double somaDoQuadradoDosCatetos = pow(abs(_p1.x - _p2.y), 2) + pow(abs(_p1.y - _p2.y), 2);
	return sqrt(somaDoQuadradoDosCatetos);
}

void usage(int argc, char **argv) {
	printf("usage: %s <v4|v6> <server port>", argv[0]);
	printf("example: %s v4 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

int verify_exists(){
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

int get_size_locations() {
    int i;    
    int cont = 0;
    for(i = 0; i < MAX_LOCATION_QUANTITY; i++) {
        if(vaccination_coordenates[i].x != -1 && vaccination_coordenates[i].y != -1){
            cont += 1;
        }
    }
    return cont;
}


void add_local_vaccination(int client_socket){
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

	send_message(client_socket, buf);
}

void remove_local_vaccination(int client_socket){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	char buf[BUFSZ];

	if(isExists != -1){
		vaccination_coordenates[isExists].x = -1;
		vaccination_coordenates[isExists].y = -1;

		//Regrouping list
		int i;    
		for(i = isExists + 1; i < MAX_LOCATION_QUANTITY; i++) {
			vaccination_coordenates[i-1].x = vaccination_coordenates[i].x;
			vaccination_coordenates[i-1].y = vaccination_coordenates[i].y;        
		}

		//Removing last position
		vaccination_coordenates[MAX_LOCATION_QUANTITY - 1].x = -1;
		vaccination_coordenates[MAX_LOCATION_QUANTITY - 1].y = -1;

		sprintf(buf, "%d %d removed\n", coordenate_x, coordenate_y);
	}else{
		sprintf(buf, "%d %d does not exist\n", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf);
}

void query_local_vaccination(int client_socket) {
    double min_distance = 9999;
	char buf[BUFSZ];
    int index = -1;
    
    if(get_size_locations() == 0) {
        sprintf(buf,"none\n");        
    }     

    int i;        
    for(i = 0; i < MAX_LOCATION_QUANTITY; i++) {      
        if(vaccination_coordenates[i].x != -1 && vaccination_coordenates[i].y != -1) {  

            double x_value = (double)vaccination_coordenates[i].x - coordenate_x;       
            double y_value = (double)vaccination_coordenates[i].y - coordenate_y;
            
            double distance = sqrt(pow(x_value, 2) + pow(y_value, 2));
            if(distance >= 0){
                euclidean_distances[i] = distance;
            }            
        }
    }

    //Getting min value
    for(i = 0; i < MAX_LOCATION_QUANTITY; i++) {
        if(euclidean_distances[i] >= 0) {
            if(euclidean_distances[i] < min_distance){
                min_distance = euclidean_distances[i];
                index = i;
            }
        }
    }

    if(index == -1) {
        logexit("query");
    }

    sprintf(buf,"%d %d\n", vaccination_coordenates[index].x, vaccination_coordenates[index].y); 

	send_message(client_socket, buf);      
}

void list_local_vaccination(int socket_client){
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
		sprintf(coordenate, "%d %d ", vaccination_coordenates[i].x, vaccination_coordenates[i].y);

		strcat(list_coordenates, coordenate);
	}

	list_coordenates[strlen(list_coordenates)-1] = 0;

	sprintf(buf, "%s", list_coordenates);
	strcat(buf,"\n");

	if(exist_local == 0){
		sprintf(buf, "none\n");
	}

	send_message(socket_client, buf);
}

void clear_array(){
	//zera as coordenadas de vacinação
	int i;
	for(i = 0; i < 50; i++) {
		vaccination_coordenates[i].x = -1;
		vaccination_coordenates[i].y = -1;
		euclidean_distances[i] = 9999;
	}
}

int check_valid_command(char *buf) {
    char * token = strtok(buf, " ");
    int cont = 0;    

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

    int optionAdd = strcmp(function, OPTION_ADD) == 0;
    int optionRm = strcmp(function, OPTION_REMOVE) == 0;
    int optionQuery = strcmp(function, OPTION_QUERY) == 0;
    int optionList = strcmp(function, OPTION_LIST) == 0;    

    int functionWithParameters = (optionAdd || optionRm || optionQuery) && cont == 3;
    int functionWithoutParameters = optionList && cont == 1;    
    if(!functionWithParameters && !functionWithoutParameters) {             
        return -1;
    }

    int xValid = coordenate_x >= MIN_VALUE_VALID_COORDINATE && coordenate_x <= MAX_VALUE_VALID_COORDINATE;
    int yValid = coordenate_y >= MIN_VALUE_VALID_COORDINATE && coordenate_y <= MAX_VALUE_VALID_COORDINATE;
    if(functionWithParameters && (!xValid || !yValid)) {        
        return -1;
    }    

    return 0;
}

void select_command(char *buf, int cliet_socket) {
    if(strcmp(function, OPTION_ADD) == 0) {
        add_local_vaccination(cliet_socket);        
    } else if(strcmp(function, OPTION_REMOVE) == 0) {
        remove_local_vaccination(cliet_socket);
    } else if(strcmp(function, OPTION_QUERY) == 0) {
        query_local_vaccination(cliet_socket);
    } else if (strcmp(function, OPTION_LIST) == 0) {
        list_local_vaccination(cliet_socket);
    } else {
        logexit("option");
    }    
}

void format_message(char *buf){
    int i=0;    
    while(i <= strlen(buf)) {        
        if(buf[i] == '\n') {
            buf[i] = 0;
        }        
        i++;
    }    
}

void check_incomplete_message(char *buf, int count, int csock) {
    int hasCompleteMessage = 0;
    while (hasCompleteMessage == 0) {
        int i;
        for(i = 0; i < strlen(buf); i++)
        {                    
            if(buf[i] == '\n'){                        
                hasCompleteMessage = 1;
                break;
            }
        }  

        if(hasCompleteMessage == 0) {            
            char buf_aux[BUFSZ];
            memset(buf_aux, 0, BUFSZ);
            count += recv(csock, buf_aux, BUFSZ, 0);
            strcat(buf, buf_aux);
            strcat(buf,"\n");            
        }
    } 

}

int continue_command(char *buf, int count, int csock) {    
    format_message(buf);    
    
    if ((int)count > 500) {         
        close(csock); //Terminating program execution if bytes greater than 500
        return -1;
    }            
    
    if (strcmp(buf, "kill") == 0) {
        clear_array();
        close(csock); //Terminating program execution if client sends kill command 
        return -1;
    }

    if(check_valid_command(buf) != 0 ) {        
        close(csock); //Terminating program execution if invalid request
        return -1;
    }

    select_command(buf, csock);              

    // count = send(csock, buf, strlen(buf), 0);
    // if (count != strlen(buf)) {
    //     logexit("send");
    // }    

    return 0;
}

// Recebe a porta para aguardar conexoes do cliente
int main(int argc, char *argv[])
{
	// Numero insuficiente de argumentos
	if(argc < 3) {
		usage(argc, argv);
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

	clear_array();

	// Log de inicio do servidor
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

		// Log de conexão com o cliente
		char caddrstr[BUFSZ];
		addrtostr(caddr, caddrstr, BUFSZ);
		printf("\n[log] Conectado com %s\n", caddrstr);

		while (1) { 
			char buf[BUFSZ];
			memset(buf, 0, 	BUFSZ);
			ssize_t numBytesRcvd = recv(csock, buf, BUFSZ, 0);

			// Variavel auxiliar pra a manipulação do buffer
			char buf_aux[BUFSZ]; 
            memset(buf_aux, 0, BUFSZ);
            strcpy(buf_aux, buf);

			char *token = strtok(buf_aux,"\n");
			int c_command = 0;
			char commands[BUFSZ][BUFSZ];
			memset(commands, 0, BUFSZ*BUFSZ);			

			while(token != NULL) {              
				strcpy(commands[c_command], token);
				c_command += 1;
				token = strtok(NULL, "\n");    
			}

			if(c_command > 1) {
                int i;
                for (i = 0; i < c_command; i++) {                    
                    if(continue_command(commands[i], numBytesRcvd, csock) != 0) {
                        break;
                    }
                }
            } else {                
                check_incomplete_message(buf, numBytesRcvd, csock);
                if(continue_command(buf, numBytesRcvd, csock) != 0) {                    
                    break;
                }
            }
		}
	}
	exit(EXIT_SUCCESS);
}
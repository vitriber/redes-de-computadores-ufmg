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
#define MIN_VALUE_COORDINATE 0
#define MAX_VALUE_COORDINATE 9999
#define MAX_LOCATION 50
#define O_ADD "add"
#define O_REMOVE "rm"
#define O_LIST "list"
#define O_QUERY "query"

Local vaccination_coordenates[MAX_LOCATION];
double distances_e[MAX_LOCATION];
char function[5];
int coordenate_x, coordenate_y;

// Mensagem caso passe parametros inválidos
void usage(int argc, char **argv) {
	printf("usage: %s <v4|v6> <server port>", argv[0]);
	printf("example: %s v4 51511\n", argv[0]);
	exit(EXIT_FAILURE);
}

// Verifica se existe a coordenada
int verify_exists(){
	int i = 0;
	for(i = 0; i < MAX_LOCATION; i++) {
		if(vaccination_coordenates[i].x == coordenate_x && vaccination_coordenates[i].y == coordenate_y){
			return i;
		}
	}
	return -1;
}

// Encontra uma posição valida
int get_position(){
	int i;
	for(i = 0; i < MAX_LOCATION; i++) {
		if(vaccination_coordenates[i].x == -1 && vaccination_coordenates[i].y == -1){
			return i;
		}
	}
	return - 1;
}

// Verifica se existe alguma coordenda
int get_size_locations() {
    int i;    
    int cont = 0;
    for(i = 0; i < MAX_LOCATION; i++) {
        if(vaccination_coordenates[i].x != -1 && vaccination_coordenates[i].y != -1){
            cont += 1;
        }
    }
    return cont;
}

// Adiciona um local de vacinação
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

// Remove um local de vacinação
void remove_local_vaccination(int client_socket){
	int isExists = verify_exists(coordenate_x, coordenate_y);
	char buf[BUFSZ];

	if(isExists != -1){
		vaccination_coordenates[isExists].x = -1;
		vaccination_coordenates[isExists].y = -1;

		int i;    
		for(i = isExists + 1; i < MAX_LOCATION; i++) {
			vaccination_coordenates[i-1].x = vaccination_coordenates[i].x;
			vaccination_coordenates[i-1].y = vaccination_coordenates[i].y;        
		}

		vaccination_coordenates[MAX_LOCATION - 1].x = -1;
		vaccination_coordenates[MAX_LOCATION - 1].y = -1;

		sprintf(buf, "%d %d removed\n", coordenate_x, coordenate_y);
	}else{
		sprintf(buf, "%d %d does not exist\n", coordenate_x, coordenate_y);
	}

	send_message(client_socket, buf);
}

// Procura o local de vacinação mais próximo
void query_local_vaccination(int client_socket) {
    double min_distance = 9999;
	char buf[BUFSZ];
    int index = -1;
    
    if(get_size_locations() == 0) {
        sprintf(buf,"none\n");        
    }     

    int i;        
    for(i = 0; i < MAX_LOCATION; i++) {      
        if(vaccination_coordenates[i].x != -1 && vaccination_coordenates[i].y != -1) {  

            double x_value = (double)vaccination_coordenates[i].x - coordenate_x;       
            double y_value = (double)vaccination_coordenates[i].y - coordenate_y;
            
            double distance = sqrt(pow(x_value, 2) + pow(y_value, 2));
            if(distance >= 0){
                distances_e[i] = distance;
            }            
        }
    }

    for(i = 0; i < MAX_LOCATION; i++) {
        if(distances_e[i] >= 0) {
            if(distances_e[i] < min_distance){
                min_distance = distances_e[i];
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

// Lista os locais de vacinação cadastrados
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

//zera as coordenadas de vacinação
void clear_array(){
	int i;
	for(i = 0; i < 50; i++) {
		vaccination_coordenates[i].x = -1;
		vaccination_coordenates[i].y = -1;
		distances_e[i] = 9999;
	}
}

// Verifica se o comando enviado é valido
int v_valid_command(char *buf) {
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

    int add = strcmp(function, O_ADD) == 0;
    int remove = strcmp(function, O_REMOVE) == 0;
    int query = strcmp(function, O_QUERY) == 0;
    int list = strcmp(function, O_LIST) == 0;    

    int functionParam = (add || remove || query) && cont == 3;
    int functionWParam = list && cont == 1;    
    if(!functionParam && !functionWParam) {             
        return -1;
    }

    int xV = coordenate_x >= MIN_VALUE_COORDINATE &&
	 coordenate_x <= MAX_VALUE_COORDINATE;
    int yV = coordenate_y >= MIN_VALUE_COORDINATE &&
	 coordenate_y <= MAX_VALUE_COORDINATE;
    if(functionParam && (!xV || !yV)) {        
        return -1;
    }    

    return 0;
}

// Aciona as funções de acordo com o comando
void function_select(int cliet_socket) {
    if(strcmp(function, O_ADD) == 0) {
        add_local_vaccination(cliet_socket);        
    } else if(strcmp(function, O_REMOVE) == 0) {
        remove_local_vaccination(cliet_socket);
    } else if(strcmp(function, O_QUERY) == 0) {
        query_local_vaccination(cliet_socket);
    } else if (strcmp(function, O_LIST) == 0) {
        list_local_vaccination(cliet_socket);
    } else {
        logexit("option");
    }    
}

// Formata a mensagem recebida
void f_message(char *buf){
    int i=0;    
    while(i <= strlen(buf)) {        
        if(buf[i] == '\n') {
            buf[i] = 0;
        }        
        i++;
    }    
}

// Verifica uma mensagem imcompleta
void v_message_incomplete(char *buf, int count, int csock) {
    int hasComMessage = 0;
    while (hasComMessage == 0) {
        int i;
        for(i = 0; i < strlen(buf); i++)
        {                    
            if(buf[i] == '\n'){                        
                hasComMessage = 1;
                break;
            }
        }  

        if(hasComMessage == 0) {            
            char buf_aux[BUFSZ];
            memset(buf_aux, 0, BUFSZ);
            count += recv(csock, buf_aux, BUFSZ, 0);
            strcat(buf, buf_aux);
            strcat(buf,"\n");            
        }
    } 

}

// Verifica se o comando é válido antes de selecionar a ação
int command_invite(char *buf, int count, int csock) {    
    f_message(buf);    
    
    if ((int)count > 500) {         
        close(csock); 
        return -1;
    }            
    
    if (strcmp(buf, "kill") == 0) {
        clear_array();
        close(csock); 
        return -1;
    }

    if(v_valid_command(buf) != 0 ) {        
        close(csock);
        return -1;
    }

    function_select(csock);              

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
                    if(command_invite(commands[i], numBytesRcvd, csock) != 0) {
                        break;
                    }
                }
            } else {                
                v_message_incomplete(buf, numBytesRcvd, csock);
                if(command_invite(buf, numBytesRcvd, csock) != 0) {                    
                    break;
                }
            }
		}
	}
	exit(EXIT_SUCCESS);
}
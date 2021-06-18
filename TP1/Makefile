all:
	gcc -g -Wall -c common.c
	gcc -g -Wall cliente.c common.o -o cliente
	gcc -g -Wall servidor.c common.o -o servidor

clean:
	rm common.o cliente.o servidor.o
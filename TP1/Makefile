all:
	gcc -g -Wall -c common.c
	gcc -g -Wall cliente.c common.o -o cliente
	gcc -g -Wall servidor.c common.o -o servidor -lm

clean:
	rm common.o cliente servidor
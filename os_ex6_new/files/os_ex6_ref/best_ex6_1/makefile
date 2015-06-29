all: http_client.c http_server.c my_lib.h
	gcc -g http_client.c -o http_client.o
	gcc -g http_server.c -o http_server.o -pthread

clean:
	rm http_server.o http_client.o

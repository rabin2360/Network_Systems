CC = gcc
EXE1 = client
EXE2 = server

all: client server

CLEAN = rm -f $(EXE1) $(EXE2) *.o

TARGET := $(SRC:.c=)

#target
client: udp_client.c md5.h
	gcc -o $(EXE1) udp_client.c md5.c -lcrypto

server: udp_server.c md5.h
	gcc -o $(EXE2) udp_server.c md5.c -lcrypto

clean:
	$(CLEAN)

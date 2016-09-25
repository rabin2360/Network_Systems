CC = gcc
EXE1 = client
EXE2 = server

all: client server

CLEAN = rm -f $(EXE1) $(EXE2) *.o

TARGET := $(SRC:.c=)

#target
client: client.c md5.h
	gcc -o $(EXE1) client.c md5.c -lcrypto

server: server.c md5.h
	gcc -o $(EXE2) server.c md5.c -lcrypto

clean:
	$(CLEAN)

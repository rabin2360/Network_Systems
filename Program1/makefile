CC = gcc
EXE1 = client
EXE2 = server

all: clientObj serverObj

CLEAN = rm -f $(EXE1) $(EXE2) *.o

#flags
CFLAGS = -03 -Wall

#target
clientObj: udp_client.c #md5.h
	gcc -o $(EXE1) udp_client.c #md5.c
serverObj: udp_server.c #md5.h
	gcc -o $(EXE2) udp_server.c #md5.c

clean:
	$(CLEAN)

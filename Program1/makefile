CC = gcc
EXE1 = client
EXE2 = server

all: clientObj serverObj

CLEAN = rm -f $(EXE1) $(EXE2) *.o

#flags
CFLAGS = -03 -Wall

#target
clientObj: udp_client.c
	gcc -o $(EXE1) udp_client.c
serverObj: udp_server.c
	gcc -o $(EXE2) udp_server.c

clean:
	$(CLEAN)

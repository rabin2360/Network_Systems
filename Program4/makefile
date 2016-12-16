CC = gcc
EXE1 = webproxy

all: webproxy

CLEAN: rm -f *.o

TARGET := $(SRC:.c=)

webproxy: webProxy.c
	$(CC) -o $(EXE1) webProxy.c -lcurl -lpthread -lcrypto

clean:
	$(CLEAN)

.PHONY: clean
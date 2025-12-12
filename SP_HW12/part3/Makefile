CC = gcc
CFLAGS = -Wall -g

CLIENT = udp_client
SERVER = udp_server

all: $(CLIENT) $(SERVER)

$(CLIENT): main.c lookup9.c dict.h
	$(CC) $(CFLAGS) -o $(CLIENT) main.c lookup9.c

$(SERVER): udp_server.c lookup5.c dict.h
	$(CC) $(CFLAGS) -o $(SERVER) udp_server.c lookup5.c

clean:
	rm -f $(CLIENT) $(SERVER) *.o
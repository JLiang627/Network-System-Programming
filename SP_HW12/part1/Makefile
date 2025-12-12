# 編譯器與旗標設定
CC = gcc
CFLAGS = -Wall -std=c99 -g

# 執行檔名稱
CLIENT = usock_client
SERVER = usock_server
RM = rm -f

# 依賴的目標檔
CLIENT_OBJS = main.o lookup7.o
SERVER_OBJS = usock_server.o lookup5.o

# 預設目標：編譯所有需要的執行檔
all: $(CLIENT) $(SERVER)

# 清理目標
clean:
	$(RM) $(CLIENT) $(SERVER) *.o *~ core TUNNEL

# --------------------------------------------------------------------------
# Client 端的編譯規則 (usock_client)
# 連結 main.o 和 lookup7.o
# --------------------------------------------------------------------------
$(CLIENT): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $(CLIENT_OBJS) $(LDFLAGS)

main.o: main.c dict.h
	$(CC) $(CFLAGS) -c $<

lookup7.o: lookup7.c dict.h
	$(CC) $(CFLAGS) -c $<

# --------------------------------------------------------------------------
# Server 端的編譯規則 (usock_server)
# 連結 usock_server.o 和 lookup5.o
# --------------------------------------------------------------------------
$(SERVER): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $@ $(SERVER_OBJS) $(LDFLAGS)

usock_server.o: usock_server.c dict.h
	$(CC) $(CFLAGS) -c $<

lookup5.o: lookup5.c dict.h
	$(CC) $(CFLAGS) -c $<


# 額外目標：單獨編譯
usock_client: $(CLIENT)
usock_server: $(SERVER)
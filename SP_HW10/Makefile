# 執行檔名稱
SERVER_EXEC = fifo_seqnum_server
CLIENT_EXEC = fifo_seqnum_client
VERIFY_EXEC = fifo_nonblocking

# 所有的執行檔 (新增 VERIFY_EXEC)
ALL_TARGETS = $(SERVER_EXEC) $(CLIENT_EXEC) $(VERIFY_EXEC)

# 編譯器與旗標
CC = gcc
# -I. 包含當前目錄的頭文件
# -g 產生調試信息
# -Wall 開啟所有警告
CFLAGS = -I. -g -Wall

# 鏈接器旗標 (用於數學庫等，本例可能需要 -lrt, 但這裡先省略)
LDLIBS = 

# 共同的輔助原始檔 (Obj 檔)
COMM_OBJS = error_functions.o get_num.o

.PHONY: all clean verify

# 預設目標：編譯所有執行檔
all: $(ALL_TARGETS)

# --- 伺服器編譯規則 ---
$(SERVER_EXEC): fifo_seqnum_server.o $(COMM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# --- 客戶端編譯規則 ---
$(CLIENT_EXEC): fifo_seqnum_client.o $(COMM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# --- 驗證程式編譯規則 (新增) ---
$(VERIFY_EXEC): fifo_nonblocking.o $(COMM_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

# --- 通用 .c 到 .o 的規則 ---
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 額外依賴關係：確保頭文件變動時重新編譯
error_functions.o: error_functions.c error_functions.h tlpi_hdr.h
get_num.o: get_num.c get_num.h tlpi_hdr.h error_functions.h

fifo_seqnum_server.o: fifo_seqnum_server.c fifo_seqnum.h tlpi_hdr.h error_functions.h get_num.h
fifo_seqnum_client.o: fifo_seqnum_client.c fifo_seqnum.h tlpi_hdr.h error_functions.h get_num.h
# 驗證程式的依賴 (新增)
fifo_nonblocking.o: fifo_nonblocking.c tlpi_hdr.h error_functions.h

# 清理目標
clean:
	rm -f $(ALL_TARGETS) *.o
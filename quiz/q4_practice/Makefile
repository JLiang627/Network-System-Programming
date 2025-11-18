# 編譯器設定
CC = gcc

# 編譯旗標：
# -Wall: 開啟所有警告 
# -g:    加入除錯資訊 (方便用 gdb 除錯)
CFLAGS = -Wall -g

# 連結旗標：
# -pthread:  (mutex, broadcast) 需要 POSIX 執行緒庫
# -lrt:      (pos_shm) 需要 Real-time 庫 (用於 shared memory)
LDFLAGS = -pthread -lrt

# 自動抓取目錄下所有的 .c 原始碼檔案
SRCS = $(wildcard *.c)

# 自動產生對應的執行檔名稱 (去掉 .c 副檔名)
# (例如: m9_listener.c -> m9_listener)
TARGETS = $(SRCS:.c=)

# --- 規則區 ---

# 預設目標：只要輸入 'make' 就會編譯所有程式
all: $(TARGETS)

# 通用規則：告訴 make 如何把 .c 變成執行檔
# $@ 代表目標檔案名稱 (target)
# $< 代表第一個依賴檔案 (source .c)
%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

# 清除規則：輸入 'make clean' 會刪除所有編譯出來的執行檔
clean:
	rm -f $(TARGETS)

# 宣告偽目標，避免資料夾中有同名檔案造成衝突
.PHONY: all clean
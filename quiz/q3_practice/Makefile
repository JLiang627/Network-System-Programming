# 編譯器
CC = gcc

# 編譯旗標 (啟用所有警告、設定為錯誤、加入除錯資訊)
CFLAGS = -Wall -Werror -g

# 連結器旗標 (最重要：連結 pthread 函式庫)
LDFLAGS = -pthread

# 所有的 .c 原始碼檔案
SRCS = mutex_increm.c mutex_stack.c mutex_trylock.c sema_binary.c sema_count.c sema_signal.c pro_con_thread.c

# 根據 .c 檔案自動產生對應的執行檔名稱
# (例如：mutex_increm.c -> mutex_increm)
TARGETS = $(SRCS:.c=)

# 預設目標：編譯所有程式
# .PHONY 告訴 make，'all' 不是一個真正的檔案
.PHONY: all
all: $(TARGETS)

# 通用規則：如何從 .c 檔案編譯出執行檔
# $< 代表第一個依賴項 (%.c)
# $@ 代表目標檔案 (%)
%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)
	@echo "編譯完成: $@"

# 清除目標：刪除所有編譯出來的執行檔
# .PHONY 告訴 make，'clean' 也不是一個真正的檔案
.PHONY: clean
clean:
	@echo "清除所有執行檔..."
	rm -f $(TARGETS)
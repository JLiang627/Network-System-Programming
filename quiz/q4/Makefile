# 編譯器設定
CC = gcc

CFLAGS = -Wall -g -pthread -lrt

TARGETS = solution_sema solution_mutex

# 預設動作
all: $(TARGETS)

# 通用編譯規則
%: %.c
	$(CC) $(CFLAGS) -o $@ $<

# 清除指令
clean:
	rm -f $(TARGETS)
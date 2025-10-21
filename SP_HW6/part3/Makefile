# 預設目標：編譯 convert 和 file_lookup
all: convert file_lookup

# Part 3.1: convert
convert: convert.c dict.h
	gcc -Wall -o convert convert.c

# Part 3.2: file_lookup
# main.c 和 lookup1.c 連結在一起
file_lookup: main.c lookup1.c dict.h
	gcc -Wall -o file_lookup main.c lookup1.c

# 清除
clean:
	rm -f convert file_lookup *.o myfixrec

.PHONY: all clean
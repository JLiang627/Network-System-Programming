CC = gcc
CFLAGS = -Wall

# 最終目標
all: myshell

# 連結 (Link)
myshell: shell.o parse.o run_command.o builtin.o is_background.o redirect_in.o redirect_out.o pipe_present.o pipe_command.o
	$(CC) $(CFLAGS) -o myshell shell.o parse.o run_command.o builtin.o is_background.o redirect_in.o redirect_out.o pipe_present.o pipe_command.o

# 編譯 (Compile)
shell.o: shell.c shell.h
	$(CC) $(CFLAGS) -c shell.c

parse.o: parse.c shell.h
	$(CC) $(CFLAGS) -c parse.c

run_command.o: run_command.c shell.h
	$(CC) $(CFLAGS) -c run_command.c

builtin.o: builtin.c shell.h
	$(CC) $(CFLAGS) -c builtin.c

is_background.o: is_background.c shell.h
	$(CC) $(CFLAGS) -c is_background.c

redirect_in.o: redirect_in.c shell.h
	$(CC) $(CFLAGS) -c redirect_in.c

redirect_out.o: redirect_out.c shell.h
	$(CC) $(CFLAGS) -c redirect_out.c

pipe_present.o: pipe_present.c shell.h
	$(CC) $(CFLAGS) -c pipe_present.c

pipe_command.o: pipe_command.c shell.h
	$(CC) $(CFLAGS) -Wno-infinite-recursion -c pipe_command.c
# 清除
clean:
	rm -f myshell *.o

.PHONY: all clean
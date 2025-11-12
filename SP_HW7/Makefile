CC = gcc
CFLAGS = -Wall -Wextra -g -std=gnu99

SRCS = toUpp.c popen_pclose.c

OBJS = $(SRCS:.c=.o)

TARGET = toUpp

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(OBJS)
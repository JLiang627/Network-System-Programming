CC = gcc
CFLAGS = -Wall -g
TARGET = data_aggregator
SRC = data_aggregator.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
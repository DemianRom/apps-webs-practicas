CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
LDFLAGS = -pthread
TARGET = servidor

OBJS = servidor.o cJSON.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

servidor.o: servidor.c cJSON.h
	$(CC) $(CFLAGS) -c servidor.c

cJSON.o: cJSON.c cJSON.h
	$(CC) $(CFLAGS) -c cJSON.c

clean:
	rm -f $(OBJS) $(TARGET)

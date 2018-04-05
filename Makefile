TARGET = demo

SRCS = fiber.c demo.c

OBJS = $(SRCS:.c=.o) asm.o

CC = gcc
CFLAGS = -std=c11 -Og -g -Wall
LDFLAGS =

.PHONY: build run $(TARGET) clean

build: $(TARGET)

run: $(TARGET)
	./$(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean:
	- rm *.o $(TARGET)

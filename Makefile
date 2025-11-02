CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -Iinclude
LDFLAGS := -lpthread

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)
TARGET := chash

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

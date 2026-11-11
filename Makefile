CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude
SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:.c=.o)

all: csh

csh: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

src/%.o: src/%.c include/shell.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) csh

.PHONY: all clean

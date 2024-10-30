CC = gcc
CFLAGS = -Wall
SRCS = ver1.c ver2.c ver3.c ver4.c
TARGETS = $(SRCS:.c=)

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(TARGETS)

.PHONY: all clean
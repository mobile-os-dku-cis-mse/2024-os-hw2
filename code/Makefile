# Makefile

CC = gcc
CFLAGS = -pthread -Wall -Wextra
TARGETS = prod_cons multi_cons word_count_cons

all: $(TARGETS)

prod_cons: prod_cons.c
	$(CC) $(CFLAGS) -o prod_cons prod_cons.c

multi_cons: multi_cons.c
	$(CC) $(CFLAGS) -o multi_cons multi_cons.c

word_count_cons: word_count_cons.c
	$(CC) $(CFLAGS) -o word_count_cons word_count_cons.c

clean:
	rm -f $(TARGETS)

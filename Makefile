CC = gcc

CFLAGS = -Wall

TARGET = app

SRCS = prod_cons.c functions.c

OBJS = prod_cons.o functions.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	rm *.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm $(TARGET)
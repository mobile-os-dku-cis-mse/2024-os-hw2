CC = gcc

CFLAGS = -Wall -pthread

TARGET = app

SRCS = prod_cons.c functions.c

OBJS = prod_cons.o functions.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	rm *.o

clean:
	rm $(TARGET)
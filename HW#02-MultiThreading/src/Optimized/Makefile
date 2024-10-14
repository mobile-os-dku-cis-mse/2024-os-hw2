CC = g++ -std=c++20 -pthread -O2
OBJS = minhyuk_prod_cons.o main.o
TARGET = word_count

clean:
	rm -f $(OBJS)
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

minhyuk_prod_cons.o: minhyuk_prod_cons.cpp word_count.h
	$(CC) -c minhyuk_prod_cons.cpp

main.o: word_count.h main.cpp
	$(CC) -c main.cpp
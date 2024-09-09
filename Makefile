## Variable to be modified
rm = rm -rf

TARGET = threading
TEST_TARGET = threading_test

CFLAGS = -Wextra -Wall -Iinclude
LDFLAGS = -lcheck -lm -lpthread

SRC = ./src/main.c \
	./src/buffer.c \
	./src/producer.c \
	./src/consumer.c

OBJ = $(SRC:.c=.o)

TEST_SRC = ./src/main.c \
           	./src/buffer.c \
           	./src/producer.c \
           	./src/consumer.c

TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(TARGET)

%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $< \
	&& printf "[\033[1;35mcompiled\033[0m] % 29s\n" $< | tr ' ' '.' \
	|| printf "[\033[1;31merror\033[0m] % 29s\n" $< | tr ' ' '.'

$(TARGET): $(OBJ)
	@$(CC) -o $@ $^
	@echo -e "\e[1;36mFinished compiling $@\e[0m"

$(TEST_TARGET): $(TEST_OBJ)
	@$(CC) -o $@ $(TEST_OBJ) $(LDFLAGS)
	@echo -e "\e[1;36mFinished compiling $@\e[0m"

.PHONY: all fclean clean re test run_test

debug: CFLAGS += -g3
debug: clean all

clean:
	@$(rm) $(OBJ) $(TEST_OBJ)
	@echo "Removed .o files"

fclean: clean
	@$(rm) $(TARGET) $(TEST_TARGET)
	@echo "Removed binary files"

re: fclean
	@$(MAKE) all

# Build and run tests
test: $(TEST_TARGET)

run_test: test
	@./$(TEST_TARGET)

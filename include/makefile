##
## EPITECH PROJECT, 2023
## Zappy
## File description:
## Makefile
##

SRC =	parser.c \
		process.c \
		sighandlers.c \
		word_arrays.c \
		custom_commands.c \

SRC_MAIN =	main.c

SRC_TEST =

SRC_RELEASE	= $(SRC) $(SRC_MAIN)
SRC_DEBUG	= $(SRC) $(SRC_MAIN)

OBJ_RELEASE	= $(SRC_RELEASE:%.c=obj/release/%.o)
OBJ_DEBUG	= $(SRC_DEBUG:%.c=obj/debug/%.o)
OBJ_TESTS	= $(SRC:%.c=obj/cov/%.o) $(SRC_TEST:%.c=obj/tests/%.o)
OBJ_FTESTS	= $(SRC_DEBUG:%.c=obj/cov/%.o)

NAME_RELEASE= SiSH
NAME_DEBUG	= SiSH_debug
NAME_TESTS	= unit_tests
NAME_FTESTS	= SiSH_ftests

CC			=	gcc
CC_RELEASE	=	$(CC)
CC_DEBUG	=	clang
CC_TESTS	=	$(CC)
CC_FTESTS	=	$(CC)

CPPFLAGS	+=	-iquote include -D_GNU_SOURCE
CFLAGS		+=	-Wall -Wextra

CFLAGS_RELEASE	=	$(CFLAGS) -O2
CFLAGS_DEBUG	=	$(CFLAGS) -g3 -fsanitize=address
CFLAGS_TESTS	=	$(CFLAGS) --coverage
CFLAGS_FTESTS	=	$(CFLAGS) --coverage

LDFLAGS		+=
LDLIBS		+=

LDFLAGS_RELEASE	= $(LDFLAGS)
LDFLAGS_DEBUG	= $(LDFLAGS) -fsanitize=address
LDFLAGS_TESTS	= $(LDFLAGS) --coverage
LDFLAGS_FTESTS	= $(LDFLAGS) --coverage

LDLIBS_RELEASE	= $(LDLIBS)
LDLIBS_DEBUG	= $(LDLIBS)
LDLIBS_TESTS	= $(LDLIBS) -lcriterion
LDLIBS_FTESTS	= $(LDLIBS)

all:	$(NAME_RELEASE)
debug:	$(NAME_DEBUG)

obj/release/%.o:	src/%.c
	@mkdir -p $(@D)
	$(CC_RELEASE) -c $(CFLAGS_RELEASE) $(CPPFLAGS) $< -o $@

obj/debug/%.o:	src/%.c
	@mkdir -p $(@D)
	$(CC_DEBUG) -c $(CFLAGS_DEBUG) $(CPPFLAGS) $< -o $@

obj/cov/%.o:	src/%.c
	@mkdir -p $(@D)
	$(CC_TESTS) -c $(CFLAGS_TESTS) $(CPPFLAGS) $< -o $@

obj/tests/%.o:	../tests/server/%.c
	@mkdir -p $(@D)
	$(CC_TESTS) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(NAME_RELEASE): $(OBJ_RELEASE)
	$(CC_RELEASE) $(LDFLAGS_RELEASE) $(LDLIBS_RELEASE) -o $@ $(OBJ_RELEASE)

$(NAME_DEBUG): $(OBJ_DEBUG)
	$(CC_DEBUG) $(LDFLAGS_DEBUG) $(LDLIBS_DEBUG) -o $@ $(OBJ_DEBUG)

$(NAME_TESTS): $(OBJ_TESTS)
	$(CC_TESTS) $(LDFLAGS_TESTS) $(LDLIBS_TESTS) -o $@ $(OBJ_TESTS)

$(NAME_FTESTS): $(OBJ_FTESTS)
	$(CC_FTESTS) $(LDFLAGS_FTESTS) $(LDLIBS_FTESTS) -o $@ $(OBJ_FTESTS)

utests_run: clean_cov $(NAME_TESTS)
	./$(NAME_TESTS)

ftests_run: clean_cov $(NAME_FTESTS)
	$(MAKE) -C ../tests/functional/

tests_run: utests_run ftests_run
	gcovr
	gcovr -b

clean_cov:
	find . -name "*.gcda" -delete

clean: clean_cov
	find . -name "*.gcno" -delete
	$(RM) -r ./obj

fclean: clean
	$(RM) $(NAME_RELEASE) $(NAME_DEBUG) $(NAME_TESTS)

re: fclean
	$(MAKE) all

redebug: fclean
	$(MAKE) debug

.PHONY: all clean clean_cov fclean re  debug redebug tests_run

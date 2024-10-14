NAME	=	prod_cons

SRCDIR	=	src

SRC		=	$(SRCDIR)/main.cpp				\

OBJ		=	$(SRC:.cpp=.o)

CFLAGS	=	-W -Wall -Wextra -Werror -lpthread

all:	$(NAME)

$(NAME):	$(OBJ)
		g++ -o $(NAME) $(OBJ)

clean:
		rm -f $(OBJ)

fclean:	clean
		rm -f $(NAME)

re:	fclean all

debug:	CFLAGS += -g
debug:	re
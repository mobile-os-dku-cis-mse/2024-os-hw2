/*
** EPITECH PROJECT, 2022
** rip
** File description:
** ma fonction
*/

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

void print_board(char **board, char delimiter)
{
    for (size_t i = 0; board[i] != NULL; i++)
        printf("%s%c", board[i], delimiter);
}

size_t boardlen(char **board)
{
    int i = 0;

    while (board[i] != NULL)
        i++;
    return i;
}

char **copy_board(char **board)
{
    char **new_board = malloc(sizeof(char *) * (boardlen(board) + 2));

    for (int i = 0; board[i] != NULL; i++){
        new_board[i] = board[i];
        new_board[i + 1] = NULL;
        new_board[i + 2] = NULL;
    }
    return new_board;
}

void free_board(char **board)
{
    for (int y = 0; board[y] != NULL; y++) {
        free(board[y]);
        board[y] = NULL;
    }
    free(board);
    board = NULL;
}

static bool is_not_delim(char c, char delim)
{
    if (c == delim)
        return false;
    return true;
}

static int countcharw(char const *str, int i, char delim)
{
    int count = 0;

    while (is_not_delim(str[i], delim) && str[i] != '\0') {
        ++count;
        ++i;
    }
    return (count);
}

static char *copy_in_board(int *cpy2, char const *str, int *i, char delim)
{
    char *dest = NULL;

    *cpy2 = countcharw(str, *i, delim);
    dest = malloc(sizeof(char) * (*cpy2 + 1));
    dest[*cpy2] = '\0';
    *cpy2 = 0;
    return dest;
}

static char **copy(char **dest, char const *str, int words, char delim)
{
    int i = 0;
    int cpy1 = 0;
    int cpy2 = 0;

    while (cpy1 < words) {
        while (is_not_delim(str[i], delim) == false)
            ++i;
        if (is_not_delim(str[i], delim))
            dest[cpy1] = copy_in_board(&cpy2, str, &i, delim);
        while (is_not_delim(str[i], delim) && str[i] != '\0') {
            dest[cpy1][cpy2] = str[i];
            ++i;
            ++cpy2;
        }
        if (is_not_delim(str[i], delim) == false || str[i] == '\0')
            ++cpy1;
        while (is_not_delim(str[i], delim) == false && str[i] != '\0')
            ++i;
    }
    return (dest);
}

char **str_to_word_array(char const *str, char delim)
{
    int words = 0;
    char **dest = NULL;

    if (str == NULL)
        return NULL;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] != '\0' && is_not_delim(str[i], delim)) {
            ++words;
            ++i;
        }
        while (str[i + 1] != '\0' && is_not_delim(str[i], delim))
            ++i;
    }
    dest = malloc (sizeof(char *) * (words + 1));
    dest[words] = NULL;
    dest = copy(dest, str, words, delim);
    return (dest);
}

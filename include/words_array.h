#pragma once

char **str_to_word_array(char const *str, char delim);

void free_board(char **board);

int boardlen(char **board);

char **copy_board(char **board);

void print_board(char **board, char delimiter);

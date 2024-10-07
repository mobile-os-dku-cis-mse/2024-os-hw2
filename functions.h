#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <stdio.h>
#include "struct.h"

void check_syntax(FILE *rfile, int *Nprod, int *Ncons, char *argv[]);
void read_line(ssize_t *read, char *line, size_t *len, FILE* rfile, so_t *so, int *i);
void process_line(char *line, size_t *len, so_t *so, int *i);

#endif
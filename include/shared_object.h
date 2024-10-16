#pragma once
#define MAX_THREADS 100
#define BUFFER_SIZE 10

#include <pthread.h>
#include <stdio.h>

typedef struct sharedobject {
	FILE *rfile;
	char *buffer[BUFFER_SIZE];   // Tampon circulaire pour stocker les lignes
	int linenum[BUFFER_SIZE];    // Numéros de ligne correspondants
	int in;                      // Index où le producteur écrit
	int out;                     // Index où le consommateur lit
	int count;                   // Nombre d'éléments dans le tampon
	pthread_mutex_t lock;
	pthread_cond_t not_full;      // Condition pour vérifier si le tampon n'est pas plein
	pthread_cond_t not_empty;     // Condition pour vérifier si le tampon n'est pas vide
	int done;                    // Indicateur de fin des producteurs
} so_t;

void parse_args(int argc, char *argv[], int *Nprod, int *Ncons);
so_t* init_shared_object(FILE *rfile);

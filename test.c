#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_THREADS 100
#define BUFFER_SIZE 10  // Taille du tampon circulaire

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

/* Fonction pour initialiser la structure partagée */
so_t* init_shared_object(FILE *rfile) {
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	share->rfile = rfile;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->not_full, NULL);
	pthread_cond_init(&share->not_empty, NULL);
	return share;
}

/* Fonction pour analyser les arguments de la ligne de commande */
void parse_args(int argc, char *argv[], int *Nprod, int *Ncons) {
	if (argc < 2) {
		printf("usage: ./prod_cons <readfile> #Producer #Consumer\n");
		exit(1);
	}

	*Nprod = (argc >= 3) ? atoi(argv[2]) : 1;
	*Ncons = (argc >= 4) ? atoi(argv[3]) : 1;

	*Nprod = (*Nprod > MAX_THREADS) ? MAX_THREADS : *Nprod;
	*Nprod = (*Nprod == 0) ? 1 : *Nprod;
	*Ncons = (*Ncons > MAX_THREADS) ? MAX_THREADS : *Ncons;
	*Ncons = (*Ncons == 0) ? 1 : *Ncons;
}

/* Fonction pour produire des lignes */
void *producer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	FILE *rfile = so->rfile;
	int i = 0;
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
		read = getdelim(&line, &len, '\n', rfile);
		pthread_mutex_lock(&so->lock);

		while (so->count == BUFFER_SIZE) {  // Tampon plein
			pthread_cond_wait(&so->not_full, &so->lock);
		}

		if (read == -1) {  // Fin de fichier
			so->done = 1;
			pthread_cond_broadcast(&so->not_empty);  // Notifier les consommateurs
			pthread_mutex_unlock(&so->lock);
			break;
		}

		// Placer la ligne dans le tampon circulaire
		so->buffer[so->in] = strdup(line);
		so->linenum[so->in] = i;
		so->in = (so->in + 1) % BUFFER_SIZE;
		so->count++;
		i++;

		pthread_cond_signal(&so->not_empty);  // Notifier les consommateurs
		pthread_mutex_unlock(&so->lock);
	}

	free(line);
	printf("Prod_%x: %d lines\n", (unsigned int)pthread_self(), i);
	*ret = i;
	pthread_exit(ret);
}

/* Fonction pour consommer des lignes */
void *consumer(void *arg) {
	so_t *so = arg;
	int *ret = malloc(sizeof(int));
	int i = 0;
	char *line;

	while (1) {
		pthread_mutex_lock(&so->lock);

		while (so->count == 0 && !so->done) {  // Tampon vide et production non terminée
			pthread_cond_wait(&so->not_empty, &so->lock);
		}

		if (so->count == 0 && so->done) {  // Si la production est terminée et qu'il n'y a plus de données
			pthread_mutex_unlock(&so->lock);
			break;
		}

		// Consommer une ligne depuis le tampon circulaire
		line = so->buffer[so->out];
		printf("Cons_%x: [%02d:%02d] %s", (unsigned int)pthread_self(), i, so->linenum[so->out], line);
		free(line);  // Libérer la ligne après consommation
		so->out = (so->out + 1) % BUFFER_SIZE;
		so->count--;
		i++;

		pthread_cond_signal(&so->not_full);  // Notifier les producteurs
		pthread_mutex_unlock(&so->lock);
	}

	printf("Cons: %d lines\n", i);
	*ret = i;
	pthread_exit(ret);
}

/* Fonction pour initialiser et lancer les threads */
void create_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons, so_t *share) {
	int i;

	for (i = 0; i < Nprod; i++)
		pthread_create(&prod[i], NULL, producer, share);

	for (i = 0; i < Ncons; i++)
		pthread_create(&cons[i], NULL, consumer, share);
}

/* Fonction pour rejoindre les threads et récupérer les résultats */
void join_threads(pthread_t *prod, pthread_t *cons, int Nprod, int Ncons) {
	int *ret;
	int rc, i;

	for (i = 0; i < Ncons; i++) {
		rc = pthread_join(cons[i], (void **) &ret);
		printf("main: consumer_%d joined with %d\n", i, *ret);
		free(ret);
	}

	for (i = 0; i < Nprod; i++) {
		rc = pthread_join(prod[i], (void **) &ret);
		printf("main: producer_%d joined with %d\n", i, *ret);
		free(ret);
	}
}

/* Fonction pour libérer les ressources */
void cleanup(so_t *share) {
	pthread_mutex_destroy(&share->lock);
	pthread_cond_destroy(&share->not_full);
	pthread_cond_destroy(&share->not_empty);
	fclose(share->rfile);
	free(share);
}

int main(int argc, char *argv[]) {
	pthread_t prod[MAX_THREADS];
	pthread_t cons[MAX_THREADS];
	int Nprod, Ncons;
	FILE *rfile;

	parse_args(argc, argv, &Nprod, &Ncons);

	rfile = fopen(argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		exit(1);
	}

	so_t *share = init_shared_object(rfile);

	create_threads(prod, cons, Nprod, Ncons, share);

	join_threads(prod, cons, Nprod, Ncons);

	cleanup(share);

	pthread_exit(NULL);
	return 0;
}

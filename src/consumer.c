#include "shared_object.h"
#include <stdlib.h>

size_t number_of_alphabets_chars(char *line)
{
    size_t count = 0;

    for (size_t i = 0; line[i] != '\0'; i++)
        if ((line[i] >= 'a' && line[i] <= 'z') || (line[i] >= 'A' && line[i] <= 'Z'))
            count++;
    return count;
}

void *consumer(void *arg)
{
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
		printf("Cons_%lx: [%02d:%02d] %s", (unsigned long)pthread_self(), i, so->linenum[so->out], line);
        so->alpha_char_number += number_of_alphabets_chars(line);
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

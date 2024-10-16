#include "shared_object.h"
#include <stdlib.h>
#include <string.h>

so_t* init_shared_object(FILE *rfile)
{
	so_t *share = malloc(sizeof(so_t));
	memset(share, 0, sizeof(so_t));
	share->rfile = rfile;
	pthread_mutex_init(&share->lock, NULL);
	pthread_cond_init(&share->not_full, NULL);
	pthread_cond_init(&share->not_empty, NULL);
	return share;
}

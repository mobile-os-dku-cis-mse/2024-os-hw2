#include "shared_object.h"
#include <stdlib.h>

void parse_args(int argc, char *argv[], int *Nprod, int *Ncons)
{
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

#include "shared_object.h"
#include "threads.h"

int main(int argc, char *argv[]) {
	pthread_t prod[MAX_THREADS];
	pthread_t cons[MAX_THREADS];
	int Nprod, Ncons;
	FILE *rfile;

	parse_args(argc, argv, &Nprod, &Ncons);
	rfile = fopen(argv[1], "r");
	if (rfile == NULL) {
		perror("rfile");
		return(1);
	}
	so_t *share = init_shared_object(rfile);
	create_threads(prod, cons, Nprod, Ncons, share);
	join_threads(prod, cons, Nprod, Ncons);
    printf("Number of alphabets characters: %lu\n", share->alpha_char_number);
	cleanup(share);
	pthread_exit(NULL);
	return 0;
}

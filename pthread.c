#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

struct thread_info {
	pthread_t 	thread_id;
	int			thread_num;
	char *		argv_string;
};

static void * thread_fn(void *arg)
{
	struct thread_info *tinfo = arg;
	char *uargv, *p;

	printf("Thread %d: top of stack near %p; argv_string=%s\n",
	tinfo->thread_num, &p, tinfo->argv_string);

	uargv = strdup(tinfo->argv_string);
	if (uargv == NULL) {
		perror("strdup");
		exit(0);
	}
	for (p = uargv; *p != '\0'; p++)
		*p = toupper(*p);

	return uargv;
}

int main(int argc, char * argv[])
{
	int thread_id = 0;
	int tnum = 0;
	int opt = 0;
	int num_threads = 0;
	unsigned long stack_size = -1;
	struct thread_info *tinfo = NULL;
	void *res = NULL;
	pthread_attr_t attr;

   while ((opt = getopt(argc, argv, "s:")) != -1) {
	   switch (opt) {
	   case 's':
		   stack_size = strtoul(optarg, NULL, 0);
		   break;

	   default:
		   fprintf(stderr, "Usage: %s [-s stack-size] arg...\n",
				   argv[0]);
		   exit(EXIT_FAILURE);
	   }
   }

   num_threads = argc - optind;


	thread_id = pthread_attr_init(&attr);
	if (thread_id != 0) {
		perror("pthread_attr_init");
		exit(0);
	}

	tinfo = calloc(num_threads, sizeof(struct thread_info));
	if (tinfo == NULL) {
		perror("calloc");
		exit(0);
	}

    for (tnum = 0; tnum < num_threads; tnum++) {
		tinfo[tnum].thread_num = tnum + 1;
		tinfo[tnum].argv_string = argv[optind + tnum];

	   /* The pthread_create() call stores the thread ID into
		  corresponding element of tinfo[] */

		thread_id = pthread_create(&tinfo[tnum].thread_id, &attr,
						  &thread_fn, &tinfo[tnum]);
		if (thread_id != 0) {
			perror("pthread_create");
			exit(0);
		}
   }

   /* Destroy the thread attributes object, since it is no
	  longer needed */

   thread_id = pthread_attr_destroy(&attr);
   if (thread_id != 0) {
	   perror("pthread_attr_destroy");
		exit(0);
	}

   /* Now join with each thread, and display its returned value */

   for (tnum = 0; tnum < num_threads; tnum++) {
	   thread_id = pthread_join(tinfo[tnum].thread_id, &res);
	   if (thread_id != 0) {
		   perror("pthread_join");
			exit(0);
		}

	   printf("Joined with thread %d; returned value was %s\n",
			   tinfo[tnum].thread_num, (char *) res);
	   free(res);      /* Free memory allocated by thread */
   }

   free(tinfo);
   exit(EXIT_SUCCESS);
}

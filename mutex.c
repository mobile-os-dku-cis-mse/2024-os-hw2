#include <pthread.h>
#include <stdio.h>

// shared var
int shared_var;

// lock var
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// create two threads that adds up the values

// thread fn
// that add up shared var
// has no input arg, no return

void* thr_fn(void * thr_arg){
	// each thr will add up shared_var;
	int local_var = 0;
	pthread_mutex_lock(&lock);
	for(int i = 0; i < 0x10000000; i++){
		shared_var++;
		pthread_mutex_unlock(&lock);
		local_var++;
	}
	return &local_var;
}


int main(){
	shared_var = 0;
	
	void * res;

	pthread_t t1, t2;
	pthread_create(&t1, NULL, thr_fn, NULL);
	pthread_create(&t2, NULL, thr_fn, NULL);

	printf("t1: %d\n", (int)t1);
	printf("t2: %d\n", (int)t2);

	pthread_join(t1, &res);
	pthread_join(t2, &res);

	printf("shared_var: %x\n", shared_var);

	return 0;
}	

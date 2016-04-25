#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

sem_t thread_init_sem;
sem_t thread_done_sem;

int boat_taken = 0;
pthread_mutex_t boat_lock;
pthread_cond_t checking_boat;

// number of children on each island
int num_children_on_island[2];
pthread_mutex_t num_children_on_island_lock;
// number of adults on each island
int num_adults_on_island[2];
pthread_mutex_t num_adults_on_island_lock;
// which island each child is on
int *where_is_child;
pthread_mutex_t where_is_child_lock;
// which island each adult is on
int *where_is_adult;
pthread_mutex_t where_is_adult_lock;


void* child(void* args) {
	// INITIALIZATION //
	// acquire num_children lock
	pthread_mutex_lock(&num_children_on_island_lock);
	int id = num_children_on_island[0];
	// increment num_children
	num_children_on_island[0]++;
	// release num_children lock
	pthread_mutex_unlock(&num_children_on_island_lock);
	printf("child %d ready\n", id); 
	// wait for main
	sem_wait(&thread_init_sem);

	// acquire where_is_child lock
	pthread_mutex_lock(&where_is_child_lock);
	// initialize where_is_child, if it's not already initialized
	if(where_is_child == NULL) {
		where_is_child = malloc(num_children_on_island[0]);
	}
	// put where I am into array
	where_is_child[id] = 0;
	// release where_is_child lock
	pthread_mutex_unlock(&where_is_child_lock);

	// acquire boat lock
	pthread_mutex_lock(&boat_lock);
	while(boat_taken) {
		pthread_cond_wait(&checking_boat, &boat_lock);
	}

	// release boat lock IF WE SHOULD!!!
	pthread_mutex_unlock(&boat_lock);

	printf("child %d getting into boat on oahu\n", id);

	printf("child %d rowing boat\n", id);

	printf("child %d arrived on molokai\n", id);

	//signal main
	sem_post(&thread_done_sem);

}

void* adult(void* args) {
	// INITIALIZATION //
	// acquire num_adults lock
	pthread_mutex_lock(&num_adults_on_island_lock);
	int id = num_adults_on_island[0];
	// increment num_adults
	num_adults_on_island[0]++;
	// release num_adults lock
	pthread_mutex_unlock(&num_adults_on_island_lock);
	printf("child %d ready\n", id); 
	// wait for main
	sem_wait(&thread_init_sem);

	// acquire where_is_adult lock
	pthread_mutex_lock(&where_is_adult_lock);
	// initialize where_is_adult, if it's not already initialized
	if(where_is_adult == NULL) {
		where_is_adult = malloc(num_adults_on_island[0]);
	}
	// put where I am into array
	where_is_adult[id] = 0;
	// release where_is_adult lock
	pthread_mutex_unlock(&where_is_adult_lock);

	printf("adult %d getting into boat on oahu\n", id);

	printf("adult %d rowing boat from oahu to molokai\n", id);

	printf("adult %d arrived on molokai\n", id);

	//signal main
	sem_post(&thread_done_sem);

}

void initSynch() {
	sem_init(&thread_init_sem, 0, 0);
	sem_init(&thread_done_sem, 0, 0);
	pthread_mutex_init(&boat_lock, NULL);
	pthread_cond_init(&checking_boat, NULL);
	pthread_mutex_init(&num_children_on_island_lock, NULL);
	pthread_mutex_init(&num_adults_on_island_lock, NULL);
	pthread_mutex_init(&where_is_child_lock, NULL);
	pthread_mutex_init(&where_is_adult_lock, NULL);
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("usage: %s <number_of_children> <number_of_adults>\n", argv[0]);
		return 1;
	}
	int numChildren = atoi(argv[1]);
	int numAdults = atoi(argv[2]);
	printf("children: %d\n", numChildren);
	printf("adults: %d\n", numAdults);

	initSynch();

	int numPeople = numChildren + numAdults;
	pthread_t *threads = malloc(numPeople);

	// initialize threads; they'll sleep on the init semaphore
	int i=0;
	for(; i<numChildren; i++) {
		pthread_create(&threads[i], NULL, child, NULL);
	}
	for(; i<numPeople; i++) {
		pthread_create(&threads[i], NULL, adult, NULL);
	}
	// wake up all the threads
	for(i=0; i<numPeople; i++) {
		sem_post(&thread_init_sem);
	}
	// wait for all threads to finish
	for(i=0; i<numPeople; i++) {
		sem_wait(&thread_done_sem);
	}
	printf("all threads done; main exiting\n");

}

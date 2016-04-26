#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// semaphores for initialization and termination
sem_t thread_init_sem;
sem_t thread_done_sem;

// where the boat is.
// if it's unlocked on oahu, then children_in_boat could be 0, 1, or 2
// otherwise it'll be 0
int boat_location = 0;
pthread_mutex_t boat_location_lock;

// number of children in the boat, waiting to leave oahu
int children_in_boat = 0;
pthread_mutex_t children_in_boat_lock;

// queue for a child to wait for another child to enter the boat on oahu
pthread_cond_t child_in_boat;

// lock to synchronize printing of two children arriving on molokai
pthread_mutex_t child_print_lock;

// number of children on oahu
int num_children_on_oahu;
pthread_mutex_t num_children_on_oahu_lock;

// number of children on molokai
int num_children_on_molokai;
pthread_mutex_t num_children_on_molokai_lock;

// number of adults left to be transported
int num_adults_on_oahu;
pthread_mutex_t num_adults_on_oahu_lock;

// queue for waiting for boat for each type of person
pthread_cond_t adults_to_molokai;
pthread_cond_t children_to_molokai;
pthread_cond_t children_to_oahu;


void* child(void* args) {
	// INITIALIZATION //
	// acquire num_children lock
	pthread_mutex_lock(&num_children_on_oahu_lock);
	int id = num_children_on_oahu;
	// increment num_children_on_oahu
	num_children_on_oahu++;
	// release num_children lock
	pthread_mutex_unlock(&num_children_on_oahu_lock);
	// remember which island I'm on
	int location = 0;
	printf("child %d ready\n", id); 
	fflush(stdout);
	// wait for main
	sem_wait(&thread_init_sem);

	while(1) {

		printf("\tchild %d at LOCATION %d\n", id, location);
		// figure out where I want to go
		if(location == 1) {
			// go back to oahu iff I need to pick up adults
			pthread_mutex_lock(&num_adults_on_oahu_lock);
			if(num_adults_on_oahu == 0) {
				// we're done
				break;
			}
			pthread_mutex_unlock(&num_adults_on_oahu_lock);
		}

		// now go there
		if(location == 0) {
			// we want to go to molokai with another child

			// wait until the boat is on oahu and contains fewer than 2 children
			pthread_mutex_lock(&boat_location_lock);
			pthread_mutex_lock(&children_in_boat_lock);
			while(boat_location == 1 || children_in_boat > 1) {
				pthread_mutex_unlock(&children_in_boat_lock);
				pthread_cond_wait(&children_to_molokai, &boat_location_lock);
				pthread_mutex_lock(&children_in_boat_lock);
			}

			pthread_mutex_unlock(&boat_location_lock);

			if(children_in_boat == 0) {
				// we're the first one in
				children_in_boat = 1;
				printf("child %d getting into boat on oahu (pilot)\n", id);
				fflush(stdout);
				pthread_mutex_unlock(&children_in_boat_lock);
				// wait for another child to get in the boat
				pthread_mutex_lock(&children_in_boat_lock);
				while(children_in_boat == 1) {
					pthread_cond_wait(&child_in_boat, &children_in_boat_lock);
				}
				// so we get to print first
				pthread_mutex_lock(&child_print_lock);
				// and we get to row the boat
				printf("child %d rowing boat from oahu to molokai\n", id);
				fflush(stdout);

				printf("child %d arrived on molokai\n", id);
				fflush(stdout);

				// let the passenger child print
				pthread_mutex_unlock(&child_print_lock);

				// and we get to reset children_in_boat
				children_in_boat = 0;
				pthread_mutex_unlock(&children_in_boat_lock);
			} else {
				// we're the second one in
				children_in_boat = 2;
				printf("child %d getting into boat on oahu\n", id);
				fflush(stdout);
				// tell the first child that it's ok to row
				pthread_cond_signal(&child_in_boat);
				pthread_mutex_unlock(&children_in_boat_lock);
				// wait to print that we've arrived until the other child says he's arrived
				pthread_mutex_lock(&child_print_lock);
				printf("child %d arrived on molokai\n", id);
				fflush(stdout);
				pthread_mutex_unlock(&child_print_lock);
			}

			location = 1;
			pthread_mutex_lock(&boat_location_lock);
			boat_location = 0;
			pthread_mutex_unlock(&boat_location_lock);


			// increment num_children_on_molokai
			pthread_mutex_lock(&num_children_on_molokai_lock);
			num_children_on_molokai++;
			pthread_mutex_unlock(&num_children_on_molokai_lock);


			// wake up one child, if we should
			pthread_mutex_lock(&num_children_on_molokai_lock);
			if(num_children_on_oahu > 0) {
				pthread_cond_signal(&children_to_oahu);
			}
			pthread_mutex_unlock(&num_children_on_molokai_lock);

		} else { // location == 1
			// we want to go back to oahu alone

			// wait until the boat is on molokai
			pthread_mutex_lock(&boat_location_lock);
			while(boat_location == 0) {
				pthread_cond_wait(&children_to_oahu, &boat_location_lock);
			}

			printf("child %d getting into boat on molokai\n", id);
			fflush(stdout);
			printf("child %d rowing boat from molokai to oahu\n", id);
			fflush(stdout);
			
			pthread_mutex_lock(&num_children_on_molokai_lock);
			num_children_on_molokai--;
			pthread_mutex_unlock(&num_children_on_molokai_lock);

			printf("child %d arrived on oahu\n", id);
			fflush(stdout);

			location = 0;

			boat_location = 0;
			pthread_mutex_unlock(&boat_location_lock);
			// wake up two children and one adult
			pthread_cond_signal(&adults_to_molokai);
			pthread_cond_signal(&children_to_molokai);
			pthread_cond_signal(&children_to_molokai);

		}

	}
	//signal main
	sem_post(&thread_done_sem);

	// exit
	return (void*)NULL;

}

void* adult(void* args) {
	// INITIALIZATION //
	// acquire num_adults lock
	pthread_mutex_lock(&num_adults_on_oahu_lock);
	int id = num_adults_on_oahu;
	// increment num_adults
	num_adults_on_oahu++;
	// release num_adults lock
	pthread_mutex_unlock(&num_adults_on_oahu_lock);
	printf("adult %d ready\n", id); 
	fflush(stdout);
	// wait for main
	sem_wait(&thread_init_sem);

	// wait until the boat is empty on oahu and there is a child on molokai to row it back
	pthread_mutex_lock(&boat_location_lock);
	printf("\tadult %d acquired boat_location_lock\n", id);
	fflush(stdout);
	pthread_mutex_lock(&num_children_on_molokai_lock);
	printf("\tadult %d acquired num_children_on_molokai_lock\n", id);
	fflush(stdout);
	pthread_mutex_lock(&children_in_boat_lock);
	printf("\tadult %d acquired children_in_boat_lock\n", id);
	fflush(stdout);
	while(boat_location == 1 || num_children_on_molokai == 0 || children_in_boat > 0) {
		pthread_mutex_unlock(&children_in_boat_lock);
		pthread_mutex_unlock(&num_children_on_molokai_lock);
		pthread_cond_wait(&adults_to_molokai, &boat_location_lock);
		printf("\tadult %d WOKEN UP\n", id);
		pthread_mutex_lock(&num_children_on_molokai_lock);
		pthread_mutex_lock(&children_in_boat_lock);
	}

	// we can give up these lock
	pthread_mutex_unlock(&num_children_on_molokai_lock);
	pthread_mutex_unlock(&children_in_boat_lock);

	// get the boat
	printf("adult %d getting into boat on oahu\n", id);
	fflush(stdout);

	printf("adult %d rowing boat from oahu to molokai\n", id);
	fflush(stdout);

	// now we're on molokai
	boat_location = 1;
	printf("adult %d arrived on molokai\n", id);
	fflush(stdout);

	// we can give up this lock, since we still have the boat
	pthread_mutex_unlock(&boat_location_lock);

	// decrement remaining number of adults to transport
	pthread_mutex_lock(&num_adults_on_oahu_lock);
	num_adults_on_oahu--;
	pthread_mutex_unlock(&num_adults_on_oahu_lock);

	// signal a child on molokai
	pthread_cond_signal(&children_to_oahu);

	//signal main
	sem_post(&thread_done_sem);

	// exit
	return (void*)NULL;

}

void initSynch() {
	sem_init(&thread_init_sem, 0, 0);
	sem_init(&thread_done_sem, 0, 0);
	pthread_mutex_init(&boat_location_lock, NULL);
	pthread_cond_init(&adults_to_molokai, NULL);
	pthread_cond_init(&children_to_molokai, NULL);
	pthread_cond_init(&children_to_oahu, NULL);
	pthread_cond_init(&child_in_boat, NULL);
	pthread_mutex_init(&child_print_lock, NULL);
	pthread_mutex_init(&children_in_boat_lock, NULL);
	pthread_mutex_init(&num_children_on_oahu_lock, NULL);
	pthread_mutex_init(&num_children_on_molokai_lock, NULL);
	pthread_mutex_init(&num_adults_on_oahu_lock, NULL);
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		printf("usage: %s <number_of_children> <number_of_adults>\n", argv[0]);
		fflush(stdout);
		return 1;
	}
	int numChildren = atoi(argv[1]);
	int numAdults = atoi(argv[2]);

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
	fflush(stdout);

}

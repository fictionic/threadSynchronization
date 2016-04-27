#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// semaphores for initialization and termination
sem_t thread_init_sem;
sem_t thread_done_sem;

// queues for waiting for boat
pthread_cond_t adults_to_molokai;
pthread_cond_t children_to_molokai;
pthread_cond_t children_to_oahu;
pthread_cond_t waiting_for_passenger;

// lock for checking shared variables
pthread_mutex_t var_lock;

// shared variables
int num_children_total = 0;
int num_adults_total = 0;
int boat_location = 0;
int num_children_on_oahu = 0;
int num_children_on_molokai = 0;
int num_adults_on_oahu = 0;
int children_in_boat = 0;
int turn_to_print = 0;

void* child(void* args) {
	// INITIALIZATION //
	pthread_mutex_lock(&var_lock);
	int id = num_children_total;
	num_children_on_oahu++;
	num_children_total++;
	pthread_mutex_unlock(&var_lock);
	int location = 0;
	printf("child %d ready (on oahu)\n", id);
	// wait for main
	sem_wait(&thread_init_sem);

	// run the algorithm
	while(1) {
		if(location == 0) {
			// we need to go to molokai
			pthread_mutex_lock(&var_lock);
			while(boat_location == 1 || children_in_boat > 1) {
				pthread_cond_wait(&children_to_molokai, &var_lock);
			}
			// see if there is a child in the boat already
			if(children_in_boat == 0) {
				// we're the first one in
				children_in_boat = 1;
				printf("child %d getting into boat on oahu (as rower)\n", id);
				// check if we should wait for another child to get in
				if(num_children_on_oahu > 1) {
					// wait for the next child to get in
					turn_to_print = 1;
					while(turn_to_print == 1) {
						/* printf("\tchild %d waiting for passenger to get into boat\n", id); */
						pthread_cond_wait(&waiting_for_passenger, &var_lock);
					}
					printf("child %d rowing boat from oahu to molokai\n", id);
					// wait for passenger to say he's being rowed, then get out
					turn_to_print = 1;
					pthread_cond_signal(&waiting_for_passenger);
					while(turn_to_print == 1) {
						/* printf("\tchild %d waiting for passenger to get out of boat\n", id); */
						pthread_cond_wait(&waiting_for_passenger, &var_lock);
					}
					printf("child %d getting out of boat on molokai\n", id);
					turn_to_print = 1;
					pthread_cond_signal(&waiting_for_passenger);
				} else {
					printf("child %d rowing boat from oahu to molokai\n", id);
					printf("child %d arrived on molokai\n", id);
					printf("child %d getting out of boat on molokai\n", id);
				}
				num_children_on_oahu--;
				num_children_on_molokai++;
				boat_location = 1;
				children_in_boat--;
				pthread_mutex_unlock(&var_lock);
				location = 1;
				// now loop
			} else if(children_in_boat == 1) {
				// we're the second one here
				// wait until it's our turn to print
				printf("child %d getting into boat on oahu (as passenger)\n", id);
				children_in_boat = 2;
				// wait for rower child to start rowing
				turn_to_print = 0;
				pthread_cond_signal(&waiting_for_passenger);
				while(turn_to_print == 0) {
					/* printf("\tchild %d waiting for rower\n", id); */
					pthread_cond_wait(&waiting_for_passenger, &var_lock);
				}
				printf("child %d being rowed from oahu to molokai\n", id);
				// get out of boat
				printf("child %d getting out of boat on molokai\n", id);
				children_in_boat--;
				num_children_on_oahu--;
				num_children_on_molokai++;
				// signal and wait for rower child
				turn_to_print = 0;
				pthread_cond_signal(&waiting_for_passenger);
				while(turn_to_print == 0) {
					/* printf("\tchild %d waiting for rower to get out of the boat\n", id); */
					pthread_cond_wait(&waiting_for_passenger, &var_lock);
				}
				// see if we should signal a child
				if(num_children_on_molokai > 0) {
					// signal a child waiting for boat
					pthread_cond_signal(&children_to_oahu);
				}
				pthread_mutex_unlock(&var_lock);
				location = 1;
				// now loop
				printf("\tchild %d looping\n", id);
			} else {
				printf("ERROR: children_in_boat negative: %d\n", children_in_boat);
				break;
			}
		} else { // location == 1
			pthread_mutex_lock(&var_lock);
			// wait for the boat
			while(boat_location == 0) {
				pthread_cond_wait(&children_to_oahu, &var_lock);
			}
			// see if we need to back to oahu
			printf("\tchild %d sees num_adults_on_oahu == %d and num_children_on_oahu == %d\n", id, num_adults_on_oahu, num_children_on_oahu);
			if(num_adults_on_oahu + num_children_on_oahu == 0) {
				// we don't, so break
				pthread_mutex_unlock(&var_lock);
				break;
			}
			printf("child %d getting into boat on molokai\n", id);
			printf("child %d rowing boat from molokai to oahu \n", id);
			printf("child %d arrived on oahu\n", id);
			printf("child %d getting out of boat on oahu\n", id);
			num_children_on_oahu++;
			num_children_on_molokai--;
			boat_location = 0;
			pthread_mutex_unlock(&var_lock);
			location = 0;
			// signal an adult and two children
			pthread_cond_signal(&adults_to_molokai);
			pthread_cond_signal(&children_to_molokai);
			pthread_cond_signal(&children_to_molokai);
			// now loop
		}
	}

	printf("\tchild %d DONE\n", id);
	// signal main
	sem_post(&thread_done_sem);
	return (void*)NULL;
}


void* adult(void* args) {
	// INITIALIZATION //
	pthread_mutex_lock(&var_lock);
	int id = num_adults_total;
	num_adults_on_oahu++;
	num_adults_total++;
	pthread_mutex_unlock(&var_lock);
	printf("adult %d ready (on oahu)\n", id);
	// wait for main
	sem_wait(&thread_init_sem);

	// run the algorithm
	pthread_mutex_lock(&var_lock);
	// wait for the boat
	while(boat_location == 1 || children_in_boat > 0 || num_children_on_molokai == 0) {
		pthread_cond_wait(&adults_to_molokai, &var_lock);
	}
	printf("adult %d getting into boat on oahu\n", id);
	printf("adult %d rowing boat from oahu to molokai\n", id);
	printf("adult %d arrived on molokai\n", id);
	printf("adult %d getting out of boat on molokai\n", id);
	num_adults_on_oahu--;
	boat_location = 1;
	pthread_mutex_unlock(&var_lock);
	// signal an adult and two children
	pthread_cond_signal(&adults_to_molokai);
	pthread_cond_signal(&children_to_molokai);
	pthread_cond_signal(&children_to_molokai);

	printf("\tadult %d DONE\n", id);
	// signal main
	sem_post(&thread_done_sem);
	return (void*)NULL;
}

void initSynch() {
	sem_init(&thread_init_sem, 0, 0);
	sem_init(&thread_done_sem, 0, 0);
	pthread_cond_init(&adults_to_molokai, NULL);
	pthread_cond_init(&children_to_molokai, NULL);
	pthread_cond_init(&children_to_oahu, NULL);
	pthread_mutex_init(&var_lock, NULL);
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

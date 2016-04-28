/* oahu_to_molokai.c
 * Dylan Forbes and Ben Withbroe
 * 2016-04-27
 * Operating Systems
 */

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// semaphores for initialization and termination
sem_t* thread_init_sem;
sem_t* thread_done_sem;

// queues for waiting for boat
pthread_cond_t adults_to_molokai;
pthread_cond_t children_to_molokai;
pthread_cond_t children_to_oahu;
pthread_cond_t child_print_turn;

// lock for checking shared variables
pthread_mutex_t the_seeing_stone;

// shared variables
int num_children_total = 0;
int num_adults_total = 0;
int boat_location = 0;
int num_children_on_oahu = 0;
int num_children_on_oahu_as_believed_on_molokai = 0;
int num_children_on_molokai = 0;
int num_children_on_molokai_as_believed_on_oahu = 0;
int num_adults_on_oahu = 0;
int num_adults_on_oahu_as_believed_on_molokai = 0;

int children_in_boat = 0;
int turn_to_print = 0;

void* child(void* args) {
	// INITIALIZATION //
	pthread_mutex_lock(&the_seeing_stone);
	int id = num_children_total;
	num_children_on_oahu++;
	num_children_total++;
	pthread_mutex_unlock(&the_seeing_stone);
	int remembered_num_children_on_oahu;
	int remembered_num_children_on_molokai;
	int remembered_num_adults_on_oahu;
	int location = 0;
	printf("child %d ready (on oahu)\n", id);
	// wait for main
	sem_wait(thread_init_sem);
	printf("\tchild %d woken up by main\n", id);

	// run the algorithm
	while(1) {
		if(location == 0) {
			// we need to go to molokai
			pthread_mutex_lock(&the_seeing_stone);
			printf("\tchild %d sees boat_location == %d ", id, boat_location);
			if(boat_location == 0) {
				printf("and children_in_boat == %d\n", children_in_boat);
			} else {
				printf("\n");
			}
			while(boat_location == 1 || children_in_boat > 1) {
				pthread_cond_wait(&children_to_molokai, &the_seeing_stone);
			}
			num_children_on_oahu--;
			// see if there is a child in the boat already
			if(children_in_boat == 0) {
				// we're the first one in
				children_in_boat = 1;
				printf("child %d getting into boat on oahu (as rower)\n", id);
				// check if we should wait for another child to get in
				printf("\tchild %d sees num_children_on_oahu == %d\n", id, num_children_on_oahu);
				if(num_children_on_oahu > 0) {
					// wait for the next child to get in
					turn_to_print = 1;
					while(turn_to_print == 1) {
						/* printf("\tchild %d waiting for passenger to get into boat\n", id); */
						pthread_cond_wait(&child_print_turn, &the_seeing_stone);
					}
					// remember what these were
					remembered_num_children_on_oahu = num_children_on_oahu;
					remembered_num_adults_on_oahu = num_adults_on_oahu;
					printf("child %d rowing boat from oahu to molokai\n", id);
					// wait for passenger to say he's being rowed and to get out
					turn_to_print = 1;
					pthread_cond_signal(&child_print_turn);
					while(turn_to_print == 1) {
						/* printf("\tchild %d waiting for passenger to get out of boat\n", id); */
						pthread_cond_wait(&child_print_turn, &the_seeing_stone);
					}
					printf("child %d getting out of boat on molokai\n", id);
					turn_to_print = 1;
					pthread_cond_signal(&child_print_turn);
				} else {
					// remember what these are
					remembered_num_children_on_oahu = num_children_on_oahu;
					remembered_num_adults_on_oahu = num_adults_on_oahu;
					printf("child %d rowing boat from oahu to molokai\n", id);
					printf("child %d arrived on molokai\n", id);
					printf("child %d getting out of boat on molokai\n", id);
				}
				boat_location = 1;
				children_in_boat--;
				// tell everyone on the molokai what we remember about oahu
				num_children_on_oahu_as_believed_on_molokai = remembered_num_children_on_oahu;
				num_adults_on_oahu_as_believed_on_molokai = remembered_num_adults_on_oahu;
				// release the lock
				pthread_mutex_unlock(&the_seeing_stone);
				location = 1;
				// now loop
			} else if(children_in_boat == 1) {
				// we're the second one here
				// remember these things for later
				remembered_num_children_on_oahu = num_children_on_oahu;
				remembered_num_adults_on_oahu = num_adults_on_oahu;
				// wait until it's our turn to print
				printf("child %d getting into boat on oahu (as passenger)\n", id);
				children_in_boat = 2;
				// wait for rower child to start rowing
				turn_to_print = 0;
				pthread_cond_signal(&child_print_turn);
				while(turn_to_print == 0) {
					/* printf("\tchild %d waiting for rower\n", id); */
					pthread_cond_wait(&child_print_turn, &the_seeing_stone);
				}
				printf("child %d being rowed from oahu to molokai\n", id);
				// get out of boat
				printf("child %d getting out of boat on molokai\n", id);
				children_in_boat--;
				// tell everyone on the molokai what we remember about oahu
			printf("\tchild %d remembers num_adults_on_oahu == %d and num_children_on_oahu == %d\n", id, num_adults_on_oahu_as_believed_on_molokai, num_children_on_oahu_as_believed_on_molokai);
				num_children_on_oahu_as_believed_on_molokai = remembered_num_children_on_oahu;
				num_adults_on_oahu_as_believed_on_molokai = remembered_num_adults_on_oahu;

				// signal and wait for rower child
				turn_to_print = 0;
				pthread_cond_signal(&child_print_turn);
				while(turn_to_print == 0) {
					/* printf("\tchild %d waiting for rower to get out of the boat\n", id); */
					pthread_cond_wait(&child_print_turn, &the_seeing_stone);
				}
				// see if we should signal a child
				if(num_children_on_molokai > 0) {
					// signal a child waiting for boat
					pthread_cond_signal(&children_to_oahu);
				}
				pthread_mutex_unlock(&the_seeing_stone);
				location = 1;
				// now loop
				printf("\tchild %d looping\n", id);
			} else {
				printf("ERROR: children_in_boat negative: %d\n", children_in_boat);
				break;
			}
		} else { // location == 1
			pthread_mutex_lock(&the_seeing_stone);
			// wait for the boat
			while(boat_location == 0) {
				pthread_cond_wait(&children_to_oahu, &the_seeing_stone);
			}
			// see if we need to go back to oahu
			printf("\tchild %d remembers num_adults_on_oahu == %d and num_children_on_oahu == %d\n", id, num_adults_on_oahu_as_believed_on_molokai, num_children_on_oahu_as_believed_on_molokai);
			if(num_adults_on_oahu_as_believed_on_molokai + num_children_on_oahu_as_believed_on_molokai == 0) {
				// we don't, so break
				pthread_mutex_unlock(&the_seeing_stone);
				break;
			}
			// decrement number of children on molokai
			num_children_on_molokai--;
			// we're going back to oahu, so remember this for when we get there
			remembered_num_children_on_molokai = num_children_on_molokai;
			// (don't care about num_adults_on_molokai)
			printf("child %d getting into boat on molokai\n", id);
			printf("child %d rowing boat from molokai to oahu \n", id);
			printf("child %d arrived on oahu\n", id);
			printf("child %d getting out of boat on oahu\n", id);
			num_children_on_oahu++;
			boat_location = 0;
			location = 0;
			// determine whom we should signal
			if(num_adults_on_oahu > 0) {
				printf("child %d signaling adults_to_molokai\n", id);
				pthread_mutex_unlock(&the_seeing_stone);
				pthread_cond_signal(&adults_to_molokai);
			} else if(num_children_on_oahu > 0) {
				// signal two children on oahu
				pthread_mutex_unlock(&the_seeing_stone);
				printf("\tchild %d signaling children_to_molokai (twice)\n", id);
				pthread_cond_signal(&children_to_molokai);
				pthread_cond_signal(&children_to_molokai);
			}
			// now loop
			printf("\tchild %d looping\n", id);
		}
	}

	printf("\tchild %d DONE\n", id);
	// signal main
	sem_post(thread_done_sem);
	return (void*)NULL;
}


void* adult(void* args) {
	// INITIALIZATION //
	pthread_mutex_lock(&the_seeing_stone);
	int id = num_adults_total;
	num_adults_on_oahu++;
	num_adults_total++;
	int remembered_num_adults_on_oahu;
	int remembered_num_children_on_oahu;
	pthread_mutex_unlock(&the_seeing_stone);
	printf("adult %d ready (on oahu)\n", id);
	// wait for main
	sem_wait(thread_init_sem);
	printf("\tadult %d woken up by main\n", id);

	// run the algorithm
	pthread_mutex_lock(&the_seeing_stone);
	// wait for the boat
	printf("\tadult %d sees boat_location == %d ", id, boat_location);
	if(boat_location == 0) {
		printf("and children_in_boat == %d\n", children_in_boat);
	} else {
		printf("\n");
	}
	printf("\tadult %d sees num_children_on_oahu == %d, and num_children_on_molokai_as_believed_on_oahu == %d\n", id, num_children_on_oahu, num_children_on_molokai_as_believed_on_oahu);
	while(boat_location == 1 || children_in_boat > 0 || (num_children_on_molokai_as_believed_on_oahu == 0 && num_children_on_oahu > 0)) {
		// ^ children_in_boat won't be accessed unless boat_location == 0
		pthread_cond_wait(&adults_to_molokai, &the_seeing_stone);
	}
	// decrement the number of adults on oahu
	num_adults_on_oahu--;
	// remember this for later
	remembered_num_adults_on_oahu = num_adults_on_oahu;
	remembered_num_children_on_oahu = num_children_on_oahu;
	printf("adult %d getting into boat on oahu\n", id);
	printf("adult %d rowing boat from oahu to molokai\n", id);
	printf("adult %d arrived on molokai\n", id);
	printf("adult %d getting out of boat on molokai\n", id);
	// tell everyone what we remember
	num_adults_on_oahu_as_believed_on_molokai = remembered_num_adults_on_oahu;
	num_children_on_oahu_as_believed_on_molokai = remembered_num_children_on_oahu;
	boat_location = 1;
	pthread_mutex_unlock(&the_seeing_stone);
	// signal an adult and two children
	pthread_cond_signal(&adults_to_molokai);
	pthread_cond_signal(&children_to_molokai);
	pthread_cond_signal(&children_to_molokai);

	printf("\tadult %d DONE\n", id);
	// signal main
	sem_post(thread_done_sem);
	return (void*)NULL;
}

void initSynch() {
	thread_init_sem = sem_open("init_sem", O_CREAT | O_EXCL, 0644, 0);
	thread_done_sem = sem_open("done_sem", O_CREAT | O_EXCL, 0644, 0);
	pthread_cond_init(&adults_to_molokai, NULL);
	pthread_cond_init(&children_to_molokai, NULL);
	pthread_cond_init(&children_to_oahu, NULL);
	pthread_mutex_init(&the_seeing_stone, NULL);
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
		printf("created thread %d\n", i);
	}
	for(; i<numPeople; i++) {
		pthread_create(&threads[i], NULL, adult, NULL);
		printf("created thread %d\n", i);
	}
	// acquire the lock, so no threads can continue
	printf("\tmain acquired lock\n");
	pthread_mutex_lock(&the_seeing_stone);
	// wake up all the threads
	for(i=0; i<numPeople; i++) {
		sem_post(thread_init_sem);
		printf("woke up thread %d\n", i);
	}
	pthread_mutex_unlock(&the_seeing_stone);
	printf("\tmain released lock\n");
	// wait for all threads to finish
	for(i=0; i<numPeople; i++) {
		sem_wait(thread_done_sem);
	}
	printf("all threads done; main exiting\n");
	fflush(stdout);

}

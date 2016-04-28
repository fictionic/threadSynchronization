/* oahu_to_molokai.c
 * Dylan Forbes and Ben Withbroe
 * 2016-04-27
 * Operating Systems
 */

#include <pthread.h>
#include <errno.h>
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
pthread_cond_t child_print_queue;
pthread_cond_t startup;

// lock for checking shared variables
pthread_mutex_t the_seeing_stone;

// shared variables
int ready_to_go = 0;
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

void* child(void* args);
void* adult(void* args);
void initSynch();

// MAIN FUNCTION //
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
	// wake up all the threads
	for(i=0; i<numPeople; i++) {
		sem_wait(thread_init_sem);
	}
	ready_to_go = 1;
	pthread_mutex_lock(&the_seeing_stone);
	pthread_cond_broadcast(&startup);
	pthread_mutex_unlock(&the_seeing_stone);
	printf("woke up threads\n");
	// wait for all threads to finish
	for(i=0; i<numPeople; i++) {
		sem_wait(thread_done_sem);
	}
	printf("all threads done; main exiting\n");
	//close semaphores
	sem_close(thread_init_sem);
	sem_unlink("init_sem");
	sem_close(thread_done_sem);
	sem_unlink("done_sem");
	fflush(stdout);
}


void* child(void* args) {
	// INITIALIZATION //
	pthread_mutex_lock(&the_seeing_stone);
	int id = num_children_total;
	num_children_on_oahu++;
	num_children_total++;
	int remembered_num_children_on_oahu;
	int remembered_num_children_on_molokai;
	int remembered_num_adults_on_oahu;
	int location = 0;
	printf("child %d ready (on oahu)\n", id);
	// wait for main
	sem_post(thread_init_sem);
	while(ready_to_go == 0) {
		pthread_cond_wait(&startup, &the_seeing_stone);
	}
	pthread_mutex_unlock(&the_seeing_stone);
	// run the algorithm
	while(1) {
		pthread_mutex_lock(&the_seeing_stone);
		if(location == 0) {
			// we need to go to molokai
			while(boat_location == 1 || children_in_boat > 1 || (num_adults_on_oahu > 0 && num_children_on_molokai_as_believed_on_oahu > 0)) {
				pthread_cond_wait(&children_to_molokai, &the_seeing_stone);
			}
			// see if there is a child in the boat already
			if(children_in_boat == 0) {
				// we're the first one in
				children_in_boat = 1;
				printf("child %d (rower) getting into boat on oahu (as rower)\n", id);
				// remember what these were
				num_children_on_oahu--;
				// check if we should wait for another child to get in
				if(num_children_on_oahu > 0) {
					// wait for the next child to get in
					turn_to_print = 1;
					while(turn_to_print == 1) {
						pthread_cond_wait(&child_print_queue, &the_seeing_stone);
					}
					printf("child %d (rower) rowing boat from oahu to molokai\n", id);
					remembered_num_children_on_oahu = num_children_on_oahu;
					remembered_num_adults_on_oahu = num_adults_on_oahu;
					// we've arrived
					boat_location = 1;
					// wait for passenger to get out
					turn_to_print = 1;
					pthread_cond_signal(&child_print_queue);
					while(turn_to_print == 1) {
						pthread_cond_wait(&child_print_queue, &the_seeing_stone);
					}
					printf("child %d (rower) getting out of boat on molokai\n", id);
				} else {
					printf("child %d (rower) rowing boat from oahu to molokai\n", id);
					// we've arrived
					printf("child %d (rower) getting out of boat on molokai\n", id);
				}
				children_in_boat--;
				num_children_on_molokai++;
				// tell everyone on the molokai what we remember about oahu
				num_children_on_oahu_as_believed_on_molokai = remembered_num_children_on_oahu;
				num_adults_on_oahu_as_believed_on_molokai = remembered_num_adults_on_oahu;
				// release the lock
				pthread_mutex_unlock(&the_seeing_stone);
				location = 1;
				// now loop
			} else if(children_in_boat == 1) {
				// we're the second one here
				children_in_boat = 2;
				printf("child %d (passenger) getting into boat on oahu\n", id);
				// decrement num_children_on_oahu
				num_children_on_oahu--;
				// wait for rower child to start rowing
				turn_to_print = 0;
				pthread_cond_signal(&child_print_queue);
				while(turn_to_print == 0) {
					pthread_cond_wait(&child_print_queue, &the_seeing_stone);
				}
				// remember these things for later
				remembered_num_children_on_oahu = num_children_on_oahu;
				remembered_num_adults_on_oahu = num_adults_on_oahu;
				// get out of boat
				printf("child %d (passenger) getting out of boat on molokai\n", id);
				children_in_boat--;
				num_children_on_molokai++;
				// tell everyone on the molokai what we remember about oahu
				num_children_on_oahu_as_believed_on_molokai = remembered_num_children_on_oahu;
				num_adults_on_oahu_as_believed_on_molokai = remembered_num_adults_on_oahu;
				// signal rower child final time
				turn_to_print = 0;
				pthread_cond_signal(&child_print_queue);
				// see if we should signal a child going back to oahu
				if(num_children_on_molokai > 0) {
					// signal a child waiting for boat
					pthread_cond_signal(&children_to_oahu);
				}
				pthread_mutex_unlock(&the_seeing_stone);
				location = 1;
				// now loop
			} else {
				printf("ERROR: children_in_boat negative: %d\n", children_in_boat);
				break;
			}
		} else { // location == 1
			// wait for the boat
			while(boat_location == 0 || children_in_boat > 0) {
				pthread_cond_wait(&children_to_oahu, &the_seeing_stone);
			}
			// see if we need to go back to oahu
			if(num_adults_on_oahu_as_believed_on_molokai + num_children_on_oahu_as_believed_on_molokai == 0) {
				// we don't, so break
				pthread_mutex_unlock(&the_seeing_stone);
				break;
			}
			// get into boat
			printf("child %d getting into boat on molokai\n", id);
			// decrement number of children on molokai
			num_children_on_molokai--;
			// we're going back to oahu, so remember this for when we get there (don't care about num_adults_on_molokai)
			remembered_num_children_on_molokai = num_children_on_molokai;
			printf("child %d rowing boat from molokai to oahu \n", id);
			printf("child %d arrived on oahu\n", id);
			printf("child %d getting out of boat on oahu\n", id);
			num_children_on_oahu++;
			boat_location = 0;
			location = 0;
			// tell everyone what we remember
			num_children_on_molokai_as_believed_on_oahu = remembered_num_children_on_molokai;
			// determine whom we should signal
			if(num_adults_on_oahu > 0 && num_children_on_molokai_as_believed_on_oahu > 0) {
				printf("child %d signaling adults_to_molokai\n", id);
				pthread_mutex_unlock(&the_seeing_stone);
				pthread_cond_signal(&adults_to_molokai);
			} else if(num_children_on_oahu > 0) {
				if(!(boat_location == 1 || children_in_boat > 1 || (num_adults_on_oahu > 0 && num_children_on_molokai_as_believed_on_oahu > 0))) {
					// signal two children on oahu
					pthread_mutex_unlock(&the_seeing_stone);
					pthread_cond_signal(&children_to_molokai);
					if(num_children_on_oahu > 1) {
						pthread_cond_signal(&children_to_molokai);
					}
				}
			}
			// now loop
		}
	}
	// let any waiting children know that we're done
	pthread_cond_broadcast(&children_to_oahu);
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
	printf("adult %d ready (on oahu)\n", id);
	// wait for main
	sem_post(thread_init_sem);
	while(ready_to_go == 0) {
		pthread_cond_wait(&startup, &the_seeing_stone);
	}
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
	// signal two children
	pthread_cond_signal(&children_to_oahu);
	pthread_cond_signal(&children_to_oahu);
	// signal main
	sem_post(thread_done_sem);
	return (void*)NULL;
}

void initSynch() {
	thread_init_sem = sem_open("init_sem", O_CREAT | O_EXCL, 0644, 0);
	while(thread_init_sem == SEM_FAILED) {
		// unlinks and reopens semaphore if it was already opened
		if(errno == EEXIST) {
			printf("unlinking and reopening init_sem, as it already exists\n");
			sem_unlink("init_sem");
			// initializes the lock semaphores with value 1, and the rest with value 0
			thread_init_sem = sem_open("init_sem", O_CREAT | O_EXCL, 0644, 0);
		// exits if the semaphore could not be opened
		} else {
			printf("semaphore could not be opened, error # %d\n", errno);
			exit(1);
		}
	}

	thread_done_sem = sem_open("done_sem", O_CREAT | O_EXCL, 0644, 0);
	while(thread_done_sem == SEM_FAILED) {
		// unlinks and reopens semaphore if it was already opened
		if(errno == EEXIST) {
			printf("unlinking and reopening done_sem, as it already exists\n");
			sem_unlink("done_sem");
			// initializes the lock semaphores with value 1, and the rest with value 0
			thread_done_sem = sem_open("done_sem", O_CREAT | O_EXCL, 0644, 0);
		// exits if the semaphore could not be opened
		} else {
			printf("semaphore could not be opened, error # %d\n", errno);
			exit(1);
		}
	}
	pthread_cond_init(&startup, NULL);
	pthread_cond_init(&adults_to_molokai, NULL);
	pthread_cond_init(&children_to_molokai, NULL);
	pthread_cond_init(&children_to_oahu, NULL);
	pthread_cond_init(&child_print_queue, NULL);
	pthread_mutex_init(&the_seeing_stone, NULL);
}


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "H2SO4.h"

// semaphores used as locks to avoid race conditions when forming molecules
sem_t *molecule_lock, *num_lock, *sulfur_lock;
// semaphores used to keep threads from busy waiting while atoms form
sem_t *oxygen_sem, *hydro_sem;
// semaphore used for synchronizing leaving molecules
sem_t *leave_sem;

// number of each molecule for
int num_oxygen = 0;
int num_hydrogen = 0;
int num_sulfur = 0;

// semaphores stored in arrays to simplify error checking
sem_t** sems;
char** sem_names;
int num_sems = 6;

void* oxygen(void* args) {
	sem_wait(num_lock);
	num_oxygen++;
	sem_post(num_lock);

	printf("oxygen produced\n");

	// alerts the sulfur thread that we have created a new atom
	sem_post(molecule_lock);

	// waits until there are enough atoms for this one to leave
	sem_wait(oxygen_sem);
	sem_wait(num_lock);
	num_oxygen--;
	sem_post(num_lock);
	printf("oxygen leaving\n");

	sem_post(leave_sem);
	return (void*)NULL;
}

void* hydrogen(void* args) {
	sem_wait(num_lock);
	num_hydrogen++;
	sem_post(num_lock);

	printf("hydrogen produced\n");

	// alerts the sulfur thread that we have created a new atom
	sem_post(molecule_lock);

	// waits until there are enough atoms for this one to leave
	sem_wait(hydro_sem);
	sem_wait(num_lock);
	num_hydrogen--;
	sem_post(num_lock);
	printf("hydrogen leaving\n");

	sem_post(leave_sem);
	return (void*)NULL;
}

void* sulfur(void* args) {
	sem_wait(num_lock);
	num_sulfur++;
	sem_post(num_lock);

	printf("sulfur produced\n");
	sem_wait(sulfur_lock);

	sem_wait(num_lock);
	// fails and continues if we have enough atoms, or waits if we don't
	while (!(num_oxygen >= 4 && num_hydrogen >= 2 && num_sulfur >= 1)) {
		sem_post(num_lock);
		sem_wait(molecule_lock);
		sem_wait(num_lock);
	}
	sem_post(num_lock);

	printf("\nmolecule formed!\n");

	// waits for hydrogen atoms to leave
	sem_post(hydro_sem);
	sem_wait(leave_sem);
	sem_post(hydro_sem);
	sem_wait(leave_sem);

	sem_wait(num_lock);
	num_sulfur -= 1;
	sem_post(num_lock);
	printf("sulfur leaving\n");

	// waits for oxygen atoms to leave
	for (int i = 0; i < 4; i++) {
		sem_post(oxygen_sem);
		sem_wait(leave_sem);
	}

	// allow other sulfur molecules to check for enough atoms
	// delay(rand()%2000);
	sem_post(sulfur_lock);
	return (void*)NULL;
}

// opens the semaphores and links them to the global variables
void openSems() {
	// dynamically allocates memory for the semaphors and their respective
	// names based on how many there are
	sems = (sem_t**) malloc( sizeof(sem_t*) * num_sems );
	sem_names = (char**) malloc( sizeof(char*) * num_sems );

	sem_names[0] = "moleculelock";
	sem_names[1] = "numlock";

	sem_names[2] = "oxygensmphr";
	sem_names[3] = "hydrosmphr";
	sem_names[4] = "sulfursmphr";

	sem_names[5] = "leavesmphr";

	for (int i = 0; i < num_sems; i++) {
		// initializes the lock semaphores with value 1, and the rest with value 0
		if (i < 3)
			sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0644, 1);
		else
			sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0644, 0);

		// ensures semaphores opened correctly
		while (sems[i] == SEM_FAILED) {
			// unlinks and reopens semaphore if it was already opened
			if (errno == EEXIST) {
				printf("semaphore %s already exists, unlinking and reopening\n", sem_names[i]);
				sem_unlink(sem_names[i]);

				// initializes the lock semaphores with value 1, and the rest with value 0
				if (i < 3)
					sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0644, 1);
				else
					sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0644, 0);

			// exits if the semaphore could not be opened
			} else {
				printf("semaphore could not be opened, error # %d\n", errno);
				exit(1);
			}
		}
	}

	// associates global variables with the newly opened semaphores
	molecule_lock = sems[0];
	num_lock = sems[1];
	sulfur_lock = sems[2];

	oxygen_sem = sems[3];
	hydro_sem = sems[4];

	leave_sem = sems[5];
}

// closes and unlinks all the semaphores
void closeSems() {
	for (int i = 0; i < num_sems; i++) {
		sem_close(sems[i]);
		sem_unlink(sem_names[i]);
	}
}

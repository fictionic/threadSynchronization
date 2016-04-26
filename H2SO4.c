#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "H2SO4.h"

sem_t *molecule_lock, *num_lock;
sem_t *oxygen_sem, *hydro_sem, *sulfur_sem;
int num_sems = 5;

int num_oxygen = 0;
int num_hydrogen = 0;
int num_sulfur = 0;

sem_t** sems;
char** sem_names;

void formMolecule();

void formMolecule() {
	sem_wait(num_lock);
	if (num_oxygen >= 4 && num_hydrogen >= 2 && num_sulfur >= 1) {
		sem_post(num_lock);

		printf("== molecule created! %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
		fflush(stdout);

		for (int i = 0; i < 2; i++) {
			sem_post(hydro_sem);
		}

		sem_post(sulfur_sem);

		for (int i = 0; i < 4; i++) {
			sem_post(oxygen_sem);
		}

		sem_wait(num_lock);
		num_oxygen -= 4;
		num_hydrogen -= 2;
		num_sulfur -= 1;
		sem_post(num_lock);

	} else {
		sem_post(num_lock);
	}
}

void* oxygen(void* args) {
	sem_wait(num_lock);
	num_oxygen++;
	sem_post(num_lock);

	printf("oxygen created!   %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);

	sem_wait(molecule_lock);
	formMolecule();
	sem_post(molecule_lock);

	// printf("oxygen about to wait   * %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	sem_wait(oxygen_sem);
	printf("oxygen used up!   ** %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);
	return (void*)NULL;
}

void* hydrogen(void* args) {
	sem_wait(num_lock);
	num_hydrogen++;
	sem_post(num_lock);

	printf("hydrogen created! %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);

	sem_wait(molecule_lock);
	formMolecule();
	sem_post(molecule_lock);

	sem_wait(hydro_sem);
	printf("hydrogen used up! ** %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);
	return (void*)NULL;
}

void* sulfur(void* args) {
	sem_wait(num_lock);
	num_sulfur++;
	sem_post(num_lock);

	printf("sulfur created!   %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);

	sem_wait(molecule_lock);
	formMolecule();
	sem_post(molecule_lock);

	sem_wait(sulfur_sem);
	printf("sulfur used up!   ** %d %d %d\n", num_hydrogen, num_sulfur, num_oxygen);
	fflush(stdout);
	return (void*)NULL;
}

void openSems() {
	printf("currently in openSems()\n");
	fflush(stdout);

	sems = (sem_t**) malloc( sizeof(sem_t*) * num_sems );
	sem_names = (char**) malloc( sizeof(char*) * num_sems );

	sem_names[0] = "moleculelock";
	sem_names[1] = "numlock";

	sem_names[2] = "oxygensmphr";
	sem_names[3] = "hydrosmphr";
	sem_names[4] = "sulfursmphr";

	for (int i = 0; i < num_sems; i++) {
		if (i < 2)
			sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0466, 1);
		else
			sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0466, 0);

		while (sems[i] == SEM_FAILED) {
			if (errno == EEXIST) {
				printf("semaphore %s already exists, unlinking and reopening\n", sem_names[i]);
				fflush(stdout);
				sem_unlink(sem_names[i]);

				if (i < 4)
					sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0466, 1);
				else
					sems[i] = sem_open(sem_names[i], O_CREAT|O_EXCL, 0466, 0);

			} else {
				printf("semaphore could not be opened, error # %d\n", errno);
				fflush(stdout);
				exit(1);
			}
		}
	}

	molecule_lock = sems[0];
	num_lock = sems[1];

	oxygen_sem = sems[2];
	hydro_sem = sems[3];
	sulfur_sem = sems[4];
}

void closeSems() {
	printf("currently in closeSems()\n");

	for (int i = 0; i < num_sems; i++) {
		sem_close(sems[i]);
		sem_unlink(sem_names[i]);
	}
}

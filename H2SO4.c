#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "H2SO4.h"

sem_t* oxygen_sem, *hydro_sem, *sulfur_sem;
int num_sems = 3;

sem_t** sems;
char** sem_names;

void* oxygen(void* args) {
	printf("currently in oxygen()\n");
	return (void*)NULL;
}

void* hydrogen(void* args) {
	printf("currently in hydrogen()\n");
	return (void*)NULL;
}

void* sulfur(void* args) {
	printf("currently in sulfur()\n");
	return (void*)NULL;
}

void openSems() {
	printf("currently in openSems()\n");

	oxygen_sem = sem_open("oxygensmphr", O_CREAT|O_EXCL, 0466, 0);
	hydro_sem = sem_open("hydrosmphr", O_CREAT|O_EXCL, 0466, 0);
	sulfur_sem = sem_open("sulfursmphr", O_CREAT|O_EXCL, 0466, 0);

	sems = (sem_t**) malloc( sizeof(sem_t*) * num_sems );
  sems[0] = oxygen_sem;
	sems[1] = hydro_sem;
	sems[2] = sulfur_sem;

	sem_names = (char**) malloc( sizeof(char*) * num_sems );
	sem_names[0] = "oxygensmphr";
	sem_names[0] = "hydrosmphr";
	sem_names[0] = "sulfursmphr";

	for (int i = 0; i < num_sems; i++) {
		while (sems[i] == SEM_FAILED) {
			if (errno == EEXIST) {
				printf("semaphore %s already exists, unlinking and reopening\n", sem_names[i]);
				fflush(stdout);
				sem_unlink(sem_names[i]);
				hydro_sem = sem_open(sem_names[i], O_CREAT|O_EXCL, 0466, 0);
			} else {
				printf("semaphore could not be opened, error # %d\n", errno);
				fflush(stdout);
				exit(1);
			}
		}
	}
}

void closeSems() {
	printf("currently in closeSems()\n");

	for (int i = 0; i < num_sems; i++) {
		sem_close(sems[i]);
		sem_unlink(sem_names[i]);
	}
}

int main() {
	openSems();
	closeSems();

	return 0;
}

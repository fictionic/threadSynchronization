#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <time.h>
#include "H2SO4.h"


void* oxygen(void*) {
	printf("currently in oxygen()\n");
	return (void*)NULL;
}

void* hydrogen(void*) {
	printf("currently in hydrogen()\n");
	return (void*)NULL;
}

void* sulfur(void*) {
	printf("currently in sulfur()\n");
	return (void*)NULL;
}

void openSems() {
	printf("currently in openSems()\n");
	return (void*)NULL;
}

void closeSems() {
	printf("currently in closeSems()\n");
	return (void*)NULL;
}

int main() {
	return 0;
}

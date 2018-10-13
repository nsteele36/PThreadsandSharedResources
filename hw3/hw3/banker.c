//Homework 3
//Programmed by: Nicholas Steele
//CS4348.0U1

#include "pthread.h"
#include "semaphore.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "ctype.h"

/* these may be any values >= 0 */
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3
/* the available amount of each resource */
int available[NUMBER_OF_RESOURCES];
/*the maximum demand of each customer */
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
/* the amount currently allocated to each customer */
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
/* the remaining need of each customer */
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

pthread_t *customerThreads;
pthread_t bankerThread;
pthread_mutex_t mutexLock;
sem_t customerSem;
sem_t bankerSem;
int semVal;
int NUMBER_OF_R1, NUMBER_OF_R2, NUMBER_OF_R3;

int safeState(int customer_num, int request[]);
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
int validateInput(char**, int);
void createCustomerThread();
void createBankerThread();
void mergeBanker();
void mergeCustomers();
void *customer(void *);
void *banker(void *);

int main(int argc, char* argv[])
{
	if (argc != 4) {
		printf("ERROR: Not enough arguments!");
		return -1;
	}
	NUMBER_OF_R1 = validateInput(argv, 1);
	NUMBER_OF_R2 = validateInput(argv, 2);
	NUMBER_OF_R3 = validateInput(argv, 3);
	available[0] = NUMBER_OF_R1;
	available[1] = NUMBER_OF_R2;
	available[2] = NUMBER_OF_R3;

	for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		maximum[i][0] = NUMBER_OF_R1;
		maximum[i][1] = NUMBER_OF_R2;
		maximum[i][2] = NUMBER_OF_R3;
		allocation[i][0] = 0;
		allocation[i][1] = 0;
		allocation[i][2] = 0;
		need[i][0] = 0;
		need[i][1] = 0;
		need[i][2] = 0;
	}

	//initalize the semaphores
	sem_init(&customerSem, 0, 1);
	sem_init(&bankerSem, 0, 1);

	createBankerThread();
	createCustomerThread();
	mergeBanker();
	mergeCustomers();

	//destroy semaphores
	sem_destroy(&customerSem);
	sem_destroy(&bankerSem);
}

int request_resources(int customer_num, int request[])
{
	for (int i = 0; i < (sizeof(request) / sizeof(request[0])); i++)
	{
		if (request[i] > available[i])
			return -1;

		if (request[i] > maximum[customer_num][i])
			return -1;
	}

	if (safeState(customer_num, request) == -1)
		return -1;

	for (int i = 0; i < (sizeof(request) / sizeof(request[0])); i++)
	{
		available[i] -= available[i];
		allocation[customer_num][i] += available[i];
		need[customer_num][i] -= request[i];
	}

	return 0;
}

int release_resources(int customer_num, int release[])
{
	for (int i = 0; i < (sizeof(release) / sizeof(release[0])); i++)
	{
		available[i] += release[i];
		allocation[customer_num][i] -= release[i];
	}

	return 0;
}

int safeState(int customer_num, int request[])
{
	int tempAvailable[NUMBER_OF_CUSTOMERS];
	int tempAllocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
	memcpy(available, tempAvailable, sizeof(available));
	memcpy(allocation, tempAllocation, sizeof(allocation));
	for (int i = 0; i < (sizeof(tempAvailable) / sizeof(tempAvailable[0])); i++)
	{
		if (request[i] > tempAvailable[i])
			return -1;
	}

	for (int i = 0; i < (sizeof(tempAvailable) / sizeof(tempAvailable[0])); i++)
	{
		tempAvailable[i] -= request[i];
		tempAllocation[customer_num][i] += request[i];
	}

	int finish[NUMBER_OF_CUSTOMERS];

	for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
		finish[i] = -1;

	for (int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		for (int j = 0; j < NUMBER_OF_CUSTOMERS; j++)
		{
			if (finish[j] == -1)
			{
				bool check = true;
				for (int k = 0; k < (sizeof(tempAvailable) / sizeof(tempAvailable[0])); i++)
				{
					if (!((maximum[j][k] - tempAllocation[j][k]) > tempAvailable[k]))
					{
						finish[j] = 0;
						for (int l = 0; l < (sizeof(tempAvailable) / sizeof(tempAvailable[0])); l++)
						{
							tempAvailable[l] += tempAllocation[j][l];
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < (sizeof(tempAvailable) / sizeof(tempAvailable[0])); i++)
		tempAllocation[customer_num][i] -= request[i];

	for (int i = 0; i < (sizeof(finish) / sizeof(finish[0])); i++)
	{
		if (finish[i] == -1)
			return -1;
	}
	return 0;
}

//validates user inputted integers
int validateInput(char* argv[], int index)
{
	bool isValid = true;
	for (int i = 0; i < strlen(argv[index]); i++) {
		if (!isdigit(argv[index][i]))
			isValid = false;
	}
	if (isValid) {
		if (atoi(argv[index]) > 0)
			return atoi(argv[index]);
		else {
			printf("ERROR: Value passed is not positive!\nPlease enter a valid positive integer into command line.\n");
			return -1;
		}
	}
	else {
		printf("ERROR: Value passed must be an integer!\nPlease enter a valid positive integer into command line.\n");
		return -1;
	}
}

//routine done by the customers
void * customer(void * p)
{
	//get initial values
	int i = *((int *)p);

	int request[NUMBER_OF_RESOURCES];
	if (i == 0)
	{
		request[0] = rand() % NUMBER_OF_R1;
		request[1] = rand() % NUMBER_OF_R2;
		request[2] = rand() % NUMBER_OF_R3;
	}
	else
	{
		request[0] = (need[i][0] = maximum[i][0] - allocation[i][0]);
		request[1] = (need[i][1] = maximum[i][1] - allocation[i][1]);
		request[2] = (need[i][2] = maximum[i][2] - allocation[i][2]);
	}
	//print for how long the the student is programming for
	printf("Customer %d requesting resources\n\tRequesting:\n\tResource A: %d\n\tResource B: %d\n\tResource C: %d\n", i, request[0], request[1], request[2]);

	//Unlock the mutex
	pthread_mutex_unlock(&mutexLock);
	//lock the mutex
	pthread_mutex_lock(&mutexLock);

	//Get the value of the semaphore 
	sem_getvalue(&bankerSem, &semVal);

	pthread_mutex_unlock(&mutexLock);

	//if the semVal is over 0 than that means that students are still getting help
	if (semVal > 0) {
		sem_wait(&bankerSem);

		//Lock the mutex
		pthread_mutex_lock(&mutexLock);


	}

	//perform the following opeartion if the semaphore value is less than 0
	else
		sem_post(&customerSem);

	//Call the students()
	customer((void*)&i);
}

void * banker(void * p)
{
	printf("banker\n");
}

//makes a teachers assistant thread
void createBankerThread()
{
	if (pthread_create(&bankerThread, NULL, banker, NULL))
		printf("TA thread creation error\n");
}

void createCustomerThread()
{
	int i;
	//creates a single thread for each student
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++) {
		int *a = malloc(sizeof(int));
		*a = i;
		if (pthread_create(&customerThreads[i], NULL, customer, (void *)a))
			printf("ERROR: could not create customer \n");
		free(a);
	}

	//creates a teachers assistant thread
	if (pthread_create(&bankerThread, NULL, banker, NULL))
		printf("ERROR: could not create TA thread\n");
}

//merges teachers assistant threads
void mergeBanker()
{
	if (pthread_join(bankerThread, NULL))
		printf("ERROR: could not join TA thread!\n");
}

//merges student threads
void mergeCustomers()
{
	// this function joins all the student thread
	int i;
	for (i = 0; i < NUMBER_OF_CUSTOMERS; i++)
	{
		if (pthread_join(customerThreads[i], NULL))
			printf("ERROR: could not join student threads!\n");
	}

}
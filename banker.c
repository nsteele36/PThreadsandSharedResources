//Homework 3
//Programmed by: Nicholas Steele
//CS4348.0U1

//preprocessor directives
#include "pthread.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "stdlib.h"
#include "unistd.h"
#include "ctype.h"

//macros
#define NUMBER_OF_CUSTOMERS 5
#define NUMBER_OF_RESOURCES 3

//global data structures and variables

//used for bankers alogrithm
int request[NUMBER_OF_RESOURCES];
int available[NUMBER_OF_RESOURCES];
int maximum[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int allocation[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];
int need[NUMBER_OF_CUSTOMERS][NUMBER_OF_RESOURCES];

//array holding customer threads and a mutex lock
pthread_t customerThreads[NUMBER_OF_CUSTOMERS];
pthread_mutex_t banker;

//function prototypes
int safeState();
int request_resources(int customer_num, int request[]);
int release_resources(int customer_num, int release[]);
int validateInput(char**, int);
void createCustomerThread();
void mergeCustomers();
void *customer(void *);

int main(int argc, char* argv[])
{
	if(argc != 4)
	{
		printf("ERROR: Not enough arguments!\n");
		return -1;
	}

	//initalize the max, need, allocation and avaiable data structures
    for (int i = 0; i < NUMBER_OF_RESOURCES; i++) 
	{
		//validate the input of the command line arguments and save them as the max available value for each resource
        available[i] = validateInput(argv, (i+1));

        //initalize the max and need from a customer to a random value
		//set the starting allocation to 0 since no one has requested resources
        for(int j = 0; j < NUMBER_OF_CUSTOMERS; j++) 
		{
			allocation[j][i] = 0;
            maximum[j][i] = rand() % available[i];
			need[j][i] = maximum[j][i];
        }        
    }

	//initialize mutex lock 
    pthread_mutex_init(&banker, NULL);

	//initialize pthreads
	createCustomerThread();
	
	//merge pthreads
	mergeCustomers();

	//program will never reach past this point
	//destroy mutex lock and exit 
    pthread_mutex_destroy(&banker);
    return 0;
}

//creates customer pthread
void createCustomerThread()
{
   int i;
   //creates a single thread for each student
   for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
   {
        int *a = malloc(sizeof(int));
        *a = i;
        if(pthread_create(&customerThreads[i], NULL, customer, (void *)a))
           printf("ERROR: could not create customer \n");

		free(a);
   }
}

//merges customer pthreads
void mergeCustomers()
{
	int i;
	for(i = 0; i < NUMBER_OF_CUSTOMERS; i++)
		if(pthread_join(customerThreads[i], NULL))
			printf("ERROR: could not join student threads!\n");

}

//routine done by the customers
void * customer(void * p)
{
	//get the customer number
    int customerNum = *(int*)p;
    bool acquiredResources = false;

    //the arrays of the resources for the specific customer
    int request [NUMBER_OF_RESOURCES];
    
	//infinite loop
    while(true)
	{
		//request a random number of resources using the maximum array
        for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
            request[i] = rand() % (maximum[customerNum][i] + 1); 

		//lock the banker mutex lock
        pthread_mutex_lock(&banker);
        
		//attempt to get resources
        acquiredResources = request_resources(customerNum,request);

		//unlock the banker mutex lock
        pthread_mutex_unlock(&banker);

		//if the resource request is granted, then use resources and releas them
        if(acquiredResources == 0)
		{
			//simulate using resources
            sleep(rand() % 3 + 1);

			//reset flag
            acquiredResources = -1;
			
			//lock the mutex lock
            pthread_mutex_lock(&banker);

			//attempt to release resources
            release_resources(customerNum,request); //release the resources used

			//unlock the mutex lock
            pthread_mutex_unlock(&banker);
        }
        //sleep for a random time
		sleep(rand() % 3 + 1);
    }
    return 0;
}

//requests resources and allocates them after checking if the system is in a safe state
int request_resources(int customer_num, int request[])
{	
	//print the resource request
    printf("\n* Customer %d is requesting the following resources from the banker: \n", customer_num + 1);
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
        printf("\tResource %d: %d\n", i + 1, request[i]);

	//print the available resource before the request
    printf("\nSYSTEM: Available resources BEFORE resource request from customer %d:\n", customer_num + 1);
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
        printf("\tResource %d: %d\n", i + 1, available[i]);
    
	//checks if the system is in a safe state after request
	for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
		//system is only safe if the request is less than or equal to the need
        if(request[i] <= need[customer_num][i])
		{
			//if the request is larger than the resources available then reject the request
            if(request[i] > available[i])
			{
                printf("\n$ Banker rejects request. Request leaves system in unsafe state.\n");

                //sleep for a random time
                sleep(rand() % 3 + 1);

                //exit error
                return -1;
            }

			//banker continues to check request
            else
			{
                printf("\n$ Banker is verifying the request... ");

				//calculate the allocation, available and need data structures after customer request
                for(int k = 0; k < NUMBER_OF_RESOURCES; k++)
				{
					need[customer_num][k] -= request[k];
					available[k] -= request[k];
                    allocation[customer_num][k] += request[k];
                }
                
				//check if the system is in a safe state
				//if it is tell the user
				if(safeState() == 0)
				{
                    printf("System is safe!\n\tResource request granted!\n");

					//print the available resources after the customer request
                    printf("\nSYSTEM: Available resources AFTER resource request from customer %d:\n", customer_num + 1);
                    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
                        printf("\tResource %d: %d\n", i + 1, available[i]);
                    
                    //sleep for a random time
                    sleep(rand() % 3 + 1);

					//exit success
                    return 0;
                }
                
				//if the system is not safe tell the user
				//rollback the available, need, and allocation data structures to their safe state
				else
				{
                    printf("System is unsafe!!\n\tResources request rejected!\n");
					
					//calculate the original values of the available, need, and allocation data structures
                    for(int k = 0; k < NUMBER_OF_RESOURCES; k++)
					{
						need[customer_num][k] += request[k];
						available[k] += request[k]; 
						allocation[customer_num][k] -= request[k];
                    }

					//exit error
                    return -1;
                }
            }
        }
		//customer is requesting more than they need 
		else
		{
            printf("\n* Customer %d is requesting more than they need! Request rejected!\n", customer_num + 1);

			//sleep for a random time
            sleep(rand() % 3 + 1);

			//exit error
            return -1;
        }
    }
 
	//exit success
    return 0;
}

//releases resources used by customers
int release_resources(int customer_num, int release[])
{
	//print the resources that are being released by the customers
    printf("\n* Customer %d is releasing the following resources:\n", customer_num + 1);
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
	{
        printf("\tResource %d: %d\n", i + 1, release[i]);

		//add release resources to the need and available resources
		need[customer_num][i] += release[i]; 
        available[i] += release[i]; 

		//subtract release resources from allocation since they are no longer being used
        allocation[customer_num][i] -= release[i]; 
    }

	//print the available resources after the customer request
    printf("\nSYSTEM: Available resources AFTER release request from customer %d:\n", customer_num + 1);
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
        printf("\tResource %d: %d\n", i + 1, available[i]);
    
	//sleep for a random time
    sleep(rand() % 3 + 1);
    
	//exit success
	return 0;
}

//checks if a request makes the system safe or unsafe
int safeState()
{	
	//initialize counters
    int index = 0, 
		counter1 = -1, 
		counter2 = -1;

    bool safeFlag = true;

	//create a copy of the available data structure
    int tempAvailable[NUMBER_OF_RESOURCES];
    for(int i = 0; i < NUMBER_OF_RESOURCES; i++)
        tempAvailable[i] = available[i];
	
    //a bit map that holds the values to tell if all the customers are safe or not
    int safe [NUMBER_OF_CUSTOMERS];
    for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
        safe[i] = -1;

    //check if all customers are in a safe state
    while (index < NUMBER_OF_CUSTOMERS)
	{
        counter2 = counter1;

		//loop through the customers
        for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
		{
            //if the customer is not done, check to see if it can be completed
			//check if the request by the customer is safe
            if(safe[i] == -1)
			{
				
				//loop through the resources
                for(int j = 0; j < NUMBER_OF_RESOURCES; j++)
					//if the need is greater than the available, then system is not safe
                    if (need[i][j] > tempAvailable[j])
                        safeFlag = false;
                
				
				//if the system is safe after the request, set the bit map value to true
                if(safeFlag)
				{
					//set counter to the index since this customer is safe
                    counter1 = i;

					//add the allocated resources back to the temporary available array
                    for(int j = 0; j < NUMBER_OF_RESOURCES; j++)
                        tempAvailable[j] += allocation[i][j];
                    
					//set the safe flags to true
                    safe[i] = 0;
					safeFlag = true;

					//increment index
                    index++;
                    break;
                }
            }
        }
		
		//check if the sytem is safe
        for(int i = 0; i < NUMBER_OF_CUSTOMERS; i++)
            if(safe[i] == -1)
                break;
        
        //if the safe state counters were unchanged then return false since system is in unsafe sate
        if(counter2 == counter1)
			//exit error
            return -1;
    }

	//system is fully check and safe
	//exit success
    return 0;
}

//validates user inputted integers
int validateInput(char* argv[], int index)
{
    bool isValid = true;
    for( int i = 0; i < strlen(argv[index]); i++)     
        if(!isdigit(argv[index][i]))
            isValid = false;
    
    if(isValid)
	{
        if(atoi(argv[index]) > 0)
            return atoi(argv[index]);
        else
		{
            printf("ERROR: Value passed is not positive!\nPlease enter a valid positive integer into command line.\n");
            //exit error
            return -1;
        }
    }
    else
	{
        printf("ERROR: Value passed must be an integer!\nPlease enter a valid positive integer into command line.\n");
        //exit error
        return -1;
    }
}


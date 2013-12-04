/*
  Josh Tan
  CSCI 474
  Project 2: Hotel Simulation
  12/6/13
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define TOTAL_GUESTS 10
#define TOTAL_ROOMS 5
#define SEM_SHARED 0

// activities for customers and check-in/out reservationists
void *guest_activity(void *);
void *check_in_activity(void *);
void *check_out_activity(void *);

// mutual exclusion for reservationists
sem_t check_in_res_avail, check_out_res_avail;

// event ordering for check-in
sem_t guest_at_check_in, res_assigned_room;

// even ordering for check-out
sem_t guest_at_check_out, res_calc_balance, guest_pay_balance;

// limit available rooms and concurrent modification of room status
sem_t room_available, can_modify_room;

// reservation consists of guest ID and room number
struct reservation
{
    int	guest_id;
    int room_num;
};

// guest reservation being processed
struct reservation guest_reservation;

// room availability
int rooms[TOTAL_ROOMS];

int main(int argc, char *argv[])
{
    pthread_t guests[TOTAL_GUESTS];
    pthread_t check_in_res;
    pthread_t check_out_res;
    pthread_attr_t attr;

    int thread_error, guest_id, join_status;

    // initalize semaphores
    sem_init(&check_in_res_avail, SEM_SHARED, 1); // 1 = mutual-exclusion
    sem_init(&check_out_res_avail, SEM_SHARED, 1);

    // initialize and set thread detached attribute for reservationist threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // create check-in reservationist
    thread_error = pthread_create(&check_in_res, &attr, check_in_activity, (void *)TOTAL_GUESTS);
    if (thread_error)
    {
	printf("Error create check-in reservationist.\n");
	exit(EXIT_FAILURE);
    }

    // create check-out reservationist
    thread_error = pthread_create(&check_out_res, &attr, check_out_activity, (void *)TOTAL_GUESTS);
    if (thread_error)
    {
	printf("Error create check-out reservationist.\n");
	exit(EXIT_FAILURE);
    }

    // free attribute
    pthread_attr_destroy(&attr);

    sleep(.5); // testing purposes

    // create guests
    for (guest_id = 0; guest_id < TOTAL_GUESTS; guest_id++)
    {
	thread_error = pthread_create(&guests[guest_id], NULL, guest_activity, (void *)guest_id);
	if (thread_error)
	{
	    printf("Error creating guest %d.\n", guest_id);
	    exit(EXIT_FAILURE);
	}

	// use sem_wait(&empty) and sem_post(&full);
    }

    // wait for all guests to check out
    thread_error = pthread_join(check_out_res, (void **)&join_status);
    if (thread_error)
    {
	printf("Error waiting for all guests to check out.\n");
	exit(EXIT_FAILURE);
    }

    sleep(1.5); // testing purposes
    printf("Program finished.\n");
    exit(EXIT_SUCCESS);
}

void *guest_activity(void *cust_id)
{
    printf("Guest %d, ready to throw down!\n", (int)cust_id + 1);
    pthread_exit(NULL);
}

void *check_in_activity(void *total_guests)
{
    int guest_processed;
    printf("Hi, welcome to Nocturnal Ned's!\n");
    pthread_exit(NULL);
}

void *check_out_activity(void *total_guests)
{
    sleep(1); // testing purposes
    printf("Thanks for staying at Nocturnal Ned's!\n");
    pthread_exit(NULL);
}

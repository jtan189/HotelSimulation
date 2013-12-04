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
typedef struct
{
    int guest_id;
    int room_num;
} reservation;

// guest reservation being processed by check-in reservationist
reservation check_in_reservation;

// guest reservation being processed by check-out reservationist
reservation check_out_reservation;

// room availability
int rooms[TOTAL_ROOMS];

int main(int argc, char *argv[])
{
    pthread_t guests[TOTAL_GUESTS];
    pthread_t check_in_res;
    pthread_t check_out_res;
    pthread_attr_t attr;

    int thread_error, guest_id, join_status;

    printf("Hotel simulation starting...\n");

    // initalize semaphores
    sem_init(&check_in_res_avail, SEM_SHARED, 1); // 1 = mutual exclusion
    sem_init(&check_out_res_avail, SEM_SHARED, 1);
    sem_init(&can_modify_room, SEM_SHARED, 1);
    sem_init(&guest_at_check_in, SEM_SHARED, 0); // 0 = event ordering
    sem_init(&res_assigned_room, SEM_SHARED, 0);
    sem_init(&guest_at_check_out, SEM_SHARED, 0);
    sem_init(&res_calc_balance, SEM_SHARED, 0);
    sem_init(&guest_pay_balance, SEM_SHARED, 0);
    sem_init(&room_available, SEM_SHARED, TOTAL_ROOMS); // limit concurrent reservations

    // initially, all rooms vacant
    rooms = {0};

    // initialize and set thread detached attribute for reservationist threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // create check-in reservationist
    thread_error = pthread_create(&check_in_res, &attr, check_in_activity, (void *)TOTAL_GUESTS);
    if (thread_error)
    {
	printf("Error creating check-in reservationist.\n");
	exit(EXIT_FAILURE);
    }

    // create check-out reservationist
    thread_error = pthread_create(&check_out_res, &attr, check_out_activity, (void *)TOTAL_GUESTS);
    if (thread_error)
    {
	printf("Error creating check-out reservationist.\n");
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

    }

    // wait for all guests to check out
    thread_error = pthread_join(check_out_res, (void **)&join_status);
    if (thread_error)
    {
	printf("Error waiting for all guests to check out.\n");
	exit(EXIT_FAILURE);
    }

    sleep(1.5); // testing purposes
    printf("Hotel simulation finished.\n");
    exit(EXIT_SUCCESS);
}

void *guest_activity(void *cust_id)
{
    printf("Guest %d, ready to throw down!\n", (int)cust_id + 1);

    // wait for check-in reservationist to become available
    sem_wait(&check_in_res_avail);

    pthread_exit(NULL);
}

void *check_in_activity(void *total_guests)
{
    int guests_processed = 0;

    while(guests_processed < TOTAL_GUESTS)
    {
	int room_num;

	// wait for guest to arrive before greeting
	sem_wait(&guest_at_check_in);
	printf("The check-in reservationist greets Guest %d.", check_in_reservation->guest_id);

	// find available room and asign to guest
	sem_wait(&can_modify_room);
	room_num = get_avail_room();
	printf("Assign room %d to Guest %d.", room_num, check_in_reservation->guest_id);

	// signal availablility to new guests
	sem_post(&check_in_res_avail);

    }
    

    pthread_exit(NULL);
}

int get_avail_room()
{
    // figure out wait order for can_modify_room and room_available
    int i;
    // get first available room
    for(i = 0; i < TOTAL_ROOMS; i++)
    {
	if (rooms[i] == 0)
	{
	    break;
	}
    }

    return i + 1;
}

void *check_out_activity(void *total_guests)
{
    sleep(1); // testing purposes
    printf("Thanks for staying at Nocturnal Ned's!\n");
    pthread_exit(NULL);
}

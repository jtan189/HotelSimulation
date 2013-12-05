/*
  JOSH Tan
  CSCI 474
  Project 2: Hotel Simulation
  12/6/13

  Resources:
  http://stackoverflow.com/questions/822323/how-to-generate-a-random-number-in-c
  http://stackoverflow.com/questions/2999075/generate-a-random-number-within-range/2999130#2999130
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define TOTAL_GUESTS 10
#define TOTAL_ROOMS 5
#define MAX_SLEEP 3
#define HOTEL_RATE 100
#define SEM_SHARED 0

// activities for customers and check-in/out reservationists
void *guest_activity(void *);
void *check_in_activity(void *);
void *check_out_activity(void *);
int get_avail_room();
int rand_lim(int, unsigned int *);

// mutual exclusion for reservationists
sem_t check_in_res_avail, check_out_res_avail;

// event ordering for check-in
sem_t guest_at_check_in, res_assigned_room;

// event ordering for check-out
sem_t guest_at_check_out, res_calc_balance, guest_pay_balance;

// limit available rooms and concurrent modification of room status
sem_t room_available, can_modify_room;

// reservation information
typedef struct
{
    int guest_id;
    int room_num;
    int duration;
    float balance;
} reservation;

// guest reservation being processed by check-in reservationist
reservation check_in_reservation;

// guest reservation being processed by check-out reservationist
reservation check_out_reservation;

// counter variables
int pool_count, rest_count, fit_count, biz_count;

// occupancy status for each room (0 = vacant, 1 = occupied)
int room_occupancy[TOTAL_ROOMS] = {0};

int main(int argc, char *argv[])
{
    pthread_t guests[TOTAL_GUESTS];
    pthread_t check_in_res; // check-in reservationist
    pthread_t check_out_res; // check-out reservationist
    pthread_attr_t attr;
    int thread_error, guest_id, join_status, check_in_res_id, check_out_res_id;

    // seed random number generator to use as seed for
    // thread random number generator seed (using nrand48)
    //srand(time(NULL));

    // initailize action counts
    pool_count = 0;
    rest_count = 0;
    fit_count = 0;
    biz_count = 0;

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

    // initialize and set thread detached attribute for reservationist threads
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // create check-in reservationist
    check_in_res_id = -1;
    thread_error = pthread_create(&check_in_res, &attr, check_in_activity, (void *)check_in_res_id);
    if (thread_error)
    {
	printf("Error creating check-in reservationist.\n");
	exit(EXIT_FAILURE);
    }

    // create check-out reservationist
    check_out_res_id = -2;
    thread_error = pthread_create(&check_out_res, &attr, check_out_activity, (void *)check_out_res_id);
    if (thread_error)
    {
	printf("Error creating check-out reservationist.\n");
	exit(EXIT_FAILURE);
    }

    // free thread attribute
    pthread_attr_destroy(&attr);

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

    // print simulation stats
    printf("Hotel simulation finished.\n\n");
    printf("\t\t\tNumber of Customers\n");
    printf("Total Guests:\t\t%d\n", TOTAL_GUESTS);
    printf("Pool:\t\t\t%d\n", pool_count);
    printf("Restaurant:\t\t%d\n", rest_count);
    printf("Fitness Center:\t\t%d\n", fit_count);
    printf("Business Center:\t%d\n", biz_count);

    exit(EXIT_SUCCESS);
}

/*
  Activities performed by guests.
  Includes checking in, performing an action within the hotel, and checking out.

  http://man7.org/linux/man-pages/man3/drand48_r.3.html
  http://linux.about.com/library/cmd/blcmdl3_nrand48.htm
  http://stackoverflow.com/questions/6161322/using-stdlibs-rand-from-multiple-threads?lq=1
 */
void *guest_activity(void *guest_id)
{
    int assigned_room_num, action_choice, action_duration;
    float balance_due;
    /* struct drand48_data *seed_buffer; */
    /* double *rand_num; */

    // wait for check-in reservationist to become available
    sem_wait(&check_in_res_avail);
    check_in_reservation.guest_id = (int)guest_id;
    printf("GUEST %d: Guest %d goes to the check-in reservationist.\n",
	   (int)guest_id, (int)guest_id);
    sem_post(&guest_at_check_in);
    
    // wait for check-in reservationist to assign room
    sem_wait(&res_assigned_room);
    assigned_room_num = check_in_reservation.room_num;
    printf("GUEST %d: Guest %d receives Room %d and completes check-in.\n",
	   (int) guest_id, (int)guest_id, assigned_room_num);

    // signal check-in reservationist availablility to new guests
    sem_post(&check_in_res_avail);

    // randomly perform an action within the hotel, for a random amount of time
    //seed48_r((short unsigned int *)guest_id, seed_buffer); // seed using rand()
    //seed48_r((short unsigned int *)rand(), seed_buffer); // seed using rand()
    //drand48_r(seed_buffer, rand_num);
    //action_choice = (int)(*rand_num / 0.25);
    
    action_duration = 1;
    //drand48_r(seed_buffer, rand_num);
    //action_duration = ((int)(*rand_num / ((double)1/(double)3)) + 1);

    //srand(time(NULL)); // seed random number
    unsigned int seedp = (int)time(NULL) + (int)guest_id;
    action_choice = rand_lim(3, &seedp);
    action_duration = rand_lim(MAX_SLEEP - 1, &seedp) + 1;

    // perform action, with a 25% chance of selecting a particular action
    switch(action_choice)
    {
	case 0:
	    printf("GUEST %d: Go to hotel swimming pool.\n", (int) guest_id);
	    pool_count++;
	    break;
	case 1:
	    printf("GUEST %d: Go to hotel restaurant.\n", (int) guest_id);
	    rest_count++;
	    break;
	case 2:
	    printf("GUEST %d: Go to hotel fitness center.\n", (int) guest_id);
	    fit_count++;
	    break;
	case 3:
	    printf("GUEST %d: Go to hotel business center.\n", (int) guest_id);
	    biz_count++;
	    break;
    }

    sleep(action_duration); 
    //printf("GUEST %d: Took %d seconds performing activity.\n", (int) guest_id, action_duration);
    
    // wait for check-out reservation to become available
    sem_wait(&check_out_res_avail);
    check_out_reservation.guest_id = (int)guest_id;
    check_out_reservation.room_num = assigned_room_num;
    check_out_reservation.duration = action_duration;
    printf("GUEST %d: Guest %d goes to the check-out reservationist and returns Room %d.\n",
	   (int)guest_id, (int)guest_id, assigned_room_num);
    sem_post(&guest_at_check_out);

    // wait for check-out reservationist to calculate balance
    sem_wait(&res_calc_balance);
    balance_due = check_out_reservation.balance;
    printf("GUEST %d: Guest %d receives the total balance of $%.2f.\n",
	   (int)guest_id, (int)guest_id, balance_due);

    // pay balance
    printf("GUEST %d: Guest %d makes a payment.\n", (int)guest_id, (int)guest_id);
    sem_post(&guest_pay_balance);

    pthread_exit(NULL);
}

/*
  Activities performed by check-in reservationist.
  Includes greeting guests and guest room number assignment.
 */
void *check_in_activity(void *check_in_res_id)
{
    int guests_processed = 0;

    // check-in TOTAL_GUESTS number of guests
    while(guests_processed < TOTAL_GUESTS)
    {
	int avail_room_num;

	// wait for guest to arrive before greeting
	sem_wait(&guest_at_check_in);
	printf("CHECK-IN: The check-in reservationist greets Guest %d.\n",
	       check_in_reservation.guest_id);

	// wait for a room vacancy
	sem_wait(&room_available);

	// find available room
	sem_wait(&can_modify_room); // lock room status modification during room assignment
	avail_room_num = get_avail_room();

	// assign room to guest
	check_in_reservation.room_num = avail_room_num;
	room_occupancy[avail_room_num ] = 1;
	sem_post(&can_modify_room); // unlock room status modification
	printf("CHECK-IN: Assign Room %d to Guest %d.\n",
	       avail_room_num, check_in_reservation.guest_id);
	sem_post(&res_assigned_room);

	guests_processed++;
    }
    
    pthread_exit(NULL);
}

/*
  Return the room number for a currently vacant room. Gets the first vacant room,
  according to increasing room number.
 */
int get_avail_room()
{
    int i;

    // get first available room
    for(i = 0; i < TOTAL_ROOMS; i++)
    {
	if (room_occupancy[i] == 0)
	{
	    break;
	}
    }

    return i;
}

/*
  Activities performed by check-out reservationist.
  Includes greeting guests, calculating balance for reservations, and receiving guest payment.
 */
void *check_out_activity(void *check_out_res_id)
{
    int guests_processed = 0;

    // check-out TOTAL_GUESTS number of guests
    while(guests_processed < TOTAL_GUESTS)
    {
	// wait for guest to arrive before greeting
	sem_wait(&guest_at_check_out);
	printf("CHECK-OUT: The check-out reservationist greets Guest %d and receives the key for Room %d.\n", 
	       check_out_reservation.guest_id, check_out_reservation.room_num);

	printf("CHECK-OUT: Calculate the balance for Guest %d.\n", check_out_reservation.guest_id);
	float balance_due = check_out_reservation.duration * HOTEL_RATE;
	check_out_reservation.balance = balance_due;
	sem_post(&res_calc_balance);
	sem_wait(&guest_pay_balance);
	printf("CHECK-OUT: Receive $%.2f from Guest %d and complete the check-out.\n",
	       balance_due,check_out_reservation.guest_id);

	sem_wait(&can_modify_room);
	room_occupancy[check_out_reservation.room_num] = 0;
	sem_post(&can_modify_room);
	
	sem_post(&room_available);

	// signal availablility to new guests
	sem_post(&check_out_res_avail);

	guests_processed++;

    }

    pthread_exit(NULL);
}

/*
 * Return a random number between 0 and limit, inclusive.
 *
 * Source:
 * http://stackoverflow.com/questions/2999075/generate-a-random-number-within-range/2999130#2999130
 */
int rand_lim(int limit, unsigned int *seedp) {

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do { 
        retval = rand_r(seedp) / divisor;
    } while (retval > limit);

    return retval;
}

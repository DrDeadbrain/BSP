/** *****************************************************************
 * @file bsp2/general.h
 * @author Christian Caus (christian.caus@haw-hamburg.de)
 * @author Stefan Subotin (stefan.subotin@haw-hamburg.de)
 * @version 1.0
 * @date 08.11.2019
 * @brief Philosophs Workout
 * Mutexes, Semaphores, cond vars
 * Monitor concept
 ********************************************************************
 */


#ifndef GENERAL_H
#define GENERAL_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#define SUCESS 0
#define ERROR -1

#define N_PHIL 5

#define KEYCOMBO 10

//!Helps to cope with ASCII
#define ASCII 48

//loop limits
#define REST_LOOP 1000000000
#define TEST_RL 10
#define WORKOUT_LOOP 10

#define ANNA_ID 0
#define BERND_ID 1
#define CLARA_ID 2
#define DIRK_ID 3
#define EMMA_ID 4

typedef enum {GET_WEIGHTS, WORKOUT, PUT_WEIGHTS, REST} State;


//array for keyinput
extern char keyinput[KEYCOMBO];

//array for transmitting b, u, p to philosophers
extern char listen[N_PHIL];

//mutex with pthreads
extern pthread_mutex_t mutex;

//mutex with pthreads for put weights
extern pthread_mutex_t mutex_pw;

//cond vars with pthreads - one per philosopher
extern pthread_cond_t cond[N_PHIL];

//semaphores one per philosopher
extern sem_t semaphore[N_PHIL];

//philosopher id
extern unsigned int philoIDs[N_PHIL];

//array for philosopher states
extern State philoStates[N_PHIL];

//array for thread status
extern char status[N_PHIL];


extern unsigned int avail[];
extern unsigned int taken[5][3];





//function declaration
void *philothread(void *pID);
void get_weights(int philoID);
void workout (int philoID);
void rest(int philoID);
void put_weights(int philoID);

char convertState(State philoState);
void output(void);

#endif

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

#define SUCESS 0
#define ERROR -1

#define N_PHIL 5

#define KEYCOMBO 10

//!Helps to cope with ASCII
#define ASCII 48

//loop limits
#define REST_LOOP 1000000000
#define WORKOUT_LOOP 500000000

#define RIGHT_NEIGHB(philoID) ((philoID == N_PHIL-1) ? 0 : philoID+1)
#define LEFT_NEIGHB(philoID) ((philoID == 0)? N_PHIL-1 : philoID-1)

#define ANNA_ID 0
#define BERND_ID 1
#define CLARA_ID 2
#define DIRK_ID 3
#define EMMA_ID 4

typedef enum {GET_WEIGHTS, WORKOUT, PUT_WEIGHTS, REST} State;

//array for keyinput
char keyinput[KEYCOMBO];

//array for transmitting b, u, p to philosophers
char listen[N_PHIL];

//mutex with pthreads
pthread_mutex_t mutex;

//cond vars with pthreads - one per philosopher
pthread_cond_t cond[N_PHIL];

//semaphores one per philosopher
sem_t semaphore[N_PHIL];

//philosopher id
int philoID[N_PHIL] = {ANNA_ID, BERND_ID, CLARA_ID, DIRK_ID, EMMA_ID};

//array for philosopher states
State philoStates[N_PHIL];

//array for thread status
char status[N_PHIL];








//function declaration
void philothread(void *arg);
void get_weights(int philoID);
void workout (int philoID);
void rest(int philoID);
void put_weights(int philoID);

char convertState(State philoState);
void output(void);

#endif

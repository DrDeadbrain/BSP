#ifndef _PHILOTHREAD_H_
#define __PHILOTHREAD_H_
#include "semaphore.h"
#include <stdbool.h>

/**
 * global vars etc.
 */
#define PHIL_T 5
#define INPUT_SIZE 200
#define KEYMAX 5
#define NO_INPUT '0'

#define SEM_TH 0
#define SEM_M 0

#define ANNA_W 6
#define BERND_W 8
#define CLARA_W 12
#define DIRK_W 12
#define EMMA_W 14


#define REST_LOOP 100000000
#define WORKOUT_LOOP 50000000
#define INIT_THREAD \
      { .ID = 0, .weightNeeded = 0, .status_inf = '0', .state = REST, .wStack = EMPTY }

typedef enum philoThreadStates
{
  GET_WEIGHTS,
  WORKOUT,
  PUT_WEIGHTS,
  REST
} State_p;

#define DEFAULT \
      { .twoKG = 4, .threeKG = 4, .fiveKG = 5 }
#define EMPTY \
      { .twoKG = 0, .threeKG = 0, .fiveKG = 0 }

typedef struct weightStack_struct
{
  int twoKG;
  int threeKG;
  int fiveKG;
} Weights_s;

typedef struct philoThread_args
{
  int ID;
  char name[10];
  int weightNeeded;
  char status;
  sem_t *semaphore_threadID;
  //pthread_barrier_t *barrier;
} Thread_arguments;

typedef struct thread_informations
{
  int ID;
  int weightNeeded;
  char status_inf;
  State_p state;
  Weights_s wStack;
} Thread_info;


void *philoThread (void *input_arguments);
void *thread_dependencies (void *);
Thread_info thread_info_init (Thread_arguments currArgs, State_p currState,
			      Weights_s currStack);
char convertForOutput (State_p state);


char readInput (int threadID);
void writeInput (int threadID, char value);



#endif

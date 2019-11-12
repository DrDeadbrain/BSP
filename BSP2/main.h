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

typedef enum philoThreadStates {
      GET_WEIGHTS,
      WORKOUT,
      PUT_WEIGHTS,
      REST
} state_p;

#define DEFAULT \
      { .twoKG = 4, .threeKG = 4, .fiveKG = 5 }
#define EMPTY \
      { .twoKG = 0, .threeKG = 0, .fiveKG = 0 }

typedef struct weightStack_struct {
      int twoKG;
      int threeKG;
      int fiveKG;
} weights_s;

typedef struct philoThread_args {
      int ID;
      char name[10];
      int weightNeeded;
      char status;
      sem_t *semaphore_threadID;
      //pthread_barrier_t *barrier;
} thread_arguments;

typedef struct thread_informations {
      int ID;
      int weightNeeded;
      char status_inf;
      state_p state;
      weights_s wStack;
} thread_info;


char convertForOutput(state_p state);
void *thread_dependencies(void *);



void monitor_destroy();
void get_weights(int needed_weights, weights_s *input, thread_info threadInfo);
char readInput(int threadID);
void writeInput(int threadID, char value);
void displayInfoStatus(thread_info info);
thread_info thread_info_init(thread_arguments currArgs, state_p currState, weights_s currStack);
void initMonitor();
void put_weights(weights_s *userWStack, thread_info threadInfo);
void logError(int val);

int getTotalWeight(weights_s wStack);
void copyWeight(weights_s *from, weights_s *to);
bool checkWeight(int totalWeight, weights_s *from, weights_s *to);

#endif

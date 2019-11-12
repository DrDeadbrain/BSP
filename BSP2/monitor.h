#ifndef __MONITOR_H__
#define __MONITOR_H__

#include <stdbool.h>

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

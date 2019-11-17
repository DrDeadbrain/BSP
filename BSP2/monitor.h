#ifndef __MONITOR_H__
#define __MONITOR_H__


/**
 * mutex for input in keyPad vector
 */
static pthread_mutex_t keyPad_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * input vector for key inputs
 */
static char KPAD[PHIL_T];

void monitor_destroy();
void get_weights(int needed_weights, weights_s *input, thread_info threadInfo);

void displayInfoStatus(thread_info info);
void initMonitor();
void put_weights(weights_s *userWStack, thread_info threadInfo);

int getTotalWeight(weights_s wStack);
void copyWeight(weights_s *from, weights_s *to);
bool checkWeight(int totalWeight, weights_s *from, weights_s *to);

#endif

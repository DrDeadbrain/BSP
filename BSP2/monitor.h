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

void monitor_destroy ();
void get_weights (int needed_weights, Weights_s * input,
		  Thread_info threadInfo);

void displayInfoStatus (Thread_info info);
void initMonitor ();
void put_weights (Weights_s * userWStack, Thread_info threadInfo);

int getTotalWeight (Weights_s wStack);
void copyWeight (Weights_s * from, Weights_s * to);
bool checkWeight (int totalWeight, Weights_s * from, Weights_s * to);

#endif

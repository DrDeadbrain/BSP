
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "monitor.h"


/**
 * mutex for get_weights and put_weights
 */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * condition variable to hold threads
 */
static pthread_cond_t condVar;

/**
 * weight stack with default init(defined values)
 */
static weights_s _weight_S = DEFAULT;

/**
 * info struct for each thread
 */
static thread_info threadInfo[PHIL_T];



/**
 * destroys all mutexes and cond_vars
 */
void monitor_destroy() {
	printf("Destroy EVERYTHING!\n");
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&condVar);
	pthread_mutex_destroy(&keyPad_mutex);
}

/**
 * monitor method for get weight -safed with pthread mutexes
 */
void get_weights(int needed_weights, weights_s *input, thread_info threadInfo) {
	pthread_mutex_lock(&mutex);

	while (!checkWeight(needed_weights, &_weight_S, input)) {
		threadInfo.status_inf = 'b';
		pthread_cond_wait(&condVar, &mutex);
	}
	threadInfo.status_inf = 'n';
	threadInfo.wStack = *input;
	displayInfoStatus(threadInfo);
	pthread_mutex_unlock(&mutex);
}



/**
 * displays information like seen in bsp2 task
 */
void displayInfoStatus(thread_info info) {
	threadInfo[info.ID] = info;
	printf("Thread %d changed\n", info.ID);
	int sumStack = 0;
	for (int i = 0; i < PHIL_T; i++) {
		int ID = threadInfo[i].ID;
		int wNeeded = threadInfo[i].weightNeeded;
		char KEYP = threadInfo[i].status_inf;
		char state = convertForOutput(threadInfo[i].state);
		sumStack = sumStack+ getTotalWeight(threadInfo[i].wStack);
		int w1 = threadInfo[i].wStack.twoKG;
		int w2 = threadInfo[i].wStack.threeKG;
		int w3 = threadInfo[i].wStack.fiveKG;
		
		printf("ID:%d (%d), %c, %c, [%d, %d, %d] ", ID, wNeeded, KEYP, state, w1, w2, w3);
	}
	printf(" Supply: [%d, %d, %d]\n", _weight_S.twoKG, _weight_S.threeKG, _weight_S.fiveKG);
	
	weights_s testStack = DEFAULT;
	int maxW = getTotalWeight(testStack);
	int stackW = getTotalWeight(_weight_S);
	if ( sumStack != (maxW - stackW)) {
		printf(" sumStack not same as expected! %d\n", sumStack);
	}
}



/**
 * holds the thread on cond var and then parses new thread_info to struct
 */
void initMonitor() {
	pthread_cond_init(&condVar, NULL);
	for (int i = 0; i < PHIL_T; i++) {
		thread_info newInfo = INIT_THREAD;
		threadInfo[i] = newInfo;
		KPAD[i] = NO_INPUT;
		pthread_mutex_unlock(&keyPad_mutex);
	}
}

/**
 * monitor method to lock and unlock threads to safley
 * use checkWeight and getTotalWeight to put back the
 * used weights
 */
void put_weights(weights_s *userWStack, thread_info threadInfo) {
	pthread_mutex_lock(&mutex);
	if (!checkWeight(getTotalWeight(*userWStack), userWStack, &_weight_S)) {
		printf("Copy Weight dindt work\n");
	}
	threadInfo.wStack = *userWStack;
	displayInfoStatus(threadInfo);
	pthread_cond_broadcast(&condVar);
	pthread_mutex_unlock(&mutex);
}

/**
 * method that guarantees that needed weights are available
 * and safes the putBack
 */
bool checkWeight(int totalWeight, weights_s *from, weights_s *to) {
	weights_s from_copy = EMPTY;
	weights_s to_copy = EMPTY;
	copyWeight(from, &from_copy);
	copyWeight(to, &to_copy);
	if (from_copy.twoKG < 0 || from_copy.threeKG < 0 || from_copy.fiveKG < 0) {
		return false;
	}
	else if (totalWeight == 0) {
		copyWeight(&from_copy, from);
		copyWeight(&to_copy, to);
		return true;
	}
	else if (getTotalWeight(from_copy) < totalWeight || totalWeight < 2) {
		return false;
	}
	from_copy.fiveKG--;
	to_copy.fiveKG++;
	if (!checkWeight(totalWeight - 5, &from_copy, &to_copy)) {
		from_copy.fiveKG++;
		to_copy.fiveKG--;

		from_copy.threeKG--;
		to_copy.threeKG++;
		if (!checkWeight(totalWeight - 3, &from_copy, &to_copy)) {
			from_copy.threeKG++;
			to_copy.threeKG--;

			from_copy.twoKG--;
			to_copy.twoKG++;
			if (!checkWeight(totalWeight - 2, &from_copy, &to_copy)) {
				from_copy.twoKG++;
				to_copy.twoKG--;
				return false;
			}
		}
	}
	copyWeight(&from_copy, from);
	copyWeight(&to_copy, to);
	return true;
}

/**
 * copies the weights from one weight stack to another
 */
void copyWeight(weights_s *from, weights_s *to) {
	(*to).twoKG = (*from).twoKG;
	(*to).threeKG = (*from).threeKG;
	(*to).fiveKG = (*from).fiveKG;
}

/**
 * returns the totalWeight
 */
int getTotalWeight(weights_s wStack) {
	return (wStack.twoKG * 2) + (wStack.threeKG * 3) + (wStack.fiveKG * 5);
}






































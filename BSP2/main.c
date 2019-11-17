#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "main.h"
#include "monitor.h"

/**
 * vector for semaphores - one for each thread
 */
static sem_t semaphore[PHIL_T];

/**
 * static function declarations
 */
static int updateState(state_p *state, long *counter_l);
static void init_m();

/**
 * thread barrier for creation
 */
static pthread_barrier_t create_b;

/**
 * Main thread that represents the philosophers
 */
void *philoThread(void *input_arguments) {
	thread_arguments args = *((thread_arguments *)input_arguments);
	state_p current_State = REST;
	weights_s weights = EMPTY;

	printf("Name: %s, WEIGHT: %dkg\n\n", args.name, args.weightNeeded);

	char keyInp = NO_INPUT;
	long counter_l = 0;
	int update = 0;

	int val = pthread_barrier_wait(&create_b);
	if(PTHREAD_BARRIER_SERIAL_THREAD == val) {
		printf("Main Thread passed barrier!\n");
	}
	else if (val == 0) {
		printf("Barrier passed by thread %d\n", args.ID);
	}

	while(!(keyInp == 'q')) {
		//read
		keyInp = readInput(args.ID);
		if (keyInp != NO_INPUT) {
			printf("ID: %d | Input: %c\n", args.ID, keyInp);
		}
		//state update
		if (keyInp == 'p') {
			printf("Proceed thread, loop finished!\n");
			counter_l = 0;
		}
		else if (keyInp == 'b') {
			args.status = 'b';
			printf("Thread blocked!!!\n");
			displayInfoStatus(thread_info_init(args, current_State, weights));
			sem_wait(args.semaphore_threadID);
			args.status = 'n';
			printf("Thread unblocked!!!\n");
		}
		update = updateState(&current_State, &counter_l);
		if (update != 0) {
			displayInfoStatus(thread_info_init(args, current_State, weights));
		}
		//write
		if (current_State == WORKOUT || current_State == REST) {
			counter_l--;
		}
		else if (current_State == PUT_WEIGHTS) {
			put_weights(&weights, thread_info_init(args, current_State, weights));
		}
		else if (current_State == GET_WEIGHTS) {
			get_weights(args.weightNeeded, &weights, thread_info_init(args, current_State, weights));
		}
	}
	args.status = 'q';
	printf("ID: %d | exited\n", args.ID);
	pthread_exit(NULL);
	return NULL;
}

/**
 * resṕresents state machine
 * updates State
 */
static int updateState(state_p *state, long *counter_l) {
	int hasNew = 0;
	switch (*state) {
		case WORKOUT:
				if (*counter_l <= 0) {
					*state = PUT_WEIGHTS;
				}
			break;
		case PUT_WEIGHTS:
			*counter_l = REST_LOOP;
			*state = REST;
			hasNew = 1;
			break;
		case GET_WEIGHTS:
			*counter_l = WORKOUT_LOOP;
			*state = WORKOUT;
			hasNew = 1;
			break;
		case REST:
				if (*counter_l <= 0) {
					*state = GET_WEIGHTS;
				}
			break;
		default:
			break;
	}
	return hasNew;
}

/**
 * Initializes Threads and their semaphores.
 */
static void init_m() {
	pthread_barrier_init(&create_b, NULL, PHIL_T);
	for (int i = 0; i < PHIL_T; i++) {
		sem_init(&semaphore[i], SEM_TH, SEM_M);
	}
}

/**
 * parses arguments to arguments vector for thread
 */
void *thread_dependencies(void *input_arguments) {
	thread_arguments args = *((thread_arguments *)input_arguments);
	printf("Thread: %s | Weight needed: %dkg", args.name, args.weightNeeded);
	return NULL;
}

/**
 * Initializes the Info Struct for thread
 */
thread_info thread_info_init(thread_arguments currArgs, state_p currState, weights_s currStack) {
	thread_info outInfo;
	outInfo.ID = currArgs.ID;
	outInfo.status_inf = currArgs.status;
	outInfo.state = currState;
	outInfo.weightNeeded = currArgs.weightNeeded;
	outInfo.wStack = currStack;
	return outInfo;
}

/**
 * converts State to corresponding output token.
 */
char convertForOutput(state_p state) {
	switch(state) {
		case WORKOUT:
				return 'W';
				break;
		case REST:
				return 'R';
				break;
		case GET_WEIGHTS:
				return 'G';
				break;
		case PUT_WEIGHTS:
				return 'G';
				break;
		default:
				return 'X';
	}
}

/**
 * function that reads and returns the input of Keypad
 */
char readInput(int threadID) {
	if(!(threadID >= 0 && threadID < PHIL_T)) {
		printf("Thread ID is invalid!\n");
	}
	pthread_mutex_lock(&keyPad_mutex);
	char output = KPAD[threadID];
	KPAD[threadID] = NO_INPUT;
	pthread_mutex_unlock(&keyPad_mutex);
	return output;
}

/**
 * function that writes input to keypad -
 * safed by mutex
 */
void writeInput(int threadID, char value) {
	if (!(threadID >= 0 && threadID < PHIL_T)) {
		printf("Thread ID is invalid!\n");
	}
	pthread_mutex_lock(&keyPad_mutex);
	KPAD[threadID] = value;
	pthread_mutex_unlock(&keyPad_mutex);
}

int main(int argv, char *args[]) {
	pthread_t philoThreads[PHIL_T];
	thread_arguments input_v[PHIL_T];
	
	init_m();
	initMonitor();
	
	input_v[0] = (thread_arguments){.ID = 0, .name = "Anna", .weightNeeded = ANNA_W, 'n', &semaphore[0]};
	input_v[1] = (thread_arguments){.ID = 1, .name = "Bernd", .weightNeeded = BERND_W, 'n', &semaphore[1]};
	input_v[2] = (thread_arguments){.ID = 2, .name = "Clara", .weightNeeded = CLARA_W,'n', &semaphore[2]};
	input_v[3] = (thread_arguments){.ID = 3, .name = "Dirk", .weightNeeded = DIRK_W,'n', &semaphore[3]};
	input_v[4] = (thread_arguments){.ID = 4, .name = "Emma", .weightNeeded = EMMA_W,'n', &semaphore[4]};
	
	for (int i = 0; i < PHIL_T; i++) {
		int err = pthread_create(&(philoThreads[i]), NULL, &philoThread, &(input_v[i]));
		if(err != 0) {
			printf("Error: %d", err);
			return 1;
		}
		
	}
	bool q_flag = false;
	while(!q_flag) {
		char keyInput[KEYMAX];
		fgets(keyInput, KEYMAX, stdin);
		if(keyInput[0] == 'q' || keyInput[0] == 'Q') {
			for(int i = 0; i < PHIL_T; i++) {
				writeInput(i, 'q');
			}
			q_flag = true;
		}
		else if (keyInput[1] == 'u') {
			int thread_id = keyInput[0] - '0';
			if (thread_id < PHIL_T && thread_id >= 0) {
				printf("Thread Nr. %d unblocked.\n", thread_id);
				sem_post(&semaphore[thread_id]);
			} else {
				printf("Something went wrong. Unknown Thread_ID %d" , thread_id);
			}
			
		}
		else if ((keyInput[0] >= '0') && ((keyInput[0] - '0') <= PHIL_T)) {
			int thread_number = keyInput[0] - '0';
			writeInput(thread_number, keyInput[1]);
		} else {
			printf("Unknown Key '%c'\n", keyInput[0]);
		}
	}
	printf("Thread joining...\n");
	pthread_barrier_destroy(&create_b);
	for (int i = 0; i < PHIL_T; i++) {
		sem_post(&semaphore[i]);
		pthread_join(philoThreads[i], NULL);
	}
	for (int i = 0; i < PHIL_T; i++) {

		sem_destroy(&semaphore[i]);
	}
	monitor_destroy();
	printf("EXIT PROGRAM...\n");
}



/** *****************************************************************
 * @file bsp2/main.c
 * @author Christian Caus (christian.caus@haw-hamburg.de)
 * @author Stefan Subotin (stefan.subotin@haw-hamburg.de)
 * @version 1.0
 * @date 08.11.2019
 * @brief Philosophs Workout
 * Mutexes, Semaphores, cond vars
 * Monitor concept
 ********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "general.h"

//array for thread status
char status[N_PHIL] = {'n','n','n','n','n'};

//array for philosopher states
State philoStates[N_PHIL];

unsigned int philoIDs[N_PHIL] = {0,1,2,3,4};

//semaphores one per philosopher
sem_t semaphore[N_PHIL];

//cond vars with pthreads - one per philosopher
pthread_cond_t cond[N_PHIL];

//array for keyinput
char keyinput[KEYCOMBO];

//array for transmitting b, u, p to philosophers
char listen[N_PHIL];

int main (void) {
	pthread_t philoThreadIDs[N_PHIL];
	int i;
	int err;
	int res[N_PHIL];
	
	
	//init
	for(i = 0; i < N_PHIL; i++) {
		philoStates[i] = REST;
		err = pthread_cond_init(&cond[i], NULL);
		assert(!err);
		sem_init(&semaphore[i], 0, 0);
	}
	pthread_mutex_init(&mutex, NULL);
	
	//start threads
	for(i = 0; i < N_PHIL; i++) {
		res[i] = pthread_create(&philoThreadIDs[i], NULL, *philothread, &philoIDs[i]);
		philoIDs[i] = i;
		
		if(res[i] != 0) {
			perror("Thread cration unsucessful!!!");
			exit(EXIT_FAILURE);
		}
	}
	//keyboard input
	bool q_flag = false;
	bool c_flag = false;
	while(!q_flag) {
		
		fgets(keyinput, KEYCOMBO, stdin);
		switch(keyinput[0]) {
			case 'q' || 'Q':
				q_flag = true;
				break;
			case '0':
				c_flag = true;
				break;
			case '1':
				c_flag = true;
				break;
			case '2':
				c_flag = true;
				break;
			case '3':
				c_flag = true;
				break;
			case '4':
				c_flag = true;
				break;
			default:
				printf("WRONG INPUT!");
		}
		if (q_flag) {
			printf("Quitting...\n");
			printf("Unblocking all Threads\n");
			for(i = 0; i < N_PHIL; i++) {
				listen[i] = 'q';
				sem_post(&semaphore[keyinput[0] - ASCII]);
			}
			for ( i = 0; i < N_PHIL; i++) {
				printf("Thread %d joined\n", i);
				pthread_cond_signal(&cond[i]);
				pthread_join(philoThreadIDs[i], NULL);
			}
			printf("Destroy ALL HUMANS!\n");
			for(i = 0; i < N_PHIL; i++) {
				pthread_cond_destroy(&cond[i]);
				sem_destroy(&semaphore[i]);
			}
			pthread_mutex_destroy(&mutex);
			
			printf("EXITING PROGRAMM!\n");
			pthread_exit(NULL);
			exit(EXIT_FAILURE); //programm stops here
		}
		else if (c_flag) {
			switch(keyinput[1]) {
				case 'b': 
					//set block in listen array
					listen[keyinput[0] - ASCII] = keyinput[1];
				break;
				case 'u':
					listen[keyinput[0] - ASCII] = 'u';
					sem_post(&semaphore[keyinput[0] - ASCII]);
					
				break;
				case 'p':
					//proceed
					listen[keyinput[0] - ASCII] = 'p';
				    sem_post(&semaphore[keyinput[0] - ASCII]);
				break;
			}
		}
	}
	return 0;	
}


/**
 * proceeds the workout using a count loop
 * to defined max val
 */
void workout (int philoID) {
	int count;
	
	while (count < WORKOUT_LOOP) {
		if(listen[philoID] == 'b') {
			sem_wait(&semaphore[philoID]);
		}	
		if(listen[philoID] == 'p') {
			count = WORKOUT_LOOP;
		}
		count++;
	}
	philoStates[philoID] = PUT_WEIGHTS;
}
/**
 * proceeds the rest using a count loop to 
 * defined max val
 */
void rest (int philoID) {
	int count;
	while (count < TEST_RL) {
		if(listen[philoID] == 'b') {
			sem_wait(&semaphore[philoID]);
		}
		if(listen[philoID] == 'p') {
			count = WORKOUT_LOOP;
		}
		count++;
	}
	philoStates[philoID] = GET_WEIGHTS;
}

/**
 * \brief The main philosopher function
 * \param pID philosoher ID from thread creation
 * \return nothing
 */
void *philothread(void *pID) {
	int *philoID = pID;
	int running = 1; //init
	
	printf("Philosopher %d just awoke\n", *philoID);
	
	while (running) {
		
		rest(*philoID);
		get_weights(*philoID);
		workout(*philoID);
		put_weights(*philoID);
		
		if(listen[*philoID] == 'q' || listen[*philoID] == 'Q') {
			running = 0;
		}
	}
	return 0;
}

int displayStatus(void) {
	/**
	int soll = 45;
	int ist;
	int i;
	
	for(i=0; i<5; i++) {
		ist = ist + (2*taken[i][0]) + (3*taken[i][1]) + (5+taken[i][2]);
	}
	ist = ist + (2+avail[0]) + (3*avail[1]) + (5*avail[2]);

	if (soll != ist) {
		printf("WARNING! Gewichte stimmen nicht! WARNING!\n");
	} else {
	
	
	for (i=0; i<5; i++) {
		char status_char = status[i];
		char state_char;
		int weight_programm;
		
		switch (i){
			case ANNA_ID:
				weight_programm = 6;
				break;
			case BERND_ID:
				weight_programm = 8;
				break;
			case EMMA_ID:
				weight_programm = 14;
				break;
			default:
				weight_programm = 12;
				break;
		}
		switch(philoStates[i]) {
			case GET_WEIGHTS:
				state_char = 'G';
				break;
			case WORKOUT:
				state_char = 'W';
				break;
			case PUT_WEIGHTS:
				state_char = 'P';
				break;
			case REST:
				state_char = 'R';
				break;
			default:
				break;
		}
		
		printf("%d(%d)%c:%c:[%d, %d, %d] ", i, weight_programm, status_char, state_char, taken[i][0], taken[i][1], taken[i][2]);
	}
	printf("  Supply: [%d, %d, %d]\n", avail[0], avail[1], avail[2]);
	}
	*/
	printf("I made it this far YIPPIE!!!!\n");
	return 0;
}





























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
#include <monitor.h>
#include <stdbool.h>
#include "general.h"


int main (void) {
	pthread_t philoThreadIds[N_PHIL];
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
		res[i] = pthread_create(&philoThreadIDs[i], NULL, 	philo, &philoID[i]);
		philoID[i] = i;
		
		if(res[i] != 0) {
			perror("Thread cration unsucessful!!!");
			exit(EXIT_FAILURE);
		}
	}
	//keyboard input
	bool q_flag = false;
	bool c_flag = false;
	while(TRUE) {
		
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
				sprintf("WRONG INPUT!");
		}
		if (q_flag) {
			//send 
			
		
		
	
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
		count++;
	}
}
/**
 * proceeds the rest using a count loop to 
 * defined max val
 */
void rest (int value) {
	int count;
	while (count < REST_LOOP) {
		if(listen[philoID] == 'b') {
			sem_wait(semaphore[philoID]);
		}
		count++;
	}
}

void unblock_sem(int philoID) {
	if(listen[philoID] == 'u' 
}



























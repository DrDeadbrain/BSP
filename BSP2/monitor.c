/** *****************************************************************
 * @file bsp2/philo.c
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
#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>


#define SEMAPHORE 1

#include "general.h"
#include "monitor.h"







uint8_t avail[] = {4, 4, 5};
uint8_t taken[N_PHIL][avail];


int get_weights (int philoID) {
	int quantity_2kg = avail[0];
	int quantity_3kg = avail[1];
	int quantity_5kg = avail[2];

	pthread_mutex_lock(&mutex);
	display_status();
	
	philoStates[philoID] = GET_WEIGHTS;
	
	//check if needed weight is available
	while() {
		pthread_cond_wait(&cond[philoID], &mutex);
	}
	
	switch(philoID) {
		case ANNA_ID: //ANNA:6kg
			if (quantity_2kg >= 3) {
				avail[0] = avail[0] - 3;
			}
			else if (quantity_3kg >= 2) {
				avail[1] = avail[1] - 2;
			}
		break;
		case BERND_ID: //Bernd:8kg
			if (quantity_2kg == 4) {
				avail[0] = avail[0] - 4;
			}
			else if ((quantity_3kg >= 2) && (quantity_2kg >= 1)) {
				avail[1] = avail[1] - 2;
				avail[0] = avail[0] - 1;
			}
			else if ((quantity_5kg >= 1) && (quantity_3kg >= 1)) {
				avail[2] = avail[2] - 1;
				avail[1] = avail[1] - 1;
			}
		break;
		case EMMA_ID: //Emma:
			if (quantity_2kg == 4 && (quantity_3kg >= 2)) {
				avail[0] = avail[0] - 4;
				avail[1] = avail[1] - 2;
			}
			else if ((quantity_2kg >= 1) && (quantity_3kg >= 4)) {
				avail[0] = avail[0] - 1;
				avail[1] = avail[1] - 4;
			}
			else if ((quantity_2kg >= 3) && (quantity_3kg >= 1) && (quantity_5kg >= 1)) {
				avail[0] = avail[0] - 3;
				avail[1] = avail[1] - 1;
				avail[2] = avail[2] - 1;
			}
			else if ((quantity_3kg >= 3) && (quantity_5kg >= 1)) {
				avail[1] = avail[1] - 3;
				avail[2] = avail[2] - 1;
			}
			else if ((quantity_2kg >= 2) && (quantity_5kg >= 2)) {
				avail[0] = avail[0] - 2;
				avail[2] = avail[2] - 2;
			}
		break;
		default: //Dirk || Clara : 12kg
			if ((quantity_2kg >= 3) && (quantity_3kg >= 2)) {
				avail[0] = avail[0] - 3;
				avail[1] = avail[1] - 2;
			}
			else if ((quantity_3kg == 4)) {
				avail[1] = avail[1] - 4;
			}
			else if ((quantity_2kg >= 2) && (quantity_3kg >= 1) && (quantity_5kg >= 1)) {
				avail[0] = avail[0] - 2;
				avail[1] = avail[1] - 1;
				avail[2] = avail[2] - 1;
			}
			else if ((quantity_2kg >= 1) && (quantity_5kg >= 2)) {
				avail[0] = avail[0] - 1;
				avail[2] = avail[2] - 2;
			}
		break;
	}
	//mark philosopher to workout 
	philoStates[philoID] = WORKOUT;
	
	//unlock the mutex
	pthread_mutex_unlock(&mutex);

}





int put_weights(void){
	return 0;
}
			


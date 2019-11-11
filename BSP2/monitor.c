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
	
	//check if needed weight is available
	while() {
		pthread_cond_wait(&cond[philoID], &mutex);
	}
	
	switch(philoID) {
		case ANNA_ID: //ANNA:6kg
			if (quantity_3kg >= 2) { //2
				avail[1] = avail[1] - 2;
				taken[philoID][1] = 2;
			}
			else if (quantity_2kg >= 3) { //3
				avail[0] = avail[0] - 3;
				taken[philoID][0] = 3;
			}
		break;
		case BERND_ID: //Bernd:8kg
			if ((quantity_5kg >= 1) && (quantity_3kg >= 1)) { //2
				avail[2] = avail[2] - 1;
				avail[1] = avail[1] - 1;
				taken[philoID][2] = 1;
				taken[philoID][1] = 1;
			}
			else if ((quantity_3kg >= 2) && (quantity_2kg >= 1)) { //3
				avail[1] = avail[1] - 2;
				avail[0] = avail[0] - 1;
				taken[philoID][1] = 2;
				taken[philoID][0] = 1;
			}
			else if (quantity_2kg == 4) { //4
				avail[0] = avail[0] - 4;
				taken[philoID][0] = 4;
			}
		break;
		case EMMA_ID: //Emma:14kg
			if ((quantity_2kg >= 2) && (quantity_5kg >= 2)) { //4.1
				avail[0] = avail[0] - 2;
				avail[2] = avail[2] - 2;
				taken[philoID][0] = 2;
				taken[philoID][2] = 2;
			}
			else if ((quantity_3kg >= 3) && (quantity_5kg >= 1)) { //4.2
				avail[1] = avail[1] - 3;
				avail[2] = avail[2] - 1;
				taken[philoID][1] = 3;
				taken[philoID][2] = 1;
			}
			else if ((quantity_2kg >= 3) && (quantity_3kg >= 1) && (quantity_5kg >= 1)) { //5.1
				avail[0] = avail[0] - 3;
				avail[1] = avail[1] - 1;
				avail[2] = avail[2] - 1;
				taken[philoID][0] = 3;
				taken[philoID][1] = 1;
				taken[philoID][2] = 1;
			}
			else if ((quantity_2kg >= 1) && (quantity_3kg >= 4)) { //5.2
				avail[0] = avail[0] - 1;
				avail[1] = avail[1] - 4;
				taken[philoID][0] = 1;
				taken[philoID][1] = 4;
			}
			else if (quantity_2kg == 4 && (quantity_3kg >= 2)) { //6
				avail[0] = avail[0] - 4;
				avail[1] = avail[1] - 2;
				taken[philoID][0] = 4;
				taken[philoID][1] = 2;
			}
		break;
		default: //Dirk || Clara : 12kg
			if ((quantity_2kg >= 1) && (quantity_5kg >= 2)) { //3
				avail[0] = avail[0] - 1;
				avail[2] = avail[2] - 2;
				taken[philoID][0] = 1;
				taken[philoID][2] = 2;
			}
			else if ((quantity_2kg >= 2) && (quantity_3kg >= 1) && (quantity_5kg >= 1)) { //4.1
				avail[0] = avail[0] - 2;
				avail[1] = avail[1] - 1;
				avail[2] = avail[2] - 1;
				taken[philoID][0] = 2;
				taken[philoID][1] = 1;
				taken[philoID][2] = 1;
			}
			else if ((quantity_3kg == 4)) { //4.2
				avail[1] = avail[1] - 4;
				taken[philoID][1] = 4;
			}
			else if ((quantity_2kg >= 3) && (quantity_3kg >= 2)) { //5
				avail[0] = avail[0] - 3;
				avail[1] = avail[1] - 2;
				taken[philoID][0] = 3;
				taken[philoID][1] = 2;
			}
		break;
}
	//mark philosopher to workout 
	philoStates[philoID] = WORKOUT;
	
	//unlock the mutex
	pthread_mutex_unlock(&mutex);

}



int put_weights(void){
	pthread_mutex_lock(&mutex_pw);
	
	avail[0] = avail[0] + taken[philoID][0];
	taken[philoID][0] = 0;
	avail[1] = avail[1] + taken[philoID][1];
	taken[philoID][1] = 0;
	avail[2] =avail[2] + taken[philoID][2];
	taken[philoID][2] = 0;
	
	philoStates[philoID] = REST;
	pthread_mutex_unlock(&mutex);
	
	for(i = 0; i < N_PHIL; i++) {
		pthread_cond_signal(&cond[i]);
	}
}
			


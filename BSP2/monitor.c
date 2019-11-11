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

//main mutex for get_weights
static pthread_mutex_t main_mutex_gw = PTHREAD_MUTEX_INITIALIZER;

//main mutex for put weights
static pthread_mutex_t main_mutex_pw = PTHREAD_MUTEX_INITIALIZER;

//mutex for philo_id_0
static pthread_mutex_t philo_m_0 = PTHREAD_MUTEX_INITIALIZER;

//mutex for philo_id_0
static pthread_mutex_t philo_m_1 = PTHREAD_MUTEX_INITIALIZER;

//mutex for philo_id_0
static pthread_mutex_t philo_m_2 = PTHREAD_MUTEX_INITIALIZER;

//mutex for philo_id_0
static pthread_mutex_t philo_m_3 = PTHREAD_MUTEX_INITIALIZER;

//mutex for philo_id_0
static pthread_mutex_t philo_m_4 = PTHREAD_MUTEX_INITIALIZER;

static int philo_0_isRunning;
static int philo_1_isRunning;
static int philo_2_isRunning;
static int philo_3_isRunning;
static int philo_4_isRunning;

#ifdef SEMAPHORE
//semaphore for get_weights
sem_t sem_get_weights;

//semaphore for put_weights
sem_t sem_put_weights;

#else
//cond var for get weights
pthread_cond_t cond_gw;

//cond var for put weights
pthread cond_t cond_put_weigths;
#endif
uint8_t avail[] = {4, 4, 5};

//help func to free locked mutex for cond var
void cleanUpMutex(void *arg) {
		pthread_mutex_unlock(&main_mutex_gw);
}

void* philothread(pthread_mutex_t* philo_m, int *philo_isRunning, int id){}

void* gw_Monitor(pthread_mutex_t* philo_m, int *philo_isRunning, int id) {
	while(*philo_isRunning) {
			pthread_mutex_lock(philo_m); //check if philo is blocked
			pthread_mutex_unlock(philo_m); //unblock thread
				#ifdef SEMAPHORE
					sem_wait(&sem_get_weights); //check get_weights is free
					pthread_setcancelstate(PTHREAD_CANCEL_DISABLE);

					pthread_mutex_lock(&main_mutex_gw);
					get_weights(id, REST);
					pthread_mutex_unlock(&main_mutex_pw);

					pthread_setcancelstate(PTHREAD_CANCEL_DISABLE);
					sem_post(&sem_get_weights);
				#else
					pthread_cleanup_push(cleanUpMutex, null);
					//critical sec
					pthread_mutex_lock(&main_mutex_gw);

					pthread_cond_wait(&cond_gw, &main_mutex_gw); //cond var
					get_weights(id, REST);
					pthread_mutex_unlock(&main_mutex_gw);

					pthread_cleanup_pop(0);
				#endif
		}
}


void control()


int get_weights (int id, int* value) {
	int quantity_2kg = avail[0];
	int quantity_3kg = avail[1];
	int quantity_5kg = avail[2];



	switch(id) {
		case ANNA_ID: //ANNA:6kg
			if (quantity_2kg >= 3) {
				avail[0] = avail[0] - 3;
			}
			else if (quantity_3kg >= 2) {
				avail[1] = avail[1] - 2;
			}
			else {
				pthread_mutex_lock(&philo_m_0);
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
			else {
				pthread_mutex_lock(philo_m_1);
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
			else {
				pthread_mutex_lock(philo_m_4);
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
			else {
				if (id == CLARA_ID) {
					pthread_mutex_lock(&philo_m_2);
				}
				else if (id == DIRK_ID) {
					pthread_mutex_lock(&philo_m_3);
				}
			}
		break;
	}
	*value = GET_WEIGHTS;

}





int put_weights(void){
	return 0;
}
			
int workout (int* value) {
	int count;
	while (count < WORKOUT_LOOP) {
		count++;
	}
	*value = PUT_WEIGHTS;
	return SUCESS;
}
/**
 * proceeds the rest using a count loop to 
 * defined max val
 */
int rest (int* value) {
	int count;
	while (count < REST_LOOP) {
		count++;
	}
	*value = REST;
	return SUCESS;
}


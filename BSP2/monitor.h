/** *****************************************************************
 * @file bsp2/philo.h
 * @author Christian Caus (christian.caus@haw-hamburg.de)
 * @author Stefan Subotin (stefan.subotin@haw-hamburg.de)
 * @version 1.0
 * @date 08.11.2019
 * @brief Philosophs Workout
 * Mutexes, Semaphores, cond vars
 * Monitor concept
 ********************************************************************
 */

#ifndef PHILO_H
#define PHILO_H

typedef enum {GET_WEIGHTS = 0, WORKOUT, PUT_WEIGHTS, REST} State;
State curr_state = REST;

void* philothread (pthread_mutex_t* philo_m, int *philo_isRunning, int id);

#endif 

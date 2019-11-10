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

int main (void) {
	printf("Programm ist gestartet worden\n");
	fflush(stdout);
	
	#ifdef SEMAPHORE
		sem_init(&sem_get_weights, 0 , 0);
		sem_init(&sem_put_weights, 0, 0);
	#endif
	
	pthread_t control_t;
	pthread_t philo_0_t;
	pthread_t philo_1_t;
	pthread_t philo_2_t;
	pthread_t philo_3_t;
	pthread_t philo_4_t;
	
	//startet alle threads
	
	if(pthread_create(&control_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	if(pthread_create(&philo_0_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	if(pthread_create(&philo_1_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	if(pthread_create(&philo_2_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	if(pthread_create(&philo_3_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	if(pthread_create(&philo_4_t, NULL, philothread, NULL)) {
		printf("Error: Thread can't be started.");
		fflush(stdout);
		return -1;
	}
	
	if(pthread_join(control_t, NULL)) {
		printf("Error: Thread join failed.");
		fflush(stdout);
		pthread_cancel(philo_0_t);
		pthread_cancel(philo_1_t);
		pthread_cancel(philo_2_t);
		pthread_cancel(philo_3_t);
		pthread_cancel(philo_4_t);
		return -1;
	}
	if(pthread_cancel(philo_0_t) != 0) {
		printf("Error: cancel thread failed");
		fflush(stdout);
		return -1;
	}
	if(pthread_join(philo_0_t, NULL)) {
		printf("Error: Thread join failure.");
		fflush(stdout);
		return -1;
	}
	printf("Thread philo_0 exited\n");
	fflush(stdout);
	
	if(pthread_cancel(philo_1_t) != 0) {
		printf("Error: cancel thread failed");
		fflush(stdout);
		return -1;
	}
	if(pthread_join(philo_1_t, NULL)) {
		printf("Error: Thread join failure.");
		fflush(stdout);
		return -1;
	}
	printf("Thread philo_0 exited\n");
	fflush(stdout);
	
	if(pthread_cancel(philo_2_t) != 0) {
		printf("Error: cancel thread failed");
		fflush(stdout);
		return -1;
	}
	if(pthread_join(philo_2_t, NULL)) {
		printf("Error: Thread join failure.");
		fflush(stdout);
		return -1;
	}
	printf("Thread philo_0 exited\n");
	fflush(stdout);
	
	if(pthread_cancel(philo_3_t) != 0) {
		printf("Error: cancel thread failed");
		fflush(stdout);
		return -1;
	}
	if(pthread_join(philo_3_t, NULL)) {
		printf("Error: Thread join failure.");
		fflush(stdout);
		return -1;
	}
	printf("Thread philo_0 exited\n");
	fflush(stdout);
	
	if(pthread_cancel(philo_4_t) != 0) {
		printf("Error: cancel thread failed");
		fflush(stdout);
		return -1;
	}
	if(pthread_join(philo_4_t, NULL)) {
		printf("Error: Thread join failure.");
		fflush(stdout);
		return -1;
	}
	printf("Thread philo_0 exited\n");
	fflush(stdout);
	
	#ifdef SEMAPHORE
		sem_destroy(&sem_get_weights);
		sem_destroy(&sem_put_weights);
	#else
		pthread_cond_destroy(&cond_gw);
		pthread_cond_destroy(&cond_pw);
	#endif
	
	pthread_mutex_destroy(&main_mutex_gw);
	pthread_mutex_destroy(&main_mutex_pw);
	pthread_mutex_destroy(&philo_m_0);
	pthread_mutex_destroy(&philo_m_1);
	pthread_mutex_destroy(&philo_m_2);
	pthread_mutex_destroy(&philo_m_3);
	pthread_mutex_destroy(&philo_m_4);
	
}

























/** *****************************************************************
 * @file bsp2/general.h
 * @author Christian Caus (christian.caus@haw-hamburg.de)
 * @author Stefan Subotin (stefan.subotin@haw-hamburg.de)
 * @version 1.0
 * @date 08.11.2019
 * @brief Philosophs Workout
 * Mutexes, Semaphores, cond vars
 * Monitor concept
 ********************************************************************
 */

#ifndef GENERAL_H
#define GENERAL_H

#define SUCESS 0
#define ERROR -1

//Thread Indices
#define ANNA_ID  0
#define BERND_ID 1
#define CLARA_ID 2
#define DIRK_ID  3
#define EMMA_ID  4

//weights 
#define ANNA_W  6
#define BERND_W 8
#define CLARA_W 12
#define DIRK_W  12
#define EMMA_W  14

//loop limits

#define REST_LOOP 1000000000
#define WORKOUT_LOOP 500000000


#endif

/** \file
 * \brief Counter Timer
 */

#ifndef _COUNTER_H
#define _COUNTER_H

#include <iup.h>

#ifdef __cplusplus
extern "C" {
#endif


/** Utility to starts a timer using clock(). */
void ctTimerStart(void);

/** Stops the timer and returns a formated string */
char* ctTimerCount(void);

/** Returns a formated string */
char* ctTimerDelay(int delay_time);


#ifdef __cplusplus
}
#endif

#endif

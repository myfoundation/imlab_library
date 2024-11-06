
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "counter.h"

static clock_t ctStartClock = 0;

void ctTimerStart(void)
{
  ctStartClock = clock();
}

char* ctTimerCount(void)
{
  return ctTimerDelay(clock() - ctStartClock);
}

char* ctTimerDelay(int delay_time)
{
  static char time_desc[30];
  float time = ((float)delay_time)/CLOCKS_PER_SEC;

  if (time < 1)
    sprintf(time_desc, "%d ms", (int)(time*1000));
  else if (time < 60)
    sprintf(time_desc, "%d sec", (int)time);
  else
    sprintf(time_desc, "%d min %d sec", ((int)time)/60, ((int)time)%60);

  return time_desc;
}



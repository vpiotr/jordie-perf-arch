/////////////////////////////////////////////////////////////////////////////
// Name:        timeutils.cpp
// Project:     perfLib
// Purpose:     Utility (general-purpose) functions
// Author:      Piotr Likus
// Modified by:
// Created:     04/10/2008
/////////////////////////////////////////////////////////////////////////////

#include "perf/time_utils.h"
#include "perf/W32Timer.h"

double cpu_time()
{
  double res;
  res = (double) clock() / (double) CLOCKS_PER_SEC;
  return res;
}

cpu_ticks cpu_time_ms()
{
  return (cpu_ticks)(cpu_time()*1000.0);
}

#ifndef PERF_USE_WIN32_TICKS
cpu_ticks cpu_time_ticks()
{
  clock_t t = clock();
  if (t != static_cast<clock_t>(-1))
    return t;
  else
    return 0;
}
#endif // PERF_USE_WIN32_TICKS

#ifndef PERF_USE_WIN32_TICKS
cpu_ticks cpu_time_ticks_to_ms(cpu_ticks ticks)
{
  double res;
  res = (double) ticks / (double) CLOCKS_PER_SEC;
  res *= 1000.0;
  return (cpu_ticks)res;
}
#endif // PERF_USE_WIN32_TICKS

bool is_cpu_time_elapsed_ms(cpu_ticks a_startTime, cpu_ticks a_delay)
{
  bool res;
  cpu_ticks currTime = cpu_time_ms();

  if (
         (a_startTime + a_delay <= currTime)
         ||
         (currTime < a_startTime)
     )
  {
    res = true;
  } else {
    res = false;
  }
  return res;
}

uint64 calc_cpu_time_delay(uint64 startTime, uint64 endTime)
{
  uint64 res;
  if (startTime <= endTime)
    res = endTime - startTime;
  else
  {
    uint64 maxTicks = uint64(0) - 1;
    res = endTime + (maxTicks - startTime) + 1;
  }
  return res;
}

uint64 os_uptime_ms()
{
#ifdef WIN32
   return w32_os_uptime_ms();
#else
  return 0;
#endif
}

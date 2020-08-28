/////////////////////////////////////////////////////////////////////////////
// Name:        W32Timer.cpp
// Project:     perfLib
// Purpose:     Timer functions for Windows (Win32).
// Author:      Piotr Likus
// Modified by:
// Created:     19/11/2012
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>

#include "perf/W32Timer.h"

cpu_ticks w32_cpu_time_ticks()
{
  LARGE_INTEGER watch;
  QueryPerformanceCounter(&watch) ;
  return watch.QuadPart;
}

cpu_ticks w32_cpu_time_ticks_to_ms(cpu_ticks ticks)
{
  double res;
  LARGE_INTEGER frequency;
  QueryPerformanceFrequency( &frequency );
  res = (static_cast<double>(ticks) /static_cast<double>(frequency.QuadPart));
  res *= 1000.0;
  return static_cast<cpu_ticks>(res);
}

cpu_ticks w32_os_uptime_ms()
{
  return GetTickCount64();
}



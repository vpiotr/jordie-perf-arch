/////////////////////////////////////////////////////////////////////////////
// Name:        time_utils.h
// Project:     perfLib
// Purpose:     Utility, general-purpose functions
// Author:      Piotr Likus
// Modified by:
// Created:     04/10/2008
/////////////////////////////////////////////////////////////////////////////


#ifndef _PERFTIMEUTILS_H__
#define _PERFTIMEUTILS_H__

//#define PERF_OPT_USE_WIN32API

#ifdef WIN32
#if defined(PERF_OPT_USE_WIN32API)
#define PERF_USE_WIN32_TICKS
#endif
#endif

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "perf/details/ptypes.h"

#ifdef PERF_USE_WIN32_TICKS
#include "perf/W32Timer.h"
#endif

// ----------------------------------------------------------------------------
// date & time handling
// ----------------------------------------------------------------------------

/// Calculates current time in millisecs counted from start of application
/// \return current time in msecs
cpu_ticks cpu_time_ms();

/// Calculates current time in "cpu ticks" counted from start of application
/// \return current time in "cpu ticks"
#ifndef PERF_USE_WIN32_TICKS
cpu_ticks cpu_time_ticks();
#else
#define cpu_time_ticks_to_ms(a) w32_cpu_time_ticks_to_ms(a)
#endif

/// Converts time expressed in "cpu ticks" to millisecs
#ifndef PERF_USE_WIN32_TICKS
cpu_ticks cpu_time_ticks_to_ms(cpu_ticks ticks);
#else
#define cpu_time_ticks w32_cpu_time_ticks
#endif

/// Checks if elapsed time is already greater then a specified delay
/// \return <true> if a specified delay elapsed from a given start time
bool is_cpu_time_elapsed_ms(cpu_ticks a_startTime, cpu_ticks a_delay);

/// Calculates time passed between given start & end time (units: can be ticks or msecs)
uint64 calc_cpu_time_delay(uint64 startTime, uint64 endTime);

uint64 os_uptime_ms();

#endif // _PERFTIMEUTILS_H__

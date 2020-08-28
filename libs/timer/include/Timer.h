/////////////////////////////////////////////////////////////////////////////
// Name:        Timer.h
// Project:     perfLib
// Purpose:     Timer / profiler classes & definitions
// Author:      Piotr Likus
// Modified by:
// Created:     11/11/2008
/////////////////////////////////////////////////////////////////////////////

#ifndef _PERFTIMER_H__
#define _PERFTIMER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file Timer.h
///
///

#define PERF_TIMER_USE_UNORDERED

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#ifdef PERF_TIMER_USE_UNORDERED
#include <unordered_map>
#else
#include "boost/ptr_container/ptr_map.hpp"
#endif

//#include "sc/utils.h"
#include "perf/details/ptypes.h"

namespace perf {

// ----------------------------------------------------------------------------
// Simple type definitions
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Forward class definitions
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Constants
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// Class definitions
// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Timer
// ----------------------------------------------------------------------------
namespace Details {
  class TimerItem {
  public:
    TimerItem() {m_lock = 0; m_totalTime = 0; }
    ~TimerItem() {}
    void start();
    /// returns <true> if stop was performed successfuly
    bool stop(cpu_ticks a_stopTime = 0);
    void inc(cpu_ticks value);
    void reset();
    cpu_ticks getTotal();
    bool isRunning();
  private:
    cpu_ticks m_startTime;
    cpu_ticks m_totalTime;
    uint m_lock;
  };

#ifdef PERF_TIMER_USE_UNORDERED
  typedef std::unordered_map<dtpString,TimerItem *> TimerItemMapColn;
#else
  typedef boost::ptr_map<dtpString,TimerItem> TimerItemMapColn;
#endif
};

/// Timer visitor
class TimerVisitorIntf {
public:
  TimerVisitorIntf() {}
  virtual ~TimerVisitorIntf() {}
  virtual void visit(const dtpString &timerName, cpu_ticks value) = 0;
};

enum TimerStatusFlag {
   tsfRunning = 1,
   tfsStopped = 2   
};

const uint tsfAny = tsfRunning + tfsStopped;

/// global timer collection
class Timer {
public:
  static void start(const dtpString &a_name);
  /// \return <true> if stop was performed successfuly
  static bool stop(const dtpString &a_name);
  static void reset(const dtpString &a_name);
  static void inc(const dtpString &a_name, cpu_ticks value);
  static cpu_ticks getTotal(const dtpString &a_name);
  static bool isRunning(const dtpString &a_name);
  static void visitAll(TimerVisitorIntf *visitor);
  static void getAll(dtp::dnode &output);
  static void getByFilter(const dtp::dnode &filterList, dtp::dnode &output, uint statusMask = tsfAny);
protected:
  static Details::TimerItem *addItem(const dtpString &a_name);
  static Details::TimerItem *getItem(const dtpString &a_name);
  static Details::TimerItem *checkItem(const dtpString &a_name);
  static dtp::dnode removeNonMatching(const dtp::dnode &input, const dtp::dnode &filterList, uint statusMask);
private:
#ifdef PERF_TIMER_USE_UNORDERED
  static boost::shared_ptr<Details::TimerItemMapColn> m_items;
#else
  static Details::TimerItemMapColn m_items;
#endif
};

/// local, private timer collection
class LocalTimer {
public:
  // construct
  LocalTimer();
  virtual ~LocalTimer();
  // properties
  cpu_ticks getTotal(const dtpString &a_name);
  void getAll(dtp::dnode &output);
  void getByFilter(const dtp::dnode &filterList, dtp::dnode &output, uint statusMask = tsfAny);
  // run
  void start(const dtpString &a_name);
  void stop(const dtpString &a_name);
  void reset(const dtpString &a_name);
  void inc(const dtpString &a_name, cpu_ticks value);
  void visitAll(TimerVisitorIntf *visitor);
protected:
  Details::TimerItem *addItem(const dtpString &a_name);
  Details::TimerItem *getItem(const dtpString &a_name);
  Details::TimerItem *checkItem(const dtpString &a_name);
  dtp::dnode removeNonMatching(const dtp::dnode &input, const dtp::dnode &filterList, uint statusMask);
private:
  Details::TimerItemMapColn m_items;
};

}; // namespace perf
#endif // _PERFTIMER_H__

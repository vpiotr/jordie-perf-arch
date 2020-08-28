/////////////////////////////////////////////////////////////////////////////
// Name:        Timer.cpp
// Project:     perfLib
// Purpose:     Timer / profiler classes & definitions
// Author:      Piotr Likus
// Modified by:
// Created:     11/11/2008
/////////////////////////////////////////////////////////////////////////////

//#include "sc/defs.h"

#include "base/wildcard.h"
#include "base/string.h"

#include "perf/Timer.h"
#include "perf/time_utils.h"

#ifdef DEBUG_MEM
#include "dbg/DebugMem.h"
#endif

#ifdef _DEBUG
#define DEBUG_TIMER
#endif

#ifdef DEBUG_TIMER
#include <iostream>
#endif

using namespace perf;

//TODO: add destruction of timers

// ----------------------------------------------------------------------------
// Details::TimerItem
// ----------------------------------------------------------------------------
void Details::TimerItem::start()
{
  if (m_lock == 0)
  {
    m_lock++;
    m_startTime = cpu_time_ticks();
  } else {
    m_lock++;
  }
}

bool Details::TimerItem::stop(cpu_ticks a_stopTime)
{
  bool res = (m_lock == 1);
  if (m_lock > 1)
  {
    --m_lock;
  } else if (m_lock == 1) {
    --m_lock;
    cpu_ticks stopTime;
    if (!a_stopTime)
      stopTime = cpu_time_ticks();
    else
      stopTime = a_stopTime;
    m_totalTime += (calc_cpu_time_delay(m_startTime, stopTime));
  }
  return res;
}

void Details::TimerItem::inc(cpu_ticks value)
{
  m_totalTime += value;
}

void Details::TimerItem::reset()
{
  m_lock = 0;
  m_totalTime = 0;
}

cpu_ticks Details::TimerItem::getTotal()
{
  return cpu_time_ticks_to_ms(m_totalTime);
}

bool Details::TimerItem::isRunning()
{
  return (m_lock > 0);
}

template<typename map_type>
struct map_deleter
{
    void operator()(void *ptr) const
    {
         map_type *map = static_cast<map_type *>(ptr);
         for(map_type::iterator it = map->begin(), epos = map->end(); it != epos; ++it)
           delete (it->second);
         map->erase(map->begin(), map->end());
         delete map;
    }
};

// ----------------------------------------------------------------------------
// Timer
// ----------------------------------------------------------------------------

#ifdef PERF_TIMER_USE_UNORDERED
boost::shared_ptr<Details::TimerItemMapColn> Timer::m_items(new Details::TimerItemMapColn(), map_deleter<Details::TimerItemMapColn>());
#else
Details::TimerItemMapColn Timer::m_items;
#endif

void Timer::start(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
#pragma omp critical(timer)
{
  item->start();
}
}

bool Timer::stop(const dtpString &a_name)
{
#ifdef DEBUG_TIMER
  if (a_name == "gx-exp-all")
    std::cout << "DEBUG-stop: " << a_name << std::endl;
#endif

  cpu_ticks stopTime = cpu_time_ticks();
  Details::TimerItem *item = checkItem(a_name);
#pragma omp critical(timer)
{
  return item->stop(stopTime);
}
}

void Timer::reset(const dtpString &a_name)
{
#ifdef DEBUG_TIMER
  if (a_name == "gx-exp-all")
    std::cout << "DEBUG-reset: " << a_name << std::endl;
#endif

  Details::TimerItem *item = checkItem(a_name);
#pragma omp critical(timer)
{
  item->reset();
}
}

void Timer::inc(const dtpString &a_name, cpu_ticks value)
{
  Details::TimerItem *item = checkItem(a_name);

#pragma omp critical(timer)
{
  item->inc(value);
}
}

cpu_ticks Timer::getTotal(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
  cpu_ticks res;
#pragma omp critical(timer)
{
  res = item->getTotal();
}
  return res;
}

bool Timer::isRunning(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
  bool res;
#pragma omp critical(timer)
{
  res = item->isRunning();
}
  return res;
}

Details::TimerItem *Timer::addItem(const dtpString &a_name)
{
  std::auto_ptr<Details::TimerItem> guard(new Details::TimerItem());
#ifdef PERF_TIMER_USE_UNORDERED
  m_items->insert(Details::TimerItemMapColn::value_type(a_name, guard.get()));
#else
  m_items.insert(const_cast<dtpString &>(a_name), guard.get());
#endif
  return guard.release();
}

Details::TimerItem *Timer::getItem(const dtpString &a_name)
{
  Details::TimerItemMapColn::iterator p;

#ifdef PERF_TIMER_USE_UNORDERED
  Details::TimerItemMapColn *items = m_items.get();
#else
  Details::TimerItemMapColn *items = &m_items;
#endif

  p = items->find(a_name);
  if(p != items->end())
    return (p->second);
  else
    return DTP_NULL;
}

Details::TimerItem *Timer::checkItem(const dtpString &a_name)
{
  Details::TimerItem *res;
#pragma omp critical(timer)
{
  res = getItem(a_name);
  if (!res)
    res = addItem(a_name);
}
  assert(res != DTP_NULL);
  return res;
}

dtp::dnode Timer::removeNonMatching(const dtp::dnode &input, const dtp::dnode &filterList, uint statusMask)
{
  dtp::dnode res;
  dtpString name;
  bool bRunning;
  bool bIncludeRunning = (statusMask & tsfRunning) != 0; 
  bool bIncludeStopped = (statusMask & tfsStopped) != 0;

  res.setAsParent();

  for(uint j=0, eposj = filterList.size(); j != eposj; j++)
  {
    WildcardMatcher matcher(filterList.getString(j));
    for(uint i=0, epos = input.size(); i != epos; i++)
    {
      name = input.getElementName(i);
      bRunning = isRunning(name);

      if ((bRunning && bIncludeRunning) ||
          (!bRunning && bIncludeStopped))
      {
            if (!res.hasChild(name) && matcher.isMatching(name))
              res.addChild(name, new dtp::dnode(input.getElement(i)));
      }
    }
  }

  return res;
}

void Timer::visitAll(TimerVisitorIntf *visitor)
{
#ifdef PERF_TIMER_USE_UNORDERED
  Details::TimerItemMapColn *items = m_items.get();
#else
  Details::TimerItemMapColn *items = &m_items;
#endif

  cpu_ticks itemTime;
  for (Details::TimerItemMapColn::iterator p = items->begin(); p != items->end(); p++)
  {
    itemTime = p->second->getTotal();
    visitor->visit(p->first, itemTime);
  }
}

void Timer::getAll(dtp::dnode &output)
{
  cpu_ticks itemTime;

  output.clear();
  output.setAsParent();

#ifdef PERF_TIMER_USE_UNORDERED
  Details::TimerItemMapColn *items = m_items.get();
#else
  Details::TimerItemMapColn *items = &m_items;
#endif

  for (Details::TimerItemMapColn::iterator p = items->begin(); p != items->end(); p++)
  {
    itemTime = p->second->getTotal();
    output.addChild(p->first, new dtp::dnode(itemTime));
  }
}

void Timer::getByFilter(const dtp::dnode &filterList, dtp::dnode &output, uint statusMask)
{
  dtpString itemName;
  bool bRunning;
  bool bIncludeRunning = (statusMask & tsfRunning) != 0; 
  bool bIncludeStopped = (statusMask & tfsStopped) != 0;

  output.clear();
  output.setAsParent();

#ifdef PERF_TIMER_USE_UNORDERED
  Details::TimerItemMapColn *items = m_items.get();
#else
  Details::TimerItemMapColn *items = &m_items;
#endif

  if (items->empty())
    return;

  boost::ptr_vector<WildcardMatcher> matchers;

  for(uint j=0, eposj = filterList.size(); j != eposj; j++)
    matchers.push_back(new WildcardMatcher(filterList.getString(j)));

  for (Details::TimerItemMapColn::iterator p = items->begin(); p != items->end(); p++)
  {
    itemName = p->first;
    bRunning = p->second->isRunning();

    if ((bRunning && bIncludeRunning) ||
        (!bRunning && bIncludeStopped))
    {
       for(uint j=0, eposj = matchers.size(); j != eposj; j++)
         if (matchers[j].isMatching(itemName)) {
            output.addChild(itemName, new dtp::dnode(p->second->getTotal()));
            break;
         }
    }
  }
}

// ----------------------------------------------------------------------------
// LocalTimer
// ----------------------------------------------------------------------------
LocalTimer::LocalTimer()
{
}

LocalTimer::~LocalTimer()
{
  for(Details::TimerItemMapColn::iterator it = m_items.begin(), epos = m_items.end(); it != epos; ++it)
    delete (it->second);
}

void LocalTimer::start(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
  item->start();
}

void LocalTimer::stop(const dtpString &a_name)
{
  cpu_ticks stopTime = cpu_time_ticks();
  Details::TimerItem *item = checkItem(a_name);
  item->stop(stopTime);
}

void LocalTimer::reset(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
  item->reset();
}

void LocalTimer::inc(const dtpString &a_name, cpu_ticks value)
{
  Details::TimerItem *item = checkItem(a_name);
  item->inc(value);
}

cpu_ticks LocalTimer::getTotal(const dtpString &a_name)
{
  Details::TimerItem *item = checkItem(a_name);
  cpu_ticks res;
  res = item->getTotal();
  return res;
}

Details::TimerItem *LocalTimer::addItem(const dtpString &a_name)
{
  std::auto_ptr<Details::TimerItem> guard(new Details::TimerItem());
#ifdef PERF_TIMER_USE_UNORDERED
  m_items.insert(Details::TimerItemMapColn::value_type(a_name, guard.get()));
#else
  m_items.insert(const_cast<dtpString &>(a_name), guard.get());
#endif
  return guard.release();
}

Details::TimerItem *LocalTimer::getItem(const dtpString &a_name)
{
  Details::TimerItemMapColn::iterator p;

  p = m_items.find(a_name);
  if(p != m_items.end())
    return (p->second);
  else
    return DTP_NULL;
}

Details::TimerItem *LocalTimer::checkItem(const dtpString &a_name)
{
  Details::TimerItem *res;

  res = getItem(a_name);
  if (!res)
    res = addItem(a_name);

  assert(res != DTP_NULL);
  return res;
}

dtp::dnode LocalTimer::removeNonMatching(const dtp::dnode &input, const dtp::dnode &filterList, uint statusMask)
{
  dtp::dnode res;
  dtpString name;
  bool found, bRunning;
  bool bIncludeRunning = (statusMask & tsfRunning) != 0; 
  bool bIncludeStopped = (statusMask & tfsStopped) != 0;

  res.setAsParent();

  for(uint j=0, eposj = filterList.size(); j != eposj; j++)
  {
    WildcardMatcher matcher(filterList.getString(j));
    for(uint i=0, epos = input.size(); i != epos; i++)
    {
      name = input.getElementName(i);
      found = false;
      bRunning = checkItem(name)->isRunning();

      if ((bRunning && bIncludeRunning) ||
          (!bRunning && bIncludeStopped))
      {
        if (!res.hasChild(name) && matcher.isMatching(name))
          res.addChild(name, new dtp::dnode(input.getElement(i)));
      }
    }
  }
  return res;
}

void LocalTimer::visitAll(TimerVisitorIntf *visitor)
{
  cpu_ticks itemTime;
  for (Details::TimerItemMapColn::iterator p = m_items.begin(); p != m_items.end(); p++)
  {
    itemTime = p->second->getTotal();
    visitor->visit(p->first, itemTime);
  }
}

void LocalTimer::getAll(dtp::dnode &output)
{
  cpu_ticks itemTime;

  output.clear();
  output.setAsParent();

  for (Details::TimerItemMapColn::iterator p = m_items.begin(); p != m_items.end(); p++)
  {
    itemTime = p->second->getTotal();
    output.addChild(p->first, new dtp::dnode(itemTime));
  }
}

void LocalTimer::getByFilter(const dtp::dnode &filterList, dtp::dnode &output, uint statusMask)
{
  dtp::dnode items;
  this->getAll(items);
  output = removeNonMatching(items, filterList, statusMask);
}

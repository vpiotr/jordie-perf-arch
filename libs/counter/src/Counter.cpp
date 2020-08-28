/////////////////////////////////////////////////////////////////////////////
// Name:        Counter.cpp
// Project:     perfLib
// Purpose:     Counter / profiler classes & definitions
// Author:      Piotr Likus
// Modified by:
// Created:     08/08/2009
/////////////////////////////////////////////////////////////////////////////

//#include "sc/defs.h"

#include "base/wildcard.h"
#include "base/string.h"

#include "perf/Counter.h"

#ifdef DEBUG_MEM
#include "dbg/DebugMem.h"
#endif

using namespace perf;

// ----------------------------------------------------------------------------
// Counter
// ----------------------------------------------------------------------------
CounterItemMapColn Counter::m_items;

void CounterItem::inc()
{
  m_total++;
}

void CounterItem::inc(uint64 value)
{
  if (value > 1000000000)
    assert(false);
  m_total += value;
}

void CounterItem::reset()
{
  m_total = 0;
}

uint64 CounterItem::getTotal() const
{
  return m_total;
}

typedef boost::ptr_map<dtpString,CounterItem> CounterItemMapColn;

void Counter::inc(const dtpString &a_name)
{
  CounterItem *item = checkItem(a_name);
#pragma omp critical(counter)
{
  item->inc(1);
}
}

void Counter::inc(const dtpString &a_name, uint64 value)
{
  CounterItem *item = checkItem(a_name);
#pragma omp critical(counter)
{
  item->inc(value);
}
}

void Counter::reset(const dtpString &a_name)
{
  CounterItem *item = checkItem(a_name);
#pragma omp critical(counter)
{
  item->reset();
}
}

uint64 Counter::getTotal(const dtpString &a_name)
{
  CounterItem *item = checkItem(a_name);
  uint64 res;
#pragma omp critical(counter)
{
  res = item->getTotal();
}
  return res;
}

CounterItem *Counter::addItem(const dtpString &a_name)
{
  std::auto_ptr<CounterItem> guard(new CounterItem());
  m_items.insert(const_cast<dtpString &>(a_name), guard.get());
  return guard.release();
}

CounterItem *Counter::getItem(const dtpString &a_name)
{
  CounterItemMapColn::iterator p;

  p = m_items.find(a_name);
  if(p != m_items.end())
    return (p->second);
  else
    return DTP_NULL;
}

CounterItem *Counter::checkItem(const dtpString &a_name)
{
  CounterItem *res;
#pragma omp critical(counter)
{
  res = getItem(a_name);
  if (!res)
    res = addItem(a_name);
}
  assert(res != DTP_NULL);
  return res;
}

void Counter::visitAll(CounterVisitorIntf *visitor)
{
  uint64 itemTotal;
  for (CounterItemMapColn::iterator p = m_items.begin(); p != m_items.end(); p++)
  {
    itemTotal = p->second->getTotal();
    visitor->visit(p->first, itemTotal);
  }
}

void Counter::getAll(scDataNode &output)
{
  uint64 itemTotal;

  output.clear();
  output.setAsParent();

  for (CounterItemMapColn::iterator p = m_items.begin(); p != m_items.end(); p++)
  {
    itemTotal = p->second->getTotal();
    output.addChild(p->first, new scDataNode(itemTotal));
  }
}

scDataNode Counter::removeNonMatching(const scDataNode &input, const scDataNode &filterList)
{
  scDataNode res;
  dtpString name;
  bool found;
  res.setAsParent();

  for(uint j=0, eposj = filterList.size(); j != eposj; j++)
  {
    WildcardMatcher matcher(filterList.getString(j));

    for(uint i=0, epos = input.size(); i != epos; i++)
    {
      name = input.getElementName(i);
      if (!res.hasChild(name) && matcher.isMatching(name) )
        res.addChild(name, new scDataNode(input.getElement(i)));
    }
  }

  return res;
}

void Counter::getByFilter(const scDataNode &filterList, scDataNode &output)
{
  output.clear();
  output.setAsParent();

  dtpString itemName;

  if (m_items.empty())
    return;

  boost::ptr_vector<WildcardMatcher> matchers;

  for(uint j=0, eposj = filterList.size(); j != eposj; j++)
    matchers.push_back(new WildcardMatcher(filterList.getString(j)));

  for (CounterItemMapColn::iterator p = m_items.begin(); p != m_items.end(); p++)
  {
      itemName = p->first;
      for(uint j=0, eposj = matchers.size(); j != eposj; j++)
      {
        if (matchers[j].isMatching(itemName))
        {
          output.addChild(itemName, new scDataNode(p->second->getTotal()));
          break;
        }
      }
  }
}

// ----------------------------------------------------------------------------
// LocalCounter
// ----------------------------------------------------------------------------
LocalCounter::LocalCounter()
{
}

LocalCounter::~LocalCounter()
{
}

void LocalCounter::inc(const dtpString &a_name, uint value)
{
  if (!m_values.hasChild(a_name))
    m_values.addChild(a_name, new scDataNode(value));
  else
    m_values.setUInt64(a_name, m_values.getUInt64(a_name) + value);
}

void LocalCounter::inc(const dtpString &a_name, uint64 value)
{
  if (!m_values.hasChild(a_name))
    m_values.addChild(a_name, new scDataNode(value));
  else
    m_values.setUInt64(a_name, m_values.getUInt64(a_name) + value);
}

void LocalCounter::reset(const dtpString &a_name)
{
  if (m_values.hasChild(a_name))
    m_values.setUInt64(a_name, 0);
}

uint64 LocalCounter::getTotal(const dtpString &a_name)
{
  if (m_values.hasChild(a_name))
    return m_values.getUInt64(a_name);
  else
    return 0;
}

void LocalCounter::clear()
{
  m_values.clear();
}

void LocalCounter::getAll(scDataNode &output)
{
  output = m_values;
}


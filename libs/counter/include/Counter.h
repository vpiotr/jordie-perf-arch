/////////////////////////////////////////////////////////////////////////////
// Name:        Counter.h
// Project:     perfLib
// Purpose:     Counter / profiler classes & definitions
// Author:      Piotr Likus
// Modified by:
// Created:     08/08/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _PERFCOUNTER_H__
#define _PERFCOUNTER_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file Counter.h
///
///

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
#include "boost/ptr_container/ptr_map.hpp"
//#include "sc/utils.h"

#include "dtp/dnode.h"

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
// Counter
// ----------------------------------------------------------------------------
class CounterItem {
public:
  CounterItem() { m_total = 0; }
  ~CounterItem() {}
  void inc();
  void inc(uint64 value);
  void reset();
  uint64 getTotal() const;
private:
  uint64 m_total;
};

typedef boost::ptr_map<dtpString,CounterItem> CounterItemMapColn;

/// Counter visitor
class CounterVisitorIntf {
public:
  CounterVisitorIntf() {}
  virtual ~CounterVisitorIntf() {}
  virtual void visit(const dtpString &timerName, uint64 value) = 0;
};

/// Global counter storage
class Counter {
public:
  static void inc(const dtpString &a_name);
  static void inc(const dtpString &a_name, uint64 value);
  static void reset(const dtpString &a_name);
  static uint64 getTotal(const dtpString &a_name);
  static void visitAll(CounterVisitorIntf *visitor);
  static void getAll(scDataNode &output);
  static void getByFilter(const scDataNode &filterList, scDataNode &output);
protected:
  static CounterItem *addItem(const dtpString &a_name);
  static CounterItem *getItem(const dtpString &a_name);
  static CounterItem *checkItem(const dtpString &a_name);
  static scDataNode removeNonMatching(const scDataNode &input, const scDataNode &filterList);
private:
  static CounterItemMapColn m_items;
};

/// Counter storage that can be used as private counter collection
class LocalCounter {
public:
  LocalCounter();
  virtual ~LocalCounter();
  void inc(const dtpString &a_name, uint value = 1);
  void inc(const dtpString &a_name, uint64 value);
  void reset(const dtpString &a_name);
  uint64 getTotal(const dtpString &a_name);
  void clear();
  void getAll(scDataNode &output);
protected:
  scDataNode m_values;
};

}; // namespace perf

#endif // _PERFCOUNTER_H__

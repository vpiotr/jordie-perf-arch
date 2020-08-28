/////////////////////////////////////////////////////////////////////////////
// Name:        W32DebugViewLog.cpp
// Project:     dbgLib
// Purpose:     Class that saves log messages to DebugView window
// Author:      Piotr Likus
// Modified by:
// Created:     04/08/2010
/////////////////////////////////////////////////////////////////////////////

#include "sc/dbg/W32DebugViewLog.h"
#include "Windows.h"

using namespace perf;

scW32DebugViewLog::scW32DebugViewLog():perf::LogDevice()
{
}

scW32DebugViewLog::~scW32DebugViewLog()
{
}

void scW32DebugViewLog::intAddText(const scString &a_text)
{
  intAddText(a_text, lmlInfo, 0);
}

void scW32DebugViewLog::intAddText(const scString &a_text, LogMsgLevel level, uint msgCode)
{
  std::string s(stringToStdString(a_text));
  OutputDebugStringA(s.c_str());
}

/////////////////////////////////////////////////////////////////////////////
// Name:        DebugMem.cpp
// Project:     dbgLib
// Purpose:     Memory debugging utility module
// Author:      Piotr Likus
// Modified by:
// Created:     09/08/2010
/////////////////////////////////////////////////////////////////////////////

#include <list>

#include <Windows.h>

#include "sc/dbg/DebugMem.h"
#include "sc/defs.h"


#ifndef DTP_COMP_VS

typedef struct {
  DWORD	address;
  DWORD	size;
  char	file[64];
  DWORD	line;
} ALLOC_INFO;

typedef std::list<ALLOC_INFO*> AllocList;

AllocList *allocList;

void AddTrack(DWORD addr,  DWORD asize,  const char *fname, DWORD lnum)
{
  ALLOC_INFO *info;

  if(!allocList) {
    allocList = new(AllocList);
  }

  info = new(ALLOC_INFO);
  info->address = addr;
  strncpy(info->file, fname, 63);
  info->line = lnum;
  info->size = asize;
  allocList->insert(allocList->begin(), info);
};

void RemoveTrack(DWORD addr)
{
  AllocList::iterator i;

  if(!allocList)
    return;
  for(i = allocList->begin(); i != allocList->end(); i++)
  {
    if((*i)->address == addr)
    {
      allocList->remove((*i));
      break;
    }
  }
};

#endif

#ifdef DEBUG_ON

#ifdef DTP_COMP_VS
void DumpUnfreed()
{
    // Dump memory leaks to output window.
    _CrtDumpMemoryLeaks();
}

#else
//non-DTP_COMP_VS

inline void * __cdecl operator new(unsigned int size,
                                   const char *file, int line)
{
  void *ptr = (void *)malloc(size);
  AddTrack((DWORD)ptr, size, file, line);
  return(ptr);
};

inline void __cdecl operator delete(void *p)
{
  RemoveTrack((DWORD)p);
  free(p);
};

void DumpUnfreed()
{
  AllocList::iterator i;
  DWORD totalSize = 0;
  char buf[1024];

  if(!allocList)
    return;

  for(i = allocList->begin(); i != allocList->end(); i++) {
    sprintf(buf, "%-50s:\t\tLINE %d,\t\tADDRESS %d\t%d unfreed\n",
      (*i)->file, (*i)->line, (*i)->address, (*i)->size);
    OutputDebugString(buf);
    totalSize += (*i)->size;
  }
  sprintf(buf, "-----------------------------------------------------------\n");
  OutputDebugString(buf);
  sprintf(buf, "Total Unfreed: %d bytes\n", totalSize);
  OutputDebugString(buf);
};
#endif

#else

void DumpUnfreed()
{}

#endif

/////////////////////////////////////////////////////////////////////////////
// Name:        DebugMem.h
// Project:     dbgLib
// Purpose:     Memory debugging utility module
// Author:      Piotr Likus
// Modified by:
// Created:     09/08/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _DEBUGMEM_H__
#define _DEBUGMEM_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file DebugMem.h
///
/// Memory debugging utility module

// ----------------------------------------------------------------------------
// Headers
// ----------------------------------------------------------------------------
//#include "sc/defs.h"

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
#ifdef MSVS

#ifdef DEBUG_ON
#include <stdlib.h>
#include <crtdbg.h>
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

#else
//not MSVS

#ifdef DEBUG_ON
#define DEBUG_NEW new(__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif

#ifdef DEBUG_ON
inline void * __cdecl operator new(unsigned int size,
                                   const char *file, int line);
inline void __cdecl operator delete(void *p);
#endif

#define new DEBUG_NEW

#endif // non-MSVS

void DumpUnfreed();

#endif // _DEBUGMEM_H__

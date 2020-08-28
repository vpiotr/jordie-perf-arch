/////////////////////////////////////////////////////////////////////////////
// Name:        StackTrace.h
// Project:     dbgLib
// Purpose:     Stack trace functions
// Author:      Piotr Likus
// Modified by:
// Created:     09/08/2010
/////////////////////////////////////////////////////////////////////////////

#ifndef _SCDBGSTACKTRC_H__
#define _SCDBGSTACKTRC_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file StackTrace.h
///
/// Stack tracing utility functions using "Micro Tracing Kernel"
/// Library address: http://www.stahlworks.com/dev/index.php?tool=mtk
/// 
/// Usage:
/// - define TRACE_STACK_MK to use tracking
/// - change code as follows:
///    void foo()
///    {  
///       _TRCENTRY_("foo");                    // block entry trace
///       try { 
///         long i=0;
///         _TRCSTEP_;                          // simple line trace 
///         for (i=0; i<100; i++)               
///            printf("%ld and counting\n",i);  // (1)
///         _TRCSTEP_;
///         printf("done\n");                   
///         _TRCSTEP_;                          // simple line trace  
///       } catch(...) {
///	        _TRCSTACK_; // show/log current stact trace  
///       }
///    }
///

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


#ifdef TRACE_STACK_MK
// include "Micro Tracing Kernel" header
#include "mtk/mtktrace.hpp"
#define _TRCENTRY_(a) _mtkb_(a)
#define _TRCSTEP_ mtklog("[%s %d]", __FILE__, __LINE__);
#define _TRCSTACK_ mtkDumpStackTrace(0)
#else
#define _TRCENTRY_(a) 
#define _TRCSTEP_
#define _TRCSTACK_ 

#endif // _SCDBGSTACKTRC_H__

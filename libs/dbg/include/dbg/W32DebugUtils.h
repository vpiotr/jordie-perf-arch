/////////////////////////////////////////////////////////////////////////////
// Name:        W32DebugUtils.h
// Project:     scLib
// Purpose:     Win32 debugging utilities
// Author:      Piotr Likus
// Modified by:
// Created:     30/09/2009
/////////////////////////////////////////////////////////////////////////////

#ifndef _SC_DEBUG_UTILS_H__
#define _SC_DEBUG_UTILS_H__

// ----------------------------------------------------------------------------
// Description
// ----------------------------------------------------------------------------
/// \file W32DebugUtils.h
///
/// Win32-only debugging utilities.
/// Requires the following options (VS2008) to be active:
/// C/C++
/// - Optimization
/// -> Optimization = Disabled
/// - Code Generation
/// -> Enable C++ Exceptions
///   -> Yes With SEH Exceptions
/// -> Basic Runtime Checks
///   -> Stack Frames
/// -> Runtime Library
///   -> one with Debugging 
///
/// Linker
/// - Debugging
/// -> Generate Debug Info
///   -> Yes
/// -> Generate Program Database File
///   -> ..\bin\vc_msw\$(TargetName).pdb

#include "sc/dtypes.h"

scString W32DebugGetExceptionStackTrace();

#endif // _SC_DEBUG_UTILS_H__

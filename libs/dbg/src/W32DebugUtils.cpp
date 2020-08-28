/////////////////////////////////////////////////////////////////////////////
// Name:        W32DebugUtils.cpp
// Project:     dbgLib
// Purpose:     Debugging utilities
// Author:      Piotr Likus
// Modified by:
// Created:     30/09/2009
/////////////////////////////////////////////////////////////////////////////

#include "sc/dbg/W32DebugUtils.h"
//#include "sc/utils.h"
#include "base/W32String.h"

#ifdef WIN32

//#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif

#include <windows.h>
#include <tchar.h>
#include <string>
#include <vector>

#include <crtdbg.h>
#include <Tlhelp32.h>

#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")

#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

#ifdef _DLL
//#define W32DEBUG_DLL
#endif

using std::string;
using std::vector;


///////////////////////////////////////////////////////////////////////////////
//
// bool GetPdbFileName(LPCTSTR FileName, LPTSTR DebugDir)
// Adaptation of DebugDir example source code by Oleg Starodumov (www.debuginfo.com)
//
//


///////////////////////////////////////////////////////////////////////////////
// Include files
//


///////////////////////////////////////////////////////////////////////////////
// Helper macros
//

    // Thanks to Matt Pietrek
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))


///////////////////////////////////////////////////////////////////////////////
// CodeView debug information structures
//

#define CV_SIGNATURE_NB10   '01BN'
#define CV_SIGNATURE_RSDS   'SDSR'

// CodeView header
struct CV_HEADER
{
    DWORD CvSignature; // NBxx
    LONG  Offset;      // Always 0 for NB10
};

// CodeView NB10 debug information
// (used when debug information is stored in a PDB 2.00 file)
struct CV_INFO_PDB20
{
    CV_HEADER  Header;
    DWORD      Signature;       // seconds since 01.01.1970
    DWORD      Age;             // an always-incrementing value
    BYTE       PdbFileName[1];  // zero terminated string with the name of the PDB file
};

// CodeView RSDS debug information
// (used when debug information is stored in a PDB 7.00 file)
struct CV_INFO_PDB70
{
    DWORD      CvSignature;
    GUID       Signature;       // unique identifier
    DWORD      Age;             // an always-incrementing value
    BYTE       PdbFileName[1];  // zero terminated string with the name of the PDB file
};


///////////////////////////////////////////////////////////////////////////////
// Functions
//


//
// Check whether the specified IMAGE_OPTIONAL_HEADER belongs to
// a PE32 or PE32+ file format
//
// Return value: "true" if succeeded (bPE32Plus contains "true" if the file
//  format is PE32+, and "false" if the file format is PE32),
//  "false" if failed
//
bool IsPE32Plus( PIMAGE_OPTIONAL_HEADER pOptionalHeader, bool& bPE32Plus )
{
    // Note: The function does not check the header for validity.
    // It assumes that the caller has performed all the necessary checks.

    // IMAGE_OPTIONAL_HEADER.Magic field contains the value that allows
    // to distinguish between PE32 and PE32+ formats

    if( pOptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC )
    {
        // PE32
        bPE32Plus = false;
    }
    else if( pOptionalHeader->Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC )
    {
        // PE32+
        bPE32Plus = true;
    }
    else
    {
        // Unknown value -> Report an error
        bPE32Plus = false;
        return false;
    }

    return true;
}

//
// Returns (in [out] parameters) the RVA and size of the debug directory,
// using the information in IMAGE_OPTIONAL_HEADER.DebugDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG]
//
// Return value: "true" if succeeded, "false" if failed
//
bool GetDebugDirectoryRVA( PIMAGE_OPTIONAL_HEADER pOptionalHeader, DWORD& DebugDirRva, DWORD& DebugDirSize )
{
    // Check parameters

    if( pOptionalHeader == 0 )
    {
        _ASSERT( 0 );
        return false;
    }


    // Determine the format of the PE executable

    bool bPE32Plus = false;

    if( !IsPE32Plus( pOptionalHeader, bPE32Plus ) )
    {
        // Probably invalid IMAGE_OPTIONAL_HEADER.Magic
        return false;
    }

    // Obtain the debug directory RVA and size

    if( bPE32Plus )
    {
        PIMAGE_OPTIONAL_HEADER64 pOptionalHeader64 = (PIMAGE_OPTIONAL_HEADER64)pOptionalHeader;
        DebugDirRva = pOptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        DebugDirSize = pOptionalHeader64->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }
    else
    {
        PIMAGE_OPTIONAL_HEADER32 pOptionalHeader32 = (PIMAGE_OPTIONAL_HEADER32)pOptionalHeader;
        DebugDirRva = pOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
        DebugDirSize = pOptionalHeader32->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }

    if( ( DebugDirRva == 0 ) && ( DebugDirSize == 0 ) )
    {
        // No debug directory in the executable -> no debug information
        return true;
    }
    else if( ( DebugDirRva == 0 ) || ( DebugDirSize == 0 ) )
    {
        // Inconsistent data in the data directory
        return false;
    }

    return true;
}

//
// The function walks through the section headers, finds out the section
// the given RVA belongs to, and uses the section header to determine
// the file offset that corresponds to the given RVA
//
// Return value: "true" if succeeded, "false" if failed
//
bool GetFileOffsetFromRVA( PIMAGE_NT_HEADERS pNtHeaders, DWORD Rva, DWORD& FileOffset )
{
    // Check parameters

    if( pNtHeaders == 0 )
    {
        _ASSERT( 0 );
        return false;
    }


    // Look up the section the RVA belongs to

    bool bFound = false;

    PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION( pNtHeaders );

    for( int i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++, pSectionHeader++ )
    {
        DWORD SectionSize = pSectionHeader->Misc.VirtualSize;

        if( SectionSize == 0 ) // compensate for Watcom linker strangeness, according to Matt Pietrek
            pSectionHeader->SizeOfRawData;

        if( ( Rva >= pSectionHeader->VirtualAddress ) &&
            ( Rva < pSectionHeader->VirtualAddress + SectionSize ) )
        {
            // Yes, the RVA belongs to this section
            bFound = true;
            break;
        }
    }

    if( !bFound )
    {
        // Section not found
        return false;
    }


    // Look up the file offset using the section header

    INT Diff = (INT)( pSectionHeader->VirtualAddress - pSectionHeader->PointerToRawData );

    FileOffset = Rva - Diff;

    return true;
}

//
// Display information about CodeView debug information block
//
bool DumpCodeViewDebugInfo( LPBYTE pDebugInfo, DWORD DebugInfoSize, LPTSTR DebugDir )
{
    // Check parameters

    if( ( pDebugInfo == 0 ) || ( DebugInfoSize == 0 ) )
        return false;

    if( IsBadReadPtr( pDebugInfo, DebugInfoSize ) )
        return false;

    if( DebugInfoSize < sizeof(DWORD) ) // size of the signature
        return false;


    // Determine the format of the information and display it accordingly

    DWORD CvSignature = *(DWORD*)pDebugInfo;

    if( CvSignature == CV_SIGNATURE_NB10 )
    {
        // NB10 -> PDB 2.00

        CV_INFO_PDB20* pCvInfo = (CV_INFO_PDB20*)pDebugInfo;

        if( IsBadReadPtr( pDebugInfo, sizeof(CV_INFO_PDB20) ) )
            return false;

        if( IsBadStringPtrA( (CHAR*)pCvInfo->PdbFileName, UINT_MAX ) )
            return false;

        charPtrToTCharPtr((const char*)pCvInfo->PdbFileName, DebugDir);
        return true;
    }
    else if( CvSignature == CV_SIGNATURE_RSDS )
    {
        // RSDS -> PDB 7.00

        CV_INFO_PDB70* pCvInfo = (CV_INFO_PDB70*)pDebugInfo;

        if( IsBadReadPtr( pDebugInfo, sizeof(CV_INFO_PDB70) ) )
            return false;

        if( IsBadStringPtrA( (CHAR*)pCvInfo->PdbFileName, UINT_MAX ) )
            return false;

        charPtrToTCharPtr((const char*)pCvInfo->PdbFileName, DebugDir);
        return true;
    }

    return false;
}

//
// Display information about debug directory entry
//
bool DumpDebugDirectoryEntry( LPBYTE pImageBase, PIMAGE_DEBUG_DIRECTORY pDebugDir, LPTSTR DebugDir  )
{
    // Check parameters

    if( pDebugDir == 0 )
    {
        _ASSERT( 0 );
        return false;
    }

    if( pImageBase == 0 )
    {
        _ASSERT( 0 );
        return false;
    }


    // Display additional information for some interesting debug information types

    LPBYTE pDebugInfo = pImageBase + pDebugDir->PointerToRawData;

    if( pDebugDir->Type == IMAGE_DEBUG_TYPE_CODEVIEW )
    {
        return DumpCodeViewDebugInfo( pDebugInfo, pDebugDir->SizeOfData, DebugDir );
    }
    return false;
}

//
// Walk through each entry in the debug directory and display information about it
//
bool DumpDebugDirectoryEntries( LPBYTE pImageBase, PIMAGE_DEBUG_DIRECTORY pDebugDir, DWORD DebugDirSize, LPTSTR DebugDir )
{
    // Check parameters

    if( pImageBase == 0 )
    {
        _ASSERT( 0 );
        return false;
    }


    // Determine the number of entries in the debug directory

    int NumEntries = DebugDirSize / sizeof(IMAGE_DEBUG_DIRECTORY);

    if( NumEntries == 0 )
    {
        _ASSERT( 0 );
        return false;
    }

    // Display information about every entry

    for( int i = 1; i <= NumEntries; i++, pDebugDir++ )
    {
        if (DumpDebugDirectoryEntry( pImageBase, pDebugDir, DebugDir ))
            return true;
    }

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// main
//

bool GetPdbFileName(LPCTSTR FileName, LPTSTR DebugDir)
{
    // Process the command line and obtain the file name

    if( FileName == 0 )
        return 0;

    // Process the file

    HANDLE hFile      = NULL;
    HANDLE hFileMap   = NULL;
    LPVOID lpFileMem  = 0;

    bool bOK = false;

    do
    {
        // Open the file and map it into memory

        hFile = CreateFile( FileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );

        if( ( hFile == INVALID_HANDLE_VALUE ) || ( hFile == NULL ) )
        {
            break;
        }

        hFileMap = CreateFileMapping( hFile, NULL, PAGE_READONLY, 0, 0, NULL );

        if( hFileMap == NULL )
        {
            break;
        }

        lpFileMem = MapViewOfFile( hFileMap, FILE_MAP_READ, 0, 0, 0 );

        if( lpFileMem == 0 )
        {
            break;
        }


        PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)lpFileMem;
        PIMAGE_NT_HEADERS pNtHeaders = MakePtr( PIMAGE_NT_HEADERS, pDosHeader, pDosHeader->e_lfanew );

        // Look up the debug directory

        DWORD DebugDirRva   = 0;
        DWORD DebugDirSize  = 0;

        if( !GetDebugDirectoryRVA( &pNtHeaders->OptionalHeader, DebugDirRva, DebugDirSize ) )
        {
            break;
        }

        if( ( DebugDirRva == 0 ) || ( DebugDirSize == 0 ) )
        {
            break;
        }

        DWORD DebugDirOffset = 0;

        if( !GetFileOffsetFromRVA( pNtHeaders, DebugDirRva, DebugDirOffset ) )
        {
            break;
        }

        PIMAGE_DEBUG_DIRECTORY pDebugDir = MakePtr( PIMAGE_DEBUG_DIRECTORY, lpFileMem, DebugDirOffset );

        // Display information about every entry in the debug directory

        if (!DumpDebugDirectoryEntries( (LPBYTE)lpFileMem, pDebugDir, DebugDirSize, DebugDir ))
            break;

        bOK = true;
    }
    while (false);


    // Cleanup

    if( lpFileMem != 0 )
    {
        if( !UnmapViewOfFile( lpFileMem ) )
        {
            _ASSERT( 0 );
        }
    }

    if( hFileMap != NULL )
    {
        if( !CloseHandle( hFileMap ) )
        {
            _ASSERT( 0 );
        }
    }

    if( ( hFile != NULL ) && ( hFile != INVALID_HANDLE_VALUE ) )
    {
        if( !CloseHandle( hFile ) )
        {
            _ASSERT( 0 );
        }
    }

    // Complete

    return bOK;
}


///////////////////////////////////////////////////////////////////////////////


enum { MAXNAMELENGTH = 1024 }; // max name length for found symbols


struct ModuleEntry
{
    string imageName;
    string moduleName;
    DWORD baseAddress;
    DWORD size;
};
typedef vector<ModuleEntry> ModuleList;


bool GetModuleList(ModuleList& modules, DWORD pid)
{
    HANDLE hSnap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );
    if (hSnap == (HANDLE) -1)
        return false;

    MODULEENTRY32 me;
    me.dwSize = sizeof(me);

    for (BOOL ok = Module32First(hSnap, &me)
        ; ok
        ; ok = Module32Next(hSnap, &me))
    {
        ModuleEntry e;
        tcharToStdString(me.szExePath, e.imageName);
        tcharToStdString(me.szModule, e.moduleName);
        e.baseAddress = (DWORD) me.modBaseAddr;
        e.size = me.modBaseSize;
        modules.push_back(e);
    }

    CloseHandle(hSnap);

    return !modules.empty();
}


std::string GetModuleDebugSearchPath(HMODULE hModule)
{
    const int BUF_SIZE = 10000;
    CHAR tt[BUF_SIZE] = "";
    TCHAR tt1[BUF_SIZE] = _T("");
    TCHAR tt2[BUF_SIZE] = _T("");

    string semicolon(";");

    // build symbol search path from:
    string symSearchPath = "";
    // current directory
    if ( GetCurrentDirectoryA( BUF_SIZE, tt ) )
      symSearchPath += tt + semicolon;
    // dir with executable
    if ( GetModuleFileNameA( 0, tt, BUF_SIZE ) )
    {
        //PathRemoveFileSpec(tt);
        PathRemoveFileSpecA(tt);
        symSearchPath += tt + semicolon;
    }

    if ( GetEnvironmentVariableA( "_NT_SYMBOL_PATH", tt, BUF_SIZE ) )
      symSearchPath += tt + semicolon;
    if ( GetEnvironmentVariableA( "_NT_ALTERNATE_SYMBOL_PATH", tt, BUF_SIZE ) )
      symSearchPath += tt + semicolon;
    if ( GetEnvironmentVariableA( "SYSTEMROOT", tt, BUF_SIZE ) )
      symSearchPath += tt + semicolon;

    if (hModule != NULL
        && GetModuleFileNameA(hModule, tt, BUF_SIZE)
        && GetPdbFileName(tt1, tt2))
    {
        //PathRemoveFileSpec(tt);
        PathRemoveFileSpec(tt1);
        symSearchPath += tt + semicolon;
    }

    if ( symSearchPath.size() > 0 ) // if we added anything, we have a trailing semicolon
      symSearchPath = symSearchPath.substr( 0, symSearchPath.size() - 1 );

    return symSearchPath;
}


void EnumAndLoadModuleSymbols(HANDLE hProcess, DWORD pid)
{
  ModuleList modules;

  // fill in module list
  GetModuleList(modules, pid);

  for (ModuleList::iterator it(modules.begin()); it != modules.end(); ++it)
  {
    string sPath = GetModuleDebugSearchPath((HMODULE)it->baseAddress);
    SymSetSearchPath(hProcess, const_cast<char*>(sPath.c_str()));

    SymLoadModule64( hProcess, 0,
        const_cast<char*>(it->imageName.c_str()), const_cast<char*>(it->moduleName.c_str()),
        it->baseAddress, it->size );
  }
}


HMODULE ModuleFromAddress(LPCVOID pv)
{
    MEMORY_BASIC_INFORMATION mbi;
    return (VirtualQuery(pv, &mbi, sizeof(mbi)) != 0) ? (HMODULE) mbi.AllocationBase : NULL;
}

scString DoStackTrace( HANDLE hThread,
                 CONTEXT& c,
                 HANDLE hSWProcess)
{
  scString res;
  HANDLE hProcess = GetCurrentProcess();

  struct
  {
    IMAGEHLP_SYMBOL64 sym;
    char name[MAXNAMELENGTH];
  }
  symBuffer = {0};

  IMAGEHLP_SYMBOL64* const pSym = &symBuffer.sym;
  IMAGEHLP_MODULE64 Module;
  IMAGEHLP_LINE64 Line;

  static bool bInitialized = false;

  if (!bInitialized)
  {
    HMODULE hModule = ModuleFromAddress(&ModuleFromAddress);
    string symSearchPath = GetModuleDebugSearchPath(hModule);

    if (!SymInitialize(hProcess, const_cast<char*>(symSearchPath.c_str()), false))
    {
      res += scString("SymInitialize(): GetLastError = ")+toString(GetLastError())+"\n";
      return res;
    }

    DWORD symOptions = SymGetOptions();
    symOptions |= SYMOPT_LOAD_LINES;
    symOptions &= ~(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    SymSetOptions( symOptions );

    bInitialized = true;
  }

  EnumAndLoadModuleSymbols(hProcess, GetCurrentProcessId());

  STACKFRAME64 s = {0};
  // init STACKFRAME for first call
  s.AddrPC.Offset = c.Eip;
  s.AddrPC.Mode = AddrModeFlat;
  s.AddrFrame.Offset = c.Ebp;
  s.AddrFrame.Mode = AddrModeFlat;
  s.AddrStack.Offset = c.Ebp;
  s.AddrStack.Mode = AddrModeFlat;

  pSym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
  pSym->MaxNameLength = MAXNAMELENGTH;

  memset( &Line, '\0', sizeof Line );
  Line.SizeOfStruct = sizeof Line;

  memset( &Module, '\0', sizeof Module );
  Module.SizeOfStruct = sizeof Module;

  do
  {
    // CONTEXT not needed if imageType is IMAGE_FILE_MACHINE_I386
    if (!StackWalk64(IMAGE_FILE_MACHINE_I386, hSWProcess, hThread, &s, NULL, NULL, &SymFunctionTableAccess64, &SymGetModuleBase64, NULL))
    {
        break;
    }

    HMODULE hModule = ModuleFromAddress((LPCVOID)s.AddrPC.Offset);
    char szModuleName[MAX_PATH] = "";
    GetModuleFileNameA(hModule, szModuleName, MAX_PATH);

    if (s.AddrPC.Offset != 0)
    {
        char undecoratedName[MAXNAMELENGTH] = "";
        char undecoratedFullName[MAXNAMELENGTH] = "";
        DWORD64 offsetFromSymbol = 0;
        BOOL hasSymbols = SymGetSymFromAddr64(hProcess, s.AddrPC.Offset, &offsetFromSymbol, pSym);
        if (hasSymbols) {
            UnDecorateSymbolName(pSym->Name, undecoratedName, MAXNAMELENGTH, UNDNAME_NAME_ONLY);
            UnDecorateSymbolName(pSym->Name, undecoratedFullName, MAXNAMELENGTH, UNDNAME_COMPLETE);
        }

        DWORD offsetFromLine = 0;
        if ( ! SymGetLineFromAddr64( hProcess, s.AddrPC.Offset, &offsetFromLine, &Line ) )
        {
            if (hasSymbols)
                res += scString(szModuleName)+": "+
                  scString(undecoratedName)+" (+"+
                  toString((long) offsetFromSymbol)+" bytes)\n";
            else
                res +=
                  scString(szModuleName)+": ("+
                  toString((DWORD)hModule)+" + "+
                  toString((DWORD)s.AddrPC.Offset - (DWORD)hModule)+" (+"+
                  toString((long) offsetFromSymbol)+" bytes)\n";
        }
        else
        {
            if (hasSymbols)
              res += scString(Line.FileName)+"("+
                toString(Line.LineNumber)+"): "+
                toString(offsetFromLine)+" bytes ("+
                scString(undecoratedName) + " / " +
                scString(undecoratedFullName)+
                ")\n";
            else
              res += scString(Line.FileName)+"("+
                toString(Line.LineNumber)+"): "+
                toString(offsetFromLine)+" bytes (no symbol info)"+
                "\n";
        }
    }
  }
  while (s.AddrReturn.Offset != 0);
  return res;
}


struct CriticalSection : public CRITICAL_SECTION
{
    CriticalSection()
    {
        InitializeCriticalSection(this);
    };
    ~CriticalSection()
    {
        DeleteCriticalSection(this);
    };
};


static CriticalSection g_csStackTrace;

scString StackTrace( HANDLE hThread,
                 CONTEXT& c,
                 HANDLE hSWProcess)

{
    scString res;
    EnterCriticalSection(&g_csStackTrace);
    res = DoStackTrace(hThread,
                 c,
                 hSWProcess) ;

    LeaveCriticalSection(&g_csStackTrace);
    return res;
}

#ifdef _MT

extern "C"
{
    // merged from VC 6, .NET and 2005 internal headers in CRT source code
    struct _tiddata
    {
        unsigned long   _tid;       /* thread ID */

#if _MSC_VER >= 1400
        uintptr_t _thandle;         /* thread handle */
#else
        unsigned long   _thandle;   /* thread handle */
#endif
        int     _terrno;            /* errno value */
        unsigned long   _tdoserrno; /* _doserrno value */
        unsigned int    _fpds;      /* Floating Point data segment */
        unsigned long   _holdrand;  /* rand() seed value */
        char *      _token;         /* ptr to strtok() token */
        wchar_t *   _wtoken;        /* ptr to wcstok() token */
        unsigned char * _mtoken;    /* ptr to _mbstok() token */

        /* following pointers get malloc'd at runtime */
        char *      _errmsg;        /* ptr to strerror()/_strerror()
                                       buff */
#if _MSC_VER >= 1300
        wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
#endif
        char *      _namebuf0;      /* ptr to tmpnam() buffer */
        wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
        char *      _namebuf1;      /* ptr to tmpfile() buffer */
        wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
        char *      _asctimebuf;    /* ptr to asctime() buffer */
        wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
        void *      _gmtimebuf;     /* ptr to gmtime() structure */
        char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */
#if _MSC_VER >= 1400
        unsigned char _con_ch_buf[MB_LEN_MAX]; /* ptr to putch() buffer */
        unsigned short _ch_buf_used;   /* if the _con_ch_buf is used */
#endif
        /* following fields are needed by _beginthread code */
        void *      _initaddr;      /* initial user thread address */
        void *      _initarg;       /* initial user thread argument */

        /* following three fields are needed to support
         * signal handling and
         * runtime errors */
        void *      _pxcptacttab;   /* ptr to exception-action table */
        void *      _tpxcptinfoptrs; /* ptr to exception info pointers*/
        int         _tfpecode;      /* float point exception code */
#if _MSC_VER >= 1300
        /* pointer to the copy of the multibyte character
         * information used by the thread */
        /*pthreadmbcinfo*/ void *  ptmbcinfo;

        /* pointer to the copy of the locale informaton
         * used by the thead */
        /*pthreadlocinfo*/ void *  ptlocinfo;
#endif
#if _MSC_VER >= 1400
        int         _ownlocale;     /* if 1, this thread owns its own locale */
#endif
        /* following field is needed by NLG routines */
        unsigned long   _NLG_dwCode;

#if _MSC_VER == 1200 && !defined(_DEBUG) && defined(_DLL)
        void *dummy1, *dummy2; // Weird misalignment in msvcrt.dll fixed
#endif

        /*
         * Per-Thread data needed by C++ Exception Handling
         */
        void *      _terminate;     /* terminate() routine */
        void *      _unexpected;    /* unexpected() routine */
        void *      _translator;    /* S.E. translator */
#if _MSC_VER >= 1400
        void *      _purecall;      /* called when pure virtual happens */
#endif
        void *      _curexception;  /* current exception */
        void *      _curcontext;    /* current exception context */
#if _MSC_VER >= 1300
        int         _ProcessingThrow; /* for uncaught_exception */
#endif
    };

    typedef struct _tiddata * _ptiddata;

    _ptiddata __cdecl _getptd();
}


#ifdef W32DEBUG_DLL


CONTEXT* GetCurrentExceptionContext()
{
    _tiddata* pTiddata = (_tiddata*)(((BYTE*) __pxcptinfoptrs()) - offsetof(_tiddata, _tpxcptinfoptrs));
    return (CONTEXT*) pTiddata->_curcontext;
}

#else

CONTEXT* GetCurrentExceptionContext()
{
    _tiddata* p = _getptd();
    return (CONTEXT*) p->_curcontext;
}

#endif

#else

extern CONTEXT* _pCurrentExContext;

CONTEXT* GetCurrentExceptionContext()
{
    return _pCurrentExContext;
}

#endif


scString W32DebugGetExceptionStackTrace()
{
  scString res;

    CONTEXT* pEC = GetCurrentExceptionContext();
    if (pEC != NULL)
    {
        HANDLE hThread = NULL;
        DuplicateHandle( GetCurrentProcess(), GetCurrentThread(),
            GetCurrentProcess(), &hThread, 0, false, DUPLICATE_SAME_ACCESS );

        res = StackTrace( hThread, *pEC, GetCurrentProcess());

        CloseHandle( hThread );
    }
    return res;
}

#endif // WIN32

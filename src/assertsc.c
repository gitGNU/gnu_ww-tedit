/*

File: assertsc.h
COPYING: Full text of the copyrights statement at the bottom of the file
Descrition:
  Assert function that shows stack context when condition fails.

*/

#ifdef _WIN32
// we need some functions for pointer validation. So tell global.h that
// we need WINDOWS.H
#  ifndef USE_WINDOWS
#    define USE_WINDOWS
#  endif
#endif

#include "global.h"
#include "dbghelp.h"  /* From Microsoft Windows Platform SDK */

#ifdef _WIN32
#ifdef _DEBUG

void DumpStack(CONTEXT *pContext, int nSkip, HANDLE hThread)
{
#ifndef __BOUNDSCHECKER__
  /* We can not call SymInitialize if the program is to be used
  along with BoundsChecker */
  HANDLE hProcess;
  char sModulePath[_MAX_PATH];
  char *psEnv;
  char *p;
  BYTE SymbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
  IMAGEHLP_SYMBOL *pSymbol;
  IMAGEHLP_MODULE stModule;
  IMAGEHLP_LINE stLine;
  DWORD dwSymDisplacement;
  DWORD dwAddr;
  DWORD dwOpts;
  STACKFRAME stStackFrame;
  BOOL bResult;
  CONTEXT stContext;

  memcpy(&stContext, pContext, sizeof(stContext));
  hProcess = GetCurrentProcess();

  if (GetModuleFileName(NULL, sModulePath, sizeof(sModulePath)) == 0)
    abort();

  p = strrchr(sModulePath, '\\');
  if (p != NULL)
    *p = '\0';
  psEnv = getenv("_NT_SYMBOL_PATH");
  if (psEnv != NULL && (strlen(psEnv) + strlen(sModulePath) + 1) < _MAX_PATH)
  {
    strcat(sModulePath, ";");
    strcat(sModulePath, psEnv);
  }
  psEnv = getenv("_NT_ALTERNATE_SYMBOL_PATH");
  if (psEnv != NULL && (strlen(psEnv) + strlen(sModulePath) + 1) < _MAX_PATH)
  {
    strcat(sModulePath, ";");
    strcat(sModulePath, psEnv);
  }
  psEnv = getenv("SYSTEMROOT");
  if (psEnv != NULL && (strlen(psEnv) + strlen(sModulePath) + 1) < _MAX_PATH)
  {
    strcat(sModulePath, ";");
    strcat(sModulePath, psEnv);
  }

  printf("Loading symbol information for modules...");
  dwOpts = SymGetOptions();
  SymSetOptions(dwOpts | SYMOPT_LOAD_LINES);  // We need line numbers info as well
  if (!SymInitialize(GetCurrentProcess(), sModulePath, TRUE))
  {
    printf("SymInitialize() failed!\n");
    TRACE0("SymInitialize() failed!\n");
    abort();
  }
  printf("\nStack context information stored in DEBUG.LOG as well\n");
  TRACE0("Calling stack context dump\n");
  printf("Calling stack context dump\n");

  // Initialize the STACKFRAME structure for the first call
  //
  memset(&stStackFrame, 0, sizeof(stStackFrame));
  stStackFrame.AddrPC.Offset = stContext.Eip;
  stStackFrame.AddrPC.Mode = AddrModeFlat;
  stStackFrame.AddrStack.Offset = stContext.Esp;
  stStackFrame.AddrStack.Mode = AddrModeFlat;
  stStackFrame.AddrFrame.Offset = stContext.Ebp;
  stStackFrame.AddrFrame.Mode = AddrModeFlat;
  
  while (1)
  {
    bResult = StackWalk(IMAGE_FILE_MACHINE_I386, hProcess, hThread,
      &stStackFrame, &stContext,
      NULL, SymFunctionTableAccess, SymGetModuleBase, NULL);

    if (!bResult)
      break;

    if (stStackFrame.AddrFrame.Offset == 0) // Basic sanity check to make sure
      continue;

    if (nSkip > 0)
    {
      --nSkip;
      continue;
    }

    dwAddr = stStackFrame.AddrPC.Offset;

    if (dwAddr == 0)
      break;

    memset(SymbolBuffer, 0, sizeof(SymbolBuffer));
    pSymbol = (IMAGEHLP_SYMBOL *)SymbolBuffer;
    pSymbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL);
    pSymbol->MaxNameLength = 512;
    pSymbol->Address = dwAddr;

    memset(&stModule, 0, sizeof(stModule));
    stModule.SizeOfStruct = sizeof(stModule);

    TRACE1("0x%08x ", dwAddr);
    printf("0x%08x ", dwAddr);

    if (SymGetModuleInfo(hProcess, dwAddr, &stModule))
    {
      TRACE1("%s: ", stModule.ImageName);
      printf("%s: ", stModule.ImageName);
    }
    else
    {
      TRACE0("<unknown module>: ");
      printf("<unknown module>: ");
    }

    if (SymGetSymFromAddr(hProcess, dwAddr,
      &dwSymDisplacement, pSymbol))
    {
      TRACE2("%s +%x", pSymbol->Name, dwSymDisplacement);
      printf("%s +%x", pSymbol->Name, dwSymDisplacement);

      memset(&stLine, 0, sizeof(stLine));
      stLine.SizeOfStruct = sizeof(stLine);

      if (SymGetLineFromAddr(hProcess, dwAddr, &dwSymDisplacement, &stLine))
      {
        TRACE2("\t%s, Line %d ", stLine.FileName, stLine.LineNumber);
        printf("\t%s, Line %d ", stLine.FileName, stLine.LineNumber);
      }
    }
    else  // No symbol found. Print error info
    {
      LPVOID pMsgBuf;

      FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &pMsgBuf, 0, NULL
      );
      TRACE1("%s ", pMsgBuf);
      printf("%s ", pMsgBuf);
      LocalFree(pMsgBuf);
      TRACE0("<unknown symbol>");
      printf("<unknown symbol>");
    }
    TRACE0("\n");
    printf("\n");
  }
#endif /* !BoundsChecker */
}

void _assert_sc( const char *cond, const char *name, unsigned int line )
{
  HANDLE hProcess;
  HANDLE hThread;
  CONTEXT stContext;
  WINBASEAPI BOOL WINAPI IsDebuggerPresent(VOID);

  if (IsDebuggerPresent())
  {
    printf("Assertion %s failed, file: %s, line: %d\n", cond, name, line);
    DebugBreak();
  }

  printf("\n");
  printf("/*******************************************/\n");
  printf("/*                                         */\n");
  printf("/* You are not expected to understand this */\n");
  printf("/*                                         */\n");
  printf("/*******************************************/\n");
  printf("\n");
  printf("Assertion %s failed, file: %s, line: %d\n", cond, name, line);
  TRACE0("/*************************************************************/\n");
  TRACE0("/*                                                           */\n");
  TRACE0("/* You are not expected to understand this. Deliver to Peter */\n");
  TRACE0("/*                                                           */\n");
  TRACE0("/*************************************************************/\n");
  TRACE0("\n");
  TRACE3("Assertion %s failed, file: %s, line: %d\n", cond, name, line);

  hProcess = GetCurrentProcess();
  hThread = GetCurrentThread();

  stContext.ContextFlags = CONTEXT_FULL;

  if (!GetThreadContext(hThread, &stContext))
    abort();

  DumpStack(&stContext, 2, hThread);
  abort();
}

static char *exception_text(int code)
{
  switch (code)
  {
    case EXCEPTION_ACCESS_VIOLATION:
      return "access violation";

    case EXCEPTION_DATATYPE_MISALIGNMENT:
      return "datatype misalignment";

    case EXCEPTION_BREAKPOINT:
      return "breakpoint";

    case EXCEPTION_SINGLE_STEP:
      return "single step";

    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      return "array bounds exceeded";

    case EXCEPTION_FLT_DENORMAL_OPERAND:
      return "flt denormal operand";

    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "flt divide by zero";

    case EXCEPTION_FLT_INEXACT_RESULT:
      return "flt inexact result";

    case EXCEPTION_FLT_INVALID_OPERATION:
      return "flt invalid operation";

    case EXCEPTION_FLT_OVERFLOW:
      return "flt overflow";

    case EXCEPTION_FLT_STACK_CHECK:
      return "flt stack check";

    case EXCEPTION_FLT_UNDERFLOW:
      return "flt underflow";

    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "int divide by zero";

    case EXCEPTION_INT_OVERFLOW:
      return "int overflow";

    case EXCEPTION_PRIV_INSTRUCTION:
      return "priv instruction";

    case EXCEPTION_IN_PAGE_ERROR:
      return "in page error";

    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "illegal instruction";

    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      return "non-continuable exception";

    case EXCEPTION_STACK_OVERFLOW:
      return "stack overflow";

    case EXCEPTION_INVALID_DISPOSITION:
      return "invalid disposition";

    case EXCEPTION_GUARD_PAGE:
      return "guard page";

    case EXCEPTION_INVALID_HANDLE:
      return "invalid handle";

    case CONTROL_C_EXIT:
      return "control_c exit";

    default:
      return "unindentified exception";
  }
}

DWORD FilterFunction(EXCEPTION_POINTERS *p_except)
{
  HANDLE hProcess;
  HANDLE hThread;
  WINBASEAPI BOOL WINAPI IsDebuggerPresent(VOID);

  if (IsDebuggerPresent())
    return 0; /* EXCEPTION_CONTINUE_SEARCH */

  if (   p_except->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT
      || p_except->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
    return 0; /* EXCEPTION_CONTINUE_SEARCH */

  printf("\n");
  printf("/*******************************************/\n");
  printf("/*                                         */\n");
  printf("/* You are not expected to understand this */\n");
  printf("/*                                         */\n");
  printf("/*******************************************/\n");
  printf("exception:\n");
  printf("  %s\n", exception_text(p_except->ExceptionRecord->ExceptionCode));
  printf("\n");
  TRACE0("/*************************************************************/\n");
  TRACE0("/*                                                           */\n");
  TRACE0("/* You are not expected to understand this. Deliver to Peter */\n");
  TRACE0("/*                                                           */\n");
  TRACE0("/*************************************************************/\n");
  TRACE0("exception:\n");
  TRACE1("  %s\n", exception_text(p_except->ExceptionRecord->ExceptionCode));
  TRACE0("\n");

  hProcess = GetCurrentProcess();
  DuplicateHandle( hProcess, GetCurrentThread(),
    hProcess, &hThread, 0, 0, DUPLICATE_SAME_ACCESS);

  DumpStack(p_except->ContextRecord, 0, hThread);
  abort();
}
#endif
#endif  /* ifdef _WIN32 */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2002
Petar Marinov

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


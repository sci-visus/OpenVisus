#if defined(__CYGWIN__) || defined(_WIN32)
// Adapted from
// http://www.rioki.org/2017/01/09/windows_stacktrace.html and
// https://stackoverflow.com/questions/22467604/how-can-you-use-capturestackbacktrace-to-capture-the-exception-stack-not-the-ca
#include <process.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "InputOutput.h"
#include "Mutex.h"
#include <DbgHelp.h>
#include <Windows.h>


namespace idx2
{


static mutex StacktraceMutex;


bool
PrintStacktrace(printer* Pr)
{
  lock Lck(&StacktraceMutex);
  idx2_Print(Pr, "Stack trace:\n");
  SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_INCLUDE_32BIT_MODULES | SYMOPT_UNDNAME);
  HANDLE Process = GetCurrentProcess();
  HANDLE Thread = GetCurrentThread();
  CONTEXT Context = {};
  Context.ContextFlags = CONTEXT_FULL;
  RtlCaptureContext(&Context);
  char Buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
  PSYMBOL_INFO PSymbol = (PSYMBOL_INFO)Buffer;
  STACKFRAME64 Stack;
  memset(&Stack, 0, sizeof(STACKFRAME64));
  if (!SymInitialize(Process, "http://msdl.microsoft.com/download/symbols", true))
    return false;

  for (ULONG Frame = 0;; ++Frame)
  {
    bool Result = StackWalk64(IMAGE_FILE_MACHINE_AMD64,
                              Process,
                              Thread,
                              &Stack,
                              &Context,
                              nullptr,
                              SymFunctionTableAccess64,
                              SymGetModuleBase64,
                              nullptr);
    if (!Result)
      break;
    PSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    PSymbol->MaxNameLen = MAX_SYM_NAME;
    DWORD64 Displacement = 0;
    SymFromAddr(Process, (ULONG64)Stack.AddrPC.Offset, &Displacement, PSymbol);
    IMAGEHLP_LINE64 Line;
    Line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    DWORD Offset = 0;
    if (SymGetLineFromAddr64(Process, Stack.AddrPC.Offset, &Offset, &Line))
    {
      idx2_Print(
        Pr, "Function %s, file %s, line %lu: \n", PSymbol->Name, Line.FileName, Line.LineNumber);
    }
    else
    { // failed to get the line number
      HMODULE HModule = nullptr;
      char Module[256] = "";
      GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        (LPCTSTR)(Stack.AddrPC.Offset),
                        &HModule);
      if (HModule)
        GetModuleFileNameA(HModule, Module, 256);
      idx2_Print(
        Pr, "Function %s, file %s, address 0x%0llX\n", PSymbol->Name, Module, PSymbol->Address);
    }
  }
  return SymCleanup(Process);
}


} // namespace idx2


#elif defined(__linux__) || defined(__APPLE__)
// Adapted from
// stacktrace.h (c) 2008, Timo Bingmann from http://idlebox.net/
// published under the WTFPL v2.0
#include "InputOutput.h"
#include "Macros.h"
#include <cxxabi.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>


namespace idx2
{


// TODO: get the line number (add2line)
bool
PrintStacktrace(printer* Pr)
{
  idx2_Print(Pr, "Stack trace:\n");
  constexpr int MaxFrames = 63;
  void* AddrList[MaxFrames + 1]; // Storage array for stack trace address data
  /* Retrieve current stack addresses */
  int AddrLen = backtrace(AddrList, sizeof(AddrList) / sizeof(void*));
  if (AddrLen == 0)
  {
    idx2_Print(Pr, "  <empty, possibly corrupt>\n");
    return false;
  }
  /* Resolve addresses into strings containing "filename(function+address)", */
  char** SymbolList = backtrace_symbols(AddrList, AddrLen);
  size_t FuncNameSize = 128;
  char Buffer[128];
  char* FuncName = Buffer;
  // iterate over the returned symbol lines (skip the first)
  for (int I = 1; I < AddrLen; ++I)
  {
    // fprintf(stderr, "%s\n", SymbolList[I]);
    char *BeginName = 0, *BeginOffset = 0, *EndOffset = 0;
    /* Find parentheses and +address offset surrounding the mangled name:
    e.g., ./module(function+0x15c) [0x8048a6d] */
    for (char* P = SymbolList[I]; *P; ++P)
    {
      if (*P == '(')
        BeginName = P;
      else if (*P == '+')
        BeginOffset = P;
      else if (*P == ')' && BeginOffset)
      {
        EndOffset = P;
        break;
      }
    }
    if (BeginName && BeginOffset && EndOffset && BeginName < BeginOffset)
    {
      *BeginName++ = '\0';
      *BeginOffset++ = '\0';
      *EndOffset = '\0';
      /* mangled name is now in [BeginName, BeginOffset) and caller offset in
[BeginOffset, EndOffset) */
      int Status;
      char* Ret = abi::__cxa_demangle(BeginName, FuncName, &FuncNameSize, &Status);
      if (Status == 0)
      {
        FuncName = Ret; // use possibly realloc()-ed string
        idx2_Print(Pr, "  %s: %s +%s [%p]\n", SymbolList[I], FuncName, BeginOffset, AddrList[I]);
      }
      else
      { // demangling failed
        idx2_Print(Pr, "  %s: %s() +%s [%p]\n", SymbolList[I], BeginName, BeginOffset, AddrList[I]);
      }
      /* get file names and line numbers using addr2line */
      constexpr int BufLen = 1024;
      char Syscom[BufLen];
      // last parameter is the name of this app
      snprintf(Syscom, BufLen, "addr2line %p -e %s", AddrList[I], "main");
      FILE* F = popen(Syscom, "r");
      if (F)
      {
        char Buffer[BufLen] = { 0 };
        while (fgets(Buffer, sizeof(Buffer), F))
          idx2_Print(Pr, "    %s", Buffer);
        pclose(F);
      }
    }
    else
    { // couldn't parse the line? print the whole line.
      idx2_Print(Pr, "  %s\n", SymbolList[I]);
    }
  }
  free(SymbolList);
  return true;
}


} // namespace idx2

#endif

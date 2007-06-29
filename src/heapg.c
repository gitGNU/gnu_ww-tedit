/*

File: heapg.h
Descrition:
  Heap maintenance functions.

Authors:
  Peter Marinov and Tzvetan Mikov

*/

#ifdef _WIN32
// we need some functions for pointer validation. So tell global.h that
// we need WINDOWS.H
#  ifndef USE_WINDOWS
#    define USE_WINDOWS
#  endif
#endif

#include "global.h"
#include "clist.h"
#include "heapg.h"
#ifdef WIN32
#include "dbghelp.h"  /* From Microsoft Windows Platform SDK */
#endif

/*
  If HEAP_DBG is defined the heap checking is activated.
  HEAP_DBG must be set to one of the following values:
    0 - full heap checking ( slow xfree() )
    1 - fast xfree()
*/

extern void OutOfMemory( void );

//////////////////////////////////////////////////////////////////////////
// Global data

jmp_buf  ErrJmpBuf;  // used on exit on fatal errors


//--------------------------------------------------------------------------
// Name         ErrorJump
//
// Description  Called when we want to exit on fatal error
//--------------------------------------------------------------------------
void ErrorJump ( int value )
{
  longjmp( ErrJmpBuf, value );
};

////////////////////// Memory services ///////////////////////////////////

#ifdef HEAP_DBG

#define FREE_FILL_VALUE    0xCC             // int 3 for x86
#define MALLOC_FILL_VALUE  0xAA
#define END_MARK_LEN    4
#ifndef MAXSTACKTRACE
#define MAXSTACKTRACE 6
#endif

typedef struct dbg_head
{
  TListEntry  link;
  unsigned    size;
  unsigned    line;
  char        file[12];
  #ifdef WIN32
  DWORD       stkentries[MAXSTACKTRACE];
  #endif
  unsigned    checkSum;      // ~sum( of bytes before checkSum )
  char        sgn[4];
} dbg_head;

static const char * heapErrMsg[] =
{
  "OK",                                          // 0
  "Can't access block",                          // 1
  "Start signature is bad",                      // 2
  "End signature is bad",                        // 3
  "Block not in heap list",                      // 4
  "Heap is corrupt",                             // 5
  "Heap list is corrupt",                        // 6
  "Bad check sum of block header",               // 7
};

static TListEntry heap = { &heap, &heap };

void (*_dbg_heap_abort_proc)(void) = NULL;

long _dbg_UsedHeapSize    = 0;
long _dbg_MaxUsedHeapSize = 0;
long _dbg_UsedBlocksCount = 0;
long _dbg_MaxUsedBlocksCount = 0;
long _dbg_xmallocCount    = 0;
long _dbg_xfreeCount      = 0;
long _dbg_xreallocCount   = 0;

//--------------------------------------------------------------------------
// Name         AbortProg
//
// Description  Aborts the program if a fatal error is encountered. Flushes
//              all file buffers.
//--------------------------------------------------------------------------
static void AbortProg ( void )
{
  if (_dbg_heap_abort_proc)
    _dbg_heap_abort_proc();
#ifndef __GNUC__      // GNU C seems to have no flushall
  flushall();
#endif
  abort();
};

//--------------------------------------------------------------------------
// Name         CalcCheckSum
//
// Description  Calculate the check sum of the header
//--------------------------------------------------------------------------
static unsigned CalcCheckSum ( dbg_head * p )
{
  unsigned sum, cnt;
  BYTE * pb;

  pb = (BYTE *)p;
  cnt = offsetof( dbg_head, checkSum );
  sum = 0;
  do
  {
    sum += *pb++;
  }
  while (--cnt);

  return ~sum;
};

//--------------------------------------------------------------------------
// Name         StoreStackContext
//
// Description  Stores calling stack context (win32 specific)
//--------------------------------------------------------------------------
static void StoreStackContext( dbg_head *p )
{
  #ifdef WIN32
  HANDLE hProcess;
  HANDLE hThread;
  STACKFRAME stStackFrame;
  BOOL bResult;
  CONTEXT stContext;
  int nSkip;
  int nIndex;

  hProcess = GetCurrentProcess();
  hThread = GetCurrentThread();

  stContext.ContextFlags = CONTEXT_FULL;

  if (!GetThreadContext(hThread, &stContext))
    return;

  // Initialize the STACKFRAME structure for the first call
  //
  memset(&stStackFrame, 0, sizeof(stStackFrame));
  stStackFrame.AddrPC.Offset = stContext.Eip;
  stStackFrame.AddrPC.Mode = AddrModeFlat;
  stStackFrame.AddrStack.Offset = stContext.Esp;
  stStackFrame.AddrStack.Mode = AddrModeFlat;
  stStackFrame.AddrFrame.Offset = stContext.Ebp;
  stStackFrame.AddrFrame.Mode = AddrModeFlat;

  nSkip = 3;  // Skip first 3 entries
  nIndex = 0;
  memset(p->stkentries, 0, sizeof(p->stkentries));
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

    p->stkentries[nIndex++] = stStackFrame.AddrPC.Offset;
    if (nIndex == MAXSTACKTRACE)
      break;
  }
  #endif
}

//--------------------------------------------------------------------------
// Name         DumpStackContext
//
// Description  Dumps calling stack context and correspondent
//              symbolic information (win32 specific)
//--------------------------------------------------------------------------
static void DumpStackContext( const dbg_head *p )
{
  #if defined(WIN32) && !defined(__BOUNDSCHECKER__)  /* BC fails SymInitialize */
  HANDLE hProcess;
  char sModulePath[_MAX_PATH];
  char *psEnv;
  char *s;
  BYTE SymbolBuffer[sizeof(IMAGEHLP_SYMBOL) + 512];
  IMAGEHLP_SYMBOL *pSymbol;
  IMAGEHLP_MODULE stModule;
  IMAGEHLP_LINE stLine;
  DWORD dwSymDisplacement;
  DWORD dwAddr;
  int nIndex;
  DWORD dwOpts;
  static BOOLEAN bSymInit = FALSE;

  if (!bSymInit)
  {
    if (GetModuleFileName(NULL, sModulePath, sizeof(sModulePath)) == 0)
      return;

    s = strrchr(sModulePath, '\\');
    if (s != NULL)
      *s = '\0';
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
      return;
    }
    bSymInit = TRUE;
    printf("\nStack context information stored in DEBUG.LOG\n");
    TRACE0("Heap blocks calling stack context dump\n");
  }

  TRACE0("------\n");
  TRACE2("%p  %7u", p + 1, p->size);
  TRACE2(" :%s(%d)\n", p->file, p->line);

  nIndex = 0;
  hProcess = GetCurrentProcess();
  while (nIndex < MAXSTACKTRACE)
  {
    dwAddr = p->stkentries[nIndex++];
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

    if (SymGetModuleInfo(hProcess, dwAddr, &stModule))
      TRACE1("%s: ", stModule.ImageName);
    else
      TRACE0("<unknown module>: ");

    if (SymGetSymFromAddr(hProcess, dwAddr,
      &dwSymDisplacement, pSymbol))
    {
      TRACE2("%s +%x", pSymbol->Name, dwSymDisplacement);

      memset(&stLine, 0, sizeof(stLine));
      stLine.SizeOfStruct = sizeof(stLine);

      if (SymGetLineFromAddr(hProcess, dwAddr, &dwSymDisplacement, &stLine))
        TRACE2("\t%s, Line %d ", stLine.FileName, stLine.LineNumber);
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
      LocalFree(pMsgBuf);
      TRACE0("<unknown symbol>");
    }
    TRACE0("\n");
  }
  #endif
}

//--------------------------------------------------------------------------
// Name         IsValidBlockPrim
//
// Description  Checks if a heap block is valid
// Returns:
//      0 - ok
//      1 - can't access block (protected mode violation under Win32)
//      2 - Start signature is bad
//      3 - End signature is bad
//--------------------------------------------------------------------------
static int IsValidBlockPrim ( dbg_head * p )
{
  if (p == NULL)
    return 1;

#if defined(_WIN32)
  //
  // We are able to make some more validations under WIN32
  //
  if (IsBadReadPtr( p, sizeof( dbg_head ) ) ||
      IsBadReadPtr( p+1, p->size + END_MARK_LEN ))
  {
    return 1;
  }
#endif
  if (CalcCheckSum( p ) != p->checkSum)
    return 7;

  if (p->sgn[0] != 'D' ||
      p->sgn[1] != 'B' ||
      p->sgn[2] != 'G' ||
      p->sgn[3] != 'H')
  {
    return 2;
  }
  if (((BYTE *)(p + 1))[p->size]   != 'e' ||
      ((BYTE *)(p + 1))[p->size+1] != 'n' ||
      ((BYTE *)(p + 1))[p->size+2] != 'd' ||
      ((BYTE *)(p + 1))[p->size+3] != 'b')
  {
    return 3;
  }

  return 0;
};

#ifdef WIN32
#include <crtdbg.h>
#endif

//--------------------------------------------------------------------------
// Name         _dbg_validate_heap
//
// Description  Validates the entire heap.
//--------------------------------------------------------------------------
void _dbg_validate_heap ( const char * file, unsigned line )
{
  TListEntry * curP;
  int err;
  long blkCount;

  #ifdef WIN32
  _CrtCheckMemory();
  #endif

  blkCount = 0;
  for ( curP = heap.Flink; !END_OF_LIST( &heap, curP ); curP = curP->Flink )
  {
    ++blkCount;

    // Check if there are too many blocks
    //
    if (blkCount > _dbg_UsedBlocksCount)
    {
      printf( "\n\n!!! %s(%u): VALIDATE_HEAP() failed; %s\n\n", file, line,
              heapErrMsg[6] );
      AbortProg();
    }

    if ((err = IsValidBlockPrim( (dbg_head *)curP )) != 0)
    {
      printf( "\n\n!!! %s(%u): VALIDATE_HEAP(%p) failed; %s\n\n", file, line,
              curP + 1, heapErrMsg[err] );
      AbortProg();
    }
  }

  // Check if there are too few blocks
  //
  if (blkCount != _dbg_UsedBlocksCount)
  {
    printf( "\n\n!!! %s(%u): VALIDATE_HEAP() failed; %s\n\n", file, line,
            heapErrMsg[6] );
    AbortProg();
  }
}

//--------------------------------------------------------------------------
// Name         IsValidBlock
//
// Description  Checks if a heap block is valid
// Returns:
//      0 - ok
//      1 - can't access block (protected mode violation under Win32)
//      2 - Start signature is bad
//      3 - End signature is bad
//      4 - Invalid block (Block is not on heap list)
//      5 - Heap is corrupt
//      etc... for complete list of errors see heapErrMsg[]
//--------------------------------------------------------------------------
static int IsValidBlock ( dbg_head * p )
{
  PListEntry curP;
  long blkCount;

  if (p == NULL)
    return 1;

  // Iterate through the block list until we reach p
  //
  blkCount = 0;
  for ( curP = heap.Flink; ; curP = curP->Flink )
  {
    if (END_OF_LIST( &heap, curP ))
      return 4;                     // p is not in the list !

    ++blkCount;
    if (blkCount > _dbg_UsedBlocksCount)
      return 6;                     // the list is corrupt

    if (curP == &p->link)
      break;                        // we found the block

    if (IsValidBlockPrim( (dbg_head *)curP ) != 0)
      return 5;                     // The current block is damaged
  }

  return IsValidBlockPrim( p );     // validate p
};

//--------------------------------------------------------------------------
// Name         _dbg_validate_heap_ptr_size
//
// Description  Checks if a heap block pointer is valid
//--------------------------------------------------------------------------
void _dbg_validate_heap_ptr_size ( const void * ptr, unsigned size,
                                   const char * file, unsigned line )
{
  dbg_head * p;
  int  err;

  p = (dbg_head *)ptr - 1;
  if ((err = IsValidBlock( p )) != 0)
  {
    printf( "\n\n!!! %s(%u): VALIDATE_HEAP_PTR(%p) failed:%s\n\n", file, line,
            ptr, heapErrMsg[err] );
    AbortProg();
  }
  if (size != 0 && p->size != size)
  {
    printf( "\n\n!!! %s(%u): VALIDATE_HEAP_PTR(%p) size mismatch.\n"
            "size = %u, block size = %u\n\n", file, line, ptr, size, p->size );
    AbortProg();
  }
};

//--------------------------------------------------------------------------
// Name         _dbg_print_heap
//
// Description  Dumps heap statistics
//--------------------------------------------------------------------------
void _dbg_print_heap ( const char * msg )
{
  dbg_head * p;
  int err;

  printf( msg );
  printf( "|Used size |Peak size |Used blocks |Peak count |xmalloc() |xfree() |xrealloc()\n"
    "|%-10ld|%-10ld|%-12ld|%-11ld|%-10ld|%-8ld|%ld\n",
    _dbg_UsedHeapSize,
    _dbg_MaxUsedHeapSize,
    _dbg_UsedBlocksCount,
    _dbg_MaxUsedBlocksCount,
    _dbg_xmallocCount,
    _dbg_xfreeCount,
    _dbg_xreallocCount);

  for ( p = (dbg_head *)heap.Flink;
        !END_OF_LIST( &heap, p ); p = (dbg_head *)p->link.Flink )
  {
    if ((err = IsValidBlockPrim( p )) != 0)
    {
      printf( "*** INVALID BLOCK at %p:%s\n", p, heapErrMsg[err] );
      AbortProg();
    }

    printf( "%p  %7u :%s(%d)\n", p + 1, p->size, p->file, p->line );
    DumpStackContext( p );
  }
};

//--------------------------------------------------------------------------
// Name         InsertBlock
//
// Description  Inserts a block in the heap list.
//              Sets its checkSum and the checkSum of the prev. one
//--------------------------------------------------------------------------
static void InsertBlock ( dbg_head * p )
{
  INSERT_TAIL_LIST( &heap, &p->link );
  p->checkSum = CalcCheckSum( p );

  if (!END_OF_LIST( &heap, p->link.Blink ))
    ((dbg_head *)p->link.Blink)->checkSum = CalcCheckSum( (dbg_head *)p->link.Blink );
};

//--------------------------------------------------------------------------
// Name         RemoveBlock
//
// Description  Removes a block from the heap list.
//              Sets the check sum of the prev and next blocks
//--------------------------------------------------------------------------
static void RemoveBlock ( dbg_head * p )
{
  REMOVE_ENTRY_LIST( &p->link );

  if (!END_OF_LIST( &heap, p->link.Blink ))
    ((dbg_head *)p->link.Blink)->checkSum = CalcCheckSum( (dbg_head *)p->link.Blink );
  if (!END_OF_LIST( &heap, p->link.Flink ))
    ((dbg_head *)p->link.Flink)->checkSum = CalcCheckSum( (dbg_head *)p->link.Flink );
};

//--------------------------------------------------------------------------
// Name         SetMarks
//
// Description  Sets the start and end safety marks of the block
//--------------------------------------------------------------------------
static void SetMarks ( dbg_head * p )
{
  p->sgn[0] = 'D';
  p->sgn[1] = 'B';
  p->sgn[2] = 'G';
  p->sgn[3] = 'H';

  // Mark end of heap block
  ((BYTE *)(p + 1))[p->size]   = 'e';
  ((BYTE *)(p + 1))[p->size+1] = 'n';
  ((BYTE *)(p + 1))[p->size+2] = 'd';
  ((BYTE *)(p + 1))[p->size+3] = 'b';
};

//--------------------------------------------------------------------------
// Name         CopyFileName
//
// Description  Copies the file name to p->file. If the file name is too
//              long copies the last characters of the name. This is because
//              the first characters are likely to be directories and we
//              are not interested in them.
//--------------------------------------------------------------------------
static void CopyFileName ( dbg_head * p, const char * file )
{
  int l;

  l = strlen( file );
  if (l < sizeof( p->file ))
    memcpy( p->file, file, l + 1 );
  else
    memcpy( p->file, file + l - sizeof( p->file ) + 1, sizeof( p->file ) );
};

void * _dbg__xmalloc ( unsigned size, const char * file, unsigned line )
{
  dbg_head * p;

  ++_dbg_xmallocCount;

  if ((p = malloc( size + sizeof( dbg_head ) + END_MARK_LEN )) == NULL)
    return NULL;

  if ((_dbg_UsedHeapSize += size) > _dbg_MaxUsedHeapSize)
    _dbg_MaxUsedHeapSize = _dbg_UsedHeapSize;

  if (++_dbg_UsedBlocksCount > _dbg_MaxUsedBlocksCount)
    _dbg_MaxUsedBlocksCount = _dbg_UsedBlocksCount;

  p->size = size;
  p->line = line;
  CopyFileName( p, file );
  SetMarks( p );
  StoreStackContext( p );
  InsertBlock( p );

  memset( p + 1, MALLOC_FILL_VALUE, size );

  return p + 1;
};

void * _dbg_xmalloc ( unsigned size, const char * file, unsigned line )
{
  void * p;

  if ((p = _dbg__xmalloc( size, file, line )) == NULL)
    OutOfMemory();

  return p;
};

void _dbg_xfree ( void * toFree, const char * file, unsigned line )
{
  dbg_head * p;
  int err;

  if (toFree == NULL)
    return;

  ++_dbg_xfreeCount;

  p = (dbg_head *)toFree - 1;

#if HEAP_DBG == 1
  if ((err = IsValidBlockPrim( p )) != 0)
#else
  if ((err = IsValidBlock( p )) != 0)
#endif
  {
    printf( "\n\n!!! %s(%u): xfree() with invalid block:%s\n\n", file, line,
            heapErrMsg[err] );
    AbortProg();
  }

  _dbg_UsedHeapSize -= p->size;
  --_dbg_UsedBlocksCount;

  RemoveBlock( p );

  memset( p, FREE_FILL_VALUE, p->size + sizeof( dbg_head ) + END_MARK_LEN );

  free( p );
};

void * xrealloc ( void * blk, unsigned size )
{
  dbg_head * p;
  int  err;

  if (size == 0)
  {
    xfree( blk );
    return NULL;
  }

  ++_dbg_xreallocCount;

  p = (dbg_head *)blk - 1;
  if ((err = IsValidBlock( p )) != 0)
  {
    printf( "\n\n!!!: xrealloc() with invalid block:%s\n\n", heapErrMsg[err] );
    AbortProg();
  }

  _dbg_UsedHeapSize -= p->size;
  if ((_dbg_UsedHeapSize += size) > _dbg_MaxUsedHeapSize)
    _dbg_MaxUsedHeapSize = _dbg_UsedHeapSize;

  RemoveBlock( p );

  // Destroy marks
  memset( p->sgn, FREE_FILL_VALUE, sizeof( p->sgn ) );
  memset( (BYTE *)(p + 1) + p->size, FREE_FILL_VALUE, END_MARK_LEN );

  if ((p = realloc( p, size + sizeof( dbg_head ) + END_MARK_LEN )) == NULL)
    OutOfMemory();

  p->size = size;
  SetMarks( p );
  InsertBlock( p );

  return p + 1;
};

#else

void * xmalloc ( unsigned size )
{
  void * p;
  if ((p = _xmalloc( size )) == NULL)
    OutOfMemory();
  return p;
};

void * xrealloc ( void * p, unsigned size )
{
  if (size == 0)
  {
    free( p );
    return NULL;
  }
  if ((p = realloc( p, size )) == NULL)
    OutOfMemory();
  return p;
};

#endif

/////////////////////////////// String support ///////////////////////////

#ifdef HEAP_DBG

UCHAR * _dbg_AllocString ( unsigned len, const UCHAR * str,
                           const char * file, unsigned line )
{
  UCHAR * p = _dbg_xmalloc( len + 1, file, line );
  return memcpy( p, str, len + 1 );
};

#else

UCHAR * AllocString ( unsigned len, const UCHAR * str )
{
  UCHAR * p = xmalloc( len + 1 );
  return memcpy( p, str, len + 1 );
};

#endif

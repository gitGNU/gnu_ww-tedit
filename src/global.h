#ifndef GLOBAL_H
#define GLOBAL_H

////////////////////////////////////////////////////////////////////
// Standart Windows compatible type definitions

#ifndef USE_WINDOWS

// fix the defines that define the environment
// Everyone defines either _WIN32, WIN32, MSDOS,__MSDOS__, sometimes both.
// So, we use a general convention: WIN32 and MSDOS. We try to determine
// the environment by any possible means and define these apropriately.

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#if defined(__MSDOS__) && !defined(MSDOS)
#define MSDOS
#undef UNIX  /* as DJGPP defines unix and MSDOS */
#undef __unix__
#endif

#ifdef __linux__
#define LINUX
#define UNIX
#endif

#ifdef __unix__
#define UNIX
#endif

#ifdef __unix
#define UNIX
#endif

// FAR     : modifier for far pointer in large DOS models
// _DSPTR  : modifier for DS pointers in large DOS models
#if defined(MSDOS) && defined(__TURBOC__)
#  define _DSPTR  _ds
#  define FAR     _far
#  define HUGE    _huge
#else
#  define _DSPTR
#  define FAR
#  define HUGE
#endif

// Fixed size types
typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  BYTE;

typedef int           BOOL;
typedef unsigned      UINT;
typedef unsigned long ULONG;
typedef long          LONG;

typedef void *        PVOID;
typedef void FAR *    LPVOID;

typedef unsigned char UCHAR;

typedef unsigned char BOOLEAN;

typedef const char *   LPCTSTR;

#ifdef _UNICODE
#  define _T( x )     L ## x
#else
#  define _T( x )     x
#endif

#define TRUE  1
#define FALSE 0

///////////////////////////////////////////////////////////////////
// Useful macros defined in Windows.h

#define MulDiv( a, b, c )     ((long)(a) * (b) / (c))
#define MAKELONG( l, h )      ((WORD)(l) + ((DWORD)((WORD)(h)) << 16))

#ifdef __STDC__
#  define CDECL
#else
#  define CDECL  _cdecl
#endif

#else  //USE_WINDOWS
#  include <windows.h>
#  include <tchar.h>
#endif //USE_WINDOWS

////////////////////// All include files ////////////////////////
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>

#ifdef __GNUC__
#include <unistd.h>
#endif

///////////////////////////////////////////////////////////////////
// Useful macros

#ifndef offsetof
#  define offsetof( s, x )      ((UINT)&(((s *)0)->x))
#endif
#ifndef _countof
#  define _countof( x )         (sizeof( x ) / sizeof( (x)[0] ))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define ALIGN_TO(x, align_size) ((x) + (-(x) & (align_size-1)))

//////////////////////////////////////////////////////////////////
// Debug support

extern void debug_trace ( const char * fmt, ... );

#ifdef WIN32
extern void _assert_sc( const char *cond, const char *name, unsigned int line );
#define assert_sc( x ) (void)( (x) || ( _assert_sc( #x, __FILE__, __LINE__ ), 0 ) )
#endif

#ifdef _DEBUG
#  ifdef WIN32
#  define ASSERT( x )       assert_sc( x )
#  define ASSUME( x )
#  else
#  define ASSERT( x )       assert( x )
#  define ASSUME( x )
#  endif
#  define VERIFY( x )       ASSERT( x )
#  define TRACE0( x )       debug_trace( x )
#  define TRACE1( x, a1 )   debug_trace( x, a1 )
#  define TRACE2( x, a1, a2 )  debug_trace( x, a1, a2 )
#  define TRACE3( x, a1, a2, a3 )  debug_trace( x, a1, a2, a3 )
extern char szDebugLogName[];
extern BOOL bDebugTraceEnabled;
#else
#  define ASSUME( x )  __assume(x)
#  define ASSERT( x )
#  define VERIFY( x )       (x)
#  define TRACE0( x )
#  define TRACE1( x, a1 )
#  define TRACE2( x, a1, a2 )
#  define TRACE3( x, a1, a2, a3 )
#endif

///////////////////////////////////////////////////////////////////////////////
// definitions that could optimize the code but are specific for the compiler

#if !(_MSC_VER >= 1000)
#  define __assume(x)
#endif

// INLINE :specifies an explicit inline function
#if (_MSC_VER >= 1000) || (__GNUC__ > 1)
#  define INLINE  __inline
#  define SUPPORT_INLINE  1
#else
#  define INLINE
#endif

#ifndef HAVE_WIN32EH
#  define __try        
#  define __except(x)  if (0)
#endif

#define INTEL_BYTE_ORDER  1

// The section below is used to iron out the diferences between various
// compilers I have used. The general case is the last.
//
#if  defined(__TURBOC__)

//char * ultoa ( unsigned long num, char * res, int radix );
//char * ltoa ( long num, char * res, int radix );
//char * itoa ( int num, char * res, int radix );

#define  _stricmp stricmp

#elif defined(_MSC_VER)

#define snprintf _snprintf
#define vsnprintf _vsnprintf

//char * ultoa ( unsigned long num, char * res, int radix );
//char * ltoa ( long num, char * res, int radix );
//char * itoa ( int num, char * res, int radix );

//int _stricmp ( const char * dst, const char * src  );

#elif defined(__DJGPP__)

char * ultoa ( unsigned long num, char * res, int radix );
char * ltoa ( long num, char * res, int radix );
char * itoa ( int num, char * res, int radix );

#define  _stricmp stricmp

#elif defined(__WATCOMC__)

char * ultoa ( unsigned long num, char * res, int radix );
char * ltoa ( long num, char * res, int radix );
char * itoa ( int num, char * res, int radix );

#define  _stricmp stricmp

#elif defined(UNIX)

#ifndef __CYGWIN__
char * ultoa ( unsigned long num, char * res, int radix );
char * ltoa ( long num, char * res, int radix );
char * itoa ( int num, char * res, int radix );
char * strupr ( char * s );
char * strlwr ( char * s );

#define _stricmp(s, d) strcasecmp(s, d)  /* for compatibility to Ceco */
#define stricmp(s, d) strcasecmp(s, d)
#define strnicmp(s, d, n) strncasecmp(s, d, n)
#endif

#else

char * ultoa ( unsigned long num, char * res, int radix );
char * ltoa ( long num, char * res, int radix );
char * itoa ( int num, char * res, int radix );

int _stricmp ( const char * dst, const char * src  );
char * strupr ( char * s );
#endif

#endif // GLOBAL_H

#ifdef _WIN32
// we need some functions for pointer validation. So tell global.h that
// we need WINDOWS.H
#  ifndef USE_WINDOWS
#    define USE_WINDOWS
#  endif
#endif

#include "global.h"
#include <stdarg.h>
#include "maxpath.h"

char szDebugLogName[_MAX_PATH] = "debug.log";
BOOL bDebugTraceEnabled = TRUE;
static BOOL used = FALSE;

void debug_trace ( const char * fmt, ... )
{
  FILE * debugLog;

  if (!bDebugTraceEnabled)
    return;

#ifdef WIN32
  {
    char buf[256];
    va_list ap;
    va_start( ap, fmt );

    _vsnprintf( buf, sizeof(buf), fmt, ap );
    buf[sizeof(buf)-1] = 0;

    va_end( ap );

    OutputDebugString( buf );
  }
#endif

  if (!used)    // if first call to debug_trace => create empty file
  {
    used = TRUE;
    debugLog = fopen( szDebugLogName, "wt" );
  }
  else   // else just append to end of file
    debugLog = fopen( szDebugLogName, "at" );

  if (debugLog)
  {
    va_list ap;
    va_start( ap, fmt );

    vfprintf( debugLog, fmt, ap );
    fclose( debugLog );

    va_end( ap );
  }
  else
  {
    printf("Fatal: log file (%s) error - %s\n", szDebugLogName, strerror(errno));
    abort();
  };
};

/*

File: ww.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th November, 1998
Descrition:
  Win32 Console application startup module.

*/

#include "global.h"
#include "main2.h"

void OutOfMemory(void)
{
  puts("\n\nOut of memory!!!\n");
  abort();
}

#if defined(WIN32) && defined(_NON_TEXT)
HINSTANCE g_hInst = NULL;
int       g_nCmdShow = SW_NORMAL;

int WINAPI WinMain(
  HINSTANCE hInstance,      // handle to current instance
  HINSTANCE hPrevInstance,  // handle to previous instance
  LPSTR lpCmdLine,          // command line
  int nCmdShow              // show state
)
{
  extern int __argc;
  extern char ** __argv;

  // Store parameters in global variables
  g_hInst = hInstance; 
  g_nCmdShow = nCmdShow; 

  main2( __argc, __argv );

  return 0;
}

#else

int main(int argc, char **argv)
{
  extern DWORD FilterFunction(EXCEPTION_POINTERS *p_except);

  /* 
  Boost the current execution priotity to smooth the console performance.
  This is not necessary under WinNT as the console function performance is good,
  but under Win95 is poor. Anyway, boosting thread priority will do no harm under
  WinNT.
  Another way to boost the execitoin priority is to change the priority class
  of the process. But as this may be restricted under WinNT, it is best to choose
  boosting thread priority in its current priority class.

  Similar technique is applied at another console editor, the Q heir e32 for win32. 
  Start 'wintop' from MSPowerToys to see that the priority class is boosted.
  */
  /*SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);*/

  __try
  {
    main2(argc, argv);
  }
  __except(FilterFunction(GetExceptionInformation()))
  {
    printf("olleh");
  }

  return (0);
}
#endif

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


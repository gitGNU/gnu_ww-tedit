/*!
@file p_win_g.h
@brief [disp] WIN32 GUI platform specific implementation of the console API

Private definitions specific to the WIN32 GUI implementation of the
console API.

@section a Header

File: win_g_disp.h\n
COPYING: Full text of the copyrights statement at the bottom of the file\n
Project: WW text editor\n
Started: 27th October, 1998\n
Refactored: 27th Sep, 2006 from w_scr.c and w_kbd.h\n


@section b Module disp

Module disp is a standalone library that abstracts the access to
console output for WIN32 console, WIN32 GUI, ncurses and X11.

These are private data structures, function prototypes and definitions
specific to the WIN32 GUI implementation of the console API.


@section c Compile time definitions

D_ASSERT -- External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.

*/

#ifndef P_WIN_G_H
#define P_WIN_G_H

#pragma warning(disable:4200)
/* C4200: nonstandard extension used : zero-sized array in struct/union */

#define DISP_EVENT_QUEUE_SIZE 32

struct disp_char
{
  char c;
  unsigned char a;
};

#define disp_char_equal(c1, c2) (((c1).c == (c2).c) && ((c1).a == (c2).a))

typedef struct disp_char disp_char_t;

struct dispc
{
  #ifdef _DEBUG
  char *string_id;
  #endif

  enum disp_error code;
  char error_msg[MAX_DISP_ERROR_MSG_LEN];
  char os_error_msg[MAX_DISP_ERROR_MSG_LEN];

  disp_wnd_param_t geom_param;

  /*
  screen output
  */
  HWND  wnd;
  HFONT font;
  SIZE  char_size;
  TCHAR *window_class;

  disp_char_t *char_buf;
  int buf_height;
  int buf_width;

  int caption_height;
  int border_width;
  int cursor_is_visible;
  int window_holds_focus;
  int paint_is_suspended;  /* wait for the first put_text */

  int cursor_x;
  int cursor_y;

  void (*handle_resize)(disp_event_t *ev, void *ctx);
  void *handle_resize_ctx;

  int last_width;
  int last_height;
  int last_x;
  int last_y;

  /*
  input
  */
  HANDLE input;
  HANDLE timer;
  unsigned int ev_c;
  unsigned int ev_h;
  unsigned int ev_t;
  disp_event_t ev_q[DISP_EVENT_QUEUE_SIZE];
  int ctrl_is_released;

  /*
  elapsed time
  */
  int time_elapsed;
  int hours;
  int minutes;
  int seconds;
  int win32_timer_id;

  /*
  memory manager
  */
  void *(*safe_malloc)(size_t size);
  void (*safe_free)(void *buf);
};

struct disp_char_buf
{
  #ifdef _DEBUG
  unsigned char magic_byte;
  #define DISP_CHAR_BUF_MAGIC 0x63
  #endif

  int max_characters;

  disp_char_t cbuf[0];
};

/* To be used in ASSERT()! */
#ifdef _DEBUG
int disp_is_valid(const dispc_t *disp);
#define VALID_DISP(disp) (disp_is_valid(disp))
#else
#define VALID_DISP(disp) (1)
#endif

/*
External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.
*/
#ifdef _DEBUG

#ifdef D_ASSERT

/* Link to external assert() replacement */
extern void D_ASSERT(const char *cond, const char *name, unsigned int line);
#define D_ASSERT_WRAP(x) (void)((x) || (D_ASSERT(#x, __FILE__, __LINE__), 0))
#define ASSERT(x) D_ASSERT_WRAP(x)

#else

/* Fall back to standard C assert */
#include <assert.h>
#define ASSERT(x) assert(x)

#endif

#else

#define ASSERT(x)

#endif  /* ifdef _DEBUG */

#ifdef _UNICODE
#  define _T( x )     L ## x
#else
#  define _T( x )     x
#endif

#endif  /* ifndef P_WIN_G_H */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2007
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

/*!
@file ncurs_disp.c
@brief ncurses platform specific implementation of the console API

@section a Header

File: ncurs_disp.c\n
COPYING: Full text of the copyrights statement at the bottom of the file\n
Project: WW text editor\n
Started: 27th October, 1998\n
Refactored: 27th Sep, 2006 from l_scr.c and l_kbd.h\n


@section b Module disp

Module disp is a standalone library that abstracts the access to
console output for WIN32 console, WIN32 GUI, ncurses and X11.

This is an implementation of the API described in disp.h for ncurses
library.


@section c Compile time definitions

D_ASSERT -- External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.
*/

#include <stdio.h>
#include <malloc.h>
#include "disp.h"
#include "p_ncurs.h"

#include "disp_common.c"

/*!
@brief Marks that area of the window has been changed. (win32 GUI)

Call to this function will eventually make the text of the area
appear on the output window.

@param disp  a dispc object
@param x    upper left corner coordinates of the destination rectangle
@param y    upper left corner coordinates of the destination rectangle
@param w    rectangle geometry
@param h    rectangle geometry
*/
static void s_disp_validate_rect(dispc_t *disp,
                                 int x, int y,
                                 int w, int h)
{
}

/*!
@brief Waits for event from the display window. (win32 GUI)

The function also is the event pump on GUI platforms.

@param disp  a dispc object
@param event receives the next event
@return 0 failure in system message loop
@return 1 no error
*/
int disp_event_read(dispc_t *disp, disp_event_t *event)
{
  return 0;
}

/*!
@brief Makes the caret visible or invisible (win32 GUI)

@param disp              a dispc object
@param caret_is_visible  new state of the caret
*/
static void s_disp_show_cursor(dispc_t *disp, int caret_is_visible)
{
}


#if 0
#define KEY_TRACE0 debug_trace
#define KEY_TRACE1 debug_trace
#define KEY_TRACE2 debug_trace
#else
#define KEY_TRACE0 (void)
#define KEY_TRACE1 (void)
#define KEY_TRACE2 (void)
#endif

/*!
@brief initial setup of display (win32 GUI)

Sets up font, DC and win32 instance

@param disp a dispc object
@returns true for success
@returns false for failure and error messages and code are set
*/
static int s_disp_init(dispc_t *disp)
{
  return 1;
}

/*!
@brief platform specific disp cleanup (win32 GUI)

@param disp  a display object
*/
static void s_disp_done(dispc_t *disp)
{
}

/*!
@brief Sets the caret on a specific position (win32 GUI)

@param disp  a display object
@param x     coordinates in character units
@param y     coordinates in character units
*/
static void s_disp_set_cursor_pos(dispc_t *disp, int x, int y)
{
}

/*!
@brief Changes the title of the window (win32 GUI)

@param disp    a dispc object
@param title   a string for the title
*/
static void s_disp_wnd_set_title(dispc_t *disp, const char *title)
{
}

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

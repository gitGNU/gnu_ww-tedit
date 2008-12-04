/*!

@file disp.h
@brief [disp] Multiplatform console API. Public interface file.

@section a Header

File: disp.h\n
COPYING: Full text of the copyrights statement at the bottom of the file\n
Project: WW text editor\n
Started: 27th October, 1998\n
Refactored: 19th Feb, 2006 from scr.h and kbd.h\n


@section b Module disp

Module disp is a standalone library that abstracts the access to
console output for WIN32 console, WIN32 GUI, ncurses and X11.

@section c Compile time definitions

D_ASSERT -- External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.

DISP_MAX_PAL -- default is 128, define externally to increase palette size

*/

/*!
@defgroup group_disp disp -- display access
@{
*/

#ifndef DISP_H
#define DISP_H

#ifndef DISP_MAX_PAL
#define DISP_MAX_PAL 128
#endif

#define MAX_FONT_NAME_LEN 256
#define MAX_DISP_ERROR_MSG_LEN  256

/*! @brief display object */
typedef struct dispc dispc_t;
/*! @brief window set/get geometry structure */
typedef struct disp_wnd_param disp_wnd_param_t;
/*! @brief character/attribute buffer */
typedef struct disp_char_buf disp_char_buf_t;
/*! @brief cursor set/get geometry structure */
typedef struct disp_cursor_param disp_cursor_param_t;
/*! @brief event structure */
typedef struct disp_event disp_event_t;
/*! @brief counts time since start of the module */
typedef struct disp_elapsed_time disp_elapsed_time_t;

/* Not visible for user of the module */
struct dispc;
struct disp_char_buf;

int  disp_obj_size(void);
int  disp_init(const disp_wnd_param_t *wnd_param, void *disp_obj);

void disp_done(dispc_t *disp);

enum disp_error
{
  /*! no error */
  DISP_NO_ERROR = 0,
  /*! attempt to load system font failed */
  DISP_FONT_FAILED_TO_LOAD,
  /*! attempt to register main window failed */
  DISP_FAILED_TO_OPEN_WINDOW,
  /*! attempt to open main window failed */
  DISP_FAILED_TO_CREATE_WINDOW,
  /*! function call to position main window failed */
  DISP_FAILED_TO_POSITION_WINDOW,
  /*! message loop failure */
  DISP_MESSAGE_LOOP_FAILURE,
  /*! failed to set a timer */
  DISP_FAILED_TIMER_SETUP,
  /*! ncurses failed to initialize */
  DISP_FAILED_NCURS_INIT,
  /*! terminal doesn't support cursor operations */
  DISP_TERMINAL_NO_CURSOR_OPERATIONS,
  /*! terminal doesn't support color */
  DISP_TERMINAL_NO_COLOR,
  /*! ncurses mode setup failure */
  DISP_NCURSES_MODE_SETUP_FAILURE,
  /*! failed in allocating color pairs */
  DISP_NCURSES_COLOR_ALLOC_FAIL,
  /*! no more room for font styles */
  DISP_FONT_STYLE_OVERFLOW,
  /*! no more space in palette table */
  DISP_PALETTE_FULL,
};

void disp_error_get(dispc_t *disp, enum disp_error *code,
                    char **error, char **os_error);
void disp_error_clear(dispc_t *disp);

void disp_set_safemem_proc(dispc_t *disp,
                        void *(*safe_malloc)(size_t size),
                        void (*safe_free)(void *buf));

struct disp_wnd_param
{
  int  set_defaults;
  int  pos_x;
  int  pos_y;
  int  height;
  int  width;
  int  is_maximized;
  char font_name[MAX_FONT_NAME_LEN];
  int  font_size;
  unsigned long font1_style;
  unsigned long font2_style;
  unsigned long font3_style;
  int height_before_maximize;
  int width_before_maximize;
  int instance;  /* used only by disp_init */
  int show_state;  /* used only by disp_init */
};

void disp_wnd_set_param(dispc_t *disp, const disp_wnd_param_t *wnd_param);
void disp_wnd_get_param(const dispc_t *disp, disp_wnd_param_t *wnd_param);
int  disp_wnd_get_width(const dispc_t *disp);
int  disp_wnd_get_height(const dispc_t *disp);

void disp_set_resize_handler(dispc_t *disp,
                             void (*handle_resize)(struct disp_event *ev,
                                                   void *ctx),
                             void *handle_resize_ctx);

/* bit masks for font_style */
#define DISP_FONT_ITALIC 1
#define DISP_FONT_BOLD  2
#define DISP_FONT_UNDERLINE 4
#define DISP_FONT_REVERSE 8

unsigned long disp_pal_get_standard(const dispc_t *disp, int color);
unsigned long disp_pal_compose_rgb(const dispc_t *disp, int r, int g, int b);
int disp_pal_add(dispc_t *disp,
                 unsigned long rgb_color, unsigned long rgb_background,
                 unsigned font_style, int *palette_id);
void disp_pal_free(dispc_t *disp, int palette_id);

int disp_is_only_basic_colors(dispc_t *disp);
int disp_is_gui(dispc_t *disp);

void disp_cbuf_reset(const dispc_t *disp, disp_char_buf_t *cbuf, int max_characters);
void disp_cbuf_mark_invalid(const dispc_t *disp, disp_char_buf_t *cbuf);
void disp_cbuf_put_char(const dispc_t *disp,
                        disp_char_buf_t *dest_buf, int index, char c);
void disp_cbuf_put_attr(const dispc_t *disp,
                        disp_char_buf_t *dest_buf, int index, int attr);
void disp_cbuf_put_char_attr(const dispc_t *disp,
                             disp_char_buf_t *dest_buf,
                             int index, int c, int attr);

int  disp_calc_rect_size(const dispc_t *disp, int width, int height);

void disp_put_block(dispc_t *disp,
                    int x, int y, int w, int h, const disp_char_buf_t *buf);
void disp_get_block(const dispc_t *disp,
                    int x, int y, int w, int h, disp_char_buf_t *buf);

void disp_write(dispc_t *disp,
                const char *s, int x, int y, int attr);
void disp_flex_write(dispc_t *disp,
                     const char *s, int x, int y, int attr1, int attr2);
void disp_fill(dispc_t *disp, char c, int attr, int x, int y, int count);

struct disp_cursor_param
{
  int cursor_is_visible;
  int pos_x;
  int pos_y;
};

void disp_cursor_hide(dispc_t *disp);
void disp_cursor_goto_xy(dispc_t *disp, int x, int y);
void disp_cursor_get_xy(dispc_t *disp, int *x, int *y);

void disp_cursor_get_param(dispc_t *disp,
                           disp_cursor_param_t *cparam);
void disp_cursor_restore_param(dispc_t *disp,
                               const disp_cursor_param_t *cparam);

void disp_wnd_set_title(dispc_t *disp, const char *title);

/* Shift state mask constants */
#define kbShift      1
#define kbReserved   2
#define kbCtrl       4
#define kbAlt        8
#define kbScroll     16
#define kbNum        32
#define kbCaps       64


/*
Convertion macros:
A key is universaly presented by a DWORD -- hi WORD is shift state, lo
WORD is KEY code; KEY code on other hand is as follows -- hi BYTE is
keyboard scan code for this key, lo BYTE is ascii code of the key.
*/
#define KEY(state, scan)	(((unsigned long)(((unsigned long)state) << 16) | (scan << 8)))
#define ASC(key)	((char)(key & 0x000000ff))
#define SH_STATE(key)	((unsigned short)((key & 0xffff0000l) >> 16))
#define SCANCODE(key)	((unsigned char)((key & 0x0000ff00) >> 8))
#define NO_ASC(key)     ((unsigned long)(key & 0xffffff00l))
#define NO_SH_STATE(key)	((unsigned long)(key & 0x0000ffff))
#define NO_SH_STATE2(state, key)    ((unsigned long)(key & ~KEY(state, 0)))

enum key_defs
{
  kbEsc=1,
  kb1, kb2, kb3, kb4, kb5, kb6, kb7, kb8, kb9,
  kb0, kbMinus, kbEqual, kbBckSpc, kbTab, kbQ,
  kbW, kbE, kbR, kbT, kbY, kbU, kbI, kbO, kbP,
  kbLBrace, kbRBrace, kbEnter, kb_Ctrl, kbA, kbS,
  kbD, kbF, kbG, kbH, kbJ, kbK, kbL, kbColon, kb_1,
  kb_2, kb_RShift, kbBSlash, kbZ, kbX, kbC, kbV,
  kbB, kbN, kbM, kbComa, kbPeriod, kbSlash, kb_LShift,
  kbPrSc, kb_Alt, kbSpace, kbCapsLock, kbF1, kbF2,
  kbF3, kbF4, kbF5, kbF6, kbF7, kbF8, kbF9, kbF10,
  kbNumLock, kbScrLock, kbHome, kbUp,
  kbPgUp, kbGrayMinus, kbLeft, kbPad5, kbRight,
  kbGrayPlus, kbEnd, kbDown, kbPgDn, kbIns, kbDel,
  kbF11=87, kbF12=88, kbTilda,
};

enum event_type
{
  EVENT_NONE = -1,
  EVENT_KEY = 1,
  EVENT_MOUSE,
  EVENT_RESIZE,
  EVENT_TIMER_5SEC,
  EVENT_CLIPBOARD_PASTE,
  EVENT_CLIPBOARD_CLEAR,
  EVENT_CLIPBOARD_COPY_REQUESTED,
  EVENT_USR
};

#define MOUSE_BUTTON1 1
#define MOUSE_RIGHTMOST 2
#define MOUSE_BUTTON2 4
#define MOUSE_BUTTON3 8
#define MOUSE_BUTTON4 16

#define MOUSE_DOUBLE_CLICK 1
#define MOUSE_MOVE 2
#define MOUSE_WHEEL 4

struct disp_event
{
  #ifdef _DEBUG
  unsigned char magic_byte;
  #define EVENT_MAGIC 0x5f
  #endif

  /* defines event type and additional data */
  struct
  {
    enum event_type code;
    int msg;
    int user_msg_code;
  } t;

  /* extra user data attached to the event */
  void *data1;
  void *data2;

  /* keyboard, mouse or resize event data */
  union
  {
    int param;
    void *pdata;

    struct
    {
      int param1;
      int param2;
      int param3;
      int param4;
      int param5;
    } p;

    struct
    {
      int x1;
      int y1;
      int width;
      int height;
    } new_size;

    struct
    {
      int x;
      int y;
      int button_state;
      int ctrl_state;
      int flags;
      int button_data;
    } mouse;

    struct
    {
      /* combined scan_code + shift_state + ascii */
      unsigned long key;
      /* separate scan_code + shift_state */
      enum key_defs scan_code_only;
      int shift_state;
      /* special case for ctrl_released flag */
      int ctrl_is_released;
    } kbd;
  } e;
};

int disp_event_read(dispc_t *disp, disp_event_t *event);
void disp_event_clear(disp_event_t *event);
int  disp_event_is_valid(const disp_event_t *event);

void disp_get_key_name(dispc_t *disp,
  unsigned long key, char *key_name_buf, int buf_size);

struct disp_elapsed_time
{
  int hours;
  int minutes;
  int seconds;
  int total_seconds;
};

void disp_elapsed_time_get(dispc_t *disp, disp_elapsed_time_t *t);
void disp_elapsed_time_set(dispc_t *disp, const disp_elapsed_time_t *t);

/*!
@}
*/

/*!
@page disp_page1 TODO: documenting disp module

disp library

--manages a window for input and output\n
--main event loop\n

Files:

disp_common.c -- the top level API functions, it is included in the
platform specific implementations.

disp_win_g.c -- WIN32 GUI emulation functions. It includes disp_common.c

disp_ncurs.c -- ncurses functions. It includes disp_common.c

Depends:

disp depends only on standard libraries

*/

#endif  /* ifndef DISP_H */

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2008
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


/*

File: xscr_i.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th Jun, 2005
Descrition:
  Internal structures for X Window screen and keyboard

*/

#ifndef XSCR_I_H
#define XSCR_I_H

#ifndef UNIX
#error UNIX platforms only
#endif

#ifndef _NON_TEXT
#error X Window only
#endif

#include <X11/Xlib.h>

struct disp_ctx
{
  Display *x11_disp;
  int screen_num;
  Window h_win;
  Window h_root_win;

  int char_height;
  int char_width;
  int char_ascent;

  BOOLEAN wait_first_expose;

  int cursor_x;
  int cursor_y;
  int last_cursor_x;
  int last_cursor_y;
  BOOLEAN cursor_visible;

  int top_window_width;
  int top_window_height;
};

Atom disp_get_atom(const struct disp_ctx *disp, const char *atom_name);
void disp_on_paint(const struct disp_ctx *disp, int x1, int y1, int x2, int y2);
void disp_draw_cursor(struct disp_ctx *disp);

#endif


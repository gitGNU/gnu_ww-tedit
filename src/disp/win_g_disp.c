/*!
@file win_g_disp.c
@brief WIN32 GUI platform specific implementation of the console API

@section a Header

File: win_g_disp.c\n
COPYING: Full text of the copyrights statement at the bottom of the file\n
Project: WW text editor\n
Started: 27th October, 1998\n
Refactored: 27th Sep, 2006 from w_scr.c and w_kbd.h\n


@section b Module disp

Module disp is a standalone library that abstracts the access to
console output for WIN32 console, WIN32 GUI, ncurses and X11.

This is an implementation of the API described in disp.h for WIN32 GUI
platform. It emulates console oriented output into a GUI window.


@section c Compile time definitions

D_ASSERT -- External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <malloc.h>
#include "disp.h"
#include "p_win_g.h"

#include "disp_common.c"

/*!
@brief Gets human readable message for a system error (win32 GUI)

@param disp  a dispc object
*/
static void s_disp_translate_os_error(dispc_t *disp)
{
  LPVOID msg_buf;
  DWORD win_err;

  win_err = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                NULL, win_err,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&msg_buf,
                0, NULL);

  _snprintf(disp->os_error_msg, sizeof(disp->os_error_msg),
            "WINDOWS error %#0x: %s", win_err, msg_buf);
}

/*!
@brief Marks area of the screen as invalid. (win32 GUI)

This will make Windows generate WM_PAINT event for this area and
the data will then be moved from disp->char_buf to the display

@param disp a dispc object
@param r    the RECT
*/
static void s_disp_invalidate_rect(const dispc_t *disp, const RECT *r)
{
  int window_right;
  int window_bottom;
  RECT r2;

  memcpy(&r2, r, sizeof(RECT));
  window_right = disp->geom_param.width * disp->char_size.cx;
  window_bottom = disp->geom_param.height * disp->char_size.cy;
  if (r2.right == window_right);
    r2.right += disp->char_size.cx; /* for the artefact @ maximize */
  if (r2.bottom == window_bottom);
    r2.bottom += disp->char_size.cy;
  InvalidateRect(disp->wnd, &r2, FALSE);
  /*debug_trace("disp_invalidate_rect: %d, %d to %d, %d\n",
              r2.left, r2.top, r2.right, r2.bottom);*/
}

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
  RECT area;

  ASSERT(w > 0);
  ASSERT(h > 0);
  ASSERT(x >= 0);
  ASSERT(y >= 0);

  disp->paint_is_suspended = 0;
  s_disp_calc_area(disp, &area, x, y, w, h);
  s_disp_invalidate_rect(disp, &area);
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
  MSG msg;

  event->t.code = EVENT_NONE;
  for (;;)
  {
    if (s_disp_ev_q_get(disp, event))
      return 1;

    if (!GetMessage(&msg, NULL, 0, 0))
    {
      disp->code = DISP_MESSAGE_LOOP_FAILURE;
      _snprintf(disp->error_msg, sizeof(disp->error_msg),
                "failed to get message");
      s_disp_translate_os_error(disp);
      return 0;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

/*!
@brief Makes the caret visible or invisible (win32 GUI)

@param disp              a dispc object
@param caret_is_visible  new state of the caret
*/
static void s_disp_show_cursor(dispc_t *disp, int caret_is_visible)
{
  if (disp->cursor_is_visible != caret_is_visible)
  {
    disp->cursor_is_visible = caret_is_visible;
    if (disp->window_holds_focus)
    {
      if (caret_is_visible)
        ShowCaret(disp->wnd);
      else
        HideCaret(disp->wnd);
    }
  }
}

/*
@brief default parameters for the display
*/
static disp_wnd_param_t s_default_wnd_param =
{
  0,   /* int  set_defaults; */
  10,  /* int  pos_x; */
  10,  /* int  pos_y; */
  50,  /* int  height; */
  80,  /* int  width; */
  0,   /* int  is_maximized; */
  "courier new",  /* char font_name[MAX_FONT_NAME_LEN]; */
  16,  /* int  font_size; */
  0,   /* unsigned long font1_style; */
  0,   /* unsigned long font2_style; */
  0,   /* unsigned long font3_style; */
  50,  /* int height_before_maximize; */
  80,  /* int width_before_maximize; */
  0,   /* int instance; */
  0,   /* int show_state; */
};

/*!
@brief 16 colors
*/
static DWORD s_disp_colors[16] =
{
  RGB( 0x00, 0x00, 0x00 ),
  RGB( 0x00, 0x00, 0x80 ),
  RGB( 0x00, 0x80, 0x00 ),
  RGB( 0x00, 0x80, 0x80 ),
  RGB( 0x80, 0x00, 0x00 ),
  RGB( 0x80, 0x00, 0x80 ),
  RGB( 0x80, 0x80, 0x00 ),
  RGB( 0xC0, 0xC0, 0xC0 ),

  RGB( 0x80, 0x80, 0x80 ),
  RGB( 0x00, 0x00, 0xFF ),
  RGB( 0x00, 0xFF, 0x00 ),
  RGB( 0x00, 0xFF, 0xFF ),
  RGB( 0xFF, 0x00, 0x00 ),
  RGB( 0xFF, 0x00, 0xFF ),
  RGB( 0xFF, 0xFF, 0x00 ),
  RGB( 0xFF, 0xFF, 0xFF ),
};

/*!
@brief Called whenever a segment of the window must be redrawn  (win32 GUI)

The WM_PAINT message is sent when the system or another application makes
a request to paint a portion of an application's window. This function
validates the update region.

@param disp  a dispc object
@param dc    drawing device context
@param upd   the update region
*/
static void s_disp_on_paint(dispc_t *disp, HDC dc, const RECT *upd)
{
  int y;
  int x1, y1, x2, y2;
  int cur_attr;
  int w;
  int c;
  RECT rc;
  disp_char_t *char_ln;
  char *disp_buf;
  int disp_buf_size;
  int segment_start;
  int segment_len;
  int correction_len;

  /* check for empty update area */
  if (upd->left == upd->right || upd->top == upd->bottom)
    return;

  SelectObject(dc, disp->font);

  x1 = upd->left / disp->char_size.cx;
  x2 = (upd->right - 1) / disp->char_size.cx + 1;
  if (x2 > disp->geom_param.width)
    x2 = disp->geom_param.width;
  ASSERT(x2 > x1);

  y1 = upd->top / disp->char_size.cy;
  y2 = (upd->bottom - 1) / disp->char_size.cy + 1;
  if (y2 > disp->geom_param.height)
    y2 = disp->geom_param.height;
  ASSERT(y2 > y1);

  disp_buf_size = disp->geom_param.width;
  disp_buf = alloca(disp_buf_size);
  ASSERT(disp_buf != NULL);  /* no escape from here */

  cur_attr = 0;
  rc.left = rc.top = rc.bottom = rc.right = 0;

  for (y = y1; y < y2; ++y)
  {
    /*
    Line might hold a few segments of text with different attributes
    They all have to be output one by one
    */
    w = x2 - x1;
    c = w;
    segment_start = 0;
    while (c > 0)  /* for all segments of text from this line */
    {
      if (disp->paint_is_suspended)  /* we don't yet have charbuf? */
      {
        /* display white spaces */
        segment_len = w;
        memset(disp_buf, ' ', w);
        cur_attr = s_disp_colors[15];
      }
      else
      {
        /* copy a region of text that has the same attributes */
        char_ln = s_disp_buf_access(disp, x1, y);

        segment_len = 0;
        cur_attr = char_ln[segment_start].a;
        while (cur_attr == char_ln[segment_start + segment_len].a)
        {
          disp_buf[segment_len] =
            char_ln[segment_start + segment_len].c;
          ++segment_len;
          if (segment_start + segment_len == w)
            break;
        }
      }

      /*
      disp_buf[segment_len] = '\0';
      debug_trace("ln: %d @%d %#0x %s*\n",
                  y, x1 + segment_start, cur_attr, disp_buf);
      */

      /* rc represents one square in pixels, it is a line of characters */
      rc.left = (x1 + segment_start) * disp->char_size.cx;
      rc.top = y * disp->char_size.cy;
      rc.bottom = rc.top + disp->char_size.cy;

      rc.right = rc.left + segment_len * disp->char_size.cx;

      SetTextColor(dc, s_disp_colors[cur_attr & 15]);
      SetBkColor(dc, s_disp_colors[(cur_attr >> 4) & 15]);

      TextOut(dc, rc.left, rc.top, disp_buf, segment_len);

      /*
      Patch artifacts for the case the window is maximized
      (screen might happen to be not a multiple of char_size)
      */
      if (y == disp->geom_param.height - 1)  /* last line on screen? */
      {
        /* Output one entire line with the color of the last line */
        correction_len = segment_len;
        if (x2 == disp->geom_param.width)
          ++correction_len;  /* correct the bottom right corner */
        memset(disp_buf, ' ', correction_len);
        TextOut(dc, rc.left, rc.top + disp->char_size.cy, disp_buf, correction_len);
      }

      rc.left = rc.right;  /* this for after the loop */
      c -= segment_len;
      segment_start += segment_len;
      ASSERT(c >= 0);
    }

    /*
    Patch artifacts for the case the window is maximized
    (screen might happen to be not a multiple of char_size)
    */
    if (x2 == disp->geom_param.width)  /* last char on screen? */
    {
      /* Output one space with the color of the last character */
      SetTextColor(dc, s_disp_colors[cur_attr & 15]);
      SetBkColor(dc, s_disp_colors[(cur_attr >> 4) & 15]);

      /* rc.left is already at the edge of the screen */
      TextOut(dc, rc.left, rc.top, " ", 1);
    }
  }  /* for (y = y1; y < y2; ++y) */
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
@brief Translation of Windows keyboard endcodings to disp keydefs  (win32 GUI)

Generates and enqueues an event by translating from Windows message
and params passed to the s_disp_wnd_proc().
*/
static void s_disp_on_key(dispc_t *disp,
                          unsigned message,
                          unsigned param1,
                          unsigned param2)
{
  disp_event_t ev;
  unsigned char ascii_code;
  enum key_defs scan_code;
  unsigned int shift_state;
  unsigned long int code;
  MSG msg;
  char buf[1024];

  switch (message)
  {
    case WM_KEYUP:
      KEY_TRACE0("WM_KEYUP\n");
      break;
    case WM_SYSKEYUP:
      KEY_TRACE0("WM_SYSKEUP\n");
      break;
    case WM_CHAR:
      KEY_TRACE0("WM_CHAR ");
      break;
    case WM_DEADCHAR:
      KEY_TRACE0("WM_DEADCHAR ");
      break;
    case WM_SYSCHAR:
      KEY_TRACE0("WM_SYSCHAR ");
      break;
    case WM_SYSDEADCHAR:
      KEY_TRACE0("WM_SYSDEADCHAR ");
      break;
    case WM_KEYDOWN:
      KEY_TRACE0("WM_KEYDOWN ");
      break;
    case WM_SYSKEYDOWN:
      KEY_TRACE0("WM_SYSKEYDOWN ");
      break;
    default:
      KEY_TRACE0("WM_UKNOWN ");
  }

  ascii_code = (unsigned char)(param1 & 255);
  scan_code = (unsigned char)((param2 >> 16) & 255);
  shift_state = 0;
  if (GetKeyState( VK_MENU ) < 0)
    shift_state |= kbAlt;
  if (GetKeyState( VK_CONTROL ) < 0)
    shift_state |= kbCtrl;
  if (GetKeyState( VK_SHIFT ) < 0)
    shift_state |= kbShift;
  code = ((unsigned long int)shift_state) << 16 | (scan_code << 8) | ascii_code;

  disp_event_clear(&ev);
  ev.t.code = EVENT_KEY;

  if (message == WM_KEYUP && scan_code == kb_Ctrl)
    ev.e.kbd.ctrl_is_released = 1;

  if (message == WM_SYSKEYDOWN)
  {
    if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
    {
      if (msg.message == WM_SYSCHAR)
      {
        KEY_TRACE0("[WM_SYSCHAR] ");
        ascii_code = 0;
        scan_code = (unsigned char)((msg.lParam >> 16) & 255);
        code = ((unsigned long int)shift_state) << 16
               | (scan_code << 8) | ascii_code;
        disp_get_key_name(disp, code, buf, sizeof(buf));
        KEY_TRACE2("sys_key: %s, ascii: %c\n", buf, ascii_code);

        ev.e.kbd.scan_code_only = scan_code;
        ev.e.kbd.shift_state = shift_state;
        ev.e.kbd.key = code;
        s_disp_ev_q_put(disp, &ev);
        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
      }
    }
    else
      if (!(scan_code == kb_Ctrl || scan_code == kb_RShift ||
          scan_code == kb_LShift || scan_code == kb_Alt ||
          scan_code == kbCapsLock || scan_code == kbNumLock ||
          scan_code == kbScrLock))
      {
        ascii_code = 0;
        code = ((unsigned long int)shift_state) << 16
               | (scan_code << 8) | ascii_code;
        disp_get_key_name(disp, code, buf, sizeof(buf));
        KEY_TRACE2("sys_key: %s, ascii: %c\n", buf, ascii_code);

        ev.e.kbd.scan_code_only = scan_code;
        ev.e.kbd.shift_state = shift_state;
        ev.e.kbd.key = code;
        s_disp_ev_q_put(disp, &ev);
      }
  }
  else if (message == WM_KEYDOWN)
  {
    if (!(scan_code == kb_Ctrl || scan_code == kb_RShift ||
        scan_code == kb_LShift || scan_code == kb_Alt ||
        scan_code == kbCapsLock || scan_code == kbNumLock ||
        scan_code == kbScrLock))
    {
      if (scan_code == kbPrSc || scan_code == kbHome ||
         scan_code == kbUp || scan_code == kbPgUp ||
         scan_code == kbGrayMinus || scan_code == kbLeft ||
         scan_code == kbPad5 || scan_code == kbRight ||
         scan_code == kbGrayPlus || scan_code == kbEnd ||
         scan_code == kbDown || scan_code == kbPgDn ||
         scan_code == kbIns || scan_code == kbDel)
      scan_code = 0;  /* in case of num-lock */

      if (scan_code == kbEsc)
        ascii_code = 0;
      if (scan_code == kbEnter)
        ascii_code = 0;
      if (scan_code == kbTab)
        ascii_code = 0;
      if (scan_code == kbBckSpc)
        ascii_code = 0;

      if (PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE) && msg.message == WM_CHAR)
      {
        KEY_TRACE0("[WM_CHAR] ");
        if (ascii_code != 0)
          ascii_code = (unsigned char)(msg.wParam & 255);
        code = ((unsigned long int)shift_state) << 16
               | (scan_code << 8) | ascii_code;
        disp_get_key_name(disp, code, buf, sizeof(buf));
        KEY_TRACE2("key: %s, ascii: %c\n", buf, ascii_code);

        ev.e.kbd.scan_code_only = scan_code;
        ev.e.kbd.shift_state = shift_state;
        ev.e.kbd.key = code;
        s_disp_ev_q_put(disp, &ev);

        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
      }
      else
      {
        scan_code = (BYTE)((param2 >> 16) & 255);
        ascii_code = 0;  /* a non WM_CHAR key */
        code = ((unsigned long int)shift_state) << 16
               | (scan_code << 8) | ascii_code;
        disp_get_key_name(disp, code, buf, sizeof(buf));
        KEY_TRACE1("just_a_key2: %s\n", buf);

        ev.e.kbd.scan_code_only = scan_code;
        ev.e.kbd.shift_state = shift_state;
        ev.e.kbd.key = code;
        s_disp_ev_q_put(disp, &ev);
      }
    }
  }  /* message == WM_KEYDOWN */
}

/*!
@brief Aligns new size to certain granularity in units  (win32 GUI)
*/
static int s_disp_align_size(int new_size, int units)
{
  new_size /= units;
  new_size *= units;
  return new_size;
}

/*!
@brief Call back function for Windows events  (win32 GUI)
*/
static LRESULT CALLBACK
s_disp_wnd_proc(HWND hWnd,
                UINT message,
                WPARAM wParam,
                LPARAM lParam)
{
  WINDOWPOS *win_pos;
  WINDOWPLACEMENT wndpl;
  extern int caption_height;
  extern int border_width;
  static int last_width = 0;
  static int last_height = 0;
  static int last_x = 0;
  static int last_y = 0;
  int resize_x;
  int resize_y;
  int move_x;
  int move_y;
  int new_height;
  int new_width;
  disp_event_t ev;
  CREATESTRUCT *create;
  dispc_t *disp;
  int real_caret_pos_x;
  int real_caret_pos_y;
  PAINTSTRUCT ps;
  HDC dc;

  disp = (dispc_t *)GetWindowLong(hWnd, GWL_USERDATA);

  if (disp != NULL)
  {
    ASSERT(VALID_DISP(disp));
    ASSERT(disp->wnd == hWnd);
    if (!(GetKeyState(VK_CONTROL) < 0))
      disp->ctrl_is_released = 1;
  }

  switch (message)
  {
    case WM_CREATE:
      /* Transfer lParam into SetWindoLong() */
      create = (CREATESTRUCT *)lParam;
      SetWindowLong(hWnd, GWL_USERDATA, (LONG)create->lpCreateParams);
      break;

    case WM_SETFOCUS:
      if (disp == NULL)  /* no window yet? */
        goto _def_action;

      CreateCaret(disp->wnd, (HBITMAP)NULL,
                  max(2, GetSystemMetrics(SM_CXBORDER)),
                  disp->char_size.cy);
      real_caret_pos_x = disp->cursor_x * disp->char_size.cx;
      real_caret_pos_y = disp->cursor_y * disp->char_size.cy;
      SetCaretPos(real_caret_pos_x, real_caret_pos_y);
      if (disp->cursor_is_visible)
        ShowCaret(disp->wnd);
      disp->window_holds_focus = 1;
      break;

    case WM_KILLFOCUS:
      if (disp == NULL)  /* no window yet? */
        goto _def_action;

      DestroyCaret();
      disp->window_holds_focus = 0;
      break;

    case WM_PAINT:
      if (disp == NULL)  /* no window yet? */
        goto _def_action;

      dc = BeginPaint(disp->wnd, &ps);
      s_disp_on_paint(disp, dc, &ps.rcPaint);
      EndPaint(disp->wnd, &ps);
      break;

    case WM_MOVE:
      if (disp == NULL)  /* no window yet? */
        goto _def_action;

      if (GetWindowPlacement(disp->wnd, &wndpl))
      {
        disp->geom_param.pos_x = wndpl.rcNormalPosition.left;
        disp->geom_param.pos_y = wndpl.rcNormalPosition.top;
      }
      break;

    case WM_WINDOWPOSCHANGING:
      if (disp == NULL)  /* no window yet? */
        goto _def_action;

      win_pos = (WINDOWPOS *)lParam;
      if (win_pos->flags & SWP_NOSIZE)  /* no new size in this event? */
        return 0;

      if (IsIconic(disp->wnd))  /* event is _minimize_? */
        return 0;  /* not an interesting event */

      /*
      {cx,cy} must only hold client area for the computations
      */
      win_pos->cx -= disp->border_width * 2;
      win_pos->cy -= disp->caption_height + disp->border_width;

      /*
      Current cx/cy is _last_ width/height next time
      */
      if (disp->last_width == 0)
        disp->last_width = win_pos->cx;
      if (disp->last_height == 0)
        disp->last_height = win_pos->cy;
      if (disp->last_x == 0)
        disp->last_x = win_pos->x;
      if (disp->last_y == 0)
        disp->last_y = win_pos->y;
      resize_x = win_pos->cx - disp->last_width;
      resize_y = win_pos->cy - disp->last_height;
      move_x = win_pos->x - disp->last_x;
      move_y = win_pos->y - disp->last_y;
      disp->last_width = win_pos->cx;
      disp->last_height = win_pos->cy;
      disp->last_x = win_pos->x;
      disp->last_y = win_pos->y;

      /*
      Compute (align) the new size
      */
      if (IsZoomed(disp->wnd))  /* event is _maximize_? */
      {
        disp->geom_param.is_maximized = 1;
      }
      else
      {
        if (disp->geom_param.is_maximized)  /* restoring from maximized? */
          disp->geom_param.is_maximized = 0;  /* Old size must be fine */
        else
        {
          /*
          {cx, cy} width and height could only be set
          to character size aligned positions
          */
          win_pos->cx = s_disp_align_size(win_pos->cx, disp->char_size.cx);
          if (move_x != 0 && resize_x != 0)  /* resize left side? */
            win_pos->x += (disp->last_width - win_pos->cx);

          win_pos->cy = s_disp_align_size(win_pos->cy, disp->char_size.cy);
          if (move_y != 0 && resize_y != 0)  /* resize top side? */
            win_pos->y += (disp->last_height - win_pos->cy);
        }
      }

      /*
      Adjust back to include border and caption area
      */
      new_width = win_pos->cx / disp->char_size.cx;
      new_height = win_pos->cy / disp->char_size.cy;
      win_pos->cx += disp->border_width * 2;
      win_pos->cy += disp->caption_height + disp->border_width;

      /*
      Generate resize event if there is new size
      */
      if (   new_height != disp->geom_param.height
          || new_width != disp->geom_param.width)
      {
        disp_event_clear(&ev);
        ev.t.code = EVENT_RESIZE;
        ev.e.new_size.x1 = 0;
        ev.e.new_size.y1 = 0;
        if (disp->geom_param.is_maximized)
        {
          disp->geom_param.width_before_maximize = disp->geom_param.width;
          disp->geom_param.height_before_maximize = disp->geom_param.height;
        }
        disp->geom_param.height = new_height;
        disp->geom_param.width = new_width;
        ev.e.new_size.width = disp->geom_param.width;
        ev.e.new_size.height= disp->geom_param.height;
        if (!disp->paint_is_suspended)  /* we don't yet have charbuf? */
          s_disp_alloc_char_buf(disp);

        if (disp->handle_resize != NULL)
          disp->handle_resize(&ev, disp->handle_resize_ctx);
        else
          s_disp_ev_q_put(disp, &ev);
      }
      break;

    case WM_CHAR:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_SYSCHAR:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_DEADCHAR:
    case WM_SYSDEADCHAR:
      s_disp_on_key(disp, message, (unsigned)wParam, (unsigned)lParam);
      break;

    case WM_TIMER:
      disp_event_clear(&ev);
      ev.t.code = EVENT_TIMER_5SEC;
      s_disp_ev_q_put(disp, &ev);
      break;

    case WM_CLOSE:
      break;

    case WM_DESTROY:
      PostQuitMessage( 0 );
      break;

    case WM_NCDESTROY:
      if (disp == NULL)  /* no window yet? */
        break;

      disp->wnd = 0;
      // fallthrough

    default:
_def_action:
      return DefWindowProc(hWnd, message, wParam, lParam);
  }

  return 0;
}

/*!
@brief initial setup of display (win32 GUI)

Sets up font, DC and win32 instance

@param disp a dispc object
@returns true for success
@returns false for failure and error messages and code are set
*/
static int s_disp_init(dispc_t *disp)
{
  HDC dc;
  WNDCLASS wc;
  ATOM c;
  RECT rc;
  DWORD style;
  WINDOWPLACEMENT wndpl;
  int is_maximized2;

  /*
  Load font
  */
  disp->font =
    CreateFont(disp->geom_param.font_size, /* nHeight, height of font */
               0,            /* nWidth, average character width */
               0,            /* nEscapement, angle of escapement */
               0,            /* nOrientation, base-line orientation angle */
               400,          /* fnWeight, font weight */
               FALSE,        /* fdwItalic, italic attribute option */
               FALSE,        /* fdwUnderline, underline attribute option */
               FALSE,        /* fdwStrikeOut, strikeout attribute option */
               DEFAULT_CHARSET,      /* fdwCharSet, character set identifier */
               OUT_DEFAULT_PRECIS,   /* fdwOutputPrecision, output precision */
               CLIP_DEFAULT_PRECIS,  /* fdwClipPrecision, clipping precision */
               DEFAULT_QUALITY,      /* fdwQuality, output quality */
               FIXED_PITCH | FF_DONTCARE, /* fdwPitchAndFamily, pitch and family*/
               disp->geom_param.font_name);  /* lpszFace, typeface name */
  if (disp->font == NULL)
  {
    disp->code = DISP_FONT_FAILED_TO_LOAD;
    _snprintf(disp->error_msg, sizeof(disp->error_msg),
              "failed to load font %s", disp->geom_param.font_name);
    s_disp_translate_os_error(disp);
    return 0;
  }

  /*
  Get character size
  */
  dc = GetDC(NULL);
  SelectObject(dc, disp->font);
  GetTextExtentPoint32(dc, _T("H"), 1, &disp->char_size);
  ReleaseDC(NULL, dc);

  /*
  Register class
  */
  disp->window_class = _T("disp-wnd");
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = (WNDPROC)s_disp_wnd_proc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = (HINSTANCE)disp->geom_param.instance;
  wc.hIcon = NULL;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = NULL;
  wc.lpszMenuName = NULL;
  wc.lpszClassName = disp->window_class;

  c = RegisterClass(&wc);
  if (c == 0)
  {
    disp->code = DISP_FAILED_TO_OPEN_WINDOW;
    _snprintf(disp->error_msg, sizeof(disp->error_msg),
              "failed to open window");
    s_disp_translate_os_error(disp);
    return 0;
  }

  /*
  Init instance
  */
  is_maximized2 = disp->geom_param.is_maximized;  /* Destroyed by CreateWindow() */
  style = WS_OVERLAPPED  |
          WS_CAPTION     |
          WS_SYSMENU     |
          WS_SIZEBOX     |
          WS_MAXIMIZEBOX |
          WS_MINIMIZEBOX;
  rc.left = 0;
  rc.top = 0;
  rc.right = disp->geom_param.width * disp->char_size.cx;
  rc.bottom = disp->geom_param.height * disp->char_size.cy;

  AdjustWindowRect(&rc, style, FALSE);
  /* Now rc.left/top holds negative values,
  which are the widths of the border and the
  height of the caption bar */
  disp->caption_height = -rc.top;
  disp->border_width = -rc.left;

  disp->wnd =
    CreateWindow(disp->window_class,
                 "no title", style,
                 CW_USEDEFAULT, 0,
                 rc.right - rc.left, rc.bottom - rc.top,
                 NULL, NULL,
                 (HINSTANCE)disp->geom_param.instance,
                 disp);
  if (disp->wnd == 0)
  {
    disp->code = DISP_FAILED_TO_CREATE_WINDOW;
    _snprintf(disp->error_msg, sizeof(disp->error_msg),
              "failed to create window");
    s_disp_translate_os_error(disp);
    return 0;
  }

  /*
  Restore window placement and size
  */
  memset(&wndpl, 0, sizeof(wndpl));
  wndpl.length = sizeof(wndpl);
  wndpl.showCmd = SW_SHOW;
  wndpl.rcNormalPosition.left = disp->geom_param.pos_x;
  wndpl.rcNormalPosition.top = disp->geom_param.pos_y;
  wndpl.rcNormalPosition.right =
    disp->geom_param.pos_x + rc.right + (-rc.left);
  wndpl.rcNormalPosition.bottom =
    disp->geom_param.pos_y + rc.bottom + (-rc.top);
  if (!SetWindowPlacement(disp->wnd, &wndpl))
  {
    disp->code = DISP_FAILED_TO_POSITION_WINDOW;
    _snprintf(disp->error_msg, sizeof(disp->error_msg),
              "failed to position window");
    s_disp_translate_os_error(disp);
    return 0;
  }

  if (is_maximized2)
    ShowWindow(disp->wnd, SW_MAXIMIZE);
  else
    ShowWindow(disp->wnd, disp->geom_param.show_state);

  UpdateWindow(disp->wnd);

  /* set timer for 5 seconds */
  disp->win32_timer_id = SetTimer(disp->wnd, 1, 5000, NULL);
  if (disp->win32_timer_id == 0)
  {
    disp->code = DISP_FAILED_TIMER_SETUP;
    _snprintf(disp->error_msg, sizeof(disp->error_msg),
              "failed to setup a timer");
    s_disp_translate_os_error(disp);
    return 0;
  }

  return TRUE;
}

/*!
@brief platform specific disp cleanup (win32 GUI)

@param disp  a display object
*/
static void s_disp_done(dispc_t *disp)
{
  KillTimer(disp->wnd, disp->win32_timer_id);
}

/*!
@brief Sets the caret on a specific position (win32 GUI)

@param disp  a display object
@param x     coordinates in character units
@param y     coordinates in character units
*/
static void s_disp_set_cursor_pos(dispc_t *disp, int x, int y)
{
  int caret_x;
  int caret_y;

  ASSERT(x < disp->geom_param.width);
  ASSERT(y < disp->geom_param.height);

  caret_x = x * disp->char_size.cx;
  caret_y = y * disp->char_size.cy;

  if (disp->window_holds_focus)
    SetCaretPos(caret_x, caret_y);

  disp->cursor_x = x;
  disp->cursor_y = y;
}

/*!
@brief Changes the title of the window (win32 GUI)

@param disp    a dispc object
@param title   a string for the title
*/
static void s_disp_wnd_set_title(dispc_t *disp, const char *title)
{
  SetWindowText(disp->wnd, title );
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

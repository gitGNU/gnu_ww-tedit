/*

File: x_scr.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 3rd February, 2003
Descrition:
  Screen and cursor manipulation functions, implemented
  for X Windows

*/

#include "global.h"
#include "memory.h"
#include "scr.h"
#include "xscr_i.h"

#include <X11/Xft/Xft.h>

int ScreenPosX = -1;
int ScreenPosY = -1;
int ScreenHeight = 40;
int ScreenWidth = 80;
BOOLEAN bMaximized = FALSE;
char FontName[MAX_FONT_NAME_LEN] = "vera";
int FontSize = 11;
DWORD Font1Style = 0;
DWORD Font2Style = 0;
DWORD Font3Style = 0;
int scr_height_before_maximize = 0;
int scr_width_before_maximize = 0;

int CurHeight;
int CurWidth;

int nTop;
int nLeft;

extern int prog_argc;
extern char **prog_argv;

static TCharInfo *ScrBuf;
static int ScrBufHeight;  /* always >= ScreenHeight */
static int ScrBufWidth;  /* always >= ScreenWidth */

BOOLEAN bWaitFirstExpose;
Display *x11_disp;
int screen_num;
XSizeHints *size_hints;
XWMHints *wm_hints;
XClassHint *class_hints;
char *window_name;
XTextProperty windowName;
Window WinHndl;
GC gc;
XFontStruct *pFontInfo = NULL;
XftFont *pxft_font = NULL;
XftDraw *pdraw = NULL;
int CharHeight;  /* in pixels */
int CharWidth;  /* in pixels */
static int CharAscent;  /* in pixels */

#if 0
/* Current cursor position on the screen */
int CursorX;
int CursorY;
BOOLEAN bCursorVisible;
int LastCursorX;  /* store the last position where */
int LastCursorY;  /* the cursor was shown          */
#endif

static XColor Colors[16];

static struct
{
    int r, g, b;
} dcolors[] =
{
    {   0,   0,   0 },  //     black
    {   0,   0, 160 },  // darkBlue
    {   0, 160,   0 },  // darkGreen
    {   0, 160, 160 },  // darkCyan
    { 160,   0,   0 },  // darkRed
    { 160,   0, 160 },  // darkMagenta
    { 160, 160,   0 },  // darkYellow
    { 204, 204, 204 },  // paleGray
    { 160, 160, 160 },  // darkGray
    {   0,   0, 255 },  //     blue
    {   0, 255,   0 },  //     green
    {   0, 255, 255 },  //     cyan
    { 255,   0,   0 },  //     red
    { 255,   0, 255 },  //     magenta
    { 255, 255,   0 },  //     yellow
    { 255, 255, 255 },  //     white
};

struct disp_ctx disp_ctx_data;

/* ************************************************************************
   Function: SetColor
   Description:
*/
static void SetColor(int i)
{
  ASSERT(i >= 0);
  ASSERT(i <= 15);

  Colors[i].blue  = (dcolors[i].b << 8) | dcolors[i].b;
  Colors[i].green = (dcolors[i].g << 8) | dcolors[i].g;
  Colors[i].red   = (dcolors[i].r << 8) | dcolors[i].r;
  Colors[i].flags = DoRed | DoGreen | DoBlue;
}

static Colormap colormap;

/* ************************************************************************
   Function: disp_get_atom
   Description:
     Returns X atom for Display corresponding to atom_name
   TODO:
     Cache the result, repeated calls are a round trip to the server
*/
Atom
disp_get_atom(const struct disp_ctx *disp, const char *atom_name)
{
  return XInternAtom(disp->x11_disp, atom_name, FALSE);
}

/* ************************************************************************
   Function: disp_x11_init_colors
   Description:
*/
static int
disp_x11_init_colors(void)
{
  int i;

  colormap = XCreateColormap(x11_disp, WinHndl, DefaultVisual(x11_disp, screen_num), AllocNone);
  for (i = 0; i < 16; i++)
  {
    SetColor(i);
    if (XAllocColor(x11_disp, colormap, &Colors[i]) == 0)
    {
      TRACE1("failed allocating color %d\n", i);
      return FALSE;
    }
  }
  XSetWindowColormap(x11_disp, WinHndl, colormap);
  return TRUE;
}

/* ************************************************************************
   Function: EnableResize
   Description:
     Enables the input of resize events.
*/
void EnableResize(void)
{
}

/* ************************************************************************
   Function: DisableResize
   Description:
     Disables the input of resize events.
*/
void DisableResize(void)
{
}

/* ************************************************************************
   Function: GetScreenMetrix
   Description:
     Initializes ScreenHeight, ScreenWidth and
     CurHeight, CurWidth, hInput and hOutput
*/
void GetScreenMetrix(void)
{
}

/* ************************************************************************
   Function: CalcRectSz
   Description:
     Calculates the size of a region to be used in gettextblock or
     puttextblock
*/
int CalcRectSz(int w, int h)
{
  ASSERT((signed)w > 0);
  ASSERT((signed)h > 0);
  ASSERT(h <= ScreenHeight);

  return (sizeof(TCharInfo) * w * h);
}

/* ************************************************************************
   Function: disp_put_line_segment
   Description:
     A service function of disp_on_paint(), puts a segment of a line on the screen.
     Segment of a line is one part that has same color and background.
*/
static void
disp_put_line_segment(const struct disp_ctx *disp, int line_x, int line_y,
  int left_edge, int region_len,
  unsigned char co, unsigned char bg, unsigned char *segment_buf)
{
  int region_x;
  int region_y;
  XftColor color;

  line_x *= disp->char_width;
  line_y *= disp->char_height;

  if (pxft_font != NULL)
  {
    region_x = line_x + left_edge * disp->char_width;
    region_y = line_y;

    #if 0  /* I believe that XFillRect is faster! */
    color.pixel = 0xffff; //Colors[b].pixel;
    color.color.red = dcolors[bg].r << 8;
    color.color.green = dcolors[bg].g << 8;
    color.color.blue = dcolors[bg].b << 8;
    color.color.alpha = 0xffff;
    XftDrawRect(pdraw, &color,
      region_x, region_y, region * disp->char_width, disp->char_height);
    #endif
    XSetForeground(x11_disp, gc, Colors[bg].pixel);
    XSetBackground(x11_disp, gc, Colors[bg].pixel);
    XFillRectangle(x11_disp, WinHndl, gc, region_x, region_y,
      region_len * disp->char_width, disp->char_height);

    color.pixel = 0xffff; //Colors[c].pixel;
    color.color.red = dcolors[co].r << 8;
    color.color.green = dcolors[co].g << 8;
    color.color.blue = dcolors[co].b << 8;
    color.color.alpha = 0xffff;
    XftDrawString8(pdraw, &color, pxft_font,
      region_x, region_y + disp->char_ascent, segment_buf, region_len);
  }
  else
  {
    XSetForeground(x11_disp, gc, Colors[co].pixel);
    XSetBackground(x11_disp, gc, Colors[bg].pixel);
    XDrawImageString(x11_disp, WinHndl, gc,
      line_x + left_edge * disp->char_width, line_y + disp->char_ascent,
      (char *)segment_buf, region_len);
  }
}

/* ************************************************************************
   Function: disp_on_paint
   Description:
     Puts text on the screen from ScrBuf.
     This function is to be called from the event handling loop
     in response to an Expose event.
     x2:y2 are not inclusive!
*/
void disp_on_paint(const struct disp_ctx *disp, int x1, int y1, int x2, int y2)
{
  int y;
  TCharInfo *p;
  TCharInfo *d;
  int width;
  unsigned char buf[1024];
  char pad_buf[2096];
  int w;
  int i;
  unsigned int c;
  unsigned int b;
  unsigned int attr;
  int left_edge;
  int region_len;

  if (disp->wait_first_expose)  /* X server book prohibits output before */
    return;              /* the first Expose event is received    */

  width = x2 - x1;
  for (y = y1; y < y2; ++y)
  {
    p = ScrBuf + y * ScrBufWidth;
    i = 0;
    attr = (p + x1)->attr;
    left_edge = 0;

    while (left_edge < width)
    {
      d = p + x1 + left_edge;
      region_len = 0;
      do
      {
        buf[region_len] = d->ch;
        ++region_len;
        ++d;
      }
      while ( d->attr == attr
              && left_edge + region_len < width
              && region_len < sizeof(buf) - 1);

      buf[region_len] = '\0';
      //TRACE3(":%d->%d:%s\n", region_len, y, buf);

      c = attr & 0x0f;
      b = (attr >> 4) & 0x0f;
      attr = d->attr;

      disp_put_line_segment(disp, x1, y, left_edge, region_len, c, b, buf);

      /*
      Patch artifacts for the case the window is maximized
      (screen might be not a multiple of disp->char_height/width)
      */
      if (y == disp->top_window_height - 1)
      {
        /*
        Output one entire line with the color of the last line
        */
        memset(pad_buf, ' ', sizeof(pad_buf));
        w = min(region_len, sizeof(pad_buf) - 1);
        if (x2 == disp->top_window_width)
          ++w;  /* correct the bottom right corner */
        pad_buf[w] = '\0';
        disp_put_line_segment(disp, x1, y + 1, left_edge, w, c, b, pad_buf);
      }

      left_edge += region_len;
    }

    /*
    Patch artifacts for the case the window is maximized
    (screen might be not a multiple of disp->char_height/width)
    */
    if (x2 == disp->top_window_width)  /* did we change the last char on screen? */
    {
      /*
      Output one space with the color of the last character
      */
      disp_put_line_segment(disp, x1, y, left_edge, 1, c, b, " ");
    }
  }
}

/* ************************************************************************
   Function: puttextblock
   Description:
     Puts a text block.
     Invalidates the region (XClearArea). X Windows will send
     expose event and then _puttext() will copy from ScrBuf
     to screen. This way X Windows keeps tab of what is modified
     on the screen.
     note: x2, y2 are inclusive coordinates!
*/
void puttextblock(int x1, int y1, int x2, int y2, void *buf)
{
  int y;
  TCharInfo *p;
  TCharInfo *dest_ln;
  TCharInfo *d;
  int width;
  int i;
  int pixel_x;
  int pixel_y;
  BOOLEAN bRedrawCursor;
  int rstart;
  int rend;
  BOOLEAN b_update;

  pixel_x = x1 * CharWidth;
  pixel_y = y1 * CharHeight;
  p = buf;
  width = x2 - x1 + 1;
  bRedrawCursor = FALSE;
  for (y = y1; y <= y2; ++y)
  {
    dest_ln = ScrBuf + y * ScrBufWidth;  /* dest line */
    b_update = FALSE;
    rstart = -1;
    rend = -1;
    for (i = 0; i < width; ++i)
    {
      if (y == disp_ctx_data.cursor_y && (x1 + i) == disp_ctx_data.cursor_x)
        bRedrawCursor = TRUE;
      d = dest_ln + i + x1;
      if (! (p[i].ch == d->ch && p[i].attr == d->attr))
      {
        d->ch = p[i].ch;
        d->attr = p[i].attr;
        if (rstart == -1)  /* establish rstart */
          rstart = i + x1;
        rend = i + x1;
        b_update = TRUE;
      }
    }
    if (b_update)
      disp_on_paint(&disp_ctx_data, rstart, y, rend + 1, y + 1);
    p += width;  /* next line in buf */
    pixel_y += CharHeight;
  }

  if (bRedrawCursor)
    disp_draw_cursor(&disp_ctx_data);
}

/* ************************************************************************
   Function: gettextblock
   Description:
     Gets a text block from ScrBuf to be stored into buf
     note: x2, y2 are inclusive coordinates!
*/
void gettextblock(int x1, int y1, int x2, int y2, void *buf)
{
  int y;
  TCharInfo *p;
  TCharInfo *src;
  TCharInfo *d;
  int width;
  int i;

  p = buf;
  width = x2 - x1 + 1;
  for (y = y1; y <= y2; ++y)
  {
    src = ScrBuf + y * ScrBufWidth;  /* src line */
    for (i = 0; i < width; ++i)
    {
      d = src + i + x1;
      p[i].ch = d->ch;
      p[i].attr = d->attr;
    }
    p += width;
  }
}

/* ************************************************************************
   Function: WriteXY
   Description:
     Displays a string at a specific screen position
*/
void WriteXY(const char *s, int x, int y, BYTE attr)
{
  TCharInfo buf[255];
  const char *p;
  TCharInfo *b;

  ASSERT(s != NULL);
  ASSERT(x + strlen(s) <= (unsigned)ScreenWidth);
  ASSERT(y <= ScreenHeight);

  for (p = s, b = buf; *p; ++p, ++b)
  {
    b->ch = *p;
    b->attr = attr;
  }

  puttextblock(x, y, x + (p - s - 1), y, buf);
}

/* ************************************************************************
   Function: PutChar
   Description:
*/
void _PutChar(TCharInfo *p, char c)
{
  p->ch = c;
}

/* ************************************************************************
   Function: PutAttr
   Description:
*/
void _PutAttr(TCharInfo *p, WORD attr)
{
  p->attr = attr;
}

/* ************************************************************************
   Function: FlexWriteXY
   Description:
     Writes a sting at position x and y. Attributes are alternated
     between attr1 and attr2 when '~' occures
*/
void FlexWriteXY(const char *s, int x, int y, BYTE attr1, BYTE attr2)
{
  char *c;
  TCharInfo *d;
  TCharInfo buf[255];
  BYTE attr;
  int ctilds = 0;

  ASSERT(s != NULL);
  ASSERT(y <= ScreenHeight);
  ASSERT(ScreenWidth < _countof(buf));

  attr = attr1;
  d = buf;
  for (c = (char *)s; *c; ++c)
  {
    if (*c == '~')
    {
      attr = (attr == attr1) ? attr2 : attr1;
      ++ctilds;
      continue;
    }
    d->ch = *c;
    d->attr = attr;
    ++d;
  }

  puttextblock(x, y, x + (c - s) - 1 - ctilds, y, buf);
}

/* ************************************************************************
   Function: FillXY
   Description:
     Fills an area on the screen with a specific character
*/
void FillXY(char c, BYTE attr, int x, int y, int count)
{
  int i;
  TCharInfo buf[255];

  ASSERT(count > 0);
  ASSERT(x + count <= ScreenWidth);
  ASSERT(y <= ScreenHeight);
  ASSERT(ScreenWidth < _countof(buf));

  for (i = 0; i < count; ++i)
  {
    buf[i].ch = c;
    buf[i].attr = attr;
  }

  puttextblock(x, y, x + count - 1, y, buf);
}

/* ************************************************************************
   Function: disp_draw_cursor
   Description:
     Draws a cursor on the screen.
*/
void
disp_draw_cursor(struct disp_ctx *disp)
{
  TCharInfo p;
  unsigned char ch;
  int c;  /* color */
  int b;  /* background */
  BYTE attr;

  if (disp->wait_first_expose)
    return;

  /* Remove the cursor from the previous pos (restore original text) */
  disp_on_paint(disp,
    disp->last_cursor_x, disp->last_cursor_y,
    disp->last_cursor_x + 1, disp->last_cursor_y + 1);

  /* Get character and attributes under the cursor */
  gettextblock(disp->cursor_x, disp->cursor_y,
    disp->cursor_x, disp->cursor_y, &p);
  ch = p.ch;
  attr = p.attr;

  if (disp->cursor_visible)
    attr = (attr ^ 0x77);  /* This is the cursor */
  c = attr & 0x0f;
  b = attr >> 4;

  if (disp->cursor_visible)
    disp_put_line_segment(disp, disp->cursor_x, disp->cursor_y, 0, 1, c, b, &ch);

  disp->last_cursor_x = disp->cursor_x;
  disp->last_cursor_y = disp->cursor_y;
}

/* ************************************************************************
   Function: HideCursor
   Description:
     Hides the cursor
*/
void HideCursor(void)
{
  disp_ctx_data.cursor_visible = FALSE;
  disp_draw_cursor(&disp_ctx_data);
}

/* ************************************************************************
   Function: GetCursorParam
   Description:
     Gets information about the cursor's size and visibility state
*/
void GetCursorParam(CursorParam *pInfo)
{
  pInfo->x = disp_ctx_data.cursor_x;
  pInfo->y = disp_ctx_data.cursor_y;
  pInfo->bVisible = disp_ctx_data.cursor_visible;
}

/* ************************************************************************
   Function: RestoreCursor
   Description:
     Restores what GetCursorParam have returned early
*/
void RestoreCursor(CursorParam *pInfo)
{
  disp_ctx_data.cursor_x = pInfo->x;
  disp_ctx_data.cursor_y = pInfo->y;
  disp_ctx_data.cursor_visible = pInfo->bVisible;
  disp_draw_cursor(&disp_ctx_data);
}

/* ************************************************************************
   Function: GotoXY
   Description:
     Sets new cursor position on the screen
*/
void GotoXY(int x, int y)
{
  disp_ctx_data.cursor_x = x;
  disp_ctx_data.cursor_y = y;
  disp_draw_cursor(&disp_ctx_data);
}

/* ************************************************************************
   Function: GetCursorXY
   Description:
*/
void GetCursorXY(int *x, int *y)
{
  *x = disp_ctx_data.cursor_x;
  *y = disp_ctx_data.cursor_y;
}

/* ************************************************************************
   Function: MapPalette
   Description:
*/
BOOLEAN MapPalette(BYTE *pPalette, int nEntries)
{
  return TRUE;
}

/* ************************************************************************
   Function: LoadFont
   Description:
*/
static BOOLEAN LoadFont(void)
{
  char *fontname;

  fontname = "9x15";
  //fontname = "8x13";
  pFontInfo = XLoadQueryFont(x11_disp, fontname);
  if (pFontInfo == NULL)
  {
    TRACE0("LoadFont: XLoadQueryFont() failed\n");
    return FALSE;
  }
  CharHeight = pFontInfo->max_bounds.ascent + pFontInfo->max_bounds.descent;
  CharWidth = pFontInfo->max_bounds.width;
  CharAscent = pFontInfo->max_bounds.ascent;
  return TRUE;
}

/* ************************************************************************
   function: disp_load_xft_font
   description:
*/
static BOOLEAN disp_load_xft_font(void)
{
  XftPattern *pat, *match;
  XftResult result;
  double face_size;

  face_size = FontSize;
  pat = XftNameParse(FontName);
  XftPatternBuild(pat,
		  XFT_FAMILY, XftTypeString, "mono",
		  XFT_SIZE, XftTypeDouble, face_size,
		  XFT_SPACING, XftTypeInteger, XFT_MONO,
		  (void *) 0);
  match = XftFontMatch(x11_disp, DefaultScreen(x11_disp), pat, &result);
  pxft_font = XftFontOpenPattern(x11_disp, match);
  XftPatternDestroy(match);
  if (pxft_font == NULL)
  {
    TRACE0("disp_load_xft_font: XftFontOpenName() failed\n");
    return FALSE;
  }
  CharWidth = pxft_font->max_advance_width;
  CharHeight = max(pxft_font->height, pxft_font->ascent + pxft_font->descent);
  CharAscent = pxft_font->ascent;
  return TRUE;
}

/* ************************************************************************
   Function: GetGC
   Description:
*/
static BOOLEAN GetGC(void)
{
  int line_width;
  int line_style;
  int cap_style;
  int join_style;
  int dash_offset;
  static char dash_list[] = {12, 24};
  int list_length;
  int valuemask;

  valuemask = 0;
  line_width = 6;
  line_style = LineOnOffDash;
  cap_style = CapRound;
  join_style = JoinRound;
  dash_offset = 0;
  list_length = 2;

  gc = XCreateGC(x11_disp, WinHndl, 0, NULL);
  if (pxft_font == NULL)
    XSetFont(x11_disp, gc, pFontInfo->fid);
  XSetForeground(x11_disp, gc, BlackPixel(x11_disp, screen_num));
  XSetBackground(x11_disp, gc, WhitePixel(x11_disp, screen_num));
  XSetLineAttributes(x11_disp, gc, line_width, line_style, cap_style,
    join_style);
  XSetDashes(x11_disp, gc, dash_offset, dash_list, list_length);
  return TRUE;
}

static BOOLEAN disp_get_draw(void)
{
  pdraw = XftDrawCreate(x11_disp, (Drawable) WinHndl,
    DefaultVisual(x11_disp, screen_num), DefaultColormap(x11_disp, screen_num));
  if (pdraw == NULL)
    return FALSE;
  else
    return TRUE;
}

/* ************************************************************************
   Function: disp_realloc_buf
   Description:
*/
void disp_realloc_buf(void)
{
  int scr_size;

  if (ScrBufHeight < ScreenHeight ||
      ScrBufWidth < ScreenWidth)
  {
    s_free(ScrBuf);
    ScrBufHeight = ScreenHeight;
    ScrBufWidth = ScreenWidth;
    scr_size = ScrBufHeight * ScrBufWidth * sizeof(TCharInfo);
    ScrBuf = alloc(scr_size);
    if (ScrBuf == NULL)
    {
      TRACE1("disp_realloc_buf: no memory for the initial buffer of %d bytes\n", scr_size);
      return /*FALSE*/;
    }
    memset(ScrBuf, 0, scr_size);
  }
}

/* ************************************************************************
   Function: disp_restore_maximized_state
   Description:
*/
static void
disp_restore_maximized_state(struct disp_ctx *disp, BOOLEAN bMaximized)
{
  Atom atoms[7];
  int num_atoms;

  num_atoms = 0;

  if (bMaximized)
  {
    atoms[num_atoms] = disp_get_atom(disp, "_NET_WM_STATE_MAXIMIZED_VERT");
    ++num_atoms;
    atoms[num_atoms] = disp_get_atom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ");
    ++num_atoms;
    XChangeProperty(x11_disp, WinHndl,
      disp_get_atom(&disp_ctx_data, "_NET_WM_STATE"),
      XA_ATOM, 32, PropModeReplace, (unsigned char*) atoms, num_atoms);
  }
  else
  {
    XDeleteProperty(x11_disp, WinHndl,
      disp_get_atom(&disp_ctx_data, "_NET_WM_STATE"));
  }
}

/* ************************************************************************
   Function: OpenConsole
   Description:
*/
BOOLEAN OpenConsole(void)
{
  BOOLEAN result;
  Window parent_win;
  int scr_size;
  int width;
  int height;
  int x;
  int y;
  int border_width;

  size_hints = NULL;
  wm_hints = NULL;
  class_hints = NULL;
  x11_disp = NULL;

  result = TRUE;

  ScrBufHeight = 120;
  ScrBufWidth = 120;
  scr_size = ScrBufHeight * ScrBufWidth * sizeof(TCharInfo);
  ScrBuf = alloc(scr_size);
  if (ScrBuf == NULL)
  {
    TRACE1("OpenConsole: no memory for the initial buffer of %d bytes\n", scr_size);
    return FALSE;
  }
  memset(ScrBuf, 0, scr_size);

  size_hints = XAllocSizeHints();
  wm_hints = XAllocWMHints();
  class_hints = XAllocClassHint();

  if (size_hints == NULL ||
    wm_hints == NULL ||
    class_hints == NULL)
  {
    TRACE0("OpenConsole: basic allocations failed\n");
    goto _cleanup;
  }

  x11_disp = XOpenDisplay(NULL);
  if (x11_disp == NULL)
  {
    TRACE0("OpenConsole: XOpenDisplay() failed\n");
    goto _cleanup;
  }

  /* we need the font to determine the window size */
  //if (!LoadFont())
  //  goto _cleanup;
  if (!disp_load_xft_font())
    goto _cleanup;

  screen_num = DefaultScreen(x11_disp);

  CurWidth = ScreenWidth - 1;
  CurHeight = ScreenHeight - 1;
  width = CharWidth * ScreenWidth;
  height = CharHeight * ScreenHeight;
  border_width = 4;
  parent_win = RootWindow(x11_disp, screen_num);
  x = ScreenPosX;
  y = ScreenPosY;

  WinHndl = XCreateWindow(x11_disp, parent_win,
    x, y, width, height,
    border_width,
    CopyFromParent, InputOutput, CopyFromParent,
    0, 0);

  /* bkg set to None will allow XClearArea() to invalidate region */
  XSetWindowBackground(x11_disp, WinHndl, None);

  size_hints->flags = PMinSize | PResizeInc;
  size_hints->min_width = CharWidth * 60;
  size_hints->min_height = CharHeight * 25;
  size_hints->width_inc = CharWidth;
  size_hints->height_inc = CharHeight;

  if (XStringListToTextProperty(&window_name, 1, &windowName) == 0)
  {
    TRACE0("OpenConsole: structure allocation for windowName failed\n");
    goto _cleanup;
  }

  wm_hints->initial_state = NormalState;
  wm_hints->input = True;
  wm_hints->icon_pixmap = 0;
  wm_hints->flags = StateHint | InputHint;

  class_hints->res_name = "ww editor";
  class_hints->res_class = "Basicwin";

  XSetWMProperties(x11_disp, WinHndl, &windowName, 0, prog_argv, prog_argc,
    size_hints, wm_hints, class_hints);

  XSelectInput(x11_disp, WinHndl, ExposureMask | KeyPressMask |
    ButtonPressMask | StructureNotifyMask | PropertyChangeMask);

  disp_x11_init_colors();
  if (!GetGC())
    goto _cleanup;
  if (!disp_get_draw())
    goto _cleanup;

  memset(&disp_ctx_data, 0, sizeof(struct disp_ctx));
  disp_ctx_data.x11_disp = x11_disp;
  disp_ctx_data.h_win = WinHndl;
  disp_ctx_data.screen_num = screen_num;
  disp_ctx_data.h_root_win = parent_win;
  disp_ctx_data.char_height = CharHeight;
  disp_ctx_data.char_width = CharWidth;
  disp_ctx_data.char_ascent = CharAscent;
  disp_ctx_data.wait_first_expose = TRUE;
  disp_ctx_data.cursor_visible = TRUE;
  disp_ctx_data.top_window_height = ScreenHeight;
  disp_ctx_data.top_window_width = ScreenWidth;

  disp_restore_maximized_state(&disp_ctx_data, bMaximized);

  XMapWindow(x11_disp, WinHndl);

  bWaitFirstExpose = TRUE;

  return TRUE;

_cleanup:
  s_free(ScrBuf);
  if (x11_disp != NULL)
    XCloseDisplay(x11_disp);
  return FALSE;
}

/* ************************************************************************
   Function: CloseConsole
   Description:
*/
BOOLEAN CloseConsole(void)
{
  bWaitFirstExpose = FALSE;
  XCloseDisplay(x11_disp);
  s_free(ScrBuf);
  return TRUE;
}

/* ************************************************************************
   Function: SetTitle
   Description: Sets the window title.
*/
void SetTitle ( const char * sTitle )
{
  Atom atom_wm_name;

  atom_wm_name = XInternAtom(x11_disp, "WM_NAME", FALSE);
  XChangeProperty(x11_disp, WinHndl, atom_wm_name,
    XA_STRING, 8, PropModeReplace, sTitle, strlen(sTitle) + 1);
}

/* ************************************************************************
   Function: disp_set_resize_handler
   Description:
*/
void disp_set_resize_handler(void (*handle_resize)(struct event *ev))
{
}

struct disp_ctx *scr_get_disp_ctx(void)
{
  return &disp_ctx_data;
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2003
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


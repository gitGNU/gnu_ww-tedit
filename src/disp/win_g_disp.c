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

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include "disp.h"
#include "p_win_g.h"

static char disp_id[] = "disp-lib";

#ifdef WIN32
#define snprintf _snprintf
#endif

/*!
@brief Returns the size of the dispc (display) object

This function should be used for the caller of disp_init() to allocate memory
for the display object.
*/
int disp_obj_size(void)
{
  return sizeof(dispc_t);
}

#ifdef _DEBUG
/*!
@brief Checks if *disp points to a valid dispc object.

@param disp a display object
*/
int disp_is_valid(const dispc_t *disp)
{
  if (disp == NULL)
    return 0;
  return (disp->string_id == disp_id);
}
#endif

/*!
@brief Initializes a dispc object

Object memory must be allocated by the caller. Object size must be
obtained by a call to disp_obj_size()

@param wnd_param parameters for the display window
@param disp_obj  points to the memory where the display object will be
                 constructed
@returns false failure to init
@returns true  init ok
*/
int disp_init(const disp_wnd_param_t *wnd_param, void *disp_obj)
{
  dispc_t *disp;
  int r;

  disp = disp_obj;
  memset(disp, 0, sizeof(*disp));

  #ifdef _DEBUG
  disp->string_id = disp_id;
  #endif

  disp_wnd_set_param(disp, wnd_param);
  disp->paint_is_suspended = 1;
  r = _disp_init(disp);

  if (r)
  {
    _disp_set_cursor_pos(disp, 0, 0);
    _disp_show_cursor(disp, 1);
  }

  return r;
}

/*!
@brief Gets the code and the text of the last error

@param disp      a dispc object
@param code      receives the error code
@param error     receives the disp error message
@param os_error  receives the OS formated error
*/
void disp_error_get(dispc_t *disp, enum disp_error *code,
                    char **error, char **os_error)
{
  ASSERT(VALID_DISP(disp));

  *code = disp->code;
  *error = disp->error_msg;
  *os_error = disp->os_error_msg;
}

/*!
@brief Clears the error

@param disp  a dispc object
*/
void disp_error_clear(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  disp->code = DISP_NO_ERROR;
  disp->error_msg[0] = '\0';
  disp->os_error_msg[0] = '\0';
}

/*!
@brief Frees the display buffer

To be called upon shutdown of the disp object

@param disp a dispc object
*/
static void _disp_free_char_buf(dispc_t *disp)
{
  void (*my_free)(void *buf);

  my_free = disp->safe_free;
  if (my_free == NULL)  /* no user established mem handlers? */
  {
    /* then use the libc's malloc() and free() */
    my_free = &free;
  }

  if (disp->char_buf != NULL)
    my_free(disp->char_buf);
}

/*!
@brief Shuts down the dispc object

Disposes all memory that was allocated during the life of the object.

@param disp a dispc object
*/
void disp_done(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  _disp_ev_q_done(disp);
  _disp_free_char_buf(disp);
  _disp_done(disp);

  #ifdef _DEBUG
  memset(disp, -1, sizeof(dispc_t));
  #endif
}

/*!
@brief allocates memory for the character buffer if necessary

disp->char_buf can't be allocated at disp_init() time because we don't know
if disp_set_safemem_proc() will not be called after that to establish custom
memory handlers.

This function is called by evey function that interacts with char_buf and it
either establishes char_buf for the first time or it reallocates if new window
parameters exceed the capacity of char_buf

@param disp a display object
*/
static void _disp_alloc_char_buf(dispc_t *disp)
{
  int required_size;
  int char_buf_size;
  void *(*my_malloc)(size_t size);
  void (*my_free)(void *buf);

  required_size = disp->geom_param.height * disp->geom_param.width;
  char_buf_size = disp->buf_height * disp->buf_width;

  ASSERT(required_size > 0);

  if (char_buf_size >= required_size)
    return;
  else
  {
    my_malloc = disp->safe_malloc;
    my_free = disp->safe_free;
    if (my_malloc == NULL)  /* no user established mem handlers? */
    {
      /* then use the libc's malloc() and free() */
      my_malloc = &malloc;
      my_free = &free;
    }

    if (disp->char_buf != NULL)
      my_free(disp->char_buf);

    disp->char_buf = my_malloc(required_size * sizeof(disp_char_t));
    ASSERT(disp->char_buf != NULL);  /* malloc should've a the safety buffer */

    disp->buf_height = disp->geom_param.height;
    disp->buf_width = disp->geom_param.width;
  }
}

/*!
@brief Replaces memory manager functions.

If none are specified the disp module will use standard malloc and free.
The functions that replace malloc and free must have safety pool, the display
module can't handle mem failures gracefully.

Call this function immediately after disp_init().

@param disp         a display object
@param safe_malloc  a function to replace malloc
@param safe_free    a function to replace free
*/
void disp_set_safemem_proc(dispc_t *disp,
                        void *(*safe_malloc)(size_t size),
                        void (*safe_free)(void *buf))
{
  ASSERT(VALID_DISP(disp));
  ASSERT(disp->char_buf == NULL);  /* only before the first mem use */

  ASSERT(safe_malloc != NULL);
  ASSERT(safe_free != NULL);

  disp->safe_malloc = safe_malloc;
  disp->safe_free = safe_free;

  _disp_alloc_char_buf(disp);
}

static disp_wnd_param_t default_wnd_param;  /* forward declaration */

/*!
@brief Changes windows parameters like position, size and fonts.

@param disp       a display object
@param wnd_param  description of the desired window geometry
*/
void disp_wnd_set_param(dispc_t *disp, const disp_wnd_param_t *wnd_param)
{
  ASSERT(VALID_DISP(disp));

  if (wnd_param->set_defaults)
  {
    memcpy(&disp->geom_param, &default_wnd_param, sizeof(disp_wnd_param_t));
    disp->geom_param.instance = wnd_param->instance;
    disp->geom_param.show_state = wnd_param->show_state;
  }
  else
    memcpy(&disp->geom_param, wnd_param, sizeof(disp_wnd_param_t));
}

/*!
@brief Returns description of the current window geometry

@param disp       a display object
@param wnd_param  descrition of the window geometry
*/
void disp_wnd_get_param(const dispc_t *disp, disp_wnd_param_t *wnd_param)
{
  ASSERT(VALID_DISP(disp));

  memcpy(wnd_param, &disp->geom_param, sizeof(disp->geom_param));
}

/*!
@brief Returns the width of the window

@param disp a display object
*/
int disp_wnd_get_width(const dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  return disp->geom_param.width;
}

/*!
@brief Returns the height of the window

@param disp a display object
*/
int disp_wnd_get_height(const dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  return disp->geom_param.height;
}

/*!
@brief Sets the resize event handler call-back function

On Windows GUI platform the resizing is not an event that can be returned by
disp_event_read() but must be handled in the event loop. The even loop will
call this call-back for it to redraw the display according to the new
geometry.

@param disp               a display object
@param handle_resize      a call-back function that redraws the display
@param handle_resize_ctx  pointer to context that is passed to the call-back
*/
void disp_set_resize_handler(dispc_t *disp,
                             void (*handle_resize)(struct disp_event *ev,
                                                   void *ctx),
                             void *handle_resize_ctx)
{
  ASSERT(VALID_DISP(disp));
  disp->handle_resize = handle_resize;
  disp->handle_resize_ctx = handle_resize_ctx;
}

#ifdef _DEBUG
/*!
@brief Checks if *cbuf points to a valid disp_char_buf object.

@param cbuf buffer

@returns true  cbuf points to an object of type disp_char_buf_t
@returns false cbuf points to memory that is no an object of
               type disp_char_buf_t
*/
int _disp_cbuf_is_valid(const disp_char_buf_t *cbuf)
{
  if (cbuf == NULL)
    return 0;
  return (cbuf->magic_byte == DISP_CHAR_BUF_MAGIC);
}
#endif

/*!
@brief Calulates how much memory is needed to accomodate text data

@param num_chars the size of the text data

@returns the size in bytes of memory needed for a disp_char_buf_t buffer
         to accomodate text data
*/
static int _disp_cbuf_calc_size(int num_chars)
{
  ASSERT(num_chars > 0);
  return sizeof(disp_char_buf_t) + num_chars * sizeof(disp_char_t);
}

/*!
@brief Initializes cbuf object.

disp_char_buf_t is the data carrier for functions that put text on
the screen. The buffer knows its size to prevent overflows. This function
initializes one such buffer -- stores the size and resets the data area.

@param disp a dispc object (unused)
@param cbuf pointer to a buffer of type disp_char_buf_t
@param size how many characters are to be stored in this buffer
*/
void disp_cbuf_reset(const dispc_t *disp, disp_char_buf_t *cbuf, int max_characters)
{
  #ifdef _DEBUG
  cbuf->magic_byte = DISP_CHAR_BUF_MAGIC;
  #endif

  memset(cbuf, _disp_cbuf_calc_size(max_characters), 0);
  cbuf->max_characters = max_characters;
}

/*!
@brief Makes a cbuf buffer become invalid

For debug purposes before memory that is freed it must be invalidated by
a call to this function.

@param disp a dispc object
@param cbuf pointer to a buffer of type disp_char_buf_t
*/
void disp_cbuf_mark_invalid(const dispc_t *disp, disp_char_buf_t *cbuf)
{
  ASSERT(VALID_DISP_CHAR_BUF(cbuf));
  memset(cbuf, _disp_cbuf_calc_size(cbuf->max_characters), 0);
}

/*!
@brief Puts a character at a specific position in dest_buf

The structure of dest_buf is not visible outside the disp module. This
function gives write access to disp_char_but_t data.

The dest_buf can be displayed on screen by disp_put_block().

@param disp      a display object
@param dest_buf  output here
@param index     position
@param c         data
*/
void disp_cbuf_put_char(const dispc_t *disp,
                        disp_char_buf_t *dest_buf, int index, char c)
{
  ASSERT(VALID_DISP_CHAR_BUF(dest_buf));
  ASSERT(index < dest_buf->max_characters);

  dest_buf->cbuf[index].c = c;
}

/*!
@brief Changes the attributes at a specific position in dest_buf

The structure of dest_buf is not visible outside the disp module, this is
function gives write access to disp_char_but_t data.

@param disp      a display object
@param dest_buf  output here
@param index     position
@param attr      data
*/
void disp_cbuf_put_attr(const dispc_t *disp,
                        disp_char_buf_t *dest_buf, int index, int attr)
{
  ASSERT(VALID_DISP_CHAR_BUF(dest_buf));
  ASSERT(index < dest_buf->max_characters);

  dest_buf->cbuf[index].a = attr;
}

/*!
@brief Changes the attributes at a specific position in dest_buf

The structure of dest_buf is not visible outside the disp module, this is
function gives write access to disp_char_but_t data.

@param disp      a display object
@param dest_buf  output here
@param index     position
@param c         data
@param attr      data
*/
void disp_cbuf_put_char_attr(const dispc_t *disp,
                             disp_char_buf_t *dest_buf,
                             int index, int c, int attr)
{
  ASSERT(VALID_DISP_CHAR_BUF(dest_buf));
  ASSERT(index < dest_buf->max_characters);

  dest_buf->cbuf[index].c = c;
  dest_buf->cbuf[index].a = attr;
}

/*!
@brief Calculates how much memory is needed for a ractangle of text

@param disp   a dispc object
@param width  rectangle geometry
@param height rectangle geometry

@returns the size in bytes of memory needed for a disp_char_buf_t buffer
         to accomodate rectangle text data
*/
int disp_calc_rect_size(const dispc_t *disp, int width, int height)
{
  return _disp_cbuf_calc_size(width * height);
}

/*!
@brief Calculates the position of screen position x,y in disp->char_buf

@param disp  a dispc object
@param x     window position coordonates
@param y     window position coordonates
*/
static disp_char_t* _disp_buf_access(dispc_t *disp, int x, int y)
{
  _disp_alloc_char_buf(disp);
  return &disp->char_buf[y * disp->buf_width + x];
}

/*!
@brief forms a RECT based on rectangle geometry

@param disp a dispc object
@param r  the output RECT
@param x  upper left corner coordinates of the destination rectangle
@param y  upper left corner coordinates of the destination rectangle
@param w  rectangle geometry
@param h  rectangle geometry
*/
static void _disp_calc_area(const dispc_t *disp,
                            RECT *r, int x, int y, int w, int h)
{
  r->left = x * disp->char_size.cx;
  r->right = r->left + w * disp->char_size.cx;
  r->top = y * disp->char_size.cy;
  r->bottom = r->top + h * disp->char_size.cy;
}

/*!
@brief Marks area of the screen as invalid. (win32 GUI)

This will make Windows generate WM_PAINT event for this area and
the data will then be moved from disp->char_buf to the display

@param disp a dispc object
@param r    the RECT
*/
static void _disp_invalidate_rect(const dispc_t *disp, RECT *r)
{
  int window_right;
  int window_bottom;

  window_right = disp->geom_param.width * disp->char_size.cx;
  window_bottom = disp->geom_param.height * disp->char_size.cy;
  if (r->right == window_right);
    r->right += disp->char_size.cx; /* for the artefact @ maximize */
  if (r->bottom == window_bottom);
    r->bottom += disp->char_size.cy;
  InvalidateRect(disp->wnd, r, FALSE);
  /*debug_trace("disp_invalidate_rect: %d, %d to %d, %d\n",
              r->left, r->top, r->right, r->bottom);*/
}

/*!
@brief Marks that area of the window has been changed.

Call to this function will eventually make the text of the area
appear on the output window.

@param disp  a dispc object
@param x    upper left corner coordinates of the destination rectangle
@param y    upper left corner coordinates of the destination rectangle
@param w    rectangle geometry
@param h    rectangle geometry
*/
static void _disp_validate_rect(dispc_t *disp,
                                int x, int y,
                                int w, int h)
{
  RECT area;

  ASSERT(w > 0);
  ASSERT(h > 0);
  ASSERT(x >= 0);
  ASSERT(y >= 0);

  disp->paint_is_suspended = 0;
  _disp_calc_area(disp, &area, x, y, w, h);
  _disp_invalidate_rect(disp, &area);
}

#ifdef _DEBUG
static void disp_is_inside_disp_buf(dispc_t *disp, disp_char_t *dest, int width)
{
  disp_char_t *last_buf_pos;  /* end point of disp->char_buf */
  disp_char_t *last_dest_pos;  /* somewhere inside disp->char_buf */

  ASSERT(disp->char_buf != NULL);
  last_buf_pos = disp->char_buf + disp->buf_height * disp->buf_width;
  last_dest_pos = dest + width;
  ASSERT(last_dest_pos <= last_buf_pos);
}
#endif

/*!
@brief Puts a rectangle of text on the screen

The function verifies the changes against the content of the screen
and will only update the subset of text that is different

@param disp a dispc object
@param x    upper left corner coordinates of the destination rectangle
@param y    upper left corner coordinates of the destination rectangle
@param w    rectangle geometry
@param h    rectangle geometry
@param buf  buffer that holds the text data that is to be displayed
*/
void disp_put_block(dispc_t *disp,
                    int x, int y, int w, int h, const disp_char_buf_t *buf)
{
  disp_char_t *char_ln;
  disp_char_t *d;
  const disp_char_t *s;
  int ln;
  int range_start;
  int range_end;
  int range_num_chars;
  int i;

  ASSERT(VALID_DISP(disp));
  ASSERT(VALID_DISP_CHAR_BUF(buf));

  for (ln = 0; ln < h; ++ln)
  {
    char_ln = _disp_buf_access(disp, x, y + ln);

    ASSERT((ln * w + w) < buf->max_characters);  /* valid source */
    disp_is_inside_disp_buf(disp, char_ln, w);  /* valid dest */

    /* Find the range of the line that has changed against what is on screen */
    range_start = -1;
    range_end = -1;
    for (i = 0; i < w; ++i)
    {
      s = &buf->cbuf[ln * w + i];
      d = char_ln + i;
      if (!disp_char_equal(*s, *d))
      {
        if (range_start == -1)
          range_start = i;
        range_end = i;
      }
    }
    range_num_chars = 0;
    if (range_start != -1)
      range_num_chars = range_end - range_start + 1;

    /* Copy the range of changes from buf onto the screen line */
    if (range_num_chars > 0)
    {
      s = &buf->cbuf[ln * w + range_start];
      memcpy(char_ln + range_start, s, range_num_chars * sizeof(disp_char_t));
      /*debug_trace("put_block %d:%d len %d (-%d)\n", x, y + ln, w,
                  w - range_num_chars);*/

      /* Invalidate only the range that has changed */
      _disp_validate_rect(disp, x + range_start, y, range_num_chars, h);
    }
  }
}

/*!
@brief Gets the text from a rectangle window area

@param disp a dispc object
@param x    upper left corner coordinates of the destination rectangle
@param y    upper left corner coordinates of the destination rectangle
@param w    rectangle geometry
@param h    rectangle geometry
@param buf  buffer that receives the text data
*/
void disp_get_block(const dispc_t *disp,
                    int x, int y, int w, int h, disp_char_buf_t *buf)
{
  disp_char_t *char_ln;
  int ln;

  ASSERT(VALID_DISP(disp));
  ASSERT(VALID_DISP_CHAR_BUF(buf));
  ASSERT(!disp->paint_is_suspended);

  for (ln = 0; ln < h; ++ln)
  {
    char_ln = _disp_buf_access((void *)disp, x, y + ln);
    ASSERT((ln * w + w) < buf->max_characters);
    memcpy(&buf->cbuf[ln * w], char_ln, w * sizeof(disp_char_t));
    /*debug_trace("get_block %d:%d len %d bytes: %d\n",
      x, y + ln, w, (ln * w + w) * sizeof(disp_char_t));*/
  }
}

/*!
@brief Displays a string at a specific screen position

Function verifies if screen content changes and will not emit
refresh call to the OS if nothing changed.

@param disp  a display object
@param s     string to display
@param x     coordinates
@param y     coordinates
@param attr  attributes
*/
void disp_write(dispc_t *disp,
                const char *s, int x, int y, int attr)
{
  int len;
  int i;
  disp_char_t *char_ln;
  disp_char_t *d;
  disp_char_t a;
  int range_start;
  int range_end;
  int range_num_chars;

  ASSERT(VALID_DISP(disp));

  len = strlen(s);
  if (len == 0)
    return;

  char_ln = _disp_buf_access(disp, x, y);

  disp_is_inside_disp_buf(disp, char_ln, len);

  /*
  Put characters into the screen buffer
  Also mark the segment that encompases the range of changes
  */
  range_start = -1;
  range_end = -1;
  for (i = 0; i < len; ++i)
  {
    ASSERT(x + i < disp->buf_width);

    a.c = s[i];
    a.a = attr;

    d = char_ln + i;
    if (!disp_char_equal(a, *d))
    {
      if (range_start == -1)
        range_start = i;
      range_end = i;
    }

    char_ln[i].c = s[i];
    char_ln[i].a = attr;
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    _disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
  }

  /*debug_trace("write %d:%d len %d (-%d)\n", x, y, len,
              len - range_num_chars);*/
}

/*!
@brief Displays a string with alternating attributes at a specific screen pos

Attributes are alternated between attr1 and attr2 by the '~' character

Function verifies if screen content changes and will not emit
refresh call to the OS if nothing changed.

@param disp  a display object
@param s     string to display
@param x     coordinates
@param y     coordinates
@param attr1 attributes
@param attr2 attributes
*/
void disp_flex_write(dispc_t *disp,
                     const char *s, int x, int y, int attr1, int attr2)
{
  int display_len;
  int len;
  int i;
  disp_char_t *char_ln;
  disp_char_t *d;
  disp_char_t a;
  int attr;
  int range_start;
  int range_end;
  int range_num_chars;

  ASSERT(VALID_DISP(disp));

  len = strlen(s);
  if (len == 0)
    return;

  char_ln = _disp_buf_access(disp, x, y);

  disp_is_inside_disp_buf(disp, char_ln, len);

  /*
  Put characters into the screen buffer
  Also mark the segment that encompases the range of changes
  */
  range_start = -1;
  range_end = -1;
  display_len = 0;
  attr = attr1;
  for (i = 0; i < len; ++i)
  {
    ASSERT(x + display_len < disp->buf_width);

    if (s[i] == '~')
       attr = (attr == attr1 ? attr2 : attr1);
    else
    {
      a.c = s[i];
      a.a = attr;

      d = char_ln + display_len;
      if (!disp_char_equal(a, *d))
      {
        if (range_start == -1)
          range_start = display_len;
        range_end = display_len;
      }

      char_ln[display_len].c = s[i];
      char_ln[display_len].a = attr;
      ++display_len;
    }
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    _disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
  }

  /*debug_trace("flex_write %d:%d len %d (-%d)\n", x, y, display_len,
              display_len - range_num_chars);*/
}

/*!
@brief Fills an area on the screen with a specific character

Function verifies if screen content changes and will not emit
refresh call to the OS if nothing changed.

@param disp  a display object
@param c     character with whcih to fill
@param attr  attribute of the character
@param x     coordinates
@param y     coordinates
@param count how much to fill
*/
void disp_fill(dispc_t *disp, char c, int attr, int x, int y, int count)
{
  int i;
  disp_char_t *char_ln;
  disp_char_t *d;
  disp_char_t a;
  int range_start;
  int range_end;
  int range_num_chars;

  ASSERT(VALID_DISP(disp));
  ASSERT(count >= 0);

  if (count == 0)
    return;

  char_ln = _disp_buf_access(disp, x, y);
  disp_is_inside_disp_buf(disp, char_ln, count);

  /*
  Put characters into the screen buffer
  Also mark the segment that encompases the range of changes
  */
  range_start = -1;
  range_end = -1;
  a.c = c;
  a.a = attr;
  for (i = 0; i < count; ++i)
  {
    ASSERT(x + i < disp->buf_width);

    d = char_ln + i;
    if (!disp_char_equal(a, *d))
    {
      if (range_start == -1)
        range_start = i;
      range_end = i;
    }

    char_ln[i].c = c;
    char_ln[i].a = attr;
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    _disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
  }

  /*debug_trace("fill %d:%d len %d (-%d)\n", x, y, count,
              i - range_num_chars);*/
}

/*!
@brief Hides the cursor

@param disp a display object
*/
void disp_cursor_hide(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  _disp_show_cursor(disp, 0);
}

/*!
@brief Sets new cursor position

@param disp a display object
@param x    coordinates
@param y    coordinates
*/
void disp_cursor_goto_xy(dispc_t *disp, int x, int y)
{
  ASSERT(VALID_DISP(disp));
  _disp_set_cursor_pos(disp, x, y);
}

/*!
@brief Gets the current cursor position

@param disp a display object
@param x    receives the X coordinate
@param y    receives the Y coordinate
*/
void disp_cursor_get_xy(dispc_t *disp, int *x, int *y)
{
  ASSERT(VALID_DISP(disp));

  *x = disp->cursor_x;
  *y = disp->cursor_y;
}

/*!
@brief Gets the cursor parameters

Cursor parameters are cursor_is_visible, position X and position Y

@param disp    a dispc object
@param cparam  structure that receives the parameters
*/
void disp_cursor_get_param(dispc_t *disp,
                           disp_cursor_param_t *cparam)
{
  ASSERT(VALID_DISP(disp));

  cparam->cursor_is_visible = disp->cursor_is_visible;
  cparam->pos_x = disp->cursor_x;
  cparam->pos_y = disp->cursor_y;
}

/*!
@brief Sets cursors parameters

Cursor parameters are cursor_is_visible, position X and position Y

@param disp    a dispc object
@param cparam  structure from where to get cursor parameters
*/
void disp_cursor_restore_param(dispc_t *disp,
                               const disp_cursor_param_t *cparam)
{
  ASSERT(VALID_DISP(disp));

  _disp_set_cursor_pos(disp, cparam->pos_x, cparam->pos_y);
  _disp_show_cursor(disp, cparam->cursor_is_visible);
}

/*!
@brief Changes the title of the window

@param disp    a dispc object
@param title   a string for the title
*/
void disp_wnd_set_title(dispc_t *disp, const char *title)
{
  ASSERT(VALID_DISP(disp));

  _disp_wnd_set_title(disp, title);
}

/*!
@brief Waits for event from the display window.

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
  while (1)
  {
    if (_disp_ev_q_get(disp, event))
      return 1;

    if (!GetMessage(&msg, NULL, 0, 0))
    {
      disp->code = DISP_MESSAGE_LOOP_FAILURE;
      _snprintf(disp->error_msg, sizeof(disp->error_msg),
                "failed to get message");
      _disp_translate_os_error(disp);
      return 0;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

/*!
@brief Clears an event structure

Beside plain memset(0), in _DEBUG mode the structure carries unique ID.

@param event the event to clear
*/
void disp_event_clear(disp_event_t *event)
{
  memset(event, 0, sizeof(*event));
}

/*!
@brief Verifies if an event structure is valid

Only in _DEBUG mode might return false if event doesn't point to a
valid structure

@param event pointer to an event structure for verification
*/
int  disp_event_is_valid(const disp_event_t *event)
{
  return 1;
}

/*!
@brief just a safety wrap of strcat
*/
static void _safe_strcat(char *dest, int dest_buf_size, const char *src)
{
  int copy_len;
  int dest_len;

  ASSERT(dest_buf_size > 0);

  dest_len = strlen(dest);
  copy_len = dest_buf_size - dest_len;
  --copy_len;  /* strncat needs one for asciiz terminator */
  if (copy_len <= 0)
    return;

  strncat(dest, src, copy_len);
}

/*
Key names (scan code as index)
*/
static const char *key_names[] =
{
  "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "BckSpc",
  "Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Return",
  "Ctrl", "A", "S", "D", "F", "G", "H", "J", "K", "L", ";", "'", "`",
  "Shift", "\\", "Z", "X", "C", "V", "B", "N", "M", ",", ".", "/", "Shift", "PrSc",
  "Alt", "Space", "CapsLock",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
  "NumLock", "ScrollLock", "Home", "Up", "PgUp", "Gray-",
  "Left", "Pad5", "Right", "Gray+",
  "End", "Down", "PgDn", "Ins", "Del"
};

/*!
@brief returns string representation for a specific key

@param disp          a dispc object
@param key           the key
@param key_name_buf  receives the string
@param buf_size      how big is the destination buffer
*/
void disp_get_key_name(dispc_t *disp,
  unsigned long key, char *key_name_buf, int buf_size)
{
  unsigned char scan_code;
  unsigned char ascii_code;
  unsigned int shift_state;
  char scan_code_buf[25];

  scan_code = (unsigned char )((key & 0x0000ff00L) >> 8);
  ascii_code = (unsigned char)(key & 0x000000ffL);
  shift_state = (unsigned int)((key & 0xffff0000L) >> 16);

  key_name_buf[0] = '\0';

  if (scan_code == 0)  /* Code from alt+numpad combination */
  {
    snprintf(key_name_buf, buf_size, "ASCII: %d", ascii_code);
    return;
  }

  if (shift_state & kbCtrl)
    _safe_strcat(key_name_buf, buf_size, "Ctrl+");

  if (shift_state & kbAlt)
    _safe_strcat(key_name_buf, buf_size, "Alt+");

  if (shift_state & kbShift)
    _safe_strcat(key_name_buf, buf_size, "Shift+");

  if (scan_code > 83)
  {
    if (scan_code == 87)
      _safe_strcat(key_name_buf, buf_size, "F11");
    else
      if (scan_code == 88)
        _safe_strcat(key_name_buf, buf_size, "F12");
      else
      {
        snprintf(scan_code_buf, sizeof(scan_code_buf) - 1,
                 "<%d>", scan_code);
        _safe_strcat(key_name_buf, buf_size, scan_code_buf);
      }
  }
  else
    _safe_strcat(key_name_buf, buf_size, key_names[scan_code - 1]);
}

/*!
@brief gets the elapsed time since last set

@param disp  a dispc object
@param t     receives the elapsed time (fields for hour, min, sec)
*/
void disp_elapsed_time_get(dispc_t *disp, disp_elapsed_time_t *t)
{
  ASSERT(VALID_DISP(disp));

  t->hours = disp->hours;
  t->minutes = disp->minutes;
  t->seconds = disp->seconds;
  t->total_seconds = disp->time_elapsed;
}

/*!
@brief sets the elapsed timer to certain time

The library counts the time while the program is running.

@param disp  a dispc object
@param t     the time (fields for hour, min, sec)
*/
void disp_elapsed_time_set(dispc_t *disp, const disp_elapsed_time_t *t)
{
  ASSERT(VALID_DISP(disp));

  disp->hours = t->hours;
  disp->minutes = t->minutes;
  disp->seconds = t->seconds;
  disp->time_elapsed = t->total_seconds;
}

/*!
@brief Initial setup of the internal event queue

Various Windows events are converted into disp events and put into
this internal queue from which the disp_event_read() function extracts
them one by one

@param disp a dispc object
*/
void _disp_ev_q_init(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  disp->ev_c = disp->ev_h = disp->ev_t = 0;
}

/*!
@brief Shuts down the internal event queue

@param disp a dispc object
*/
void _disp_ev_q_done(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
}

/*!
@brief Puts an event into the internal event queue

@param disp  a dispc object
@param ev    the event to copy in the queue
*/
void _disp_ev_q_put(dispc_t *disp, const disp_event_t *ev)
{
  ASSERT(VALID_DISP(disp));

  if (disp->ev_c < DISP_EVENT_QUEUE_SIZE)
  {
    memcpy(&disp->ev_q[disp->ev_t], ev, sizeof(disp_event_t));
    disp->ev_t = (disp->ev_t + 1) % DISP_EVENT_QUEUE_SIZE;
    ++disp->ev_c;
  }
}

/*!
@brief Gets an event from the internal event queue

@param disp  a dispc object
@param ev    memory where the event will be copyed
*/
int _disp_ev_q_get(dispc_t *disp, disp_event_t *ev)
{
  ASSERT(VALID_DISP(disp));

  if (disp->ev_c == 0)
    return 0;

  memcpy(ev, &disp->ev_q[disp->ev_h], sizeof(disp_event_t));
  disp->ev_h = (disp->ev_h + 1) % DISP_EVENT_QUEUE_SIZE;
  --disp->ev_c;
  return 1;
}

/*!
@brief Makes the caret visible or invisible (win32 GUI)

@param disp              a dispc object
@param caret_is_visible  new state of the caret
*/
void _disp_show_cursor(dispc_t *disp, int caret_is_visible)
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

/*!
@brief Gets human readable message for a system error (win32 GUI)

@param disp  a dispc object
*/
void _disp_translate_os_error(dispc_t *disp)
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

/*
@brief default parameters for the display
*/
static disp_wnd_param_t default_wnd_param =
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
static DWORD disp_colors[16] =
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
@brief Called whenever a segment of the window must be redrawn

The WM_PAINT message is sent when the system or another application makes
a request to paint a portion of an application's window. This function
validates the update region.

@param disp  a dispc object
@param dc    drawing device context
@param upd   the update region
*/
void _disp_on_paint(dispc_t *disp, HDC dc, const RECT *upd)
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
        cur_attr = disp_colors[15];
      }
      else
      {
        /* copy a region of text that has the same attributes */
        char_ln = _disp_buf_access(disp, x1, y);

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

      SetTextColor(dc, disp_colors[cur_attr & 15]);
      SetBkColor(dc, disp_colors[(cur_attr >> 4) & 15]);

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
      SetTextColor(dc, disp_colors[cur_attr & 15]);
      SetBkColor(dc, disp_colors[(cur_attr >> 4) & 15]);

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
@brief Handles the translation of Windows keyboard endcodings to disp keydefs

Generates and enqueues an event by translating from Windows message
and params passed to the _disp_wnd_proc().
*/
void _disp_on_key(dispc_t *disp,
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

  ascii_code = param1 & 255;
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
        _disp_ev_q_put(disp, &ev);
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
        _disp_ev_q_put(disp, &ev);
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
          ascii_code = msg.wParam & 255;
        code = ((unsigned long int)shift_state) << 16
               | (scan_code << 8) | ascii_code;
        disp_get_key_name(disp, code, buf, sizeof(buf));
        KEY_TRACE2("key: %s, ascii: %c\n", buf, ascii_code);

        ev.e.kbd.scan_code_only = scan_code;
        ev.e.kbd.shift_state = shift_state;
        ev.e.kbd.key = code;
        _disp_ev_q_put(disp, &ev);

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
        _disp_ev_q_put(disp, &ev);
      }
    }
  }  /* message == WM_KEYDOWN */
}

/*!
@brief Aligns new size to certain granularity in units
*/
int _disp_align_size(int new_size, int units)
{
  new_size /= units;
  new_size *= units;
  return new_size;
}

/*!
@brief Call back function for Windows events
*/
LRESULT CALLBACK
_disp_wnd_proc(HWND hWnd,
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
      _disp_on_paint(disp, dc, &ps.rcPaint);
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
          win_pos->cx = _disp_align_size(win_pos->cx, disp->char_size.cx);
          if (move_x != 0 && resize_x != 0)  /* resize left side? */
            win_pos->x += (disp->last_width - win_pos->cx);

          win_pos->cy = _disp_align_size(win_pos->cy, disp->char_size.cy);
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
          _disp_alloc_char_buf(disp);

        if (disp->handle_resize != NULL)
          disp->handle_resize(&ev, disp->handle_resize_ctx);
        else
          _disp_ev_q_put(disp, &ev);
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
      _disp_on_key(disp, message, (unsigned)wParam, (unsigned)lParam);
      break;

    case WM_TIMER:
      disp_event_clear(&ev);
      ev.t.code = EVENT_TIMER_5SEC;
      _disp_ev_q_put(disp, &ev);
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
int _disp_init(dispc_t *disp)
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
    _disp_translate_os_error(disp);
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
  wc.lpfnWndProc = (WNDPROC)_disp_wnd_proc;
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
    _disp_translate_os_error(disp);
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
    _disp_translate_os_error(disp);
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
    _disp_translate_os_error(disp);
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
    _disp_translate_os_error(disp);
    return 0;
  }

  return TRUE;
}

/*!
@brief platform specific disp cleanup (win32 GUI)

@param disp  a display object
*/
void _disp_done(dispc_t *disp)
{
  KillTimer(disp->wnd, disp->win32_timer_id);
}

/*!
@brief Sets the caret on a specific position (win32 GUI)

@param disp  a display object
@param x     coordinates in character units
@param y     coordinates in character units
*/
void _disp_set_cursor_pos(dispc_t *disp, int x, int y)
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
void _disp_wnd_set_title(dispc_t *disp, const char *title)
{
  SetWindowText(disp->wnd, title );
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2006
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

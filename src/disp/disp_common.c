/*!
@file disp_common.c
@brief [disp] Platform independent top level API impelementation. An include file.

@section a Header

File: disp_common.c\n
COPYING: Full text of the copyrights statement at the bottom of the file\n
Project: WW text editor\n
Started: 27th October, 1998\n
Refactored: 27th Sep, 2006 from w_scr.c and w_kbd.h\n


@section b Module disp

Module disp is a standalone library that abstracts the access to
console output for WIN32 console, WIN32 GUI, ncurses and X11.

This is an implementation of the top level API described in disp.h.
It is an include file.


@section c Compile time definitions

D_ASSERT -- External assert() replacement function can be supplied in the form
of a define -- D_ASSERT. It must conform to the standard C library
prototype of assert.
*/

static char disp_id[] = "disp-lib";

#ifdef WIN32
#define snprintf _snprintf
#define DISP_REFERENCE(x) (x);
#else
#define DISP_REFERENCE(x)
#endif

#define DISP_LO_BYTE(x) (unsigned char)((unsigned)(x) & 0xff)

/* To be used in ASSERT()! */
#ifdef _DEBUG
static int s_disp_cbuf_is_valid(const disp_char_buf_t *cbuf);
#define VALID_DISP_CHAR_BUF(cbuf) (s_disp_cbuf_is_valid(cbuf))
#else
#define VALID_DISP_CHAR_BUF(cbuf) (1)
#endif

/*
Forward definitions of the platform specific functions that are
called from the top-level API.

Implementations are in win32_g_disp.c or ncurs_disp.c, depending
on which platform the library is compiled.
*/
static int s_disp_init(dispc_t *disp);
static void s_disp_set_cursor_pos(dispc_t *disp, int x, int y);
static void s_disp_show_cursor(dispc_t *disp, int caret_is_visible);
static void s_disp_validate_rect(dispc_t *disp,
                                 int x, int y,
                                 int w, int h);
static void s_disp_done(dispc_t *disp);
static void s_disp_wnd_set_title(dispc_t *disp, const char *title);
static int s_disp_process_events(dispc_t *disp);

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
@brief Initial setup of the internal event queue

Various Windows events are converted into disp events and put into
this internal queue from which the disp_event_read() function extracts
them one by one

@param disp a dispc object
*/
static void s_disp_ev_q_init(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
  disp->ev_c = disp->ev_h = disp->ev_t = 0;
}

/*!
@brief Shuts down the internal event queue

@param disp a dispc object
*/
static void s_disp_ev_q_done(dispc_t *disp)
{
  ASSERT(VALID_DISP(disp));
}

/*!
@brief Puts an event into the internal event queue

@param disp  a dispc object
@param ev    the event to copy in the queue
*/
static void s_disp_ev_q_put(dispc_t *disp, const disp_event_t *ev)
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
static int s_disp_ev_q_get(dispc_t *disp, disp_event_t *ev)
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
  r = s_disp_init(disp);

  if (r)
  {
    s_disp_set_cursor_pos(disp, 0, 0);
    s_disp_show_cursor(disp, 1);
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
static void s_disp_free_char_buf(dispc_t *disp)
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
  s_disp_ev_q_done(disp);
  s_disp_free_char_buf(disp);
  s_disp_done(disp);

  #ifdef _DEBUG
  memset(disp, -1, sizeof(dispc_t));
  #endif
}

/*!
@brief Wrapper to call the optional malloc replacement function

@param disp display context
@param size size to pass to malloc
*/
static void *s_disp_malloc(dispc_t *disp, size_t size)
{
  void *(*my_malloc)(size_t size);

  my_malloc = disp->safe_malloc;
  if (my_malloc == NULL)  /* no user established mem handlers? */
  {
    /* then use the libc's malloc() and free() */
    my_malloc = &malloc;
  }
  return my_malloc(size);
}

/*!
@brief Wrapper to call the optional free() replacement

@param disp display context
@param p    block address to pass to free()
*/
static void s_disp_free(dispc_t *disp, void *p)
{
  void (*my_free)(void *buf);

  my_free = disp->safe_free;
  if (my_free == NULL)  /* no user established mem handlers? */
  {
    /* then use the libc's malloc() and free() */
    my_free = &free;
  }
  my_free(p);
}

/*!
 * @brief allocates memory for the character buffer if necessary
 *
 * disp->char_buf can't be allocated at disp_init() time because we don't know
 * if disp_set_safemem_proc() will not be called after that to establish custom
 * memory handlers.
 *
 * This function is called by evey function that interacts with char_buf and it
 * either establishes char_buf for the first time or it reallocates if new
 * window parameters exceed the capacity of char_buf
 *
 * @param disp a display object
*/
static void s_disp_alloc_char_buf(dispc_t *disp)
{
  int required_size;
  int char_buf_size;

  required_size = disp->geom_param.height * disp->geom_param.width;
  char_buf_size = disp->buf_height * disp->buf_width;

  ASSERT(required_size >= 0);

  if (char_buf_size >= required_size)
    return;
  else
  {
    if (disp->char_buf != NULL)
      s_disp_free(disp, disp->char_buf);

    disp->char_buf = s_disp_malloc(disp, required_size * sizeof(disp_char_t));
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

  s_disp_alloc_char_buf(disp);
}

static disp_wnd_param_t s_default_wnd_param;  /* forward declaration */

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
    memcpy(&disp->geom_param, &s_default_wnd_param, sizeof(disp_wnd_param_t));
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
static int s_disp_cbuf_is_valid(const disp_char_buf_t *cbuf)
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
static int s_disp_cbuf_calc_size(int num_chars)
{
  ASSERT(num_chars > 0);
  return sizeof(disp_char_buf_t) + num_chars * sizeof(disp_char_t);
}

/*!
@brief Initializes cbuf object.

disp_char_buf_t is the data carrier for functions that put text on
the screen. The buffer knows its size to prevent overflows. This function
initializes one such buffer -- stores the size and resets the data area.

@param disp           a dispc object (unused)
@param cbuf           pointer to a buffer of type disp_char_buf_t
@param max_characters how many characters are to be stored in this buffer
*/
void disp_cbuf_reset(const dispc_t *disp, disp_char_buf_t *cbuf,
                     int max_characters)
{
  #ifdef _DEBUG
  cbuf->magic_byte = DISP_CHAR_BUF_MAGIC;
  #endif

  DISP_REFERENCE(*disp);
  memset(cbuf, s_disp_cbuf_calc_size(max_characters), 0);
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
  DISP_REFERENCE(*disp);
  ASSERT(VALID_DISP_CHAR_BUF(cbuf));
  memset(cbuf, s_disp_cbuf_calc_size(cbuf->max_characters), 0);
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
  DISP_REFERENCE(*disp);
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
  DISP_REFERENCE(*disp);
  ASSERT(VALID_DISP_CHAR_BUF(dest_buf));
  ASSERT(index < dest_buf->max_characters);

  dest_buf->cbuf[index].a = DISP_LO_BYTE(attr);
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
  DISP_REFERENCE(*disp);
  ASSERT(VALID_DISP_CHAR_BUF(dest_buf));
  ASSERT(index < dest_buf->max_characters);

  dest_buf->cbuf[index].c = DISP_LO_BYTE(c);
  dest_buf->cbuf[index].a = DISP_LO_BYTE(attr);
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
  DISP_REFERENCE(*disp);
  return s_disp_cbuf_calc_size(width * height);
}

/*!
@brief Calculates the position of screen position x,y in disp->char_buf

@param disp  a dispc object
@param x     window position coordonates
@param y     window position coordonates
*/
static disp_char_t* s_disp_buf_access(dispc_t *disp, int x, int y)
{
  s_disp_alloc_char_buf(disp);
  return &disp->char_buf[y * disp->buf_width + x];
}

#ifdef _DEBUG
static void s_disp_is_inside_disp_buf(dispc_t *disp, disp_char_t *dest, int width)
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
    char_ln = s_disp_buf_access(disp, x, y + ln);

    ASSERT((ln * w + w) < buf->max_characters);  /* valid source */
    s_disp_is_inside_disp_buf(disp, char_ln, w);  /* valid dest */

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
      s_disp_validate_rect(disp, x + range_start, y, range_num_chars, h);
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
    char_ln = s_disp_buf_access((void *)disp, x, y + ln);
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

  char_ln = s_disp_buf_access(disp, x, y);

  s_disp_is_inside_disp_buf(disp, char_ln, len);

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
    a.a = DISP_LO_BYTE(attr);

    d = char_ln + i;
    if (!disp_char_equal(a, *d))
    {
      if (range_start == -1)
        range_start = i;
      range_end = i;
    }

    char_ln[i].c = s[i];
    char_ln[i].a = DISP_LO_BYTE(attr);
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    s_disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
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

  char_ln = s_disp_buf_access(disp, x, y);

  s_disp_is_inside_disp_buf(disp, char_ln, len);

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
      a.a = DISP_LO_BYTE(attr);

      d = char_ln + display_len;
      if (!disp_char_equal(a, *d))
      {
        if (range_start == -1)
          range_start = display_len;
        range_end = display_len;
      }

      char_ln[display_len].c = s[i];
      char_ln[display_len].a = DISP_LO_BYTE(attr);
      ++display_len;
    }
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    s_disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
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

  char_ln = s_disp_buf_access(disp, x, y);
  s_disp_is_inside_disp_buf(disp, char_ln, count);

  /*
  Put characters into the screen buffer
  Also mark the segment that encompases the range of changes
  */
  range_start = -1;
  range_end = -1;
  a.c = c;
  a.a = DISP_LO_BYTE(attr);
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
    char_ln[i].a = DISP_LO_BYTE(attr);
  }

  range_num_chars = 0;
  if (range_start != -1)
  {
    range_num_chars = range_end - range_start + 1;
    s_disp_validate_rect(disp, x + range_start, y, range_num_chars, 1);
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
  s_disp_show_cursor(disp, 0);
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
  s_disp_set_cursor_pos(disp, x, y);
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

  s_disp_set_cursor_pos(disp, cparam->pos_x, cparam->pos_y);
  s_disp_show_cursor(disp, cparam->cursor_is_visible);
}

/*!
@brief Changes the title of the window

@param disp    a dispc object
@param title   a string for the title
*/
void disp_wnd_set_title(dispc_t *disp, const char *title)
{
  ASSERT(VALID_DISP(disp));

  s_disp_wnd_set_title(disp, title);
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
  DISP_REFERENCE(*event);
  return 1;
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
  event->t.code = EVENT_NONE;
  for (;;)
  {
    if (s_disp_ev_q_get(disp, event))
      return 1;

    if (!s_disp_process_events(disp))
      return 0;
  }
}

/*!
@brief just a safety wrap of strcat
*/
static void s_safe_strcat(char *dest, int dest_buf_size, const char *src)
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
                       unsigned long key,
                       char *key_name_buf,
                       int buf_size)
{
  unsigned char scan_code;
  unsigned char ascii_code;
  unsigned int shift_state;
  char scan_code_buf[25];

  DISP_REFERENCE(*disp);
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
    s_safe_strcat(key_name_buf, buf_size, "Ctrl+");

  if (shift_state & kbAlt)
    s_safe_strcat(key_name_buf, buf_size, "Alt+");

  if (shift_state & kbShift)
    s_safe_strcat(key_name_buf, buf_size, "Shift+");

  if (scan_code > 83)
  {
    if (scan_code == 87)
      s_safe_strcat(key_name_buf, buf_size, "F11");
    else
      if (scan_code == 88)
        s_safe_strcat(key_name_buf, buf_size, "F12");
      else
      {
        snprintf(scan_code_buf, sizeof(scan_code_buf) - 1,
                 "<%d>", scan_code);
        s_safe_strcat(key_name_buf, buf_size, scan_code_buf);
      }
  }
  else
    s_safe_strcat(key_name_buf, buf_size, key_names[scan_code - 1]);
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


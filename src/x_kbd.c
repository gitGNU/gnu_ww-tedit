/*

File: x_kbd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 3rd February, 2003
Descrition:
  Keyboard manipulation functions. Implemented for X Windows.

*/

#include "global.h"
#include "options.h"
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"

#include "xscr_i.h"

#define XK_MISCELLANY
#include <X11/keysymdef.h>

BOOLEAN bCtrlReleased = TRUE;
WORD ShiftState;

int nTimeElapsed = 0;
int nGlobalHours = 0;  /* Global hours elapsed */
int nGlobalMinutes = 0;  /* Global minutes elapsed */
int nGlobalSeconds = 0;  /* Global seconds elapsed */

BOOLEAN bTraceKbd = FALSE;  /* toggled from the diag term in diag.c */

extern void DrawCursor(void);

/* ************************************************************************
   Function: GetKeyName
   Description:
     Produces a key name combination by a particular code
   On exit:
     KeyName - key combination image string
*/
void GetKeyName(DWORD dwKey, char *psKeyName)
{
  BYTE ScanCode;
  BYTE AsciiCode;
  WORD ShiftState;

  ScanCode = (BYTE)((dwKey & 0x0000ff00L) >> 8);
  AsciiCode = (BYTE)(dwKey & 0x000000ffL);
  ShiftState = (WORD)((dwKey & 0xffff0000L) >> 16);

  psKeyName[0] = '\0';

  if (ScanCode == 0)  /* Code from alt+numpad combination */
  {
    sprintf(psKeyName, "ASCII: %d", AsciiCode);
    return;
  }

  if (ShiftState & kbCtrl)
    strcat(psKeyName, "Ctrl+");

  if (ShiftState & kbAlt)
    strcat(psKeyName, "Alt+");

  if (ShiftState & kbShift)
    strcat(psKeyName, "Shift+");

  if (ScanCode > 83)
  {
    if (ScanCode == 87)
      strcat(psKeyName, "F11");
    else
      if (ScanCode == 88)
        strcat(psKeyName, "F12");
      else
        sprintf(strchr(psKeyName, '\0'), "<%d>", ScanCode);
  }
  else
    strcat(psKeyName, KeyNames[ScanCode - 1]);
}

typedef struct KeySequence
{
  /* Description of what can be read from the terminal */
  char *sSequence;
  /* Description of how this to be transformed into scancode/shift state pair */
  DWORD nKey;  /* hi word is shift state, lo word is scan code + asci char */
} TKeySequence;

/* X Server generates ASCII for a key event.
We need to convert those characters into
a Ctrl/Shift/Alt key combination */
TKeySequence AKeys[] =
{
  {"`",         '`'},
  {"~",         '~'},
  {"1",         KEY(0,       kb1) | '1'},
  {"!",         KEY(kbShift, kb1) | '!'},
  {"2",         KEY(0,       kb2) | '2'},
  {"@",         KEY(kbShift, kb2) | '@'},
  {"3",         KEY(0,       kb3) | '3'},
  {"#",         KEY(kbShift, kb3) | '#'},
  {"4",         KEY(0,       kb4) | '4'},
  {"$",         KEY(kbShift, kb4) | '$'},
  {"5",         KEY(0,       kb5) | '5'},
  {"%",         KEY(kbShift, kb5) | '%'},
  {"6",         KEY(0,       kb6) | '6'},
  {"^",         KEY(kbShift, kb6) | '^'},
  {"7",         KEY(0,       kb7) | '7'},
  {"&",         KEY(kbShift, kb7) | '&'},
  {"8",         KEY(0,       kb8) | '8'},
  {"*",         KEY(kbShift, kb8) | '*'},
  {"9",         KEY(0,       kb9) | '9'},
  {"(",         KEY(kbShift, kb9) | '('},
  {"0",         KEY(0,       kb0) | '0'},
  {")",         KEY(kbShift, kb0) | ')'},
  {"-",         KEY(0,       kbMinus) | '-'},
  {"_",         KEY(kbShift, kbMinus) | '_'},
  {"=",         KEY(0,       kbEqual) | '='},
  {"+",         KEY(kbShift, kbEqual) | '+'},
  //{"\x7f",      KEY(0,       kbBckSpc) /*| '\x7f'*/},
  //{"\x09",      KEY(0,       kbTab) /*| '\x09'*/},
  {"q",         KEY(0,       kbQ) | 'q'},
  {"Q",         KEY(kbShift, kbQ) | 'Q'},
  {"w",         KEY(0,       kbW) | 'w'},
  {"W",         KEY(kbShift, kbW) | 'W'},
  {"e",         KEY(0,       kbE) | 'e'},
  {"E",         KEY(kbShift, kbE) | 'E'},
  {"r",         KEY(0,       kbR) | 'r'},
  {"R",         KEY(kbShift, kbR) | 'R'},
  {"t",         KEY(0,       kbT) | 't'},
  {"T",         KEY(kbShift, kbT) | 'T'},
  {"y",         KEY(0,       kbY) | 'y'},
  {"Y",         KEY(kbShift, kbY) | 'Y'},
  {"u",         KEY(0,       kbU) | 'u'},
  {"U",         KEY(kbShift, kbU) | 'U'},
  {"i",         KEY(0,       kbI) | 'i'},
  {"I",         KEY(kbShift, kbI) | 'I'},
  {"o",         KEY(0,       kbO) | 'o'},
  {"O",         KEY(kbShift, kbO) | 'O'},
  {"p",         KEY(0,       kbP) | 'p'},
  {"P",         KEY(kbShift, kbP) | 'P'},
  {"[",         KEY(0,       kbLBrace) | '['},
  {"{",         KEY(kbShift, kbLBrace) | '{'},
  {"]",         KEY(0,       kbRBrace) | ']'},
  {"}",         KEY(kbShift, kbRBrace) | '}'},
  {"\x0d",      KEY(0,       kbEnter) /*| '\x0d'*/},
  {"a",         KEY(0,       kbA) | 'a'},
  {"A",         KEY(kbShift, kbA) | 'A'},
  {"s",         KEY(0,       kbS) | 's'},
  {"S",         KEY(kbShift, kbS) | 'S'},
  {"d",         KEY(0,       kbD) | 'd'},
  {"D",         KEY(kbShift, kbD) | 'D'},
  {"f",         KEY(0,       kbF) | 'f'},
  {"F",         KEY(kbShift, kbF) | 'F'},
  {"g",         KEY(0,       kbG) | 'g'},
  {"G",         KEY(kbShift, kbG) | 'G'},
  {"h",         KEY(0,       kbH) | 'h'},
  {"H",         KEY(kbShift, kbH) | 'H'},
  {"j",         KEY(0,       kbJ) | 'j'},
  {"J",         KEY(kbShift, kbJ) | 'J'},
  {"k",         KEY(0,       kbK) | 'k'},
  {"K",         KEY(kbShift, kbK) | 'K'},
  {"l",         KEY(0,       kbL) | 'l'},
  {"L",         KEY(kbShift, kbL) | 'L'},
  {";",         KEY(0,       kbColon) | ';'},
  {":",         KEY(kbShift, kbColon) | ':'},
  {"\"",        KEY(0,       kb_1) | '\"'},
  {"\'",        KEY(kbShift, kb_1) | '\''},
  {"\\",        KEY(0,       kb_2) | '\\'},
  {"|",         KEY(kbShift, kb_2) | '|'},
  {"z",         KEY(0,       kbZ) | 'z'},
  {"Z",         KEY(kbShift, kbZ) | 'Z'},
  {"x",         KEY(0,       kbX) | 'x'},
  {"X",         KEY(kbShift, kbX) | 'X'},
  {"c",         KEY(0,       kbC) | 'c'},
  {"C",         KEY(kbShift, kbC) | 'C'},
  {"v",         KEY(0,       kbV) | 'v'},
  {"V",         KEY(kbShift, kbV) | 'V'},
  {"b",         KEY(0,       kbB) | 'b'},
  {"B",         KEY(kbShift, kbB) | 'B'},
  {"n",         KEY(0,       kbN) | 'n'},
  {"N",         KEY(kbShift, kbN) | 'N'},
  {"m",         KEY(0,       kbM) | 'm'},
  {"M",         KEY(kbShift, kbM) | 'M'},
  {",",         KEY(0,       kbComa) | ','},
  {"<",         KEY(kbShift, kbComa) | '<'},
  {".",         KEY(0,       kbPeriod) | '.'},
  {">",         KEY(kbShift, kbPeriod) | '>'},
  {"/",         KEY(0,       kbSlash) | '/'},
  {"?",         KEY(kbShift, kbSlash) | '?'},
  {" ",         KEY(0,       kbSpace) | ' '},

  {"\x01",      KEY(kbCtrl,  kbA) | '\x01'},
  {"\x02",      KEY(kbCtrl,  kbB) | '\x02'},
  {"\x03",      KEY(kbCtrl,  kbC) | '\x03'},
  {"\x04",      KEY(kbCtrl,  kbD) | '\x04'},
  {"\x05",      KEY(kbCtrl,  kbE) | '\x05'},
  {"\x06",      KEY(kbCtrl,  kbF) | '\x06'},
  {"\x07",      KEY(kbCtrl,  kbG) | '\x07'},
  {"\x08",      KEY(kbCtrl,  kbH) | '\x08'},
  {"\x09",      KEY(kbCtrl,  kbI) | '\x09'},
  {"\x0a",      KEY(kbCtrl,  kbJ) | '\x0a'},
  {"\x0b",      KEY(kbCtrl,  kbK) | '\x0b'},
  {"\x0c",      KEY(kbCtrl,  kbL) | '\x0c'},
  {"\x0d",      KEY(kbCtrl,  kbM) | '\x0d'},
  {"\x0e",      KEY(kbCtrl,  kbN) | '\x0e'},
  {"\x0f",      KEY(kbCtrl,  kbO) | '\x0f'},
  {"\x10",      KEY(kbCtrl,  kbP) | '\x10'},
  {"\x11",      KEY(kbCtrl,  kbQ) | '\x11'},
  {"\x12",      KEY(kbCtrl,  kbR) | '\x12'},
  {"\x13",      KEY(kbCtrl,  kbS) | '\x13'},
  {"\x14",      KEY(kbCtrl,  kbT) | '\x14'},
  {"\x15",      KEY(kbCtrl,  kbU) | '\x15'},
  {"\x16",      KEY(kbCtrl,  kbV) | '\x16'},
  {"\x17",      KEY(kbCtrl,  kbW) | '\x17'},
  {"\x18",      KEY(kbCtrl,  kbX) | '\x18'},
  {"\x19",      KEY(kbCtrl,  kbY) | '\x19'},
  {"\x1a",      KEY(kbCtrl,  kbZ) | '\x1a'},
  {"\033[[G",   KEY(kbCtrl,  kbLBrace) | '\x1b'},
};

/*
Functional keys are defined in a separate table
*/
typedef struct _FKeyDefs
{
  long nKeySym;
  DWORD nKey;
} TFKeyDefs;

TFKeyDefs FKeys[] =
{
  {XK_Escape, KEY(0,kbEsc)},
  {XK_Tab, KEY(0, kbTab)},
  {0xfe20, KEY(0, kbTab)},
  {XK_Return, KEY(0, kbEnter)},
//  {XK_Pause, KEY(0, kbPause)},
  {XK_BackSpace, KEY(0, kbBckSpc)},
  {XK_Home, KEY(0, kbHome)},
  {XK_Up, KEY(0, kbUp)},
  {XK_Prior, KEY(0, kbPgUp)},
  {XK_Left, KEY(0, kbLeft)},
  {XK_Right, KEY(0, kbRight)},
  {XK_End, KEY(0, kbEnd)},
  {XK_Down, KEY(0, kbDown)},
  {XK_Next, KEY(0, kbPgDn)},
  {XK_Select, KEY(0, kbEnd)},
  {XK_KP_Enter, KEY(0, kbEnter)},
  {XK_Insert, KEY(0, kbIns)},
  {XK_Delete, KEY(0, kbDel)},
  {XK_KP_Insert, KEY(0, kbIns)},
  {XK_KP_Delete, KEY(0, kbDel)},
  {XK_KP_Add, KEY(0, kbGrayPlus)},
  {XK_KP_Subtract, KEY(0 , kbGrayMinus)},
//  {XK_KP_Multiply,    '*' | kfGray },
//  {XK_KP_Divide,      '/' | kfGray },
  {XK_KP_Begin, KEY(0, kbPgUp)},
  {XK_KP_Home, KEY(0, kbHome)},
  {XK_KP_Up, KEY(0, kbUp)},
  {XK_KP_Prior, KEY(0, kbPgUp)},
  {XK_KP_Left, KEY(0, kbLeft)},
  {XK_KP_Right, KEY(0, kbRight)},
  {XK_KP_End, KEY(0, kbEnd)},
  {XK_KP_Down, KEY(0, kbDown)},
  {XK_KP_Next, KEY(0, kbPgDn)},
//  {XK_Num_Lock, KEY(0, kbNumLock)},
//  {XK_Caps_Lock, KEY(0, kbCapsLock)},
  {XK_Print, KEY(0, kbPrSc)},
//  {XK_Shift_L, KEY(0, kb_LShift)},
//  {XK_Shift_R, KEY(0, kb_RShift)},
//  {XK_Control_L, KEY(0, kb_Ctrl)},
//  {XK_Control_R, KEY(0, kb_Ctrl)},
//  {XK_Alt_L, KEY(0, kbAlt)},
//  {XK_Alt_R, KEY(0, kbAlt)},
//  {XK_Meta_L, KEY(0, kbAlt)},
//  {XK_Meta_R, KEY(0, kbAlt)},
  {XK_F1, KEY(0, kbF1)},
  {XK_F2, KEY(0, kbF2)},
  {XK_F3, KEY(0, kbF3)},
  {XK_F4, KEY(0, kbF4)},
  {XK_F5, KEY(0, kbF5)},
  {XK_F6, KEY(0, kbF6)},
  {XK_F7, KEY(0, kbF7)},
  {XK_F8, KEY(0, kbF8)},
  {XK_F9, KEY(0, kbF9)},
  {XK_F10, KEY(0, kbF10)},
  {XK_F11, KEY(0, kbF11)},
  {XK_F12, KEY(0, kbF12)},
//  {XK_KP_0,           '0' | kfGray },
//  {XK_KP_1,           '1' | kfGray },
//  {XK_KP_2,           '2' | kfGray },
//  {XK_KP_3,           '3' | kfGray },
//  {XK_KP_4,           '4' | kfGray },
//  {XK_KP_5,           '5' | kfGray },
//  {XK_KP_6,           '6' | kfGray },
//  {XK_KP_7,           '7' | kfGray },
//  {XK_KP_8,           '8' | kfGray },
//  {XK_KP_9,           '9' | kfGray },
//  {XK_KP_Decimal,     '.' | kfGray },
//  {0x1000FF6F, KEY(0, kbDel | kfShift | kfGray)},
//  {0x1000FF70, KEY(0, kbIns | kfCtrl | kfGray)},
//  {0x1000FF71, KEY(0, kbIns | kfShift | kfGray)},
//  {0x1000FF72, KEY(0, kbIns | kfGray)},
//  {0x1000FF73, KEY(0, kbDel | kfGray)},
//  {0x1000FF74, KEY(0, kbTab | kfShift)},
//  {0x1000FF75, KEY(0, kbTab | kfShift)},
//  {0,                 0 }
};

extern void PrintString(const char *fmt, ...);  /* from filecmd.c */

/* ************************************************************************
   Function: GetShiftState
   Description:
*/
static BYTE GetShiftState(XKeyEvent *e)
{
  BYTE state;

  state = 0;
  if (e->state & ShiftMask)
    state |= kbShift;
  if (e->state & ControlMask)
    state |= kbCtrl;
  if (e->state & Mod1Mask)
    state |= kbAlt;

  return state;
}

/* ************************************************************************
   Function: TranslateKey
   Description:
     X event to our event when key is pressed.
*/
static BOOLEAN TranslateKey(XKeyEvent *e, DWORD *code)
{
  int CharCount;
  char buffer[5];
  int bufsize = _countof(buffer);
  KeySym keysym;
  XComposeStatus compose;
  int i;

  CharCount = XLookupString(e, buffer, bufsize, &keysym, &compose);
  buffer[CharCount] = '\0';  /* add a terminator */

  ShiftState = GetShiftState(e);  /* Global variable to ref. the state */

  /* Scan the function keys */
  for (i = 0; i < _countof(FKeys); ++i)
  {
    if (keysym == FKeys[i].nKeySym)
    {
      *code = FKeys[i].nKey | (ShiftState << 16);
      {
        char KeyName[100];
        GetKeyName(*code, KeyName);
        /*TRACE2("key2 %d %s\n", i, KeyName);*/
      }
      return TRUE;
    }
  }

  /* Scan the predefined array os strings to match a sequence */
  for (i = 0; i < _countof(AKeys); ++i)
  {
    if (strcmp(buffer, AKeys[i].sSequence) == 0)
    {
      *code =  AKeys[i].nKey | (ShiftState << 16);
      {
        char KeyName[100];
        GetKeyName(*code, KeyName);
        /*TRACE2("key1 %d %s\n", i, KeyName);*/
      }
      return TRUE;
    }
  }

  /*TRACE2("key unrecognized: %s %#0x\n", buffer, keysym);*/
  return FALSE;
}

extern BOOLEAN bWaitFirstExpose;

#define SLEEP_TIME  50000  /* Wait for character with timeout 100ms */

/* ************************************************************************
   Function: WaitEvent
   Description:
     Waits for an event.
     In *pnSleepTime return the elapsed time.
   Returns:
     TRUE event awaits.
     FALSE the predetermined time out expired and no event arrived.
*/
BOOLEAN WaitEvent(int *pnSleepTime)
{
  fd_set rset;
  struct timeval tv;
  int nSelect;
  int nConnectionNumber;

  *pnSleepTime = 0;
  if (XPending(x11_disp))
    return TRUE;

  FD_ZERO(&rset);
  nConnectionNumber = ConnectionNumber(x11_disp);
  FD_SET(nConnectionNumber, &rset);

  tv.tv_sec = 0;
  tv.tv_usec = SLEEP_TIME;

  nSelect = select(nConnectionNumber + 1, &rset, NULL, NULL, &tv);

  if (nSelect == -1)
    return FALSE;

  /* Calculate the elapsed time */
  *pnSleepTime = SLEEP_TIME;
  if (nSelect > 0)
    *pnSleepTime = tv.tv_usec;

  #ifdef LINUX
  /* Calculate the elapsed time */
  *pnSleepTime = SLEEP_TIME - tv.tv_usec;
  #else
  *pnSleepTime = SLEEP_TIME;  /* Not very precise but would be enough */
  #endif

  return nSelect > 0;
}

BOOLEAN kbhit(int *pnSleepTime)
{
  return WaitEvent(pnSleepTime);
}

/* ************************************************************************
   Function: disp_x11_maximized
   Description:
     Checks if the window is in maximized state
*/
BOOLEAN
disp_x11_maximized(const struct disp_ctx *disp)
{
  Atom type;
  int format;
  unsigned long bytes_after;
  unsigned long num_items;
  Atom *atoms = NULL;
  unsigned char *data;
  int i;
  static Atom atom_net_wm_state = 0;
  static Atom atom_maxvert = 0;
  static Atom atom_maxhorz = 0;
  BOOLEAN maximized_vert;
  BOOLEAN maximized_horz;

  if (atom_net_wm_state == 0)
    atom_net_wm_state = disp_get_atom(disp, "_NET_WM_STATE");
  if (atom_maxvert == 0)
    atom_maxvert = disp_get_atom(disp, "_NET_WM_STATE_MAXIMIZED_VERT");
  if (atom_maxhorz == 0)
    atom_maxhorz = disp_get_atom(disp, "_NET_WM_STATE_MAXIMIZED_HORZ");

  XGetWindowProperty(disp->x11_disp, disp->h_win, atom_net_wm_state,
    0, 4096, False, XA_ATOM, &type, &format, &num_items,&bytes_after,&data);

  maximized_vert = FALSE;
  maximized_horz = FALSE;
  if (type != None)
  {
    atoms = (Atom *)data;
    for (i = 0; i < num_items; ++i)
    {
      if (atoms[i] == atom_maxvert)
        maximized_vert = TRUE;
      if (atoms[i] == atom_maxhorz)
        maximized_horz = TRUE;
    }

    XFree(atoms);
  }

  return maximized_vert && maximized_horz;
}

/* ************************************************************************
   Function: ReadEvent
   Description:
     Waits for an event from the console.
   When returns 0xffff nRecoveryTime expired.
*/
void ReadEvent(struct event *pEvent)
{
  int x;
  int y;
  int x2;
  int y2;
  XEvent e;
  int nSecond;  /* Counts a second here */
  int nSleepTime;  /* Time elapsed in WaitEvent */
  Atom sel_type;
  Atom prop;
  Atom utf8;
  int sel_format;
  unsigned long sel_len;
  unsigned long sel_bytes_left;
  unsigned char *sel_data;
  int result;
  XEvent respond;
  int new_height;
  int new_width;
  extern struct disp_ctx disp_ctx_data;
  int scaled_x;
  int scaled_y;

  pEvent->t.code = EVENT_NONE;
  nSecond = 0;

_loop:
  while (!WaitEvent(&nSleepTime))
  {
    nSecond += nSleepTime;
    if (nSecond >= 1000000)  /* 1000 milliseconds */
    {
      nSecond = 0;  /* Clear the _one_second_ counter */
      /* Increase the global time elapsed */
      ++nGlobalSeconds;
      if (nGlobalSeconds >= 60)
      {
	++nGlobalMinutes;
        nGlobalSeconds = 0;
	if (nGlobalMinutes >= 60)
	{
	  ++nGlobalHours;
          nGlobalMinutes = 0;
	}
      }

      if (nRecoveryTime > 0 && ++nTimeElapsed % nRecoveryTime == 0)
      {
        pEvent->t.code = EVENT_RECOVERY_TIMER_EXPIRED;
        return;
      }
      continue;
    }
  }

  XNextEvent(x11_disp, &e);

  switch (e.type)
  {
    case Expose:
      ASSERT(e.xexpose.window == WinHndl);
      ASSERT(e.xexpose.display == x11_disp);
      scaled_x = e.xexpose.x / CharWidth;
      scaled_y = e.xexpose.y / CharHeight;
      x = scaled_x - 1;
      y = scaled_y - 1;
      /* why +2? 1 for character alignment and 1 because */
      /* x2 and y2 are not inclusive                     */
      x2 = scaled_x + e.xexpose.width / CharWidth + 2;
      y2 = scaled_y + e.xexpose.height / CharHeight + 2;
      if (x < 0)
        x = 0;
      if (y < 0)
        y = 0;
      if (x2 > ScreenWidth)
        x2 = ScreenWidth;
      if (y2 > ScreenHeight)
        y2 = ScreenHeight;
      /*TRACE2("expose: x=%d, y=%d", x, y);*/
      /*TRACE2(" x2=%d, y2=%d\n", x2, y2);*/
      /*TRACE0("\n");*/
      disp_ctx_data.wait_first_expose = FALSE;
      disp_on_paint(&disp_ctx_data, x, y, x2, y2);
      disp_draw_cursor(&disp_ctx_data);
      break;

    case PropertyNotify:
      bMaximized = disp_x11_maximized(&disp_ctx_data);
      break;

    case ConfigureNotify:
      /*TRACE2("resize: w=%d, h=%d\n", e.xconfigure.width, e.xconfigure.height);*/
      new_height = e.xconfigure.height / CharHeight;
      new_width = e.xconfigure.width / CharWidth;
      pEvent->t.code = EVENT_RESIZE;
      pEvent->e.stNewSizeEvent.NewX1 = 0;
      pEvent->e.stNewSizeEvent.NewY1 = 0;
      scr_width_before_maximize = ScreenWidth;
      scr_height_before_maximize = ScreenHeight;
      ScreenHeight = new_height;
      ScreenWidth = new_width;
      CurHeight = ScreenHeight - 1;
      CurWidth = ScreenWidth - 1;
      ScreenPosX = e.xconfigure.x;
      ScreenPosY = e.xconfigure.y;
      disp_ctx_data.top_window_height = new_height;
      disp_ctx_data.top_window_width = new_width;
      pEvent->e.stNewSizeEvent.NewWidth = ScreenWidth;
      pEvent->e.stNewSizeEvent.NewHeight = ScreenHeight;
      disp_realloc_buf();
      break;

    case KeyRelease:
      if ((GetShiftState(&e.xkey) & kbCtrl) == 0)  /* We monitor the Ctrl key */
        bCtrlReleased = TRUE;
      break;

    case KeyPress:
      if ((GetShiftState(&e.xkey) & kbCtrl) == 0)  /* We monitor the Ctrl key */
        bCtrlReleased = TRUE;
      if (TranslateKey(&e.xkey, &pEvent->e.nKey))
        pEvent->t.code = EVENT_KEY;  /* We have a recognized key */
      break;

    case SelectionNotify:
      prop = XInternAtom(x11_disp, "WW_SELECTION", FALSE);
      /*TRACE1("event SelectionNotify: prop=%x\n", e.xselection.property);*/
      if (e.xselection.property == prop)  /* is it for the requested prop? */
      {
        XGetWindowProperty(x11_disp,
          e.xselection.requestor, e.xselection.property/*was XA_STRING*/, 0, 0, 0, AnyPropertyType,
      	  &sel_type, &sel_format, &sel_len, &sel_bytes_left, &sel_data);
        if (sel_bytes_left > 0)
        {
          result = XGetWindowProperty(x11_disp,
            e.xselection.requestor, e.xselection.property/*XA_STRING*/, 0, sel_bytes_left, 0,
            AnyPropertyType, &sel_type, &sel_format,
            &sel_len, &sel_bytes_left, &sel_data);
          /*if (result == Success)
            TRACE1("clipboard: %s\n", sel_data);*/
          pEvent->t.code = EVENT_CLIPBOARD_PASTE;
          pEvent->e.pdata = sel_data;
        }
      }
      break;

    case SelectionClear:
      pEvent->t.code = EVENT_CLIPBOARD_CLEAR;
      break;

    case SelectionRequest:
      utf8 = XInternAtom(x11_disp, "UTF8_STRING", FALSE);
      if (e.xselectionrequest.target != XA_STRING
          && e.xselectionrequest.target != utf8)
      {
        respond.xselection.type = SelectionNotify;
	respond.xselection.display = e.xselectionrequest.display;
        respond.xselection.requestor = e.xselectionrequest.requestor;
        respond.xselection.selection = e.xselectionrequest.selection;
        respond.xselection.property = None;
        respond.xselection.target = e.xselectionrequest.target;
        XSendEvent(x11_disp, e.xselectionrequest.requestor, 0, 0, &respond);
        XFlush(x11_disp);
        /*TRACE2("event SelectionRequest: rejected for target=%x (%s)\n",
          e.xselectionrequest.target,
          XGetAtomName(x11_disp, e.xselectionrequest.target));*/
      }
      else
      {
        pEvent->t.code = EVENT_CLIPBOARD_COPY_REQUESTED;
        pEvent->e.p.param1 = e.xselectionrequest.requestor;
        pEvent->e.p.param2 =
          e.xselectionrequest.selection == XA_PRIMARY ? TRUE : FALSE;
        pEvent->e.p.param3 = e.xselectionrequest.property;
        pEvent->e.p.param4 = e.xselectionrequest.time;
        pEvent->e.p.param5 = e.xselectionrequest.target;
        /*TRACE2("event SelectionRequest: requestor=%x, requestor_property=%x, target=%x\n",
          e.xselectionrequest.requestor,
          e.xselectionrequest.property);*/
        /*TRACE2("event SelectionRequest: target=%x (%s)\n",
          e.xselectionrequest.target,
          XGetAtomName(x11_disp, e.xselectionrequest.target));*/
      }
      break;

    default:
      break;
  }

  if (pEvent->t.code == EVENT_NONE)  /* No event is recognized */
    goto _loop;

  return;
}

/* ************************************************************************
   Function: ReadKey
   Description:
     Filters all events out and returns only the key events.
*/
DWORD ReadKey(void)
{
  struct event Event;

  Event.t.code = EVENT_NONE;
  while (Event.t.code != EVENT_KEY && Event.t.code != EVENT_RECOVERY_TIMER_EXPIRED)
    ReadEvent(&Event);
  return Event.e.nKey;
}

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

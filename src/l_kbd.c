/*

File: l_kbd.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 17th January, 2000
Descrition:
  Keyboard manipulation functions.
  Linux

*/

#include "global.h"
#include <curses.h>

#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>  /* For fcntl() */

#include "options.h"
#include "keydefs.h"
#include "kbd.h"

BOOLEAN bCtrlReleased = TRUE;
WORD ShiftState;
int nTimeElapsed = 0;
int nGlobalHours = 0;  /* Global hours elapsed */
int nGlobalMinutes = 0;  /* Global minutes elapsed */
int nGlobalSeconds = 0;  /* Global seconds elapsed */

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

/* ************************************************************************
   Function: GetShiftState
   Description:
     Reads the shift state of the keyboard by using
     a semi-documented ioctl() call the Linux kernel.
*/
static BYTE GetShiftState(void)
{
  int nArg;
  int nShift;

  nArg = 6;  /* TIOCLINUX function #6 */
  nShift = 0;  /* by default */

  #ifdef LINUX
  if (ioctl(fileno(stdin), TIOCLINUX, &nArg) == 0)
    nShift = nArg;
  #endif

  return nShift;
}

#define SLEEP_TIME  20000  /* Wait for character with timeout 25ms */

/* ************************************************************************
   Function: kbhit
   Description:
     Waits for a key.
     In *pnSleepTime return the elapsed time.
   Returns:
     TRUE a key is pressed an waits in stdin.
     FALSE the predetermined time out expired and no key has been pressed.
*/
BOOLEAN kbhit(int *pnSleepTime)
{
  fd_set rset;
  struct timeval tv;
  int nSelect;
  void *tvp;

  FD_ZERO(&rset);
  FD_SET(fileno(stdin), &rset);

  tv.tv_sec = 0;
  tv.tv_usec = SLEEP_TIME;
  tvp = &tv;
  if (pnSleepTime == NULL)
    tvp = NULL;
  else
    *pnSleepTime = 0;

  nSelect = select(fileno(stdin) + 1, &rset, NULL, NULL, &tv);

  if (nSelect == -1)
    return FALSE;

  /* Calculate the elapsed time */
  if (pnSleepTime != NULL)
  {
    *pnSleepTime = SLEEP_TIME;
    if (nSelect > 0)
      *pnSleepTime = tv.tv_usec;

    #ifdef LINUX
    /* Calculate the elapsed time */
    *pnSleepTime = SLEEP_TIME - tv.tv_usec;
    #else
    *pnSleepTime = SLEEP_TIME;  /* Not very precise but would be enough */
    #endif
  }

  return nSelect > 0;
}

typedef struct KeySequence
{
  /* Description of what can be read from the terminal */
  char *sSequence;
  /* Description of how this to be transformed into scancode/shift state pair */
  DWORD nKey;  /* hi word is shift state, lo word is scan code + asci char */
  char *sterm_esc_seq;  /* ESC sequence ID specific for a terminal */
} TKeySequence;

/* The UNIX consoles generate only ASCII symbols
or sequence of symbols. We need to convert those characters into
a Ctrl/Shift/Alt key combination */
TKeySequence FKeys[] =
{
  {"1",         KEY(0,       kb1) | '1'},
  {"!",         KEY(kbShift, kb1) | '!'},
  {"\0331",     KEY(kbAlt,   kb1)},
  {"\xb1",      KEY(kbAlt,   kb1)},  /* hardcoded for xterm */
  {"2",         KEY(0,       kb2) | '2'},
  {"@",         KEY(kbShift, kb2) | '@'},
  {"\0332",     KEY(kbAlt,   kb2)},
  {"\xb2",      KEY(kbAlt,   kb2)},
  {"3",         KEY(0,       kb3) | '3'},
  {"#",         KEY(kbShift, kb3) | '#'},
  {"\0333",     KEY(kbAlt,   kb3)},
  {"\xb3",      KEY(kbAlt,   kb3)},
  {"4",         KEY(0,       kb4) | '4'},
  {"$",         KEY(kbShift, kb4) | '$'},
  {"\0334",     KEY(kbAlt,   kb4)},
  {"\xb4",      KEY(kbAlt,   kb4)},
  {"5",         KEY(0,       kb5) | '5'},
  {"%",         KEY(kbShift, kb5) | '%'},
  {"\0335",     KEY(kbAlt,   kb5)},
  {"\xb5",      KEY(kbAlt,   kb5)},
  {"6",         KEY(0,       kb6) | '6'},
  {"^",         KEY(kbShift, kb6) | '^'},
  {"\0336",     KEY(kbAlt,   kb6)},
  {"\xb6",      KEY(kbAlt,   kb6)},
  {"7",         KEY(0,       kb7) | '7'},
  {"&",         KEY(kbShift, kb7) | '&'},
  {"\0337",     KEY(kbAlt,   kb7)},
  {"\xb7",      KEY(kbAlt,   kb7)},
  {"8",         KEY(0,       kb8) | '8'},
  {"*",         KEY(kbShift, kb8) | '*'},
  {"\0338",     KEY(kbAlt,   kb8)},
  {"\xb8",      KEY(kbAlt,   kb8)},
  {"9",         KEY(0,       kb9) | '9'},
  {"(",         KEY(kbShift, kb9) | '('},
  {"\0339",     KEY(kbAlt,   kb9)},
  {"\xb9",      KEY(kbAlt,   kb9)},
  {"0",         KEY(0,       kb0) | '0'},
  {")",         KEY(kbShift, kb0) | ')'},
  {"\0330",     KEY(kbAlt,   kb0)},
  {"\xb0",      KEY(kbAlt,   kb0)},
  {"-",         KEY(0,       kbMinus) | '-'},
  {"_",         KEY(kbShift, kbMinus) | '_'},
  {"\033-",     KEY(kbAlt,   kbMinus)},
  {"\xad",      KEY(kbAlt,   kbMinus)},
  {"=",         KEY(0,       kbEqual) | '='},
  {"+",         KEY(kbShift, kbEqual) | '+'},
  {"\033=",     KEY(kbAlt,   kbEqual)},
  {"\xbd",      KEY(kbAlt,   kbEqual)},
  {"\x7f",      KEY(0,       kbBckSpc) /*| '\x7f'*/, "kbs"},
  {"\033\x7f",  KEY(kbAlt,   kbBckSpc)},
  {"\x88",      KEY(kbAlt,   kbBckSpc)},
  {"\x09",      KEY(0,       kbTab) /*| '\x09'*/},
  {"q",         KEY(0,       kbQ) | 'q'},
  {"Q",         KEY(kbShift, kbQ) | 'Q'},
  {"\033q",     KEY(kbAlt,   kbQ)},
  {"\xf1",      KEY(kbAlt,   kbQ)},
  {"w",         KEY(0,       kbW) | 'w'},
  {"W",         KEY(kbShift, kbW) | 'W'},
  {"\033w",     KEY(kbAlt,   kbW)},
  {"\xf7",      KEY(kbAlt,   kbW)},
  {"e",         KEY(0,       kbE) | 'e'},
  {"E",         KEY(kbShift, kbE) | 'E'},
  {"\033e",     KEY(kbAlt,   kbE)},
  {"\xe5",      KEY(kbAlt,   kbE)},
  {"r",         KEY(0,       kbR) | 'r'},
  {"R",         KEY(kbShift, kbR) | 'R'},
  {"\033r",     KEY(kbAlt,   kbR)},
  {"\xf2",      KEY(kbAlt,   kbR)},
  {"t",         KEY(0,       kbT) | 't'},
  {"T",         KEY(kbShift, kbT) | 'T'},
  {"\033t",     KEY(kbAlt,   kbT)},
  {"\xf4",      KEY(kbAlt,   kbT)},
  {"y",         KEY(0,       kbY) | 'y'},
  {"Y",         KEY(kbShift, kbY) | 'Y'},
  {"\033y",     KEY(kbAlt,   kbY)},
  {"\xf9",      KEY(kbAlt,   kbY)},
  {"u",         KEY(0,       kbU) | 'u'},
  {"U",         KEY(kbShift, kbU) | 'U'},
  {"\033u",     KEY(kbAlt,   kbU)},
  {"\xf5",      KEY(kbAlt,   kbU)},
  {"i",         KEY(0,       kbI) | 'i'},
  {"I",         KEY(kbShift, kbI) | 'I'},
  {"\033i",     KEY(kbAlt,   kbI)},
  {"\xe9",      KEY(kbAlt,   kbI)},
  {"o",         KEY(0,       kbO) | 'o'},
  {"O",         KEY(kbShift, kbO) | 'O'},
  {"\033o",     KEY(kbAlt,   kbO)},
  {"\xef",      KEY(kbAlt,   kbO)},
  {"p",         KEY(0,       kbP) | 'p'},
  {"P",         KEY(kbShift, kbP) | 'P'},
  {"\033p",     KEY(kbAlt,   kbP)},
  {"\xf0",      KEY(kbAlt,   kbP)},
  {"[",         KEY(0,       kbLBrace) | '['},
  {"{",         KEY(kbShift, kbLBrace) | '{'},
  {"\xdb",      KEY(kbAlt,   kbLBrace)},
/*
  This is comented out as "\033[" is a start of all the
  function key character sequences.
  {"\033[",     KEY(kbAlt,   kbLBrace)},
*/
  {"]",         KEY(0,       kbRBrace) | ']'},
  {"}",         KEY(kbShift, kbRBrace) | '}'},
  {"\033]",     KEY(kbAlt,   kbRBrace)},
  {"\xdd",      KEY(kbAlt,   kbRBrace)},
  {"\x0d",      KEY(0,       kbEnter) /*| '\x0d'*/},
  {"\033\x0d",  KEY(kbAlt,   kbEnter)},
  {"\x8d",      KEY(kbAlt,   kbEnter)},
  {"a",         KEY(0,       kbA) | 'a'},
  {"A",         KEY(kbShift, kbA) | 'A'},
  {"\033a",     KEY(kbAlt,   kbA)},
  {"\xe1",      KEY(kbAlt,   kbA)},
  {"s",         KEY(0,       kbS) | 's'},
  {"S",         KEY(kbShift, kbS) | 'S'},
  {"\033s",     KEY(kbAlt,   kbS)},
  {"\xf3",      KEY(kbAlt,   kbS)},
  {"d",         KEY(0,       kbD) | 'd'},
  {"D",         KEY(kbShift, kbD) | 'D'},
  {"\033d",     KEY(kbAlt,   kbD)},
  {"\xe4",      KEY(kbAlt,   kbD)},
  {"f",         KEY(0,       kbF) | 'f'},
  {"F",         KEY(kbShift, kbF) | 'F'},
  {"\033f",     KEY(kbAlt,   kbF)},
  {"\xe6",      KEY(kbAlt,   kbF)},
  {"g",         KEY(0,       kbG) | 'g'},
  {"G",         KEY(kbShift, kbG) | 'G'},
  {"\033g",     KEY(kbAlt,   kbG)},
  {"\xe7",      KEY(kbAlt,   kbG)},
  {"h",         KEY(0,       kbH) | 'h'},
  {"H",         KEY(kbShift, kbH) | 'H'},
  {"\033h",     KEY(kbAlt,   kbH)},
  {"\xe8",      KEY(kbAlt,   kbH)},
  {"j",         KEY(0,       kbJ) | 'j'},
  {"J",         KEY(kbShift, kbJ) | 'J'},
  {"\033j",     KEY(kbAlt,   kbJ)},
  {"\xea",      KEY(kbAlt,   kbJ)},
  {"k",         KEY(0,       kbK) | 'k'},
  {"K",         KEY(kbShift, kbK) | 'K'},
  {"\033k",     KEY(kbAlt,   kbK)},
  {"\xeb",      KEY(kbAlt,   kbK)},
  {"l",         KEY(0,       kbL) | 'l'},
  {"L",         KEY(kbShift, kbL) | 'L'},
  {"\033l",     KEY(kbAlt,   kbL)},
  {"\xec",      KEY(kbAlt,   kbL)},
  {";",         KEY(0,       kbColon) | ';'},
  {":",         KEY(kbShift, kbColon) | ':'},
  {"\033;",     KEY(kbAlt,   kbColon)},
  {"\xbb",      KEY(kbAlt,   kbColon)},
  {"\"",        KEY(0,       kb_1) | '\"'},
  {"\'",        KEY(kbShift, kb_1) | '\''},
  {"\033\x27",  KEY(kbAlt,   kb_1)},
  {"\xa7",      KEY(kbAlt,   kb_1)},
  {"\\",        KEY(0,       kb_2) | '\\'},
  {"|",         KEY(kbShift, kb_2) | '|'},
  {"\033\\",    KEY(kbAlt,   kb_2)},
 {"\xdc",       KEY(kbAlt,   kb_2)},
/*
  {"\x3c",      KEY(0,       kbBSlash) | '\x3c'},
  {"\x3e",      KEY(kbShift, kbBSlash) | '\x3e'},
  {"\033\x3c",  KEY(kbAlt,   kbBSlash)},
*/
  {"z",         KEY(0,       kbZ) | 'z'},
  {"Z",         KEY(kbShift, kbZ) | 'Z'},
  {"\033z",     KEY(kbAlt,   kbZ)},
  {"\xfa",      KEY(kbAlt,   kbZ)},
  {"x",         KEY(0,       kbX) | 'x'},
  {"X",         KEY(kbShift, kbX) | 'X'},
  {"\033x",     KEY(kbAlt,   kbX)},
  {"\xf8",      KEY(kbAlt,   kbX)},
  {"c",         KEY(0,       kbC) | 'c'},
  {"C",         KEY(kbShift, kbC) | 'C'},
  {"\033c",     KEY(kbAlt,   kbC)},
  {"\xe3",      KEY(kbAlt,   kbC)},
  {"v",         KEY(0,       kbV) | 'v'},
  {"V",         KEY(kbShift, kbV) | 'V'},
  {"\033v",     KEY(kbAlt,   kbV)},
  {"\xf6",      KEY(kbAlt,   kbV)},
  {"b",         KEY(0,       kbB) | 'b'},
  {"B",         KEY(kbShift, kbB) | 'B'},
  {"\033b",     KEY(kbAlt,   kbB)},
  {"\xe2",      KEY(kbAlt,   kbB)},
  {"n",         KEY(0,       kbN) | 'n'},
  {"N",         KEY(kbShift, kbN) | 'N'},
  {"\033n",     KEY(kbAlt,   kbN)},
  {"\ee",       KEY(kbAlt,   kbN)},
  {"m",         KEY(0,       kbM) | 'm'},
  {"M",         KEY(kbShift, kbM) | 'M'},
  {"\033m",     KEY(kbAlt,   kbM)},
  {"\xed",      KEY(kbAlt,   kbM)},
  {",",         KEY(0,       kbComa) | ','},
  {"<",         KEY(kbShift, kbComa) | '<'},
  {"\033,",     KEY(kbAlt,   kbComa)},
  {"\xac",      KEY(kbAlt,   kbComa)},
  {".",         KEY(0,       kbPeriod) | '.'},
  {">",         KEY(kbShift, kbPeriod) | '>'},
  {"\033.",     KEY(kbAlt,   kbPeriod)},
  {"\xae",      KEY(kbAlt,   kbPeriod)},
  {"/",         KEY(0,       kbSlash) | '/'},
  {"?",         KEY(kbShift, kbSlash) | '?'},
  {"\033/",     KEY(kbAlt,   kbSlash)},
  {"\xaf",      KEY(kbAlt,   kbSlash)},
  {" ",         KEY(0,       kbSpace) | ' '},
  {"\033 ",     KEY(kbAlt,   kbSpace)},
  {"\xa0 ",     KEY(kbAlt,   kbSpace)},

  {"\033[[A", KEY(0, kbF1), "kf1"},
  {"\033[[B", KEY(0, kbF2), "kf2"},
  {"\033[[C", KEY(0, kbF3), "kf3"},
  {"\033[[D", KEY(0, kbF4), "kf4"},
  {"\033[[E", KEY(0, kbF5), "kf5"},
  {"\033[17~", KEY(0, kbF6), "kf6"},
  {"\033[18~", KEY(0, kbF7), "kf7"},
  {"\033[19~", KEY(0, kbF8), "kf8"},
  {"\033[20~", KEY(0, kbF9), "kf9"},
  {"\033[21~", KEY(0, kbF10), "kf10"},
  {"\033\x5b\x32\x33\x7e", KEY(0, kbF11), "kf11"},
  {"\033\x5b\x32\x34\x7e", KEY(0, kbF12), "kf12"},
  {"\033[1~", KEY(0, kbHome), "khome"},
  {"\033[2~", KEY(0, kbIns), "kich1"},
  {"\033[3~", KEY(0, kbDel), "kdch1"},
  {"\033[4~", KEY(0, kbEnd), "kend"},
  {"\033[5~", KEY(0, kbPgUp), "kpp"},
  {"\033[6~", KEY(0, kbPgDn), "knp"},
  {"\033[M", 0x7f},  /* Macro */
  {"\033[P", 0x7f},  /* Pause */

  {"\033\x5b\x41", KEY(0, kbUp), "kcuu1"},
  {"\033\x5b\x42", KEY(0, kbDown), "kcud1"},
  {"\033\x5b\x44", KEY(0, kbLeft), "kcub1"},
  {"\033\x5b\x43", KEY(0, kbRight), "kcuf1"},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x41", KEY(0, kbUp)},
  {"\033\x5b\x42", KEY(0, kbDown)},
  {"\033\x5b\x44", KEY(0, kbLeft)},
  {"\033\x5b\x43", KEY(0, kbRight)},
  {"\033\x5b\x48", KEY(0, kbHome)},
  {"\033\x5b\x46", KEY(0, kbEnd)},
  
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x35\x48", KEY(kbCtrl, kbHome)},
  {"\033\x5b\x31\x3b\x35\x46", KEY(kbCtrl, kbEnd)},
  {"\033\x5b\x35\x3b\x35\x7e", KEY(kbCtrl, kbPgUp)},
  {"\033\x5b\x36\x3b\x35\x7e", KEY(kbCtrl, kbPgDn)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x32\x48", KEY(kbShift, kbHome)},
  {"\033\x5b\x31\x3b\x32\x46", KEY(kbShift, kbEnd)},
  {"\033\x5b\x35\x3b\x32\x7e", KEY(kbShift, kbPgUp)},
  {"\033\x5b\x36\x3b\x32\x7e", KEY(kbShift, kbPgDn)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x36\x48", KEY(kbCtrl+kbShift, kbHome)},
  {"\033\x5b\x31\x3b\x36\x46", KEY(kbCtrl+kbShift, kbEnd)},
  {"\033\x5b\x35\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbPgUp)},
  {"\033\x5b\x36\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbPgDn)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x32\x41", KEY(kbShift, kbUp)},
  {"\033\x5b\x31\x3b\x32\x42", KEY(kbShift, kbDown)},
  {"\033\x5b\x31\x3b\x32\x44", KEY(kbShift, kbLeft)},
  {"\033\x5b\x31\x3b\x32\x43", KEY(kbShift, kbRight)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x35\x41", KEY(kbCtrl, kbUp)},
  {"\033\x5b\x31\x3b\x35\x42", KEY(kbCtrl, kbDown)},
  {"\033\x5b\x31\x3b\x35\x44", KEY(kbCtrl, kbLeft)},
  {"\033\x5b\x31\x3b\x35\x43", KEY(kbCtrl, kbRight)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x3b\x36\x41", KEY(kbCtrl+kbShift, kbUp)},
  {"\033\x5b\x31\x3b\x36\x42", KEY(kbCtrl+kbShift, kbDown)},
  {"\033\x5b\x31\x3b\x36\x44", KEY(kbCtrl+kbShift, kbLeft)},
  {"\033\x5b\x31\x3b\x36\x43", KEY(kbCtrl+kbShift, kbRight)},

  /* xterm reports sequences, we need those hard coded here */
  {"\033\x4f\x32\x50", KEY(kbShift,kbF1)},
  {"\033\x4f\x35\x50", KEY(kbCtrl,kbF1)},
  {"\033\x4f\x33\x50", KEY(kbAlt,kbF1)},
  {"\033\x4f\x36\x50", KEY(kbCtrl+kbShift,kbF1)},
  {"\033\x4f\x34\x50", KEY(kbAlt+kbShift,kbF1)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x4f\x32\x51", KEY(kbShift,kbF2)},
  {"\033\x4f\x35\x51", KEY(kbCtrl,kbF2)},
  {"\033\x4f\x33\x51", KEY(kbAlt,kbF2)},
  {"\033\x4f\x36\x51", KEY(kbCtrl+kbShift,kbF2)},
  {"\033\x4f\x34\x51", KEY(kbAlt+kbShift,kbF2)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x4f\x32\x52", KEY(kbShift,kbF3)},
  {"\033\x4f\x35\x52", KEY(kbCtrl,kbF3)},
  {"\033\x4f\x33\x52", KEY(kbAlt,kbF3)},
  {"\033\x4f\x36\x52", KEY(kbCtrl+kbShift,kbF3)},
  {"\033\x4f\x34\x52", KEY(kbAlt+kbShift,kbF3)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x4f\x32\x53", KEY(kbShift,kbF4)},
  {"\033\x4f\x35\x53", KEY(kbCtrl,kbF4)},
  {"\033\x4f\x33\x53", KEY(kbAlt,kbF4)},
  {"\033\x4f\x36\x53", KEY(kbCtrl+kbShift,kbF4)},
  {"\033\x4f\x34\x53", KEY(kbAlt+kbShift,kbF4)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x35\x3b\x32\x7e", KEY(kbShift, kbF5)},
  {"\033\x5b\x31\x35\x3b\x35\x7e", KEY(kbCtrl, kbF5)},
  {"\033\x5b\x31\x35\x3b\x33\x7e", KEY(kbAlt, kbF5)},
  {"\033\x5b\x31\x35\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF5)},
  {"\033\x5b\x31\x35\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF5)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x37\x3b\x32\x7e", KEY(kbShift, kbF6)},
  {"\033\x5b\x31\x37\x3b\x35\x7e", KEY(kbCtrl, kbF6)},
  {"\033\x5b\x31\x37\x3b\x33\x7e", KEY(kbAlt, kbF6)},
  {"\033\x5b\x31\x37\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF6)},
  {"\033\x5b\x31\x37\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF6)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x38\x3b\x32\x7e", KEY(kbShift, kbF7)},
  {"\033\x5b\x31\x38\x3b\x35\x7e", KEY(kbCtrl, kbF7)},
  {"\033\x5b\x31\x38\x3b\x33\x7e", KEY(kbAlt, kbF7)},
  {"\033\x5b\x31\x38\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF7)},
  {"\033\x5b\x31\x38\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF7)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x39\x3b\x32\x7e", KEY(kbShift, kbF8)},
  {"\033\x5b\x31\x39\x3b\x35\x7e", KEY(kbCtrl, kbF8)},
  {"\033\x5b\x31\x39\x3b\x33\x7e", KEY(kbAlt, kbF8)},
  {"\033\x5b\x31\x39\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF8)},
  {"\033\x5b\x31\x39\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF8)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x30\x3b\x32\x7e", KEY(kbShift, kbF9)},
  {"\033\x5b\x31\x30\x3b\x35\x7e", KEY(kbCtrl, kbF9)},
  {"\033\x5b\x31\x30\x3b\x33\x7e", KEY(kbAlt, kbF9)},
  {"\033\x5b\x31\x30\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF9)},
  {"\033\x5b\x31\x30\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF9)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x31\x3b\x32\x7e", KEY(kbShift, kbF10)},
  {"\033\x5b\x31\x31\x3b\x35\x7e", KEY(kbCtrl, kbF10)},
  {"\033\x5b\x31\x31\x3b\x33\x7e", KEY(kbAlt, kbF10)},
  {"\033\x5b\x31\x31\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF10)},
  {"\033\x5b\x31\x31\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF10)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x33\x3b\x32\x7e", KEY(kbShift, kbF11)},
  {"\033\x5b\x31\x33\x3b\x35\x7e", KEY(kbCtrl, kbF11)},
  {"\033\x5b\x31\x33\x3b\x33\x7e", KEY(kbAlt, kbF11)},
  {"\033\x5b\x31\x33\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF11)},
  {"\033\x5b\x31\x33\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF11)},
  /* xterm reports sequences, we need those hard coded here */
  {"\033\x5b\x31\x34\x3b\x32\x7e", KEY(kbShift, kbF12)},
  {"\033\x5b\x31\x34\x3b\x35\x7e", KEY(kbCtrl, kbF12)},
  {"\033\x5b\x31\x34\x3b\x33\x7e", KEY(kbAlt, kbF12)},
  {"\033\x5b\x31\x34\x3b\x36\x7e", KEY(kbCtrl+kbShift, kbF12)},
  {"\033\x5b\x31\x34\x3b\x34\x7e", KEY(kbAlt+kbShift, kbF12)},

  {"\x01",      KEY(kbCtrl,  kbA) | '\x01'},
  {"\x02",      KEY(kbCtrl,  kbB) | '\x02'},
  {"\x03",      KEY(kbCtrl,  kbC) | '\x03'},
  {"\x04",      KEY(kbCtrl,  kbD) | '\x04'},
  {"\x05",      KEY(kbCtrl,  kbE) | '\x05'},
  {"\x06",      KEY(kbCtrl,  kbF) | '\x06'},
  {"\x07",      KEY(kbCtrl,  kbG) | '\x07'},
  {"\x08",      KEY(0, kbBckSpc) /*KEY(kbCtrl,  kbH) | '\x08'*/}, /* some xterms */
  {"\033[[F",   KEY(kbCtrl,  kbI) | '\x09'},  /* redefined by loadkeys */
  {"\x0a",      KEY(kbCtrl,  kbJ) | '\x0a'},
  {"\x0b",      KEY(kbCtrl,  kbK) | '\x0b'},
  {"\x0c",      KEY(kbCtrl,  kbL) | '\x0c'},
  /*
  {"\x0d",      KEY(kbCtrl,  kbM) | '\x0d'},
  */
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
  {"\033[[G",   KEY(kbCtrl,  kbLBrace) | '\x1b'}  /* redefined by laodkeys */
};

void GetNCursesKeys(void)
{
  int i;
  char *s;

  for (i = 0; i < _countof(FKeys); ++i)
  {
    if (FKeys[i].sterm_esc_seq != NULL)
    {
      s = tigetstr(FKeys[i].sterm_esc_seq);
      if ((int)s != -1 && (int)s != 0)
      {
        #if 0
        char sKeyName[35];
        char *c;

        c = s;
        GetKeyName(FKeys[i].nKey, sKeyName);
        TRACE1("%s: ", sKeyName);
        while (*c != '\0')
          TRACE1("%x ", *c++);
        TRACE0("\n");
        #endif
        FKeys[i].sSequence = s;
      }
    }
  }
}

/*
Table of keys to be supplied 'loadkeys'
control	keycode  15 = Tab
control	shift keycode  15 = Tab
control keycode  23 = F30
control keycode  26 = F31
string F30 = "\033[[F"
string F31 = "\033[[G"
*/

#define MAX_KBUF 50

BOOLEAN bTraceKbd = FALSE;  /* toggled from the diag term in diag.c */
extern void PrintString(const char *fmt, ...);  /* from filecmd.c */

typedef struct _queue
{
  int head;
  int tail;
  char kbuf[MAX_KBUF * 4];
} TQueue;

static TQueue kbuf;
static TQueue cbuf;

static void kbuf_put(TQueue *q, char key)
{
  q->kbuf[q->tail] = key;
  ++q->tail;
  if (q->tail == MAX_KBUF)
    q->tail = 0;
}

static char kbuf_get(TQueue *q)
{
  DWORD x;

  x = q->kbuf[q->head];
  ++q->head;
  if (q->head == MAX_KBUF)
    q->head = 0;
  return x;
}

static int kbuf_calcfree(TQueue *q)
{
  int f;

  if (q->tail >= q->head)
    f = q->tail - q->head;
  else /* tail wrapped */
    f = q->tail + (MAX_KBUF - q->head);

  return (MAX_KBUF - f - 1);  /* -1 as tail never steps over head */
}

static int kbuf_calcoccupied(TQueue *q)
{
  int f;

  if (q->tail >= q->head)
    f = q->tail - q->head;
  else /* tail wrapped */
    f = q->tail + (MAX_KBUF - q->head);

  return f;  /* -1 as tail never steps over head */
}

static BOOLEAN kbuf_isempty(TQueue *q)
{
  return (kbuf_calcoccupied(q) == 0);
}

static DWORD kbuf_getdword(TQueue *q)
{
  DWORD c1;
  DWORD c2;
  DWORD c3;
  DWORD c4;

  c1 = kbuf_get(q);
  c2 = kbuf_get(q) << 8;
  c3 = kbuf_get(q) << 16;
  c4 = kbuf_get(q) << 24;

  return c1 | c2 | c3 | c4;
}

static void kbuf_putdword(TQueue *q, DWORD key)
{
  char c1;
  char c2;
  char c3;
  char c4;

  c1 = key & (0x000000ff);
  c2 = (key & (0x0000ff00)) >> 8;
  c3 = (key & (0x00ff0000)) >> 16;
  c4 = (key & (0xff000000)) >> 24;

  kbuf_put(q, c1);
  kbuf_put(q, c2);
  kbuf_put(q, c3);
  kbuf_put(q, c4);
}

static int kbuf_calcfreedword(TQueue *q)
{
  return kbuf_calcfree(q) / 4;
}

/* ************************************************************************
   Function: ReadEvent
   Description:
     Text-only linux console -> only one event: keyboard
*/
void ReadEvent(struct event *pEvent)
{
  DWORD Key;

  memset(pEvent, 0, sizeof(struct event));
  Key = ReadKey();
  pEvent->t.code = EVENT_KEY;
  pEvent->e.nKey = Key;
}

/* ************************************************************************
   Function: TraceKbd
   Description:
*/
void TraceKbd(const char * fmt, ...)
{
  char buf[256];

  if (!bTraceKbd)
    return;

  va_list ap;
  va_start(ap, fmt);

  vsnprintf(buf, sizeof(buf), fmt, ap);
  buf[sizeof(buf)-1] = 0;

  va_end( ap );

  PrintString(buf);
}

/* ************************************************************************
   Function: ReadKey
   Description:
     ReadKey() could have used getch() from ncurses library. Alas, this
     library follows some arcane rules that ESC is not ESC, but ESC:ESC is
     single ESC. This is being irritated stimulated me to write
     a sibstitution.
     Dumb rule2: A key is a sequence of characters that start with ESC!
     Dumb rule3: The sequence may be in between 1 up to 5 characters.
     Dumb rule4: If you do select() on stdin and select() unblocks,
       read as much as possible characters, as it may occure that
       select() is blocked and there are characters lefts from the
       previous unblock.

     In order to accomodate these "rules" we need two queue.

     First queue is a character queue that stores all the incoming
     characters from a single fread().
     Then we have a matchin loop that collects key sequences and
     stores the matching kye in a DWORD queue. This is the
     second queue. The loop ends when all tha characters are
     processed, this may result in a numerous "pushes" in the
     DWORD queue.
     We return from the function only upon time-out. Then
     we return a key from the DWORD queue.
*/
DWORD ReadKey(void)
{
  char sKeyBuf[MAX_KEY_SEQUENCE + 1];  /* Collect a key sequence here */
  BYTE c;
  BYTE tmp_buf[MAX_KBUF - 1];
  int nLen = 0;
  int nSeqTime;  /* Counts the time inbetween 2 keys */
  int nSecond;  /* Counts a second here */
  int nSleepTime;  /* Time elapsed in kbhit() */
  int i;
  int nRead;
  BOOLEAN bShown;
  DWORD key;
  extern int b_refresh;
  char *p;

  nSecond = 0;
  nSeqTime = 0;

_clear:
  nLen = 0;
  sKeyBuf[0] = '\0';

  /* Keep returning keys buffered from previous calls */
  if (!kbuf_isempty(&kbuf))
  {
    key = kbuf_getdword(&kbuf);
    return key;
  }

  goto _pop_c;  /* process any unprocessed chars from last time */

_wait_key:
  if (!kbuf_isempty(&kbuf) && nLen == 0)
  {
    key = kbuf_getdword(&kbuf);
    if (nLen != 0)
      TraceKbd("[WARN1] ");
    return key;
  }

  if (b_refresh)
    VERIFY(refresh() != ERR);
  b_refresh = FALSE;

  nSeqTime = 0;
  while (!kbhit(&nSleepTime))
  {
    if ((GetShiftState() & kbCtrl) == 0)
      bCtrlReleased = TRUE;

    /*
    No more keys wait in the OS buffer to be processed,
    so if we have anything buffered we can return
    one of the keys
    */
    if (!kbuf_isempty(&kbuf) && nLen == 0)
    {
      key = kbuf_getdword(&kbuf);
      if (nLen != 0)
        TraceKbd("[WARN2] ");
      return key;
    }

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
        if (nLen != 0)
          TraceKbd("[WARN3] ");
	return (0xffff);
      }
      else
	continue;
    }

    nSeqTime += nSleepTime;
    if (nSeqTime > KEY_TIMEOUT)
    {
      if (nLen > 0)
        TraceKbd("[timeout] ");
      bShown = TRUE;
      if (nLen == 1)
      {
        if (kbuf_calcfreedword(&kbuf) > 0)
        {
          if (sKeyBuf[0] == '\x1b')
          {
            TraceKbd("SINGLE ESC HERE1\n");
            kbuf_putdword(&kbuf, KEY(0, kbEsc));
          }
          else
          {
            TraceKbd("a key\n");
            kbuf_putdword(&kbuf, KEY(0, sKeyBuf[0]));  /* single ESC maybe */
          }
        }
        else
          TraceKbd("no space to store a key. scrapped!\n");
        TraceKbd("[c1] ");
        goto _clear;
      }
      if (nLen != 0)
      {
        TraceKbd("key sequence interrupted. scrapped!\n");
        TraceKbd("[c2] ");
        goto _clear;  /* Time out in between 2 keys expired */
      }
      goto _wait_key;
    }
  }

  nSeqTime = 0;  /* we have a key, reset the time-out counter */

  /*
  IMPORTANT: Don't uncomment fread()! It's an example of a great
  problem which took 2-3 years to diagnose properly and fix -- when
  under ncurses don' use direct file i/o to handle the keyboard.
  NOW:
  1. Use kbhit() to detect that a key is pending.
  2. extract keys one by one
  */
  #if 0
  nRead = fread(tmp_buf, 1, sizeof(tmp_buf), stdin);
  if (nRead == 0)
  {
    TraceKbd("\t\t*** WARNING 0 chars *** \n");
    TraceKbd("[w2] ");
    goto _wait_key;  /* This will check for more than 1 key in the buffer */
  }
  #endif
  nRead = 0;
  while (kbhit(NULL) && nRead < sizeof(tmp_buf))
  {
    tmp_buf[nRead++] = getch();
    nSeqTime = 0;  /* we have a key, reset the time-out counter */
  }

  #if 0  /* uncomment for diagnosing kbd problems */
  for (i = 0; i < nRead; ++i)
    TraceKbd("to buf: %x\n", tmp_buf[i]);
  #endif
  for (i = 0; i < nRead; ++i)
  {
    if (kbuf_calcfree(&cbuf) > 0)
      kbuf_put(&cbuf, tmp_buf[i]);
    else
      TraceKbd("\t\t*** WARNING  chars lost !*** \n");
  }

_pop_c:
  if (kbuf_calcoccupied(&cbuf) == 0)  /* no more chars to process? */
  {
    TraceKbd("[w3] ");
    goto _wait_key;
  }
  if (kbuf_calcfreedword(&kbuf) == 0)  /* space to push more keys? */
  {
    TraceKbd("[w4] ");
    goto _wait_key;  /* actually return keys to the application */
  }

  c = kbuf_get(&cbuf);
  if (nLen == 0)
    TraceKbd("\n");
  TraceKbd("[p %x] ", c);
  if (nLen == MAX_KEY_SEQUENCE)
  {
    TraceKbd("The received sequence is too long\n");
    nLen = 0; /* The received sequence is too long */
  }
  /* Rule: we can have 0x1b (ESC) only at the start */
  if (c == '\x1b')
  {
    if (nLen != 0)
    {
      /* New sequence here, we didn't recognize the collected up to here
      so scrap it! */
      nLen = 0;
      TraceKbd("unrecognized sequence. scrapped!\n");
    }
  }
  sKeyBuf[nLen] = c;
  ++nLen;
  sKeyBuf[nLen] = '\0';

  /* Scan the predefined array os strings to match a sequence */
  for (i = 0; i < _countof(FKeys); ++i)
  {
    if (strncmp(sKeyBuf, FKeys[i].sSequence, nLen) == 0)
    {
      if (strlen(FKeys[i].sSequence) == nLen)
      {
        TraceKbd("[match] ", sKeyBuf);
        ShiftState = GetShiftState();  /* Global variable to ref. the state */
        key =  FKeys[i].nKey | (ShiftState << 16);

        ASSERT(kbuf_calcfreedword(&kbuf) != 0);  /* output buf mustn't be full? */

        kbuf_putdword(&kbuf, key);
        nLen = 0;  /* start new sequence */
        sKeyBuf[0] = '\0';
      }
      goto _pop_c;  /* Partial match: keep on collecting keys */
    }
  }

  TraceKbd("Unrecognized sequence\n");
  /* Below we have a test dump of what we have collected up to here */
  p = sKeyBuf;
  //if (bShown)
  //  PrintString("\n");
  TraceKbd(": ");
  while (*p != '\0')
    TraceKbd("%x ", *p++);
  TraceKbd("\n");
  //bShown = FALSE;
  TraceKbd("[c4] ");
  goto _clear;
}


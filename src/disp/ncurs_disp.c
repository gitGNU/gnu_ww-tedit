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
#include <curses.h>

#include "disp.h"
#include "p_ncurs.h"

/* The top level API, platform independent part */
#include "disp_common.c"

#define DISP_COUNTOF(x)  (sizeof(x) / sizeof((x)[0]))

/*!
@brief

Use a byte as color/background combination from a
PC palette to get the CURSES color/backgraound color pair index.
Notice that not all the entries in the array will
have their CURSES pair counterpart. As CURSES can
have up to 64 color pair definitions at one and the same time.
   Generates PaletteToPairs[] and the actual pairs for all
   the entries in pPalette.

@return 0 failure in system message loop
@return 1 no error
*/
int disp_map_palette(dispc_t *disp, unsigned char *pal, int num_entries)
{
  int pal_map_size8;
  int pal_flags_size8;

  /*
  Allocate palette map buffers
  */
  pal_map_size8 = num_entries * sizeof(disp->palette_to_color_pairs[0]);
  disp->palette_to_color_pairs = s_disp_malloc(disp, pal_map_size8);
  if (disp->palette_to_color_pairs = NULL)
  {
    disp->code = DISP_NCURSES_MEM_FAIL;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "no memory to place palette map");
    return 0;
  }

  pal_flags_size8 = num_entries * sizeof(disp->palette_flags[0]);
  disp->palette_flags = s_disp_malloc(disp, pal_flags_size8);
  if (disp->palette_flags = NULL)
  {
    disp->code = DISP_NCURSES_MEM_FAIL;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "no memory to place palette map flags");
    s_disp_free(disp, disp->palette_to_color_pairs);
    return 0;
  }

  memset(&disp->palette_flags, 0, pal_flags_size8);

  /*
  Enumerate all user palette entries and generate
  a new color pair for the ncurses library
  */
  return 1;
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

typedef struct key_sequence
{
  /*! @brief Description of what can be read from the terminal */
  char *esq_seq;
  /*! @brief Description of how this to be transformed into scancode/shift state pair */
  unsigned int key;  /* hi word is shift state, lo word is scan code + asci char */
  /*! @brief ESC sequence ID specific for a terminal */
  char *term_esc_seq;
} key_sequence_t;

/* The UNIX consoles generate only ASCII symbols
or sequence of symbols. We need to convert those characters into
a Ctrl/Shift/Alt key combination */
static key_sequence_t s_keys[] =
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
  {"\\",        KEY(0,       kb_2) | '\x5c'}, /* <=== syn-highlighting problem */
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

/*
Table of keys to be supplied 'loadkeys'
control	keycode  15 = Tab
control	shift keycode  15 = Tab
control keycode  23 = F30
control keycode  26 = F31
string F30 = "\033[[F"
string F31 = "\033[[G"
*/

/*!
@brief Gets some keydefs string from terminal's capabilities

For some of the keys of the keyboard there are methods (tigetstr) to ask
the terminal what are the correspondent string sequences that it emits when
the keys are pressed.

For example if we ask tigetstr for "kcuu1", which is kbKeyUp,
it might return something like "\033\x5b\x41".

Not all keys have capabilities names. We enumerate all key sequences in our
own small database and for those of which we know there is capability string
we use it to ask tigetstr() and user the result to override our
assumtion which was obtained by experimentation on some standard terminal.
*/
static void s_disp_get_ncurses_keys(dispc_t *disp)
{
 int i;
 char *s;

 for (i = 0; i < DISP_COUNTOF(s_keys); ++i)
 {
   if (s_keys[i].term_esc_seq != NULL)
   {
     s = tigetstr(s_keys[i].term_esc_seq);
     if ((int)s != -1 && (int)s != 0)
     {
       //#if 0
       char key_name_buf[128];
       char *c;

       c = s;
       disp_get_key_name(disp, s_keys[i].key,
                         key_name_buf,
                         sizeof(key_name_buf));
       KEY_TRACE1("%s: ", key_name_buf);
       while (*c != '\0')
         KEY_TRACE1("%x ", *c++);
       KEY_TRACE0("\n");
       //#endif

       /* override the manually coded sequence with what terminal capabilities
       returned */
       s_keys[i].esq_seq = s;
     }
   }
 }
}

/*!
@brief Waits for event from the display window. (ncurses)

The function also is the event pump on ncurses platforms.

@param disp  a dispc object
@param event receives the next event
@return 0 failure in system message loop
@return 1 no error
*/
static int s_disp_process_events(dispc_t *disp)
{
}

/*!
@brief initial setup of display (ncurses)

@param disp a dispc object
@returns true for success
@returns false for failure and error messages and code are set
*/
static int s_disp_init(dispc_t *disp)
{
  if (initscr() == NULL)
  {
    disp->code = DISP_FAILED_NCURS_INIT;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "ncurses initialization failed");
    return 0;
  }

  if (tigetstr("cup") == NULL)
  {
    disp->code = DISP_TERMINAL_NO_CURSOR_OPERATIONS;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "terminal doesn't support cursor oriented operations");
    endwin();
    return 0;
  }

  if (start_color() == ERR)
  {
    disp->code = DISP_TERMINAL_NO_COLOR;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "terminal doesn't support color");
    endwin();
    return 0;
  }

  if (!(
         (raw() != ERR)  /* no interrupt, quist, suspend and flow control */
      && (noecho() != ERR)  /* no auto echo */
      && (nonl() != ERR)  /* don't wait for new line to process keys */
      && (nodelay(stdscr, TRUE) != ERR)
      && (intrflush(stdscr, FALSE) != ERR)
     ))
  {
    disp->code = DISP_NCURSES_MODE_SETUP_FAILURE;
    snprintf(disp->error_msg, sizeof(disp->error_msg),
             "failed to set desired ncurses mode");
    endwin();
    return 0;
  }

  s_disp_get_ncurses_keys(disp);

  return 1;
}

/*!
@brief platform specific disp cleanup (win32 GUI)

@param disp  a display object
*/
static void s_disp_done(dispc_t *disp)
{
 if (endwin() == ERR)
 {
   /* "error: CURSES library failed to restore the original screen" */
   /* & no-body cares */
 }
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
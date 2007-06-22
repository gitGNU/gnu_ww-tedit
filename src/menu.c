/*

File: menu.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 21st December, 1998
Descrition:
  Drop-down menu system.

*/

#include "global.h"
#include "disp.h"
#include "palette.h"
#include "heapg.h"
#include "memory.h"
#include "l2disp.h"
#include "defs.h"
#include "menu.h"

TMenuItem itSep = {0, meSEPARATOR, 0, {0}, 0, 0};

#define MAXITEMS 32  /* This is only with diagnostic purposes */

/* ************************************************************************
   Function: AdjustItemsNum
   Description:
     Display no more items that would fit on the display
*/
static int AdjustItemsNum(const TMenu *p)
{
  if (p->ItemsNumber > disp_wnd_get_height(p->disp) - 5)
    return disp_wnd_get_height(p->disp) - 5;
  return p->ItemsNumber;
}

/* ************************************************************************
   Function: frame
   Description:
     Displays an rectangular frame
*/
void frame(int x1, int y1, int x2, int y2, BYTE attr, dispc_t *disp)
{
  int i;

  ASSERT(x1 < x2);
  ASSERT(y1 < y2);
  ASSERT(x2 < disp_wnd_get_width(disp));  /* This check ensures that x1 is valid as well */
  ASSERT(y2 < disp_wnd_get_height(disp)); /* This check ensures that y1 is valid as well */

  #if USE_ASCII_BOXES
  disp_fill(disp, '-', attr, x1 + 1, y1, x2 - x1);
  disp_fill(disp,'-', attr, x1 + 1, y2, x2 - x1);
  for (i = y1 + 1; i < y2; i++)
  {
    disp_write(disp, "|", x1, i, attr);
    disp_write(disp, "|", x2, i, attr);
  }
  disp_write(disp, "+", x1, y1, attr);
  disp_write(disp, "+", x2, y1, attr);
  disp_write(disp, "+", x1, y2, attr);
  disp_write(disp, "+", x2, y2, attr);
  #else
  disp_fill(disp,'Ä', attr, x1 + 1, y1, x2 - x1);
  disp_fill(disp,'Ä', attr, x1 + 1, y2, x2 - x1);
  for (i = y1 + 1; i < y2; i++)
  {
    disp_write(disp, "³", x1, i, attr);
    disp_write(disp, "³", x2, i, attr);
  }
  disp_write(disp, "Ú", x1, y1, attr);
  disp_write(disp, "¿", x2, y1, attr);
  disp_write(disp, "À", x1, y2, attr);
  disp_write(disp, "Ù", x2, y2, attr);
  #endif
}

/* ************************************************************************
   Function: CalcFlexLen
   Description:
     Calculates Flex write string length. That means not to count
     the '~' characters.
*/
static int CalcFlexLen(const char *s)
{
  const char *p;
  int c;

  if (s == 0)  /* If the address of the string is NULL */
    return 0;

  c = 0;
  for (p = s; *p; ++p)
    if (*p != '~')
      c++;

  return c;
}

/* ************************************************************************
   Function: CalcMenuWidth
   Description:
     Calculates the width of a menu -- horizontal or vertical. ItemsNumber
     is used to calculate the position of a specific horizontal menu item
*/
static int CalcMenuWidth(const TMenu *p, int ItemsNumber)
{
  int i;
  int w;
  int pw;  /* Keep up here max Prompt width */
  int mw;  /* Keep up here max Msg2 width */
  int w1;
  int w2;
  const TMenuItem *m;

  ASSERT(p != NULL);
  ASSERT(p->Orientation == MEVERTICAL || p->Orientation == MEHORIZONTAL);
  ASSERT(ItemsNumber < MAXITEMS);  /* This is a certain control value */

  if (p->Orientation == MEVERTICAL)
  {
    pw = 0;
    mw = 0;
    for (i = 0; i < ItemsNumber; ++i)
    {
      m = (*p->Items)[i];
      w2 = CalcFlexLen(m->Msg2);
      w1 = CalcFlexLen(m->Prompt);
      if (w1 > pw)
        pw = w1;
      if (w2 > mw)
	mw = w2;
    }
    w = pw + mw;
    if (mw != 0)
      w += 4;  /* Add 4 spaces in between Prompt column and Msg2 column */
  }
  else
  {
    /* Horizontal menu */
    w = 2;  /* Initial spacing */
    for (i = 0; i < ItemsNumber; i++)
      w += CalcFlexLen((*p->Items)[i]->Prompt) + 2;
  }

  return w;
}

/* ************************************************************************
   Function: GetShortCut
   Description:
     Returns the character just after '~' in a string. Returns 0 if
     there's no short cut in this string.
*/
static char GetShortCut(const char *s)
{
  char c;
  const char *p;

  ASSERT(s != NULL);
  ASSERT(s[0] != '\0');  /* Empty string is an error */

  for (p = s; *p; ++p)  /* Search for '~' sharacter */
    if (*p == '~')
    {
      c = *++p;
      if (c >= 'A' && c <= 'Z')  /* Convert to lower case */
        c += 'a' - 'A';
      return c;
    }
  return 0;  /* No short cut letter */
}

struct ADesc
{
  WORD ScanCode;
  char CharCode;
};

struct ADesc AltKeys[36] =
{
  {kb1, '1'},  /* ScanCode, AsciiCode */
  {kb2, '2'},
  {kb3, '3'},
  {kb4, '4'},
  {kb5, '5'},
  {kb6, '6'},
  {kb7, '7'},
  {kb8, '8'},
  {kb9, '9'},
  {kb0, '0'},
  {kbQ, 'q'},
  {kbW, 'w'},
  {kbE, 'e'},
  {kbR, 'r'},
  {kbT, 't'},
  {kbY, 'y'},
  {kbU, 'u'},
  {kbI, 'i'},
  {kbO, 'o'},
  {kbP, 'p'},
  {kbA, 'a'},
  {kbS, 's'},
  {kbD, 'd'},
  {kbF, 'f'},
  {kbG, 'g'},
  {kbH, 'h'},
  {kbJ, 'j'},
  {kbK, 'k'},
  {kbL, 'l'},
  {kbZ, 'z'},
  {kbX, 'x'},
  {kbC, 'c'},
  {kbV, 'v'},
  {kbB, 'b'},
  {kbN, 'n'},
  {kbM, 'm'}
};

/* ************************************************************************
   Function: CheckShortCut
   Description:
     Returns the item number holding short cut for this Key. Returns -1 if
     there's no short cur for this Key.
*/
static int CheckShortCut(const TMenu *p, DWORD Key)
{
  int i;
  int j;
  char s;
  char c;
  BYTE ScanCode;
  WORD ShiftState;

  ASSERT(p != NULL);
  ASSERT(p->ItemsNumber > 0 && p->ItemsNumber < MAXITEMS);

  ScanCode = SCANCODE(Key);
  ShiftState = SH_STATE(Key);
  c = ASC(Key);
  for (i = 0; i < p->ItemsNumber; ++i)
  {
    if ((*p->Items)[i]->Options & meSEPARATOR)
      continue;  /* Skip separator lines */
    s = GetShortCut((*p->Items)[i]->Prompt);
    if (s == 0)  /* No short cut letter in this menu item */
      continue;
    if (ShiftState == kbAlt)  /* This is Alt combination */
      for (j = 0; j < 36; ++j)  /* Get the character that stands for Alt+key */
	 if (AltKeys[j].ScanCode == ScanCode)
	 {
	   c = AltKeys[j].CharCode;
	   break;
	 }
    if (c >= 'A' && c <= 'Z')  /* Convert to lower case */
      c += 'a' - 'A';
    if (c == s)
      return i;
  }
  return -1;  /* Short cut not found */
}

/* ************************************************************************
   Function: SelectByShortCut
   Description:
     Returns 0 -- selection was in the current menu.
     Returns 1 -- selection was in the root menu.
       The Sel field of the corespondetn menu is set with the index of
       the selected item.
     Returns 2 -- there's no selected item by short cut.
*/
static int SelectByShortCut(TMenu *root, TMenu *p, DWORD Key)
{
  int s;
  TMenu *m;
  unsigned int fRoot;  /* Where is the selection */

  ASSERT(root != NULL);
  /* This may catch if the pointer is not NULL but points to invalid struct */
  ASSERT(root->ItemsNumber > 0 && root->ItemsNumber < MAXITEMS);

  ASSERT(p != NULL);
  /* This may catch if the pointer is not NULL but points to invalid struct */
  ASSERT(p->ItemsNumber > 0 && p->ItemsNumber < MAXITEMS);

  fRoot = 0;  /* By default in the current menu */
  m = p;
  /* For Alt combination check the Root menu only */
  if (SH_STATE(Key) == kbAlt)
  {
    /* Functional key */
    s = CheckShortCut(root, Key);
    m = root;
    fRoot = 1;
  }
  else
    s = CheckShortCut(p, Key);
  if (s == -1)
    return 2;

  /* Check wether the item is not disabled */
  if (((*m->Items)[s]->Options & meDISABLED) != 0)
    return 2;

  /* Make the selected item current */
  m->Sel = s;
  return fRoot;
}

/* ************************************************************************
   Function: SaveScreenBlock
   Description:
     Saves a block of screen in the heap.
*/
void *SaveScreenBlock(int x1, int y1, int x2, int y2, dispc_t *disp)
{
  void *b;
  int rect_size;
  int w;
  int h;

  ASSERT(x1 >= 0);
  ASSERT(x1 < x2);
  ASSERT(x2 <= disp_wnd_get_width(disp) - 1);
  ASSERT(y1 >= 0);
  ASSERT(y1 < y2);
  ASSERT(y2 <= disp_wnd_get_height(disp) - 1);

  h = y2 - y1 + 1;
  w = x2 - x1 + 1;
  rect_size = disp_calc_rect_size(disp, w, h);
  b = s_alloc(rect_size);
  disp_cbuf_reset(disp, b, rect_size);
  disp_get_block(disp, x1, y1, w, h, b);
  return b;
}

/* ************************************************************************
   Function: RestoreScreenBlock
   Description:
     Restores a block from the heap. Disposes the heap allocation.
*/
void RestoreScreenBlock(int x1, int y1, int x2, int y2,
                        void *b, dispc_t *disp)
{
  int w;
  int h;

  w = x2 - x1 + 1;
  h = y2 - y1 + 1;
  disp_put_block(disp, x1, y1, w, h, b);
  disp_cbuf_mark_invalid(disp, b);
  s_free (b);
}

/* ************************************************************************
   Function: SaveMenuScr
   Description:
     Saves the screen under a menu.
   Returns a pointer in the heap where the screen was stored.
*/
static void *SaveMenuScr(const TMenu *p, int x, int y)
{
  int w;
  int i;

  ASSERT(p != NULL);
  ASSERT(p->ItemsNumber > 0 && p->ItemsNumber < MAXITEMS);

  ASSERT(x < disp_wnd_get_width(p->disp));
  ASSERT(y < disp_wnd_get_height(p->disp));

  i = AdjustItemsNum(p);
  w = CalcMenuWidth(p, i);
  w += 6;
  i += 1;
  if (p->Orientation == MEHORIZONTAL)
    w =	disp_wnd_get_height(p->disp) - 1;

  return SaveScreenBlock(x, y, x + w, y + i, p->disp);
}

/* ************************************************************************
   Function: RestoreMenuScr
   Description:
     Restores a screen allocation. Disposes the allocation buffer from
     the heap.
*/
static void RestoreMenuScr(const TMenu *p, int x, int y, void *b)
{
  int w;
  int h;

  ASSERT(p != NULL);
  ASSERT(p->ItemsNumber > 0 && p->ItemsNumber < MAXITEMS);

  ASSERT(x < disp_wnd_get_width(p->disp));
  ASSERT(y < disp_wnd_get_height(p->disp));

  if (p->Orientation == MEHORIZONTAL)
  {
    w =	disp_wnd_get_width(p->disp) - 1;
    h = 1;
  }
  else
  {
    h = AdjustItemsNum(p);
    w = CalcMenuWidth(p, h);
    w += 6;
    h += 1;
  }

  RestoreScreenBlock(x, y, x + w, y + h, b, p->disp);
}

int SelectedX;  /* In horizontal menu -- selected item position x */
int SelectedY;	 /* In vertical menu -- selected item position y */

/* ************************************************************************
   Function: DisplayVItem
   Description:
     Displays an item in vertical menu. The item will be displayed
     with attributes of selected if fSelected = TRUE.
*/
static void DisplayVItem(const TMenuItem *p, int x, int y,
  int w, BOOLEAN fSelected, int nPalette, dispc_t *disp)
{
  int plen;
  int mlen;
  BYTE _coShortCut;
  BYTE _coMenu;
  BYTE buff[512];
  char *c;

  ASSERT(p != NULL);

  if (fSelected)
  {
    if (p->Options & meDISABLED)
    {
      _coMenu = GetColor(nPalette + _co_MenuSeDisabl);
      _coShortCut = GetColor(nPalette + _co_MenuSeDisabl);
    }
    else
    {
      _coMenu = GetColor(nPalette + _co_MenuSelected);
      _coShortCut = GetColor(nPalette + _co_MenuSeShortCt);
    }
    SelectedY = y;  /* Save the position where a sub menu can appear */
    SelectedX = x;
  }
  else
  {
    if (p->Options & meDISABLED)
    {
      _coMenu = GetColor(nPalette + _co_MenuDisabled);
      _coShortCut = GetColor(nPalette + _co_MenuDisabled);
    }
    else
    {
      _coMenu = GetColor(nPalette + _co_Menu);
      _coShortCut = GetColor(nPalette + _co_MenuShortCut);
    }
  }

  if (p->Options & meSEPARATOR)
  {
    /* Compose a separator line */
    #if USE_ASCII_BOXES
    memset(buff, '-', 79);
    buff[0] = (BYTE)'+';
    buff[w + 3] = (BYTE)'+';
    buff[w + 4] = (BYTE)'\0';  /* End of string marker */
    #else
    memset(buff, 'Ä', 79);
    buff[0] = (BYTE)'Ã';
    buff[w + 3] = (BYTE)'´';
    buff[w + 4] = (BYTE)'\0';  /* End of string marker */
    #endif
    disp_write(disp, (char *)buff, x, y, GetColor(nPalette + _co_MenuFrame));
    return;
  }

  /*
  Compose the item to be displayed: p->Prompt + Spaces + p->Msg2,
  where p->Msg2 should be justified at the most right position.
  */
  c = (char *)buff;
  memset(c, '\x20', 79);

  ++c;  /* Skip a single space at the start */
  strcpy(c, p->Prompt);
  c = strchr(c, '\0');
  *c = ' ';  /* Remove the trailing 0 from strcpy */
  plen = CalcFlexLen(p->Prompt);
  mlen = CalcFlexLen(p->Msg2);
  c += w - plen - mlen;
  if (p->Msg2[0] != '\0')
  {
    strcpy(c, p->Msg2);
    c = strchr(c, '\0');  /* Putting ' ' here will remove the trailing 0 from strcpy */
  }
  *c++ = ' ';
  *c = '\0';  /* After the final ' ' add the trailing 0 */
  if (p->SubMenu)
  {
    #if USE_ASCII_BOXES
    *(c - 1) = '>';
    #else
    *(c - 1) = '\x10';
    #endif
  }
  disp_flex_write(disp, (char *)buff, x + 1, y, _coMenu, _coShortCut);
}

/* ************************************************************************
   Function: DisplayHItem
   Description:
     Displays an item in vertical menu. The item will be displayed
     with attributes of selected if fSelected = TRUE.
     *x is updated with the position where next item should be displayed
*/
static void DisplayHItem(const TMenuItem *p, int *x,
  int y, BOOLEAN fSelected, int nPalette, dispc_t *disp)
{
  BYTE _coShortCut;
  BYTE _coMenu;

  ASSERT(p != NULL);
  ASSERT(p->Prompt != NULL);

  if (fSelected)
  {
    if (p->Options & meDISABLED)
    {
      _coMenu = GetColor(nPalette + _co_MenuSeDisabl);
      _coShortCut = GetColor(nPalette + _co_MenuSeDisabl);
    }
    else
    {
      _coMenu = GetColor(nPalette + _co_MenuSelected);
      _coShortCut = GetColor(nPalette + _co_MenuSeShortCt);
    }
    SelectedY = y;  /* Save the position where a sub menu can apear */
    SelectedX = *x;
  }
  else
  {
    if (p->Options & meDISABLED)
    {
      _coMenu = GetColor(nPalette + _co_MenuDisabled);
      _coShortCut = GetColor(nPalette + _co_MenuDisabled);
    }
    else
    {
      _coMenu = GetColor(nPalette + _co_Menu);
      _coShortCut = GetColor(nPalette + _co_MenuShortCut);
    }
  }
  disp_fill(disp, 0x20, _coMenu, *x, y, 1);
  (*x)++;
  disp_flex_write(disp, p->Prompt, *x, y, _coMenu, _coShortCut);
  *x += CalcFlexLen(p->Prompt);
  disp_fill(disp, 0x20, _coMenu, *x, y, 1);
  (*x)++;
}

static const TMenu *LastMenu;

/* ************************************************************************
   Function: DisplayMenu
   Description:
     Displays a menu and saves the overlapped screen area (if savescr != 0)
     fDoSelection determines wether or not to highlight the selected item
*/
static void DisplayMenu(const TMenu *p, int x, int y,
  int w, BOOLEAN fDoSelection, int nPalette)
{
  int i;
  int curw;

  ASSERT(p != NULL);
  ASSERT(p->Orientation == MEVERTICAL || p->Orientation == MEHORIZONTAL);

  if (p->Orientation == MEVERTICAL)
  {
    curw = CalcMenuWidth(p, p->ItemsNumber);

    /*
      Redraw the frame only when new menu is to be displayed.
      This will speed up the whole menu displaying.
    */
    if (LastMenu != p)
    {
      frame(x, y, x + curw + 3, y + AdjustItemsNum(p) + 1,
        GetColor(nPalette + _co_MenuFrame), p->disp);
    }
    LastMenu = p;

    for (i = 0; i < AdjustItemsNum(p); ++i)
      DisplayVItem((*p->Items)[i], x, y + i + 1, curw,
                   (BOOLEAN)((i == p->Sel) && fDoSelection), nPalette, p->disp);
  }
  else
  {
    /* Horizontal menu */
    disp_fill(p->disp, 0x20, GetColor(nPalette + _co_Menu), x, y, 2);  /* Show two leading spaces */
    curw = x + 2;  /* Current horizontal position */
    for (i = 0; i < p->ItemsNumber; ++i)
      DisplayHItem((*p->Items)[i], &curw, y,
                   (BOOLEAN)((i == p->Sel) && fDoSelection), nPalette, p->disp);
    if (curw < w)  /* Display the trailing spaces */
    {
      disp_fill(p->disp, 0x20,
                GetColor(nPalette + _co_Menu),
                x + curw, y, w - curw);
    }
    x += 2;
  }
}

/* ************************************************************************
   Function: Menu
   Description:
     Opens a menu on position x, y and width w.
     w has meaning only when the root menu is horizontal.
     A sub menu can be invoked by Alt combination in SKey.
   Returns:
     -2 -- menu was not activated
     -3 -- menu was activated and no command was selected
*/
int Menu(TMenu *Root, int x, int y, int w, DWORD SKey)
{
  struct MenuStack
  {
    int x, y;   /* Upper-left corner of a menu */
    int w;  /* Menu width -- horizontal only */
    void *savescr;  /* Here saved the overlapped screen region */
    TMenu *m;  /* A Menu in the stack */
  } SubMenus[MAXMENUDEPTH];
  int CurDepth;
  disp_event_t ev;
  DWORD Key;
  BOOLEAN fMenuExit;
  TMenu *p;
  disp_cursor_param_t cparam;
  int r;  /* Exit code */
  int c;
  void (*func2)(void);
  int selx;
  disp_char_buf_t *pStatLn;
  int h;  /* Calc height here */
  int w2;  /* Calc width here */
  dispc_t *disp;

  ASSERT(Root != NULL);
  ASSERT(Root->Orientation == MEVERTICAL || Root->Orientation == MEHORIZONTAL);
  ASSERT(Root->ItemsNumber > 0 && Root->ItemsNumber < MAXITEMS);
  ASSERT(Root->Sel < Root->ItemsNumber);

  disp = Root->disp;
  LastMenu = NULL;  /* Clear frame redraw cache */
  disp_cursor_get_param(disp, &cparam);
  disp_cursor_hide(disp);
  CurDepth = 0;
  fMenuExit = FALSE;
  r = -1;

  /*
  Fix the menu coordinates so the menu always fit the screen
  */
  if (Root->Orientation == MEVERTICAL)
  {
    h = AdjustItemsNum(Root);
    w2 = CalcMenuWidth(Root, h);
    w2 += 7;
    h += 3;
    if (x + w2 >= disp_wnd_get_width(disp) - 1)
      x = disp_wnd_get_width(disp) - 1 - w2;
    if (y + h >= disp_wnd_get_height(disp) - 1)
      y = disp_wnd_get_height(disp) - 1 - h;
  }

  /* Push the Root menu in the menu stack */
  SubMenus[0].x = x;
  SubMenus[0].y = y;
  SubMenus[0].w = w;
  SubMenus[0].m = Root;
  SubMenus[0].savescr = SaveMenuScr(Root, x, y);
  p = Root;

  pStatLn = SaveStatusLine(disp);

  if (SKey != 0)
  {
    /* Check for a selection in the root menu */
    c = SelectByShortCut(Root, Root, SKey);
    if (c == 2)
    {
      r = -2;
      goto ExitMenu;
    }
    /* Alt key combination was invoked outside the menu */
    /* Select the corespondent sub menu			*/
    DisplayStatusStr2(sStatMenu, coStatusTxt, coStatusShortCut, disp);
    DisplayMenu(Root, x, y, w, 1, Root->nPalette);  /* Display the root menu first */
    goto EnterSubMenu;
  }
  if (Root->InvokeHelp != NULL)
    DisplayStatusStr2(sStatMenu, coStatusTxt, coStatusShortCut, disp);
  else
    DisplayStatusStr2(sStatMenuShort, coStatusTxt, coStatusShortCut, disp);

  do
  {
    /*
      Main loop of the Menu()
    */
    p = SubMenus[CurDepth].m;
    DisplayMenu(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
      SubMenus[CurDepth].w, 1, Root->nPalette);
    do
    {
      disp_event_read(disp, &ev);
    }
    while (ev.t.code != EVENT_KEY);
    Key = ev.e.kbd.key;
    switch (NO_ASC(Key))
    {
      case KEY(0, kbLeft):
	if (p->Orientation == MEVERTICAL)
	{
	  /* Change the vertical menu with the left one */
	  if (CurDepth > 1)
	    break;  /* Only for the first line sub menus */
          if (CurDepth == 0)
            break;  /* No where to exit from zero level */
	  RestoreMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].savescr);
	  CurDepth--;
	  p = SubMenus[CurDepth].m;
	  if (p->Sel > 0)
	    p->Sel--;
	  else
	    p->Sel = AdjustItemsNum(p) - 1;
	  DisplayMenu(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].w, 1, Root->nPalette);
	  goto EnterSubMenu;
	}
	if (p->Sel > 0)
	  p->Sel--;
	else
	  p->Sel = AdjustItemsNum(p) - 1;
	break;

      case KEY(0, kbRight):
	if (p->Orientation == MEVERTICAL)
	{
	  /* Change the vertical menu with the right one */
	  if (CurDepth > 1)
	    break;  /* Only for the first line sub menus */
          if (CurDepth == 0)
            break;  /* No where to exit from zero level */
	  RestoreMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].savescr);
	  CurDepth--;
	  p = SubMenus[CurDepth].m;
	  if (p->Sel < AdjustItemsNum(p) - 1)
	    p->Sel++;
	  else
	    p->Sel = 0;
	  DisplayMenu(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].w, 1, Root->nPalette);
	  goto EnterSubMenu;
	}
	if (p->Orientation == MEVERTICAL)
	  break;
	if (p->Sel < AdjustItemsNum(p) - 1)
	  p->Sel++;
	else
	  p->Sel = 0;
	break;

      case KEY(0, kbUp):
	if (p->Orientation != MEVERTICAL)
	  break;
	IncSel:
	if (p->Sel > 0)
	  p->Sel--;
	else
	  p->Sel = AdjustItemsNum(p) - 1;
	if ((*p->Items)[p->Sel]->Options & meSEPARATOR)
	  goto IncSel;
	break;

      case KEY(0, kbDown):
	if (p->Orientation != MEVERTICAL)
	  goto EnterSubMenu;
	DecSel:
	if (p->Sel < AdjustItemsNum(p) - 1)
	  p->Sel++;
	else
	  p->Sel = 0;
	if ((*p->Items)[p->Sel]->Options & meSEPARATOR)
	  goto DecSel;
	break;

      case KEY(0, kbEnd):
      case KEY(0, kbPgDn):
	p->Sel = AdjustItemsNum(p) - 1;
	break;

      case KEY(0, kbHome):
      case KEY(0, kbPgUp):
	p->Sel = 0;
	break;

      case KEY(0, kbSpace):
        /* it is suitable to use Spacebar to toggle on/off options */
        /* on/off options are supported by a call-back function    */
	if ((*p->Items)[p->Sel]->SubMenu != 0)
          break;
	/* Check wether to call an option support function */
	func2 =  (*p->Items)[p->Sel]->func;
        if (func2 == NULL)
          break;
        goto _execute_func;

      case KEY(0, kbEnter):
	/* Check wether there's a sub menu attached to the current item */
	/* Push this sub menu in the SubMenus stack                     */
	EnterSubMenu:
	if ((*p->Items)[p->Sel]->Options & meDISABLED)
	  break;
	if ((*p->Items)[p->Sel]->SubMenu == 0)
	{
	  /* There's no submenu */
	  /* Check wether to call an option support function */
	  func2 =  (*p->Items)[p->Sel]->func;
	  if (func2 != 0)
	  {
_execute_func:
	    disp_cursor_restore_param(disp, &cparam);
	    (*func2)();  /* Call option support function */
	    disp_cursor_hide(disp);
	    break;
	  }
	  r = (*p->Items)[p->Sel]->Command;
	  while (CurDepth != 0)
	  {
	    /* Exit from all the sub menus */
	    RestoreMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	      SubMenus[CurDepth].savescr);
	    CurDepth--;
	    p = SubMenus[CurDepth].m;
	  }
	  fMenuExit = TRUE;
	  break;
	}
	/* Enter into a submenu */
	selx = SelectedX;
	if (p->Orientation == MEVERTICAL)  /* Small indent */
	  selx = SelectedX + CalcMenuWidth(p, AdjustItemsNum(p)) / 4;
	p = (*p->Items)[p->Sel]->SubMenu;
        p->disp = disp;
	CurDepth++;
	SubMenus[CurDepth].x = selx;
	SubMenus[CurDepth].y = SelectedY + 1;
	/* Fix the menu position to fit in the screen */
        h = AdjustItemsNum(p);
        w2 = CalcMenuWidth(p, h);
        w2 += 7;
        h += 3;
        if (selx + w2 >= disp_wnd_get_width(disp) - 1)
          SubMenus[CurDepth].x = disp_wnd_get_width(disp) - 1 - w2;
        if (SelectedY + 1 + h >= disp_wnd_get_height(disp) - 1)
          SubMenus[CurDepth].y = disp_wnd_get_height(disp) - 1 - h;
	SubMenus[CurDepth].w = 0;  /* On vertical type as sub menus */
	SubMenus[CurDepth].m = p;
	SubMenus[CurDepth].savescr =
	SaveMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y);
	break;

      case KEY(0, kbEsc):
	if (CurDepth == 0)
	{
	  fMenuExit = TRUE;
	  r = -3;
	}
	else
	{
	  RestoreMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].savescr);
	  CurDepth--;
  	  LastMenu = NULL;  /* Clear frame redraw cache */
        }
	break;

      case KEY(0, kbF1):
        if (Root->InvokeHelp == NULL)
          break;
        disp_cursor_restore_param(disp, &cparam);
        (*Root->InvokeHelp)((*p->Items)[p->Sel]->Command,
          (*p->Items)[p->Sel]->psPage,
          (*p->Items)[p->Sel]->psCtxHlpItem, (*p->Items)[p->Sel]->psInfoFile);
        disp_cursor_hide(disp);
        break;

      default:  /* Check for short cut */
	c = SelectByShortCut(Root, p, Key);
    	if (c == 2)
	  break;  /* There's no selection by short cut */
	if (c == 1)
	{
	  /* Selection into the root menu */
	  while (CurDepth != 0)
	  {
	    /* Exit from all the sub menus */
	    RestoreMenuScr(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	      SubMenus[CurDepth].savescr);
	    CurDepth--;
	    p = SubMenus[CurDepth].m;
	  }
	  /* Clear frame redraw cache in case Alt combination is for the same submenu */
	  LastMenu = NULL;
	  DisplayMenu(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	    SubMenus[CurDepth].w, 1, Root->nPalette);  /* Display root menu selection first */
	  goto EnterSubMenu;
	}
	/* c == 0 -- selection into the current sub menu */
	DisplayMenu(p, SubMenus[CurDepth].x, SubMenus[CurDepth].y,
	  SubMenus[CurDepth].w, 1, Root->nPalette);  /* Display root menu selection first */
	goto EnterSubMenu;
    }
  } while (!fMenuExit);

  ExitMenu:
  /* Restore the root menu */
  RestoreMenuScr(p, SubMenus[0].x, SubMenus[0].y,
    SubMenus[0].savescr);
  RestoreStatusLine(pStatLn, disp);
  disp_cursor_restore_param(disp, &cparam);
  return r;
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


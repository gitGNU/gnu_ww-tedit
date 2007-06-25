/*

File: umenu.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 21st December, 1998
Descrition:
  Universal list menu.

*/

#include "global.h"
#include "heapg.h"  /* for occasional use of VALIDATE_HEAP() */
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"
#include "palette.h"
#include "defs.h"
#include "menu.h"
#include "umenu.h"

#define MAX_UMENU_ITEMS  1024  /* This is with diagnostic purposes */

static void DisplayItem(int i, int x, int y, int nWidth, int nWrtEdge, int nNumberOfItems, 
  const char **ppItemsList, BYTE attr)
{
  int l;
  char sItemBuf[128];
  const char *p;

  ASSERT(ppItemsList != NULL);
  ASSERT(nWidth > 0);
  ASSERT(nNumberOfItems >= 0);

  if (i >= nNumberOfItems)
    FillXY(' ', attr, x, y, nWidth);  /* Fill with blanks */
  else
  {
    l = strlen(ppItemsList[i]);
    ASSERT(l < 128);
    p = ppItemsList[i];
    /* Calc the length of what is visible acording to nWrtEdge */
    if (l < nWrtEdge)  /* nVisibleLen have to be always >0 */
      p = "";
    else
      p = &ppItemsList[i][nWrtEdge];
    sprintf(sItemBuf, " %-*s", nWidth, p);
    sItemBuf[nWidth] = '\0';  /* Ensure that the item will fit in the menu (by width) */
    WriteXY(sItemBuf, x, y, attr);
  }
}

/* ************************************************************************
   Function: CountItems
   Description:
*/
static int CountItems(const char **ppItemsList)
{
  const char **ppItem;
  int nNumberOfItems;

  ASSERT(ppItemsList != NULL);

  ppItem = ppItemsList;
  nNumberOfItems = 0;
  while (*ppItem)  /* An address of 0 marks the end of the array */
  {
    nNumberOfItems++;
    ppItem++;

    /* To prevent endless loop */
    ASSERT(nNumberOfItems < MAX_UMENU_ITEMS);
  }

  if (nNumberOfItems == 0)
  {
    *ppItemsList = (char *)sNone;
    nNumberOfItems = 1;
  }
  return (nNumberOfItems);
}

int UMenu_SelectedX;
int UMenu_SelectedY;

/* ************************************************************************
   Function: UMenu
   Description:
     --List menu--

   On entry:
     x, y -- coordinates of the upper left corner
     nCols, nRows -- specify list deployment on the screen
     nWidth -- limit each item into a specified width of characters
     pTitle -- what to display at the top line of the frame
     ppItemsList -- array of pointers to strings, last is NULL
     pSResult -- stores here pointer to string result as selected by the user
     pVResult -- this is input/output parameter. On entry positiones
       the selection bar on the *pVResult item; on exit stores here the
       number of the item selected by the user
    pCmdKeyMap -- set of keys to map external commands, if NULL no
      external keys
    pDlgProc -- invoked when is pressed a key from the pCmdKeyMap, if NULL no
      external commands
    nPalette -- marks the offset of the palette to be user when writing
      the menu on the screen

   TODO:
   Display nWrtEdge for single column menu for example "+5" in the upper
   left corner when > 0.
*/
BOOLEAN UMenu(int x, int y, int nCols, int nRows,
  int nWidth, const char *pTitle, const char **ppItemsList, char *pSResult, int *pVResult,
  struct KeySequence *pCmdKeyMap, int (*pDlgProc)(int nCurItem, int nCmd),
  int nPalette)
{
  int i;
  int nNumberOfItems;
  int w;
  int r, c;  /* Used when display the whole menu */
  int nULCorner;  /* The number of the item at the upper left corner */
  int nDRCorner;  /* The number of the item at the down righ corner */
  int ix, iy;  /* Used when display the menu */
  BYTE co;  /* GetColor here */
  int lsrch;  /* The length of the search string */
  char srch[80];  /* Search string */
  char srchdir;  /* Search direction */
  char s1[512], s2[80];
  int nCur;  /* Current item */
  BOOLEAN bRedraw;  /* Wether or not to redraw the whole menu */
  int nPgItems;  /* Items on a page */
  DWORD dwKey;  /* To read a dwKey here */
  BOOLEAN bExitCode;
  int nWrtEdge;  /* Left write edge */
  void *pMenuScreen;  /* Save the screen here */
  char chr;
  struct KeySequence KeySeq;
  int iCmdCode;  /* Correspondent cmd code for KeySeq */

  ASSERT(x >= 0);
  ASSERT(y >= 0);
  ASSERT(nCols > 0);
  ASSERT(nRows > 0);
  ASSERT(nWidth > 0);
  ASSERT(pTitle != NULL);
  ASSERT(strlen(pTitle) < (unsigned)ScreenWidth);
  ASSERT(ppItemsList != NULL);

  ASSERT(pVResult != NULL);
  ASSERT(pSResult != NULL);
  ASSERT(*pVResult >= 0);

  nULCorner = 0;
  nCur = *pVResult;  /* Positionize on the desired item */
  bRedraw = TRUE;  /* Display the whole menu at the beginning */
  nPgItems = nCols * nRows;
  srch[0] = '\0';
  lsrch = 0;
  nWrtEdge = 0;
  nWidth++;  /* Because of leading ' ' before each item */

  /* Count the number of items; Calc the nMaxWidth */
  if ((nNumberOfItems = CountItems(ppItemsList)) == 0)
    return (FALSE);  /* There's no items at all */

  w = nCols * nWidth;

  /*
  Make the menu position to fit at the screen
  */
  if (x + w + 1 > CurWidth)
    x = CurWidth - w - 1;
  if (y + nRows + 1 > CurHeight)
    y = CurHeight - nRows - 1;
  
  /* Save the screen under the menu */
  pMenuScreen = SaveScreenBlock(x, y, x + w + 1, y + nRows + 1);

  ReDisplay:
  /* Display a frame and pTitle */
  frame(x, y, x + w + 1, y + nRows + 1, GetColor(nPalette + _coUMenuFrame));
  WriteXY(pTitle, (w - strlen(pTitle)) / 2 + x, y, GetColor(nPalette + _coUMenuTitle));
  /* Display the number of items */
  if (*ppItemsList == (char *)sNone)  /* no items */
    strcpy(s1, "0");
  else
  {
    sprintf(s1, "%d", nNumberOfItems);
    /* was itoa(nNumberOfItems, s1, 10); */
  }
  WriteXY(s1, x + w - strlen(s1), y, GetColor(nPalette + _coUMenuFrame));

  /* Recalc the corner position */
  FixCorners:
  ASSERT(nCur >= 0);
  ASSERT(nCur < nNumberOfItems);
  ASSERT(nULCorner >= 0);
  ASSERT(nULCorner < nNumberOfItems);

  nDRCorner = nULCorner + nPgItems - 1;

  ASSERT(nDRCorner >= 0);

  if (nCur < nULCorner)
  {
    nULCorner = nCur;
    bRedraw = 1;
  }
  if (nCur > nDRCorner)
  {
    nULCorner += nCur - nDRCorner;
    bRedraw = 1;
  }

  /* Display the whole menu */
  if (bRedraw)
  {
    c = r = 0;  /* Col and Row of the displayed item */
    i = nULCorner;  /* Current item to be displayed */
    co = GetColor(nPalette + _coUMenuItems);
    do
    {
      ix = x + c * nWidth + 1;
      iy = y + r + 1;
      DisplayItem(i, ix, iy, nWidth, nWrtEdge, nNumberOfItems, ppItemsList, co);
      i++;
      r++;
      if (r == nRows)
      {
	r = 0;
	c++;
      }
    } while (c != nCols);
  } /* if redraw */

  bRedraw = 0;
  /* Display the current item in a light and wait for a dwKey */
  i = nCur - nULCorner;
  c = i / nRows;
  r = i % nRows;
  ix = c * nWidth + x + 1;
  iy = r + y + 1;
  DisplayItem(nCur, ix, iy, nWidth, nWrtEdge, nNumberOfItems, ppItemsList, GetColor(nPalette + _coUMenuSelected));

  GotoXY(ix + lsrch + 1, iy);
  dwKey = ReadKey();

  /* Remove the light from the current item */
  DisplayItem(nCur, ix, iy, nWidth, nWrtEdge, nNumberOfItems, ppItemsList, GetColor(nPalette + _coUMenuItems));
  UMenu_SelectedX = ix;  /* Set the global variables to be used by dlg_proc() */
  UMenu_SelectedY = iy;

  chr = ASC(dwKey);
  if (chr > 32)  /* 32 is code for spacebar */
  {
    if (lsrch > nWidth - 1)  /* We'll overload the search string */
      goto FixCorners;
    srch[lsrch] = chr;
    lsrch++;
    srchdir = 1;
    FixStr:
    srch[lsrch] = '\0';
    /* Search the string amongst the menu items */
    i = nCur;
    if (srchdir == -1)
    {
      srchdir = 1;
      i = 0;  /* Current item's being compared */
    }
    FixStr2:
    strcpy(s2, srch);
    strlwr(s2);
    do
    {
      ASSERT(strlen(ppItemsList[i]) < 511);
      strcpy(s1, ppItemsList[i]);
      strlwr(s1);
      s1[lsrch] = '\0';
      if (strcmp(s1, s2) == 0)
	goto SearchOk;
      i++;
      if (i == nNumberOfItems)
	i = 0;
    } while (i != nCur);
    /* Check the current item as well */
    ASSERT(strlen(ppItemsList[i]) < 511);
    strcpy(s1, ppItemsList[i]);
    strlwr(s1);
    s1[lsrch] = '\0';
    if (strcmp(s1, s2) == 0)
      goto SearchOk;
    /* Search failed */
    srch[0] = '\0';
    lsrch = 0;
    goto FixCorners;
    SearchOk:
    nCur = i;
    goto FixCorners;
  }
  if (dwKey == KEY(0, kbBckSpc))
  {
    /* Remove last char, goto prev matching item */
    if (lsrch == 0)
    {
      nCur = 0;  /* Search string is empty -> goto beginning of menu */
      goto FixCorners;
    }
    else
    {
      lsrch--;  /* Remove the last character */
      srchdir = -1;
      goto FixStr;
    }
  }
  if (dwKey == KEY(0, kbDown)) /* Goto next matching item */
    if (lsrch != 0)
    {
      /* In case of search string holds something */
      srchdir = 1;
      i = nCur + 1;  /* Start search position */
      if (i == nNumberOfItems)
	i = 0;
      goto FixStr2;
    }

  /* Every action causing moving have to empty the srch string */
  lsrch = 0;
  srch[0] = '\0';

  if (pCmdKeyMap != NULL)
  {
    /* Check in the pCmdKeyMap */
    ClearKeySequence(&KeySeq);  /* Only single key commands */
    AddKey(&KeySeq, dwKey);

    if ((iCmdCode = ChkKeySequence(pCmdKeyMap, &KeySeq)) != -1)
    {
      if (iCmdCode != -2)  /* This is a key for a dlg_proc function */
      {
        /* Highlight the current item */
  	DisplayItem(nCur, ix, iy, nWidth, nWrtEdge, nNumberOfItems, ppItemsList, GetColor(nPalette + _coUMenuSelected));

        /* Call the call-back DlgProc function to process the command */
	(pDlgProc)(nCur, iCmdCode);

	nNumberOfItems = CountItems(ppItemsList);
	ASSERT(nNumberOfItems > 0);
	if (nCur >= nNumberOfItems)
	  nCur = nNumberOfItems - 1;
	bRedraw = TRUE;  /* Redraw always after an external command */
        goto ReDisplay;
      }
    }
  }

  switch (dwKey)
  {
    case KEY(0, kbUp):
      if ((signed int)nCur > 0)
	nCur--;
      break;
    case KEY(0, kbDown):
      if (nCur < nNumberOfItems - 1)
	nCur++;
      break;
    case KEY(0, kbHome):
      nCur = 0;
      if (nWrtEdge > 0)
	bRedraw = 1;
      nWrtEdge = 0;
      break;
    case KEY(0, kbEnd):
      nCur = nNumberOfItems - 1;
      if (nWrtEdge > 0)
	bRedraw = 1;
      nWrtEdge = 0;
      break;
    case KEY(0, kbLeft):
      if (nCols == 1)
      {
	if (nWrtEdge > 0)
	{
	  nWrtEdge--;
	  bRedraw = 1;
	}
	break;
      }
      if (nCur < nRows)
	nCur = 0;
      else
      {
	nCur -= nRows;
	if (nCur < nULCorner)
	{
	  nULCorner -= nRows;
	  if ((signed int)nULCorner < 0)
	    nULCorner = 0;
	  bRedraw = 1;
	}
      }
      break;
    case KEY(0, kbRight):
      if (nCols == 1)
      {
	if (nWrtEdge < 1024)
	{
	  nWrtEdge++;
	  bRedraw = 1;
	}
	break;
      }
      if ((signed int)nCur < (signed int)(nNumberOfItems - nRows))
      {
	nCur += nRows;
	if (nCur > nDRCorner)
	{
	  nULCorner += nRows;
	  bRedraw = 1;
	}
      }
      else
	nCur = nNumberOfItems - 1;
      break;
    case KEY(0, kbPgUp):
      if (nCur < nPgItems)
	nCur = 0;
      else {
	nCur -= nPgItems;
	nULCorner -= nPgItems;
	if ((signed int)nULCorner < 0)
	  nULCorner = 0;
	bRedraw = 1;
      }
      break;
    case KEY(0, kbPgDn):
      if ((signed int)nCur < (signed int)(nNumberOfItems - nPgItems))
      {
	nCur += nPgItems;
	nULCorner += nPgItems;
	bRedraw = 1;
      }
      else
	nCur = nNumberOfItems - 1;
      break;
    case KEY(0, kbEsc):
      bExitCode = FALSE;
      *pVResult = nCur;
      goto ExitPoint;
    case KEY(0, kbEnter):
      if (nNumberOfItems == 1)  /* Check for (none) */
        if (*ppItemsList == (char *)sNone)  /* no items */
          break;  /* Nothing to return -- only <ESC> to exit */
      strcpy(pSResult, ppItemsList[nCur]);
      *pVResult = nCur;
      bExitCode = TRUE;
      goto ExitPoint;
  }
  goto FixCorners;

  ExitPoint:
  /* Restore the screen under the menu */
  RestoreScreenBlock(x, y, x + w + 1, y + nRows + 1, pMenuScreen);
  return bExitCode;
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


/*

File: menu.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 21st December, 1998
Descrition:
  Drop-down menu system.

*/

#ifndef MENU_H
#define MENU_H

#define MAXMENUDEPTH 5
#define MAXITEMWIDTH 80

/* Menu orientation */
#define MEVERTICAL 0
#define MEHORIZONTAL 1

/* Menu item options -- bit masks */
#define meDISABLED 1  /* The item is disabled */
#define meSEPARATOR 2  /* Put here a separator line in vertical menus */

#define MAX_MSG2 12

typedef struct _TMenuItem
{
  int Command;  /* Command returned when the item is selected */
  WORD Options;  /* Options -- disabled, enabled... */
  const char *Prompt;  /* The item text */
  char Msg2[MAX_MSG2];  /* The right padded text of item, 0 - no one */
  struct _TMenu *SubMenu;  /* Points to the sub menu if there's any */
  void (*func)(void);  /* This function will be called to change item "on/off" */
  /* Context help system uses either Command code
  either these 3 elements to identify help page, help file and
  element to put in the context help stack menu */
  const char *psPage;
  const char *psCtxHlpItem;
  const char *psInfoFile;
} TMenuItem;

typedef struct _TMenu
{
  int Orientation;
  int ItemsNumber;
  TMenuItem *(*Items)[];  /* Points to array of pointers to items */
  int Sel;  /* Current item selected */
  void (*InvokeHelp)(int nItemCommand, const char *psPage,
    const char *psCtxHlpItem, const char *psInfoFile);
  int nPalette;
} TMenu;

extern TMenuItem itSep;

extern int SelectedX;  /* In horizontal menu -- selected item position x */
extern int SelectedY;	 /* In vertical menu -- selected item position y */

void frame(int x1, int y1, int x2, int y2, BYTE attr);
void *SaveScreenBlock(int x1, int y1, int x2, int y2);
void RestoreScreenBlock(int x1, int y1, int x2, int y2, void *b);
int Menu(TMenu *Root, int x, int y, int w, DWORD SKey);

#endif  /* ifndef MENU_H */

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


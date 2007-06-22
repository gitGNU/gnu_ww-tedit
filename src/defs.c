/*

File: defs.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 20th January, 1999
Descrition:
  Options, definitions and control variables for Layer2.

*/

#include "global.h"
#include "defs.h"

const char *sPressEsc = " -- Press <ESC>";
const char *sAskYN = " (Y/N)?";
const char *sAskYNEsc = " (Y/N/ESC)?";

const char *sNone = "(none)";
const char *sOn = "On";
const char *sOff = "Off";
const char *sYes = "Yes";
const char *sNo = "No";
const char *sStatHelp = " ~Esc~ Cancel ~F1~ Help";
const char *sStatMenu = " ~Esc~ Cancel ~F1~ Help";
const char *sStatMenuShort = " ~Esc~ Cancel";
const char *sStatInfordr = "%s (%s) %c ~Esc~ Close ~Shift+F10~ Menu";

#ifdef MSDOS
const char *sDirMask = "*.*";
#else
const char *sDirMask = "*";
#endif
const char *sAfterExt = "*";  /* To match remaining of the name if '.' is present */
const char *sPromptOpenFile = "File:";
const char *sPromptSaveAs = "Save As:";
const char *sPromptCalculator = "Calc:";
const char *sPromptFind = "Find:";
const char *sPromptFindInFiles = "FindInFiles:";
const char *sPromptFindOptions = "Options (ibr):";
const char *sFileExists = "File (filename) already exists. Overwrite";
const char *sFileExistsInMemory = "File (filename) is loaded for editing";
const char *sReadingDir = "Reading directory...";
const char *sInputRecoveryTime = "Recovery time (sec):";
const char *sInvalidRecoveryTimeValue = "Invalid recovery time value";
const char *sInvalidRecoveryTimeRange = "Recovery time value is not in valid range";
const char *sInputRightMargin = "Right margin:";
const char *sInvalidRightMarginValue = "Invalid right margin value";
const char *sInvalidRightMarginRange = "Right margin value is not in valid range";
const char *sInputTabSize = "Tab size:";
const char *sInvalidTabValue = "Invalid tab value";
const char *sInvalidTabRange = "Tab value is not in valid range";
const char *sInputFileExt = "Filename extention:";
const char *sNoMoreTypes = "No room to add one more document type";
const char *sSaveOptionsPrompt = "Path to save the options:";

const char *sStatDocEdit = " ~Ins~ Add  ~Del~ Remove  ~Enter~ Edit  ~Esc~ Cancel";

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


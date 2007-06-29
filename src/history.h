/*

File: history.h
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 14th December, 1998
Descrition:
  Input line history functions.

*/

#ifndef HISTORY_H
#define HISTORY_H

#include "clist.h"

typedef struct _THistory
{
  TListRoot hlist;
  int nMaxItems;
} THistory;

void InitHistory(THistory *pHist, int nMaxItems);
void SetMaxHistoryItems(THistory *pHist, int nMaxItems);
int CountHistoryLines(const THistory *pHist);
void AddHistoryLine(THistory *pHist, const char *sLine);
void DoneHistory(THistory *pHist);

#ifdef _DEBUG
void DumpHistory(const THistory *pHist);
#endif

void HistoryForEach(const THistory *pHist,
  void (*Process)(const char *sHistLine, void *pContext), BOOLEAN bUpward,
  void *pContext);
void HistorySetLimit(const THistory *pHist, int nMaxItems);
void HistoryCheckUpdate(THistory *pHist,
  const char *psOldText, const char *psNewText);

#endif /* ifndef HISTORY_H */

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


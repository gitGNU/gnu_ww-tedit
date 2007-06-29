/*

File: ini.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 24th December, 1998
Descrition:
  INI file processing functions.

*/

#ifndef INI_H
#define INI_H

#include "file.h"

extern char sINIFileName[_MAX_PATH];
extern char sINIFilePath[_MAX_PATH];
extern char sMasterINIFileName[_MAX_PATH];
extern char sMasterINIFilePath[_MAX_PATH];

typedef struct _SectionBuf
{
  TArray(char) s;  /* buf */
  int n;  /* num characters */
} TSectionBuf;

void GetINIFileName(void);
TFile *CreateINI(const char *psFileName);
BOOLEAN	OpenINI(TFile *pINIFile);
void CloseINI(TFile *pINIFile);
BOOLEAN StoreINI(TFile *pINIFile);
BOOLEAN INIFileIsEmpty(TFile *pINIFile);

int SearchSection(TFile *pINIFile, const char *sSectionName, int *nSectionStart);
BOOLEAN EndOfSection(TFile *pINIFile, int nCurLine);
int RemoveSection(TFile *pINIFile, const char *psSectionName);
BOOLEAN SectionBegin(TSectionBuf *pSection);
BOOLEAN SectionPrintF(TSectionBuf *pSection, const char *fmt, ...);
BOOLEAN SectionInsert(TFile *pINIFile, int nAtLine, TSectionBuf *pSection);
void SectionReset(TSectionBuf *pSection);
void SectionDisposeBuf(TSectionBuf *pSection);

int ParseINILine(char *sLine, char *sKey, char *sVal);
BOOLEAN ValStr(const char *s, int *v, char radix);
char *ExtractQuotedString(char *s, char *psDest, const char *sSep, int nMaxDest,
  BOOLEAN *pbTooLong);
void PrintQuotedString(const char *s, char *psDest);

#endif

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


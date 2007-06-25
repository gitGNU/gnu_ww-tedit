/*

File: doctype.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th March, 1999
Descrition:
  Functions to support and maintain a set of file types
  depending on their extentions.
  Example: '.c;.cpp;.h' is a type of file
           '.asm;.inc' is another type of file
  For each of the types were supported separated options
  concernind their editing needs. For example while editing
  assembler file the tabs '\t' are necessary but while editing
  simple text file may be useless. The different file types
  need their particular syntax highlithing too.

*/

#ifndef DOCTYPE_H
#define DOCTYPE_H

#include "wlimits.h"

/* Supported file types */
#define tyC 0
#define tyASM 1
#define tyA51 2
#define	tyPlain 3

#define END_OF_DOC_LIST_CODE {'\xff'}
#define IS_END_OF_DOC_LIST(pDocType)  ((pDocType)->sExt[0] == '\xff')
#define SET_END_OF_DOC_LIST(pDocType) (pDocType)->sExt[0] = '\xff'

/* Document type description */
typedef struct _TDocType
{
  char sExt[MAX_EXT_LEN];  /* Set of filename extentions separated by ';' */

  int nType;

  int nTabSize;
  BOOLEAN bUseTabs;
  BOOLEAN bOptimalFill;
  BOOLEAN bAutoIndent;
  BOOLEAN bBackspaceUnindent;
  BOOLEAN bCursorThroughTabs;
  BOOLEAN bWordWrap;
} TDocType;

/*
Document types
Syntax related functions. Example prototypes at the moment
are in c_syntax/c_syntax2.h.
*/
typedef struct _TSyntaxHl
{
  char sName[MAX_FTYPE_NAME_LEN];
  void *pfnSyntaxHighlight;
  void *pfnFuncNameScan;
  void *pfnIsOverBracket;
  void *pfnCalcIndent;
  void *pfnExamineKey;
} TSyntax;

TDocType *DetectADocument(const char *sFileName);
TDocType *DetectDocument(TDocType *pDocTypeSet, const char *sFileName);
int DocTypesCountEntries(TDocType *pDocumentItem);
int DocTypesNumEntries(void);
int DocTypesEmptyEntries(void);
void DocTypesInsertEntry(TDocType *pDocumentItem, int nAtPos);
void SetDocumentOptions(TDocType *pDocType);
void CopyDocumentTypeSet(TDocType *pDestDocTypeSet, TDocType *pSrcDocTypeSet);
BOOLEAN CompareDocumentType(TDocType *pDocumentDest, TDocType *pDocumentItem);
BOOLEAN CheckDocumentTypeSet(TDocType *pDestDocTypeSet, TDocType *pSrcDocTypeSet);

int FindDocTypeId(const char *sDocumentType);
char *GetDocTypeName(int DocId);

int FindSyntaxType(const char *sSyntaxType);
int AddSyntaxType(char *psName);
char *GetSyntaxTypeName(int nType);

void *GetSyntaxProc(int nType);
void *GetFuncNameScanProc(int nType);
void *GetExamineKeyProc(int nType);
void *GetIsOverBracketProc(int nType);
void *GetCalcIndentProc(int nType);

void SetSyntaxProc(int nType, void *pfnSyntaxH);
void SetFuncNameScanProc(int nType, void *pfnFuncNameScan);
void SetExamineKeyProc(int nType, void *pfnFuncExamineKey);
void SetIsOverBracketProc(int nType, void *pfnIsOverBracket);
void SetCalcIndentProc(int nType, void *pfnCalcIndent);

int InitDocTypes(void);
void DocTypesDone(void);

#endif  /* ifndef DOCTYPE_H */

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

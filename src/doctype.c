/*

File: doctype.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 12th March, 1999
Descrition:
  Functions to support and maintain a set of file types
  depending on their extentions.
  Example: '*.c;*.cpp;*.h' is a type of file
           '*.asm;*.inc' is another type of file
  For each of the types were supported separated options
  concernind their editing needs. For example while editing
  assembler file were tabs '\t' are necessary but while editing
  simple text file may be useless. The different file types
  need their particular syntax highlithing too.

*/

#include "global.h"
#include "findf.h"
#include "l1opt.h"
#include "l1def.h"
#include "wlimits.h"
#include "memory.h"
#include "tarray.h"
#include "doctype.h"

static TArray(TSyntax) SyntaxTypes;

/* ************************************************************************
   Function: DetectADocument
   Description:
*/
TDocType *DetectADocument(const char *sFileName)
{
  return DetectDocument(DocumentTypes, sFileName);
}

/* ************************************************************************
   Function: DetectDocument
   Description:
     Detects the document type set. Searches amongst the document
     type descriptions in pDocTypeSet. Checks for a document
     filename extention.
*/
TDocType *DetectDocument(TDocType *pDocTypeSet, const char *sFileName)
{
  char sTempBuf[MAX_EXT_LEN];
  char *pMask;

  ASSERT(pDocTypeSet != NULL);

  if (IS_END_OF_DOC_LIST(pDocTypeSet))
    return NULL;  /* No document type definitions */

  ASSERT(pDocTypeSet->sExt[0] != '\0');
  ASSERT(strlen(pDocTypeSet->sExt) < MAX_EXT_LEN);
  ASSERT(sFileName != NULL);
  ASSERT(sFileName[0] != '\0');

  case_sensitive_match = CASE_SENSITIVE_FILENAMES;

  do
  {
    strcpy(sTempBuf, pDocTypeSet->sExt);  /* as strtok() will destroy the string */

    /*
    Try to match the filename against a mask
    in the list of masks (separated by ';')
    */
    pMask = strtok(sTempBuf, ";");
    while (pMask)
    {
      if (match_wildarg(sFileName, pMask))
        return pDocTypeSet;
      pMask = strtok(NULL, ";");
    }

    ++pDocTypeSet;  /* Try next document in the list */
  }
  while (!IS_END_OF_DOC_LIST(pDocTypeSet));

  return NULL;  /* No certain document type detected */
}

/* ************************************************************************
   Function: DocTypesCountEntries
   Description:
*/
int DocTypesCountEntries(TDocType *pDocumentItem)
{
  int nItems;

  nItems = 0;
  for ( ;!IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    ++nItems;
  return nItems;
}

/* ************************************************************************
   Function: DocTypesNumEntries
   Description:
*/
int DocTypesNumEntries(void)
{
  TDocType *pDocumentItem;
  int nItems;

  for (pDocumentItem = DocumentTypes; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    ;
  nItems = (int)(pDocumentItem - DocumentTypes);
  return nItems;
}

/* ************************************************************************
   Function: DocTypesEmptyEntries
   Description:
*/
int DocTypesEmptyEntries(void)
{
  int nNumberOfItems;

  nNumberOfItems = DocTypesNumEntries();
  return MAX_DOCS - nNumberOfItems;
}

/* ************************************************************************
   Function: DocTypesInsertEntry
   Description:
*/
void DocTypesInsertEntry(TDocType *pDocumentItem, int nAtPos)
{
  TDocType *pCurItem;
  int nNumItems;

  nNumItems = DocTypesNumEntries();
  ASSERT(DocTypesEmptyEntries() > 0);
  if (nAtPos < 0)
    nAtPos = nNumItems;

  pCurItem = DocumentTypes + nAtPos;
  memmove(pCurItem + 1, pCurItem, (nNumItems - nAtPos + 1) * sizeof(TDocType));
  memcpy(pCurItem, pDocumentItem, sizeof(TDocType));
}

/* ************************************************************************
   Function: SetDocumentOptions
   Description:
     Copies the options from a particular document type description
     to global editor options.
*/
void SetDocumentOptions(TDocType *pDocType)
{
  ASSERT(pDocType != NULL);

  bUseTabs = pDocType->bUseTabs;
  bOptimalFill = pDocType->bOptimalFill;
  bAutoIndent = pDocType->bAutoIndent;
  bBackspaceUnindent = pDocType->bBackspaceUnindent;
  bCursorThroughTabs = pDocType->bCursorThroughTabs;
  bWordWrap = pDocType->bWordWrap;
  nTabSize = pDocType->nTabSize;
}

/* ************************************************************************
   Function: CopyDocumentTypeSet
   Description:
*/
void CopyDocumentTypeSet(TDocType *pDestDocTypeSet, TDocType *pSrcDocTypeSet)
{
  TDocType *pDocumentItem;
  TDocType *pDocumentDest;

  ASSERT(pDestDocTypeSet != NULL);
  ASSERT(pSrcDocTypeSet != NULL);

  pDocumentDest = pDestDocTypeSet;
  for (pDocumentItem = pSrcDocTypeSet; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    memcpy(pDocumentDest++, pDocumentItem, sizeof(TDocType));

  SET_END_OF_DOC_LIST(pDocumentDest);
}

/* ************************************************************************
   Function: CompareDocumentType
   Description:
     Checks whether pDestDocType differs to pSrcDocType
*/
BOOLEAN CompareDocumentType(TDocType *pDocumentDest, TDocType *pDocumentItem)
{
  if (strcmp(pDocumentDest->sExt, pDocumentItem->sExt) != 0)
    return FALSE;
  if (pDocumentDest->nType != pDocumentItem->nType)
    return FALSE;
  if (pDocumentDest->nTabSize != pDocumentItem->nTabSize)
    return FALSE;
  if (pDocumentDest->bUseTabs != pDocumentItem->bUseTabs)
    return FALSE;
  if (pDocumentDest->bOptimalFill != pDocumentItem->bOptimalFill)
    return FALSE;
  if (pDocumentDest->bAutoIndent != pDocumentItem->bAutoIndent)
    return FALSE;
  if (pDocumentDest->bBackspaceUnindent != pDocumentItem->bBackspaceUnindent)
    return FALSE;
  if (pDocumentDest->bCursorThroughTabs != pDocumentItem->bCursorThroughTabs)
    return FALSE;
  if (pDocumentDest->bWordWrap != pDocumentItem->bWordWrap)
    return FALSE;
  return TRUE;
}

/* ************************************************************************
   Function: CheckDocumentTypeSet
   Description:
     Checks whether pDestDocTypeSet differs to pSrcDocTypeSet
*/
BOOLEAN CheckDocumentTypeSet(TDocType *pDestDocTypeSet, TDocType *pSrcDocTypeSet)
{
  TDocType *pDocumentItem;
  TDocType *pDocumentDest;

  ASSERT(pDestDocTypeSet != NULL);
  ASSERT(pSrcDocTypeSet != NULL);

  if (DocTypesCountEntries(pDestDocTypeSet) != DocTypesCountEntries(pSrcDocTypeSet))
    return FALSE;

  pDocumentDest = pDestDocTypeSet;
  for (pDocumentItem = pSrcDocTypeSet; !IS_END_OF_DOC_LIST(pDocumentItem); ++pDocumentItem)
    if (!CompareDocumentType(pDocumentDest++, pDocumentItem))
      return FALSE;

  return TRUE;
}

/* ************************************************************************
   Function: AddSyntaxType
   Description:
     Adds a new syntax type of documents
   Returns:
     The new id of this syntax type
   -1, no memory for the operation.
   -2, psName is too long
*/
int AddSyntaxType(char *psName)
{
  TSyntax Entry;

  if (strlen(psName) + 1 > sizeof(Entry.sName))
    return -2;

  strcpy(Entry.sName, psName);
  Entry.pfnSyntaxHighlight = NULL;
  Entry.pfnFuncNameScan = NULL;
  Entry.pfnExamineKey = NULL;
  Entry.pfnIsOverBracket = NULL;
  Entry.pfnCalcIndent = NULL;

  TArrayAdd(SyntaxTypes, Entry);
  if (!TArrayStatus(SyntaxTypes))
  {
    TArrayClearStatus(SyntaxTypes);
    return -1;
  }

  return _TArrayCount(SyntaxTypes) - 1;
}

/* ************************************************************************
   Function: FindSyntaxType
   Description:
     Searches for an ID of a syntax type
   Returns:
     result >= 0 -- this is the ID
     result < 0  -- no such syntax type registered
*/
int FindSyntaxType(const char *sSyntaxType)
{
  int i;

  for (i = 0; i < _TArrayCount(SyntaxTypes); ++i)
    if (strcmp(SyntaxTypes[i].sName, sSyntaxType) == 0)
      return i;

  return -1;
}

/* ************************************************************************
   Function: GetSyntaxTypeName
   Description:
     Gets the string name of a syntax type based on its id.
*/
char *GetSyntaxTypeName(int nType)
{
  ASSERT(nType >= 0);
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].sName;
}

/* ************************************************************************
   Function: GetSyntaxProc
   Description:
     Gets the syntax highlighting proc that is installed for
     a specific syntax type.
*/
void *GetSyntaxProc(int nType)
{
  if (nType < 0)
    return NULL;
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].pfnSyntaxHighlight;
}

/* ************************************************************************
   Function: GetFuncNameScanProc
   Description:
     Gets the function names scan proc that is installed for
     a specific syntax type.
*/
void *GetFuncNameScanProc(int nType)
{
  if (nType < 0)
    return NULL;
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].pfnFuncNameScan;
}

/* ************************************************************************
   Function: GetExamineKeyProc
   Description:
     Gets the key-scanner procthat is installed for a specific syntax type.
*/
void *GetExamineKeyProc(int nType)
{
  if (nType < 0)
    return NULL;
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].pfnExamineKey;
}

/* ************************************************************************
   Function: GetIsOverBracketProc
   Description:
*/
void *GetIsOverBracketProc(int nType)
{
  if (nType < 0)
    return NULL;
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].pfnIsOverBracket;
}

/* ************************************************************************
   Function: GetCalcIndentProc
   Description:
     Gets the proc that calculates the desired indentation of a current line.
     It might be used by CmdEditPaste() to determine the auto-indent.
*/
void *GetCalcIndentProc(int nType)
{
  if (nType < 0)
    return NULL;
  if (nType >= _TArrayCount(SyntaxTypes))
    return NULL;

  return SyntaxTypes[nType].pfnCalcIndent;
}

/* ************************************************************************
   Function: SetSyntaxProc
   Description:
     Set the syntax highlighting proc for a specific syntax type.
*/
void SetSyntaxProc(int nType, void *pfnSyntaxH)
{
  ASSERT(nType >= 0);
  ASSERT(nType < _TArrayCount(SyntaxTypes));
  ASSERT(pfnSyntaxH != NULL);

  SyntaxTypes[nType].pfnSyntaxHighlight = pfnSyntaxH;
}

/* ************************************************************************
   Function: SetFuncNameScanProc
   Description:
     Set the function names scan proc for a specific syntax type.
*/
void SetFuncNameScanProc(int nType, void *pfnFuncNameScan)
{
  ASSERT(nType >= 0);
  ASSERT(nType < _TArrayCount(SyntaxTypes));
  ASSERT(pfnFuncNameScan != NULL);

  SyntaxTypes[nType].pfnFuncNameScan = pfnFuncNameScan;
}

/* ************************************************************************
   Function: SetCalcIndentProc
   Description:
*/
void SetCalcIndentProc(int nType, void *pfnCalcIndent)
{
  ASSERT(nType >= 0);
  ASSERT(nType < _TArrayCount(SyntaxTypes));
  ASSERT(pfnCalcIndent != NULL);

  SyntaxTypes[nType].pfnCalcIndent = pfnCalcIndent;
}

/* ************************************************************************
   Function: SetExamineKeyProc
   Description:
     Set the key preprocessing proc for a specific syntax type.
*/
void SetExamineKeyProc(int nType, void *pfnFuncExamineKey)
{
  ASSERT(nType >= 0);
  ASSERT(nType < _TArrayCount(SyntaxTypes));
  ASSERT(pfnFuncExamineKey != NULL);

  SyntaxTypes[nType].pfnExamineKey = pfnFuncExamineKey;
}

/* ************************************************************************
   Function: SetIsOverBracketProc
   Description:
     Set the is-over-bracket proc for a specific syntax type.
*/
void SetIsOverBracketProc(int nType, void *pfnIsOverBracket)
{
  ASSERT(nType >= 0);
  ASSERT(nType < _TArrayCount(SyntaxTypes));
  ASSERT(pfnIsOverBracket != NULL);

  SyntaxTypes[nType].pfnIsOverBracket = pfnIsOverBracket;
}

/* ************************************************************************
   Function: InitDocTypes
   Description:
     Initial setup of document types
*/
int InitDocTypes(void)
{
  int i;
  int Result;

  TArrayInit(SyntaxTypes, 5, 5);
  for (i = 0; i < _countof(sParserTypes); ++i)
  {
    Result = AddSyntaxType(sParserTypes[i]);
    if (Result >= 0)
      continue;
    if (Result == -1)
      return -1;
    ASSERT(0);  /* Fix sParserTypes[] */
  }
  return 0;
}

/* ************************************************************************
   Function: DocTypesDone
   Description:
     Disposes the document type registry.
*/
void DocTypesDone(void)
{
  TArrayDispose(SyntaxTypes);
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


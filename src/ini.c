/*

File: ini.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 24th December, 1998
Descrition:
  INI file processing functions.

*/

#include "global.h"
#include "path.h"
#include "l1opt.h"
#include "l1def.h"
#include "l2disp.h"
#include "memory.h"
#include "file.h"
#include "block.h"
#include "nav.h"
#include "main2.h"
#include "ini.h"

char sINIFileName[_MAX_PATH];
char sINIFilePath[_MAX_PATH];
char sMasterINIFileName[_MAX_PATH];
char sMasterINIFilePath[_MAX_PATH];

/* ************************************************************************
   Function: GetINIFileName
   Description:
     Searches in the current directory first.
     Searches in the directory of w.exe.
   Result:
     sINIFileName will hold the	INI file name.
*/
void GetINIFileName(void)
{
  char *sHome;
  char sDummy[_MAX_PATH];

  sHome = getenv("HOME");

  /*
  Set the master INI file
  */
  if (sHome != NULL)  /* Only if HOME is specified in the environment */
  {
    strcpy(sMasterINIFileName, sHome);
    AddTrailingSlash(sMasterINIFileName);
    strcat(sMasterINIFileName, sDefaultMasterINIFileName);
  }
  else
  {
    /* Set the file to be in the directory of the program */
    strcpy(sMasterINIFileName, sModulePath);
    strcat(sMasterINIFileName, sDefaultMasterINIFileName);
  }
  GetFullPath(sMasterINIFileName);

  /* Check whether such a file exists in the current directory */
  strcpy(sINIFileName, sDefaultINIFileName);
  GetFullPath(sINIFileName);
  if (FileExists(sINIFileName))
    goto _separate_path;  /* Such a file exists */

  /*
  Set to be in the the directory specified by HOME environment variable
  */
  if (sHome != NULL)  /* Only if HOME is specified in the environment */
  {
    strcpy(sINIFileName, sHome);
    AddTrailingSlash(sINIFileName);
    strcat(sINIFileName, sDefaultINIFileName);
    goto _separate_path;
  }

  /* Set the file to be in the directory of the program */
  strcpy(sINIFileName, sModulePath);
  strcat(sINIFileName, sDefaultINIFileName);

_separate_path:
  FSplit(sINIFileName, sINIFilePath, sDummy, "ERROR", TRUE, TRUE);
  FSplit(sMasterINIFileName, sMasterINIFilePath, sDummy, "ERROR", TRUE, TRUE);
}

/* ************************************************************************
   Function: CreateINI
   Description:
     Allocates an empty INI file.
*/
TFile *CreateINI(const char *psFileName)
{
  TFile *pINIFile;

  ASSERT(psFileName[0] != '\0');

  pINIFile = s_alloc(sizeof(TFile));
  InitEmptyFile(pINIFile);
  strcpy(pINIFile->sFileName, psFileName);

  return pINIFile;
}

/* ************************************************************************
   Function: OpenINI
   Description:
     Opens an INI file.
     In case of failure (file doesn't exists) pINIFile remains an
     empty file. This is valid to continue further by calling all
     the functions that	parse INI file.
*/
BOOLEAN OpenINI(TFile *pINIFile, dispc_t *disp)
{
  switch (LoadFilePrim(pINIFile))
  {
    case 0:  /* Load OK */
      return TRUE;
    case 2:  /* File doesn't exists */
      pINIFile->nEOLType = DEFAULT_EOL_TYPE;  /* as no EOL type is assigned */
      return TRUE;
    case 3:  /* No memory */
      ConsoleMessageProc(disp, NULL, MSG_OK | MSG_ERROR, NULL, sNoMemoryINI);
      DisposeFile(pINIFile);
      s_free(pINIFile);
      return FALSE;
    case 4:  /* Invalid path */
      ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sINIFileName, NULL);
      return TRUE;  /* New file	will be created	upon exit */
    case 5:  /* I/O error */
      ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sINIFileName, NULL);
      DisposeFile(pINIFile);
      s_free(pINIFile);
      return FALSE;
    default:  /* Invalid LoadFilePrim() output */
      ASSERT(0);
      return FALSE;
  }
}

/* ************************************************************************
   Function: INIFileIsEmpty
   Description:
*/
BOOLEAN INIFileIsEmpty(TFile *pINIFile)
{
  return pINIFile->nNumberOfLines == 0;
}

/* ************************************************************************
   Function: CloseINI
   Description:
     Disposes an INI file allocation.
*/
void CloseINI(TFile *pINIFile)
{
  DisposeFile(pINIFile);
  s_free(pINIFile);
  pINIFile = NULL;
}

/* ************************************************************************
   Function: StoreINI
   Description:
*/
BOOLEAN StoreINI(TFile *pINIFile, dispc_t *disp)
{
  switch (StoreFilePrim(pINIFile, pINIFile->nEOLType))
  {
    case 0:
      break;
    case 1:  /* Failed to open the file for writing */
      ConsoleMessageProc(disp, NULL, MSG_ERRNO | MSG_ERROR | MSG_OK, sINIFileName, NULL);
      return FALSE;
    case 2:  /* Failed to store whole the file */
      ConsoleMessageProc(disp, NULL, MSG_ERROR | MSG_OK, pINIFile->sFileName, sSaveFailed);
      return FALSE;
    case 3:  /* No memory */
      ConsoleMessageProc(disp, NULL, MSG_OK | MSG_ERROR, NULL, sNoMemoryINI);
      return FALSE;
    default:  /* Invalid StoreFilePrim() output */
      ASSERT(0);
      return FALSE;
  }
  return TRUE;
}

/* ************************************************************************
   Function: SearchSection
   Description:
     Searches for a specific section in pINIFile.
   Returns:
     0 -- section found and *ppSectionStart points at the line
       with [SectionName].
     error_code -- problems parsing INI file. *ppSectionStart points
       the error line.
*/
int SearchSection(TFile *pINIFile, const char *sSectionName, int *nSectionStart)
{
  int nLn;
  char *p;
  char *d;
  char sSection[MAX_SECTION_NAME_LEN];
  int nExitCode;

  ASSERT(pINIFile != NULL);

  nLn = 0;
  while (nLn < pINIFile->nNumberOfLines)
  {
    p = GetLineText(pINIFile, nLn);

    while (*p)
    {
      if (*p == ';')  /* Line commented */
	break;
      if (*p == '[')
      {
	d = sSection;
	*d = '\0';  /* Assume section name is empty */
	p++;
	while (*p != ']')
	{
	  if (*p == '\0')  /* Error no closing bracket */
	  {
	    nExitCode = ERROR_NO_CLOSING_BRACKET;
	    goto _exit;
	  }
	  if (!isalpha(*p))
	  {
	    nExitCode = ERROR_INVALID_SECTION_NAME;
	    goto _exit;
	  }
	  *d++ = *p++;

	  /* Check for overloading */
	  if (d	- sSection >= MAX_SECTION_NAME_LEN)
	  {
	    nExitCode = ERROR_INVALID_SECTION_NAME;
	    goto _exit;
	  }
	}
	*d = '\0';
	if (stricmp(sSection, sSectionName) == 0)
	{
	  nExitCode = 0;
	  goto _exit;
	}
      }
      else
	if (!isspace(*p))
	  break;  /* Section name must have be the first non blank */
      p++;
    }
    ++nLn;
  }
  nExitCode = ERROR_NO_SUCH_SECTION;  /* Section not found */

  _exit:
  *nSectionStart = nLn;
  return nExitCode;
}

/* ************************************************************************
   Function: EndOfSection
   Description:
     Checks whether the current line presents an end of section.
*/
BOOLEAN EndOfSection(TFile *pINIFile, int nCurLine)
{
  char *p;

  ASSERT(pINIFile != NULL);
  ASSERT(nCurLine <= pINIFile->nNumberOfLines);

  if (nCurLine == pINIFile->nNumberOfLines)
    return TRUE;  /* End of file is end of section */

  p = GetLineText(pINIFile, nCurLine);
  /* Determine whether this is a valid section start */
  while (*p)
  {
    if (*p == ';')  /* Line commented */
      return FALSE;
    if (*p == '[')
      return TRUE;
    if (!isspace(*p))
      return FALSE;  /* Section name must have be the first non blank */
  }
  return FALSE;
}

extern BOOLEAN DeleteCharacterBlockPrim(TFile *pFile,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos);

/* ************************************************************************
   Function: RemoveSection
   Description:
     Removes specific section from the file in memory.
   Returns:
     The line where the section used to be.
*/
int RemoveSection(TFile *pINIFile, const char *psSectionName)
{
  int nErrorCode;
  int nINILine;
  int nStartLine;

  if ((nErrorCode = SearchSection(pINIFile, psSectionName, &nINILine)) != 0)
    return - 1;

  nStartLine = nINILine;
  do
  {
    ++nINILine;
  }
  while (!EndOfSection(pINIFile, nINILine));

  DeleteCharacterBlockPrim(pINIFile, nStartLine, 0, nINILine, -1);

  return nStartLine;
}

/* ************************************************************************
   Function: SectionBegin
   Description:
     Initial setup of a buffer into which to do SectionPrintF()
*/
BOOLEAN SectionBegin(TSectionBuf *pSection)
{
  ASSERT(pSection != NULL);
  pSection->n = 0;
  TArrayInit(pSection->s, 4096, 4096);
  return pSection->s != NULL;
}

/* ************************************************************************
   Function: SectionPrintF
   Description:
     Formatted output to an ini file section buffer.
*/
BOOLEAN SectionPrintF(TSectionBuf *pSection, const char *fmt, ...)
{
  char buf[8096];
  va_list ap;
  int x;

  va_start(ap, fmt);
  x = vsnprintf(buf, sizeof(buf), fmt, ap);
  ASSERT(x != -1);
  if (x == -1)
    return FALSE;
  va_end(ap);

  TArrayInsertGroup(pSection->s, pSection->n - 1, buf, x);
  if (!TArrayStatus(pSection->s))
    return FALSE;
  pSection->n += x;
  return TRUE;
}

/* ************************************************************************
   Function: SectionInsert
   Description:
     Inserts a buffer of SectionPrintF()s into an ini file.
     nAtLine tells the line, -1 add at the end of the ini file.
*/
BOOLEAN SectionInsert(TFile *pINIFile, int nAtLine, TSectionBuf *pSection)
{
  TBlock *pBlock;
  BOOLEAN r;

  /* Add one empty line */
  if (!SectionPrintF(pSection, "\n"))
    return FALSE;

  pBlock = MakeBlock(pSection->s, 0);
  if (pBlock == NULL)
    return FALSE;

  if (nAtLine > 0 && nAtLine < pINIFile->nNumberOfLines)
    GotoPosRow(pINIFile, 0, nAtLine);
  else
    GotoColRow(pINIFile, 0, pINIFile->nNumberOfLines);

  r  = InsertCharacterBlockPrim(pINIFile, pBlock);

  DisposeABlock(&pBlock);
  return r;
}

/* ************************************************************************
  Function: SectionReset
  Description:
*/
void SectionReset(TSectionBuf *pSection)
{
  char c;

  TArrayDeleteGroup(pSection->s, 0, _TArrayCount(pSection->s));
  pSection->n = 1;  /* the trailing 0 */
  c = '\0';
  TArrayInsert(pSection->s, 0, c);  /* This will be the trailing 0 */
}

/* ************************************************************************
  Function: SectionDisposeBuf
  Description:
*/
void SectionDisposeBuf(TSectionBuf *pSection)
{
  TArrayDispose(pSection->s);
}

/* ************************************************************************
   Function: ParseINILine
   Description:
     Parses an INI file line. Returns the key and value.
   Returns:
     0 - parse went OK
     error_code - in case of error.
*/
int ParseINILine(char *sLine, char *sKey, char *sVal)
{
  char *p;

  ASSERT(sKey != NULL);
  ASSERT(sVal != NULL);

  p = sLine;

  if (!*p)
    return ERROR_LINE_EMPTY;

  /* Get to first non-blank */
  while (isspace(*p))
  {
    if (!*p)
      return ERROR_LINE_EMPTY;
    p++;
  }

  /* Extract the key */
  while (isalnum(*p))
  {
    *sKey++ = *p;
    if (!*p)
      return ERROR_MISSING_EQU_CHAR;
    p++;
  }
  *sKey = '\0';

  /* Scan for '=' */
  while (*p != '=')
  {
    if (!*p || isalpha(*p) || *p != ' ')
      return ERROR_MISSING_EQU_CHAR;
    p++;
  }

  /* Skip = */
  p++;

  *sVal = '\0';  /* No value by default */

  /* Search first non-blank */
  while (isspace(*p))
  {
    if (!*p)
      return 0;  /* No value specified */
    p++;
  }

  /* Extract the value */
  strcpy(sVal, p);
  return 0;  /* Parse went OK */
}

/* ************************************************************************
   Function: ValStr
   Description:
     Converts a string to integer.
*/
BOOLEAN ValStr(const char *s, int *v, char radix)
{
  long x;
  char *endptr;

  if (s == NULL)
    return FALSE;
  x = strtol(s, &endptr, radix);
  if ((endptr != NULL) && (*endptr != '\x0'))
    return FALSE;
  *v = (int)x;
  return TRUE;
}

/* ************************************************************************
   Function: ExtractQuotedString
   Description:
     Extracts quoted string. Processes the escaped characters.
     If the string is not quoted stops at one of the characters in sSep.
*/
char *ExtractQuotedString(char *s, char *psDest, const char *sSep, int nMaxDest,
  BOOLEAN *pbTooLong)
{
  char *p;
  char *c;
  BOOLEAN bNeedsQuote;

  *pbTooLong = FALSE;
  p = s;
  if (*p == '\0')
    return NULL;

  /* Extract the pattern; Check for quoted string
  or for escaped quote character */
  c = psDest;
  *c = '\0';
  bNeedsQuote = FALSE;
  if (*p == '\"')
  {
    bNeedsQuote = TRUE;
    ++p;
  }
  while (*p != '\0')
  {
    if (strchr(sSep, *p) != NULL)
    {
      if (!bNeedsQuote)
        break;
    }
    if (*p == '\"')
      if (bNeedsQuote)
      {
        ++p;
        break;
      }
    if (*p == '\\')
    {
      if (*(p + 1) == '\"')
      {
        *c++ = '\"';
        p += 2;
        continue;
      }
    }
    *c++ = *p++;
    if (c - psDest == nMaxDest)
    {
      *pbTooLong = TRUE;
      break;
    }
  }
  *c = '\0';
  return p;
}

/* ************************************************************************
   Function: PrintQuotedString
   Description:
     Prints string in a quotes. Escapes the quotes inside.
*/
void PrintQuotedString(const char *s, char *psDest)
{
  const char *p;
  char *d;

  if (strchr(s, '\"') != NULL ||
    strchr(s, ' ') != NULL ||
    strchr(s, ',') != NULL)
  {
    p = s;
    d = psDest;

    *d++ = '\"';
    while (*p)
    {
      if (*p == '\"')
      {
        *d++ = '\\';
        *d++ = '\"';
        ++p;
        continue;
      }
      *d++ = *p++;
    }
    *d++ = '\"';
    *d = '\0';
  }
  else
    strcpy(psDest, s);
}

/*
This software is distributed under the conditions of the BSD style license.

Copyright (c)
1995-2005
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


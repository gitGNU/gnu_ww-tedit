/*

File: enterln.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 14th December, 1998
Descrition:
  Input line editor function.

*/

#include "global.h"
#include "tarray.h"
#include "palette.h"
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"
#include "memory.h"
#include "l1opt.h"
#include "l1def.h"
#include "nav.h"
#include "defs.h"
#include "l2disp.h"
#include "umenu.h"
#ifdef WIN32
#include "winclip.h"
#endif
#include "file.h"
#include "wrkspace.h"
#include "enterln.h"

#include "findf.h"

static const char **ppIndex;    /* ExtractItems() prepares here an index of items */
static const char **ppIndexFill;  /* To walk thourh the array   */
static const char *sMask;  /* This is the mask for selecting item to ppIndex */
static char sCDirMask[_MAX_PATH];  /* GetNextFileName() stores here a mask */
static int nEndPath;  /* Where in sCDirMask ends the path */
static int nMaskLen;  /* strlen(sMask) */

/* ************************************************************************
   Function: ProcessHistoryLine
   Description:
     Call-back function passed to HistoryForEach().
     Adds the lines that match sMask to ppIndex.
*/
static void ProcessHistoryLine(const char *sHistLine, void *pContext)
{
  if (nMaskLen > 0)
    if (strnicmp(sHistLine, sMask, nMaskLen) != 0)
      return;

  /* A line that matches the mask */
  *ppIndexFill = sHistLine;
  ppIndexFill++;
  *ppIndexFill = NULL;  /* Serves as end marker */
}

/* ************************************************************************
   Function: ExtractItems
   Description:
     Makes an index with all the history items that match the mask.
   On entry:
     Global variables:
       sMask -- what mask to use to match the items
   On exit:
     ppIndex -- index of items matching the mask, NULL to mark the end of the list.
*/
static void ExtractItems(THistory *pHist)
{
  ASSERT(sMask != NULL);
  ASSERT(pHist != NULL);

  /* Allocate an array of pointers */
  ppIndex = s_alloc(sizeof(char *) * (pHist->nMaxItems + 1));  /* +1 for one NULL marker */

  nMaskLen = strlen(sMask);
  ppIndexFill = ppIndex;
  *ppIndexFill = NULL;  /* No items by default */
  HistoryForEach(pHist, ProcessHistoryLine, TRUE, NULL);
  return;
}

/* ************************************************************************
   Function: HistoryMenu
   Description:
     Displays a history menu. The menu will be consisted only of
     the items that match sMask.
   Returns:
     TRUE -- an item was choosen.
     FALSE -- exit with	ESC.
*/
static BOOLEAN HistoryMenu(THistory *pHist, char *_sMask, char *sResult)
{
  int nDummy;
  BOOLEAN bMenuResult;

  ASSERT(_sMask != NULL);
  ASSERT(pHist != NULL);
  ASSERT(sResult != NULL);

  sMask = _sMask;
  ExtractItems(pHist);

  nDummy = 0;
  bMenuResult = UMenu(10, CurHeight - 15, 1, 12, 65, "History",
    ppIndex, sResult, &nDummy, NULL, NULL, coUMenu);

  s_free((void *)ppIndex);
  ppIndex = NULL;

  return bMenuResult;
}

/* ************************************************************************
   Function: GetAutocompletionString
   Description:
     Makes an autocompletion of sEdLine based on the strings set
     in pHist.
*/
static void GetAutocompletionString(THistory *pHist, char *sEdLine)
{
  ASSERT(sEdLine != NULL);
  ASSERT(pHist != NULL);

  sMask = sEdLine;
  ExtractItems(pHist);

  if (*ppIndex != NULL)  /* If there's at least one string */
    strcat(sEdLine, &(*ppIndex)[strlen(sEdLine)]);  /* Get the first matching */
  s_free((void *)ppIndex);  /* (void *) used as unconst */
}

/* ************************************************************************
   Function: GetMatchingFile
   Description:
     Gets next or previous matching file name.
     This works for 4dos alike command line Tab and Shift+Tab selections.

     There are generally 2 ways to be invoked this function:
     1. To be supplied a file name: example dpcomm.c and comm.c to be selected
     then this function will search for dp* to find next or previous occurences
     of files matching the mask;
     2. To be supplied a mask: example *.c
     then this function will store the mask and will seatch for this mask
     to find next or previoud occurences of files mathing the mask;

     Mode #2 is considered terminated when not all the filename is selected
     on one of the next function invokations;

   TODO:
     Explore the drive only specifications!
*/
static void GetNextMatchingFile(char *sEditLine, char **pSelStart, char **pSelEnd, BOOLEAN bNext)
{
  char sPath[_MAX_PATH];
  char sPrevFile[_MAX_PATH];
  char sBaseName[_MAX_PATH];
  char *p;
  const char *sMask;
  char *pNameStart;
  struct FF_DAT *ff_dat;
  struct findfilestruct ff;
  char filename[_MAX_PATH];
  BOOLEAN bBaseFound;
  int nSelStart;
  char sResultFile[_MAX_PATH];
  BOOLEAN bDriveOnly;
  static char sLastPath[_MAX_PATH];
  BOOLEAN bFirstTime;

  strcpy(sPath, sEditLine);

  /*
  Check whether sEditLine is suggesting a mask
  */
  if (strchr(sEditLine, '*') != NULL || strchr(sEditLine, '?') != NULL)
  {
    strcpy(sCDirMask, sEditLine);  /* sCDirMask stores the mask between 2 calls */
    *pSelStart = sEditLine;
    *pSelEnd = strchr(sEditLine, '\0');
    /* Let nEndPath points end of the path in sCDirMask */
    p = strchr(sCDirMask, '\0');
    while (p != sCDirMask)
    {
      if (*p == PATH_SLASH_CHAR)
        break;
      #if defined(MSDOS) || defined(WIN32)
      if (*p == ':')
        break;
      #endif
      --p;
    }
    if (*p == PATH_SLASH_CHAR)
      ++p;
    nEndPath = p - sCDirMask;
  }
  else
  {
    /*
    Check for end of mask sequence suggestion.
    The rule is: if not entire line is selected then end
    of the mask loop.
    */
    if (*pSelStart != sEditLine)
    {
      strcpy(sCDirMask, sDirMask);
      nEndPath = 0;
    }
  }

  /*
  Prepare _p_ for a back scan operation.
  It should be at the end of the selection or
  if there's no selection at the end of the line.
  */
  if (*pSelStart != NULL)
  {
    nSelStart = *pSelStart - sEditLine;
    sPath[nSelStart] = '\0';
    p = &sPath[nSelStart];
  }
  else
  {
    nSelStart = -1;
    p = strchr(sPath, '\0');
  }

  /*
  Scan back from end of the name to search for '.'
  or PATH_SLASH_CHAR. Mark the start of the name.
  */
  sMask = sCDirMask;
  bDriveOnly = FALSE;
  while (p != &sPath[0])
  {
    if (*p == '.')
      sMask = sAfterExt;
    if (*p == PATH_SLASH_CHAR)
      break;
    #if defined(MSDOS) || defined (WIN32)
    if (*p == ':')
    {
      bDriveOnly = TRUE;
      break;
    }
    #endif
    --p;
  }
  pNameStart = p;
  if (*p == PATH_SLASH_CHAR)
    ++pNameStart;

  strcpy(sBaseName, sEditLine + (pNameStart - sPath));
  if (strchr(sBaseName, '*') != NULL || strchr(sBaseName, '?') != NULL)
    sBaseName[0] = '\0';  /* First time invoked with a mask */
  else
  {
    /*
    Now check for path component in sBaseName;
    Recompose sBaseName by removing the path;
    */
    if (strcmp(sCDirMask, sDirMask) != 0)
      strcpy(sBaseName, &sEditLine[nEndPath]);
  }

  strcat(sPath, sMask);

  /*
  sLastPath stores the last path and mask that this function
  was invoked with. Here sLastPath is compared against the
  new parameter in order to detect whether this is the first
  time this function is invoked with such an argument. If
  this is first time then the search loop will start from
  the very first file in the directory and all the content
  after pSelStart will be completely ignored.
  */
  bFirstTime = FALSE;
  if (filestrcmp(sLastPath, sPath) != 0)
  {
    bFirstTime = TRUE;
    strcpy(sLastPath, sPath);
  }

  /*
  Read the directory and wait to appear the name from sEditLine (sBaseName).
  sPrevFilep[] always contains the file from previous loop iteration. Depending
  on bNext parameter the result is sPrevFile or is waited next file by rising
  a flag bBaseFound.
  */
  ff_dat = find_open(sPath, FFIND_DIRS | FFIND_FILES);
  if (ff_dat == NULL)
    return;  /* TODO: prepare something in sResult */

  sPrevFile[0] = '\0';
  bBaseFound = FALSE;
  sResultFile[0] = '\0';
  while (find_file(ff_dat, &ff) == 0)
  {
    strcpy(filename, ff.filename);
    if ((ff.st.st_mode & S_IFDIR) != 0)
      AddTrailingSlash(filename);

    if (nSelStart == -1 || bFirstTime)
    {
      if (bNext)
        goto _terminate;  /* First time when calling GetNextMatchingFile() */
      else
        break;  /* No previous files */
    }

    if (bBaseFound)
    {
      /* We are at the file next to sBaseName */
_terminate:
      strcpy(sResultFile, filename);
      break;
    }

    if (sBaseName[0] == '\0' && strcmp(sCDirMask, sDirMask) != 0)
    {
      if (bNext)
        goto _terminate;  /* First time when calling GetNextMatchingFile() */
      else
        break;  /* No previous files */
    }

    if (sBaseName[0] == '\0' || filestrcmp(filename, sBaseName) == 0)
    {
      if (bNext)
        bBaseFound = TRUE;
      else
      {
        if (sPrevFile[0] != '\0')  /* If there's previous file */
          strcpy(sResultFile, sPrevFile);
        break;  /* no previous file */
      }
    }
    strcpy(sPrevFile, filename);  /* Preserve for the next iteration */
  }

  find_close(ff_dat);

  /*
  Get the original path and add sResulFile[]
  */
  if (sResultFile[0] != '\0')
  {
    /*
    2 cases here 1. we have user specified mask in sCDirMask or
    2. sPath has a path to be used;
    */
    if (strcmp(sCDirMask, sDirMask) != 0)
    {
      /* 
      case 1
      Exclude the path only from sCDirMask
      */
      strcpy(sEditLine, sCDirMask);
      sEditLine[nEndPath] = '\0';
    }
    else
    {
      /* case 2 */
      *pNameStart = '\0';  /* Terminate in sPath where name ends */
      strcpy(sEditLine, sPath);
    }
    if (sEditLine[0] != '\0' && !bDriveOnly)  /* If there is a path portion */
      AddTrailingSlash(sEditLine);
    strcat(sEditLine, sResultFile);
  }

  /*
  Maintain the selection.
  */
  if (sResultFile[0] != '\0')  /* Only if the loop returned result */
  {
    if (nSelStart == -1)
      *pSelStart = sEditLine + strlen(sPath) + strlen(sBaseName);
    else
      *pSelStart = sEditLine + nSelStart;
    *pSelEnd = strchr(sEditLine, '\0');
  }

  if (*pSelStart == *pSelEnd)  /* Empty block */
    *pSelStart = *pSelEnd = NULL;  /* unmark */
}

/* ************************************************************************
   Function: cmpfiles
   Description:
     Call-back function invoked by qsort.
     Compares two filenames. Directories weight more.
*/
static int cmpfiles(const void *_p1, const void *_p2)
{
  char ** p1 = (char **)_p1;
  char ** p2 = (char **)_p2;

  char *l1 = strchr(*p1, '\0');
  char *l2 = strchr(*p2, '\0');

  --l1;  /* where is supposed to have directory slash characters */
  --l2;

  /* Put dir entries at first place */
  if (*l1 == PATH_SLASH_CHAR)
  {
    if (*l2 != PATH_SLASH_CHAR)
      return -1;
  }
  else
    if (*l2 == PATH_SLASH_CHAR)
      if (*l1 != PATH_SLASH_CHAR)
	return 1;

  return filestrcmp(*p1, *p2);
}

/* ************************************************************************
   Function: MatchingFilesMenu
   Description:
     Displays menu of matching file. Same like F7 in 4dos.
*/
static void MatchingFilesMenu(char *sEditLine, char **pSelStart, char **pSelEnd)
{
  char sPath[_MAX_PATH];
  char sTitle[_MAX_PATH];
  char sPrevFile[_MAX_PATH];
  char sBaseName[_MAX_PATH];
  char *p;
  const char *sMask;
  char *pNameStart;
  struct FF_DAT *ff_dat;
  struct findfilestruct ff;
  char filename[_MAX_PATH];
  BOOLEAN bBaseFound;
  int nSelStart;
  char sResultFile[_MAX_PATH];
  BOOLEAN bDriveOnly;
  TArray(char *) pIndex;
  char *h;
  int nDummy;
  int x;
  int i;

  strcpy(sPath, sEditLine);

  if (*pSelStart != NULL)
  {
    nSelStart = *pSelStart - sEditLine;
    sPath[nSelStart] = '\0';
    p = &sPath[nSelStart];
  }
  else
  {
    nSelStart = -1;
    p = strchr(sPath, '\0');
  }

  /*
  Scan back from end of the name to search for '.'
  or PATH_SLASH_CHAR. Mark the start of the name.
  */
  sMask = sDirMask;
  bDriveOnly = FALSE;
  while (p != &sPath[0])
  {
    if (*p == '.')
      sMask = sAfterExt;
    if (*p == PATH_SLASH_CHAR)
      break;
    #if defined(MSDOS) || defined (WIN32)
    if (*p == ':')
    {
      bDriveOnly = TRUE;
      break;
    }
    #endif
    --p;
  }
  pNameStart = p;
  if (*p == PATH_SLASH_CHAR)
    ++pNameStart;

  strcpy(sBaseName, sEditLine + (pNameStart - sPath));
  
  strcat(sPath, sMask);

  /*
  Read the directory into pIndex array.
  */
  TArrayInit(pIndex, 50, 25);
  if (!TArrayStatus(pIndex))
    return;

  ff_dat = find_open(sPath, FFIND_DIRS | FFIND_FILES);
  if (ff_dat == NULL)
    return;  /* TODO: prepare something in sResult */

  sPrevFile[0] = '\0';
  bBaseFound = FALSE;
  sResultFile[0] = '\0';
  while (find_file(ff_dat, &ff) == 0)
  {
    strcpy(filename, ff.filename);
    if ((ff.st.st_mode & S_IFDIR) != 0)
      AddTrailingSlash(filename);

    h = s_alloc(strlen(filename) + 1);
    strcpy(h, filename);
    TArrayAdd(pIndex, h);
    if (!TArrayStatus(pIndex))
      goto _dispose_array;
    i = _TArrayCount(pIndex);
  }

  find_close(ff_dat);

  /*
  Sort the file names and dispay a menu.
  */
  qsort(pIndex, _TArrayCount(pIndex), sizeof(char *), cmpfiles);

  h = NULL;  /* Terminal symbol */
  TArrayAdd(pIndex, h);
  if (!TArrayStatus(pIndex))
    goto _dispose_array;

  nDummy = 0;
  x = ScreenWidth - ScreenWidth / 4 - 5;
  ShrinkPath(sPath, sTitle, ScreenWidth / 4 - 5, FALSE);
  if (!UMenu(x, 3, 1, ScreenHeight - 7, ScreenWidth / 4, sTitle,
    (const char **)pIndex, sResultFile, &nDummy, NULL, NULL, coUMenu))
    goto _dispose_array;

  /*
  Get the original path and add sResulFile[]
  */
  if (sResultFile[0] != '\0')
  {
    *pNameStart = '\0';  /* Terminate in sPath where name ends */
    strcpy(sEditLine, sPath);
    if (sEditLine[0] != '\0' && !bDriveOnly)  /* If there is a path portion */
      AddTrailingSlash(sEditLine);
    strcat(sEditLine, sResultFile);
  }

  /*
  Maintain the selection.
  */
  if (sResultFile[0] != '\0')  /* Only if the loop returned result */
  {
    if (nSelStart == -1)
      *pSelStart = sEditLine + strlen(sPath) + strlen(sBaseName);
    else
      *pSelStart = sEditLine + nSelStart;
    *pSelEnd = strchr(sEditLine, '\0');
  }

  if (*pSelStart == *pSelEnd)  /* Empty block */
    *pSelStart = *pSelEnd = NULL;  /* unmark */

_dispose_array:
  for (i = 0; i < _TArrayCount(pIndex); ++i)
    s_free(pIndex[i]);

  TArrayDispose(pIndex);
}

/* ************************************************************************
   Function: SetBraces
   Description:
     In *ppOpenBracket and *ppCloseBracket sets the positions of a
     matching braces to be highlithed if at nCurPos there's a bracket.
*/
static void SetBraces(char *sEdLine, char **ppOpenBracket, char **ppCloseBracket,
  int nCurPos)
{
  BOOLEAN bOpenPar;  /* There is an opening bracket at the current pos */
  char *p;
  char cpar, opar;
  int i;
  int nDir;  /* Search direction */
  int nDepth;  /* Nested brackets depth level */
  BOOLEAN bQuit;
  BOOLEAN bMatchFound;  /* The matching bracket was found */

  *ppOpenBracket = NULL;
  *ppCloseBracket = NULL;

  bOpenPar = FALSE;
  p = &sEdLine[nCurPos];

  /* Set cpar and opar depending on what is at the current position */
  for (i = 0; i < MAX_BRACES; i++)
  {
    if (*p == OpenBraces[i])
    {
      bOpenPar = TRUE;
      break;
    }
    else
      if (*p == CloseBraces[i])
        break;
  }

  if (i == MAX_BRACES)
    return;  /* There are no brackets at the current pos */

  opar = CloseBraces[i];
  cpar = OpenBraces[i];

  nDir = -1;
  if (bOpenPar)  /* Invert braces */
  {
    nDir = 1;
    opar = OpenBraces[i];
    cpar = CloseBraces[i];
  }

  bQuit = FALSE;
  bMatchFound = FALSE;
  nDepth = 0;

  if (p == sEdLine && nDir == -1)  /* ')' at the start of the line */
    bQuit = TRUE;
  else
    p += nDir;

  while (!bQuit)
  {
    if (*p == opar)
      nDepth++;
    if (*p == cpar)
    {
      if (nDepth == 0)
      {
        bMatchFound = TRUE;
        bQuit = TRUE;
        continue;
      }
      else
        nDepth--;
    }
    if (*p == '\0')
    {
      bQuit = TRUE;  /* End of line */
      continue;
    }
    if (p == sEdLine)  /* Start of the line */
      if (nDir < 0)
      {
        bQuit = TRUE;
        continue;
      }
    p += nDir;
  }

  if (!bMatchFound)
    return;  /* No matching pair found */

  /* Export the results */
  if (&sEdLine[nCurPos] < p)
  {
    *ppOpenBracket = &sEdLine[nCurPos];
    *ppCloseBracket = p;
  }
  else
  {
    *ppOpenBracket = p;
    *ppCloseBracket = &sEdLine[nCurPos];
  }
}

/* ************************************************************************
   Function: GetClipboardLine
   Description:
*/
static void GetClipboardLine(char *s, int nMaxLen)
{
  TLine *pLine;

  ASSERT(s != NULL);

  #ifdef WIN32
  GetFromWindowsClipboard(&pClipboard);
  #endif

  *s = '\0';
  if (pClipboard)
  {
    pLine = GetBlockLine(pClipboard, 0);
    strncpy(s, pLine->pLine, nMaxLen);
    s[nMaxLen] = '\0';
  }
}

/* ************************************************************************
   Function: PutClipboardLine
   Description:
*/
static void PutClipboardLine(const char *s)
{
  if (pClipboard != NULL)
    DisposeABlock(&pClipboard);

  pClipboard = MakeBlock(s, 0);

  #ifdef WIN32
  PutToWindowsClipboard(pClipboard);
  #endif
}

/* ************************************************************************
   Function: ExpandBlock
   Description:
     Expands selection block depending on nNewPos.
     *ppSelStart and *ppSelEnd point the selections area, these
     are input/output parameters.
*/
static void ExpandBlock(char *sEdString, char **ppSelStart, char **ppSelEnd,
  int nCurPos, int nNewPos)
{
  char *pTemp;

  ASSERT(nCurPos != nNewPos);
  ASSERT(sEdString != NULL);
  ASSERT(ppSelStart != NULL);
  ASSERT(ppSelEnd != NULL);
  ASSERT(*ppSelStart <= *ppSelEnd);

  if (*ppSelStart == *ppSelEnd)
  {
    /* No block */
    *ppSelStart = *ppSelEnd = &sEdString[nCurPos];
    if (nNewPos > nCurPos)
      *ppSelEnd = &sEdString[nNewPos];
    else
      *ppSelStart = &sEdString[nNewPos];
    return;
  }
  if (*ppSelStart == &sEdString[nCurPos])
    *ppSelStart = &sEdString[nNewPos];
  if (*ppSelEnd == &sEdString[nCurPos])
    *ppSelEnd = &sEdString[nNewPos];
  if (*ppSelStart > *ppSelEnd)
  {
    pTemp = *ppSelStart;
    *ppSelStart = *ppSelEnd;
    *ppSelEnd = pTemp;
  }
}

/* ************************************************************************
   Function: _RemoveSel
   Description:
     Removes the selected area in a string.
*/
static void _RemoveSel(char *pSelStart, char *pSelEnd)
{
  ASSERT(pSelStart != NULL);
  ASSERT(pSelEnd != NULL);
  ASSERT(pSelStart <= pSelEnd);
  ASSERT(strchr(pSelEnd, (char)NULL) != NULL);

  memcpy(pSelStart, pSelEnd, (int)(strchr(pSelEnd, (char)NULL) - pSelEnd + 1));
}

/* ************************************************************************
   Function: RemoveSel
   Description:
     Macros to that removes the selected area in the current edit
     line buffer, then fixes length, cur position and selecion
     pointers.
*/
#define RemoveSel() \
  { \
    _RemoveSel(pSelStart, pSelEnd); \
    nLen = strlen(sInputString); \
    nCurPos = pSelStart - sInputString; \
    pSelStart = pSelEnd = NULL; \
  }

/* ************************************************************************
   Function: EnterLn
   Description:

   TODO: Multistep Undo/Redo.
*/
BOOLEAN EnterLn(const char *sPrompt, char *sInputString, int nMaxLen,
  THistory *pHist, char *(*pfnMenu)(void),
  void (*pfnHelp)(void *pCtx),
  void *pHelpCtx,
  BOOLEAN bCompleteMatchingNames)
{
  char sSave[300];
  char sResult[300];  /* History menu result */
  char sHistoryMask[300];  /* Compose history mask here */
  int nFirstX;
  int nWidth;
  int nCurPos;
  int nWrtEdge;
  BOOLEAN bQuit;
  BOOLEAN bExitCode;
  int nLen;
  char *pSelStart, *pSelEnd;
  CharInfo OutputBuf[600];
  CharInfo *Output;
  BYTE attr;
  BYTE attrsel;
  BYTE attrbrace;
  char *p;
  DWORD dwKey;
  char c;
  char *pOpenBracket;
  char *pCloseBracket;
  int nWordPos;
  BOOLEAN bOutsideLine;
  int nLen2;
  char *psExtern;

  ASSERT(strlen(sInputString) < 300);
  ASSERT(nMaxLen > 0);
  ASSERT(nMaxLen < 300);

  strcpy(sSave, sInputString);

  WriteXY(sPrompt, 0, CurHeight, GetColor(coEnterLnPrompt));
  #if USE_ASCII_BOXES
  WriteXY("^", CurWidth, CurHeight, GetColor(coEnterLnPrompt));
  #else
  WriteXY("\x18", CurWidth, CurHeight, GetColor(coEnterLnPrompt));
  #endif

  ReInit:
  nFirstX = strlen(sPrompt);
  nCurPos = nLen = strlen(sInputString);
  pSelStart = sInputString;
  pSelEnd = strchr(sInputString, (char)NULL);
  ASSERT(pSelEnd != NULL);
  if (pSelStart == pSelEnd)
    pSelStart = pSelEnd = NULL;
  nWrtEdge = 0;
  nWidth = ScreenWidth - nFirstX - 1;
  bQuit = FALSE;
  bExitCode = FALSE;

  attr = GetColor(coEnterLn);
  attrsel = GetColor(coUnchanged);
  attrbrace = GetColor(coEnterLnBraces);

  do
  {
    ASSERT(nCurPos >= 0);
    ASSERT(nCurPos <= nLen);
    #ifdef _DEBUG
    if (pSelStart != pSelEnd)
    {
      ASSERT(pSelStart <= pSelEnd);
      ASSERT(pSelStart >= sInputString);
    }
    #endif

    FixWrtPos(&nWrtEdge, nCurPos, nWidth);

    /* Set the positions if there are braces to be highlithed */
    SetBraces(sInputString, &pOpenBracket, &pCloseBracket, nCurPos);

    /* Display the edit string */
    Output = OutputBuf;
    for (p = &sInputString[nWrtEdge]; *p && Output - OutputBuf < nWidth; p++)
    {
      PutChar(Output, *p);
      if (p >= pSelStart && p < pSelEnd)  /* Exclusive at the right marker */
        PutAttr(Output, attrsel);
      else
        if (p == pOpenBracket || p == pCloseBracket)
          PutAttr(Output, attrbrace);
            else
              PutAttr(Output, attr);
      Output++;
    }
    /* Fill up with spaces */
    for (; Output - OutputBuf < nWidth; p++)
    {
      PutChar(Output, ' ');
      PutAttr(Output, attr);
      Output++;
    }

    puttextblock(nFirstX, CurHeight, CurWidth - 1, CurHeight, OutputBuf);
    GotoXY(nCurPos - nWrtEdge + nFirstX, CurHeight);
    dwKey = ReadKey();

    switch (dwKey)
    {
      case KEY(0, kbLeft):
        if (pSelStart != pSelEnd)  /* Go at the start of the marked area */
          nCurPos = pSelStart - sInputString;
        else
          if (nCurPos > 0)
            nCurPos--;
        pSelStart = pSelEnd = NULL;
        break;

      case KEY(0, kbRight):
            if (nCurPos < nLen)
          nCurPos++;
        pSelStart = pSelEnd = NULL;
        break;

      case KEY(0, kbUp):
      case KEY(0, kbDown):
        if (pHist == NULL)
          break;  /* No history available */
        /*
        Invoke the history with sInputString cut only to the first marked pos,
        so HistoryMenu will be capable of showing all items matching the typed
        sequence.
        */
        if (pSelStart == pSelEnd)
        {
          /* No selection */
          strcpy(sHistoryMask, sInputString);
        }
        else
        {
          strcpy(sHistoryMask, sInputString);
          ASSERT(pSelStart >= sInputString);
          sHistoryMask[pSelStart - sInputString] = '\0';
        }
        if (!HistoryMenu(pHist, sHistoryMask, sResult))
          break;
        strcpy(sInputString, sResult);
        goto ReInit;  /* Select the whole string etc... */

      case KEY(0, kbHome):
        nCurPos = 0;
        pSelStart = pSelEnd = NULL;
        break;

      case KEY(0, kbEnd):
        nCurPos = nLen;
        pSelStart = pSelEnd = NULL;
        break;

      case KEY(kbCtrl, kbRBrace):
        /* Goto the matching bracket */
        if (&sInputString[nCurPos] == pOpenBracket)
        {
          nCurPos = pCloseBracket - sInputString;
          pSelStart = pSelEnd = NULL;
          break;
        }
        if (&sInputString[nCurPos] == pCloseBracket)
        {
          nCurPos = pOpenBracket - sInputString;
          pSelStart = pSelEnd = NULL;
          break;
        }
        break;

      case KEY(0, kbF4):
        if (pfnMenu == NULL)
          break;
        psExtern = pfnMenu();
        if (psExtern == NULL)
          break;
        strcpy(sResult, psExtern);
        goto _paste;

      case KEY(0, kbF3):  /* TODO: Find next occurence */
        break;

      case KEY(kbShift, kbF3):  /* TODO: Find occurrence back */
        break;

      case KEY(kbCtrl, kbA):
        pSelStart = sInputString;
        pSelEnd = strchr(sInputString, (char)NULL);
        ASSERT(pSelEnd != NULL);
        nWrtEdge = 0;
        nCurPos = nLen;
        break;

      case KEY(kbCtrl, kbLeft):
        pSelStart = pSelEnd = NULL;
        nWordPos = nCurPos;
        GotoWord(sInputString, nLen, &nWordPos, -1, &bOutsideLine, NULL, FALSE);
        if (bOutsideLine)
          nCurPos = 0;
        else
          nCurPos = nWordPos;
        break;

      case KEY(kbCtrl, kbRight):
        pSelStart = pSelEnd = NULL;
        nWordPos = nCurPos;
        GotoWord(sInputString, nLen, &nWordPos, 1, &bOutsideLine, NULL, FALSE);
        if (bOutsideLine)
          nCurPos = nLen;
        else
          nCurPos = nWordPos;
        break;

      case KEY(kbCtrl + kbShift, kbLeft):
        nWordPos = nCurPos;
        GotoWord(sInputString, nLen, &nWordPos, -1, &bOutsideLine, NULL, FALSE);
        if (bOutsideLine)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, 0);
          nCurPos = 0;
        }
        else
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nWordPos);
          nCurPos = nWordPos;
        }
        break;

      case KEY(kbCtrl + kbShift, kbRight):
        nWordPos = nCurPos;
        GotoWord(sInputString, nLen, &nWordPos, 1, &bOutsideLine, NULL, FALSE);
        if (bOutsideLine)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nLen);
          nCurPos = nLen;
        }
        else
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nWordPos);
          nCurPos = nWordPos;
        }
        break;

      case KEY(kbShift, kbLeft):
        if (nCurPos > 0)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nCurPos - 1);
          nCurPos--;
        }
        break;

      case KEY(kbShift, kbRight):
        if (nCurPos < nLen)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nCurPos + 1);
          nCurPos++;
        }
        break;

      case KEY(kbShift, kbHome):
        if (nCurPos > 0)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, 0);
          nCurPos = 0;
        }
        break;

      case KEY(kbShift, kbEnd):
        if (nCurPos < nLen)
        {
          ExpandBlock(sInputString, &pSelStart, &pSelEnd, nCurPos, nLen);
          nCurPos = nLen;
        }
        break;

      case KEY(0, kbBckSpc):
        if (nCurPos == 0)
          break;
        if (pSelStart != pSelEnd)
        {
          RemoveSel();
          break;
        }
        memmove(&sInputString[nCurPos - 1], &sInputString[nCurPos], nLen - nCurPos);
        nLen--;
        nCurPos--;
        break;

      case KEY(kbCtrl, kbC):
      case KEY(kbCtrl, kbIns):
        if (pSelStart == pSelEnd)
          break;  /* No block marked */

        strcpy(sResult, pSelStart);
        sResult[pSelEnd - pSelStart] = '\0';
        PutClipboardLine(sResult);
        break;

      case KEY(kbCtrl, kbV):
      case KEY(kbShift, kbIns):
        GetClipboardLine(sResult, nMaxLen - 1);  /* Get a line from clipboard in WrtLnBuf */
_paste:
        if (pSelStart != pSelEnd)
          RemoveSel();
        nLen2 = strlen(sResult);
        if (nLen + nLen2 > nMaxLen - 1)  /* The string will be overloaded */
          break;
        /* Insert the string */
        memmove(&sInputString[nCurPos + nLen2], &sInputString[nCurPos], nLen - nCurPos);
        nLen += nLen2;
        memcpy(&sInputString[nCurPos], sResult, nLen2);
        nCurPos += nLen2;
        sInputString[nLen] = '\0';
        break;

      case KEY(kbCtrl, kbX):
      case KEY(kbShift, kbDel):
        if (pSelStart == pSelEnd)
          break;  /* No block marked */

        strcpy(sResult, pSelStart);
        sResult[pSelEnd - pSelStart] = '\0';
        PutClipboardLine(sResult);
        RemoveSel();
        break;

      case KEY(0, kbDel):
        if (pSelStart == pSelEnd)
        {
          /* No block */
          if (nCurPos < nLen)
          {
            memcpy(&sInputString[nCurPos], &sInputString[nCurPos + 1], nLen - nCurPos);
            nLen--;
          }
          break;
        }
        else
          RemoveSel();
        break;

      case KEY(0, kbTab):
      case KEY(0, kbF9):
        /*
        Get the next matching name from the directory.
        */
        if (!bCompleteMatchingNames)
          break;

        GetNextMatchingFile(sInputString, &pSelStart, &pSelEnd, TRUE);
        nLen = strlen(sInputString);
        nCurPos = nLen;
        break;

      case KEY(kbShift, kbTab):
      case KEY(0, kbF8):
        /*
        Get the previous matching name from the directory.
        */
        if (!bCompleteMatchingNames)
          break;

        GetNextMatchingFile(sInputString, &pSelStart, &pSelEnd, FALSE);
        nLen = strlen(sInputString);
        nCurPos = nLen;
        break;

      case KEY(0, kbF7):
      case KEY(kbCtrl, kbTab):
        /*
        Display autocompletion window.
        */
        if (!bCompleteMatchingNames)
          break;

        MatchingFilesMenu(sInputString, &pSelStart, &pSelEnd);
        nLen = strlen(sInputString);
        nCurPos = nLen;
        break;

      case KEY(0, kbEsc):
        bQuit = TRUE;
        bExitCode = FALSE;
        strcpy(sInputString, sSave);  /* Restore the string from the start */
        break;

      case KEY(0, kbEnter):
        if (pHist != NULL)
        {
          if (sInputString[0] != '\0')  /* Add only non-empty strings */
            AddHistoryLine(pHist, sInputString);
        }
        bQuit = TRUE;
        bExitCode = TRUE;
        break;

	  case KEY(0, kbF1):
		if (pfnHelp != NULL)
		  pfnHelp(pHelpCtx);
		break;

      default:
        c = ASC(dwKey);
        if (c < 32)
          break;  /* Filter the control symbols */

        if (pSelStart != pSelEnd)
          RemoveSel();
        if (nLen == nMaxLen - 1)  /* The string will be overloaded */
          break;

        /* Insert the character */
        memmove(&sInputString[nCurPos + 1], &sInputString[nCurPos], nLen - nCurPos);
        nLen++;
        sInputString[nCurPos] = c;
        nCurPos++;
        sInputString[nLen] = '\0';

        /*
        Check to autofill -- The typed character should be at the end of the
        typed string.
        */
        if (nCurPos == nLen && pHist != NULL)
        {
          GetAutocompletionString(pHist, sInputString);
          /* Select the autofill area */
          if ((int)strlen(sInputString) != nLen)
          {
            pSelStart = &sInputString[nLen];
            nLen = strlen(sInputString);
            pSelEnd = &sInputString[nLen];
            nCurPos = nLen;
          }
        }
        break;
    }
    sInputString[nLen] = '\0';
  }
  while (!bQuit);

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


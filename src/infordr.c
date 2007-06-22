/*

File: infordr.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 14th November, 2000
Descrition:
  GNU Info files reader

*/

#include "global.h"
#include "clist.h"
#include "wlimits.h"
#include "path.h"
#include "memory.h"
#include "disp.h"
#include "palette.h"
#include "l1def.h"
#include "defs.h"
#include "l2disp.h"
#include "bookm.h"  /* for the navigation history */
#include "wrkspace.h"  /* for the navigation history */
#include "bookmcmd.h"  /* to invoke the history viewer */
#include "menu.h"
#include "hypertvw.h"
#include "infordr.h"

/*#define ENABLE_DUMP_FUNCS*/

#define BLOCK_SEARCH_SIZE 4096 /* was tested with 4 */

const char sTagTable[] = "\x1f\nTag Table:";
const char sIndirect[] = "(Indirect)";
const char sIndirectTag[] = "\x1f\nIndirect:";

/* A list of block to load a file in a backward order */
typedef struct _InfoSearchBlock
{
  TListEntry link;
  char Block[BLOCK_SEARCH_SIZE];
  int nBytesRead;
} TInfoSearchBlock;

/* Search context, contains a list of block of the file
loaded in backward order */
typedef struct _InfoSearchBackContext
{
  FILE *f;
  int nPos;
  char *psOccurence;
  TInfoSearchBlock *pstOccBlock;
  TListRoot blist;  /* of TInfoSearchBlock */
} TInfoSearchBackContext;

/* If an info file is sperad into one or more files */
typedef struct _IndirectFileEntry
{
  TListEntry link;
  char sFileName[_MAX_PATH];
  int nOffset;  /* Absolute offset, not as in the root file */
  int nHeaderSize;  /* Cache for the header size (GNU copyright text usually) */
} TIndirectFileEntry;

/* All the nodes loaded from the index and their respective offsets */
typedef struct _Node
{
  TListEntry link;
  char sNode[MAX_NODE_LEN];
  TIndirectFileEntry *pFile;
  int nOffset;  /* absolute in the corespondent file (may need an offset of nHeaderSize) */
} TNodeEntry;

/* TInfoPage to organize the page cache and to contain the text data of a page */
typedef struct _InfoPage
{
  TListRoot blist;  /* List of blocks of text */
  char sPageTitle[MAX_NODE_LEN];  /* Same as in TNodeEntry:sNode */
  char sInfoFile[_MAX_PATH];  /* The root file name */
  int nMRUCount;  /* Cache maintenance */
} TInfoPage;

/* To maintain the cache of files and their respective indices */
typedef struct _InfoFile
{
  char sInfoFile[_MAX_PATH];
  TListRoot ListFileIndex;  /* (TIndirectFileEntry) */
  TListRoot ListPageIndex;  /* (TNodeEntry) */
  int nMRUCount;  /* Cache maintenance */
}
TInfoFile;

static TInfoPage PagesCache[MAX_CACHED_PAGES];
static TInfoFile FilesCache[MAX_CACHED_FILES];
static enum
{
  OPEN_FAILED,
  INDIRECT_FILELIST_NOT_FOUND,
  INDIRECT_FILENAME_TOO_LONG,
  UNEXPECTED_END_OF_DATA,
  INVALID_INDIRECT_FILELIST_FORMAT,
  INDIRECT_FILENAME_INDEX_NUMBER_TOO_LONG,
  INDIRECT_FILENAME_LIST_NO_MEMORY_TO_ADD_ENTRY,
  INDIRECT_FILENAME_INVALID_INDEX_VALUE,
  NODE_NAME_TOO_LONG,
  NODE_INDEX_NUMBER_TOO_LONG,
  NODE_INVALID_OFFSET_VALUE,
  NODE_NO_MEMORY_TO_ADD_ENTRY,
  INFOPAGE_UNABLE_TO_OPEN_FILE,
  INFOPAGE_FILEREAD_ERROR,
  INFOPAGE_NO_MEMORY_TO_LOAD_DATA,
  INFOPAGE_NOT_FOUND
} nError;

BOOLEAN ValStr(const char *s, int *v, char radix);  /* in ini.c */

/* ************************************************************************
   Function: OpenFileSearchBackContext
   Description:
     Prepares search context for FileSeachBack().
*/
static BOOLEAN OpenFileSearchBackContext(TInfoSearchBackContext *pstSearchContext, const char *psFileName)
{
  struct stat statbuf;

  pstSearchContext->f = fopen(psFileName, READ_BINARY_FILE);
  if (pstSearchContext->f == NULL)
  {
    nError = OPEN_FAILED;
    return FALSE;
  }
  INITIALIZE_LIST_HEAD(&pstSearchContext->blist);
  if (stat(psFileName, &statbuf) != 0)
  {
    fclose(pstSearchContext->f);
    nError = OPEN_FAILED;
    return FALSE;  /* unable to acquire file size */
  }
  pstSearchContext->nPos = statbuf.st_size;
  return TRUE;
}

/* ************************************************************************
   Function: DisposeFileSearchBackContext
   Description:
     Disposes a context and the list of loaded block.
*/
static void DisposeFileSearchBackContext(TInfoSearchBackContext *pstSearchContext)
{
  TInfoSearchBlock *pstBlock;

  /* now dispose the list of blocks, close
  the file and exit by reporting failure */
  while (!IS_LIST_EMPTY(&pstSearchContext->blist))
  {
    pstBlock = (TInfoSearchBlock *)REMOVE_HEAD_LIST(&pstSearchContext->blist);
    s_free(pstBlock);
  }
  fclose(pstSearchContext->f);
}

/* ************************************************************************
   Function: FileSearchBack
   Description:
     Search for an occurence of sPattern.
     Searched from the end of the file and loads blocks in blist.
*/
static BOOLEAN FileSearchBack(TInfoSearchBackContext *pstContext,
  const char *psPattern, BOOLEAN *pbFError)
{
  TInfoSearchBlock *pstBlock;
  TInfoSearchBlock *pstBlock2;
  char *p;
  char *s;
  const char *d;
  char *last;

  *pbFError = FALSE;

  do
  {
    if (pstContext->nPos > 0)
    {
      pstContext->nPos -= sizeof(pstBlock->Block);
      if (pstContext->nPos < 0)
        pstContext->nPos = 0;
      if (fseek(pstContext->f, pstContext->nPos, SEEK_SET) != 0)
      {
        *pbFError = TRUE;
        return FALSE;
      }
      pstBlock = alloc(sizeof(TInfoSearchBlock));
      if (pstBlock == NULL)
      {
        *pbFError = TRUE;
        return FALSE;
      }
      pstBlock->nBytesRead = fread(pstBlock->Block, 1, sizeof(pstBlock->Block),
        pstContext->f);
      if (pstBlock->nBytesRead == 0)
      {
        s_free(pstBlock);
        *pbFError = TRUE;
        return FALSE;
      }
      /* Add the block in the list */
      INSERT_TAIL_LIST(&pstContext->blist, &pstBlock->link);
    }
    else
    {
      /*
      There are no more block to read as we got to the
      start of the file.
      Examine the last block inserted (as we read them in
      reverse order starting from the end of the file)
      */
      pstBlock = (TInfoSearchBlock *)pstContext->blist.Blink;
    }
    /* Examine the block for psPattern entry */
    p = pstBlock->Block;
    while (p - pstBlock->Block < pstBlock->nBytesRead)
    {
      if (*p == psPattern[0])
      {
        d = psPattern;
        s = p;
        pstBlock2 = pstBlock;
_block:
        last = pstBlock2->Block + pstBlock2->nBytesRead;
        while (s < last && *d != '\0')
        {
          if (*s++ != *d++)
            goto _continue;
          if (*d == '\0')  /* Complete match */
            goto _sPatternFound;
        }
        /* we need to move "s" at the prev block in the list */
        /* (this the block we have read in the previous read operation) */
        pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
        if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
          goto _continue;  /* this is at the end of the file */
        s = pstBlock2->Block;
        goto _block;
      }
_continue:
      ++p;
    }
  }
  while  (pstContext->nPos > 0);
  return FALSE;

_sPatternFound:
  pstContext->psOccurence = p;
  pstContext->pstOccBlock = pstBlock;
  return TRUE;
}


/* ************************************************************************
   Function: CheckIndirectIndex
   Description:
     Checks whether the index is indirect
     (The next line after "Tag Table:" is "(Indirect)"
*/
static BOOLEAN CheckIndirectIndex(TInfoSearchBackContext *pstContext)
{
  const char *d;
  char *s;
  TInfoSearchBlock *pstBlock2;
  int nLines;
  char *last;

  d = sIndirect;
  s = pstContext->psOccurence;
  pstBlock2 = pstContext->pstOccBlock;
  nLines = 0;
_block:
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  d = sIndirect;
  while (s < last)
  {
    if (*s == '\n')
    {
      ++nLines;
      if (nLines > 2)
        return FALSE;
    }
    if (*s == d[0])
      goto _occurrence;
    ++s;
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    return FALSE;
  s = pstBlock2->Block;
  goto _block;
_occurrence:
  while (s < last && *d != '\0')
  {
    if (*s++ != *d++)
      return FALSE;  /* there's no room for partial coincidence */
    if (*d == '\0')  /* Complete match */
      return TRUE;
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    return FALSE;
  s = pstBlock2->Block;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  goto _occurrence;
}

/* ************************************************************************
   Function: LoadFileIndex
   Description:
     Loads the file index.
     If the file is in indirect mode we have a list of files
     and corespondent offsets.
     We search back from the position of "Tab Table" occurrence
     to find "Indirect:".
*/
static BOOLEAN LoadFileIndex(TInfoSearchBackContext *pstContext,
  TListRoot *pFileIndex)
{
  TInfoSearchBlock *pstSaveBlock;
  char *pSavePointer;
  BOOLEAN bResult;
  BOOLEAN bDummy;
  char *s;
  TInfoSearchBlock *pstBlock2;
  char *last;
  TIndirectFileEntry *pstEntry;
  char sFileName[_MAX_PATH];
  char sNum[10];
  char *d;
  int nLines;

  /* Preserve the point of "Tag Table" occurence */
  pSavePointer = pstContext->psOccurence;
  pstSaveBlock = pstContext->pstOccBlock;

  bResult = FALSE;

  /* Search for "Indirect:" tag */
  if (!FileSearchBack(pstContext, sIndirectTag, &bDummy))
  {
    nError = INDIRECT_FILELIST_NOT_FOUND;
    goto _exit;
  }

  s = pstContext->psOccurence;
  pstBlock2 = pstContext->pstOccBlock;
  nLines = 0;
  /* Skip to the next line (this line is the "Indirect:" tag */
_skiptoeol:
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  while (s < last)
  {
    if (*s == '\n')
    {
      ++nLines;
      if (nLines == 2)
      {
        ++s;
        goto _extract;
      }
    }
    ++s;
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
  {
_unexpected_end_of_data:
    nError = UNEXPECTED_END_OF_DATA;
    goto _exit;
  }
  s = pstBlock2->Block;
  goto _skiptoeol;

  /*
  Extract filenames and offsets
  */
_extract:
  d = sFileName;
_extract_fname:
  if (*s == '\x1f')
    goto _success;  /* end of list */
  while (s < last)
  {
    if (*s == '\n')   /* premature line end */
    {
      nError = INVALID_INDIRECT_FILELIST_FORMAT;
      goto _exit;
    }
    if (*s == ':')
    {
      *d = '\0';  /* make the extracted string ASCIIZ */
      goto _extract_num;
    }
    *d++ = *s++;
    if (d - sFileName == sizeof(sFileName))
    {
      nError = INDIRECT_FILENAME_TOO_LONG;
      goto _exit;  /* name overflow */
    }
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    goto _unexpected_end_of_data;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  s = pstBlock2->Block;
  goto _extract_fname;

_extract_num:
  d = sNum;
_extract_num2:
  while (s < last)
  {
    if (*s == '\n')
    {
      *d = '\0';
      ++s;
      goto _add_entry;
    }
    if (*s >= '0' && *s <= '9')
      *d++ = *s++;
    else
      ++s;
    if (d - sNum == sizeof(sNum))   /* number overflow */
    {
      nError = INDIRECT_FILENAME_INDEX_NUMBER_TOO_LONG;
      goto _exit;
    }
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    goto _unexpected_end_of_data;
  s = pstBlock2->Block;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  goto _extract_num2;

_add_entry:
  pstEntry = alloc(sizeof(TIndirectFileEntry));
  if (pstEntry == NULL)
  {
    nError = INDIRECT_FILENAME_LIST_NO_MEMORY_TO_ADD_ENTRY;
    goto _exit;
  }
  pstEntry->nHeaderSize = -1;  /* Still not scanned */
  if (!ValStr(sNum, &pstEntry->nOffset, 10))
  {
    nError = INDIRECT_FILENAME_INVALID_INDEX_VALUE;
    goto _exit;
  }
  strcpy(pstEntry->sFileName, sFileName);
  INSERT_TAIL_LIST(pFileIndex, &pstEntry->link);
  d = sFileName;  /* to restart the filename extract loop */
  goto _extract_fname;

_success:
  bResult = TRUE;

_exit:
  /* Restore the point of "Tag Table" occurence */
  pstContext->psOccurence = pSavePointer;
  pstContext->pstOccBlock = pstSaveBlock;
  return bResult;
}

#ifdef ENABLE_DUMP_FUNCS
/* ************************************************************************
   Function: DumpSearchBackList
   Description:
*/
static void DumpSearchBackList(TInfoSearchBackContext *pstContext)
{
  #ifdef _DEBUG
  TInfoSearchBlock *pstBlock2;
  int i;

  pstBlock2 = (TInfoSearchBlock *)(pstContext->blist.Blink);
  while (!END_OF_LIST(&pstContext->blist, &pstBlock2->link))
  {
    for (i = 0; i < pstBlock2->nBytesRead; ++i)
      printf("%c", pstBlock2->Block[i]);
    pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  }
  #endif
}
#endif

#ifdef ENABLE_DUMP_FUNCS
/* ************************************************************************
   Function: DumpIndirectFileList
   Description:
*/
static void DumpIndirectFileList(TListRoot *pList)
{
  #ifdef _DEBUG
  TIndirectFileEntry *pstEntry;

  pstEntry = (TIndirectFileEntry *)pList->Flink;
  while (!END_OF_LIST(&pstEntry->link, pList))
  {
    printf("%s: %d\n", pstEntry->sFileName, pstEntry->nOffset);
    pstEntry = (TIndirectFileEntry *)pstEntry->link.Flink;
  }
  #endif
}
#endif

/* ************************************************************************
   Function: DisposeIndirectFileList
   Description:
*/
static void DisposeIndirectFileList(TListRoot *pList)
{
  TInfoSearchBlock *pstEntry;
  while (!IS_LIST_EMPTY(pList))
  {
    pstEntry = (TInfoSearchBlock *)REMOVE_HEAD_LIST(pList);
    s_free(pstEntry);
  }
}

/* ************************************************************************
   Function: GetPageIndex
   Description:
     Loads page index of an info file in memory.
*/
static BOOLEAN GetPageIndex(TInfoSearchBackContext *pstContext,
  TListRoot *pList, TListRoot *pIList)
{
  char *s;
  TInfoSearchBlock *pstBlock2;
  char *last;
  TNodeEntry *pstEntry;
  BOOLEAN bResult;
  char sTitle[MAX_NODE_LEN];
  char *d;
  int nLines;
  char sNum[10];
  int nOffset;
  TIndirectFileEntry *pstIEntry;
  TIndirectFileEntry *pstIEntryNext;
  BOOLEAN bNewLine;

  bResult = FALSE;

  s = pstContext->psOccurence;
  pstBlock2 = pstContext->pstOccBlock;
  nLines = 0;
  bNewLine = FALSE;
  /* Skip 1 or 2 lines (this line and the "(Indirect)" tag if it is indirect table */
_skiptoeol:
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  while (s < last)
  {
    if (bNewLine)
    {
      if (nLines == 2)
        if (*s != '(')
          goto _extract;  /* not an indirect table */
    }
    bNewLine = FALSE;
    if (*s == '\n')
    {
      ++nLines;
      if (nLines == 3)
      {
        ++s;
        goto _extract;
      }
      bNewLine = TRUE;
    }
    ++s;
    /* check at the beginning of line2 whether contains "(Indirect)",
    check only for the opening parenteses */
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
  {
_unexpected_end_of_data:
    nError = UNEXPECTED_END_OF_DATA;
    goto _exit;
  }
  s = pstBlock2->Block;
  goto _skiptoeol;

  /*
  We are at a line of Node: title|sep|index
  */
_extract:
  /* skip to space */
_skiptospace:
  while (s < last)
  {
    if (*s == '\x1f')
      goto _exit;
    if (*s == ' ')
    {
      ++s;
      goto _extract2;
    }
    ++s;
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    goto _unexpected_end_of_data;
  s = pstBlock2->Block;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  goto _skiptospace;

_extract2:
  /* extract node title now */
  d = sTitle;
_extractloop:
  while (s < last)
  {
    if (*s == 127)
    {
      *d = '\0';
      ++s;
      goto _getoffset;
    }
    *d++ = *s++;
    if (d - sTitle == MAX_NODE_LEN)
    {
      nError = NODE_NAME_TOO_LONG;
      goto _exit;  /* title too long */
    }
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    goto _unexpected_end_of_data;
  s = pstBlock2->Block;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  goto _extractloop;

_getoffset:
  /* extract the node offset */
  d = sNum;
_offsetloop:
  while (s < last)
  {
    if (*s == '\n')
    {
      *d = '\0';
      goto _calcabsoffs;
    }
    *d++ = *s++;
    if (d - sNum == sizeof(sNum))
    {
      nError = NODE_INDEX_NUMBER_TOO_LONG;
      goto _exit;  /* Num too long */
    }
  }
  /* we need to move "s" at the prev block in the list */
  /* (this the block we have read in the previous read operation) */
  pstBlock2 = (TInfoSearchBlock *)(pstBlock2->link.Blink);
  if (END_OF_LIST(&pstContext->blist, &pstBlock2->link))
    goto _unexpected_end_of_data;
  s = pstBlock2->Block;
  last = pstBlock2->Block + pstBlock2->nBytesRead;
  goto _offsetloop;

_calcabsoffs:
  if (!ValStr(sNum, &nOffset, 10))
  {
    nError = NODE_INVALID_OFFSET_VALUE;
    goto _exit;
  }
  /* go through the indirect index to find the file
  where this offset points into */
  pstIEntry = NULL;
  if (!IS_LIST_EMPTY(pIList))
  {
    /*
    Walk through the Indirect files list and examine the offset to
    check whether our offset fits in one of the files.
    */
    pstIEntry = (TIndirectFileEntry *)pIList->Flink;
    while (1)
    {
      pstIEntryNext = (TIndirectFileEntry *)pstIEntry->link.Flink;
      if (END_OF_LIST(pIList, &pstIEntryNext->link))
        break;
      if (pstIEntryNext->nOffset > nOffset)
        break;
      pstIEntry = pstIEntryNext;
    }
    nOffset -= pstIEntry->nOffset;
  }
  pstEntry = alloc(sizeof(TNodeEntry));
  if (pstEntry == NULL)
  {
    nError = NODE_NO_MEMORY_TO_ADD_ENTRY;
    goto _exit;
  }
  pstEntry->nOffset = nOffset;
  pstEntry->pFile = pstIEntry;
  strcpy(pstEntry->sNode, sTitle);
  INSERT_TAIL_LIST(pList, &pstEntry->link);
  goto _skiptospace;

_exit:
  return bResult;
}

#ifdef ENABLE_DUMP_FUNCS
/* ************************************************************************
   Function: DumpPageIndex
   Description:
*/
static void DumpPageIndex(TListRoot *pList)
{
  #ifdef _DEBUG
  TNodeEntry *pstEntry;

  pstEntry = (TNodeEntry *)pList->Flink;
  while (!END_OF_LIST(pList, &pstEntry->link))
  {
    printf("%s: %d [%s]\n", pstEntry->pFile->sFileName, pstEntry->nOffset,
      pstEntry->sNode);
    pstEntry = (TNodeEntry *)pstEntry->link.Flink;
  }
  #endif
}
#endif

/* ************************************************************************
   Function: DisposePageIndex
   Description:
*/
static void DisposePageIndex(TListRoot *pList)
{
  TNodeEntry *pstEntry;

  while (!IS_LIST_EMPTY(pList))
  {
    pstEntry = (TNodeEntry *)REMOVE_HEAD_LIST(pList);
    s_free(pstEntry);
  }
}

/* ************************************************************************
   Function: DisposeInfoFile
   Description:
*/
static void DisposeInfoFile(TInfoFile *pstInfoFile)
{
  DisposeIndirectFileList(&pstInfoFile->ListFileIndex);
  DisposePageIndex(&pstInfoFile->ListPageIndex);
  /* reset after disposing all the pertinent data */
  memset(pstInfoFile, 0, sizeof(TInfoFile));
}

/* ************************************************************************
   Function: IsDirFile
   Description:
     Checks whether the file name represents a DIR (directory) info file.
*/
static BOOLEAN IsDirFile(const char *psInfoFileName)
{
  char sName[_MAX_PATH];
  char sPath[_MAX_PATH];

  FSplit(psInfoFileName, sPath, sName, "BAD", TRUE, TRUE);
  if (filestrcmp(sName, "dir") == 0)
    return TRUE;
  return FALSE;
}

/* ************************************************************************
   Function: OpenInfoFile
   Description:
     Searches for specific file in the cache
     if the file is there -- exits.
     Allocates space in the files cache.
     Loads the index of the file.
*/
static BOOLEAN OpenInfoFile(const char *psInfoFileName)
{
  TInfoSearchBackContext stSearchBack;
  BOOLEAN bFError;
  TInfoFile *pstInfoFile;
  int i;
  int c;
  BOOLEAN bResult;
  BOOLEAN bDirFile;
  TIndirectFileEntry *pstInfoEntry;

  bResult = FALSE;

  /* Search for the file in the cache */
  for (i = 0; i < MAX_CACHED_FILES; ++i)
    if (filestrcmp(psInfoFileName, FilesCache[i].sInfoFile) == 0)
    {
      ++FilesCache[i].nMRUCount;
      return TRUE;
    }

  /* Search for a place in the cache */
  for (i = 0; i < MAX_CACHED_FILES; ++i)
  {
    if (FilesCache[i].sInfoFile[0] == '\0')
    {
      pstInfoFile = &FilesCache[i];
      goto _open_file;
    }
  }

  /* The file is not present in the cache and there is no empty
  entry in the cache to accomodate the file. Dispose the
  LRU file */
  c = INT_MAX;
  for (i = 0; i < MAX_CACHED_FILES; ++i)
  {
    if (FilesCache[i].nMRUCount < c)
    {
      pstInfoFile = &FilesCache[i];
      c = pstInfoFile->nMRUCount;
    }
  }
  DisposeInfoFile(pstInfoFile);

_open_file:
  if (!OpenFileSearchBackContext(&stSearchBack, psInfoFileName))
    return FALSE;
  INITIALIZE_LIST_HEAD(&pstInfoFile->ListFileIndex);
  bDirFile = FALSE;
  pstInfoEntry = NULL;
  if (!FileSearchBack(&stSearchBack, sTagTable, &bFError))
  {
    /* Check for DIR file. This file has no index
    create one empty index */
    if (IsDirFile(psInfoFileName))
    {
      bDirFile = TRUE;
      goto _dummy_entry;
    }
    DisposeFileSearchBackContext(&stSearchBack);
    return FALSE;
  }
  if (CheckIndirectIndex(&stSearchBack))
  {
    if (!LoadFileIndex(&stSearchBack, &pstInfoFile->ListFileIndex))
      return FALSE;
    /*DumpIndirectFileList(&pstInfoFile->ListFileIndex);*/
  }
  else
  {
    /*
    Add one dummy entry for the main file
    */
    char sPath[_MAX_PATH];
    char sName[_MAX_PATH];

_dummy_entry:
    pstInfoEntry = s_alloc(sizeof(TIndirectFileEntry));
    if (bDirFile)
      pstInfoEntry->nHeaderSize = -1;  /* scan for "top" node */
    else
      pstInfoEntry->nHeaderSize = 0;  /* no need to scan */
    pstInfoEntry->nOffset = 0;
    FSplit(psInfoFileName, sPath, sName, "BAD", TRUE, TRUE);
    strcpy(pstInfoEntry->sFileName, sName);
    INSERT_TAIL_LIST(&pstInfoFile->ListFileIndex, &pstInfoEntry->link);
  }
  INITIALIZE_LIST_HEAD(&pstInfoFile->ListPageIndex);
  if (bDirFile)
  {
    TNodeEntry *pstEntry;

    /* Insert only one entry for the top page */
    pstEntry = alloc(sizeof(TNodeEntry));
    if (pstEntry == NULL)
    {
      nError = NODE_NO_MEMORY_TO_ADD_ENTRY;
      bResult = FALSE;
      goto _exit;
    }
    pstEntry->nOffset = 0;
    ASSERT(pstInfoEntry != NULL);
    pstEntry->pFile = pstInfoEntry;
    strcpy(pstEntry->sNode, "Top");
    INSERT_TAIL_LIST(&pstInfoFile->ListPageIndex, &pstEntry->link);
  }
  else
  {
    GetPageIndex(&stSearchBack, &pstInfoFile->ListPageIndex,
     &pstInfoFile->ListFileIndex);
  }

  /*
  DumpSearchBackList(&stSearchBack);
  DumpPageIndex(&pstInfoFile->ListPageIndex);
  */
  bResult = TRUE;
  strcpy(pstInfoFile->sInfoFile, psInfoFileName);

_exit:
  if (!bResult)
  {
    /*
    Not successfull at opening the file. Dispose all the allocated space
    */
  }
  DisposeFileSearchBackContext(&stSearchBack);
  return bResult;
}

/* ************************************************************************
   Function: DisposeInfoPage
   Description:
*/
static void DisposeInfoPage(TInfoPage *pstInfoPage)
{
  TInfoPageText *pstInfoText;

  /* now dispose the list of blocks, close
  the file and exit by reporting failure */
  while (!IS_LIST_EMPTY(&pstInfoPage->blist))
  {
    pstInfoText = (TInfoPageText *)REMOVE_HEAD_LIST(&pstInfoPage->blist);
    s_free(pstInfoText);
  }
  /* reset after disposing all the pertinent data */
  memset(pstInfoPage, 0, sizeof(TInfoPage));
}

/* ************************************************************************
   Function: OpenInfoPage
   Description:
     Searches for specific page in the cache
     if the page is there -- exits.
     Allocates space in the pages cache.
     Loads the page.
*/
static BOOLEAN OpenInfoPage(const char *psInfoFile, const char *psTitle, TInfoPage **_ppstInfoPage)
{
  int i;
  int c;
  TInfoPage *pstInfoPage;
  TInfoPageText *pstInfoText;
  TInfoFile *pstInfoFile;
  TNodeEntry *pstEntry;
  char Buf[25];
  int n;
  int nHeader;
  int nMarkers;
  int nAbsPos;
  FILE *f;
  char sPath[_MAX_PATH];
  char sDummy[_MAX_PATH];
  char sComposedName[_MAX_PATH];
  char *p;

  /* Search for this specific page topic and info file in the Pages cache */
  for (i = 0; i < MAX_CACHED_PAGES; ++i)
  {
    if (filestrcmp(PagesCache[i].sInfoFile, psInfoFile) == 0)
      if (strcmp(PagesCache[i].sPageTitle, psTitle) == 0)
      {
        ++PagesCache[i].nMRUCount;
        *_ppstInfoPage = &PagesCache[i];
        return TRUE;
      }
  }

  /* The page is not in the cache. Try to allocate an empty
  entry to load the page there */
  for (i = 0; i < MAX_CACHED_PAGES; ++i)
  {
    if (PagesCache[i].sPageTitle[0] == '\0')
    {
      pstInfoPage = &PagesCache[i];
      goto _load_page;
    }
  }

  /* There is no empty entry in the cache. Dispose the LRU entry */
  c = INT_MAX;
  for (i = 0; i < MAX_CACHED_PAGES; ++i)
  {
    if (PagesCache[i].nMRUCount < c)
    {
      pstInfoPage = &PagesCache[i];
      c = pstInfoPage->nMRUCount;
    }
  }
  DisposeInfoPage(pstInfoPage);

_load_page:
  /* Find the file in the cache */
  pstInfoFile = NULL;
  for (i = 0; i < MAX_CACHED_FILES; ++i)
    if (filestrcmp(psInfoFile, FilesCache[i].sInfoFile) == 0)
    {
      pstInfoFile = &FilesCache[i];
      break;
    }

  ASSERT(pstInfoFile != NULL);

  /*
  Look for the page in the page index
  */
  pstEntry = (TNodeEntry *)pstInfoFile->ListPageIndex.Flink;
  while (!END_OF_LIST(&pstInfoFile->ListPageIndex, &pstEntry->link))
  {
    /*printf("%s: %d [%s]\n", pstEntry->pFile->sFileName, pstEntry->nOffset,
      pstEntry->sNode);*/
    if (strcmp(pstEntry->sNode, psTitle) == 0)
      goto _open_file;
    pstEntry = (TNodeEntry *)pstEntry->link.Flink;
  }
  nError = INFOPAGE_NOT_FOUND;
  return FALSE;

_open_file:
  FSplit(psInfoFile, sPath, sDummy, "BAD", TRUE, TRUE);
  strcpy(sComposedName, sPath);
  AddTrailingSlash(sComposedName);
  strcat(sComposedName, pstEntry->pFile->sFileName);
  f = fopen(sComposedName, "r");
  if (f == NULL)
  {
    nError = INFOPAGE_UNABLE_TO_OPEN_FILE;
    return FALSE;
  }
  /* Check to see whether the nHeader offset is established */
  if (pstEntry->pFile->nHeaderSize == -1)
  {
    /*
    Open the file and try to find the first
    node marker (0x1f)
    */
    nHeader = 0;
    while (1)
    {
      n = fread(Buf, 1, sizeof(Buf) - 1, f);
      if (n == 0)
      {
_fail:
        nError = INFOPAGE_FILEREAD_ERROR;
        fclose(f);
        return FALSE;
      }
      Buf[n] = '\0';
      p = strchr(Buf, '\x1f');
      if (p != NULL)
      {
        nHeader += p - Buf;
        pstEntry->pFile->nHeaderSize = nHeader;
        break;
      }
      nHeader += n;
    }
  }

  /* Load the page in a list of blocks */
  if (fseek(f, pstEntry->pFile->nHeaderSize + pstEntry->nOffset, SEEK_SET) != 0)
    goto _fail;
  nMarkers = 0;
  INITIALIZE_LIST_HEAD(&pstInfoPage->blist);
  nAbsPos = 0;
  /* We read until we have the start marker of the next page */
  while (nMarkers < 2)
  {
    pstInfoText = alloc(sizeof(TInfoPageText));
    if (pstInfoText == NULL)
    {
      nError = INFOPAGE_NO_MEMORY_TO_LOAD_DATA;
      goto _fail;
    }
    #ifdef _DEBUG
    pstInfoText->MagicByte = INFOTEXT_MAGIC;
    #endif
    pstInfoText->nBytesRead = fread(pstInfoText->Text, 1, sizeof(pstInfoText->Text), f);
    pstInfoText->nBlockPos = nAbsPos;
    nAbsPos += pstInfoText->nBytesRead;
    p = pstInfoText->Text;
    while (p - pstInfoText->Text < pstInfoText->nBytesRead)
    {
      if (*p++ == '\x1f')
        ++nMarkers;
    }
    /* Add the block in the list */
    INSERT_TAIL_LIST(&pstInfoPage->blist, &pstInfoText->link);
    if (pstInfoText->nBytesRead == 0)
      break;
  }
  strcpy(pstInfoPage->sInfoFile, psInfoFile);
  strcpy(pstInfoPage->sPageTitle, psTitle);
  *_ppstInfoPage = pstInfoPage;
  return TRUE;
}

#ifdef ENABLE_DUMP_FUNCS
/* ************************************************************************
   Function: DumpInfoPage
   Description:
*/
static void DumpInfoPage(TListRoot *pList)
{
  #ifdef _DEBUG
  TInfoPageText *pstEntry;
  int i;

  pstEntry = (TInfoPageText *)pList->Flink;
  while (!END_OF_LIST(pList, &pstEntry->link))
  {
    ASSERT(VALID_INFOTEXTBLOCK(pstEntry));
    for (i = 0; i < pstEntry->nBytesRead; ++i)
      printf("%c", pstEntry->Text[i]);
    pstEntry = (TInfoPageText *)pstEntry->link.Flink;
  }
  #endif
}
#endif

/* ************************************************************************
   Function: SeparateLinkComponents
   Description:
     Full link format is display::link
*/
static void SeparateLinkComponents(const char *psLink,
  char *psFile, char *psDest, char *psDisplay)
{
  char *p;
  char *b;
  char *b2;

  *psFile = '\0';
  p = strchr(psLink, ':');
  if (p == NULL)
  {
    strcpy(psDest, psLink);
    strcpy(psDisplay, psLink);
    return;
  }
  if (*(p + 1) == ':')  /* double ::, short link */
  {
    strncpy(psDest, psLink, p - psLink);
    *(psDest + (p - psLink)) = '\0';
  }
  else  /* full version, copy the destination */
  {
    /* Check for file name */
    b = strchr(p, '(');
    if (b == NULL)  /* no file name */
_title:
      strcpy(psDest, p + 2);  /* skip ": " */
    else
    {
      b2 = strchr(b, ')');
      if (b2 == NULL)
        goto _title;
      strncpy(psFile, b + 1, b2 - b - 1);
      psFile[b2 - b - 1] = '\0';
      if (strchr(psFile, '.') == NULL)
        strcat(psFile, ".info");
      strcpy(psDest, b2 + 1);
      if (psDest[0] == '\0')
        strcpy(psDest, "Top");
      return;
    }
  }
  strncpy(psDisplay, psLink, p - psLink);
  *(psDisplay + (p - psLink)) = '\0';
}

/* ************************************************************************
   Function: InfoPageExists
   Description:
     Checks for specific page in an info file.
     If the page exists it is readily available in the cache
     for subsequent NavigateInfoPage() use.
*/
BOOLEAN InfoPageExists(const char *psInfoFile, const char *psTitle)
{
  char sExpandedPath[_MAX_PATH];
  TInfoPage *pstInfoPage;

  strcpy(sExpandedPath, psInfoFile);
  GetFullPath(sExpandedPath);
  if (!OpenInfoFile(sExpandedPath))
    return FALSE;
  if (!OpenInfoPage(sExpandedPath, psTitle, &pstInfoPage))
    return FALSE;
  return TRUE;
}

static TKeySequence ExtraNavigation[] =
{
  {1, DEF_KEY1(KEY(kbAlt, kbLeft))},
  {2, DEF_KEY1(KEY(kbAlt, kbRight))},
  {3, DEF_KEY1(KEY(kbAlt, kbUp))},
  {4, DEF_KEY1(KEY(kbAlt, kbHome))},
  {5, DEF_KEY1(KEY(kbShift, kbF10))},
  {6, DEF_KEY1(KEY(kbAlt, kbC))},
  {7, DEF_KEY1(KEY(kbAlt, kbD))},
  {8, DEF_KEY1(KEY(kbCtrl, kbH))},
  {END_OF_KEY_LIST_CODE}  /* LastItem */
};

static TKeySet ExtraKeySet =
{
  ExtraNavigation,
  _countof(ExtraNavigation) - 1,
  TRUE
};

typedef struct _NavContext
{
  int nDest;
} TNavContext;

static TMenuItem itBack = {1, 0, "~B~ack", {0}, 0, 0};
static TMenuItem itForward = {2, 0, "~F~orward", {0}, 0, 0};
static TMenuItem itUp = {3, 0, "~U~p", {0}, 0, 0};
static TMenuItem itTop = {4, 0, "~T~op", {0}, 0, 0};
static TMenuItem itHistory = {8, 0, "~H~istory", {0}, 0, 0};
static TMenuItem itContents = {6, 0, "~C~ontents", {0}, 0, 0};
static TMenuItem itDirectory = {7, 0, "~D~irectory", {0}, 0, 0};
static TMenuItem itMain = {9, 0, "~M~ain", {0}, 0, 0};

static TMenuItem *aContext[] =
{
  &itBack, &itForward, &itUp,
  &itSep,
  &itTop,
  &itHistory, &itContents,
  &itDirectory, &itMain
};

static TMenu meContext = {MEVERTICAL, _countof(aContext), (TMenuItem *(*)[])&aContext, 0, 0, coMenu};

typedef struct _NavCtx
{
  int nCode;
  char sNewFile[_MAX_PATH];
  char sNewPage[_MAX_PATH];
  int nNewRow;
  int nNewCol;
  int nNewTopLine;
} TNavCtx;

/* ************************************************************************
   Function: NavProc
   Description:
     Call-back function. Called by NavigatePage when any
     of the commands in ExtraNavigation[] is activated.

     Return negative values: this will exit the hypertext viewer
     and will return that value to NavigateInfoPageEx.

     Return 1: the screen was contaminated and needs complete redraw.

     Return 0: no action
*/
static int NavProc(struct _InfoPageContext *pGlobalCtx, int nCmd)
{
  int x;
  int y;
  TNavCtx *pCtx;

  pCtx = pGlobalCtx->pContext;
  switch (nCmd)  /* As mapped in ExtraNavigation() */
  {
    case 1:  /* Alt+Left: go back */
      break;

    case 2:  /* Alt+Right: go forward */
      break;

    case 3:  /* Alt+Up: go up */
      break;

    case 4:  /* Alt+Home: go to the main info page */
      break;

    case 5:  /* Shift+F10: show context menu */
      disp_cursor_get_xy(pGlobalCtx->disp, &x, &y);
      meContext.disp = pGlobalCtx->disp;
      switch (Menu(&meContext, x, y, 0, 0))
      {
        case 8:
          goto _show_history;
      }
      break;

    case 8:
_show_history:
      if (SelectHelpHistory(pCtx->sNewFile, pCtx->sNewPage,
        &pCtx->nNewRow, &pCtx->nNewCol, &pCtx->nNewTopLine))
      {
        pCtx->nCode = -8;  /* Display a new page */
        return -1;  /* Exit the navigation function */
      }
      else
        return 1;
      break;
  }

  return 0;
}

/* ************************************************************************
   Function: AddHistoryEntry
   Description:
     Adds an entry to the hyper-text navigation history.
*/
static void AddHistoryEntry(const char *psInfoFile, const char *psTitle,
  int nRow, int nCol, int nTopLine)
{
  TMarkLocation *pMark;
  TMarkLocation *pNewMark;

  pMark = stInfoHistory.pFirst;
  BMListInsert(psInfoFile, nRow, nCol, nTopLine, psTitle, NULL, 0,
    &stInfoHistory, &pNewMark, 0);
  pNewMark->pNext = pMark;  /* Put the new mark on top */
  stInfoHistory.pFirst = pNewMark;
}

/* ************************************************************************
   Function: NavigateInfoPageEx
   Description:
     Loads the file (OpenInfoFile() will load a file if not present
     in the files cache).
     Loads a specific page (OpenInfoPage() will load a page if not
     present in the pages cache).

     nDest* parameters if not set to -1 specify a specific
     position in the page to be displayed as a startup point.
*/
void NavigateInfoPageEx(const char *psInfoFile, const char *psTitle,
  int nDestRow, int nDestCol, int nDestTopLine, BOOLEAN bHalfScreen,
  dispc_t *disp)
{
#if 0
  char sExpandedPath[_MAX_PATH];
  char sFullLink[MAX_NODE_LEN * 3];
  char sTitle[MAX_NODE_LEN * 3];
  char sDestLink[MAX_NODE_LEN];
  char sDisplay[MAX_NODE_LEN];
  char sInfoFileNameOnly[_MAX_PATH];
  char sBuf[_MAX_PATH];
  char sDestInfoFile[_MAX_PATH];
  char sPath[_MAX_PATH];
  TInfoPage *pstInfoPage;
  TInfoPageContext stPgCtx;
  int nResult;
  char *p;
  char cSep;
  disp_char_buf_t *pStatLineBuf;
  extern void _PutShortCutKeys(TMenu *pMenu, const TKeySet *pKeySet);  /* menudat.c */
  BOOLEAN bRestore;
  TNavCtx stNavCtx;
  int rectangle_size;

  _PutShortCutKeys(&meContext, &ExtraKeySet);

  /* save the screen */
  bRestore = FALSE;
  rectangle_size =
    disp_calc_rect_size(disp,
                        disp_wnd_get_width(disp), disp_wnd_get_width(disp));
  pStatLineBuf = s_alloc(rectangle_size);
  disp_cbuf_reset(disp, pStatLineBuf, rectangle_size);
  disp_get_block(disp,
                 0, 0, disp_wnd_get_width(disp), disp_wnd_get_width(disp),
                 pStatLineBuf);

  strcpy(sExpandedPath, psInfoFile);
  strcpy(sTitle, psTitle);

_open_file:
  GetFullPath(sExpandedPath);
  if (!OpenInfoFile(sExpandedPath))
  {
    ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, sExpandedPath, "(filename): %s", sInfoFileErrors[nError]);
    goto _exit;
  }
  if (!OpenInfoPage(sExpandedPath, sTitle, &pstInfoPage))
  {
    ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, NULL, "(filename)|%s: %s", sTitle, sInfoFileErrors[nError]);
    goto _exit;
  }
  strcpy(sDestLink, sTitle);  /* to be displauied on the status line */
_navigate_page:
  memset(&stPgCtx, '\0', sizeof(stPgCtx));
  stPgCtx.pfnNavProc = NavProc;
  stPgCtx.pCmdKeyMap = ExtraNavigation;
  stPgCtx.pContext = &stNavCtx;
  stPgCtx.nDestTopLine = -1;
  if (nDestTopLine != -1)
  {
    stPgCtx.nDestTopLine = nDestTopLine;
    stPgCtx.nDestRow = nDestRow;
    stPgCtx.nDestCol = nDestCol;
    nDestTopLine = -1;
    nDestRow = -1;
    nDestCol = -1;
  }

  stPgCtx.x1 = 0;
  stPgCtx.y1 = 0;
  stPgCtx.x2 = CurWidth;
  stPgCtx.y2 = CurHeight - 1;
  if (bHalfScreen)
  {
    stPgCtx.y1 = CurHeight / 2 + 1;
    /*
    Display a separator line
    */
    {
      BYTE buff[512];
      int w = ScreenWidth - 1;
      #if USE_ASCII_BOXES
      memset(buff, '-', 512);
      buff[w + 1] = (BYTE)'\0';  /* End of string marker */
      #else
      memset(buff, 'Ä', 512);
      buff[w + 1] = (BYTE)'\0';  /* End of string marker */
      #endif
      WriteXY((char *)buff, 0, stPgCtx.y1 - 1, GetColor(coHelpText));
    }
  }
  stPgCtx.blist = &pstInfoPage->blist;
  stPgCtx.psLink = sFullLink;
  stPgCtx.nMaxLinkSize = sizeof(sFullLink);
  /*
  Display the status line
  */
  #if USE_ASCII_BOXES
  cSep = '|';
  #else
  cSep = '³';
  #endif
  FSplit(sExpandedPath, sBuf, sInfoFileNameOnly, "BAD", TRUE, TRUE);
  /* remove the .info component from the name */
  p = strchr(sInfoFileNameOnly, '.');
  if (p != NULL)
    *p = '\0';
  sprintf(sBuf, sStatInfordr, sInfoFileNameOnly, sDestLink, cSep);
  sBuf[CurWidth] = '\0';  /* make it fit in the status line */
  DisplayStatusStr2(sBuf, coStatusTxt, coStatusShortCut);

  /*
  Invoke the hypertext viewer and analize the result of the navigation
  */
  nResult = NavigatePage(&stPgCtx);
  nDestTopLine = -1;  /* Presume that no history was activated */
  AddHistoryEntry(sExpandedPath, sDestLink,
    stPgCtx.nRow, stPgCtx.nCol, stPgCtx.nTopLine);
  bRestore = TRUE;  /* the screen is dirty */
  if (nResult != 0)
  {
    sDestInfoFile[0] = '\0';
    if (nResult == 1)
      stNavCtx.nCode = 1;
    switch (stNavCtx.nCode)
    {
      case -8: /* A page from the history was selected */
        strcpy(sDestInfoFile, stNavCtx.sNewFile);
        strcpy(sTitle, stNavCtx.sNewPage);
        nDestTopLine = stNavCtx.nNewTopLine;
        nDestRow = stNavCtx.nNewRow;
        nDestCol = stNavCtx.nNewCol;
        break;

      case 1:  /* User pressed <Enter> to select a link */
        SeparateLinkComponents(stPgCtx.psLink, sDestInfoFile, sDestLink, sDisplay);
        break;
    }
    if (sDestInfoFile[0] != '\0')
    {
      strcpy(sExpandedPath, sDestInfoFile);
      /* Check for full-path in sDestInfoFile */
      if (sDestInfoFile[0] == PATH_SLASH_CHAR)
        goto _open_file;
      if (sDestInfoFile[1] == ':' && sDestInfoFile[2] == '\\')
        goto _open_file;
      /* The file path should be composed out of the current file path */
      /* 1. Get the path of the current file                           */
      FSplit(sExpandedPath, sPath, sBuf, "BAD", TRUE, TRUE);
      AddTrailingSlash(sPath);
      strcat(sPath, sDestInfoFile);
      strcpy(sExpandedPath, sPath);
      goto _open_file;
    }
    if (!OpenInfoPage(sExpandedPath, sDestLink, &pstInfoPage))
    {
      ConsoleMessageProc(NULL, MSG_ERROR | MSG_OK, sExpandedPath,
        "(filename)|%s: %s", sDestLink, sInfoFileErrors[nError]);
      goto _exit;
    }
    goto _navigate_page;
  }
_exit:
  if (bRestore)
  {
    /* restore the screen */
    puttextblock(0, 0, CurWidth, CurHeight, pStatLineBuf);
  }
  s_free(pStatLineBuf);
  return;
#endif
}

/* ************************************************************************
   Function: NavigateInfoPage
   Description:
     Loads the file (OpenInfoFile() will load a file if not present
     in the files cache).
     Loads a specific page (OpenInfoPage() will load a page if not
     present in the pages cache).
*/
void NavigateInfoPage(const char *psInfoFile, const char *psTitle,
  BOOLEAN bHalfScreen)
{
#if 0
  NavigateInfoPageEx(psInfoFile, psTitle, -1, -1, -1, bHalfScreen);
#endif
}

/* ************************************************************************
   Function: DisposeInfoPagesCache
   Description:
     Disposes all cached entries for the file and the page caches.
*/
void DisposeInfoPagesCache(void)
{
  int i;
  TInfoFile *pstInfoFile;
  TInfoPage *pstInfoPage;

  for (i = 0; i < MAX_CACHED_FILES; ++i)
  {
    pstInfoFile = &FilesCache[i];
    if (pstInfoFile->sInfoFile[0] != '\0')
      DisposeInfoFile(pstInfoFile);
  };

  for (i = 0; i < MAX_CACHED_PAGES; ++i)
  {
    pstInfoPage = &PagesCache[i];
    if (pstInfoPage->sPageTitle[0] != '\0')
      DisposeInfoPage(pstInfoPage);
  }
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


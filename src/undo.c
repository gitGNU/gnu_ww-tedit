/*

File: undo.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 6th August, 1999
Descrition:
  Undo/Redo functions; Safe recovery file;

*/

#include "global.h"
#include "l1def.h"
#include "nav.h"
#include "heapg.h"
#include "memory.h"
#include "block.h"
#include "ini.h"  /* ValStr() */
#include "undo.h"

/* ************************************************************************
   Function: InitEmptyUndoRecord
   Description:
     Initial setup of some fields.
     Puts a magic byte memory stamp in _DEBUG mode of compilation.
*/
static void InitEmptyUndoRecord(TUndoRecord *pUndoRec)
{
  ASSERT(pUndoRec != 0);

  memset(pUndoRec, -1, sizeof(TUndoRecord));

  #ifdef _DEBUG
  pUndoRec->MagicByte = UNDOREC_MAGIC;
  #endif

  pUndoRec->bRecoveryStore = FALSE;
  pUndoRec->bUndone = FALSE;
  pUndoRec->pData = NULL;
}

/* ************************************************************************
   Function: RecordFileStatus
   Description:
     Makes a snapshot of the current file status.
*/
static void RecordFileStatus(const TFile *pFile, TFileStatus *pStat)
{
  ASSERT(VALID_PFILE(pFile));

  pStat->nRow = pFile->nRow;
  pStat->nCol = pFile->nCol;

  pStat->nTopLine = pFile->nTopLine;
  pStat->nWrtEdge = pFile->nWrtEdge;

  pStat->nNumberOfLines = pFile->nNumberOfLines;

  pStat->nStartLine = pFile->nStartLine;
  pStat->nEndLine = pFile->nEndLine;
  pStat->nStartPos = pFile->nStartPos;
  pStat->nEndPos = pFile->nEndPos;
  pStat->blockattr = pFile->blockattr;
  pStat->bBlock = pFile->bBlock;

  pStat->bChanged = pFile->bChanged;
}

/* ************************************************************************
   Function: RestoreFileStatus
   Description:
     Restores the status of the current file as described in pStat.
*/
static void RestoreFileStatus(TFile *pFile, const TFileStatus *pStat)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(pStat != NULL);
  ASSERT(pStat->nCol >= 0);
  ASSERT(pStat->nRow >= 0);
  ASSERT(pStat->nTopLine >= 0);

  ASSERT(pFile->nNumberOfLines == pStat->nNumberOfLines);

  pFile->nWrtEdge = pStat->nWrtEdge;
  pFile->nTopLine = pStat->nTopLine;
  GotoColRow(pFile, pStat->nCol, pStat->nRow);

  pFile->nStartLine = pStat->nStartLine;
  pFile->nStartPos = pStat->nStartPos;
  pFile->nEndLine = pStat->nEndLine;
  pFile->nEndPos = pStat->nEndPos;
  pFile->blockattr = pStat->blockattr;
  pFile->bBlock = pStat->bBlock;

  pFile->bChanged = pStat->bChanged;
  pFile->bRecoveryStored = FALSE;

  pFile->bUpdatePage = TRUE;
}

/* ************************************************************************
   Function: GetLastRec
   Description:
     Returns the record that was last added to undo index.
*/
static TUndoRecord *GetLastRec(const TFile *pFile)
{
  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->nUndoLevel >= 0);
  ASSERT(pFile->nNumberOfRecords >= 0);

  return &(pFile->pUndoIndex[pFile->nNumberOfRecords - 1]);
}

/* ************************************************************************
   Function: PreOperation
   Description:
     Extract pre operation information from current file. The
     information is stored in the top undo record.
*/
static void PreOperation(TFile *pFile, int nOperation)
{
  TUndoRecord *pUndoRec;

  ASSERT(VALID_PFILE(pFile));

  pUndoRec = GetLastRec(pFile);
  RecordFileStatus(pFile, &pUndoRec->before);
  pUndoRec->nOperation = nOperation;
}

/* ************************************************************************
   Function: DisposeUndoRecord
   Description:
*/
static void DisposeUndoRecord(TUndoRecord *pUndoRec)
{
  ASSERT(VALID_PUNDOREC(pUndoRec));

  if (pUndoRec->pData != NULL)
  {
    switch (pUndoRec->nOperation)
    {
      case acINSERT:  /* Block of text is inserted */
      case acDELETE:  /* Block of text is deleted */
      case acREPLACE:  /* Text from the current line is replaced (OVR mode) */
        DisposeABlock((TBlock **)&pUndoRec->pData);
        break;
      case acREARRANGE:  /* Most likely as result from CmdEditSort() */
      case acREARRANGEBACK:  /* Can appear only from a recovery file */
        ASSERT(pUndoRec->pData != NULL);
        s_free(pUndoRec->pData);
        break;
      default:
        ASSERT(0);
    }
  }

  #ifdef _DEBUG
  pUndoRec->MagicByte = UNDOREC_MAGIC - 1;
  #endif
}

/* ************************************************************************
   Function: RemoveUndoneBlocks
   Description:
     Removes all the undone block from the undo record index.
     This function should call StoreRecovery() if comes across
     a block with with bFileSaved or bRecoveryStore set. Because actually Undo
     made changes against the file on the disk and this should
     be registered in the recovery file immediately as the undo
     blocks will be lost.
*/
static void RemoveUndoneBlocks(TFile *pFile)
{
  int i;
  TUndoRecord *pUndoRec;
  BOOLEAN bUndone;
  BOOLEAN bLoop2;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->nNumberOfRecords == 0)
    return;

  /*
  First: Remove all undone blocks that have bRecoveryStored not set.
  */
  i = pFile->nNumberOfRecords;
  bLoop2 = FALSE;
  do
  {
    --i;
    pUndoRec = &(pFile->pUndoIndex[i]);
    bUndone = pUndoRec->bUndone;  /* Will be necessary after the record is disposed */
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (bUndone)  /* Dispose the undone blocks */
    {
      if (pUndoRec->bRecoveryStore)  /* We are at point where Undo differs from disk file */
      {
        bLoop2 = TRUE;
        break;
      }
      DisposeUndoRecord(pUndoRec);
    }
  }
  while (bUndone && i > 0);

  if (bUndone && !bLoop2)
    --i;  /* first block is undone too, now let i = -1 */

  if (i + 1 < pFile->nNumberOfRecords)
  {
    /*
    Remove the entries from the undo records index.
    */
    TArrayDeleteGroup(pFile->pUndoIndex, (i + 1), pFile->nNumberOfRecords - (i + 1));
    pFile->nNumberOfRecords -= pFile->nNumberOfRecords - (i + 1);
  }
  #if 1  /* should be comment out unless a heavy duty test takes is running */
  CHECK_VALID_UNDO_LIST(pFile);
  #endif

  if (!bLoop2)
    return;

  /*
  Second: Call StoreRecoveryRecord() to store all the blocks (those having
  bRecoveryStored set will be stored as inverted operation) and then remove
  all the remaining blocks that have bUndone flag set.
  */
  pFile->bRecoveryStored = StoreRecoveryRecord(pFile);
  i = pFile->nNumberOfRecords;
  do
  {
    --i;
    pUndoRec = &(pFile->pUndoIndex[i]);
    bUndone = pUndoRec->bUndone;  /* Will be necessary after the record is disposed */
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (bUndone)  /* Dispose the undone blocks */
      DisposeUndoRecord(pUndoRec);
  }
  while (bUndone && i > 0);

  if (bUndone)
    --i;  /* first block is undone too, now let i = -1 */

  if (i + 1 < pFile->nNumberOfRecords)
  {
    /*
    Remove the entries from the undo records index.
    */
    TArrayDeleteGroup(pFile->pUndoIndex, (i + 1), pFile->nNumberOfRecords - (i + 1));
    pFile->nNumberOfRecords -= pFile->nNumberOfRecords - (i + 1);
  }
  #if 1  /* should be comment out unless a heavy duty test takes is running */
  CHECK_VALID_UNDO_LIST(pFile);
  #endif
}

/* ************************************************************************
   Function: AddUndoRecord
   Description:
     Adds a new element to the Undo record index.
     Fails if no enough memory.
     bLoad indicates that the data is loaded from a recovery
     file and some checks and conditions should be skipped.
*/
BOOLEAN AddUndoRecord(TFile *pFile, int nOperation, BOOLEAN bLoad)
{
  TUndoRecord UndoRec;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pUndoIndex == NULL)
  {
    /*
    pUndoIndex is not created. Create the index.
    */
    ASSERT(pFile->nNumberOfRecords == 0);
    TArrayInit(pFile->pUndoIndex, 50, UNDOINDEX_DELTA);
    if (pFile->pUndoIndex == NULL)
      return FALSE;
  }

  ASSERT(pFile->pUndoIndex != NULL);
  ASSERT(pFile->nUndoLevel > 0);
  ASSERT(pFile->nNumberOfRecords >= 0);

  if (!bLoad)
    RemoveUndoneBlocks(pFile);

  /*
  Add an empty undo record to the index
  */
  InitEmptyUndoRecord(&UndoRec);
  UndoRec.nUndoBlockID = pFile->nUndoIDCounter++;
  TArrayAdd(pFile->pUndoIndex, UndoRec);

  if (!TArrayStatus(pFile->pUndoIndex))
  {
    TArrayClearStatus(pFile->pUndoIndex);
    return FALSE;
  }

  ++pFile->nNumberOfRecords;

  PreOperation(pFile, nOperation);

  return TRUE;
}

/* ************************************************************************
   Function: RecordUndoData
   Description:
     _Rearrange_ operation adds an index of rearrange order,
     all other operations add a block of text;
*/
void RecordUndoData(TFile *pFile, void *pBlock,
  int nStartLine, int nStartPos, int nEndLine, int nEndPos)
{
  TUndoRecord *pUndoRec;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pBlock != NULL);
  ASSERT(nStartLine >= 0);
  ASSERT(nEndLine >= nStartLine);

  pUndoRec = GetLastRec(pFile);
  pUndoRec->pData = pBlock;
  pUndoRec->nStart = nStartLine;
  pUndoRec->nStartPos = nStartPos;
  pUndoRec->nEnd = nEndLine;
  pUndoRec->nEndPos = nEndPos;
}

/* ************************************************************************
   Function: CombineUndoBlocks
   Description:
     Combines the top two blocks in the undo index list:
     1. Combines undo blocks data
     2. Combines new block data coordinates (nStart, nStartPos, nEnd, nEndPos)
     3. Combines undo blocks 'before' and 'after' snapshots information.
     4. Inserts	this new block in the undo block lists
     5. Removes the old 2 blocks.
*/
static void CombineUndoBlocks(TFile *pFile, BOOLEAN bStright)
{
  TUndoRecord UndoRec;
  TBlock *pBlock;
  TUndoRecord *pUndoLast;
  TUndoRecord *pUndoPrev;
  TLine *pLine1;
  TLine *pLine2;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->nNumberOfRecords < 2)
    return;  /* Only one block in the undo index list */

  pUndoLast = &pFile->pUndoIndex[pFile->nNumberOfRecords - 1];
  pUndoPrev = &pFile->pUndoIndex[pFile->nNumberOfRecords - 2];

  ASSERT(VALID_PUNDOREC(pUndoLast));
  ASSERT(VALID_PUNDOREC(pUndoPrev));
  ASSERT(!pUndoPrev->bUndone);  /* As all undone blocks are already removed */
  ASSERT(((TBlock *)pUndoLast->pData)->nNumberOfLines == 1);
  ASSERT(((TBlock *)pUndoPrev->pData)->nNumberOfLines == 1);

  InitEmptyUndoRecord(&UndoRec);
  UndoRec.nUndoBlockID = pFile->nUndoIDCounter++;

  pLine1 = GetBlockLine((TBlock *)pUndoPrev->pData, 0);
  pLine2 = GetBlockLine((TBlock *)pUndoLast->pData, 0);
  if (bStright)
  {
    pBlock = DuplicateBlock((TBlock *)pUndoPrev->pData, pFile->nEOLType, 0,
      pLine2->nLen, 0, NULL);
    strcat(pBlock->pBlock, pLine2->pLine);
    UndoRec.nStart = pUndoPrev->nStart;
    UndoRec.nStartPos = pUndoPrev->nStartPos;
    UndoRec.nEnd = pUndoLast->nEnd;
    UndoRec.nEndPos = pUndoLast->nEndPos;
  }
  else
  {
    pBlock = DuplicateBlock((TBlock *)pUndoLast->pData, pFile->nEOLType, 0, 
      pLine1->nLen, 0, NULL);
    strcat(pBlock->pBlock, pLine1->pLine);
    UndoRec.nStart = pUndoLast->nStart;
    UndoRec.nStartPos = pUndoLast->nStartPos;
    UndoRec.nEnd = pUndoPrev->nEnd;
    UndoRec.nEndPos = pUndoPrev->nEndPos;
  }
  GetBlockLine(pBlock, 0)->nLen = strlen(pBlock->pBlock);
  UndoRec.pData = pBlock;
  UndoRec.nOperation = pUndoPrev->nOperation;
  UndoRec.nUndoLevel = pUndoLast->nUndoLevel;
  memcpy(&UndoRec.before, &pUndoPrev->before, sizeof(TFileStatus));
  memcpy(&UndoRec.after, &pUndoLast->after, sizeof(TFileStatus));
  if (pUndoPrev->nStartPos == pUndoLast->nStartPos)
  {
    /*
    The case of deleting something at the same position.
    Example: Pressing <Del> key multiple times to remove characters.
    Make the range pUndo->nStartPos..pUndo->nEndPos to comprise the
    whole combined block!
    */
    UndoRec.nEndPos = UndoRec.nEndPos +
      (pUndoPrev->nEndPos - pUndoLast->nStartPos + 1)
      + (pUndoLast->nEndPos - pUndoLast->nStartPos);
  }

  /*
  Remove 2 undo entries from the top of undo list
  */
  DisposeUndoRecord(pUndoLast);
  DisposeUndoRecord(pUndoPrev);
  TArrayDeleteGroup(pFile->pUndoIndex, pFile->nNumberOfRecords - 2, 2);
  pFile->nNumberOfRecords -= 2;

  /*
  Add the new record at the top of the undo list index
  */
  TArrayAdd(pFile->pUndoIndex, UndoRec);
  if (!TArrayStatus(pFile->pUndoIndex))
  {
    TArrayClearStatus(pFile->pUndoIndex);
    return;
  }
  ++pFile->nNumberOfRecords;
  return;
}

/* ************************************************************************
   Function: ConcatContinuous
   Description:
     Checks whether the	last two undo blocks represent a continuous
     operation Example: when typing the operation is continuous with
     the last typed character at the previous position.
*/
static void ConcatContinuous(TFile *pFile)
{
  TUndoRecord *pUndoLast;
  TUndoRecord *pUndoPrev;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->nNumberOfRecords < 2)
    return;  /* Only one block in the undo index list */

  pUndoLast = &pFile->pUndoIndex[pFile->nNumberOfRecords - 1];
  pUndoPrev = &pFile->pUndoIndex[pFile->nNumberOfRecords - 2];

  ASSERT(VALID_PUNDOREC(pUndoLast));
  ASSERT(VALID_PUNDOREC(pUndoPrev));
  ASSERT(!pUndoPrev->bUndone);  /* As all undone blocks are already removed */

  if (pUndoPrev->bRecoveryStore)
    return;  /* Last block is already in the recovery file */
  if (pUndoLast->nEnd - pUndoLast->nStart != 0)
    return;  /* Combine only single line actions */
  if (pUndoPrev->nEnd - pUndoPrev->nStart != 0)
    return;  /* Combine only single line actions */

  switch (pUndoLast->nOperation)
  {
    case acINSERT:  /* Block of text is inserted */
      if (pUndoPrev->nOperation != acINSERT)
        break;
      if (pUndoLast->nStart == pUndoPrev->nEnd &&
        pUndoLast->nStartPos == pUndoPrev->nEndPos + 1)
      {
        CombineUndoBlocks(pFile, TRUE);
      }
      break;
    case acDELETE:  /* Block of text is deleted */
      if (pUndoPrev->nOperation != acDELETE)
        break;
      if (pUndoLast->nStart == pUndoPrev->nEnd &&
        pUndoLast->nEndPos + 1 == pUndoPrev->nStartPos)
      {
	/* Backspace operation */
	CombineUndoBlocks(pFile, FALSE);
	break;
      }
      if (pUndoLast->nStart == pUndoPrev->nEnd &&
        pUndoLast->nStartPos == pUndoPrev->nStartPos)
      {
        /*
	Del operation.
	This is repetitive pressing of <Del> key at the same position.
	*/
	CombineUndoBlocks(pFile, TRUE);
	break;
      }
      break;
    case acREPLACE:  /* Text from the current line is replaced (OVR mode) */
      if (pUndoPrev->nOperation != acREPLACE)
        break;
      if (pUndoLast->nStart == pUndoPrev->nEnd &&
        pUndoLast->nStartPos == pUndoPrev->nEndPos + 1)
      {
        CombineUndoBlocks(pFile, TRUE);
      }
      break;
    case acREARRANGE:  /* Most likely as result from CmdEditSort() */
      /* no recombination for this operation is supposed */
      break;
    case acREARRANGEBACK:  /* Can appear only from a recovery file */
      /* no recombination for this operation is supposed */
      break;
    default:
      ASSERT(0);
  }
}

/* ************************************************************************
   Function: PostOperation
   Description:
     Stores the post operation file status.
     Checks whether the announced operation
     failed and if so removes the UndoBlock.
     Checks if the last undo blocks can be
     concatenated in one.
*/
void PostOperation(TFile *pFile)
{
  TUndoRecord *pUndoRec;

  ASSERT(pFile->nUndoLevel > 0);

  pUndoRec = GetLastRec(pFile);
  if (pFile->nUndoLevel == 1)
  {
    if (pUndoRec->pData == NULL)
    {
      /*
      Operation failed. Remove last undo record entry.
      */
      TArrayDeleteGroup(pFile->pUndoIndex, pFile->nNumberOfRecords - 1, 1);
      return;
    }
  }

  RecordFileStatus(pFile, &pUndoRec->after);
  pUndoRec->nUndoLevel = pFile->nUndoLevel;  /* Start/End of atom marker if = 1 */

  /*
  Concat the last two undo blocks if represent a continuous operation.
  */
  if (bCombineUndo)
    ConcatContinuous(pFile);
}

/* ************************************************************************
   Function: DisposeUndoIndexData
   Description:
     Disposes the pData heap allocations in the undo index.
     This function is to be called from into DisposeFile().
*/
void DisposeUndoIndexData(TFile *pFile)
{
  int i;
  TUndoRecord *pUndoRec;

  ASSERT(VALID_PFILE(pFile));

  if (pFile->pUndoIndex == NULL)
    return;  /* Undo record in empty! */

  for (i = 0; i < pFile->nNumberOfRecords;  ++i)
  {
    pUndoRec = &pFile->pUndoIndex[i];
    DisposeUndoRecord(pUndoRec);
  }
  ASSERT(pFile->nNumberOfRecords == _TArrayCount(pFile->pUndoIndex));

  TArrayDispose(pFile->pUndoIndex);
}

/* ************************************************************************
   Function: RemoveLastUndoRecord
   Description:
     Removes the last undo record as the
     operation failed. Removes the entry only if the level matches
     the current file undo level. If the record has level < of the
     current level, the AddUndoRecord() function has failed and there's nothing
     to be removed!
     This is to be called from into UNDO_BLOCK_END() macros.
*/
void RemoveLastUndoRecord(TFile *pFile)
{
  TUndoRecord *pUndoRec;

  ASSERT(VALID_PFILE(pFile));

  pUndoRec = &pFile->pUndoIndex[pFile->nNumberOfRecords];

  if (pUndoRec->nUndoLevel < pFile->nUndoLevel)
    return;  /* No extra record added that is to be removed */

  DisposeUndoRecord(pUndoRec);
  TArrayDeleteGroup(pFile->pUndoIndex, pFile->nNumberOfRecords - 1, 1);
  --pFile->nNumberOfRecords;

  ASSERT(pFile->nNumberOfRecords == _TArrayCount(pFile->pUndoIndex));
}

/* ************************************************************************
   Function: Undo
   Description:
     Performs a one step deep undo on a file.

   Q: WHAT IS TO SPLIT AN ATOM?
   A: Atom is compounded of 1 or more undo record blocks that were produced 
   by a single user action. If the atom is being split due to imposibility
   to be undone in his entity (caused by lack of memory), the user will
   expirience only a small inconveniance to undo one his DO action
   with more that one UNDO actions.
*/
void Undo(TFile *pFile)
{
  int i;
  char *p;
  TUndoRecord *pUndoRec;
  BOOLEAN bSplitAtom;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  /*
  Search the point of last undo in the undo records index.
  */
  for (i = pFile->nNumberOfRecords - 1; i >=0; --i)
  {
    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (!pUndoRec->bUndone)
      break;
  }
  if (i == -1)
    return;  /* All is undone! */

  /*
  Check whether we are at the exact position.
  */
  if (pUndoRec->after.nCol != pFile->nCol ||
    pUndoRec->after.nRow != pFile->nRow)
  {
    RestoreFileStatus(pFile, &pUndoRec->after);
    return;
    /* This was the first step. Now the user have to invoke UNDO again in order
    to actualy undo the operation. */
  }

  /*
  Undo the atom operation stored in undo record index
  starting from the item pointed by pUndoRec.
  */
  bSplitAtom = FALSE;
  do
  {
    ASSERT(pUndoRec->pData != NULL);

    switch (pUndoRec->nOperation)
    {
      case acINSERT:  /* Revert insert operation -> delete a block */
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
          bResult = DeleteColumnBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos, FALSE, (TBlock **)&pUndoRec->pData);
        else
          bResult = DeleteCharacterBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos);
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acDELETE:  /* Revert delete operation -> insert block */
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
        {
          GotoColRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertColumnBlockPrim(pFile, pUndoRec->pData);
        }
        else
        {
          if (pUndoRec->nStart == pFile->nNumberOfLines)
            GotoColRow(pFile, 0, pFile->nNumberOfLines);
          else
            GotoPosRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertCharacterBlockPrim(pFile, pUndoRec->pData);
        }
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acREPLACE:  /* Revert replace operation -> replace back */
        ASSERT(((TBlock *)(pUndoRec->pData))->nNumberOfLines = 1);
        /* Replace operates at the same position in do/undo/redo */
        RestoreFileStatus(pFile, &pUndoRec->before);
        p = GetBlockLineText((TBlock *)pUndoRec->pData, 0);
        ReplaceTextPrim(pFile, p, p);
        /* As a result of this operation in pUndoRec->pData
        will be stored what was replaced, at Redo
        same operation will result in what was initially typed
        by the user */
        break;

      case acREARRANGE:  /* Revert rearrange operation -> rearrange back */
        RevertRearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      case acREARRANGEBACK:  /* This is read from a recovery file */
        Rearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      default:
        ASSERT(0);
    }

    if (!bSplitAtom)  /* If Undo was successfull */
    {
      RestoreFileStatus(pFile, &pUndoRec->before);
      pUndoRec->bUndone = TRUE;
    }

    --i;
    if (i == -1)
      break;

    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    ASSERT(pUndoRec->pData != NULL);

    if (bSplitAtom)
      pUndoRec->nUndoLevel = 1;
  }
  while (pUndoRec->nUndoLevel > 1);  /* while end of atom */
  pFile->bPreserveSelection = TRUE;
}

/* ************************************************************************
   Function: Redo
   Description:
     Performs a one step deep redo on a file.
*/
void Redo(TFile *pFile)
{
  int i;
  char *p;
  TUndoRecord *pUndoRec;
  BOOLEAN bSplitAtom;
  BOOLEAN bResult;

  ASSERT(VALID_PFILE(pFile));

  i = pFile->nNumberOfRecords;
  if (i == 0)
    return;
  while (1)
  {
    --i;
    if (i < 0)
      break;
    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (!pUndoRec->bUndone)
      break;
  }
  ++i;  /* Go to the undone position */

  if (pFile->nNumberOfRecords == i)
    return;  /* Nothing to redo */

  /*
  Check whether we are at the exact position.
  */
  pUndoRec = &(pFile->pUndoIndex[i]);
  ASSERT(VALID_PUNDOREC(pUndoRec));
  if (pUndoRec->before.nCol != pFile->nCol ||
    pUndoRec->before.nRow != pFile->nRow)
  {
    RestoreFileStatus(pFile, &pUndoRec->before);
    return;
    /* This was the first step. Now the user have to invoke REDO again in order
    to actualy redo the operation. */
  }

  /*
  redo the atom operation stored in undo record index
  starting from the item pointed by index _i_;
  */
  bSplitAtom = FALSE;
  while (1)
  {
    ASSERT(pUndoRec->pData != NULL);

    switch (pUndoRec->nOperation)
    {
      case acINSERT:
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
        {
          GotoColRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertColumnBlockPrim(pFile, (TBlock *)pUndoRec->pData);
        }
        else
        {
          if (pUndoRec->nStart == pFile->nNumberOfLines)
            GotoColRow(pFile, 0, pFile->nNumberOfLines);
          else
            GotoPosRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertCharacterBlockPrim(pFile, (TBlock *)pUndoRec->pData);
        }
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acDELETE:
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
          bResult = DeleteColumnBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos, FALSE, (TBlock **)&pUndoRec->pData);
        else
          bResult = DeleteCharacterBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos);
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acREPLACE:  /* Revert replace operation -> replace back */
        ASSERT(((TBlock *)(pUndoRec->pData))->nNumberOfLines = 1);
        /* Replace operates at the same position in do/undo/redo */
        p = GetBlockLineText((TBlock *)pUndoRec->pData, 0);
        ReplaceTextPrim(pFile, p, p);
        /* As a result of this operation in pUndoRec->pData
        will be stored what was replaced, at Redo
        same operation will result in what was initially typed
        by the user */
        break;

      case acREARRANGE:  /* Rearrange operation -- DO rearrange */
        Rearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      case acREARRANGEBACK:  /* Loaded from a recovery file */
        RevertRearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      default:
        ASSERT(0);
    }

    if (!bSplitAtom)  /* If redo was successfull */
    {
      RestoreFileStatus(pFile, &pUndoRec->after);
      pUndoRec->bUndone = FALSE;
    }

    if (bSplitAtom)
    {
      pUndoRec->nUndoLevel = 1;
      break;
    }

    ++i;
    if (i == pFile->nNumberOfRecords)
      break;

    if (pUndoRec->nUndoLevel == 1)
      break;  /* End of atom */

    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    ASSERT(pUndoRec->pData != NULL);
  }
}

/* ************************************************************************
   Function: ParseStatLine
   Description:
     Parses a recovery file status line. Passed in sStat.
*/
static BOOLEAN ParseStatLine(char *sStat, char *_pFileName,
  int *nFileSize, int *nMonth, int *nDay, int *nYear,
  int *nHour, int *nMin, int *nSec)
{
  char *pFileName;
  char *pFileSize;
  char *pMonth;
  char *pDay;
  char *pYear;
  char *pHour;
  char *pMin;
  char *pSec;

  if (sStat[0] != '*')
    return FALSE;
  pFileName = strtok(sStat, "*,");
  if (pFileName == NULL)
    return FALSE;
  if (strlen(pFileName) >= _MAX_PATH)
    return FALSE;
  strcpy(_pFileName, pFileName);
  pFileSize = strtok(NULL, " ,");
  if (pFileSize == NULL)
    return FALSE;
  if (strcmp(pFileSize, sNewFileRec) == 0)
  {
    *nFileSize = -1;  /* Indicate new file */
    return TRUE;
  }
  pMonth = strtok(NULL, " ,-");
  pDay = strtok(NULL, "-");
  pYear = strtok(NULL, "-,");
  pHour = strtok(NULL, " :");
  pMin = strtok(NULL, ":");
  pSec = strtok(NULL, ":");

  if (!ValStr(pFileSize, nFileSize, 10) ||
    !ValStr(pMonth, nMonth, 10) ||
    !ValStr(pDay, nDay, 10) ||
    !ValStr(pYear, nYear, 10) ||
    !ValStr(pHour, nHour, 10) ||
    !ValStr(pMin, nMin, 10) ||
    !ValStr(pSec, nSec, 10))
  {
    /* File corrupted */
    return FALSE;
  }

  return TRUE;
}


/* ************************************************************************
   Function: GetStatLine
   Description:
     Gets the status line data from a rec file.
*/
BOOLEAN GetStatLine(TFile *pFile, char *_pFileName,
  int *nFileSize, int *nMonth, int *nDay, int *nYear,
  int *nHour, int *nMin, int *nSec)
{
  FILE *f;
#define MAX_LEN  _MAX_PATH + 100
  char lnbuf[MAX_LEN];  /* +100 for other stats beside name */
  BOOLEAN bResult;
  char *p;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->sRecoveryFileName[0] != '\0');

  f = NULL;
  if ((f = fopen(pFile->sRecoveryFileName, "rt")) == NULL)
    return FALSE;

  /*
  First line should be the file id line
  */
  if (fgets(lnbuf, MAX_LEN, f) == NULL)
  {
_exit3:
    fclose (f);
    return FALSE;
  }

  if ((p = strchr(lnbuf, '\n')) != NULL)
    *p = '\0';  /* Cut the line at \n character */

  if ((p = strchr(lnbuf, '\r')) != NULL)
    *p = '\0';  /* Cut the line at \r character */

  if (lnbuf[0] != '*')
    goto _exit3;

  bResult = ParseStatLine(lnbuf, _pFileName, nFileSize, nMonth,
    nDay, nYear, nHour, nMin, nSec);

  if (fclose(f) != 0)
    return FALSE;

  return bResult;
}

static const char *sInsert = "INSERT";
static const char *sDelete = "DELETE";
static const char *sReplace = "REPLACE";
static const char *sRearrange = "REARRANGE";
static const char *sRearrangeBack = "REARRANGEBACK";
static const char *sColumn = "col";
static const char *sChar = "char";

/* ************************************************************************
   Function: StoreUndoBlock
   Description:
     Stores an undo record as a recovery file record.
     Returns FALSE when disk operation fails.
*/
static BOOLEAN StoreUndoBlock(FILE *f, TFile *pFile, const TUndoRecord *pUndoRec, BOOLEAN bReversed)
{
  const char *pOperation;
  int i;
  int nNumberOfLines;
  char *pLine;
  char c;
  const TFileStatus *pFileStatus;

  ASSERT(VALID_PUNDOREC(pUndoRec));
  ASSERT(f != NULL);

  #ifdef _DEBUG
  if (bReversed)
    ASSERT(pUndoRec->bUndone);  /* Performed only for undone blocks! */
  #endif

  pFileStatus = &pUndoRec->before;  /* Default: if operation is not reversed */
  switch (pUndoRec->nOperation)
  {
    case acINSERT:
      if (bReversed)
      {
        pFileStatus = &pUndoRec->after;
        pOperation = sDelete;
      }
      else
        pOperation = sInsert;
      break;

    case acDELETE:
      if (bReversed)
      {
        pFileStatus = &pUndoRec->after;
        pOperation = sInsert;
      }
      else
        pOperation = sDelete;
      break;

    case acREPLACE:
        /* Replace is self-inversed against the text of the file
        if performed twice. So now store replace again. */
        pOperation = sReplace;
      break;

    case acREARRANGE:
      if (bReversed)
        pOperation = sRearrangeBack;
      else
        pOperation = sRearrange;
      break;

    default:
      ASSERT(0);
  }

  /*
  Not all of the paramaters from pUndo->before are necessary
  as those from	pUndo->after are reproducerable after doing the action
  */
  if (fprintf(f, "%s: (%d, %d, %d, %d): (%d, %d, %d, %d, %d): %d: %s\n",
    pOperation,
    pUndoRec->nStart, pUndoRec->nStartPos, pUndoRec->nEnd, pUndoRec->nEndPos,
    pFileStatus->nCol, pFileStatus->nRow, pFileStatus->nTopLine,
    pFileStatus->nWrtEdge, pFileStatus->nNumberOfLines,
    pUndoRec->nUndoLevel,
    ((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK ? sColumn : sChar) < 0)
    return FALSE;

  if (pOperation == sRearrange || pOperation == sRearrangeBack)
  {
    nNumberOfLines = pUndoRec->nEnd - pUndoRec->nStart + 1;
    for (i = 0; i < nNumberOfLines; ++i)
      if (fprintf(f, ">%d\n", ((int *)pUndoRec->pData)[i]) < 0)
        return FALSE;
    return TRUE;
  }

  if (pOperation == sReplace)
  {
    if (bReversed)
      goto _put_data;
    else
    {
      /*
      Get from the file what was overwritten
      */
      pLine = GetLineText(pFile, pUndoRec->nStart);
      ASSERT(GetLine(pFile, pUndoRec->nStart)->nLen > pUndoRec->nEndPos);
      c = pLine[pUndoRec->nEndPos + 1];  /* Remember what at the place of eol marker */
      pLine[pUndoRec->nEndPos + 1] = '\0';  /* Put an End-Of-Line marker at the end of region */
      if (fprintf(f, ">%s\n", &pLine[pUndoRec->nStartPos]) < 0)
      {
        pLine[pUndoRec->nEndPos + 1] = c;
        return FALSE;
      }
      pLine[pUndoRec->nEndPos + 1] = c;  /* Restore the text at the end of region */
      return TRUE;
    }
  }
  
  if (pOperation == sDelete)
    if ((((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK) == 0)
      return TRUE;

  /* We can skip storing what is deleted when deleting stream character blocks */
_put_data:
  for (i = 0; i < ((TBlock *)pUndoRec->pData)->nNumberOfLines; ++i)
    if (fprintf(f, ">%s\n", GetBlockLineText((TBlock *)pUndoRec->pData, i)) < 0)
      return FALSE;

  return TRUE;
}

#ifdef _DEBUG
/* ************************************************************************
   Function: CheckValidUndoList
   Description:
*/
void CheckValidUndoList(TFile *pFile)
{
  TUndoRecord *pUndoRec;
  int i;
  int nLines;
  int nChange;

  ASSERT(VALID_PFILE(pFile));

  i = pFile->nNumberOfRecords;

  if (i == 0)
    return;

  pUndoRec = &(pFile->pUndoIndex[0]);
  nLines = pUndoRec->before.nNumberOfLines;

  while (i > 0)
  {
    --i;
    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));

    switch (pUndoRec->nOperation)
    {
      case acINSERT:  /* Block of text is inserted */
        ASSERT(VALID_PBLOCK((TBlock *)pUndoRec->pData));
        break;
      case acDELETE:  /* Block of text is deleted */
        if (pUndoRec->pData != NULL)
          ASSERT(VALID_PBLOCK((TBlock *)pUndoRec->pData));
        break;
      case acREPLACE:  /* Text from the current line is replaced (OVR mode) */
        ASSERT(VALID_PBLOCK((TBlock *)pUndoRec->pData));
        break;
      case acREARRANGE:  /* Most likely as result from CmdEditSort() */
        ASSERT(pUndoRec->pData != NULL);
        break;
      case acREARRANGEBACK:  /* Can appear only from a recovery file */
        ASSERT(pUndoRec->pData != NULL);
        break;
      default:
        ASSERT(0);
    }
    nChange = pUndoRec->after.nNumberOfLines - pUndoRec->before.nNumberOfLines;
    if (!pUndoRec->bUndone)
      nLines += nChange;
  }

  ASSERT(nLines == pFile->nNumberOfLines);
}
#endif

/* ************************************************************************
   Function: StoreRecoveryRecord
   Description:
     This functions stores all user actions in the relevant recovery
     file (pFile->sRecFileName). The action are	stored from the bottom
     to the top of undo record list. The stored blocks were marked with
     bRecoveryStore flag set.

     Special considerations:
     1. bFileNameChanged - set by CmdFileSaveAs when a new name
     is assigned to a specific file or by CmdFileSave when
     the file is saved. When this happens all the
     blocks should be written in a newly created recovery file.
     2. bForceNewRecFile - set by CmdFileSave to indicate that
     a new rec file should be created.
     2. When blocks with bRecoveryStore were undone they
     should be stored with inversed action and backward order order.

     All blocks stored under point 2 considerations will
     have bUndone set and bRecoveryStore unset on EdStoreRecovery()
     exit.
   Returns:
     TRUE is there was a recovery store.
     FALSE nothing was stored, errno message should be displayed.
*/
BOOLEAN StoreRecoveryRecord(TFile *pFile)
{
  FILE *f;
  TUndoRecord *pUndoRec;
  const char *pOpenMode;  /* "at" or "wt" */
  BOOLEAN bStoreFileStat;
  BOOLEAN bUnlink;  /* If nothing stored */
  int i;

  ASSERT(VALID_PFILE(pFile));

  pFile->bRecoveryStored = FALSE;  /* Assume unsuccessfull recovery record */

  if (pFile->nNumberOfRecords == 0)
    return FALSE;  /* No action necessary */

  pOpenMode = "at";  /* Append the last actions to the recovery file */
  bStoreFileStat = FALSE;
  bUnlink = FALSE;
  if (pFile->bFileNameChanged || pFile->bForceNewRecoveryFile)
  {
    pOpenMode = "wt";  /* New file should be created */
    bStoreFileStat = TRUE;  /* Store filename, date, time and size */
    pFile->bFileNameChanged = FALSE;
    pFile->bForceNewRecoveryFile = FALSE;
    bUnlink = TRUE;
  }

  ASSERT(pFile->sRecoveryFileName[0] != '\0');

  if ((f = fopen(pFile->sRecoveryFileName, pOpenMode)) == NULL)
    return FALSE;  /* Examine errno */

  if (bStoreFileStat)
  {
    /*
    Store file recognition line. Would be used to be checked
    whether the file was changed from last editing.
    So if the file is changed with another editor the
    recovering will be disabled after the crash.
    */
    if (pFile->bNew)
    {
      if (fprintf(f, "*%s, %s\n", pFile->sFileName, sNewFileRec) < 0)
        return FALSE;
    }
    else
    {
      if (fprintf(f, "*%s, %u, %d-%d-%d, %d:%d:%d\n",
        pFile->sFileName,
        pFile->nFileSize,
        pFile->LastWriteTime.month,
        pFile->LastWriteTime.day,
        pFile->LastWriteTime.year,
        pFile->LastWriteTime.hour,
        pFile->LastWriteTime.min,
        pFile->LastWriteTime.sec) < 0)
        return FALSE;
    }
  }

  /*
  Walk the undo list. Direction is top->bottom.
  Walks all the undone section.
  Search for the case where stored blocks were
  undone.
  */
  i = pFile->nNumberOfRecords;
  while (i > 0)
  {
    --i;
    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (!pUndoRec->bUndone)
      break;  /* End of the undone actions */

    if (pUndoRec->bRecoveryStore)
    {
      pUndoRec->bRecoveryStore = FALSE;
      StoreUndoBlock(f, pFile, pUndoRec, TRUE);  /* Store the inversed action */
    }
  }

  /*
  Walk the undo list. Direction bottom->top.
  Walks all the _done_ section.
  Store all the blocks that have been not undone.
  */
  i = 0;
  while (i < pFile->nNumberOfRecords)
  {
    pUndoRec = &(pFile->pUndoIndex[i]);
    ASSERT(VALID_PUNDOREC(pUndoRec));
    if (pUndoRec->bUndone)
      break;  /* We reached the sector where actions are undone */

    if (!pUndoRec->bRecoveryStore)
    {
      pUndoRec->bRecoveryStore = TRUE;
      StoreUndoBlock(f, pFile, pUndoRec, FALSE);
      bUnlink = FALSE;
    }
    ++i;
  }
  if (fclose(f) != 0)
    return FALSE;  /* Examine errno */

  if (bUnlink)
  {
    /*
    bForeNewRecFile = TRUE: otherwise next time StoreRecovery() is called
    and there's still nothing new in the undo list, creating
    new file may result to a zero lenght size
    */
    pFile->bForceNewRecoveryFile = TRUE;
    if (unlink(pFile->sRecoveryFileName) != 0)
      return FALSE;
    return TRUE;
  }

  pFile->bRecoveryStored = TRUE;
  return TRUE;
}

/* ************************************************************************
   Function: LoadRecoveryFile
   Description:
     Loads the recovery file for a specific text file.
   Returns:
     0 - load OK.
     1 - errno error.
     2 - memory error. Check for partial data.
     3 - recovery file corrupted.
     4 - recovery file corrupted. Partial data available.
     5 - file in memory is not relevant to the recovery file.
*/
int LoadRecoveryFile(TFile *pFile)
{
  TFile RecoveryFile;
  char sFileName[_MAX_PATH];
  int nFileSize;
  int nMonth;
  int nDay;
  int nYear;
  int nHour;
  int nMin;
  int nSec;
  int nExitCode;
  int i;
  int j;
  char *pOperation;
  char *pBStart;
  char *pBStartPos;
  char *pBEnd;
  char *pBEndPos;
  char *pCol;
  char *pRow;
  char *pTopLine;
  char *pWrtEdge;
  char *pNumberOfLines;
  char *pUndoLevel;
  char *pBlockType;
  int nOperation;
  int nStart;
  int nStartPos;
  int nEnd;
  int nEndPos;
  int nCol;
  int nRow;
  int nTopLine;
  int nWrtEdge;
  int nNumberOfLines;
  int nUndoLevel;
  int nBlockType;
  TUndoRecord *pUndoRec;
  TLine *pLine;
  TBlock *pBlock;
  char *p;
  int nEOLSize;
  int nBlockSize;
  int *pLineArray;
  int nMax;
  int nItem;

  ASSERT(VALID_PFILE(pFile));
  ASSERT(pFile->sRecoveryFileName[0] != 0);

  InitEmptyFile(&RecoveryFile);
  strcpy(RecoveryFile.sFileName, pFile->sRecoveryFileName);

  switch (LoadFilePrim(&RecoveryFile))
  {
    case 0:
      /* Load OK */
      break;
    case 2:
      /* File doesn't exist */
      return 1;
    case 3:
      /* No memory for the file */
      return 2;
    case 4:
      return 1;
    case 5:
      return 1;
    default:
      ASSERT(0);
  }

  if (RecoveryFile.nNumberOfLines == 0)
  {
_file_corrupted:
    nExitCode = 3;  /* Recovery file corrupted */
_exit:
    DisposeFile(&RecoveryFile);
    pFile->nUndoLevel = 0;
    return nExitCode;
  }

  if (!ParseStatLine(GetLineText(&RecoveryFile, 0), sFileName, &nFileSize,
    &nMonth, &nDay, &nYear, &nHour, &nMin, &nSec))
    goto _file_corrupted;

  if (filestrcmp(pFile->sFileName, sFileName) != 0)
  {
    nExitCode = 5;
    goto _exit;
  }

  if (nFileSize == -1)
  {
    if (!pFile->bNew)
    {
      nExitCode = 5;
      goto _exit;
    }
  }
  else
    if (filestrcmp(pFile->sFileName, sFileName) != 0 ||
      nFileSize != pFile->nFileSize ||
      nMonth != pFile->LastWriteTime.month ||
      nDay != pFile->LastWriteTime.day ||
      nYear != pFile->LastWriteTime.year ||
      nHour != pFile->LastWriteTime.hour ||
      nMin != pFile->LastWriteTime.min ||
      nSec != pFile->LastWriteTime.sec)
    {
      nExitCode = 5;  /* File in memory is not relevant to the recovery file */
      goto _exit;
    }

  for (i = 1; i < RecoveryFile.nNumberOfLines; ++i)
  {
    /*
    This must be a description line
    Parse the description line
    */
    pOperation = strtok(GetLineText(&RecoveryFile, i), ":( ");
    pBStart = strtok(NULL, "(, ");
    pBStartPos = strtok(NULL, ", ");
    pBEnd = strtok(NULL, ", ");
    pBEndPos = strtok(NULL, ",): (");
    pCol = strtok(NULL,	":(, ");
    pRow = strtok(NULL, ", ");
    pTopLine = strtok(NULL, ", ");
    pWrtEdge = strtok(NULL, ", ");
    pNumberOfLines = strtok(NULL, ",) ");
    pUndoLevel = strtok(NULL, ") :");
    pBlockType = strtok(NULL, " :");

    if (pOperation == NULL ||
      pBStart == NULL ||
      pBStartPos == NULL ||
      pBEnd == NULL ||
      pBEndPos == NULL ||
      pCol == NULL ||
      pRow == NULL ||
      pTopLine == NULL ||
      pWrtEdge == NULL ||
      pNumberOfLines == NULL ||
      pUndoLevel == NULL ||
      pBlockType == NULL)
    {
_check_for_partial_data:
      if (pFile->nNumberOfRecords == 0)
        goto _file_corrupted;
      nExitCode = 4;  /* Partial data available */
_split_the_atom:
      /* Split the atom up to here */
      pUndoRec = GetLastRec(pFile);
      ASSERT(VALID_PUNDOREC(pUndoRec));
      pUndoRec->nUndoLevel = 1;
      goto _exit;
    }

    if (strcmp(pOperation, sInsert) == 0)
      nOperation = acINSERT;
    else
      if (strcmp(pOperation, sDelete) == 0)
        nOperation = acDELETE;
      else
        if (strcmp(pOperation, sReplace) == 0)
          nOperation = acREPLACE;
        else
          if (strcmp(pOperation, sRearrange) == 0)
            nOperation = acREARRANGE;
          else
            if (strcmp(pOperation, sRearrangeBack) == 0)
              nOperation = acREARRANGEBACK;
            else
              goto _check_for_partial_data;

    if (!ValStr(pBStart, &nStart, 10) ||
      !ValStr(pBStartPos, &nStartPos, 10) ||
      !ValStr(pBEnd, &nEnd, 10) ||
      !ValStr(pBEndPos, &nEndPos, 10) ||
      !ValStr(pCol, &nCol, 10) ||
      !ValStr(pRow, &nRow, 10) ||
      !ValStr(pTopLine, &nTopLine, 10) ||
      !ValStr(pWrtEdge, &nWrtEdge, 10) ||
      !ValStr(pNumberOfLines, &nNumberOfLines, 10) ||
      !ValStr(pUndoLevel, &nUndoLevel, 10))
      goto _check_for_partial_data;

    if (strcmp(pBlockType, sColumn) == 0)
      nBlockType = COLUMN_BLOCK;
    else
      if (strcmp(pBlockType, sChar) == 0)
        nBlockType = 0;
      else
        goto _check_for_partial_data;

    pFile->nUndoLevel = nUndoLevel;

    if (!AddUndoRecord(pFile, nOperation, TRUE))
    {
_fail_collecting:
      nExitCode = 2;
      if (pFile->nNumberOfRecords == 0)
        goto _exit;
      goto _split_the_atom;
    }

    /* Put the undo record information as read from the recovery file */
    pUndoRec = GetLastRec(pFile);
    pUndoRec->before.nRow = nRow;
    pUndoRec->before.nCol = nCol;
    pUndoRec->before.nTopLine = nTopLine;
    pUndoRec->before.nWrtEdge = nWrtEdge;
    pUndoRec->before.nNumberOfLines = nNumberOfLines;
    pUndoRec->nUndoLevel = nUndoLevel;

    pUndoRec->nStart = nStart;
    pUndoRec->nStartPos = nStartPos;
    pUndoRec->nEnd = nEnd;
    pUndoRec->nEndPos = nEndPos;

    if (nOperation == acDELETE)
    {
      pUndoRec->blockattr = nBlockType;  /* To be used by RecoverFile() as there's no pData */
      if ((nBlockType & COLUMN_BLOCK) == 0)
        continue;  /* No data storage for delete operations */
    }

    if (nOperation == acREARRANGE || nOperation == acREARRANGEBACK)
    {
      /* 
      Read the rearrange line numbers.
      First count the number of lines.
      */
      for (j = 1; j + i < RecoveryFile.nNumberOfLines; ++j)
      {
        pLine = GetLine(&RecoveryFile, i + j);
        if (pLine->pLine[0] != '>')
          break;
      }
      pLineArray = alloc(sizeof(int *) * (j - 1));
      if (pLineArray == NULL)
      {
        RemoveLastUndoRecord(pFile);
        goto _fail_collecting;
      }

      nMin = INT_MAX;
      nMax = 0;
      for (j = 1; j + i < RecoveryFile.nNumberOfLines; ++j)
      {
        pLine = GetLine(&RecoveryFile, i + j);
        if (pLine->pLine[0] != '>')
          break;
        if (!ValStr(pLine->pLine + 1, &nItem, 10))
        {
_dispose_pArray:
          RemoveLastUndoRecord(pFile);
          s_free(pLineArray);
          goto _check_for_partial_data;
        }
        if (nItem < nMin)
          nMin = nItem;
        if (nItem > nMax)
          nMax = nItem;
        pLineArray[j - 1] = nItem;
      }

      /*
      Check whether loaded array is valid
      */
      if (pUndoRec->nEnd - pUndoRec->nStart != nMax - nMin)
        goto _dispose_pArray;
      if (nMax - nMin + 1 != j - 1)
        goto _dispose_pArray;

      pUndoRec->pData = pLineArray;

      i += j - 1;
      continue;
    }

    /*
    Compose the data block
    */
    nBlockSize = 0;
    nEOLSize = pFile->nEOLType == CRLFtype ? 2 : 1;
    for (j = 1; j + i < RecoveryFile.nNumberOfLines; ++j)
    {
      pLine = GetLine(&RecoveryFile, i + j);
      if (pLine->pLine[0] == '>')
        nBlockSize += pLine->nLen - 1 + nEOLSize;
      else
        break;
    }

    pBlock = AllocTBlock();
    if (pBlock == NULL)
    {
      RemoveLastUndoRecord(pFile);
      goto _fail_collecting;
    }

    pBlock->pBlock = AllocateTBlock(nBlockSize);
    if (pBlock->pBlock == NULL)
    {
_dispose_pBlock:
      FreeTBlock(pBlock);
      RemoveLastUndoRecord(pFile);
      goto _fail_collecting;
    }

    pBlock->blockattr = nBlockType;
    pBlock->nNumberOfLines = j - 1;
    pBlock->nEOLType = pFile->nEOLType;

    TArrayInit(pBlock->pIndex, pBlock->nNumberOfLines, 1);
    if (pBlock->pIndex == NULL)
    {
      DisposeBlock(pBlock->pBlock);
      goto _dispose_pBlock;
    }
    pLine = pBlock->pIndex;
    p = pBlock->pBlock;

    for (j = 1; j <= pBlock->nNumberOfLines; ++j)
    {
      strcpy(p, &GetLineText(&RecoveryFile, i + j)[1]);  /* Skip the leading ">" */

      pLine->pLine = p;
      pLine->pFileBlock = pBlock->pBlock;
      pLine->attr = 0;

      p = strchr(p, '\0');  /* Search for end of line already marked with '\0' */
      pLine->nLen = p - pLine->pLine;  /* Calc the line length */
      p += nEOLSize;

      ++pLine;
      ASSERT(p != NULL);
    }
    ASSERT(p - pBlock->pBlock == nBlockSize);

    IncRef(pBlock->pBlock, pBlock->nNumberOfLines);  /* Update the file block reference counter */
    TArraySetCount(pBlock->pIndex, pBlock->nNumberOfLines);

    pUndoRec->pData = pBlock;

    i += j - 1;
  }

  DisposeFile(&RecoveryFile);
  pFile->nUndoLevel = 0;
  return 0;
}


/* ************************************************************************
   Function: RecoverFile
   Description:
     Recover a file based on what was load from the corespondent
     recovery file. This operation is simpler than Redo()
     Q: What is different to Redo()?
     A: The undo block in the undo chain has no pUndo->after
     information about the file status. This is ommited to be stored
     in the recovery file in order to reduce the file size. And now
     exactly after DO-ing one undo block is called RecordFileStatus()
     to take picture of file status after the operation. This makes a
     fully valid undo chain available for next user undo/redo
     operations.
   Returns:
     TRUE - recovered OK
     FALSE - not a complete recovery, (no enough memory)
   TODO:
     Before each operation check whether the operation
     has valid parameters in the current context of the file;
*/
BOOLEAN RecoverFile(TFile *pFile)
{
  TUndoRecord *pUndoRec;
  char *p;
  BOOLEAN bSplitAtom;
  BOOLEAN bResult;
  int i;
  int j;

  i = 0;
  bSplitAtom = FALSE;
  while (i < pFile->nNumberOfRecords)
  {
    pUndoRec = &pFile->pUndoIndex[i];
    ASSERT(VALID_PUNDOREC(pUndoRec));
    #ifdef _DEBUG
    if (pUndoRec->nOperation != acDELETE)
      ASSERT(pUndoRec->pData != NULL);
    #endif

    if (pUndoRec->before.nCol != pFile->nCol ||
      pUndoRec->before.nRow != pFile->nRow)
    {
      /* Restore the exact position of the operation */
      RestoreFileStatus(pFile, &pUndoRec->before);
    }

    switch (pUndoRec->nOperation)
    {
      case acINSERT:
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
        {
          GotoColRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertColumnBlockPrim(pFile, (TBlock *)pUndoRec->pData);
        }
        else
        {
          if (pUndoRec->nStart == pFile->nNumberOfLines)
            GotoColRow(pFile, 0, pFile->nNumberOfLines);
          else
            GotoPosRow(pFile, pUndoRec->nStartPos, pUndoRec->nStart);
          bResult = InsertCharacterBlockPrim(pFile, (TBlock *)pUndoRec->pData);
        }
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acDELETE:
        /*
        Record the data that is to be deleted. (only for non-column block)
        */
        if ((pUndoRec->blockattr & COLUMN_BLOCK) == 0)
          pUndoRec->pData = MakeACopyOfBlock(pFile,
            pUndoRec->nStart, pUndoRec->nEnd, pUndoRec->nStartPos, pUndoRec->nEndPos,
            pUndoRec->blockattr);
        if (pUndoRec->pData == NULL)
        {
          --i;  /* This block is invalid as no valid pData. So it will be removed */
          bSplitAtom = TRUE;
          break;
        }
        if (((TBlock *)pUndoRec->pData)->blockattr & COLUMN_BLOCK)
        {
          /*
          Use what is loaded from the recovery file as a pattern.
          */
          bResult = DeleteColumnBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos, FALSE, (TBlock **)&pUndoRec->pData);
        }
        else
          bResult = DeleteCharacterBlockPrim(pFile,
            pUndoRec->nStart, pUndoRec->nStartPos,
            pUndoRec->nEnd, pUndoRec->nEndPos);
        if (!bResult)
          bSplitAtom = TRUE;  /* Failed? -- split the atom operaition sequence */
        break;

      case acREPLACE:  /* Revert replace operation -> replace back */
        ASSERT(((TBlock *)(pUndoRec->pData))->nNumberOfLines = 1);
        /* Replace operates at the same position in do/undo/redo */
        p = GetBlockLineText((TBlock *)pUndoRec->pData, 0);
        ReplaceTextPrim(pFile, p, p);
        /* As a result of this operation in pUndoRec->pData
        will be stored what was replaced, at Redo
        same operation will result in what was initially typed
        by the user */
        break;

      case acREARRANGE:  /* Rearrange operation -- DO rearrange */
        Rearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      case acREARRANGEBACK:  /* Loaded from a recovery file */
        RevertRearrange(pFile, pUndoRec->nEnd - pUndoRec->nStart + 1, (int *)pUndoRec->pData);
        break;

      default:
        ASSERT(0);
    }

    if (bSplitAtom)
    {
      /*
      Unsuccessfull step in the recovery operation.
      */
      if (i > 0)
      {
        pUndoRec = &pFile->pUndoIndex[i - 1];
        /* Split the atom up to the previous operation */
        pUndoRec->nUndoLevel = 1;
      }

      /*
      Remove the rest of the chain as x->after for
      the rest of the blocks could not be properly filled.
      */
      for (j = i; j < pFile->nNumberOfRecords; ++j)
      {
        pUndoRec = &pFile->pUndoIndex[j];
        DisposeUndoRecord(pUndoRec);
      }

      /*
      Remove the entries from the undo records index.
      */
      TArrayDeleteGroup(pFile->pUndoIndex, i, pFile->nNumberOfRecords - i);
      pFile->nNumberOfRecords -= pFile->nNumberOfRecords - i;
      return FALSE;  /* Recovery has been terminated prematurely */
    }

    pFile->bChanged = TRUE;  /* We have at least one change */
    /* To have a complete undo block we need pUndoRec->after to be filled */
    RecordFileStatus(pFile, &pUndoRec->after);
    pUndoRec->bRecoveryStore = TRUE;  /* This block exists on the disk */
    if (i > 0)
      pUndoRec->before.bChanged = TRUE;

    ++i;
  }

  return TRUE;
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


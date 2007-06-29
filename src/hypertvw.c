/*

File: hypertvw.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 4th March, 2001
Descrition:
  Hypertext viewer.

*/

#include "global.h"
#include "kbd.h"
#include "scr.h"
#include "keydefs.h"
#include "keyset.h"
#include "ksetcmd.h"
#include "palette.h"
#include "memory.h"
#include "hypertvw.h"

static const char sNext[] = "Next:";
static const char sPrev[] = "Prev:";
static const char sUp[] = "Up:";
static const char sNote1[] = "*Note";
static const char sNote2[] = "*note";
static const char sMenu[] = "* Menu:";
static char sLinkBuf[MAX_NODE_LEN];

typedef struct LineOutput
{
  char c;
  char t;  /* Index in the color palette */
} TLineOutput;

/* ************************************************************************
   Function: NavigatePage
   Description:
     Gets next character from an info page text. Increments nPos in the
     context.
     Returns FALSE when there are no more characters in the page.
     nDir is the advancment direction. Reset to 1 after every call.
*/
static BOOLEAN GetNextChar(TInfoPageContext *pstCtx, char *c)
{
  int nRangeLo;
  int nRangeHi;

  ASSERT(pstCtx->nPos >= 0);

_getnext:
  if (pstCtx->pstBlock == NULL)
    pstCtx->pstBlock = (TInfoPageText *)pstCtx->blist->Flink;
_recalc_range:
  ASSERT(VALID_INFOTEXTBLOCK(pstCtx->pstBlock));
  nRangeLo = pstCtx->pstBlock->nBlockPos;
  nRangeHi = nRangeLo + pstCtx->pstBlock->nBytesRead;
  if (pstCtx->nPos >= nRangeHi)
  {
    if (END_OF_LIST(pstCtx->blist, pstCtx->pstBlock->link.Flink))
    {
      pstCtx->nDir = 1;
      return FALSE;
    }
    pstCtx->pstBlock = (TInfoPageText *)pstCtx->pstBlock->link.Flink;
    goto _recalc_range;
  }
  if (pstCtx->nPos < nRangeLo)
  {
    pstCtx->pstBlock = (TInfoPageText *)pstCtx->pstBlock->link.Blink;
    goto _recalc_range;
  }

  pstCtx->cPrev = *c;
  *c = pstCtx->pstBlock->Text[pstCtx->nPos - nRangeLo];
  pstCtx->nPos += pstCtx->nDir;
  if (*c == '\r')
    goto _getnext;
  pstCtx->nDir = 1;
  return TRUE;
}

/* ************************************************************************
   Function: LinkStartsAtPrevLine
   Description:
     Checks whether there is a link that starts at the end
     of the previous line.
     If there is a link extracts the words of the display part that are at
     the end of the line (this can be used to compose the whole link if
     the display representation is one and the same with the link) at
     the global variable sLink[].
     pnLinkPos receives the position of the link
*/
static BOOLEAN LinkStartsAtPrevLine(TInfoPageContext *pCtx, int *pnLinkPos)
{
  char c;
  int nSavePos;
  int nSavePos2;
  char sToken[9];
  char *d;
  char *p;

  nSavePos = pCtx->nPos;
  /* First go to the start of the previous line */
  if (pCtx->nPos > 0)
  {
    pCtx->nPos -= 2;
    do
    {
      pCtx->nDir = -1;
      if (!GetNextChar(pCtx, &c))
        goto _invalid_char;
    }
    while (c != '\n' && pCtx->nPos > 0);
    pCtx->nPos += 2;  /* jump over the end of a prev prev line */
  }
  do
  {
    if (!GetNextChar(pCtx, &c))
      break;  /* end of data */
    *pnLinkPos = pCtx->nPos;
    if (c == sNote1[0])
    {
      /* Check whether this is a *note or *Note */
      nSavePos2 = pCtx->nPos;
      p = sToken;
      *p++ = c;
      /* gather a token */
      while (pCtx->nPos - nSavePos2 < sizeof(sNote1))
      {
        if (!GetNextChar(pCtx, &c))
        {
_invalid_char:
          pCtx->nPos = nSavePos;
          return FALSE;
        }
        if (c == '\n')
          break;
        if (c == ' ')
          break;
        *p++ = c;
      }
      *p = '\0';  /* make ASCIIZ */
      /* check for sNote1 or sNote2 token */
      d = sLinkBuf;
      if (strcmp(sToken, sNote1) == 0 ||
        strcmp(sToken, sNote2) == 0)
      {
        if (c == '\n')  /* only *note at the end of the line */
        {
_link:
          *d = '\0';
          pCtx->nPos = nSavePos;
          return TRUE;
        }
        /* skip the space */
        if (!GetNextChar(pCtx, &c))
          goto _invalid_char;
        while (1)
        {
          if (c == '\n')  /* end of line? we have partial link */
            goto _link;
          if (d - sLinkBuf < sizeof(sLinkBuf))
            *d++ = c;
          if (!GetNextChar(pCtx, &c))
            goto _invalid_char;
          if (c == ':')
            if (pCtx->cPrev == ':')
            {
              --d;  /* remove the last ':' from the link */
              break;
            }
          if (c == '.')  /* spreading the news: ',' is e-o-link marker */
            break;
        }
        *d = '\0';  /* make the link ASCIIZ string */
        if (!GetNextChar(pCtx, &c))
          goto _invalid_char;
      }
      else  /* This is not a link */
      {
        pCtx->nPos = nSavePos - 1;
        GetNextChar(pCtx, &c);
      };
    }  /* process a link */
  }
  while (c != '\n');
  pCtx->nPos = nSavePos;
  return FALSE;
}

/* ************************************************************************
   Function: FindMenuLine
   Description:
     Checks whether there is a menu contained in the current page.
     Returns the position where the menu starts.
*/
static int FindMenuLine(TInfoPageContext *pCtx)
{
  int nSavePos;
  int nResult;
  BOOLEAN bResult;
  char sToken[sizeof(sMenu) + 1];
  char *p;
  char c;

  bResult = FALSE;
  nSavePos = pCtx->nPos;

  pCtx->nPos = 2;
  p = sToken;
  /* Extract the first token of each line,
  use as an end marker the ':' character
  and check the token against sMenu */
  while (GetNextChar(pCtx, &c))
  {
    if (c == '\n')
      p = sToken;  /* recent token collection at new line */
    if (c == '\x1f')
      goto _exit;
    if (c >= ' ')
    {
      if (p - sToken < sizeof(sToken) - 1)
        *p++ = c;
      else
        goto _nextline;
    }
    if (c == ':')
    {
      /* we get to end of a token */
      *p = '\0';
      if (strcmp(sToken, sMenu) == 0)
      {
        bResult = TRUE;
        goto _exit;
      }
      else
      {
        /* sMenu can be only as the first token on the line
        walk to the end of the line */
_nextline:
        while (c != '\n')
        {
          if (!GetNextChar(pCtx, &c))
            goto _exit;
          if (c == '\x1f')
            goto _exit;
        }
      }
      p = sToken;
    }
  }

_exit:
  nResult = pCtx->nPos;
  pCtx->nPos = nSavePos;
  if (bResult)
    return nResult;
  else
    return -1;
}

/* ************************************************************************
   Function: PartOfMenu
   Description:
     Checks whether the current line is part of a menu.
*/
static BOOLEAN PartOfMenu(TInfoPageContext *pCtx)
{
  if (pCtx->nMenuLinePos < 0)
    return FALSE;
  if (pCtx->nPos > pCtx->nMenuLinePos)
    return TRUE;
  return FALSE;
}

/* ************************************************************************
   Function: TopLine
   Description:
     The current line is the top line of the page, expect links
     of type Node-Next-Prev-Up.
     If this the page "Top", then add the pseudo link "About".
*/
static BOOLEAN TopLine(TInfoPageContext *pCtx)
{
  if (pCtx->nPos == 0)
    return TRUE;
  else
    return FALSE;
}

static int nOutputIndex;

/* ************************************************************************
   Function: PutCharAttr
   Description:
     Stores character/attributes pair if in visible region of a line.
     Visible region of a line this is left from nWrtEdge and
     up to nWinWidth of characters.
*/
static void _PutCharAttr(char c, int attr, int nWrtEdge, int nWinWidth,
  TLineOutput *pOutputBuf)
{
  /*
  Check whether nOutputIndex falls in the visible section of the line
  */
  if (nOutputIndex < nWrtEdge)
    goto _exit;
  if (nOutputIndex - nWrtEdge > nWinWidth)
    goto _exit;

  pOutputBuf[nOutputIndex - nWrtEdge].c = c;
  pOutputBuf[nOutputIndex - nWrtEdge].t = attr;

_exit:
  ++nOutputIndex;
}

#if 0
/* ************************************************************************
   Function: PutAttr
   Description:
     Fills a region of output buffer with specified attributes.
*/
static void _PutAttr(int attr, int nWrtEdge, int nWinWidth,
  int nRegionStart, int nRegionEnd, TLineOutput *pOutputBuf)
{
  int nStartPos;
  int nEndPos;
  int i;

  ASSERT(nRegionStart <= nRegionEnd);

  nStartPos = nRegionStart  - nWrtEdge;
  nEndPos = nRegionEnd - nWrtEdge;

  if (nStartPos < 0 && nEndPos < 0)
    return;

  if (nStartPos < 0)
  {
    nStartPos = 0;
    ASSERT(nEndPos >= 0);
  }

  for (i = nStartPos; i <= nEndPos && i < nWinWidth; ++i)
    pOutputBuf[i].t = attr;
}
#endif

/* ************************************************************************
   Function: DisplayLine
   Description:
     Displays the line that starts at position pCtx->nPos.
     Advances pCtx->nPos to the next line.
   Note:
     To trace all the positions of DisplayLine open:
     -- DIR page;
     -- page with *Menu;
     -- page with Index menu;
     -- page with a link that is split at two lines;
*/
static void DisplayLine(TInfoPageContext *pCtx, TLineOutput *pOutputBuf)
{
  char c;
  char sToken[9];
  char *p;
  BOOLEAN bLink;
  BOOLEAN bIndexType;
  int nAttr;
  int nFillUp;
  int nSavePos;
  int nLinkPos;
  int nDestPos;

  ASSERT(sizeof(sToken) >= sizeof(sNext));
  ASSERT(sizeof(sToken) >= sizeof(sPrev));
  ASSERT(sizeof(sToken) >= sizeof(sUp));

  nOutputIndex = 0;

  if (pCtx->bEOP)
    goto _fillup;

  if (TopLine(pCtx))
  {
    /*
    We are at the top line of a page
    We should present the node names after "Next:", "Prev:" and "Up:" as links
    */
    if (!GetNextChar(pCtx, &c))
      goto _fillup;  /* this is abnormal page, fill up with spaces */
    if (c != '\x1f')
      goto _fillup;  /* we expect a marker that is not present -> cancel */
    do
    {
      if (!GetNextChar(pCtx, &c))
        goto _fillup;
    } while (!isalpha(c));
    /* Skip the "File:" and the file itself */
    while (GetNextChar(pCtx, &c))
    {
      if (c == ':')  /* this is the separator we need */
      {
        ++pCtx->nPos;  /* skip the space as well */
        break;
      }
    }
    /* Skip the filename */
    while (GetNextChar(pCtx, &c))
    {
      if (isspace(c))
        break;
    }
    /* Skip to first no-blank */
    while (GetNextChar(pCtx, &c))
    {
      if (!isspace(c))
        break;
    }
    /* Now put characters and look for "Next:", "Prev:" or "Up:" */
    p = sToken;
    nAttr = coHelpText;
    while (c != '\n')
    {
      _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
        pOutputBuf);
      if (c > ' ')
        if (p - sToken < sizeof(sToken) - 1)
          *p++ = c;
      if (c == ' ')
      {
        *p = '\0';
        if (strcmp(sToken, sNext) == 0 ||
          strcmp(sToken, sPrev) == 0 ||
          strcmp(sToken, sUp) == 0)
        {
          if (pCtx->nPos == pCtx->nCurPos)
            nAttr = coHelpHighlightLink;
          else
            nAttr = coHelpLink;
          do
          {
            if (!GetNextChar(pCtx, &c))
              break;
            if (c == '\n')
              break;
            if (c != ',')
              _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
                pOutputBuf);
          }
          while (c != ',');
          nAttr = coHelpText;
          if (c == '\n')
            goto _fillup;  /* end of line */
          /* put the ',' */
          _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
            pOutputBuf);
        }
        p = sToken;
      }
      if (!GetNextChar(pCtx, &c))
        goto _fillup;  /* unexpected end of data */
    }
    goto _fillup;
  }

  if (PartOfMenu(pCtx))
  {
    if (!GetNextChar(pCtx, &c))
      goto _fillup;
    if (c == '\x1f')  /* this is end-of-page marker */
    {
      /* nothing more to display, walk the current position
      at the end of the page */
      pCtx->nLastPos = pCtx->nPos;
      pCtx->bEOP = TRUE;
      goto _fillup;
    }
    --pCtx->nPos;  /* ungetch() */
    /* This line is a part of a menu
    Check whether the line begins with a token marked by '::' to display it
    as a link */
    bLink = FALSE;
    nAttr = coHelpText;
    if (pCtx->bAtALink && pCtx->nLinePos == pCtx->nPos)
      nAttr = coHelpHighlightMenu;
    nSavePos = pCtx->nPos;
    bIndexType = FALSE;
    if (!GetNextChar(pCtx, &c))
      goto _fillup;  /* invalid line */
    if (c == '*')
    {
      /*
      We have valid start of a link from a menu,
      now we need to determine whether this is a regular
      *Menu or this is the generated index at the end of the file
      */
      do
      {
        /* Go to the end and check for a dot */
        if (!GetNextChar(pCtx, &c))
          goto _fillup;  /* invalid line */
        if (c == ':' && pCtx->cPrev == ':')
        {
          /* :: is an indicator of a normal *Menu */
          bIndexType = FALSE;  /* it is not a generated index */
          break;
        }
      }
      while (c != '.');
      if (!GetNextChar(pCtx, &c))
        goto _fillup;  /* invalid line */
      if (c == '\n')
      {
        /* we have ".\n" */
        bIndexType = TRUE;
        bLink = TRUE;
        goto _process_menuline;
      }
      pCtx->nPos = nSavePos + 1;  /* restore position */
      /*
      ---Heuristic check follows---
      Check for double column first '::'.
      Check for '(' after the first column.
      */
      while (1)
      {
        if (!GetNextChar(pCtx, &c))
          goto _fillup;  /* invalid line */
        if (c == '\n')
          break;
        if (c != ':')
          continue;
        /* We get to the first ':' */
        if (!GetNextChar(pCtx, &c))
           goto _fillup;  /* invalid line */
        bLink = TRUE;  /* assume generated index */
        bIndexType = TRUE;
        /* Check for 2 possible exceptions */
        if (c == ':')
        {
          bIndexType = FALSE;  /* it is not a generated index */
          break;
        }
        else
        {
          /* check for ': (' */
          if (!GetNextChar(pCtx, &c))
            break;
          if (c != '(')
            break;
          bIndexType = FALSE;  /* it is not a generated index */
          break;
        }
      }
    }
    else
    {
      /* not a link line ! */
      pCtx->nPos = nSavePos;
      goto _display_text;
    }
_process_menuline:
    pCtx->nPos = nSavePos;
    if (bLink)
      pCtx->nPos += 2;  /* Skip leading '* ' pair */
    if (bIndexType)
    {
      /* This is the generated Index at
      the end of the info file */
_display_item:
      do
      {
        /* display everything until ':' */
        if (!GetNextChar(pCtx, &c))
          goto _fillup;  /* invalid line */
        _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
          pOutputBuf);
      }
      while (c != ':');
      /*
      Walk to end of the line to check whether we have another ':'
      */
      nSavePos = pCtx->nPos;
      do
      {
        if (!GetNextChar(pCtx, &c))
          goto _fillup;
        if (c == ':')
        {
          pCtx->nPos = nSavePos;
          goto _display_item;
        }
      }
      while (c != '\n');
      pCtx->nPos = nSavePos;
      /*
      Display everything until the first non-blank
      */
      do
      {
        if (!GetNextChar(pCtx, &c))
          goto _fillup;  /* invalid line */
        if (c == '\n')
          goto _fillup;
        if (c == ' ')
          _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
            pOutputBuf);
      }
      while (c == ' ');
      /*
      Now display the link
      */
      if (pCtx->nPos - 1 == pCtx->nCurPos)
        nAttr = coHelpHighlightLink;
      else
        nAttr = coHelpLink;
      while (c != '\n')
      {
        _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
          pOutputBuf);
        if (!GetNextChar(pCtx, &c))
          goto _fillup;  /* invalid line */
      }
      goto _fillup;  /* the line is displayed */
    }
    /*
    Normal menu
    */
    if (pCtx->nPos == pCtx->nCurPos)
      nAttr = coHelpHighlightLink;
    else
      nAttr = coHelpLink;
    while (1)
    {
      if (!GetNextChar(pCtx, &c))
        goto _fillup;
      if (bLink && c == ':')
      {
        /*
        We have reached the end of the link.
        */
        if (!GetNextChar(pCtx, &c))
          goto _fillup;
        if (c == ':')  /* double ':' */
        {
          nDestPos = 0;
_end_of_link:
          bLink = FALSE;
          if (nAttr == coHelpHighlightLink)
            nAttr = coHelpHighlightMenu;
          else
            nAttr = coHelpText;
          /* At the end of the menu item (link):
          make up for the removed * and :: characters
          to fully mimic the original Info display behaviour
          this is done in regard to the fact that some of the lines
          may contain tab (\x9) characters and we need the same as the
          original (read Info) column disposition on the screen */
          nDestPos = nDestPos + nOutputIndex + 4;
          while (nDestPos != nOutputIndex)
            _PutCharAttr(' ', nAttr, pCtx->nWrtEdge, pCtx->nWidth,
              pOutputBuf);
          continue;
        }
        else
        {
          /*
          This is a link from the (dir) file
          */
          nDestPos = 0;
          nSavePos = pCtx->nPos;
          /* check for '(' */
          if (!GetNextChar(pCtx, &c))
            goto _end_of_link;
          --pCtx->nPos;
          if (c != '(')
            goto _end_of_link;
          /* We need to skip to first dot */
          nDestPos = pCtx->nPos - 1;
          while (1)
          {
            if (!GetNextChar(pCtx, &c))
              goto _end_of_link;
            if (c == '.')
            {
              GetNextChar(pCtx, &c);
              nDestPos = pCtx->nPos - nDestPos;
              goto _end_of_link;
            }
          }
        }
      }  /* Monitor for the end of the link */
      if (c == '\n')
        goto _fillup;
      if (c == '\t')
      {
        nDestPos = nOutputIndex + 8 - nOutputIndex % 8;
        while (nDestPos != nOutputIndex)
          _PutCharAttr(' ', nAttr, pCtx->nWrtEdge, pCtx->nWidth,
            pOutputBuf);
        continue;
      }
      _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
        pOutputBuf);
    }
    goto _fillup;
  }

  /*
  Not a top line and not part of a menu.
  Display this line.
  */
_display_text:
  if (LinkStartsAtPrevLine(pCtx, &nLinkPos))
  {
    /* check whether the cursor is at the start of the line,
    or if falls at the link that starts at the end of the prev line */
    if (pCtx->nCurPos == nLinkPos)
      nAttr = coHelpHighlightLink;
    else
      nAttr = coHelpLink;
    if (strchr(sLinkBuf, ':') != NULL)
      goto _proceed;  /* the whole symbol was displayed on the prev ln */
    goto _display_link;
  }
  nAttr = coHelpText;
  do
  {
    if (!GetNextChar(pCtx, &c))
      goto _fillup;  /* end of data */
    if (c == sNote1[0])
    {
      /* Check whether this is a *note or *Note */
      nSavePos = pCtx->nPos;
      p = sToken;
      *p++ = c;
      while (pCtx->nPos - nSavePos < sizeof(sNote1))
      {
        if (!GetNextChar(pCtx, &c))
        {
          /* we are unable to deliver a complete token */
          goto _restore_pos;
        }
        if (c == '\n')
          break;
        if (c == ' ')
          break;
        *p++ = c;
      }
      *p = '\0';
      if (strcmp(sToken, sNote1) == 0 ||
        strcmp(sToken, sNote2) == 0)
      {
        if (pCtx->nCurPos == nSavePos)
          nAttr = coHelpHighlightLink;
        else
          nAttr = coHelpLink;
        /* skip the space */
_display_link:
        if (c == '\n')  /* *note at the end of the line */
          goto _fillup;
        if (!GetNextChar(pCtx, &c))
          goto _restore_pos;
        if (c == '\n')  /* *note at the end of the line */
          goto _fillup;
        while (1)
        {
          _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
            pOutputBuf);
          if (!GetNextChar(pCtx, &c))
            goto _restore_pos;
          if (c == ':')
            break;
          if (c == '\n')
            goto _fillup;
        }
        nAttr = coHelpText;
        /* skip to '.' (end of link marker) */
        while (c != '.')
        {
_proceed:
          if (!GetNextChar(pCtx, &c))
            goto _restore_pos;
          if (c == '\n')
            goto _fillup;
          if (c == ':')
            if (pCtx->cPrev == ':')
              break;
        }
        if (!GetNextChar(pCtx, &c))
          goto _restore_pos;
      }
      else  /* This is not a link */
      {
_restore_pos:
        pCtx->nPos = nSavePos - 1;
        GetNextChar(pCtx, &c);  /* should always succeed */
      }
    }  /* process a link */

    if (c == '\x1f')  /* this is end-of-page marker */
    {
      /* nothing more to display, walk the current position
      at the end of the page */
      pCtx->nLastPos = pCtx->nPos;
      pCtx->bEOP = TRUE;
      goto _fillup;
    }
    if (c == '\t')
    {
      nDestPos = nOutputIndex + 8 - nOutputIndex % 8;
      while (nDestPos != nOutputIndex)
        _PutCharAttr(' ', nAttr, pCtx->nWrtEdge, pCtx->nWidth,
          pOutputBuf);
      continue;
    }
    if (c != '\n')
      _PutCharAttr(c, nAttr, pCtx->nWrtEdge, pCtx->nWidth,
        pOutputBuf);
  }
  while (c != '\n');

_fillup:
  nFillUp = nOutputIndex;
  nAttr = coHelpText;
  while (nFillUp++ <= pCtx->nWidth + pCtx->nWrtEdge)
    _PutCharAttr(' ', nAttr, pCtx->nWrtEdge, pCtx->nWidth, pOutputBuf);
}

/* ************************************************************************
   Function: FindLine
   Description:
     Searches for a start of a line position in the hypertext buffer.
   Returns:
     FALSE -- the requested line is outside the page
*/
static BOOLEAN FindLine(TInfoPageContext *pCtx, int nNewRow,
  int *pnRow, int *pnLinePos)
{
  int nBaseLine;
  char c;
  BOOLEAN bResult;

  if (nNewRow == *pnRow)
    return TRUE;

  if (nNewRow == 0)
  {
    *pnLinePos = 0;
    *pnRow = 0;
    return TRUE;
  }

  /*
  Establish new nLinePos
  */
  bResult = TRUE;
  nBaseLine = 0;
  pCtx->nPos = 0;
  if (nNewRow > *pnRow)
  {
    nBaseLine = *pnRow;
    pCtx->nPos = *pnLinePos;
  }
  else
    if (nNewRow > pCtx->nTopLine)
    {
      nBaseLine = pCtx->nTopLine;
      pCtx->nPos = pCtx->nTopLinePos;
    }

  /*
  Walk from nBaseLine:nPos to establish the new position
  */
  *pnRow = nNewRow;  /* assume there are enough rows to walk down */
  if (nBaseLine == 0)
  {
    do
    {
      /* skip the the control character line */
      if (!GetNextChar(pCtx, &c))
      {
        *pnRow = 0;
        break;
      }
    }
    while (c != '\n');
  }

  while (nBaseLine < nNewRow)
  {
    if (!GetNextChar(pCtx, &c))
    {
      /* nRow is going below the last line */
_no_morelines:
      *pnRow = nBaseLine;
      bResult = FALSE;
      break;
    }
    if (c == '\n')
      ++nBaseLine;
    if (c == '\x1f')
    {
      --pCtx->nPos;
      goto _no_morelines;
    }
  }
  *pnLinePos = pCtx->nPos;
  return bResult;
}

/* ************************************************************************
   Function: LineIsInBlock
   Description:
     Checks whether particular line falls in block.
*/
static BOOLEAN LineIsInBlock(const TInfoPageContext *pCtx, int nLine)
{
  if (!pCtx->bBlock)
    return (FALSE);

  if (nLine >= pCtx->nStartLine && nLine <= pCtx->nEndLine)
    return (TRUE);  /* This line is in the block */

  return (FALSE);
}

/* ************************************************************************
   Function: InsideBlock
   Description:
     Checks whether specific position falls inside a block.
*/
static BOOLEAN SetBlockPos(const TInfoPageContext *pCtx, int nWrtLine,
  int *nBlockStart, int *nBlockEnd, int nWidth)
{
  if (!LineIsInBlock(pCtx, nWrtLine))
    return FALSE;

  *nBlockStart = *nBlockEnd = -1;  /* Invalid by default */

  if (pCtx->bColumn)
  {
    ASSERT(pCtx->nStartPos >= 0);
    ASSERT(pCtx->nEndPos >= 0);
    ASSERT(pCtx->nEndPos >= pCtx->nStartPos);

    *nBlockStart = pCtx->nStartPos;
    *nBlockEnd = pCtx->nEndPos;
  }
  else
  {
    ASSERT(pCtx->nStartPos >= 0);
    ASSERT(pCtx->nEndPos >= -1);

    /*
    Check the cases when nWrtLine is start block line
    and nWrtLine is end block line
    */
    if (nWrtLine == pCtx->nStartLine)
      *nBlockStart = pCtx->nStartPos;
    if (nWrtLine == pCtx->nEndLine)
    {
      if (pCtx->nEndPos == -1)
        return FALSE;  /* Block ends inclusively at the end of the prev line */
      *nBlockEnd = pCtx->nEndPos;
    }

    if (*nBlockStart == -1)
      *nBlockStart = 0;  /* Line starts in a some previous line */
    if (*nBlockEnd == -1)
      *nBlockEnd = nOutputIndex;  /* Line ends in a some next line */
  }
  return TRUE;
}

/* ************************************************************************
   Function: PutAttr
   Description:
     Fills a region of output buffer with specified attributes.
*/
static void PutAttrBlock(int attr, int nWrtEdge, int nWinWidth,
  int nRegionStart, int nRegionEnd, CharInfo *pOutputBuf)
{
  int nStartPos;
  int nEndPos;
  int i;
  CharInfo *p;

  ASSERT(nRegionStart <= nRegionEnd);

  nStartPos = nRegionStart  - nWrtEdge;
  nEndPos = nRegionEnd - nWrtEdge;

  if (nStartPos < 0 && nEndPos < 0)
    return;

  if (nStartPos < 0)
  {
    nStartPos = 0;
    ASSERT(nEndPos >= 0);
  }

  p = pOutputBuf + nStartPos;
  for (i = nStartPos; i <= nEndPos && i < nWinWidth; ++i)
  {
    PutAttr(p, attr);
    ++p;
  }
}

/* ************************************************************************
   Function: DisplayPage
   Description:
     Displays a page.
     The whole page update is subject to bUpdatePage flag.
     Only a line is updates provided that bUpdateLine flag is set.
     bUpdatePage takes presecendse and overrides bUpdateLine.
     If none of the flags is set, the function exists
     without updating the display.
*/
static void DisplayPage(TInfoPageContext *pCtx)
{
  int i;
  int j;
  TLineOutput Buf[MAX_WIN_WIDTH];
  CharInfo OutputBuf[MAX_WIN_WIDTH];
  CharInfo *p;
  const TLineOutput *b;
  int nLine;
  int nBlockStart;
  int nBlockEnd;

  ASSERT(pCtx->y1 < pCtx->y2);
  ASSERT(pCtx->x1 < pCtx->x2);

  GotoXY(pCtx->nCol - pCtx->nWrtEdge + pCtx->x1,
    pCtx->nRow - pCtx->nTopLine + pCtx->y1);

  pCtx->nWidth = pCtx->x2 - pCtx->x1 + 1;
  pCtx->nPos = pCtx->nTopLinePos;
  pCtx->bEOP = FALSE;
  if (pCtx->bUpdatePage)
    pCtx->bUpdateLine = FALSE;  /* updatepage overrides updateline */
  if (pCtx->bUpdateLine)
  {
    i = (pCtx->nRow - pCtx->nTopLine) + pCtx->y1;
    pCtx->nPos = pCtx->nLinePos;
    goto _display_line;
  }
  if (!(pCtx->bUpdatePage || pCtx->bUpdateLine))
    return;
  for (i = pCtx->y1; i <= pCtx->y2; ++i)
  {
_display_line:
    DisplayLine(pCtx, Buf);

    /* transform from Buf to OutputBuf
    this code was borrowed from the text file wline() function.
    however, we don't need to map colors here */
    p = OutputBuf;
    b = Buf;

    for (j = 0; j < ScreenWidth; ++j)
    {
      PutChar(p, b->c);
      PutAttr(p, GetColor((int)b->t));
      ++p;
      ++b;
    }

    nLine = pCtx->nTopLine + (i - pCtx->y1);
    if (SetBlockPos(pCtx, nLine, &nBlockStart, &nBlockEnd, pCtx->nWidth))
      PutAttrBlock(GetColor(coHelpTextSelected), pCtx->nWrtEdge, pCtx->nWidth,
        nBlockStart, nBlockEnd, OutputBuf);
    puttextblock(pCtx->x1, i,
      pCtx->x1 + pCtx->nWidth, i, OutputBuf);
    if (pCtx->bUpdateLine)
      break;
  }

  pCtx->bUpdateLine = FALSE;
  pCtx->bUpdatePage = FALSE;

  GotoXY(pCtx->nCol - pCtx->nWrtEdge + pCtx->x1,
    pCtx->nRow - pCtx->nTopLine + pCtx->y1);
}

/* ************************************************************************
   Function: FindPos
   Description:
     Walks in through the current line to establish nCurPos for the
     new target column.
     Sets bAtALink if the cursor is highlithing a link.
     Stores the link at pCtx->psLink (if the link fits in
     nMaxLinkSize characters)
     links examples:
     *Note unixtex.ftp::.
    (*note Readline Variables::).

   TODO: This function along with DisplayLine() grew overly complex
   It needs total redesign. Now it is on the verge of being unmaintainable.
*/
static void FindPos(TInfoPageContext *pCtx, int nNewCol)
{
  char c;
  char sToken[9];
  char *p;
  BOOLEAN bLink;
  int nSavePos;
  int nLinkPos;
  int nCol;
  BOOLEAN bOld_AtALink;
  BOOLEAN bWholeLinkDisplayed;
  BOOLEAN bPeek;
  BOOLEAN bLinkStartsAtPrevLine;
  BOOLEAN bIndexType;
  char cEndMarker;
  BOOLEAN bGetLink;

  ASSERT(sizeof(sToken) >= sizeof(sNext));
  ASSERT(sizeof(sToken) >= sizeof(sPrev));
  ASSERT(sizeof(sToken) >= sizeof(sUp));

  nCol = 0;
  pCtx->nPos = pCtx->nLinePos;
  bOld_AtALink = pCtx->bAtALink;
  sLinkBuf[0] = '\0';

  if (pCtx->bAtALink)
  {
    if (pCtx->nOldRow != pCtx->nRow)
      pCtx->bUpdatePage = TRUE;  /* we left an activated link for sure */
  }

  pCtx->bAtALink = FALSE;

  if (pCtx->nLastPos > 0)
    if (pCtx->nPos >= pCtx->nLastPos)
      goto _after_eol;

  if (TopLine(pCtx))
  {
    /*
    We are at the top line of a page
    We have the node names after "Next:", "Prev:" and "Up:" presented as links
    */
    if (!GetNextChar(pCtx, &c))
    {
_failure1:
      pCtx->nCurPos = -1;
      return;
    }
    if (c != '\x1f')
      goto _failure1;  /* we expect a marker that is not present -> cancel */
    do
    {
      if (!GetNextChar(pCtx, &c))
        goto _failure1;
    } while (!isalpha(c));
    /* Skip the "File:" and the file itself */
    while (GetNextChar(pCtx, &c))
    {
      if (c == ':')  /* this is the separator we need */
      {
        ++pCtx->nPos;  /* skip the space as well */
        break;
      }
    }
    /* Skip the filename */
    while (GetNextChar(pCtx, &c))
    {
      if (isspace(c))
        break;
    }
    /* Skip to first no-blank */
    while (GetNextChar(pCtx, &c))
    {
      if (!isspace(c))
      {
        --pCtx->nPos;  /* as the loop below starts with GetNextChar() */
        break;
      }
    }
    /* Now walk the characters and look for "Next:", "Prev:" or "Up:" */
    p = sToken;
    while (c != '\n')
    {
      if (!GetNextChar(pCtx, &c))
        goto _failure1;
      if (c > ' ')
        if (p - sToken < sizeof(sToken) - 1)
          *p++ = c;
      if (c == ' ')
      {
        *p = '\0';
        pCtx->nCurPos = pCtx->nPos;  /* CurPos at the start of the link */
        if (strcmp(sToken, sNext) == 0 ||
          strcmp(sToken, sPrev) == 0 ||
          strcmp(sToken, sUp) == 0)
        {
          /*
          We are at a link after "Next", "Prev" or "Up"
          */
          pCtx->bAtALink = TRUE;
          while (1)
          {
            if (!GetNextChar(pCtx, &c))
              goto _failure1;
            if (c == '\n')
            {
              pCtx->bAtALink = FALSE;
              break;
            }
            ++nCol;
            if (c == ',')
            {
              pCtx->bAtALink = FALSE;
              break;
            }
            if (nCol == nNewCol)
            {
              pCtx->nCol = nCol;
              break;  /* and nCurPos is at the start of the link */
            }
          }

_copy_wholelink:
          /*
          Copy the whole link in psLink
          */
          if (pCtx->bAtALink != bOld_AtALink)
            pCtx->bUpdateLine = TRUE;  /* link activated/deactivated */
          if (pCtx->bAtALink)
          {
            pCtx->nPos = pCtx->nCurPos;
            p = pCtx->psLink;
            while (p - pCtx->psLink < pCtx->nMaxLinkSize)
            {
              GetNextChar(pCtx, &c);  /* should always succeed */
              if (c == ',' || c == '\n' || c == '.')
                break;
              *p++ = c;
            }
            *p = '\0';
            return;
          }

          if (c == '\n')
          {
            pCtx->nCol = nNewCol;  /* after the end of the line */
            pCtx->nCurPos = pCtx->nPos;
            return;
          }
        }
        p = sToken;
      }
      ++nCol;
      if (nCol == nNewCol)
      {
        pCtx->nCol = nCol;
        pCtx->nCurPos = pCtx->nPos;
        if (pCtx->bAtALink != bOld_AtALink)
          pCtx->bUpdateLine = TRUE;  /* link activated/deactivated */
        return;
      }
    }
    return;
  }

  if (PartOfMenu(pCtx))
  {
    if (!GetNextChar(pCtx, &c))
      goto _failure1;
    if (c == '\x1f')  /* this is end-of-page marker */
    {
      pCtx->nCol = nCol;
      pCtx->nCurPos = pCtx->nPos;
      if (pCtx->bAtALink != bOld_AtALink)
        pCtx->bUpdatePage = TRUE;  /* we are at a different line */
    }
    --pCtx->nPos;  /* ungetch() */
    /* This line is a part of a menu
    Check whether the line begins with a token marked by '::' to display it
    as a link */
    bLink = FALSE;
    nSavePos = pCtx->nPos;
    bIndexType = FALSE;
    if (!GetNextChar(pCtx, &c))
      goto _failure1;
    if (c == '\n')
    {
      --pCtx->nPos;  /* re-read this char as a normal-line */
      goto _normal_line;
    }
    if (c == '*')
    {
      do
      {
        /* Go to the end and check for a dot */
        if (!GetNextChar(pCtx, &c))
          goto _failure1;  /* invalid line */
        if (c == ':' && pCtx->cPrev == ':')
        {
          /* :: is an indicator of a normal *Menu */
          bIndexType = FALSE;  /* it is not a generated index */
          cEndMarker = ':';
          bLink = TRUE;
          goto _item;
        }
      }
      while (c != '.');
      if (!GetNextChar(pCtx, &c))
        goto _failure1;  /* invalid line */
      if (c == '\n')
      {
        /* we have ".\n" */
        bIndexType = TRUE;
        bLink = TRUE;
        cEndMarker = '.';
        goto _item;
      }
      pCtx->nPos = nSavePos + 1;  /* restore position */

      while (1)
      {
        if (!GetNextChar(pCtx, &c))
          goto _failure1;
        if (c == '\n')
          break;
        if (c != ':')
          continue;
        if (!GetNextChar(pCtx, &c))
          goto _failure1;
        bLink = TRUE;
        bIndexType = TRUE;
        if (c == ':')
        {
          cEndMarker = ':';
          bIndexType = FALSE;
          break;
        }
        /* check for ': (' */
        if (!GetNextChar(pCtx, &c))
          break;
        if (c != '(')
          break;
        bIndexType = FALSE;  /* it is not a generated index */
        cEndMarker = '.';
        break;
      }
    }
    else
    {
      /* not a link line ! */
      pCtx->nPos = nSavePos;
      goto _normal_line;
    }
_item:
    pCtx->nPos = nSavePos;
    if (bLink)
      pCtx->nPos += 2;  /* Skip leading '* ' pair */
    if (bIndexType)
    {
      /* This is the generated Index at
      the end of the info file */
      /* Peek to check for double colon */
_display_item:
      do
      {
        /* walk until ':' */
        if (!GetNextChar(pCtx, &c))
          goto _failure1;  /* invalid line */
        if (c != ':')
        {
          if (nCol == nNewCol)
            goto _after_eol;
          ++nCol;
        }
      }
      while (c != ':');
      /*
      Walk to end of the line to check for ':'
      */
      nSavePos = pCtx->nPos;
      do
      {
        if (!GetNextChar(pCtx, &c))
          goto _failure1;  /* invalid line */
        if (c == ':')
        {
          pCtx->nPos = nSavePos;
          goto _display_item;
        }
      }
      while (c != '.');
      pCtx->nPos = nSavePos;
      /*
      Walk until the first non-blank
      */
      do
      {
        if (!GetNextChar(pCtx, &c))
          goto _failure1;  /* invalid line */
        if (c == ' ')
        {
          ++nCol;
          if (nCol == nNewCol)
          {
            --pCtx->nPos;  /* as DisplayLine() looks for pos befo GetNextChar() */
            goto _after_eol;
          }
        }
        else
          --pCtx->nPos;  /* ungetch */
      }
      while (c == ' ');
      /*
      Walk the link
      */
      nSavePos = pCtx->nPos;  /* save the start of the link */
      do
      {
        if (!GetNextChar(pCtx, &c))
          goto _failure1;  /* invalid line */
        if (c == '\n')
          goto _after_eol;
        ++nCol;
        if (nCol == nNewCol)
        {
          pCtx->bAtALink = TRUE;
          pCtx->nCurPos = nSavePos;  /* CurPos at the start of the link */
          pCtx->nCol = nCol;
          goto _copy_wholelink;
        }
      }
      while (c != '.');
      goto _after_eol;
    }
    /*
    Non index type of menu
    */
    nSavePos = pCtx->nPos;  /* start of the link position */
    pCtx->bAtALink = TRUE;
    if (nCol == nNewCol)
      goto _copy_menu_link;
    while (1)
    {
      if (!GetNextChar(pCtx, &c))
        goto _failure1;
      if (bLink)
      {
        if (c == ':')
        {
          pCtx->bAtALink = FALSE;
          if (GetNextChar(pCtx, &c))
          {
            if (c == ':')  /* double ':' */
            {
              bLink = FALSE;
              /* At the end of the menu item (link):
              make up for the removed * and :: characters
              to fully mimic the original Info display behaviour
              this is done in regard to the fact that some of the lines
              may contain tab (\x9) characters and we need the same as the
              original (read Info) column disposition on the screen */
              nCol = nCol + 4;
              if (nCol >= nNewCol)
              {
                nCol = nNewCol;
                goto _after_eol;
              }
              continue;
            }
            else
            {
              /* check for ': (' */
              if (!GetNextChar(pCtx, &c))
                continue;
              if (c != '(')
                continue;
              bLink = FALSE;
              continue;
            }
          }
        }
      }
      if (c == '\n')
        goto _after_eol;
      if (c == '\t')
      {
        nCol = nCol + 8 - nCol % 8;
        if (nCol >= nNewCol)
        {
          nCol = nNewCol;
          goto _after_eol;
        }
        continue;
      }
      if (nCol == nNewCol)
      {
        if (bLink)
        {
          /*
          Export the link in the link buffer
          */
_copy_menu_link:
          pCtx->nPos = nSavePos;
          p = pCtx->psLink;
          while (1)
          {
            if (!GetNextChar(pCtx, &c))
              goto _failure1;
            if (c == cEndMarker)
              break;
            if (c == '\n')
              break;
            if (p - pCtx->psLink < pCtx->nMaxLinkSize)
              *p++ = c;
          }
          *p = '\0';
          pCtx->nPos = nSavePos;  /* nPos should be at the start of the link in */
        }
        goto _after_eol;        /* order to be highlighted                    */
      }
      ++nCol;
    }
    goto _after_eol;
  }

  /*
  Not a top line and not part of a menu.
  Walk through this line.
  */
_normal_line:
  bPeek = FALSE;
  bLinkStartsAtPrevLine = FALSE;
  bGetLink = FALSE;
  if (LinkStartsAtPrevLine(pCtx, &nLinkPos))
  {
    /* Extract the whole destination of the link
    by combining from what is at the previous line and
    at the start of thi line */
    bWholeLinkDisplayed = FALSE;
    if (strchr(sLinkBuf, ':') != NULL)
      bWholeLinkDisplayed = TRUE;
    strncpy(pCtx->psLink, sLinkBuf, pCtx->nMaxLinkSize);
    pCtx->psLink[pCtx->nMaxLinkSize] = '\0';
    bLinkStartsAtPrevLine = TRUE;
    pCtx->bAtALink = TRUE;
    /* we still have a portion to copy */
    p = strchr(sLinkBuf, '\0');
    if (sLinkBuf[0] != '\0')
    {
      *p++ = ' ';  /* separate space */
      *p = '\0';
    }
    strncpy(pCtx->psLink, sLinkBuf, pCtx->nMaxLinkSize);
    pCtx->psLink[pCtx->nMaxLinkSize] = '\0';
    bGetLink = TRUE;
    nSavePos = pCtx->nLinePos;
    if (bWholeLinkDisplayed)
    {
      p = strchr(pCtx->psLink, '\0');
      goto _copy_link;
    }
    goto _process_link;
  }
  do
  {
    if (nCol == nNewCol)
    {
      pCtx->nCol = nCol;
      pCtx->nCurPos = pCtx->nPos;
      if (GetNextChar(pCtx, &c))
        if (c == sNote1[0])
        {
          bPeek = TRUE;  /* pass the loop with this peek flag to check for a link */
          goto _process_char;
        }
      if (pCtx->bAtALink != bOld_AtALink)
        pCtx->bUpdatePage = TRUE;  /* link activated/deactivated */
      return;
    }
    if (!GetNextChar(pCtx, &c))
      goto _failure1;  /* end of data */
    if (c == sNote1[0])
    {
      /* Check whether this is a *note or *Note */
_process_char:
      nSavePos = pCtx->nPos;
      p = sToken;
      *p++ = c;
      while (pCtx->nPos - nSavePos < sizeof(sNote1))
      {
        if (!GetNextChar(pCtx, &c))
        {
          /* we are unable to deliver a complete token */
          goto _restore_pos;
        }
        if (c == '\n')
          break;
        if (c == ' ')
          break;
        *p++ = c;
      }
      *p = '\0';
      pCtx->bAtALink = TRUE;
      *pCtx->psLink = '\0';
      bGetLink = TRUE;
      if (strcmp(sToken, sNote1) == 0 ||
        strcmp(sToken, sNote2) == 0)
      {
        /* skip the space */
        if (c == '\n')  /* *note at the end of the line */
          goto _after_eol;
_process_link:
        p = strchr(pCtx->psLink, '\0');
        while (1)
        {
          if (nCol == nNewCol)
          {
            pCtx->nCol = nCol;
            pCtx->nCurPos = nSavePos;
            if (bLinkStartsAtPrevLine)
              pCtx->nCurPos = nLinkPos;
            if (pCtx->bAtALink != bOld_AtALink)
              pCtx->bUpdatePage = TRUE;  /* link activated/deactivated */
            if (bGetLink)  /* we have a link to extract */
            {
              pCtx->bUpdateLine = TRUE;  /* link activated/deactivated */
              /* Go to the end of the line to check
              whether the whole link is contained */
_copy_link:
              while (1)
              {
                if (!GetNextChar(pCtx, &c))
                  break;  /* this is most likely an invalid line */
                if (c == ':')
                  if (pCtx->cPrev == ':')
                  {
                    /* '::' is end of link */
                    --p;  /* remove the trailing ':' from the link */
                    break;
                  }
                if (c == '.')
                  break;
                if (c == '\x1f' || c == '\n')
                {
                  /*
                  Extract the rest of the link in sLinkBuf
                  */
                  bWholeLinkDisplayed = FALSE;
                  *p++ = ' ';
                  if (c == '\x1f')
                    goto _put_eos;
                  while (GetNextChar(pCtx, &c))
                  {
                    if (c == ':')
                      if (pCtx->cPrev == ':')
                        goto _put_eos;
                    if (c == '.')
                      goto _put_eos;
                    if (c == '\n')
                      goto _put_eos;
                    if (c == '\x1f')
                      goto _put_eos;
                    if (p - pCtx->psLink > pCtx->nMaxLinkSize)
                      goto _put_eos;
                    *p++ = c;
                  }
                  *p = '\0';
                  break;
                }
                if (p - pCtx->psLink > pCtx->nMaxLinkSize)
                  goto _put_eos;
                *p++ = c;
              }
_put_eos:
              *p = '\0';
              if (pCtx->bAtALink)
                if (!bWholeLinkDisplayed)
                  pCtx->bUpdatePage = TRUE;  /* this link is at a double line (this is 2nd line) */
            }
            return;
          }
          if (!GetNextChar(pCtx, &c))
            goto _restore_pos;
          if (c == ':')
            break;
          if (c == '\n')
          {
            pCtx->bAtALink = FALSE;
            goto _after_eol;
          }
          *p++ = c;
          ++nCol;
        }
        pCtx->bAtALink = FALSE;
        /* skip to '.' or '::' */
        while (c != '.')
        {
          if (c == ':')
            if (pCtx->cPrev == ':')
              break;
          if (!GetNextChar(pCtx, &c))
            goto _restore_pos;
        }
        if (!GetNextChar(pCtx, &c))
          goto _restore_pos;
      }
      else  /* This is not a link */
      {
_restore_pos:
        pCtx->bAtALink = FALSE;
        pCtx->nPos = nSavePos - 1;
        GetNextChar(pCtx, &c);  /* should always succeed */
      }
    }  /* process a link */
    if (bPeek)
      return;

    if (c == '\x1f')  /* this is end-of-page marker */
    {
      /* nothing more to display, walk the current position
      at the end of the page */
      pCtx->nPos = pCtx->pstBlock->nBlockPos + pCtx->pstBlock->nBytesRead;
      goto _after_eol;
    }
    if (c != '\n')
      ++nCol;
  }
  while (c != '\n');
_after_eol:
  pCtx->nCol = nNewCol;  /* after the end of the line */
  pCtx->nCurPos = pCtx->nPos;
  if (pCtx->bAtALink != bOld_AtALink)
    pCtx->bUpdatePage = TRUE;  /* link activated/deactivated */
}

/* ************************************************************************
   Function: FixWrtPos
   Description:
     pnWrtPos is the first visible position. nCurPos is the current
     cursor position. nWidth is the size of the visible area.
     This function changes pnWrtPos so the character under cursor
     falls in the visible area.
*/
static void FixWrtPos(int *nWrtPos, int nCurPos, int nWidth)
{
  if (nCurPos - *nWrtPos > nWidth - 2)
    *nWrtPos = nCurPos - nWidth + 2;
  else
    if (nCurPos < *nWrtPos)
      *nWrtPos = nCurPos;
}

/* ************************************************************************
   Function: GotoColRow
   Description:
     Positions the cursor at new col and row.
     Takes care to reassign all the corespondent line markers.
*/
static void GotoColRow(TInfoPageContext *pCtx, int nCol, int nRow)
{
  int nNewRow;
  int nNewCol;
  int nOldWrtEdge;
  int nNewTopLine;
  BOOLEAN bUpdatePageInternal;
  int nMostRight;

  nNewRow = pCtx->nRow;
  nNewCol = pCtx->nCol;

  if (nCol >= 0)
    nNewCol = nCol;
  else
    nNewCol = 0;

  if (nRow >= 0)
    nNewRow = nRow;
  else
    nNewRow = 0;

  pCtx->nOldCol = pCtx->nCol;
  pCtx->nOldRow = pCtx->nRow;

  FindLine(pCtx, nNewRow, &pCtx->nRow, &pCtx->nLinePos);
  FindPos(pCtx, nNewCol);

  /*
  Fix page upper left corner
  */
  nOldWrtEdge = pCtx->nWrtEdge;
  nNewTopLine = pCtx->nTopLine;
  FixWrtPos(&pCtx->nWrtEdge, pCtx->nCol, pCtx->nWidth);
  FixWrtPos(&nNewTopLine, pCtx->nRow, pCtx->y2 - pCtx->y1);
  bUpdatePageInternal = FALSE;
  if (pCtx->bIncrementalSearch)
  {
    /* We need to show the entire string on the screen */
    nMostRight = pCtx->nCol + /*strlen(pstSearchContext->sSearch)*/0;
    if (pCtx->nWrtEdge + pCtx->nWidth < nMostRight + 2)
    {
      pCtx->nWrtEdge = nMostRight - pCtx->nWidth + 2;
      bUpdatePageInternal = TRUE;
    }
  }
  if (nNewTopLine != pCtx->nTopLine)
  {
    FindLine(pCtx, nNewTopLine, &pCtx->nTopLine, &pCtx->nTopLinePos);
    bUpdatePageInternal = TRUE;
  }
  if (nOldWrtEdge != pCtx->nWrtEdge)
    bUpdatePageInternal = TRUE;
  if (bUpdatePageInternal)
    pCtx->bUpdatePage = TRUE;
}

/* ************************************************************************
   Function: GotoPage
   Description:
     Moves the cursor one page up or down. The cursor position
     relatively to the screen remains the same.
*/
static void GotoPage(TInfoPageContext *pCtx, BOOLEAN bPageUp)
{
  int nOldPage;
  int nJumpHeight;
  int nNewPageLine;
  int nWinHeight;

  nWinHeight = pCtx->y2 - pCtx->y1;
  nOldPage = pCtx->nTopLinePos;
  nJumpHeight = nWinHeight - 2;
  if (bPageUp)
    nJumpHeight = -nJumpHeight;
  nNewPageLine = pCtx->nTopLine + nJumpHeight;
_find_line:
  if (nNewPageLine < 0)  /* In the beginning of the 1st page */
    GotoColRow(pCtx, pCtx->nCol, 0);
  else
  {
    GotoColRow(pCtx, pCtx->nCol, pCtx->nRow + nJumpHeight);
    FindLine(pCtx, nNewPageLine, &pCtx->nTopLine, &pCtx->nTopLinePos);
    if (nNewPageLine != pCtx->nTopLine)
    {
      /* We tried to go beyond the last line of this page,
      prepare top line to show at least couple of lines */
      nNewPageLine = pCtx->nTopLine - 4;
      goto _find_line;
    }
  }
  if (nOldPage != pCtx->nTopLinePos)
    pCtx->bUpdatePage = TRUE;
}

TInfoPageContext *_pCtx;

/* ************************************************************************
   Function: RenderLine
   Description:
     Navigation functins need a character/type only presentation
     of the screen. We use DisplayLine() to render the line.
     Rendering starts from position 0 and involves the largest
     possible screen width.
*/
static void RenderLine(TLineOutput *pBuf)
{
  int nSaveWrtEdge;
  int nSaveWidth;

  nSaveWidth = _pCtx->nWidth;
  nSaveWrtEdge = _pCtx->nWrtEdge;
  _pCtx->nPos = _pCtx->nLinePos;
  _pCtx->nWidth = MAX_WIN_WIDTH;
  _pCtx->nWrtEdge = 0;
  _pCtx->bEOP = FALSE;
  DisplayLine(_pCtx, pBuf);
  _pCtx->nWidth = nSaveWidth;
  _pCtx->nWrtEdge = nSaveWrtEdge;
}

/* ************************************************************************
   Function: NextLink
   Description:
     Puts the cursor at the start of the next link.
*/
static void NextLink(void)
{
  TLineOutput Buf[MAX_WIN_WIDTH];
  TLineOutput *p;
  int nPos;
  int nLine;
  int nSaveLinePos;
  int nSaveCol;
  int nSaveRow;
  int nSaveTopLine;
  int nSaveTopLinePos;

  /*
  As we use GotoColRow() to navigate while
  searching for the next link we need to
  preserve the current position anchors
  */
  nSaveLinePos = _pCtx->nLinePos;
  nSaveCol = _pCtx->nCol;
  nSaveRow = _pCtx->nRow;
  nSaveTopLine = _pCtx->nTopLine;
  nSaveTopLinePos = _pCtx->nTopLinePos;

  nPos = _pCtx->nCol;
  nLine = _pCtx->nRow;
_process_line:
  RenderLine(Buf);
  p = &Buf[nPos];
  while (p->t != coHelpLink)
  {
    if ((p - Buf) > MAX_WIN_WIDTH)
    {
      /* we are at the end of the line: advance for the next line */
      if (!FindLine(_pCtx, nLine + 1, &nLine, &_pCtx->nLinePos))
        goto _start_of_the_page;
      nPos = 0;
      goto _process_line;
    }
    ++p;
  }
  /* we have a link */
_link:
  _pCtx->nLinePos = nSaveLinePos;
  _pCtx->nCol = nSaveCol;
  _pCtx->nRow = nSaveRow;
  _pCtx->nTopLine = nSaveTopLine;
  _pCtx->nTopLinePos = nSaveTopLinePos;
  GotoColRow(_pCtx, p - Buf, nLine);
  return;

  /*
  Start from the beginning of the page
  */
_start_of_the_page:
  nLine = 0;
  nPos = 0;
  GotoColRow(_pCtx, 0, 0);
  do
  {
    RenderLine(Buf);
    p = &Buf[nPos];
    while (p->t != coHelpLink)
    {
      if ((p - Buf) > MAX_WIN_WIDTH)
      {
        /* we are at the end of the line: advance for the next line */
        if (!FindLine(_pCtx, nLine + 1, &nLine, &_pCtx->nLinePos))
          break;
        nPos = 0;
        goto _end_of_line;
      }
      ++p;
    }
    goto _link;
_end_of_line:
    ;
  }
  while (nLine <= _pCtx->nRow);

  /*
  No links at all
  */
  _pCtx->nLinePos = nSaveLinePos;
  _pCtx->nCol = nSaveCol;
  _pCtx->nRow = nSaveRow;
  _pCtx->nTopLine = nSaveTopLine;
  _pCtx->nTopLinePos = nSaveTopLinePos;
}

/* ************************************************************************
   Function: CmdCharLeft
   Description:
     Moves the cursor one position left.
*/
static void CmdCharLeft(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol - 1, _pCtx->nRow);
}

/* ************************************************************************
   Function: CmdCharRight
   Description:
     Moves the cursor one position right.
*/
static void CmdCharRight(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol + 1, _pCtx->nRow);
}

/* ************************************************************************
   Function: CmdLineUp
   Description:
     Moves the cursor one line up.
*/
static void CmdLineUp(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol, _pCtx->nRow - 1);
}

/* ************************************************************************
   Function: CmdLineDown
   Description:
     Moves the cursor one line down.
*/
static void CmdLineDown(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol, _pCtx->nRow + 1);
}

/* ************************************************************************
   Function: ExtractALine
   Description:
     Word navigation functions call GotoWord() that needs a complete line
     of text. This function extracts a complete line of text in a buffer.
*/
static void ExtractALine(TInfoPageContext *pCtx, char *psBuf)
{
  TLineOutput Buf[MAX_WIN_WIDTH];
  TLineOutput *p;
  char *d;
  char *pLastNonSpace;

  RenderLine(Buf);
  p = Buf;
  d = psBuf;
  pLastNonSpace = d;
  while (p - Buf < MAX_WIN_WIDTH)
  {
    if (p->c != ' ')
      pLastNonSpace = d + 1;
    *d++ = p->c;
    ++p;
  }
  *d = '\0';
  *pLastNonSpace = '\0';  /* cut after the last non-blank */
}

/* ************************************************************************
   Function: IsAlpha
   Description:
     Checks whether a character is alpha:
     A-Z, a-z, 0-9, 128-254, _
     and sSuplSet (supplementary set)
*/
static BOOLEAN InAlpha(char c)
{
  if (c >= 'A' && c <= 'Z')
    return TRUE;
  if (c >= 'a' && c <= 'z')
    return TRUE;
  if (c >= '0' && c <= '9')
    return TRUE;
  if ((unsigned char)c >= 128 && (unsigned char)c <= 254)
    return TRUE;
  if (c == '_')
    return TRUE;

  return FALSE;
}

/* ************************************************************************
   Function: FirstCharInAlpha
   Description:
     Checks whether the first character in the line is alpha character.
*/
static BOOLEAN FirstCharInAlpha(TInfoPageContext *pCtx)
{
  TLineOutput Buf[MAX_WIN_WIDTH];

  RenderLine(Buf);
  return InAlpha(Buf[0].c);
}

extern void GotoWord(char *sLine, int nLnLen, int *nPos, int nDirection, BOOLEAN *bGotoLine,
  char *sSuplSet, BOOLEAN bGotoEndOfWord);

/* ************************************************************************
   Function: CmdNextWord
   Description:
     Moves the cursor to the start of the next word.
*/
static void CmdNextWord(void *pCtx)
{
  char sBuf[MAX_WIN_WIDTH];  /* usually the lines are 80 characters long */
  BOOLEAN bGotoLine;
  int nPos;

_find_loop:
  ExtractALine(_pCtx, sBuf);
  nPos = _pCtx->nCol;
  GotoWord(sBuf, strlen(sBuf), &nPos, 1, &bGotoLine, NULL, FALSE);

  if (bGotoLine)
  {
    GotoColRow(_pCtx, 0, _pCtx->nRow + 1);
    /* check whether we point at a non-blank character */
    _pCtx->nPos = _pCtx->nLinePos;
    if (!FirstCharInAlpha(_pCtx))
      goto _find_loop;
  }
  else
    GotoColRow(_pCtx, nPos, _pCtx->nRow);  /* Move in the range of the current line */
}

static void CmdEnd(void *pCtx);

/* ************************************************************************
   Function: CmdPrevWord
   Description:
     Moves the cursor to the start of the previous word.
*/
static void CmdPrevWord(void *pCtx)
{
  char sBuf[MAX_WIN_WIDTH];  /* usually the lines are 80 characters long */
  BOOLEAN bGotoLine;
  int nPos;

  if (_pCtx->nRow == 0 && _pCtx->nCol == 0)
    return;

  ExtractALine(_pCtx, sBuf);
  nPos = _pCtx->nCol;
  GotoWord(sBuf, strlen(sBuf), &nPos, -1, &bGotoLine, NULL, FALSE);

  if (bGotoLine)
  {
    CmdLineUp(pCtx);
    CmdEnd(pCtx);
    return;
  }
  else
    GotoColRow(_pCtx, nPos, _pCtx->nRow);   /* Move in the range of the current line */

}

/* ************************************************************************
   Function: CmdHome
   Description:
     Moves the cursor at the start of the line.
*/
static void CmdHome(void *pCtx)
{
  GotoColRow(_pCtx, 0, _pCtx->nRow);
}

/* ************************************************************************
   Function: CmdEnd
   Description:
     Moves the cursor after the last character of the line.
*/
static void CmdEnd(void *pCtx)
{
  TLineOutput Buf[MAX_WIN_WIDTH];
  TLineOutput *p;
  int nCol;

  RenderLine(Buf);
  p = &Buf[MAX_WIN_WIDTH - 1];
  while (isspace(p->c))
  {
    --p;
    if (p == Buf)
    {
      nCol = 0;  /* line is empty */
      goto _gotopos;
    }
  }
  nCol = p - Buf + 1;  /* after the last character */
_gotopos:
  GotoColRow(_pCtx, nCol, _pCtx->nRow);
}

/* ************************************************************************
   Function: CmdTopOfPage
   Description:
     Moves the cursor at the start of the current hypertext page.
*/
static void CmdTopOfPage(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol, 0);
}

/* ************************************************************************
   Function: CmdBottomOfPage
   Description:
     Moves the cursor at the bottom of the current hypertext page.
*/
static void CmdBottomOfPage(void *pCtx)
{
  GotoColRow(_pCtx, _pCtx->nCol, 10000);  /* 10000 is a big enough number */
}

/* ************************************************************************
   Function: CmdPageUp
   Description:
     Moves the cursor a page up.
*/
static void CmdPageUp(void *pCtx)
{
  GotoPage(_pCtx, TRUE);
}

/* ************************************************************************
   Function: CmdPageDown
   Description:
     Moves the cursor a page down.
*/
static void CmdPageDown(void *pCtx)
{
  GotoPage(_pCtx, FALSE);
}

/* ************************************************************************
   Function: CmdMarkBlockBegin
   Description:
     Marks the start of a block.
*/
static void CmdMarkBlockBegin(void *pCtx)
{
  _pCtx->nStartLine = _pCtx->nRow;
  _pCtx->nStartPos = _pCtx->nCol;

  /*
  Check whether the block is valid
  */
  _pCtx->bBlock = FALSE;  /* assume invalid */
  _pCtx->bUpdatePage = TRUE;

  if (_pCtx->nStartLine == -1 || _pCtx->nEndLine == -1)
    return;

  if (_pCtx->nStartLine > _pCtx->nEndLine)
    return;

  if (_pCtx->bColumn)
  {
    if (_pCtx->nStartPos <= _pCtx->nEndPos)
      _pCtx->bBlock = TRUE;
    return;
  }

  if (_pCtx->nStartLine == _pCtx->nEndLine)
    if (_pCtx->nStartPos > _pCtx->nEndPos)
      return;

  _pCtx->bBlock = TRUE;
}

/* ************************************************************************
   Function: CmdMarkBlockEnd
   Description:
     Marks the end of a block.
*/
static void CmdMarkBlockEnd(void *pCtx)
{
  _pCtx->nEndLine = _pCtx->nRow;
  _pCtx->nEndPos = _pCtx->nCol;
  if (!_pCtx->bColumn)
    --_pCtx->nEndPos;

  /*
  Check whether the block is valid
  */
  _pCtx->bBlock = FALSE;  /* assume invalid */
  _pCtx->bUpdatePage = TRUE;

  if (_pCtx->nStartLine == -1 || _pCtx->nEndLine == -1)
    return;

  if (_pCtx->nStartLine > _pCtx->nEndLine)
    return;

  if (_pCtx->bColumn)
  {
    if (_pCtx->nStartPos <= _pCtx->nEndPos)
      _pCtx->bBlock = TRUE;
    return;
  }

  if (_pCtx->nStartLine == _pCtx->nEndLine)
    if (_pCtx->nStartPos > _pCtx->nEndPos)
      return;

  _pCtx->bBlock = TRUE;
}

static void CmdToggleBlockHide(void *pCtx)
{
}

static void CmdSelectAll(void *pCtx)
{
}

static void CmdCharLeftExtend(void *pCtx)
{
}

static void CmdCharRightExtend(void *pCtx)
{
}

static void CmdLineUpExtend(void *pCtx)
{
}

static void CmdLineDownExtend(void *pCtx)
{
}

static void CmdPageUpExtend(void *pCtx)
{
}

static void CmdPageDownExtend(void *pCtx)
{
}

static void CmdHomeExtend(void *pCtx)
{
}

static void CmdEndExtend(void *pCtx)
{
}

static void CmdTopOfPageExtend(void *pCtx)
{
}

static void CmdBottomOfPageExtend(void *pCtx)
{
}

static void CmdNextWordExtend(void *pCtx)
{
}

static void CmdPrevWordExtend(void *pCtx)
{
}

static void CmdCopy(void *pCtx)
{
}

static void CmdBackspace(void *pCtx)
{
}

static void CmdIncrementalSearch(void *pCtx)
{
}

static void CmdIncrementalSearchBack(void *pCtx)
{
}

static void CmdFindSelected(void *pCtx)
{
}

static void CmdFindSelectedBack(void *pCtx)
{
}

static void CmdFindNext(void *pCtx)
{
}

static void CmdFindBack(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdEnter
   Description:
     Exits the viewer if the cursor is over a link.
*/
static void CmdEnter(void *pCtx)
{
  if (_pCtx->bAtALink)
    _pCtx->bQuit = TRUE;
}

/* ************************************************************************
   Function: CmdToggleColumnBlockMode
   Description:
*/
static void CmdToggleColumnBlockMode(void *pCtx)
{
}

/* ************************************************************************
   Function: CmdGotoNextLink
   Description:
*/
static void CmdGotoNextLink(void *pCtx)
{
  NextLink();
}

/* ************************************************************************
   Function: CmdGotoPrevLink
   Description:
*/
static void CmdGotoPrevLink(void *pCtx)
{
}

/* ************************************************************************
   Function: ExamineKey
   Description:
*/
static void ExamineKey(DWORD dwKey)
{
#if 0
  char *p;

  if (dwKey == 0xffff)  /* timer indication */
    return;

  if (pKeyBlock == NULL)
    pKeyBlock = MakeBlock(" ", 0);

  if (pKeyBlock == NULL)
    return;

  p = GetBlockLine(pKeyBlock, 0)->pLine;
  *p = ASC(dwKey);

  if (dwKey == KEY(0, kbEsc))
  {
    /*
    If pressing ESC in bInctementalSearch mode return the
    last positions prior conducting the search.
    */
    if (bIncrementalSearch)
    {
      GotoColRow(pFile, nPreISearch_Col, nPreISearch_Row);
      return;
    }
  }

  if (*p == 0 || *p == '\n' || *p == '\r')
    return;

  if (bIncrementalSearch)
  {
    IncrementalSearch(pFile, p, ScreenWidth, &stSearchContext);
    bPreserveIncrementalSearch = TRUE;
    return;
  }
#endif
}

static void CmdHelpKbd2(void *pCtx);

/*
This array comprises the list of commands whose assigned
key combinations to be extracted from the main editor key
bindings table.
*/
static int nNavCommands[] =
{
  cmEditCharLeft,
  cmEditCharRight,
  cmEditLineUp,
  cmEditLineDown,
  cmEditNextWord,
  cmEditPrevWord,
  cmEditHome,
  cmEditEnd,
  cmEditTopFile,
  cmEditBottomFile,
  cmEditPageUp,
  cmEditPageDown,

  cmEditMarkBlockBegin,
  cmEditMarkBlockEnd,
  cmEditToggleBlockHide,
  cmEditSelectAll,
  cmEditCharLeftExtend,
  cmEditCharRightExtend,
  cmEditLineUpExtend,
  cmEditLineDownExtend,
  cmEditPageUpExtend,
  cmEditPageDownExtend,
  cmEditHomeExtend,
  cmEditEndExtend,
  cmEditTopFileExtend,
  cmEditBottomFileExtend,
  cmEditNextWordExtend,
  cmEditPrevWordExtend,
  cmEditCopy,
  cmEditBackspace,

  cmEditIncrementalSearch,
  cmEditIncrementalSearchBack,
  cmEditFindSelected,
  cmEditFindSelectedBack,
  cmEditFindNext,
  cmEditFindBack,

  cmEditEnter,

  cmOptionsToggleColumnBlock,
  cmHelpKbd,
};

/* Prepare space for all of the commands to have more than one
key binding */
static TKeySequence NavigateUI[(_countof(nNavCommands) + 15) * 2];
static int nNumberOfCommands;

enum HelpCommandSet
{
  cmHelpGotoNextLink = cmDiag,  /* last of the main editor set */
  cmHelpGotoPrevLink
};

static TKeySequence HelpUI[] =
{
  {cmHelpGotoNextLink, DEF_KEY1(KEY(0, kbTab))},
  {cmHelpGotoPrevLink, DEF_KEY1(KEY(kbShift, kbTab))}
};

/* All commands in nSmallEditCommands should be described here */
static TCmdDesc NavigateUICommands[] =
{
  {cmEditCharLeft, "CharLeft", CmdCharLeft},
  {cmEditCharRight, "CharRight", CmdCharRight},
  {cmEditLineUp, "LineUp", CmdLineUp},
  {cmEditLineDown, "LineDown", CmdLineDown},
  {cmEditNextWord, "NextWord", CmdNextWord},
  {cmEditPrevWord, "PrevWord", CmdPrevWord},
  {cmEditHome, "Home", CmdHome},
  {cmEditEnd, "End", CmdEnd},
  {cmEditTopFile, "TopOfPage", CmdTopOfPage},
  {cmEditBottomFile, "BottomOfPage", CmdBottomOfPage},
  {cmEditPageUp, "PageUp", CmdPageUp},
  {cmEditPageDown, "PageDown", CmdPageDown},

  {cmEditMarkBlockBegin, "MarkBlockBegin", CmdMarkBlockBegin},
  {cmEditMarkBlockEnd, "MarkBlockEnd", CmdMarkBlockEnd},
  {cmEditToggleBlockHide, "ToggleBlockHide", CmdToggleBlockHide},
  {cmEditSelectAll, "SelectAll", CmdSelectAll},
  {cmEditCharLeftExtend, "CharLeftExtend", CmdCharLeftExtend},
  {cmEditCharRightExtend, "CharRightExtend", CmdCharRightExtend},
  {cmEditLineUpExtend, "LineUpExtend", CmdLineUpExtend},
  {cmEditLineDownExtend, "LineDownExtend", CmdLineDownExtend},
  {cmEditPageUpExtend, "PageUpExtend", CmdPageUpExtend},
  {cmEditPageDownExtend, "PageDownExtend", CmdPageDownExtend},
  {cmEditHomeExtend, "HomeExtend", CmdHomeExtend},
  {cmEditEndExtend, "EndExtend", CmdEndExtend},
  {cmEditTopFileExtend, "TopOfPageExtend", CmdTopOfPageExtend},
  {cmEditBottomFileExtend, "BottomOfPageExtend", CmdBottomOfPageExtend},
  {cmEditNextWordExtend, "NextWordExtend", CmdNextWordExtend},
  {cmEditPrevWordExtend, "PrevWordExtend", CmdPrevWordExtend},
  {cmEditCopy, "Copy", CmdCopy},
  {cmEditBackspace, "Backspace", CmdBackspace},

  {cmEditIncrementalSearch, "IncrementalSearch", CmdIncrementalSearch},
  {cmEditIncrementalSearchBack, "IncrementalSearchBack", CmdIncrementalSearchBack},
  {cmEditFindSelected, "FindSelected", CmdFindSelected},
  {cmEditFindSelectedBack, "FindSelectedBack", CmdFindSelectedBack},
  {cmEditFindNext, "FindNext", CmdFindNext},
  {cmEditFindBack, "FindBack", CmdFindBack},

  {cmEditEnter, "Enter", CmdEnter},

  {cmOptionsToggleColumnBlock, "ToggleColumnBlockMode", CmdToggleColumnBlockMode},
  {cmHelpKbd, "HelpKbd", CmdHelpKbd2},
  {cmHelpGotoNextLink, "GotoNextLink", CmdGotoNextLink},
  {cmHelpGotoPrevLink, "GotoPrevLink", CmdGotoPrevLink},

  {END_OF_CMD_LIST_CODE, "End_Of_Cmd_List_Code"}  /* LastItem */
};

static TKeySequence TerminalKeySeq = {END_OF_KEY_LIST_CODE};

/* ************************************************************************
   Function:
   Description:
*/
static void CmdHelpKbd2(void *pCtx)
{
  DisplayKeys(NavigateUICommands, NavigateUI);
}

/* ************************************************************************
   Function: ComposeKeySet
   Description:
*/
static int compare_ints(const void *pElem1, const void *pElem2)
{
  const int *i1 = (const int *)pElem1;
  const int *i2 = (const int *)pElem2;

  ASSERT(i1 != NULL);
  ASSERT(i2 != NULL);
  ASSERT(*i1 > 0);
  ASSERT(*i2 > 0);

  return (*i1 - *i2);
}

/* ************************************************************************
   Function: ComposeHyperViewerKeySet
   Description:
     Composes keyset for Hyper Viewer -- extracts the keycodes for edit
     commands.
     Uses the main key set of the editor.
     The result is in NavigateUI[].
*/
void ComposeHyperViewerKeySet(TKeySet *pMainKeySet)
{
  int i;
  int *pnResult;
  TKeySequence *pKeySeq;

  ASSERT(pMainKeySet != NULL);
  ASSERT(pMainKeySet->pKeySet != NULL);

  pKeySeq = NavigateUI;
  for (i = 0; i < pMainKeySet->nNumberOfItems; ++i)
  {
    pnResult = bsearch(&pMainKeySet->pKeySet[i].nCode, nNavCommands,
      _countof(nNavCommands), sizeof(int), compare_ints);
    if (pnResult == NULL)
      continue;

    /* Copy this keybinding in our local table */
    memcpy(pKeySeq, &pMainKeySet->pKeySet[i], sizeof(TKeySequence));
    ++pKeySeq;
  }
  memcpy(pKeySeq, HelpUI, sizeof(HelpUI));
  pKeySeq += _countof(HelpUI);
  /* Put the terminal key sequence to mark the end of the list */
  memcpy(pKeySeq, &TerminalKeySeq, sizeof(TKeySequence));

  nNumberOfCommands = SortCommands(NavigateUICommands);  /* In fact this is necessary only once */
}

/* ************************************************************************
   Function: OnCursorPosChanged
   Description:
     Called whenever a cursor changed position after executing a command
     on the file.
   Note:
     This function is invoked in smalledt.c as well.
*/
static void OnCursorPosChanged(TInfoPageContext  *pCtx)
{
  BOOLEAN bBlockSave;

  bBlockSave = pCtx->bBlock;

  if (!bPersistentBlocks)
  {
    if (!pCtx->bPreserveSelection)
    {
      if (pCtx->bBlock)
        pCtx->bUpdatePage = TRUE;
      pCtx->bBlock = FALSE;
      /* Set invalid values for all the block markers */
      #if 0  /* To keep working Ctrl+KB, Ctrl+KK keys */
      pCtx->nStartLine = -1;
      pCtx->nEndLine = -1;
      pCtx->nStartPos = -1;
      pCtx->nEndPos = -2;
      #endif
      pCtx->nExtendAncorCol = -1;
      pCtx->nExtendAncorRow = -1;
      pCtx->nExpandCol = -1;
      pCtx->nExpandRow = -1;
    }

    if (bBlockSave)
    {
      if (!pCtx->bBlock)  /* We have unmarked the current selecion */
        pCtx->bColumn = FALSE;  /* Column block mode is nonpersistent */
    }
  }
  pCtx->bPreserveSelection = FALSE;
}

/* ************************************************************************
   Function: NavigatePage
   Description:
     Displays and navigates a page from info file.
     The page is presented in blocks of text in a linked list.
     If a link is selected psDestLink gets the result.
*/
int NavigatePage(TInfoPageContext *pCtx)
{
  DWORD dwKey;
  TKeySequence KeySeq;
  int nCmdCode;
  int nExitCode;

  ASSERT(pCtx != NULL);
  ASSERT(!IS_LIST_EMPTY(pCtx->blist));

  pCtx->bQuit = FALSE;
  pCtx->bUpdatePage = TRUE;
  ClearKeySequence(&KeySeq);

  pCtx->nLastPos = -1;  /* undefined */
  pCtx->nDir = 1;
  pCtx->nMenuLinePos = FindMenuLine(pCtx);

  /* Block markers: invalid */
  pCtx->nStartLine = -1;
  pCtx->nStartPos = -1;
  pCtx->nEndLine = -1;
  pCtx->nEndPos = -1;

  pCtx->nWidth = pCtx->x2 - pCtx->x1 + 1;

  while (!pCtx->bQuit)
  {
    if (pCtx->nDestTopLine != -1)
    {
      /* Set a specific position once at startup */
      FindLine(pCtx, pCtx->nDestTopLine, &pCtx->nTopLine, &pCtx->nTopLinePos);
      GotoColRow(pCtx, pCtx->nDestCol, pCtx->nDestRow);
      pCtx->nDestTopLine = -1;
      pCtx->nDestRow = -1;
      pCtx->nDestCol = -1;
    }

    DisplayPage(pCtx);

    if (pCtx->nCol != pCtx->nPrevCol ||
      pCtx->nRow != pCtx->nPrevRow)
      OnCursorPosChanged(pCtx);
    pCtx->nPrevCol = pCtx->nCol;
    pCtx->nPrevRow = pCtx->nRow;

    dwKey = ReadKey();

    if (dwKey == 0xffff)
      continue;

    if (dwKey == KEY(0, kbEsc))
    {
      pCtx->bQuit = TRUE;
      nExitCode = 0;
    }

    AddKey(&KeySeq, dwKey);

    if ((nCmdCode = ChkKeySequence(NavigateUI, &KeySeq)) == -1)
      continue;  /* Still more keys to collect */

    if (nCmdCode == -2)
    {
      /* try the key-map supplied by the caller through the context */
      if (pCtx->pCmdKeyMap != NULL)
        if ((nCmdCode = ChkKeySequence(pCtx->pCmdKeyMap, &KeySeq)) >= 0)
        {
          ASSERT(pCtx->pfnNavProc != NULL);
          nCmdCode = (*pCtx->pfnNavProc)(pCtx, nCmdCode);
          if (nCmdCode == 0)
            goto _clear_seq;
          if (nCmdCode < 0)
          {
            nExitCode = nCmdCode;
            break;  /* exit with the exit code of the user call-back */
          }
          if (nCmdCode == 1)
          {
            pCtx->bUpdatePage = TRUE;
            goto _clear_seq;
          }
          ASSERT(0);
        }
    }

    pCtx->bPreserveIncrementalSearch = FALSE;
    if (nCmdCode == -2)
    {
      if (KeySeq.nNumber == 1)
      {
        ExamineKey(dwKey);
      }
      goto _end_loop;
    }

    /* Dispatched by KeySeq -> nCmdCode */
    _pCtx = pCtx;
    ExecuteCommand(NavigateUICommands, nCmdCode, nNumberOfCommands, NULL);
    nExitCode = 1;

_end_loop:
    if (!pCtx->bPreserveIncrementalSearch)
    {
      if (pCtx->bIncrementalSearch)
      {
        pCtx->bIncrementalSearch = FALSE;
        pCtx->bPreserveSelection = FALSE;  /* ISearch mode exited */
      }
    }
_clear_seq:
    ClearKeySequence(&KeySeq);
  }

  return nExitCode;
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


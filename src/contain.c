/*

File: contain.c
COPYING: Full text of the copyrights statement at the bottom of the file
Project: WW
Started: 1st July, 2002
Descrition:
  Views and splitters container.

*/

#include "global.h"
#include "maxpath.h"
#include "wlimits.h"
#include "contain.h"
#include "wrkspace.h"
#include "kbd.h"
#include "scr.h"
#include "palette.h"

static TContainer stContainers[MAX_CONTAINERS];

/* ************************************************************************
   Function: InsideRegion
   Description:
     Checks whether coordinates fall into a region.
*/
BOOLEAN InsideRegion(int x, int y, int x1, int y1, int x2, int y2)
{
  ASSERT(x2 >= x1);
  ASSERT(y2 >= y1);
  if (x >= x1 && y >= y1 && x <= x2 && y <= y2)
    return TRUE;
  return FALSE;
}

typedef struct _Splitter
{
  int x1;
  int y1;
  int x2;
  int y2;
  int nColor;
  BOOLEAN bInUse;
  BOOLEAN bDoRedraw;
  TContainer *pOwner;
} TSplitter;

static TSplitter splitters[MAX_CONTAINERS / 2 + 1];  /* static -> 0 */

/* ************************************************************************
   Function: SplitterNew
   Description:
     Allocate an unused splitter entry in the splitters array.
*/
static int SplitterNew(TContainer *pOwner)
{
  int i;

  for (i = 0; i < _countof(splitters); ++i)
    if (!splitters[i].bInUse)
      break;

  ASSERT(i != _countof(splitters));  /* Not found -> extend the array */
  memset(&splitters[i], 0, sizeof(splitters[i]));
  splitters[i].bInUse = TRUE;
  splitters[i].pOwner = pOwner;
  return i;
}

/* ************************************************************************
   Function: SplitterInvalidate
   Description:
     Next call for redraw will update the splitter
     on the screen.
*/
static void SplitterInvalidate(int _s)
{
  TSplitter *s;

  /*
  Splitter sanity check
  */
  ASSERT(_s >= 0);
  ASSERT(_s < _countof(splitters));
  ASSERT(splitters[_s].bInUse);

  s = &splitters[_s];
  s->bDoRedraw = TRUE;
}

/* ************************************************************************
   Function: SplitterSetCoordinates
   Description:
     Sets splitter parameters
*/
static void SplitterSetCoordinates(int _s, int x1, int y1, int x2, int y2, int c)
{
  TSplitter *s;

  /*
  Splitter sanity check
  */
  ASSERT(_s >= 0);
  ASSERT(_s < _countof(splitters));
  ASSERT(splitters[_s].bInUse);

  s = &splitters[_s];

  s->bDoRedraw = FALSE;
  if (s->x1 != x1 || s->x2 != x2 ||
    s->y1 != y1 || s->y2 != y2 || s->nColor != c)
    s->bDoRedraw = TRUE;

  s->x1 = x1;
  s->y1 = y1;
  s->x2 = x2;
  s->y2 = y2;

  /*
  Coordinates sanity check
  */
  ASSERT(s->x1 >= 0);
  ASSERT(s->y1 >= 0);
  ASSERT(s->x1 < 1000);  /* size of buff */
  ASSERT(s->y2 < 1000);
  ASSERT(s->x1 <= s->x2);
  ASSERT(s->y1 <= s->y2);
}

/* ************************************************************************
   Function: SplitterDispose
   Description:
     Disposes a splitter.
*/
static void SplitterDispose(int s)
{
  ASSERT(s >= 0);
  ASSERT(s < _countof(splitters));
  ASSERT(splitters[s].bInUse);

  splitters[s].bInUse = FALSE;
}

/* ************************************************************************
   Function: SplitterDraw
   Description:
     Redraws a splitter.
*/
static int SplitterDraw(int _s)
{
  BYTE buff[1000];
  int w;
  char c;
  int i;
  TSplitter *s;

  /*
  Splitter sanity check
  */
  ASSERT(_s >= 0);
  ASSERT(_s < _countof(splitters));
  ASSERT(splitters[_s].bInUse);

  s = &splitters[_s];

  /*
  Coordinates sanity check
  */
  ASSERT(s->x1 >= 0);
  ASSERT(s->y1 >= 0);
  ASSERT(s->x1 < 1000);  /* size of buff */
  ASSERT(s->y2 < 1000);
  ASSERT(s->x1 <= s->x2);
  ASSERT(s->y1 <= s->y2);

  if (!s->bDoRedraw)
    return 0;

  s->bDoRedraw = FALSE;

  /*
  Determine width and splitter character
  */
  if (s->x1 == s->x2)  /* vertical splitter */
  {
    w = s->y2 - s->y1;
    #if USE_ASCII_BOXES
    c = '|';
    #else
    c = '³';
    #endif
    buff[0] = c;
    buff[1] = '\0';
    for (i = 0; i < w; ++i)
    {
      WriteXY((char *)buff, s->x1, s->y1 + i, GetColor(coSmallEdText));
      WriteXY((char *)buff, s->x2, s->y1 + i, GetColor(coSmallEdText));
    }
  }
  else
  {
    ASSERT(s->y1 == s->y2);  /* horizontal splitter */
    w = s->x2 - s->x1;
    #if USE_ASCII_BOXES
    c = '-';
    #else
    c = 'Ä';
    #endif
    memset(buff, c, 1000);
    buff[w + 1] = (BYTE)'\0';  /* End of string marker */
    WriteXY((char *)buff, s->x1, s->y1, GetColor(coSmallEdText));
  }
  return 1;
}

void SetMouseProc(void (*_pfnMouseProc)(struct event *pEvent, void *pContext),
  void *pContext);

typedef struct _SplitterProcContext
{
  int nOriginalX;
  int nOriginalY;
  int _s;
} TSplitterProcContext;

/* ************************************************************************
   Function: SplitterMouseProc
   Description:
     Initial setup of a view.
     Sets the coordinates and geometry. Sets default values for the
     remaining fields.
*/
static void SplitterMouseProc(struct event *pEvent, void *pContext)
{
  int nButtonState;
  int nCorrection;
  int _s;
  TSplitter *s;
  struct event stEventResize1;
  struct event stEventResize2;
  struct event stNewMouse;
  TContainer *pSub1;
  TContainer *pSub2;
  int nNewHeight;
  int y;
  TSplitterProcContext *pCtx;

  pCtx = pContext;

  if (pEvent->t.msg == MSG_RELEASE_MOUSE)
    goto _cancel_mouse_capture;

  nButtonState = pEvent->e.stMouseEvent.nButtonState;
  if ((nButtonState & MOUSE_BUTTON1) == 0)
  {
    SetMouseProc(NULL, NULL);
    return;
  }

  /* Any other button is "cancel operation" */
  if ((nButtonState & ~MOUSE_BUTTON1) != 0)
  {
_cancel_mouse_capture:
    memset(&stNewMouse, 0, sizeof(stNewMouse));
    stNewMouse.t.code = EVENT_MOUSE;
    stNewMouse.e.stMouseEvent.x = pCtx->nOriginalX;
    stNewMouse.e.stMouseEvent.y = pCtx->nOriginalY;
    stNewMouse.e.stMouseEvent.nButtonState = MOUSE_BUTTON1;
    SplitterMouseProc(&stNewMouse, pContext);
    SetMouseProc(NULL, NULL);
    return;
  }

  _s = pCtx->_s;
  ASSERT(_s >= 0);
  ASSERT(_s < _countof(splitters));
  ASSERT(splitters[_s].bInUse);
  s = &splitters[_s];

  if (s->pOwner->bHorizontal)
  {
    ASSERT(s->y1 == s->y2);
    nCorrection = pEvent->e.stMouseEvent.y - s->y1;
    if (nCorrection == 0)
      return;
    pSub1 = s->pOwner->pSub1;
    pSub2 = s->pOwner->pSub2;
    stEventResize1.e.stNewSizeEvent.NewX1 = pSub1->x;
    stEventResize1.e.stNewSizeEvent.NewY1 = pSub1->y;
    stEventResize1.e.stNewSizeEvent.NewWidth = pSub1->nWidth;

    stEventResize2.e.stNewSizeEvent.NewX1 = pSub2->x;
    stEventResize2.e.stNewSizeEvent.NewWidth = pSub2->nWidth;

    nNewHeight = pSub1->nHeight + nCorrection;
    if (nNewHeight < pSub1->nMinHeight)
      return;

    stEventResize1.e.stNewSizeEvent.NewHeight = nNewHeight;

    stEventResize2.e.stNewSizeEvent.NewY1 =
      s->pOwner->y + nNewHeight + 1;
    stEventResize2.e.stNewSizeEvent.NewHeight =
      s->pOwner->nHeight - nNewHeight - 1;
    if (stEventResize2.e.stNewSizeEvent.NewHeight <
      pSub2->nMinHeight)
      return;

    s->pOwner->nProportion = (nNewHeight * 1000) / s->pOwner->nHeight;
  }
  y = stEventResize2.e.stNewSizeEvent.NewY1 - 1;
  SplitterSetCoordinates(_s,
    pSub1->x, y, pSub1->x + pSub1->nWidth - 1, y, s->nColor);
  ContainerHandleEvent(EVENT_RESIZE, pSub1, &stEventResize1);
  ContainerHandleEvent(EVENT_RESIZE, pSub2, &stEventResize2);
  ASSERT((s->pOwner->nHeight * s->pOwner->nProportion) / 1000 -
    pSub1->nHeight < 1);
}

static TSplitterProcContext stSplitterProcCtx;

/* ************************************************************************
   Function: HandleSplitterMouseEvent
   Description:
     Handle mouse event for a specific splitter.
   Returns:
     0 -- event not targeting the splitter;
     1 -- event handled;
*/
static int HandleSplitterMouseEvent(int _s, struct event *pEvent)
{
  TSplitter *s;
  int nButtonState;

  ASSERT(_s >= 0);
  ASSERT(_s < _countof(splitters));
  ASSERT(splitters[_s].bInUse);
  ASSERT(pEvent->t.code == EVENT_MOUSE);

  s = &splitters[_s];
  if (!InsideRegion(pEvent->e.stMouseEvent.x, pEvent->e.stMouseEvent.y,
    s->x1, s->y1, s->x2, s->y2))
    return 0;

  /*
  If right-button pressed, this is beginning of a drag operation
  Grab the mouse-proc of the main event loop (main2.c)
  */
  nButtonState = pEvent->e.stMouseEvent.nButtonState;
  if ((nButtonState & MOUSE_BUTTON1) == MOUSE_BUTTON1)
  {
    /* Check that only the rightmost button has been pressed */
    if ((nButtonState & MOUSE_BUTTON1) != nButtonState)
      return 1;  /* Ignore the event */
    /* Ask the container to set min width and height */
    ContainerHandleEvent(MSG_SET_MIN_SIZE, s->pOwner, NULL);
    /* Start the shift of the splitter (e.g. resize sub containers) */
    stSplitterProcCtx.nOriginalX = pEvent->e.stMouseEvent.x;
    stSplitterProcCtx.nOriginalY = pEvent->e.stMouseEvent.y;
    stSplitterProcCtx._s = _s;
    SetMouseProc(SplitterMouseProc, (void *)&stSplitterProcCtx);
    return 1;  /* Event processed */
  }
  return 1;  /* Ignore all other mouse events for a splitter */
}

/* ************************************************************************
   Function: ViewInit
   Description:
     Initial setup of a view.
     Sets the coordinates and geometry. Sets default values for the
     remaining fields.
*/
void ViewInit(TView *pView)
{
  ASSERT(pView != NULL);

  #ifdef _DEBUG
  pView->MagicByte = VIEW_MAGIC;
  #endif

  pView->x = 0;
  pView->y = 0;
  pView->nHeight = 0;
  pView->nWidth = 0;
  pView->sViewID[0] = '\0';  /* The view is not marked to be reopened, by def. */
  pView->pContainer = NULL;  /* Still not on screen */
  pView->bOnFocus = FALSE;
  pView->bDockedView = FALSE;
}

/* ************************************************************************
   Function: ContainerInit
   Description:
     Initial setup of the fields of a container.
*/
void ContainerInit(TContainer *pCont, TContainer *pParent, int x, int y,
  int nWidth, int nHeight)
{
  ASSERT(pCont != NULL);

  #ifdef _DEBUG
  pCont->MagicByte = CONTAINER_MAGIC;
  #endif

  pCont->x = x;
  pCont->y = y;
  pCont->nWidth = nWidth;
  pCont->nHeight = nHeight;
  pCont->pSub1 = NULL;
  pCont->pSub2 = NULL;
  pCont->pParent = pParent;
  pCont->pView = NULL;
  pCont->bTaken = TRUE;
  pCont->nProportion = 0;
  pCont->bHorizontal = TRUE;
}

/* ************************************************************************
   Function: CalcCoordinates
   Description:
     Calculates coordinates (isn't it ovbious by the name, such a beauty)
   Input:
     x, y, nWidth, nHeight -- parent container
     nProportion -- in what proportion to split (0...1000)
     bHorizontal -- in what axis
   Output:
     x1, y1, nWidth1, nHeight1 -- new size for sub-container1 (top)
     x2, y2, nWidth2, nHeight2 -- new size for sub-container2 (bottom)
*/
static void CalcCoordinates(int x, int y, int nWidth, int nHeight,
  int nProportion, int bHorizontal,
  int *x1, int *y1, int *nWidth1, int *nHeight1,
  int *x2, int *y2, int *nWidth2, int *nHeight2)
{
  ASSERT(x >= 0);
  ASSERT(y >= 0);
  ASSERT(x < 1000);
  ASSERT(y < 1000);
  ASSERT(nWidth >= 0);
  ASSERT(nHeight >= 0);
  ASSERT(nWidth < 1000);
  ASSERT(nHeight < 1000);
  ASSERT(nProportion >= 1);
  ASSERT(nProportion < 1000);

  if (bHorizontal)
  {
    /*
    The existing one will be on top
    */
    *x1 = x;
    *nHeight1 = (nHeight * nProportion) / 1000;
    *y1 = y;
    *nWidth1 = nWidth;

    /*
    New container will be bellow the existing one
    */
    *x2 = x;
    *nHeight2 = nHeight - *nHeight1;
    *y2 = y + *nHeight1;
    *nWidth2 = nWidth;
  }
  else
  {
    /*
    New container will be set to be left to the existing one
    */
    *y1 = y;
    *nWidth1 = (nWidth * nProportion) / 1000;
    *x1 = x + *nWidth1;
    *nHeight1 = nHeight;

    /*
    The existing container will be right to the new one
    */
    *y2 = y;
    *nHeight2 = nHeight;
    *x2 = x;
    *nWidth2 = nWidth - *nWidth1;
  }
}

/* ************************************************************************
   Function: ContainerHandleEvent
   Description:
     Events are handled by the single view, provided that the
     container is unsplit, or send to the two descendent containers.
*/
int ContainerHandleEvent(int nEvent, TContainer *pCont, struct event *pEvent)
{
  struct event stEventResize1;
  struct event stEventResize2;
  int s_x1;
  int s_y1;
  int s_x2;
  int s_y2;

  ASSERT(VALID_PCONTAINER(pCont));

  if (nEvent == EVENT_MOUSE)
  {
    if (pCont->pView == NULL)  /* Check splitter, or two sub-containers */
    {
      if (HandleSplitterMouseEvent(pCont->nSplitter, pEvent))
        return 1;
      if (ContainerHandleEvent(pEvent->t.code, pCont->pSub1, pEvent))
        return 1;
      if (ContainerHandleEvent(pEvent->t.code, pCont->pSub2, pEvent))
        return 1;
    }
  }

  if (nEvent == MSG_SET_MIN_SIZE)
  {
    if (pCont->pView != NULL)
    {
      pCont->pView->pfnHandleEvent(nEvent, pEvent, pCont->pView);
      return 0;
    }
    /* Ask the subcontainers for min size and calculate
    the combainer min size */
    ASSERT(pCont->pSub1 != NULL);
    ASSERT(pCont->pSub2 != NULL);
    ContainerHandleEvent(nEvent, pCont->pSub1, pEvent);
    ContainerHandleEvent(nEvent, pCont->pSub2, pEvent);
    pCont->nMinHeight =
      pCont->pSub1->nMinHeight + pCont->pSub2->nMinHeight;
    pCont->nMinWidth =
      pCont->pSub1->nMinWidth + pCont->pSub2->nMinWidth;
    if (pCont->bHorizontal)
      ++pCont->nMinHeight;  /* make up for horizontal splitter */
    else
      ++pCont->nMinWidth;  /* make up for vertical splitter */
    return 0;
  }

  if (nEvent == EVENT_RESIZE)
  {
    /* Set the new container coordinates */
    pCont->x = pEvent->e.stNewSizeEvent.NewX1;
    pCont->y = pEvent->e.stNewSizeEvent.NewY1;
    pCont->nHeight = pEvent->e.stNewSizeEvent.NewHeight;
    pCont->nWidth = pEvent->e.stNewSizeEvent.NewWidth;
    if (pCont->pView != NULL)  /* reset the coordinates of the view */
      ContainerSetView(pCont, pCont->pView);
    ASSERT(pCont->x >= 0);
    ASSERT(pCont->y >= 0);
    ASSERT(pCont->x < 1000);
    ASSERT(pCont->y < 1000);
    ASSERT(pCont->nWidth >= 0);
    ASSERT(pCont->nHeight >= 0);
    ASSERT(pCont->nWidth < 1000);
    ASSERT(pCont->nHeight < 1000);
    if (pCont->pView == NULL)
      ASSERT(pCont->nProportion >= 1);
    else
      ASSERT(pCont->nProportion >= 0);
    ASSERT(pCont->nProportion < 1000);
    if (pCont->pParent == NULL)  /* Root? */
    {
      ScreenWidth = pCont->nWidth;
      ScreenHeight = pCont->nHeight;
      --pCont->nHeight;  /* set aside room for status line */
    }
  }

  if (pCont->pView != NULL)
  {
    return pCont->pView->pfnHandleEvent(nEvent, pEvent, pCont->pView);
  }
  ASSERT(pCont->pSub1 != NULL);
  ASSERT(pCont->pSub2 != NULL);

  if (nEvent == MSG_INVALIDATE_SCR)
    SplitterInvalidate(pCont->nSplitter);
  if (nEvent == MSG_UPDATE_SCR)
    SplitterDraw(pCont->nSplitter);

  /*
  Now, proceed further with EVENT_RESIZE -- distrubute the event
  for the two sub-containers
  */
  if (nEvent == EVENT_RESIZE)
  {
    CalcCoordinates(pCont->x, pCont->y, pCont->nWidth, pCont->nHeight,
      pCont->nProportion, pCont->bHorizontal,
      &stEventResize1.e.stNewSizeEvent.NewX1,
      &stEventResize1.e.stNewSizeEvent.NewY1,
      &stEventResize1.e.stNewSizeEvent.NewWidth,
      &stEventResize1.e.stNewSizeEvent.NewHeight,
      &stEventResize2.e.stNewSizeEvent.NewX1,
      &stEventResize2.e.stNewSizeEvent.NewY1,
      &stEventResize2.e.stNewSizeEvent.NewWidth,
      &stEventResize2.e.stNewSizeEvent.NewHeight);
      /*
      Prepare the coordinates for the splitter. Subtract space for
      the splitter from the coordinates of the right (or bottom) container
      */
      if (pCont->bHorizontal)
      {
        s_x1 = stEventResize2.e.stNewSizeEvent.NewX1;
        s_y1 = stEventResize2.e.stNewSizeEvent.NewY1;
        s_x2 = stEventResize2.e.stNewSizeEvent.NewX1 +
          stEventResize2.e.stNewSizeEvent.NewWidth;
        s_y2 = stEventResize2.e.stNewSizeEvent.NewY1;
        ++stEventResize2.e.stNewSizeEvent.NewY1;
        --stEventResize2.e.stNewSizeEvent.NewWidth;
      }
      else
      {
        s_x1 = stEventResize2.e.stNewSizeEvent.NewX1;
        s_y1 = stEventResize2.e.stNewSizeEvent.NewY1;
        s_x2 = stEventResize2.e.stNewSizeEvent.NewX1;
        s_y2 = stEventResize2.e.stNewSizeEvent.NewY1 +
          stEventResize2.e.stNewSizeEvent.NewHeight;
        ++stEventResize2.e.stNewSizeEvent.NewX1;
        --stEventResize2.e.stNewSizeEvent.NewHeight;
      }
      SplitterSetCoordinates(pCont->nSplitter, s_x1, s_y1, s_x2, s_y2, 0);
      ContainerHandleEvent(nEvent, pCont->pSub1, &stEventResize1);
      ContainerHandleEvent(nEvent, pCont->pSub2, &stEventResize2);
  }
  else
  {
    if (ContainerHandleEvent(nEvent, pCont->pSub1, pEvent) != 0)
      return 1;
    if (ContainerHandleEvent(nEvent, pCont->pSub2, pEvent) != 0)
      return 1;
  }
  return 0;
}

/* ************************************************************************
   Function: ContainerSplit
   Description:
     Splits a container to contain two subcontainers. Migrates
     the current view to be contained in one of the subcontainers and
     makes the new view part of the other container.
   Returns:
     TRUE container split.
     FALSE unable to split. Reason: no more available container entries, or
     the split renders the views too small.
   nNewViewSize = nTotalSize * 1000 / nProportion;
*/
TContainer *ContainerSplit(TContainer *pCont, TView *pNewView, int nProportion,
  int bHorizontal)
{
  int i;
  TContainer *pNewContainer;
  TContainer *pNewContainer2;
  int x;
  int y;
  int nWidth;
  int nHeight;
  int x2;
  int y2;
  int nWidth2;
  int nHeight2;
  int s_x1;
  int s_y1;
  int s_x2;
  int s_y2;
  struct event stEventResize;

  ASSERT(VALID_PCONTAINER(pCont));
  ASSERT(VALID_PVIEW(pNewView));

  /*
  Find unoccupied containers in stContainers
  */
  pNewContainer = NULL;
  pNewContainer2 = NULL;
  for (i = 0; i < _countof(stContainers); ++i)
  {
    if (!stContainers->bTaken)
    {
      if (pNewContainer == NULL)
        pNewContainer = &stContainers[i];
      else
      {
        pNewContainer2 = &stContainers[i];
        goto _container;
      }
    }
  }
  return NULL;

_container:
  CalcCoordinates(pCont->x, pCont->y, pCont->nWidth, pCont->nHeight,
    nProportion, bHorizontal,
    &x, &y, &nWidth, &nHeight, &x2, &y2, &nWidth2, &nHeight2);
  /*
  Prepare the coordinates for the splitter. Subtract space for
  the splitter from the coordinates of the right (or bottom) container
  */
  if (bHorizontal)
  {
    s_x1 = x2;
    s_y1 = y2;
    s_x2 = x2 + nWidth - 1;
    s_y2 = y2;
    ++y2;
    --nHeight2;
  }
  else
  {
    s_x1 = x2;
    s_y1 = y2;
    s_x2 = x2;
    s_y2 = y2 + nHeight - 1;
    ++x2;
    --nWidth2;
  }
  pCont->nSplitter = SplitterNew(pCont);
  SplitterSetCoordinates(pCont->nSplitter, s_x1, s_y1, s_x2, s_y2, 0);
  ContainerInit(pNewContainer2, pCont, x2, y2, nWidth2, nHeight2);
  ContainerSetView(pNewContainer2, pNewView);
  ContainerInit(pNewContainer, pCont, x, y, nWidth, nHeight);
  /* Move the container contents in the pNewContainer (top) */
  if (pCont->pView != NULL)  /* view */
    ContainerSetView(pNewContainer, pCont->pView);
  else  /* or subcontainers */
  {
    pNewContainer->pSub1 = pCont->pSub1;
    pNewContainer->pSub2 = pCont->pSub2;
  }
  /*
  The existing container contents should be shrinked to the new
  width x height
  */
  stEventResize.e.stNewSizeEvent.NewX1 = x;
  stEventResize.e.stNewSizeEvent.NewY1 = y;
  stEventResize.e.stNewSizeEvent.NewWidth = nWidth;
  stEventResize.e.stNewSizeEvent.NewHeight = nHeight;
  ContainerHandleEvent(EVENT_RESIZE, pNewContainer, &stEventResize);

  pCont->pView = NULL;
  pCont->pSub1 = pNewContainer;
  pCont->pSub2 = pNewContainer2;
  pCont->nProportion = nProportion;
  pCont->bHorizontal = bHorizontal;
  return pNewContainer2;
}

/* ************************************************************************
   Function: ContainerCollapse
   Description:
*/
void ContainerCollapse(TContainer *pCont)
{
  TContainer *pParent;
  TContainer *pSibling;
  struct event stEventResize;

  ASSERT(VALID_PCONTAINER(pCont));
  ASSERT(pCont->pView != NULL);  /* Only containers with views may collapse */
  ASSERT(pCont->pParent != NULL);

  pParent = pCont->pParent;
  /* Find the sibling on this level */
  pSibling = pParent->pSub1;
  if (pSibling == pCont)
    pSibling = pParent->pSub2;

  /* Resize the sibling to the full size of the parent */
  stEventResize.e.stNewSizeEvent.NewX1 = pParent->x;
  stEventResize.e.stNewSizeEvent.NewY1 = pParent->y;
  stEventResize.e.stNewSizeEvent.NewWidth = pParent->nWidth;
  stEventResize.e.stNewSizeEvent.NewHeight = pParent->nHeight;
  ContainerHandleEvent(EVENT_RESIZE, pSibling, &stEventResize);

  /*
  Copy sibling contents (subcontainers or a view) to the parent
  */
  if (pSibling->pView != NULL)
  {
    pParent->pSub1 = NULL;
    pParent->pSub2 = NULL;
    ContainerSetView(pParent, pSibling->pView);
  }
  else
  {
    pParent->pSub1 = pSibling->pSub1;
    pParent->pSub2 = pSibling->pSub2;
  }

  /* Dispose the 2 containers */
  pCont->bTaken = FALSE;
  ContainerRemoveView(pCont);
  pSibling->bTaken = FALSE;
  SplitterDispose(pCont->nSplitter);

  /* Set the focus on the sibling that is now in the parent container */
  ContainerSetFocus(&stRootContainer, pParent);
}

/* ************************************************************************
   Function: ContainerWalkTree
   Description:
*/
int ContainerWalkTree(TContainer *pCont,
  int (*pfnAction)(TContainer *pCont, void *pCtx), void *pCtx)
{
  ASSERT(VALID_PCONTAINER(pCont));
  ASSERT(pfnAction != NULL);

  /* This condition is true only when we are exiting */
  if (pCont->pView == NULL && pCont->pSub1 == NULL && pCont->pSub2 == NULL)
  {
    extern BOOLEAN bQuit;
    ASSERT(bQuit == TRUE);
    return 1;
  }

  if (pfnAction(pCont, pCtx) == 0)
    return 0;  /* End of action */

  if (pCont->pView != NULL)
  {
    ASSERT(pCont->pSub1 == NULL);
    ASSERT(pCont->pSub2 == NULL);
    return 1;  /* No sub-container, keep the walk of the tree */
  }
  ASSERT(pCont->pSub1 != NULL);
  ASSERT(pCont->pSub2 != NULL);
  if (ContainerWalkTree(pCont->pSub1, pfnAction, pCtx) == 0)
    return 0;  /* Premature exit of the recursion */
  if (ContainerWalkTree(pCont->pSub2, pfnAction, pCtx) == 0)
    return 0;  /* Premature exit of the recursion */
  return 1;  /* Keep the walk of the tree */
}

/* ************************************************************************
   Function: KillFocus
   Description:
*/
static int KillFocus(TContainer *pCont, void *pCtx)
{
  if (pCont->pView != NULL)
    if (pCont->pView->bOnFocus)
      ContainerHandleEvent(MSG_KILL_FOCUS, pCont, NULL);
  return 1;
}

/* ************************************************************************
   Function: ContainerSetFocus
   Description:
*/
void ContainerSetFocus(TContainer *pRoot, TContainer *pCont)
{
  ASSERT(VALID_PCONTAINER(pRoot));
  ASSERT(VALID_PCONTAINER(pCont));

  /* Find any container that has bOnFocus set */
  ContainerWalkTree(pRoot, KillFocus, NULL);
  ContainerHandleEvent(MSG_SET_FOCUS, pCont, NULL);
  pCurrentContainer = pCont;
}

/* ************************************************************************
   Function: ContainerSetView
   Description:
*/
void ContainerSetView(TContainer *pCont, TView *pView)
{
  ASSERT(VALID_PCONTAINER(pCont));
  ASSERT(VALID_PVIEW(pView));

  ASSERT(pCont->pSub1 == NULL);
  ASSERT(pCont->pSub2 == NULL);

  pCont->pView = pView;
  pView->pContainer = pCont;
  pView->x = pCont->x;
  pView->y = pCont->y;
  pView->nHeight = pCont->nHeight;
  pView->nWidth = pCont->nWidth;
}

/* ************************************************************************
   Function: ContainerRemoveView
   Description:
*/
void ContainerRemoveView(TContainer *pCont)
{
  ASSERT(VALID_PCONTAINER(pCont));
  ASSERT(VALID_PVIEW(pCont->pView));
  KillFocus(pCont, NULL);
  pCont->pView->pContainer = NULL;  /* The View is not on the screen! */
  pCont->pView = NULL;  /* And ... we set the view on the loose */
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


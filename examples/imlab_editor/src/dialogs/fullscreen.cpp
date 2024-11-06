
#include <im.h>
#include <cd.h>
#include <cdiup.h>
#include <iup.h>

#include "imlab.h"
#include "imagewindow.h"
#include "documentlist.h"
#include "im_imageview.h"
#include "dialogs.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static imImage* fullImage = NULL;
static Ihandle *cnvFull = NULL;
static int fit_full = 0;

static int cbFullRepaint(void)
{
  int x, y, w, h, wc, hc;
  cdCanvas* fsCanvas;

  /* Activates the graphics cd_canvas */
  fsCanvas = cdCreateCanvas(CD_IUP, cnvFull);
  if (fsCanvas == NULL)
    return IUP_DEFAULT;

  /* Ocupies a full page */
  cdCanvasActivate(fsCanvas);
  cdCanvasBackground(fsCanvas, CD_BLACK);
  cdCanvasClear(fsCanvas);
  cdCanvasGetSize(fsCanvas, &wc, &hc, 0, 0);

  if (fit_full)
  {
    double rView, rImage;
    int correct;

    w = wc;
    h = hc;
    correct = 0;

    rView = ((double)hc) / wc;
    rImage = ((double)fullImage->height) / fullImage->width;

    if ((rView <= 1 && rImage <= 1) || (rView >= 1 && rImage >= 1)) /* view and image are horizontal rectangles */
    {
      if (rView > rImage)
        correct = 2;
      else
        correct = 1;
    }
    else if (rView < 1 && rImage > 1) /* view is a horizontal rectangle and image is a vertical rectangle */
      correct = 1;
    else if (rView > 1 && rImage < 1) /* view is a vertical rectangle and image is a horizontal rectangle */
      correct = 2;

    if (correct == 1)
      w = (int)(hc / rImage);
    else if (correct == 2)
      h = (int)(wc * rImage);
  }
  else
  {
    w = fullImage->width;
    h = fullImage->height;
  }

  x = (wc - w) / 2;
  y = (hc - h) / 2;

  imImageViewDrawImage(fsCanvas, fullImage, x, y, w, h);

  cdKillCanvas(fsCanvas);

  return IUP_DEFAULT;
}

static int cmFullNext(void)
{
  imlabImageDocument* document = imlabImageDocumentListNext(NULL);
  imlabImageDocument* first = document;
  while (document)
  {
    if (document->ImageFile->image == fullImage)
    {
      imlabImageDocument* next = imlabImageDocumentListNext(document);
      if (!next) next = first;
      fullImage = next->ImageFile->image;
      cbFullRepaint();
      return IUP_DEFAULT;
    }

    document = imlabImageDocumentListNext(document);
  }

  return IUP_DEFAULT;
}

static int cmFullPrevious(void)
{
  imlabImageDocument* document = imlabImageDocumentListNext(NULL);
  imlabImageDocument* previous = NULL;
  while (document)
  {
    if (document->ImageFile->image == fullImage)
    {
      if (previous)
      {
        fullImage = previous->ImageFile->image;
        cbFullRepaint();
        return IUP_DEFAULT;
      }
      // else is the last document
    }

    previous = document;
    document = imlabImageDocumentListNext(document);
  }

  if (previous)
  {
    fullImage = previous->ImageFile->image;
    cbFullRepaint();
  }

  return IUP_DEFAULT;
}

static int cmFullExit(void)
{
  return IUP_CLOSE;
}

void imlabDlgFullScreen(imImage* image, int fit)
{
  Ihandle *dlgFull;

  fit_full = fit;
  fullImage = image;

  cnvFull = IupCanvas(NULL);
  IupSetAttribute(cnvFull,"BGCOLOR","0 0 0");
  IupSetCallback(cnvFull,"ACTION",(Icallback)cbFullRepaint);
  IupSetCallback(cnvFull,"K_SP",(Icallback)cmFullNext);
  IupSetCallback(cnvFull,"K_RIGHT",(Icallback)cmFullNext);
  IupSetCallback(cnvFull,"K_BS",(Icallback)cmFullPrevious);
  IupSetCallback(cnvFull,"K_LEFT",(Icallback)cmFullPrevious);
  IupSetCallback(cnvFull,"K_ESC",(Icallback)cmFullExit);

  dlgFull = IupDialog(cnvFull);
  IupSetAttribute(dlgFull,"PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dlgFull,"FULLSCREEN","YES");

  IupPopup(dlgFull, 0, 0);

  IupDestroy(dlgFull);
  cnvFull = NULL;
  fullImage = NULL;
}

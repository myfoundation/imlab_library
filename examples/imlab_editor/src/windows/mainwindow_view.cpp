#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <cd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>

#include <cdprint.h>
#include <cdpdf.h>
#include <cdps.h>
#include <cdiup.h>
#include <cdclipbd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>
#include <cdsvg.h>

#include <iup.h>
#include <iupcontrols.h>

#include <im.h>
#include <im_image.h>
#include <im_convert.h>
#include <im_palette.h>

#include "mainwindow.h"
#include "im_imagematch.h"
#include "im_imageview.h"
#include "imagewindow.h"
#include "imagedocument.h"
#include "documentlist.h"
#include "resultswindow.h"
#include "imlab.h"
#include "dialogs.h"
#include "utl_file.h"
#include "im_clipboard.h"
#include <iup_config.h>



/*******************************/
/* View Menu call backs     */
/*******************************/

static void iwinUpdateViewMenus(imlabImageDocument *document)
{
  imImage* image = document->ImageFile->image;
  Ihandle* mnMainMenu = IupGetHandle("mnMainMenu");

  {
    int absolute = document->BitmapView.absolute;

    Ihandle* mnAbss = (Ihandle*)IupGetAttribute(mnMainMenu, "mnAbss");

    if (imImageIsSigned(image) || 
        ((image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE) && document->BitmapView.cpx2real != IM_CPX_MAG))
      IupSetAttribute(mnAbss, "ACTIVE", "YES");
    else
      IupSetAttribute(mnAbss, "ACTIVE", "NO");

    if (absolute)
      IupSetAttribute(mnAbss, "VALUE", "ON");
    else
      IupSetAttribute(mnAbss, "VALUE", "OFF");
  }
  {
    int cast_mode = document->BitmapView.cast_mode;

    Ihandle* mnModeMinMax = (Ihandle*)IupGetAttribute(mnMainMenu, "mnModeMinMax");
    Ihandle* mnModeFixed = (Ihandle*)IupGetAttribute(mnMainMenu, "mnModeFixed");
    Ihandle* mnModeDirect = (Ihandle*)IupGetAttribute(mnMainMenu, "mnModeDirect");

    IupSetAttribute(mnModeMinMax, "VALUE", "OFF");
    IupSetAttribute(mnModeFixed, "VALUE", "OFF");
    IupSetAttribute(mnModeDirect, "VALUE", "OFF");

    if (cast_mode == IM_CAST_MINMAX)
      IupSetAttribute(mnModeMinMax, "VALUE", "ON");
    else if (cast_mode == IM_CAST_FIXED)
      IupSetAttribute(mnModeFixed, "VALUE", "ON");
    else
      IupSetAttribute(mnModeDirect, "VALUE", "ON");
  }

  {
    double gamma = document->BitmapView.gamma;

    Ihandle* mnLogLite = (Ihandle*)IupGetAttribute(mnMainMenu, "mnLogLite");
    Ihandle* mnLogHeavy = (Ihandle*)IupGetAttribute(mnMainMenu, "mnLogHeavy");
    Ihandle* mnExpLite = (Ihandle*)IupGetAttribute(mnMainMenu, "mnExpLite");
    Ihandle* mnExpHeavy = (Ihandle*)IupGetAttribute(mnMainMenu, "mnExpHeavy");
    Ihandle* mnCustom = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCustom");
    Ihandle* mnLinear = (Ihandle*)IupGetAttribute(mnMainMenu, "mnLinear");

    IupSetAttribute(mnLogLite, "VALUE", "OFF");
    IupSetAttribute(mnLogHeavy, "VALUE", "OFF");
    IupSetAttribute(mnExpLite, "VALUE", "OFF");
    IupSetAttribute(mnExpHeavy, "VALUE", "OFF");
    IupSetAttribute(mnCustom, "VALUE", "OFF");
    IupSetAttribute(mnLinear, "VALUE", "OFF");

    if (imImageIsRealComplex(image))
    {
      IupSetAttribute(mnLinear, "ACTIVE", "YES");
      IupSetAttribute(mnCustom, "ACTIVE", "YES");
      IupSetAttribute(mnLogLite, "ACTIVE", "YES");
      IupSetAttribute(mnLogHeavy, "ACTIVE", "YES");
      IupSetAttribute(mnExpLite, "ACTIVE", "YES");
      IupSetAttribute(mnExpHeavy, "ACTIVE", "YES");

      if (gamma == IM_GAMMA_LINEAR)
        IupSetAttribute(mnLinear, "VALUE", "ON");
      else if (gamma == IM_GAMMA_LOGLITE)
        IupSetAttribute(mnLogLite, "VALUE", "ON");
      else if (gamma == IM_GAMMA_LOGHEAVY)
        IupSetAttribute(mnLogHeavy, "VALUE", "ON");
      else if (gamma == IM_GAMMA_EXPLITE)
        IupSetAttribute(mnExpLite, "VALUE", "ON");
      else if (gamma == IM_GAMMA_EXPHEAVY)
        IupSetAttribute(mnExpHeavy, "VALUE", "ON");
      else
        IupSetAttribute(mnCustom, "VALUE", "ON");
    }
    else
    {
      IupSetAttribute(mnLinear, "VALUE", "ON");
      IupSetAttribute(mnLinear, "ACTIVE", "YES");

      IupSetAttribute(mnLogLite, "ACTIVE", "NO");
      IupSetAttribute(mnLogHeavy, "ACTIVE", "NO");
      IupSetAttribute(mnExpLite, "ACTIVE", "NO");
      IupSetAttribute(mnExpHeavy, "ACTIVE", "NO");
      IupSetAttribute(mnCustom, "ACTIVE", "NO");
    }
  }

  {
    int cpx2real = document->BitmapView.cpx2real;

    Ihandle* mnCpxReal = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCpxReal");
    Ihandle* mnCpxImag = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCpxImag");
    Ihandle* mnCpxMag = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCpxMag");
    Ihandle* mnCpxPhase = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCpxPhase");

    IupSetAttribute(mnCpxReal, "VALUE", "OFF");
    IupSetAttribute(mnCpxImag, "VALUE", "OFF");
    IupSetAttribute(mnCpxMag, "VALUE", "OFF");
    IupSetAttribute(mnCpxPhase, "VALUE", "OFF");

    if (imImageIsComplex(image))
    {
      IupSetAttribute(mnCpxReal, "ACTIVE", "YES");
      IupSetAttribute(mnCpxImag, "ACTIVE", "YES");
      IupSetAttribute(mnCpxMag, "ACTIVE", "YES");
      IupSetAttribute(mnCpxPhase, "ACTIVE", "YES");

      switch (cpx2real)
      {
      case IM_CPX_REAL:
        IupSetAttribute(mnCpxReal, "VALUE", "ON");
        break;
      case IM_CPX_IMAG:
        IupSetAttribute(mnCpxImag, "VALUE", "ON");
        break;
      case IM_CPX_MAG:
        IupSetAttribute(mnCpxMag, "VALUE", "ON");
        break;
      case IM_CPX_PHASE:
        IupSetAttribute(mnCpxPhase, "VALUE", "ON");
        break;
      }
    }
    else
    {
      IupSetAttribute(mnCpxReal, "ACTIVE", "NO");
      IupSetAttribute(mnCpxImag, "ACTIVE", "NO");
      IupSetAttribute(mnCpxMag, "ACTIVE", "NO");
      IupSetAttribute(mnCpxPhase, "ACTIVE", "NO");
    }
  }

  {
    Ihandle* mnPseudoColor = (Ihandle*)IupGetAttribute(mnMainMenu, "mnPseudoColor");
    if (document->ImageFile->image->color_space == IM_GRAY)
      IupSetAttribute(mnPseudoColor, "ACTIVE", "YES");
    else
      IupSetAttribute(mnPseudoColor, "ACTIVE", "NO");
  }
}

static int iwinViewSetGamma(Ihandle* self, double gamma)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document->BitmapView.gamma == gamma)
    return IUP_DEFAULT;

  document->BitmapView.gamma = gamma;
  document->UpdateViews();
  return IUP_DEFAULT;
}

static int cmViewLinear(Ihandle* self)
{
  return iwinViewSetGamma(self, IM_GAMMA_LINEAR);
}

static int cmViewLogLite(Ihandle* self)
{
  return iwinViewSetGamma(self, IM_GAMMA_LOGLITE);
}

static int cmViewLogHeavy(Ihandle* self)
{
  return iwinViewSetGamma(self, IM_GAMMA_LOGHEAVY);
}

static int cmViewExpLite(Ihandle* self)
{
  return iwinViewSetGamma(self, IM_GAMMA_EXPLITE);
}

static int cmViewExpHeavy(Ihandle* self)
{
  return iwinViewSetGamma(self, IM_GAMMA_EXPHEAVY);
}

static int cmViewCustom(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  double gamma = document->BitmapView.gamma;

  if (!IupGetParam(document->DlgTitle("Custom Gamma"), NULL, NULL, "Gamma: %R\n", &gamma, NULL))
    return IUP_DEFAULT;

  return iwinViewSetGamma(self, gamma);
}

static int cmViewAbss(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->BitmapView.absolute = !document->BitmapView.absolute;
  document->UpdateViews();
  return IUP_DEFAULT;
}

static int ViewMode(Ihandle* self, int cast_mode)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document->BitmapView.cast_mode == cast_mode)
    return IUP_DEFAULT;

  document->BitmapView.cast_mode = cast_mode;
  document->UpdateViews();
  return IUP_DEFAULT;
}

static int cmModeMinMax(Ihandle* self)
{
  return ViewMode(self, IM_CAST_MINMAX);
}

static int cmModeFixed(Ihandle* self)
{
  return ViewMode(self, IM_CAST_FIXED);
}

static int cmModeDirect(Ihandle* self)
{
  return ViewMode(self, IM_CAST_DIRECT);
}

static int iwinCpxCnv(Ihandle* self, int cpx2real)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document->BitmapView.cpx2real == cpx2real)
    return IUP_DEFAULT;

  document->BitmapView.cpx2real = cpx2real;

  if (document->BitmapView.image->color_space == IM_GRAY && document->BitmapView.cpx2real == IM_CPX_PHASE)
    imImageSetPalette(document->ImageFile->image, imPaletteHues(), 256);

  document->UpdateViews();

  return IUP_DEFAULT;
}

static int cmCpxReal(Ihandle* self)
{
  return iwinCpxCnv(self, IM_CPX_REAL);
}

static int cmCpxImag(Ihandle* self)
{
  return iwinCpxCnv(self, IM_CPX_IMAG);
}

static int cmCpxMag(Ihandle* self)
{
  return iwinCpxCnv(self, IM_CPX_MAG);
}

static int cmCpxPhase(Ihandle* self)
{
  return iwinCpxCnv(self, IM_CPX_PHASE);
}

static int cmViewMenuOpen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (!document)
    IupSetAttribute(self, "ACTIVE", "NO");
  else
  {
    IupSetAttribute(self, "ACTIVE", "YES");
    iwinUpdateViewMenus(document);
  }
  return IUP_DEFAULT;
}

static int cmFullScreen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imlabDlgFullScreen(document->BitmapView.image, 0);
  return IUP_DEFAULT;
}

static int cmFitFullScreen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imlabDlgFullScreen(document->BitmapView.image, 1);
  return IUP_DEFAULT;
}

static int cmNewBitmapView(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->CreateView("Bitmap");
  return IUP_DEFAULT;
}

static int cmNewHistogramView(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->CreateView("Histogram");
  return IUP_DEFAULT;
}

static int cmNewMatrixView(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->CreateView("Matrix");
  return IUP_DEFAULT;
}

static int cmNew3DView(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->CreateView("3D");
  return IUP_DEFAULT;
}

void imlabMainWindowRegisterViewMenu(void)
{
  IupSetFunction("imlabCpxReal", (Icallback) cmCpxReal);
  IupSetFunction("imlabCpxImag", (Icallback) cmCpxImag);
  IupSetFunction("imlabCpxMag", (Icallback) cmCpxMag);
  IupSetFunction("imlabCpxPhase", (Icallback) cmCpxPhase);

  IupSetFunction("imlabLinear", (Icallback) cmViewLinear);
  IupSetFunction("imlabCustom", (Icallback) cmViewCustom);
  IupSetFunction("imlabLogLite", (Icallback) cmViewLogLite);
  IupSetFunction("imlabLogHeavy", (Icallback) cmViewLogHeavy);
  IupSetFunction("imlabExpLite", (Icallback) cmViewExpLite);
  IupSetFunction("imlabExpHeavy", (Icallback) cmViewExpHeavy);
  IupSetFunction("imlabAbss", (Icallback) cmViewAbss);

  IupSetFunction("imlabModeMinMax", (Icallback) cmModeMinMax);
  IupSetFunction("imlabModeFixed", (Icallback) cmModeFixed);
  IupSetFunction("imlabModeDirect", (Icallback) cmModeDirect);

  IupSetFunction("imlabFullScreen", (Icallback) cmFullScreen);
  IupSetFunction("imlabFitFullScreen", (Icallback) cmFitFullScreen);
  IupSetFunction("imlabNewBitmapView", (Icallback) cmNewBitmapView);
  IupSetFunction("imlabNewHistogramView", (Icallback) cmNewHistogramView);
  IupSetFunction("imlabNewMatrixView", (Icallback) cmNewMatrixView);
  IupSetFunction("imlabNew3DView", (Icallback) cmNew3DView);

  IupSetFunction("imlabViewMenuOpen", (Icallback) cmViewMenuOpen);
}

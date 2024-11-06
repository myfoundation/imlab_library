#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>


static int eqhisto(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessEqualizeHistogram(image, NewImage);

  document->ChangeImage(NewImage, "EqualizeHistogram{}");

  return IUP_DEFAULT;
}

static int exphisto_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double p = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    imProcessExpandHistogram(image, NewImage, p);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int exphisto(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double p = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Histogram Expansion"), exphisto_preview, preview,
                   "Percent: %R[0,50]\n"
                   "Preview: %b\n",
                   &p, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessExpandHistogram(image, NewImage, p);

  int low_level, high_level;
  imCalcPercentMinMax(image, p, 0, &low_level, &high_level);

  document->ChangeImage(NewImage, "ExpandHistogram{percent=%g}\n  (low_level=%d, high_level=%d)", p, low_level, high_level);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* menu;
 
  menu = IupMenu(
    imlabProcNewItem(mnProcess, "Equalization", "eqhisto", (Icallback) eqhisto, 1),
    imlabProcNewItem(mnProcess, "Expansion...", "exphisto", (Icallback) exphisto, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Histogram", menu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "eqhisto", imImageIsSciByteShortUshort);
  imlabProcPlugInUpdateItem(mnProcess, "exphisto", imImageIsSciByteShortUshort);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinHisto = &plugin;

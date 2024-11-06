#include "imagedocument.h"
#include "imlab.h"
#include "dialogs.h"
#include "plugin_process.h"

#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

typedef int (*rankFunc)(const imImage* image, imImage* NewImage, int kernel_size);

static int rank_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;
  Ihandle* kernel_param = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
  int kernel_size = IupGetInt(kernel_param, "VALUE");

  /* pressed OK - check if kernel valid */
  if (param_index == -1 && (kernel_size < 3 || kernel_size % 2 == 0))
  {
    IupMessage("Error", "Kernel Size must be odd and >= 3.");
    return 0;
  }

  int ret = imlabDlgCheckKernelParam(dialog, kernel_param, &kernel_size);
  if (param_index == 0 && ret == 0)
    return 1;  /* kernel_size can be temporarily invalid during editing, just return */

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    rankFunc rfunc = (rankFunc)IupGetFunction("_IMLAB_CONVOLVE_RANK_");

    if (!rfunc(image, NewImage, kernel_size))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (ret == -1) 
    return 0;

  if (param_readonly)
    return 0;

  return 1;
}

static int convolve_rank(Ihandle *parent, const char* name, rankFunc rfunc)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  IupSetFunction("_IMLAB_CONVOLVE_RANK_", (Icallback)rfunc);

  static int kernel_size = 3;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle(name), rank_preview, preview,
                   "Kernel Size: %i[0,]\n" 
                   "Preview: %b\n",
                   &kernel_size, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !rfunc(image, NewImage, kernel_size))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "%sConvolve{kernel_size=%d}", name, kernel_size);

  return IUP_DEFAULT;
}

static int median(Ihandle *parent)
{
  return convolve_rank(parent, "Median", imProcessMedianConvolve);
}

static int range(Ihandle *parent)
{
  return convolve_rank(parent, "Range", imProcessRangeConvolve);
}

static int rank_closest(Ihandle *parent)
{
  return convolve_rank(parent, "RankClosest", imProcessRankClosestConvolve);
}

static int local_max(Ihandle *parent)
{
  return convolve_rank(parent, "RankMax", imProcessRankMaxConvolve);
}

static int local_min(Ihandle *parent)
{
  return convolve_rank(parent, "RankMin", imProcessRankMinConvolve);
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle *submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Median...", "median", (Icallback)median, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Closest MinMax...", "rank_closest", (Icallback)rank_closest, 1),
    imlabProcNewItem(mnProcess, "Local Max...", "local_max", (Icallback)local_max, 1),
    imlabProcNewItem(mnProcess, "Local Min...", "local_min", (Icallback)local_min, 1),
    imlabProcNewItem(mnProcess, "Range (Max-Min)...", "range", (Icallback)range, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Filter (Rank)", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "median", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "range", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "local_max", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "local_min", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "rank_closest", imImageIsSciNotComplex);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinConvRank = &plugin;

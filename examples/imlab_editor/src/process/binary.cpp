#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "kernel.h"
#include "counter.h"

#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>

static int thin(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessBinMorphThin(image, NewImage))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "BinMorphThin{}");

  return IUP_DEFAULT;
}

static int perimline(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessPerimeterLine(image, NewImage))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "PerimeterLine{}");

  return IUP_DEFAULT;
}

static int fillholes(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static int connect_index = 0;

  if (!IupGetParam(document->DlgTitle("Fill Holes"), NULL, NULL,
                   "Connectivity: %l|4|8|\n",
                   &connect_index, NULL))
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int connect = (connect_index + 1) * 4;

  if (!imProcessFillHoles(image, NewImage, connect))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "FillHoles{connect=%d}", connect);

  return IUP_DEFAULT;
}

static int removebyarea_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM4", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int connect_index = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int connect = (connect_index + 1) * 4;
    int start_size = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    int end_size = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    int inside = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "VALUE");

    if (!imProcessRemoveByArea(image, NewImage, connect, start_size, end_size, inside))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int removebyarea(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  static int connect_index = 0;
  static int start_size = 10;
  static int end_size = 0;
  static int inside = 1;
  int show_preview = 0;

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  if (!IupGetParam(document->DlgTitle("Remove by Area"), removebyarea_preview, preview,
                   "Connectivity: %l|4|8|\n"
                   "Start Size: %i[0,]\n"
                   "End Size: %i[0,]{0 means no upper limits}\n"
                   "Inside Interval: %b\n"
                   "Preview: %b\n",
                   &connect_index, &start_size, &end_size, &inside, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  int connect = (connect_index + 1) * 4;

  if (!show_preview && !imProcessRemoveByArea(image, NewImage, connect, start_size, end_size, inside))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RemoveByArea{connect=%d, start_size=%d, end_size=%d, inside=%d}", connect, start_size, end_size, inside);

  return IUP_DEFAULT;
}

static int reg_max(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessRegionalMaximum(image, NewImage);

  document->ChangeImage(NewImage, "RegionalMaximum{}");

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle *submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Remove by Area...", "removebyarea", (Icallback)removebyarea, 1),
    imlabProcNewItem(mnProcess, "Fill Holes...", "fillholes", (Icallback)fillholes, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Perimeter Line...", "perimline", (Icallback)perimline, 1),
    imlabProcNewItem(mnProcess, "Thin...", "thin", (Icallback)thin, 1), 
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Regional Maximum...", "reg_max", (Icallback)reg_max, 1),
//    imlabProcNewItem(mnProcess, "Watershed...", "watershed", (Icallback)watershed, 1), 
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "thin"), "imlabStatusHelp", "Image Thinning using Neighborhood Maps from Graphics Gems IV.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "reg_max"), "imlabStatusHelp", "Find the local maximum points of the distance transform. Same as Ultimate Points.");
//  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "watershed"), "imlabStatusHelp", "Separates objects from a filtered distance transform.");

  IupAppend(mnProcess, imlabSubmenu("Binary", submenu));
 }

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "thin", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "removebyarea", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "fillholes", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "perimline", imImageIsInteger);
  imlabProcPlugInUpdateItem(mnProcess, "reg_max", imImageIsRealGray);
//  imlabProcPlugInUpdateItem(mnProcess, "watershed", imImageIsRealGray);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinBinary = &plugin;

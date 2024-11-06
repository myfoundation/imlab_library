#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "kernel.h"

#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <string.h>

static int bin_morph(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* kernel = imGetKernel(document->FileTitle, 0);
  if(!kernel)
    return IUP_DEFAULT;

  static int hit_white = 1;
  static int iter = 1;
  if (!IupGetParam(document->DlgTitle("Binary Morphology"), NULL, NULL,
                   "Iterations: %i[1,]\n"
                   "Hit is White: %b\n",
                   &iter, &hit_white, NULL))
  {
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessBinMorphConvolve(image, NewImage, kernel, hit_white, iter))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "BinMorphConvolve{kernel=\"%s\", hit_white=%d, iter=%d}", desc, hit_white, iter);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int bin_morph_loadconv(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;
  char filename[10240] = "*.krn";
  
  if (!imlabDlgSelectFile(filename, "OPEN", "Load Kernel", "Kernel Files|*.krn|All Files|*.*|", "LastKernelDirectory"))
    return IUP_DEFAULT;

  imImage* kernel = imKernelLoad(filename);
  if (!kernel)
    return IUP_DEFAULT;

  static int hit_white = 1;
  static int iter = 1;
  if (!IupGetParam(document->DlgTitle("Binary Morphology"), NULL, NULL,
                   "Iterations: %i[1,]\n"
                   "Hit is White: %b\n",
                   &iter, &hit_white, NULL))
  {
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessBinMorphConvolve(image, NewImage, kernel, hit_white, iter))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "BinMorphConvolve{kernel=\"%s\", hit_white=%d, iter=%d}", desc, hit_white, iter);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

enum{IM_BINM_ERD,IM_BINM_DLT,IM_BINM_OPEN,IM_BINM_CLOSE,IM_BINM_OUT};

static char*GetBinMorphStr(int op)
{
  switch (op)
  {
  case IM_BINM_ERD:
    return "Erode";
  case IM_BINM_DLT:
    return "Dilate";
  case IM_BINM_OPEN:
    return "Open";
  case IM_BINM_CLOSE:
    return "Close";
  case IM_BINM_OUT:
    return "Outline";
  }

  return NULL;
}

static int bin_morph_preview(Ihandle* dialog, int param_index, void* user_data)
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
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int iter = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    int op = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    int ret = 0;
    switch (op)
    {
    case IM_BINM_ERD:
      ret = imProcessBinMorphErode(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_DLT:
      ret = imProcessBinMorphDilate(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_OPEN:
      ret = imProcessBinMorphOpen(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_CLOSE:
      ret = imProcessBinMorphClose(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_OUT:
      ret = imProcessBinMorphOutline(image, NewImage, kernel_size, iter);
      break;
    }

    if (!ret)
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

static int bin_morph_op(Ihandle *parent, int op)
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

  static int kernel_size = 3;
  static int iter = 1;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Binary Morphology"), bin_morph_preview, preview,
                   "Kernel Size: %i[0,]\n"
                   "Iterations: %i[1,]\n"
                   "Operation: %l|Erode|Dilate|Open|Close|Outline|\n"
                   "Preview: %b\n",
                   &kernel_size, &iter, &op, &show_preview, NULL))
    return IUP_DEFAULT;

  int ret = 0;
  if (!show_preview)
  {
    switch (op)
    {
    case IM_BINM_ERD:
      ret = imProcessBinMorphErode(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_DLT:
      ret = imProcessBinMorphDilate(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_OPEN:
      ret = imProcessBinMorphOpen(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_CLOSE:
      ret = imProcessBinMorphClose(image, NewImage, kernel_size, iter);
      break;
    case IM_BINM_OUT:
      ret = imProcessBinMorphOutline(image, NewImage, kernel_size, iter);
      break;
    }
  }

  if (!show_preview && ret == 0)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "BinMorph%s{kernel_size=%d, iter=%d}", GetBinMorphStr(op), kernel_size, iter);

  return IUP_DEFAULT;
}

static int bin_erode(Ihandle *parent)
{
  return bin_morph_op(parent, IM_BINM_ERD);
}

static int bin_dilate(Ihandle *parent)
{
  return bin_morph_op(parent, IM_BINM_DLT);
}

static int bin_open(Ihandle *parent)
{
  return bin_morph_op(parent, IM_BINM_OPEN);
}

static int bin_close(Ihandle *parent)
{
  return bin_morph_op(parent, IM_BINM_CLOSE);
}

static int bin_out(Ihandle *parent)
{
  return bin_morph_op(parent, IM_BINM_OUT);
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle *submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Convolve...", "bin_morph", (Icallback)bin_morph, 1),
    imlabProcNewItem(mnProcess, "Load and Convolve...", "bin_morph_loadconv", (Icallback)bin_morph_loadconv, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Erode...", "bin_erode", (Icallback)bin_erode, 1),
    imlabProcNewItem(mnProcess, "Dilate...", "bin_dilate", (Icallback)bin_dilate, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Open...", "bin_open", (Icallback)bin_open, 1),
    imlabProcNewItem(mnProcess, "Close...", "bin_close", (Icallback)bin_close, 1), 
    imlabProcNewItem(mnProcess, "Outline...", "bin_out", (Icallback)bin_out, 1), 
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "bin_open"), "imlabStatusHelp", "open=erode+dilate.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "bin_close"), "imlabStatusHelp", "close=dilate+erode.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "bin_out"), "imlabStatusHelp", "outline=diff(original,erode).");

  IupAppend(mnProcess, imlabSubmenu("Morphology (Bin)", submenu));
 }

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "bin_morph", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_morph_loadconv", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_erode", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_dilate", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_open", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_close", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "bin_out", imImageIsBinary);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinMorphBin = &plugin;

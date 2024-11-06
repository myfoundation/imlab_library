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
#include <string.h>

static int gray_morph(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* kernel = imGetKernel(document->FileTitle, 0);
  if(!kernel)
    return IUP_DEFAULT;

  static int ismax = 1;
  if (!IupGetParam(document->DlgTitle("Gray Morphology"), NULL, NULL,
                   "Use Max: %b\n",
                   &ismax, NULL))
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

  if (!imProcessGrayMorphConvolve(image, NewImage, kernel, ismax))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "GrayMorphConvolve{kernel=\"%s\", ismax=%d}", desc, ismax);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int gray_morph_loadconv(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;
  char filename[10240] = "*.krn";
  
  if (!imlabDlgSelectFile(filename, "OPEN", "Load Kernel", "Kernel Files|*.krn|All Files|*.*|", "LastKernelDirectory"))
    return IUP_DEFAULT;

  imImage* kernel = imKernelLoad(filename);
  if (!kernel)
    return IUP_DEFAULT;

  static int ismax = 1;
  if (!IupGetParam(document->DlgTitle("Gray Morphology"), NULL, NULL,
                   "Use Max: %b\n",
                   &ismax, NULL))
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

  if (!imProcessGrayMorphConvolve(image, NewImage, kernel, ismax))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "GrayMorphConvolve{kernel=\"%s\", ismax=%d}", desc, ismax);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

enum{IM_GRAYM_ERD,IM_GRAYM_DLT,IM_GRAYM_OPEN,IM_GRAYM_CLOSE,IM_GRAYM_TOPH,IM_GRAYM_WELL,IM_GRAYM_GRAD};

static char*GetGrayMorphStr(int op)
{
  switch (op)
  {
  case IM_GRAYM_ERD:
    return "Erode";
  case IM_GRAYM_DLT:
    return "Dilate";
  case IM_GRAYM_OPEN:
    return "Open";
  case IM_GRAYM_CLOSE:
    return "Close";
  case IM_GRAYM_TOPH:
    return "TopHat";
  case IM_GRAYM_WELL:
    return "Well";
  case IM_GRAYM_GRAD:
    return "Gradient";
  }

  return NULL;
}

static int gray_morph_preview(Ihandle* dialog, int param_index, void* user_data)
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
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int op = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    int ret = 0;
    switch (op)
    {
    case IM_GRAYM_ERD:
      ret = imProcessGrayMorphErode(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_DLT:
      ret = imProcessGrayMorphDilate(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_OPEN:
      ret = imProcessGrayMorphOpen(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_CLOSE:
      ret = imProcessGrayMorphClose(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_TOPH:
      ret = imProcessGrayMorphTopHat(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_WELL:
      ret = imProcessGrayMorphWell(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_GRAD:
      ret = imProcessGrayMorphGradient(image, NewImage, kernel_size);
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

static int gray_morph_op(Ihandle *parent, int op)
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
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Gray Morphology"), gray_morph_preview, preview,
                   "Kernel Size: %i[0,]\n"
                   "Operation: %l|Erode|Dilate|Open|Close|Top Hat|Well|Gradient|\n"
                   "Preview: %b\n",
                   &kernel_size, &op, &show_preview, NULL))
    return IUP_DEFAULT;

  int ret = 0;
  if (!show_preview)
  {
    switch (op)
    {
    case IM_GRAYM_ERD:
      ret = imProcessGrayMorphErode(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_DLT:
      ret = imProcessGrayMorphDilate(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_OPEN:
      ret = imProcessGrayMorphOpen(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_CLOSE:
      ret = imProcessGrayMorphClose(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_TOPH:
      ret = imProcessGrayMorphTopHat(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_WELL:
      ret = imProcessGrayMorphWell(image, NewImage, kernel_size);
      break;
    case IM_GRAYM_GRAD:
      ret = imProcessGrayMorphGradient(image, NewImage, kernel_size);
      break;
    }
  }

  if (!show_preview && ret == 0)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "GrayMorph%s{kernel_size=%d}", GetGrayMorphStr(op), kernel_size);

  return IUP_DEFAULT;
}

static int gray_erode(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_ERD);
}

static int gray_dilate(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_DLT);
}

static int gray_open(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_OPEN);
}

static int gray_close(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_CLOSE);
}

static int top_hat(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_TOPH);
}

static int gmgradient(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_GRAD);
}

static int well(Ihandle *parent)
{
  return gray_morph_op(parent, IM_GRAYM_WELL);
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle *submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Convolve...", "gray_morph", (Icallback)gray_morph, 1),
    imlabProcNewItem(mnProcess, "Load and Convolve...", "gray_morph_loadconv", (Icallback)gray_morph_loadconv, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Erode...", "gray_erode", (Icallback)gray_erode, 1),
    imlabProcNewItem(mnProcess, "Dilate...", "gray_dilate", (Icallback)gray_dilate, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Open...", "gray_open", (Icallback)gray_open, 1),
    imlabProcNewItem(mnProcess, "Close...", "gray_close", (Icallback)gray_close, 1), 
    imlabProcNewItem(mnProcess, "Top Hat...", "top_hat", (Icallback)top_hat, 1), 
    imlabProcNewItem(mnProcess, "Well...", "well", (Icallback)well, 1), 
    imlabProcNewItem(mnProcess, "Gradient...", "gmgradient", (Icallback)gmgradient, 1), 
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "gray_open"), "imlabStatusHelp", "open=erode+dilate.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "gray_close"), "imlabStatusHelp", "close=dilate+erode.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "top_hat"), "imlabStatusHelp", "tophat=diff(original,open).");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "well"), "imlabStatusHelp", "well=diff(original,close).");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "gmgradient"), "imlabStatusHelp", "gradient=diff(dilate,erode).");

  IupAppend(mnProcess, imlabSubmenu("Morphology (Gray)", submenu));
 }

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "gray_morph", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gray_morph_loadconv", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gray_erode", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gray_dilate", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gray_open", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gray_close", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "top_hat", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "well", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gmgradient", imImageIsSciNotComplex);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinMorphGray = &plugin;

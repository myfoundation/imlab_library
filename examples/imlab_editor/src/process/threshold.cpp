#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_math_op.h>
#include <im_color.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>



static int GetColorTol(const char* title, int color_space, double* param, double *tol)
{
  char format[200];

  int depth = imColorModeDepth(color_space);

  if (depth == 3)
  {
    sprintf(format, "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "Tolerance: %%R\n",
            imColorModeComponentName(color_space, 0),
            imColorModeComponentName(color_space, 1),
            imColorModeComponentName(color_space, 2));

    if (!IupGetParam(title, NULL, NULL, format, param, param + 1, param + 2, tol, NULL))
      return 0;
  }
  else if (depth == 4)
  {
    sprintf(format, "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "Tolerance: %%R\n",
            imColorModeComponentName(color_space, 0),
            imColorModeComponentName(color_space, 1),
            imColorModeComponentName(color_space, 2),
            imColorModeComponentName(color_space, 3));

    if (!IupGetParam(title, NULL, NULL, format, param, param + 1, param + 2, param + 3, tol, NULL))
      return 0;
  }
  else
  {
    sprintf(format, "%s: %%R\n"
                    "Tolerance: %%R\n",
                    imColorModeComponentName(color_space, 0));

    if (!IupGetParam(title, NULL, NULL, format, param, tol, NULL))
      return 0;
  }

  return 1;
}

static int color_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static double src_const[4] = { 0, 0, 0, 0 };
  static double tol = 0;

  if (!GetColorTol(document->DlgTitle("Threshold Color"), image->color_space, src_const, &tol))
    return IUP_DEFAULT;

  imImage *NewImage = imImageCreate(image->width, image->height, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessThresholdColor(image, NewImage, src_const, tol);

  if (image->depth == 1)
    document->ChangeImage(NewImage, "ThresholdColor{src={%g}, tol=%g}", (double)src_const[0], (double)tol);
  else if (image->depth == 3)
    document->ChangeImage(NewImage, "ThresholdColor{src={%g, %g, %g}, tol=%g}", (double)src_const[0], (double)src_const[1], (double)src_const[2], (double)tol);
  else if (image->depth == 4)
    document->ChangeImage(NewImage, "ThresholdColor{src={%g, %g, %g, %g}, tol=%g}", (double)src_const[0], (double)src_const[1], (double)src_const[2], (double)src_const[3], (double)tol);

  return IUP_DEFAULT;
}

static int slice_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  double start = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
  double end = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

  if (start > end)
    return 0;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int negative = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    imProcessSliceThreshold(image, NewImage, start, end);
    if (negative) imProcessNegative(NewImage, NewImage);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int slice_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  int level_min = imColorMin(image->data_type);
  int level_max = imColorMax(image->data_type);

  char format[100];
  sprintf(format,  "Start Level: %%R[%d,%d]\n"
                   "End Level: %%R[%d,%d]\n"
                   "Negative: %%b\n"
                   "Preview: %%b\n", level_min, level_max, level_min, level_max);

  static double start = (double)level_min,
                 end = (double)level_max;
  static int negative = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Slice Threshold"), slice_preview, preview, format,
                   &start, &end, &negative, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
  {
    imProcessSliceThreshold(image, NewImage, start, end);
    if (negative) imProcessNegative(NewImage, NewImage);
  }

  document->ChangeImage(NewImage, "SliceThreshold{start=%g, end=%g}", start, end);

  return IUP_DEFAULT;
}

static int unierr_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageSetBinary(NewImage);

  int level = imProcessUniformErrThreshold(image, NewImage);

  document->ChangeImage(NewImage, "UniformErrThreshold{}");

  imlabLogMessagef("UniformErrThreshold level = %d", level);

  return IUP_DEFAULT;
}

static int diferr_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int level = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    imProcessDiffusionErrThreshold(image, NewImage, level);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int diferr_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  /* the result is not necessary a binary image */
  if (image->color_space == IM_GRAY)
    imImageSetBinary(NewImage);

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int level = 128;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Error Diffusion Dither"), diferr_preview, preview,
                   "Level: %i[0,255]\n"
                   "Preview: %b\n",
                   &level, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessDiffusionErrThreshold(image, NewImage, level);

  document->ChangeImage(NewImage, "DiffusionErrThreshold{level=%d}", level);

  return IUP_DEFAULT;
}

static int thres_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double level = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int negative = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    imProcessThreshold(image, NewImage, level, 1);
    if (negative) imProcessNegative(NewImage, NewImage);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int manual_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;
  
  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double level = -1, negative = 0;
  int show_preview = 0;

  int level_min = imColorMin(image->data_type);
  int level_max = imColorMax(image->data_type);

  if (level < level_min || level > level_max) level = (level_max + level_min) / 2;

  char format[512];
  sprintf(format, "Level: %%R[%d,%d]\n"
                  "Negative: %%b\n"
                  "Preview: %%b\n", level_min, level_max);

  if (!IupGetParam(document->DlgTitle("Manual Threshold"), thres_preview, preview,
                   format,
                   &level, &negative, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
  {
    imProcessThreshold(image, NewImage, level, 1);
    if (negative) imProcessNegative(NewImage, NewImage);
  }

  document->ChangeImage(NewImage, "Threshold{level=%d, value=1, negative=%d}", level, negative);

  return IUP_DEFAULT;
}

static int thresper_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  if (param_index == 3)  // "Returned Level" is read-only
    return 0;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double percent = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int negative = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    int level = imProcessPercentThreshold(image, NewImage, percent);
    if (negative) imProcessNegative(NewImage, NewImage);

    Ihandle* level_param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
    Ihandle* level_param_ctrl = (Ihandle*)IupGetAttribute(level_param, "CONTROL");
    IupSetInt(level_param, "VALUE", level);
    IupSetInt(level_param_ctrl, "VALUE", level);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int percent_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double percent = 50, negative = 0;
  int show_preview = 0;
  int level = 0;
  if (!IupGetParam(document->DlgTitle("Percent Threshold"), thresper_preview, preview,
    "Percent: %R[0,100]\n"
    "Negative: %b\n"
    "Preview: %b\n"
    "Returned Level: %i\n",
    &percent, &negative, &level, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
  {
    level = imProcessPercentThreshold(image, NewImage, percent);
    if (negative) imProcessNegative(NewImage, NewImage);
  }

  document->ChangeImage(NewImage, "PercentThreshold{level=%d, negative=%d}", level, negative);

  return IUP_DEFAULT;
}

static int diff_threshold(Ihandle *parent)
{
  imlabImageDocument* document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument* document2 = imlabImageDocumentListSelect("Select Second Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessThresholdByDiff(image1, image2, NewImage);
  document1->ChangeImage(NewImage, "ThresholdByDiff{image=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int otsu_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int level = imProcessOtsuThreshold(image, NewImage);

  document->ChangeImage(NewImage, "OtsuThreshold{}");

  imlabLogMessagef("OtsuThreshold level=%d", level);

  return IUP_DEFAULT;
}

static int minmax_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;
  
  imImage* NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  double level = imProcessMinMaxThreshold(image, NewImage);

  document->ChangeImage(NewImage, "MinMaxThreshold{}");

  imlabLogMessagef("MinMaxThreshold level=%g", level);

  return IUP_DEFAULT;
}

static int hystthres_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int low_thres = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
  int high_thres = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

  if (low_thres > high_thres)
    return 0;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");

    imProcessHysteresisThreshold(image, NewImage, low_thres, high_thres);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int hysteresis_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  int high_thres, low_thres;
  int show_preview = 0;
  imProcessHysteresisThresEstimate(image, &low_thres, &high_thres);
  if (!IupGetParam(document->DlgTitle("Hysteresis Threshold"), hystthres_preview, preview,
                   "Low Level: %i[0,255]\n"
                   "High Level: %i[0,255]\n"
                   "Preview: %b\n",
                   &low_thres, &high_thres, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessHysteresisThreshold(image, NewImage, low_thres, high_thres);

  document->ChangeImage(NewImage, "HysteresisThreshold{low=%d, high=%d}", low_thres, high_thres);

  return IUP_DEFAULT;
}

static int localmax_preview(Ihandle* dialog, int param_index, void* user_data)
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
    int min_thres = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessLocalMaxThreshold(image, NewImage, kernel_size, min_thres))
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

static int localmax_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int kernel_size = 7;
  static int min_thres = 0;
  int show_preview = 0;
  if (image->data_type == IM_BYTE)
    imProcessLocalMaxThresEstimate(image, &min_thres);
  if (!IupGetParam(document->DlgTitle("Local Max Threshold"), localmax_preview, preview,
                   "Kernel Size: %i[0,]\n"
                   "Minimum Level: %i[0,]\n"
                   "Preview: %b\n",
                   &kernel_size, &min_thres, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessLocalMaxThreshold(image, NewImage, kernel_size, min_thres))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "LocalMaxThreshold{kernel_size=%d, min_thres=%d}", kernel_size, min_thres);

  return IUP_DEFAULT;
}

static int localcontrast_preview(Ihandle* dialog, int param_index, void* user_data)
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
  if (param_index == 0 && ret==0)
    return 1;  /* kernel_size can be temporarily invalid during editing, just return */

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int cmin = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRangeContrastThreshold(image, NewImage, kernel_size, cmin))
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

static int localcontrast_threshold(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_BINARY, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int kernel_size = 7;
  static int cmin = 32;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Range Contrast Threshold"), localcontrast_preview, preview,
                   "Kernel Size: %i[0,]\n"
                   "Minimum Range: %i[0,]\n"
                   "Preview: %b\n",
                   &kernel_size, &cmin, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRangeContrastThreshold(image, NewImage, kernel_size, cmin))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RangeContrastThreshold{kernel_size=%d, cmin=%d}", kernel_size, cmin);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;
   
  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Manual...", "manual_threshold", (Icallback) manual_threshold, 1),
    imlabProcNewItem(mnProcess, "Percent...", "percent_threshold", (Icallback) percent_threshold, 1),
    imlabProcNewItem(mnProcess, "Slice...", "slice_threshold", (Icallback) slice_threshold, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "MinMax", "minmax_threshold", (Icallback) minmax_threshold, 1),
    imlabProcNewItem(mnProcess, "Otsu", "otsu_threshold", (Icallback) otsu_threshold, 1),
    imlabProcNewItem(mnProcess, "Uniform Error", "unierr_threshold", (Icallback) unierr_threshold, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Diffusion Error...", "diferr_threshold", (Icallback) diferr_threshold, 1),
    imlabProcNewItem(mnProcess, "Range Contrast...", "localcontrast_threshold", (Icallback) localcontrast_threshold, 1),
    imlabProcNewItem(mnProcess, "Local Max...", "localmax_threshold", (Icallback) localmax_threshold, 1),
    imlabProcNewItem(mnProcess, "Hysteresis...", "hysteresis_threshold", (Icallback) hysteresis_threshold, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "By Difference...", "diff_threshold", (Icallback) diff_threshold, 1),
    imlabProcNewItem(mnProcess, "By Color...", "color_threshold", (Icallback)color_threshold, 1),
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "minmax_threshold"), "imlabStatusHelp", "Threshold level is (max-min)/2.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "unierr_threshold"), "imlabStatusHelp", "Uniform error thresholding by local estimation. Adapted from XITE.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "localcontrast_threshold"), "imlabStatusHelp", "Local variable threshold by the method of Bernsen. Adapted from XITE.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "localmax_threshold"), "imlabStatusHelp", "Threshold if greater than local max and greater than given level.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "diff_threshold"), "imlabStatusHelp", "Threshold if difference greater than zero (i1 > i2).");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "color_threshold"), "imlabStatusHelp", "Threshold if color difference greater than tolerance.");

  IupAppend(mnProcess, imlabSubmenu("Threshold", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "diferr_threshold", imImageIsSciByte);
  imlabProcPlugInUpdateItem(mnProcess, "color_threshold", imImageIsSciByte);

  imlabProcPlugInUpdateItem(mnProcess, "slice_threshold", imImageIsGray);
  imlabProcPlugInUpdateItem(mnProcess, "minmax_threshold", imImageIsGray);
  imlabProcPlugInUpdateItem(mnProcess, "manual_threshold", imImageIsGray);
  imlabProcPlugInUpdateItem(mnProcess, "diff_threshold", imImageIsGray);
  imlabProcPlugInUpdateItem(mnProcess, "hysteresis_threshold", imImageIsGray);

  imlabProcPlugInUpdateItem(mnProcess, "unierr_threshold", imImageIsByteGray);

  imlabProcPlugInUpdateItem(mnProcess, "otsu_threshold", imImageIsGrayInteger);
  imlabProcPlugInUpdateItem(mnProcess, "percent_threshold", imImageIsGrayInteger);
  imlabProcPlugInUpdateItem(mnProcess, "localcontrast_threshold", imImageIsGrayInteger);
  imlabProcPlugInUpdateItem(mnProcess, "localmax_threshold", imImageIsGrayInteger);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinThres = &plugin;

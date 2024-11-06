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


static int normdiffratio(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  int image2_index;
  char format[4096] = "Second Image: %l";
  if (!imlabImageDocumentListInitFormat(format + strlen(format), NULL, image1, &image2_index))
    return IUP_DEFAULT;

  if (!IupGetParam(document1->DlgTitle("Normalized Difference Ratio"), NULL, NULL, format, &image2_index, NULL))
    return IUP_DEFAULT;

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
  imImage *image2 = document2->ImageFile->image;

  int data_type = IM_FLOAT;
  if (image1->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
      data_type = IM_DOUBLE;
  }
  else if (image1->data_type == IM_DOUBLE)
    data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessNormDiffRatio(image1, image2, NewImage);

  document1->ChangeImage(NewImage, "NormDiffRatio{image=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int abhypercorr_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    imImage *image_abnormal = (imImage*)IupGetAttribute(preview, "AbnormalImage");
    int threshold_consecutive = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int threshold_percent = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    imImageClear(image_abnormal);
    imProcessAbnormalHyperionCorrection(image, NewImage, threshold_consecutive, threshold_percent, image_abnormal);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int abhypercorr(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImage* image_abnormal = imImageCreateBased(image, 0, 0, IM_BINARY, IM_BYTE);
  if (!image_abnormal)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  IupSetAttribute(preview, "AbnormalImage", (char*)image_abnormal);

  static int threshold_consecutive = 0;
  static int threshold_percent = 50;
  static int show_abnormal = 0;
  int show_preview = 0;
  char format[4096] = "Threshold Consecutive: %i\n"
                      "Threshold Percent: %i\n"
                      "Preview: %b\n"
                      "Show Abnormal: %b\n";

  if (!IupGetParam(document->DlgTitle("Abnormal Hyperion Correction"), abhypercorr_preview, preview, format,
                   &threshold_consecutive, &threshold_percent, &show_abnormal, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
  {
    imImageClear(image_abnormal);
    imProcessAbnormalHyperionCorrection(image, NewImage, threshold_consecutive, threshold_percent, image_abnormal);
  }

  document->ChangeImage(NewImage, "AbnormalHyperionCorrection{threshold_consecutive=%d, threshold_percent=%d}", threshold_consecutive, threshold_percent);

  if (show_abnormal)
    imlabImageDocumentCreateFromImage(image_abnormal, "AbnormalPixels of %s", "AbnormalPixels{image=\"%s\"}", document->FileTitle);
  else
    imImageDestroy(image_abnormal);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* menu;
 
  menu = IupMenu(
    imlabProcNewItem(mnProcess, "Normalized Difference Ratio...", "normdiffratio", (Icallback)normdiffratio, 1),
    imlabProcNewItem(mnProcess, "Abnormal Hyperion Correction...", "abhypercorr", (Icallback) abhypercorr, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Remote Sensing", menu));

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "normdiffratio"), "imlabStatusHelp", "(a-b)/(a+b)");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "abhypercorr"), "imlabStatusHelp", "Detection and Correction of Abnormal Pixels in Hyperion Images.");
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "normdiffratio", imImageIsGrayNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "abhypercorr", imImageIsGrayNotComplex);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinRemoteSens = &plugin;

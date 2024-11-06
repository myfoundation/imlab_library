#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "counter.h"
#include "dialogs.h"

#include <iupcontrols.h>
#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

static int GetConst(const char* title, int color_space, double* src_param, double* dst_param)
{
  char format[200];

  int depth = imColorModeDepth(color_space);

  if (depth == 3)
  {
    sprintf(format, "Source %%t\n"
                   "%s: %%R\n" 
                   "%s: %%R\n"
                   "%s: %%R\n"
                   "Target %%t\n"
                   "%s: %%R\n"
                   "%s: %%R\n"
                   "%s: %%R\n",
                   imColorModeComponentName(color_space, 0),
                   imColorModeComponentName(color_space, 1),
                   imColorModeComponentName(color_space, 2),
                   imColorModeComponentName(color_space, 0), 
                   imColorModeComponentName(color_space, 1), 
                   imColorModeComponentName(color_space, 2));

    if (!IupGetParam(title, NULL, NULL, format, 
                     src_param, src_param+1, src_param+2, 
                     dst_param, dst_param+1, dst_param+2, NULL))
      return 0;
  }
  else if (depth == 4)
  {
    sprintf(format, "Source %%t\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "Target %%t\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n"
            "%s: %%R\n",
            imColorModeComponentName(color_space, 0),
            imColorModeComponentName(color_space, 1),
            imColorModeComponentName(color_space, 2),
            imColorModeComponentName(color_space, 3),
            imColorModeComponentName(color_space, 0),
            imColorModeComponentName(color_space, 1),
            imColorModeComponentName(color_space, 2),
            imColorModeComponentName(color_space, 3));

    if (!IupGetParam(title, NULL, NULL, format,
                     src_param, src_param+1, src_param+2, src_param+3, 
                     dst_param, dst_param+1, dst_param+2, dst_param+3, NULL))
      return 0;
  }
  else
  {
    sprintf(format, "Source %%t\n"
            "%s: %%R\n"
            "Target %%t\n"
            "%s: %%R\n",
            imColorModeComponentName(color_space, 0),
            imColorModeComponentName(color_space, 0));

    if (!IupGetParam(title, NULL, NULL, format, src_param, dst_param, NULL))
      return 0;
  }

  return 1;
}


static int replace_color(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  static double src_const[4] = {0, 0, 0, 0};
  static double dst_const[4] = {0, 0, 0, 0};

  if (!GetConst(document->DlgTitle("Replace Color"), image->color_space, src_const, dst_const))
    return IUP_DEFAULT;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessReplaceColor(image, NewImage, src_const, dst_const);

  if (image->depth == 1)
    document->ChangeImage(NewImage, "ReplaceColor{src={%g}, dst={%g}}", src_const[0], dst_const[0]);
  else if (image->depth == 3)
    document->ChangeImage(NewImage, "ReplaceColor{src={%g, %g, %g}, dst={%g, %g, %g}}", src_const[0], src_const[1], src_const[2], dst_const[0], dst_const[1], dst_const[2]);
  else if (image->depth == 4)
    document->ChangeImage(NewImage, "ReplaceColor{src={%g, %g, %g, %g}, dst={%g, %g, %g, %g}}", src_const[0], src_const[1], src_const[2], src_const[3], dst_const[0], dst_const[1], dst_const[2], dst_const[3]);

  return IUP_DEFAULT;
}

static int replace_rgb(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  char format[200];
  strcpy(format, "Source %t\n"
          "RGB: %c\n"
          "Target %t\n"
          "RGB: %c\n");
  static char src_param[30] = "0 0 0";
  static char dst_param[30] = "0 0 0";

  if (!IupGetParam(document->DlgTitle("Replace RGB"), NULL, NULL, format, src_param, dst_param, NULL))
    return 0;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int src_rgb[3];
  sscanf(src_param, "%d %d %d", &(src_rgb[0]), &(src_rgb[1]), &(src_rgb[2]));

  int dst_rgb[3];
  sscanf(dst_param, "%d %d %d", &(dst_rgb[0]), &(dst_rgb[1]), &(dst_rgb[2]));

  double src_color[3];
  src_color[0] = (double)src_rgb[0];
  src_color[1] = (double)src_rgb[1];
  src_color[2] = (double)src_rgb[2];

  double dst_color[3];
  dst_color[0] = (double)dst_rgb[0];
  dst_color[1] = (double)dst_rgb[1];
  dst_color[2] = (double)dst_rgb[2];

  imProcessReplaceColor(image, NewImage, src_color, dst_color);

  document->ChangeImage(NewImage, "ReplaceColor{src={%d, %d, %d}, dst={%d, %d, %d}}", src_rgb[0], src_rgb[1], src_rgb[2], dst_rgb[0], dst_rgb[1], dst_rgb[2]);

  return IUP_DEFAULT;
}

static int posterize_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int level = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    imProcessPosterize(image, NewImage, level);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int posterize(Ihandle* parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int level = 4;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Posterize"), posterize_preview, preview,
                   "Level: %i[1,7]\n"
                   "Preview: %b\n", 
                   &level, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
	  return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessPosterize(image, NewImage, level);

  document->ChangeImage(NewImage, "Posterize{level=%d}", level);

  return IUP_DEFAULT;
}

static int pixelate_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int box = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    imProcessPixelate(image, NewImage, box);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int pixelate(Ihandle* parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int box = 8;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Pixelate"), pixelate_preview, preview,
                   "Box Size: %i[0,]\n"
                   "Preview: %b\n", 
                   &box, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
	  return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessPixelate(image, NewImage, box);

  document->ChangeImage(NewImage, "Pixelate{box=%d}", box);

  return IUP_DEFAULT;
}

static int solarize_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double percent = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    imProcessToneGamut(image, NewImage, IM_GAMUT_SOLARIZE, &percent);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int solarize(Ihandle *parent)
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

  static double percent = 20;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Solarize"), solarize_preview, preview,
                   "Percent: %R[0,100]\n"
                   "Preview: %b\n", 
                   &percent, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
	  return IUP_DEFAULT;
  }

  ctTimerStart();

  if (!show_preview)
    imProcessToneGamut(image, NewImage, IM_GAMUT_SOLARIZE, &percent);

  imlabLogMessagef("Solarize Time = [%s]", ctTimerCount());

  document->ChangeImage(NewImage, "Solarize{percent=%g}", percent);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Pixelate...", "pixelate", (Icallback) pixelate, 1),
    imlabProcNewItem(mnProcess, "Posterize...", "posterize", (Icallback) posterize, 1),
    imlabProcNewItem(mnProcess, "Solarize...", "solarize", (Icallback) solarize, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Replace RGB...", "replace_rgb", (Icallback) replace_rgb, 1),
    imlabProcNewItem(mnProcess, "Replace Color...", "replace_color", (Icallback) replace_color, 0),
    IupSeparator(),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Effects", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "pixelate", imImageIsSciByte);
  imlabProcPlugInUpdateItem(mnProcess, "posterize", imImageIsByte);
  imlabProcPlugInUpdateItem(mnProcess, "solarize", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "replace_rgb", imImageIsByteRGB);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinEffects = &plugin;

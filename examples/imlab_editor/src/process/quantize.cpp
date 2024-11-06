#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_convert.h>

#include <stdlib.h>
#include <memory.h>
#include <string.h>


static int mediancut_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int palette_count = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    NewImage->palette_count = palette_count;

    imProcessQuantizeRGBMedianCut(image, NewImage);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int RGB2Map(Ihandle* parent, int dither)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  /* creates the new image */
  imImage* NewImage = imImageCreateBased(image, -1, -1, IM_MAP, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (dither < 0)
  {
    Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

    static int palette_count = 256;
    int show_preview = 0;
    if (!IupGetParam(document->DlgTitle("Median Cut Quantize"), mediancut_preview, preview,
                    "Number of Colors: %i[2,256]\n"
                    "Preview: %b\n",
                    &palette_count, &show_preview, NULL))
    {
      imImageDestroy(NewImage);
      return IUP_DEFAULT;
    }

    NewImage->palette_count = palette_count;
    if (!show_preview)
      imProcessQuantizeRGBMedianCut(image, NewImage);

    document->ChangeImage(NewImage, "ConvertRGB2Map{}");
  }
  else
  {
    imProcessQuantizeRGBUniform(image, NewImage, dither);
    document->ChangeImage(NewImage, "QuantizeRGBUniform{dither=%d}", dither);
  }

  return IUP_DEFAULT;
}

static int map_uniform(Ihandle* parent)
{
  return RGB2Map(parent, 0);
}
  
static int map_uniform_dither(Ihandle* parent)
{
  return RGB2Map(parent, 1);
}

static int quant_median_cut(Ihandle* parent)
{
  return RGB2Map(parent, -1);
}

static int grayquant_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int grays = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int uniform = IupGetInt(preview, "_UNIFORM");

    if (uniform)
      imProcessQuantizeGrayUniform(image, NewImage, grays);
    else
      imProcessQuantizeGrayMedianCut(image, NewImage, grays);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int QuantizeGray(Ihandle* parent, int uniform)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  /* creates the new image */
  imImage* NewImage = imImageCreateBased(image, -1, -1, IM_GRAY, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  IupSetfAttribute(preview, "_UNIFORM", "%d", uniform);

  static int grays = 64;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Reduce Grays"), grayquant_preview, preview,
                  "Number of Grays: %i[2,256]\n"
                  "Preview: %b\n",
                  &grays, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (uniform)
  {
    if (!show_preview)
      imProcessQuantizeGrayUniform(image, NewImage, grays);
    document->ChangeImage(NewImage, "QuantizeGrayUniform{grays=%d}", grays);
  }
  else
  {
    if (!show_preview)
      imProcessQuantizeGrayMedianCut(image, NewImage, grays);
    document->ChangeImage(NewImage, "QuantizeGrayMedianCut{grays=%d}", grays);
  }

  return IUP_DEFAULT;
}

static int gray_uniform(Ihandle* parent)
{
  return QuantizeGray(parent, 1);
}

static int gray_median_cut(Ihandle* parent)
{
  return QuantizeGray(parent, 0);
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Reduce Grays... (Uniform)", "gray_uniform", (Icallback) gray_uniform, 1),
    imlabProcNewItem(mnProcess, "Reduce Grays... (Median Cut)", "gray_median_cut", (Icallback) gray_median_cut, 1),
    imlabProcNewItem(mnProcess, "RGB to 216 Colors (Uniform)", "map_uniform", (Icallback) map_uniform, 1),
    imlabProcNewItem(mnProcess, "RGB to 216 Colors (Uniform+Dither)", "map_uniform_dither", (Icallback) map_uniform_dither, 1),
    imlabProcNewItem(mnProcess, "RGB to <=256 Colors... (Median Cut)", "quant_median_cut", (Icallback) quant_median_cut, 1),
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "quant_median_cut"), "imlabStatusHelp", "Adapted from jpeglib's \"jquant2.c\" used in the XV app.");

  IupAppend(mnProcess, imlabSubmenu("Quantize", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "map_uniform", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "map_uniform_dither", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "gray_uniform", imImageIsByteGray);
  imlabProcPlugInUpdateItem(mnProcess, "gray_median_cut", imImageIsByteGray);
  imlabProcPlugInUpdateItem(mnProcess, "quant_median_cut", imImageIsByteRGB);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinQuant = &plugin;

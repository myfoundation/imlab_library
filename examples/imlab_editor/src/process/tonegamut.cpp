
#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "counter.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_convert.h>
#include <im_math.h>
#include <im_color.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>


static int unnormalize(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageCreateBased(image, -1, -1, -1, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessUnNormalize(image, NewImage);

  imlabLogMessagef("UnNormalize Time = [%s]", ctTimerCount());

  document->ChangeImage(NewImage, "UnNormalize{}");

  return IUP_DEFAULT;
}

static int directconv(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageCreateBased(image, -1, -1, -1, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessDirectConv(image, NewImage);

  imlabLogMessagef("Direct Conversion Time = [%s]", ctTimerCount());

  document->ChangeImage(NewImage, "DirectConversion{}");

  return IUP_DEFAULT;
}

static char* GetGamutOpStr(int op)
{
  switch(op)
  {
  case IM_GAMUT_NORMALIZE:
	  return "Normalize to [0.0-1.0]";
  case IM_GAMUT_POW:
	  return "Normalized Power";
  case IM_GAMUT_LOG:
	  return "Normalized Log";
  case IM_GAMUT_EXP:
	  return "Normalized Exp";
  case IM_GAMUT_ZEROSTART:
    return "Zero Start";
  case IM_GAMUT_SLICE:
	  return "Slice";
  case IM_GAMUT_EXPAND:
	  return "Expand";
  case IM_GAMUT_CROP:
	  return "Tone Crop";
  case IM_GAMUT_BRIGHTCONT:
	  return "Brightness And Contrast";
  }

  return NULL;
}

static int gamut_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;
  char* param_preview = "PARAM2";
  int param_count = IupGetInt(preview, "GAMUT_PCOUNT");
  if (param_count == 1)
    param_preview = "PARAM1";

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, param_preview, preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int op = IupGetInt(preview, "GAMUT_OP");

    double params[3];
    params[0] = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    params[1] = (param_count == 1) ? 0 : IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    params[2] = 0;

    imProcessToneGamut(image, NewImage, op, params);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int gamut_op(Ihandle *parent, int op, double *param, int param_count, const char* format)
{
  imImage* NewImage;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  if (op == IM_GAMUT_NORMALIZE && (image->data_type != IM_FLOAT && image->data_type != IM_DOUBLE))
    NewImage = imImageCreateBased(image, -1, -1, -1, IM_FLOAT);
  else
    NewImage = imImageClone(image);

  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int show_preview = 0;

  if (param && format)
  {
    Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
    IupSetfAttribute(preview, "GAMUT_OP", "%d", op);
    IupSetfAttribute(preview, "GAMUT_PCOUNT", "%d", param_count);

    if (param_count == 1)
    {
      if (!IupGetParam(document->DlgTitle(GetGamutOpStr(op)), gamut_preview, preview,
                      format, &param[0], &show_preview, NULL))
      {
        imImageDestroy(NewImage);
	      return IUP_DEFAULT;
      }
    }
    else /* param_count == 2 */
    {
      if (!IupGetParam(document->DlgTitle(GetGamutOpStr(op)), gamut_preview, preview,
                      format, &param[0], &param[1], &show_preview, NULL))
      {
        imImageDestroy(NewImage);
	      return IUP_DEFAULT;
      }
    }
  }

  ctTimerStart();

  if (op == IM_GAMUT_NORMALIZE && (image->data_type != IM_FLOAT && image->data_type != IM_DOUBLE))
  {
    imProcessConvertDataType(image, NewImage, 0, 0, 0, 0);
    imProcessToneGamut(NewImage, NewImage, op, param);
  }
  else
  {
    if (!show_preview)
      imProcessToneGamut(image, NewImage, op, param);
  }

  imlabLogMessagef("%s Time = [%s]", GetGamutOpStr(op), ctTimerCount());

  if (param_count == 1)
    document->ChangeImage(NewImage, "ToneGamut{op=\"%s\", param={%g}}", GetGamutOpStr(op), param[0]);
  else if (param_count == 2)
    document->ChangeImage(NewImage, "ToneGamut{op=\"%s\", param={%g, %g}}", GetGamutOpStr(op), param[0], param[1]);
  else
    document->ChangeImage(NewImage, "ToneGamut{op=\"%s\"}", GetGamutOpStr(op));

  return IUP_DEFAULT;
}

static int norm_autogamma_unop(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  double gamma = imProcessCalcAutoGamma(image);
  return gamut_op(parent, IM_GAMUT_POW, &gamma, 1, NULL);
}

static int norm_pow_unop(Ihandle *parent)
{
  static double gamma = 1.0;
  return gamut_op(parent, IM_GAMUT_POW, &gamma, 1, "Gamma: %R[0,20]\n" 
                                                   "Preview: %b\n");
}

static int norm_log_unop(Ihandle *parent)
{
  static double k = 1;
  return gamut_op(parent, IM_GAMUT_LOG, &k, 1, "K Factor: %R[0,20]\n"
                                               "Preview: %b\n");
}

static int norm_exp_unop(Ihandle *parent)
{
  static double k = 1;
  return gamut_op(parent, IM_GAMUT_EXP, &k, 1, "K Factor: %R[0,20]\n"
                                               "Preview: %b\n");
}

static int normalize(Ihandle *parent)
{
  return gamut_op(parent, IM_GAMUT_NORMALIZE, NULL, 0, NULL);
}

static int slice(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  char format[100];

  int level_min = imColorMin(image->data_type);
  int level_max = imColorMax(image->data_type);

  /* in fact it has 3 params, but here we assume param[2]=0 (no binarize) */
  static double param[3] = { level_min, level_max, 0 };

  sprintf(format, "Start: %%R[%d,%d]\n"
                  "End: %%R[%d,%d]\n"
                  "Preview: %%b\n", level_min, level_max, level_min, level_max);

  return gamut_op(parent, IM_GAMUT_SLICE, param, 2, format);
}

static int expand(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  char format[100];

  int level_min = imColorMin(image->data_type);
  int level_max = imColorMax(image->data_type);

  static double param[2] = { level_min, level_max };

  sprintf(format, "Start: %%R[%d,%d]\n"
                  "End: %%R[%d,%d]\n"
                  "Preview: %%b\n", level_min, level_max, level_min, level_max);

  return gamut_op(parent, IM_GAMUT_EXPAND, param, 2, format);
}

static int gamutcrop(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  char format[100];

  int level_min = imColorMin(image->data_type);
  int level_max = imColorMax(image->data_type);

  static double param[2] = { level_min, level_max };

  sprintf(format, "Start: %%R[%d,%d]\n"
                  "End: %%R[%d,%d]\n"
                  "Preview: %%b\n", level_min, level_max, level_min, level_max);

  return gamut_op(parent, IM_GAMUT_CROP, param, 2, format);
}

static int brightcontrast(Ihandle *parent)
{
  static double param[2] = { 0, 0 };
  return gamut_op(parent, IM_GAMUT_BRIGHTCONT, param, 2, "Brightness Shift: %R[-100,100]\n"
                                                         "Contrast Factor: %R[-100,100]\n"
                                                         "Preview: %b\n");
}

static int zerostart(Ihandle *parent)
{
  return gamut_op(parent, IM_GAMUT_ZEROSTART, NULL, 0, NULL);
}

static int hsishift_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double h_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double s_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    double i_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    imProcessShiftHSI(image, NewImage, h_shift, s_shift, i_shift);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int shift_hsi(Ihandle *parent)
{
  imImage* NewImage;
  static double h_shift = 0, s_shift = 0, i_shift = 0;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  int show_preview = 0;

  if (!IupGetParam(document->DlgTitle("HSI Shift"), hsishift_preview, preview,
                   "Hue Shift: %R[-360,360]\n"
                   "Saturation Shift: %R[-1,1]\n"
                   "Intensity Shift: %R[-1,1]\n"
                   "Preview: %b\n", 
                   &h_shift, &s_shift, &i_shift, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  ctTimerStart();

  if (!show_preview)
    imProcessShiftHSI(image, NewImage, h_shift, s_shift, i_shift);

  imlabLogMessagef("HSI Shift Time = [%s]", ctTimerCount());

  document->ChangeImage(NewImage, "ShiftHSI{}");

  return IUP_DEFAULT;
}

static int shift_component_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double c0_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double c1_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    double c2_shift = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    imProcessShiftComponent(image, NewImage, c0_shift, c1_shift, c2_shift);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int shift_component(Ihandle *parent)
{
  imImage* NewImage;
  static double c0_shift = 0, c1_shift = 0, c2_shift = 0;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  int show_preview = 0;

  char format[100];
  sprintf(format, "%s Shift: %%R[-1,1]\n"
    "%s Shift: %%R[-1,1]\n"
    "%s Shift: %%R[-1,1]\n"
    "Preview: %%b\n", 
    imColorModeComponentName(image->color_space, 0),
    imColorModeComponentName(image->color_space, 1),
    imColorModeComponentName(image->color_space, 2));

  if (!IupGetParam(document->DlgTitle("Component Shift"), shift_component_preview, preview, 
                  format, &c0_shift, &c1_shift, &c2_shift, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  ctTimerStart();

  if (!show_preview)
    imProcessShiftComponent(image, NewImage, c0_shift, c1_shift, c2_shift);

  imlabLogMessagef("Shift Component Time = [%s]", ctTimerCount());

  document->ChangeImage(NewImage, "ShiftComponent{}");

  return IUP_DEFAULT;
}

static int negative(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessNegative(image, NewImage);

  document->ChangeImage(NewImage, "Negative{}");

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* menu, *submenu;

  menu = IupMenu(
    imlabProcNewItem(mnProcess, "Normalize (to 0-1)", "normalize", (Icallback) normalize, 1),
    imlabProcNewItem(mnProcess, "UnNormalize (to 0-255)", "unnormalize", (Icallback) unnormalize, 1),
    imlabProcNewItem(mnProcess, "Direct Conversion (to 0-255)", "directconv", (Icallback) directconv, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Power (Gamma)...", "norm_pow_op", (Icallback) norm_pow_unop, 1),
    imlabProcNewItem(mnProcess, "Auto Gamma", "norm_autogamma_unop", (Icallback)norm_autogamma_unop, 1),
    imlabProcNewItem(mnProcess, "Log...", "norm_log_op", (Icallback)norm_log_unop, 1),
		imlabProcNewItem(mnProcess, "Exp...", "norm_exp_op", (Icallback) norm_exp_unop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Zero Start", "zerostart", (Icallback) zerostart, 1),
    imlabProcNewItem(mnProcess, "Negative", "negative", (Icallback) negative, 1),
    IupSeparator(),                    
    imlabProcNewItem(mnProcess, "Crop...", "gamutcrop", (Icallback) gamutcrop, 1),
    imlabProcNewItem(mnProcess, "Slice...", "slice", (Icallback) slice, 1),
    imlabProcNewItem(mnProcess, "Expand...", "expand", (Icallback) expand, 1),
    imlabProcNewItem(mnProcess, "Brightness and Contrast...", "brightcontrast", (Icallback) brightcontrast, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Shift Component...", "shift_component", (Icallback)shift_component, 1),
    imlabProcNewItem(mnProcess, "Shift HSI...", "shift_hsi", (Icallback)shift_hsi, 1),
    NULL);

  submenu = imlabSubmenu("Tone Gamut", menu);
  IupSetAttribute(submenu, "imlabStatusHelp", "Operations that try to preserve the min-max interval in the output (the dynamic range).");

  IupAppend(mnProcess, submenu);
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "normalize", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "unnormalize", imImageIsReal);
  imlabProcPlugInUpdateItem(mnProcess, "directconv", imImageIsUShort2Real);
  imlabProcPlugInUpdateItem(mnProcess, "norm_pow_op", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "norm_autogamma_unop", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "norm_log_op", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "norm_exp_op", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "zerostart", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "slice", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "expand", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "gamutcrop", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "brightcontrast", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "shift_hsi", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "shift_component", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "negative", imImageIsNotComplex);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinGamut = &plugin;


#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <iupcontrols.h>
#include <im_process.h>
#include <im_util.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


static int addspeckle_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double percent = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    if (!imProcessRenderAddSpeckleNoise(image, NewImage, percent))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int add_specklenoise(Ihandle *parent)
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

  static double percent = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Add Speckle Noise"), addspeckle_preview, preview,
                   "Percent: %R[0,100]\n"
                   "Preview: %b\n",
                   &percent, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderAddSpeckleNoise(image, NewImage, percent))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderAddSpeckleNoise{percent=%g}", (double)percent);

  return IUP_DEFAULT;
}

static int addgaussian_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double mean = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double stddev = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderAddGaussianNoise(image, NewImage, mean, stddev))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int add_gaussiannoise(Ihandle *parent)
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

  static double mean = 0;
  static double stddev = 10;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Add Gaussian Noise"), addgaussian_preview, preview,
                   "Mean: %R[0,]\n"
                   "Standard Deviation: %R[0,]\n"
                   "Preview: %b\n",
                   &mean, &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderAddGaussianNoise(image, NewImage, mean, stddev))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderAddGaussianNoise{mean=%g, stddev=%g}", (double)mean, (double)stddev);

  return IUP_DEFAULT;
}
   
static int adduniform_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double mean = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double stddev = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderAddUniformNoise(image, NewImage, mean, stddev))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int add_uniformnoise(Ihandle *parent)
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

  static double mean = 0;
  static double stddev = 10;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Add Uniform Noise"), adduniform_preview, preview,
                   "Mean: %R[0,]\n"
                   "Standard Deviation: %R[0,]\n"
                   "Preview: %b\n",
                   &mean, &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderAddUniformNoise(image, NewImage, mean, stddev))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderAddUniformNoise{mean=%g, stddev=%g}", (double)mean, (double)stddev);

  return IUP_DEFAULT;
}
   
static int render_constant(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  static double param[4] = {0,0,0,0};
  if (!imlabDlgGetConstant(document->FileTitle, "Render Constant", image->color_space, param))
    return IUP_DEFAULT;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessRenderConstant(NewImage, param);

  int depth = imColorModeDepth(image->color_space);
  if (depth==1)
    document->ChangeImage(NewImage, "RenderConstant{c0=%g}", param[0]);
  else if (depth == 3)
    document->ChangeImage(NewImage, "RenderConstant{c0=%g, c1=%g, c2=%g}", param[0], param[1], param[2]);
  else if (depth == 4)
    document->ChangeImage(NewImage, "RenderConstant{c0=%g, c1=%g, c2=%g, c3=%g}", param[0], param[1], param[2], param[3]);

  return IUP_DEFAULT;
}

static int render_color(Ihandle *parent)
{
  static imbyte red = (imbyte)255;
  static imbyte green = (imbyte)255;
  static imbyte blue = (imbyte)255;

  if (!IupGetColor(IUP_CENTERPARENT,IUP_CENTERPARENT, &red, &green, &blue))
    return IUP_DEFAULT;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  double param[3];
  param[0] = (double)red;
  param[1] = (double)green;
  param[2] = (double)blue;
  imProcessRenderConstant(NewImage, param);

  document->ChangeImage(NewImage, "RenderConstant{r=%d, g=%d, b=%d}", red, green, blue);

  return IUP_DEFAULT;
}

static int fill_color(Ihandle *parent)
{
  char color[100], format[1024];
  static int x = 0, y = 0;
  static int red = 255;
  static int green = 255;
  static int blue = 255;
  static double tol = 10;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  sprintf(color, "%d %d %d", red, green, blue);

  sprintf(format, "X: %%i[0,%d]\n"
                  "Y: %%i[0,%d]\n"
                  "Color: %%c\n"
                  "Tolerance: %%R[0,100,1]\n",
                  image->width - 1, image->height - 1);

  if (!IupGetParam("Fill Color", NULL, NULL, format, &x, &y, color, &tol, NULL))
    return 0;

  sscanf(color, "%d %d %d", &red, &green, &blue);

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  double replace_color[3];
  replace_color[0] = (double)red;
  replace_color[1] = (double)green;
  replace_color[2] = (double)blue;
  imProcessRenderFloodFill(NewImage, x, y, replace_color, tol);

  document->ChangeImage(NewImage, "RenderFloodFill{x=%d, y=%d, color={%d, %d, %d}, tol=%g}", x, y, red, green, blue, tol);

  return IUP_DEFAULT;
}

static int fill_constant(Ihandle *parent)
{
  char format[1024];
  static int x = 0, y = 0;
  static double tol = 10;
  double replace_color[3];

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  if (image->color_space == IM_RGB)
  {
    static double red = 0;
    static double green = 0;
    static double blue = 0;

    sprintf(format, "X: %%i[0,%d]\n"
            "Y: %%i[0,%d]\n"
            "Red: %%R\n"
            "Green: %%R\n"
            "Blue: %%R\n"
            "Tolerance: %%R[0,100,1]\n",
            image->width - 1, image->height - 1);

    if (!IupGetParam("Fill Constant", NULL, NULL, format, &x, &y, &red, &green, &blue, &tol, NULL))
      return 0;

    replace_color[0] = red;
    replace_color[1] = green;
    replace_color[2] = blue;
  }
  else
  {
    static double gray = 0;

    sprintf(format, "X: %%i[0,%d]\n"
            "Y: %%i[0,%d]\n"
            "Gray: %%R\n"
            "Tolerance: %%R[0,100,1]\n",
            image->width - 1, image->height - 1);

    if (!IupGetParam("Fill Constant", NULL, NULL, format, &x, &y, &gray, &tol, NULL))
      return 0;

    replace_color[0] = gray;
  }

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessRenderFloodFill(NewImage, x, y, replace_color, tol);

  if (image->color_space == IM_RGB)
    document->ChangeImage(NewImage, "RenderFloodFill{x=%d, y=%d, color={%g, %g, %g}, tol=%g}", x, y, replace_color[0], replace_color[1], replace_color[2], tol);
  else
    document->ChangeImage(NewImage, "RenderFloodFill{x=%d, y=%d, color={%g}, tol=%g}", x, y, replace_color[0], tol);

  return IUP_DEFAULT;
}

static int render_noise(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessRenderRandomNoise(NewImage);

  document->ChangeImage(NewImage, "RandomNoise{}");

  return IUP_DEFAULT;
}

static int cosine_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    double xperiod = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double yperiod = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderCosine(NewImage, xperiod, yperiod))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_cosine(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double xperiod = 50;
  static double yperiod = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Cosine"), cosine_preview, preview,
                   "Horizontal Period: %R[0,]\n"
                   "Vertical Period: %R[0,]\n"
                   "Preview: %b\n",
                   &xperiod, &yperiod, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderCosine(NewImage, xperiod, yperiod))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderCosine{xperiod=%g, yperiod=%g}", xperiod, yperiod);

  return IUP_DEFAULT;
}

static int gaussian_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    double stddev = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    if (!imProcessRenderGaussian(NewImage, stddev))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_gaussian(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double stddev = 10;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Gaussian"), gaussian_preview, preview,
                   "Standard Deviation: %R[0,]\n"
                   "Preview: %b\n",
                   &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderGaussian(NewImage, stddev))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderGaussian{stddev=%g}", stddev);

  return IUP_DEFAULT;
}

static int lapofgaussian_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    double stddev = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    if (!imProcessRenderLapOfGaussian(NewImage, stddev))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_lapgauss(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double stddev = 10;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Laplacian of Gaussian"), lapofgaussian_preview, preview,
                   "Standard Deviation: %R[0,]\n"
                   "Preview: %b\n",
                   &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderLapOfGaussian(NewImage, stddev))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderLapOfGaussian{stddev=%g}", stddev);

  return IUP_DEFAULT;
}

static int sinc_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    double xperiod = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double yperiod = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderSinc(NewImage, xperiod, yperiod))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_sinc(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double xperiod = 50;
  static double yperiod = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Sinc"), sinc_preview, preview,
                   "Horizontal Period: %R[0,]\n"
                   "Vertical Period: %R[0,]\n"
                   "Preview: %b\n",
                   &xperiod, &yperiod, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderSinc(NewImage, xperiod, yperiod))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderSinc{xperiod=%g, yperiod=%g}", xperiod, yperiod);

  return IUP_DEFAULT;
}

static int box_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int width = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int height = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderBox(NewImage, width, height))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_box(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int width = 50;
  static int height = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Box"), box_preview, preview,
                   "Width: %i[0,]\n"
                   "Height: %i[0,]\n"
                   "Preview: %b\n",
                   &width, &height, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderBox(NewImage, width, height))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderBox{width=%d, height=%d}", width, height);

  return IUP_DEFAULT;
}

static int ramp_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int start = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int end = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    int dir = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    if (!imProcessRenderRamp(NewImage, start, end, dir))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_ramp(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int start = 50;
  static int end = 150;
  static int dir = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Ramp"), ramp_preview, preview,
                   "Start: %i\n"
                   "End: %i\n"
                   "Direction: %l|Horizontal|Vertical|\n"
                   "Preview: %b\n",
                   &start, &end, &dir, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderRamp(NewImage, start, end, dir))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderRamp{start=%d, end=%d, dir=\"%s\"}", start, end, dir ? "VERTICAL" : "HORIZONTAL");

  return IUP_DEFAULT;
}

static int tent_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int width = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int height = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderTent(NewImage, width, height))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_tent(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int width = 50;
  static int height = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Tent"), tent_preview, preview,
                   "Width: %i[0,]\n"
                   "Height: %i[0,]\n"
                   "Preview: %b\n",
                   &width, &height, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderTent(NewImage, width, height))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderTent{width=%d, height=%d}", width, height);

  return IUP_DEFAULT;
}

static int cone_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int radius = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

    if (!imProcessRenderCone(NewImage, radius))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_cone(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int radius = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Cone"), cone_preview, preview,
                   "Radius: %i[0,]\n"
                   "Preview: %b\n",
                   &radius, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderCone(NewImage, radius))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderCone{radius=%d}", radius);

  return IUP_DEFAULT;
}

static int wheel_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int int_radius = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int ext_radius = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderWheel(NewImage, int_radius, ext_radius))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_wheel(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int int_radius = 50;
  static int ext_radius = 150;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Wheel"), wheel_preview, preview,
                   "Internal Radius: %i[0,]\n"
                   "External Radius: %i[0,]\n"
                   "Preview: %b\n",
                   &int_radius, &ext_radius, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderWheel(NewImage, int_radius, ext_radius))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderWheel{in_radius=%d, ex_radius=%d}", int_radius, ext_radius);

  return IUP_DEFAULT;
}

static int grid_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int x_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int y_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderGrid(NewImage, x_space, y_space))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_grid(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int x_space = 50;
  static int y_space = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Grid"), grid_preview, preview,
                   "Horizontal Spacing: %i[0,]\n"
                   "Vertical Spacing: %i[0,]\n"
                   "Preview: %b\n",
                   &x_space, &y_space, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderGrid(NewImage, x_space, y_space))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderGrid{x_space=%d, y_space=%d}", x_space, y_space);

  return IUP_DEFAULT;
}

static int chessboard_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    int x_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int y_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessRenderChessboard(NewImage, x_space, y_space))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int render_chessboard(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int x_space = 50;
  static int y_space = 50;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Render Chessboard"), chessboard_preview, preview,
                   "Horizontal Spacing: %i[0,]\n"
                   "Vertical Spacing: %i[0,]\n"
                   "Preview: %b\n",
                   &x_space, &y_space, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessRenderChessboard(NewImage, x_space, y_space))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "RenderChessboard{x_space=%d, y_space=%d}", x_space, y_space);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* menu;

  menu = IupMenu(
    imlabProcNewItem(mnProcess, "Constant...", "render_constant", (Icallback) render_constant, 1),
    imlabProcNewItem(mnProcess, "Color...", "render_color", (Icallback) render_color, 1),
    imlabProcNewItem(mnProcess, "Fill Constant...", "fill_constant", (Icallback)fill_constant, 1),
    imlabProcNewItem(mnProcess, "Fill Color...", "fill_color", (Icallback)fill_color, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Ramp...", "render_ramp", (Icallback) render_ramp, 1),
    imlabProcNewItem(mnProcess, "Box...", "render_box", (Icallback) render_box, 1),
    imlabProcNewItem(mnProcess, "Tent...", "render_tent", (Icallback) render_tent, 1),
    imlabProcNewItem(mnProcess, "Cone...", "render_cone", (Icallback) render_cone, 1),
    imlabProcNewItem(mnProcess, "Wheel...", "render_wheel", (Icallback) render_wheel, 1),
    imlabProcNewItem(mnProcess, "Gaussian...", "render_gaussian", (Icallback) render_gaussian, 1),
    imlabProcNewItem(mnProcess, "Cosine...", "render_cosine", (Icallback) render_cosine, 1),
    imlabProcNewItem(mnProcess, "Sinc...", "render_sinc", (Icallback) render_sinc, 1),
    imlabProcNewItem(mnProcess, "Laplacian of Gaussian...", "render_lapgauss", (Icallback) render_lapgauss, 1),
    imlabProcNewItem(mnProcess, "Grid...", "render_grid", (Icallback) render_grid, 1),
    imlabProcNewItem(mnProcess, "Chessboard...", "render_chessboard", (Icallback) render_chessboard, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Random Noise", "render_noise", (Icallback) render_noise, 1),
    imlabProcNewItem(mnProcess, "Add Uniform Noise...", "add_uniformnoise", (Icallback) add_uniformnoise, 1),
    imlabProcNewItem(mnProcess, "Add Speckle Noise...", "add_specklenoise", (Icallback) add_specklenoise, 1),
    imlabProcNewItem(mnProcess, "Add Gaussian Noise...", "add_gaussiannoise", (Icallback) add_gaussiannoise, 1),
    NULL);

  IupAppend(mnProcess, IupSetAttributes(imlabSubmenu("Render", menu), "imlabStatusHelp=\"Render the function in the current image. Erase its contents, except Add Noise items.\""));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "render_color", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "fill_color", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "fill_constant", imImageIsRGB);
  imlabProcPlugInUpdateItem(mnProcess, "render_constant", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_wheel", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_box", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_ramp", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_tent", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_cone", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_gaussian", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_lapgauss", imImageIsSigned);
  imlabProcPlugInUpdateItem(mnProcess, "render_cosine", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_sinc", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_noise", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "add_uniformnoise", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "add_specklenoise", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "add_gaussiannoise", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_grid", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "render_chessboard", imImageIsSciNotComplex);
}


static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinRender = &plugin;


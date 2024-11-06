#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "counter.h"

#include <im_process.h>
#include <im_convert.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#ifdef USE_FFTW

static int fft(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = IM_CFLOAT;
  if (image->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
      data_type = IM_CDOUBLE;
  }
  else if (image->data_type == IM_DOUBLE || image->data_type == IM_CDOUBLE)
    data_type = IM_CDOUBLE;

  imImage *NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessFFT(image, NewImage);

  imlabLogMessagef("Forward FFT Time = [%s]", ctTimerCount());

  document->SetBitmapOptions(IM_CPX_MAG, IM_GAMMA_LOGHEAVY, 0, IM_CAST_MINMAX);

  document->ChangeImage(NewImage, "FFT{}");
  
  return IUP_DEFAULT;
}

static int ifft(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessIFFT(image, NewImage);

  imlabLogMessagef("Inverse FFT Time = [%s]", ctTimerCount());

  document->SetBitmapOptions(IM_CPX_MAG, IM_GAMMA_LINEAR, 0, IM_CAST_MINMAX);

  document->ChangeImage(NewImage, "IFFT{}");

  return IUP_DEFAULT;
}

static int crosscorr(Ihandle *parent)
{
  imlabImageDocument* document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument *document2 = imlabImageDocumentListSelect("Select Second Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  int data_type = IM_CFLOAT;
  if (image1->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
      data_type = IM_CDOUBLE;
  }
  else if (image1->data_type == IM_DOUBLE || image1->data_type == IM_CDOUBLE)
    data_type = IM_CDOUBLE;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessCrossCorrelation(image1, image2, NewImage);

  imlabLogMessagef("Cross Correlation Time = [%s]", ctTimerCount());

  document1->SetBitmapOptions(IM_CPX_MAG, IM_GAMMA_LINEAR, 0, IM_CAST_MINMAX);

  document1->ChangeImage(NewImage, "CrossCorrelation{image=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int autocorr(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = IM_CFLOAT;
  if (image->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
      data_type = IM_CDOUBLE;
  }
  else if (image->data_type == IM_DOUBLE || image->data_type == IM_CDOUBLE)
    data_type = IM_CDOUBLE;

  imImage *NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessAutoCorrelation(image, NewImage);

  imlabLogMessagef("Auto Correlation Time = [%s]", ctTimerCount());

  document->SetBitmapOptions(IM_CPX_MAG, IM_GAMMA_LINEAR, 0, IM_CAST_MINMAX);

  document->ChangeImage(NewImage, "Auto Correlation{}");

  return IUP_DEFAULT;
}

static int swap_quad(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int center2origin = 0;
  if (image->width % 2 || image->height % 2)
  {
    if (!IupGetParam(document->DlgTitle("Swap Quadrants"), NULL, NULL,
                "Center to Origin: %b\n",
                &center2origin, NULL))
      return IUP_DEFAULT;
  }

  imImage *NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessSwapQuadrants(NewImage, center2origin);

  document->ChangeImage(NewImage, "SwapQuadrants{center2origin=%d}", center2origin);

  return IUP_DEFAULT;
}

static int fft_raw(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static int inverse = 0;
  static int center = 0;
  static int normalize = 0;
  if (!IupGetParam(document->DlgTitle("Raw FFT"), NULL, NULL,
    "Inverse: %b\n"
    "Center: %b\n"
    "Normalization: %l|none|normal|square|\n",
    &inverse, &center, &normalize, NULL))
    return IUP_DEFAULT;

  imImage *NewImage = imImageDuplicate(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessFFTraw(NewImage, inverse, center, normalize);

  imlabLogMessagef("Raw FFT Time = [%s]", ctTimerCount());

  document->SetBitmapOptions(IM_CPX_MAG, IM_GAMMA_LINEAR, 0, IM_CAST_MINMAX);

  const char* normalize_str[] = { "none", "normal", "square" };
  document->ChangeImage(NewImage, "FFTraw{inverse=%d, center=%d, normalize=\"%s\"}", inverse, center, normalize_str[normalize]);

  return IUP_DEFAULT;
}

#endif /* USE_FFTW */

inline double sqr(double x)
{
  return x*x;
}

static int hough_lines(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int rmax = (int)(sqrt(sqr(image->width) + sqr(image->height))/2.0);
  imImage *NewImage = imImageCreate(180, 2*rmax+1, IM_GRAY, IM_INT);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  if (!imProcessHoughLines(image, NewImage))
    imImageDestroy(NewImage);
  else
    imlabImageDocumentCreateFromImage(NewImage, "HoughLines of %s", "HoughLines{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int hough_rmax;
static int ImageIsHoughFilteredImage(imImage *image)
{
  if (image->width == 180 && image->height == 2*hough_rmax+1 && imImageIsBinary(image))
    return 1;
  return 0;
}
static int ImageIsHoughImage(imImage *image)
{
  if (image->width == 180 && image->height == 2*hough_rmax+1 && imImageIsInt(image))
    return 1;
  return 0;
}

static int draw_hough_lines(Ihandle *parent)
{
  imImage *hough = NULL;
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image1 = document->ImageFile->image;

  hough_rmax = (int)(sqrt(sqr(image1->width) + sqr(image1->height))/2.0);

  imlabImageDocument* document2 = imlabImageDocumentListSelect("Select Filtered Hough Image", (imlabMatchFunc)ImageIsHoughFilteredImage, NULL);
  if (!document2)
    return IUP_DEFAULT;

  imlabImageDocument* document3 = imlabImageDocumentListSelect("Select Hough Image (Optional)", (imlabMatchFunc)ImageIsHoughImage, NULL);
  if (document3)
    hough = document3->ImageFile->image;

  imImage *image2 = document2->ImageFile->image;

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  int lines = imProcessHoughLinesDraw(image1, hough, image2, NewImage);

  imlabLogMessagef("Hough Lines: Found %d lines", lines);
  imlabLogMessagef("Draw Hough Lines Time = [%s]", ctTimerCount());

  imlabImageDocumentCreateFromImage(NewImage, "HoughLinesDraw of %s", "HoughLinesDraw{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int dist_transf(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = IM_FLOAT;
  if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
    data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreate(image->width, image->height, IM_GRAY, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  imProcessDistanceTransform(image, NewImage);

  document->ChangeImage(NewImage, "DistanceTransform{}");

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Distance Transform...", "dist_transf", (Icallback)dist_transf, 1),
#ifdef USE_FFTW
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Forward Fourier", "fft", (Icallback) fft, 1),
    imlabProcNewItem(mnProcess, "Inverse Fourier", "ifft", (Icallback) ifft, 1),
    imlabProcNewItem(mnProcess, "Swap Quadrants...", "swap_quad", (Icallback) swap_quad, 1),
    imlabProcNewItem(mnProcess, "Raw FFT...", "fft_raw", (Icallback)fft_raw, 1),
    imlabProcNewItem(mnProcess, "Cross Correlation...", "crosscorr", (Icallback)crosscorr, 1),
    imlabProcNewItem(mnProcess, "Auto Correlation", "autocorr", (Icallback)autocorr, 1),
#endif /* USE_FFTW */
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Hough Lines", "hough_lines", (Icallback) hough_lines, 1),
    imlabProcNewItem(mnProcess, "Draw Hough Lines...", "draw_hough_lines", (Icallback) draw_hough_lines, 1),
    NULL);

#ifdef USE_FFTW
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "fft"), "imlabStatusHelp", "Uses the USE_FFTW library. Moves the origin to the center of the image.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "ifft"), "imlabStatusHelp", "Uses the USE_FFTW library. Moves the origin to the bottom-left corner of the image. Complex images only.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "crosscorr"), "imlabStatusHelp", "IFFT(Conj(FFT(a))*FFT(b)).");
#endif /* USE_FFTW */
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "hough_lines"), "imlabStatusHelp", "Transforms binary image to (theta, rho) space. Adapted from XITE. Use Threshold / Local Max before drawing lines.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "draw_hough_lines"), "imlabStatusHelp", "Draws the hough lines over the binary image.");

  IupAppend(mnProcess, imlabSubmenu("Domain Transform", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
#ifdef USE_FFTW
  imlabProcPlugInUpdateItem(mnProcess, "fft", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "ifft", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "swap_quad", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "fft_raw", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "crosscorr", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "autocorr", imImageIsSci);
#endif /* USE_FFTW */
  imlabProcPlugInUpdateItem(mnProcess, "hough_lines", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "draw_hough_lines", imImageIsByteGray);
  imlabProcPlugInUpdateItem(mnProcess, "dist_transf", imImageIsBinary);
}

static imlabProcPlugIn plugin =
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinTrans = &plugin;

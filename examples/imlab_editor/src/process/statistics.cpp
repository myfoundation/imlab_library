#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_convert.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>

#include <im_counter.h>
#include <im_math_op.h>
#include <im_colorhsi.h>


static int getstr_real(char* str, int color_mode, double v1, double v2, double v3)
{
  if (color_mode == IM_RGB)
    return sprintf(str, "%7.5f  %7.5f  %7.5f", v1, v2, v3);
  else
    return sprintf(str, "%7.5f", v1);
}

static int getstr_int(char* str, int color_mode, int v1, int v2, int v3)
{
  if (color_mode == IM_RGB)
    return sprintf(str, "%3d  %3d  %3d", v1, v2, v3);
  else
    return sprintf(str, "%3d", v1);
}

static int image_stats(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  imImage* real_image = NULL;
  imStats stats[3];
  char* type = "";
  char min_str[50], max_str[50], mean_str[50], stddev_str[50], 
       zeros_str[50], pos_str[50], neg_str[50];

  memset(&stats, 0, sizeof(stats));

  if (image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE)
  {
    char* cpx2real_str[] = {" (Real)", " (Imaginary)", " (Magnitude)", " (Phase)"};
    int cpx2real = document->BitmapView.cpx2real; 
    int data_type = IM_FLOAT;
    if (image->data_type == IM_CDOUBLE) data_type = IM_DOUBLE;
    real_image = imImageCreateBased(image, -1, -1, -1, data_type);
    imProcessConvertDataType(image, real_image, cpx2real, 0, 0, 0);
    image = real_image;
    type = cpx2real_str[cpx2real];
  }

  if (!imCalcImageStatistics(image, stats))
  {
    if (real_image)
      imImageDestroy(real_image);
    return IUP_DEFAULT;
  }

  getstr_real(   min_str, image->color_space,  stats[0].min,      stats[1].min,      stats[2].min);
  getstr_real(   max_str, image->color_space,  stats[0].max,      stats[1].max,      stats[2].max);
  getstr_real(  mean_str, image->color_space,  stats[0].mean,     stats[1].mean,     stats[2].mean);
  getstr_real(stddev_str, image->color_space,  stats[0].stddev,   stats[1].stddev,   stats[2].stddev);
  getstr_int ( zeros_str, image->color_space,  stats[0].zeros,    stats[1].zeros,    stats[2].zeros);
  getstr_int (   pos_str, image->color_space,  stats[0].positive, stats[1].positive, stats[2].positive);
  getstr_int (   neg_str, image->color_space,  stats[0].negative, stats[1].negative, stats[2].negative);

  imlabDlgMessagef("Image Statistics", "Statistics of [%s]%s%s:\n"
                      "Min    = %s\n"
                      "Max    = %s\n"
                      "Mean   = %s\n"
                      "StdDev = %s\n"
                      "------------------------------\n"
                      "Negative = %s\n"
                      "Zeros    = %s\n"
                      "Positive = %s\n"
                      "------------------------------\n"
                      "Pixel Count = %d\n",
                      document->FileTitle, 
                      image->color_space == IM_RGB? " (RGB)": "",
                      type,
                      min_str,
                      max_str,
                      mean_str, 
                      stddev_str,
                      neg_str,
                      zeros_str,
                      pos_str,
                      image->count);

  if (real_image)
    imImageDestroy(real_image);

  return IUP_DEFAULT;
}

static int histo_stats(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  imStats stats[3];
  int median[3], mode[3];
  char min_str[50], max_str[50], mean_str[50], stddev_str[50], 
       mode_str[50], median_str[50], zeros_str[50], pos_str[50];

  if (!imCalcHistogramStatistics(document->BitmapView.image, stats))
      return IUP_DEFAULT;

  if (!imCalcHistoImageStatistics(document->BitmapView.image, median, mode))
    return IUP_DEFAULT;

  getstr_real(   min_str, image->color_space,  stats[0].min,      stats[1].min,      stats[2].min);
  getstr_real(   max_str, image->color_space,  stats[0].max,      stats[1].max,      stats[2].max);
  getstr_real(  mean_str, image->color_space,  stats[0].mean,     stats[1].mean,     stats[2].mean);
  getstr_real(stddev_str, image->color_space,  stats[0].stddev,   stats[1].stddev,   stats[2].stddev);
  getstr_int ( zeros_str, image->color_space,  stats[0].zeros,    stats[1].zeros,    stats[2].zeros);
  getstr_int (   pos_str, image->color_space,  stats[0].positive, stats[1].positive, stats[2].positive);
  getstr_int (  mode_str, image->color_space,   mode[0],           mode[1],           mode[2]);
  getstr_int (median_str, image->color_space, median[0],         median[1],         median[2]);

  imlabDlgMessagef("Histogram Statistics", "Histogram statistics of the bitmap view of\n  [%s]%s:\n"
                      "Min    = %s\n"
                      "Max    = %s\n"
                      "Mean   = %s\n"
                      "StdDev = %s\n"
                      "------------------------------\n"
                      "Zeros     = %s\n"
                      "Non Zeros = %s\n"
                      "------------------------------\n"
                      "Mode   = %s\n"
                      "Median = %s\n",
                      document->FileTitle, image->color_space == IM_RGB? " (RGB)": "",
                      min_str,
                      max_str,
                      mean_str, 
                      stddev_str,
                      zeros_str,
                      pos_str,
                      mode_str,
                      median_str);

  return IUP_DEFAULT;
}

static const double sqrt3 = 1.73205080757;

static inline int is_gray(int R, int G, int B, int s_tol)
{
  double v = R - (G + B) / 2.0;
  double u = (G - B) * (sqrt3 / 2.0);
  int S2 = (int)(v*v + u*u);
  if (S2 < s_tol)
    return 1;
  else
    return 0;
}

static int find_predom_color(const imImage* image, int counter, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char tol)
{
  int max_color = (1 << 24);
  unsigned int *count = (unsigned int*)calloc(sizeof(unsigned int), max_color);
  if (!count)
    return 0;

  imbyte *comp0 = (imbyte*)image->data[0];
  imbyte *comp1 = (imbyte*)image->data[1];
  imbyte *comp2 = (imbyte*)image->data[2];

  int i, index, s_tol = tol*tol*4;

  for (i = 0; i < image->count; i++)
  {
    int ri = comp0[i];
    int gi = comp1[i];
    int bi = comp2[i];

    if (!is_gray(ri, gi, bi, s_tol))
    {
      for (int rr = ri - tol; rr < ri + tol + 1; rr++)
      {
        if (rr >= 0 && rr <= 255)
        {
          for (int gg = gi - tol; gg < gi + tol + 1; gg++)
          {
            if (gg >= 0 && gg <= 255)
            {
              for (int bb = bi - tol; bb < bi + tol + 1; bb++)
              {
                if (bb >= 0 && bb <= 255)
                {
                  index = (int)rr << 16 | (int)gg << 8 | (int)bb;
                  count[index]++;
                }
              }
            }
          }
        }
      }
    }

    if (i % image->width == 0)
    {
      if (!imCounterInc(counter))
        return 0;
    }
  }

  unsigned int max_count = 0;
  int index_max = -1;
  for (index = 0; index < max_color; index++)
  {
    if (count[index] > max_count)
    {
      max_count = count[index];
      index_max = index;
    }
  }

  free(count);

  *r = (index_max >> 16) & 0xFF;
  *g = (index_max >>  8) & 0xFF;
  *b = (index_max      ) & 0xFF;

  return 1;
}

static int find_predom_hue(const imImage* image, int counter, int *hue, unsigned char tol)
{
  unsigned int *count = (unsigned int*)calloc(sizeof(unsigned int), 360);
  if (!count)
    return 0;

  imbyte *comp0 = (imbyte*)image->data[0];
  imbyte *comp1 = (imbyte*)image->data[1];
  imbyte *comp2 = (imbyte*)image->data[2];

  int i, index;

  for (i = 0; i < image->count; i++)
  {
    double hue, sat, inten;

    imColorRGB2HSIbyte(comp0[i], comp1[i], comp2[i], &hue, &sat, &inten);

    if (sat > 0.1 && inten > 0.1 && inten < 0.9 && hue > 0 && hue < 70) /* must not be near gray */
    {
      int hue_i = (int)imRound(hue);
      for (int hh = hue_i - tol; hh < hue_i + tol + 1; hh++)
      {
        index = hh;
        if (index < 0) index += 360;
        if (index >= 360) index -= 360;
        count[index]++;
      }
    }

    if (i % image->width == 0)
    {
      if (!imCounterInc(counter))
        return 0;
    }
  }

  unsigned int max_count = 0;
  int index_max = -1;
  for (index = 0; index < 360; index++)
  {
    if (count[index] > max_count)
    {
      max_count = count[index];
      index_max = index;
    }
  }

  free(count);

  *hue = index_max;

  return 1;
}

int imCalcPredomColor(const imImage* image, unsigned char *r, unsigned char *g, unsigned char *b, unsigned char tol)
{
  int ret = 0;
  int counter = imCounterBegin("CalcPredomColor");

  imCounterTotal(counter, image->height, "Calculating...");
  ret = find_predom_color(image, counter, r, g, b, tol);

  imCounterEnd(counter);
  return ret;
}

int imCalcPredomHue(const imImage* image, int *hue, unsigned char tol)
{
  int ret = 0;
  int counter = imCounterBegin("CalcPredomHue");

  imCounterTotal(counter, image->height, "Calculating...");
  ret = find_predom_hue(image, counter, hue, tol);

  imCounterEnd(counter);
  return ret;
}

static int predom_color(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  unsigned char r, g, b;
  static int tol = 4;

  if (!IupGetParam("Predominant Color", NULL, NULL, "Tolerance: %i[0-255]\n", &tol, NULL))
    return IUP_DEFAULT;

  if (imCalcPredomColor(image, &r, &g, &b, (unsigned char)tol))
    imlabDlgMessagef("Result", "Predominant Color of [%s] = [%d %d %d]", document->FileTitle, r, g, b);

  return IUP_DEFAULT;
}

static int predom_hue(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  int h;
  static int tol = 4;

  if (!IupGetParam("Predominant Hue", NULL, NULL, "Tolerance: %i[0-359]\n", &tol, NULL))
    return IUP_DEFAULT;

  if (imCalcPredomHue(image, &h, (unsigned char)tol))
    imlabDlgMessagef("Result", "Predominant Hue of [%s] = [%d]", document->FileTitle, h);

  return IUP_DEFAULT;
}

static int count_colors(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  unsigned long count;
  if (imCalcCountColors(image, &count))
    imlabDlgMessagef("Result", "Color Count of [%s] = [%d]", document->FileTitle, count);

  return IUP_DEFAULT;
}

static int rms_op( Ihandle *parent )
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument *document2 = imlabImageDocumentListSelect("Select Second Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  double rmserror;
  if (imCalcRMSError(image1, image2, &rmserror))
    imlabDlgMessagef("Result", "RMS Error = %.4f", (double)rmserror);

  return IUP_DEFAULT;
}

static int snr_op( Ihandle *parent )
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument *document2 = imlabImageDocumentListSelect("Select Noise Image", NULL, NULL);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  double snr;
  if (imCalcSNR(image1, image2, &snr))
    imlabDlgMessagef("Result", "SNR = %.4f dB", (double)snr);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* menu;
 
  menu = IupMenu(
    imlabProcNewItem(mnProcess, "Image Statistics...", "image_stats", (Icallback) image_stats, 0),
    imlabProcNewItem(mnProcess, "Histogram Statistics...", "histo_stats", (Icallback) histo_stats, 0),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Count Colors...", "count_colors", (Icallback) count_colors, 1),
    imlabProcNewItem(mnProcess, "Predominant RGB...", "predom_color", (Icallback)predom_color, 1),
    imlabProcNewItem(mnProcess, "Predominant Hue...", "predom_hue", (Icallback)predom_hue, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "RMS Error...", "rms_op", (Icallback) rms_op, 1),
    imlabProcNewItem(mnProcess, "SNR...", "snr_op", (Icallback) snr_op, 1),
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "rms_op"), "imlabStatusHelp", "Root Mean Square Error between two images.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "snr_op"), "imlabStatusHelp", "Signal Noise Ratio between image and noise.");

  IupAppend(mnProcess, imlabSubmenu("Statistics", menu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "predom_hue", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "predom_color", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "count_colors", imImageIsByte);
  imlabProcPlugInUpdateItem(mnProcess, "rms_op", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "snr_op", imImageIsSci);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinStats = &plugin;

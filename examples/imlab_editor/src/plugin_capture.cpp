
#ifdef USE_VIDEOCAPTURE

#include <iup.h>
#include <iupcontrols.h>
#include <cd.h>
#include <im.h>
#include <im_capture.h>
#include <im_process.h>

#include "imlab.h"
#include "imagedocument.h"
#include "documentlist.h"
#include "dialogs.h"
#include "imagecapture.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static int cap_image_capture(void* user_data, imImage* image, int device)
{
  static int cap_index = 1;
  imImage* NewImage = imImageDuplicate(image);
  imlabImageDocumentCreateFromImage(NewImage, "CaptureImage from %s #%d", "CaptureImage{device=\"%s\"}", imVideoCaptureDeviceDesc(device), cap_index);
  cap_index++;
  (void)user_data;
  return 0;
}

static int cap_image(Ihandle* self)
{
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), NULL, cap_image_capture, NULL, NULL);
  return IUP_DEFAULT;
}

/*************************************************************************************/

struct iCaptureCount
{
  int count, index, calc_noise; /* 0=no, 1=yes, 2=only */
  imImage *images[IMLAB_MAX_IMAGES];
};

static int nimages_capture(void* user_data, imImage* image, int device)
{
  iCaptureCount* nimages_data = (iCaptureCount*)user_data;
  if (nimages_data->index < nimages_data->count)
  {
    nimages_data->images[nimages_data->index] = imImageDuplicate(image);
    if (nimages_data->calc_noise != 2)
      imlabImageDocumentCreateFromImage(nimages_data->images[nimages_data->index], "CaptureImageN from %s #%d", "CaptureImageN{device=\"%s\"}", imVideoCaptureDeviceDesc(device), nimages_data->index);
    nimages_data->index++;
    return 1;
  }

  if (nimages_data->calc_noise)
  {
    imImage *ref_image = nimages_data->images[0];
    imImage *mean_image = imImageClone(ref_image);
    imImage *stddev_image = imImageCreateBased(ref_image, -1, -1, -1, IM_FLOAT);

    imProcessMultipleMean((const imImage**)nimages_data->images, nimages_data->count, mean_image);
    imProcessMultipleStdDev((const imImage**)nimages_data->images, nimages_data->count, mean_image, stddev_image);

    imlabImageDocumentCreateFromImage(mean_image, "MultipleMeanCapture", "MultipleMeanCapture{device=%d, count=%d}", device, nimages_data->count);
    imlabImageDocumentCreateFromImage(stddev_image, "MultipleStdDevCapture", "MultipleStdDevCapture{device=%d, count=%d}", device, nimages_data->count);

    if (nimages_data->calc_noise == 2)
    {
      int i;
      for (i = 0; i < nimages_data->count; i++)
        imImageDestroy(nimages_data->images[i]);
    }
  }

  nimages_data->index = 0;
  return 0;
}

static int cap_n_images(Ihandle* self)
{
  static iCaptureCount nimages_data = {5, 0, 1, {0,0,0,0,0,0,0,0,0,0,0,0}};

  if (!IupGetParam("Multiple Images", NULL, NULL,
                   "Number of Images: %i[1,100]\n" 
                   "Calculate Noise: %l|no|yes|only|\n", 
                   &nimages_data.count, &nimages_data.calc_noise, NULL))
    return 0;

  nimages_data.index = 0;
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), &nimages_data, nimages_capture, NULL, NULL);
  return IUP_DEFAULT;
}

/*************************************************************************************/

static int cap_all_devices(void)
{
  imlabImageCaptureAll(NULL, cap_image_capture);
  return IUP_DEFAULT;
}

/*************************************************************************************/

static int brightthres_capture(void* user_data, imImage* image, int device)
{
  static unsigned long histo[256], cut;
  double percent = *(double*)user_data;
  int i, level;
  (void)device;
  (void)user_data;

  cut = (int)((image->width * image->height * percent)/100.);

  imCalcByteHistogram((imbyte*)image->data[0], image->width * image->height, histo, 1);

  for (i = 0; i < 256; i++)
  {
    if (histo[i] > cut)
      break;
  }

  level = (i==0? 0: i==256? 254: i-1);

  if (image->color_space == IM_RGB) /* Could not set gray scale mode. */
  {
    imProcessThreshold(image, image, level, 255);
    memcpy(image->data[1], image->data[0], image->plane_size);
    memcpy(image->data[2], image->data[0], image->plane_size);
  }
  else
  {
    imProcessThreshold(image, image, level, 1);
    imImageSetBinary(image);
  }

  return 1;
}

static void brightthres_config(void* user_data, imVideoCapture* video_capture)
{
  if (video_capture)
  {
    double *percent = (double*)user_data;

    IupGetParam("Bright Threshold", NULL, NULL,
                "Histogram Count Percentage: %R[0,100]\n", 
                percent, NULL);

    if (!imVideoCaptureSetAttribute(video_capture, "VideoColorEnable", 0))
      IupMessage("Warning!", "Could not set gray scale mode.\nCapture will be done in color then converted to gray.");
  }
}

static int cap_bright_thres(Ihandle* self)
{
  static double percent = 80;
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), &percent, cap_image_capture, brightthres_capture, (imlabImageCaptureConfig)brightthres_config);
  return IUP_DEFAULT;
}

/*************************************************************************************/

static inline double sqr(double x)
{
  return x*x;
}

static void DoBackSubHongWooF(float **map, float **mean, float **stddev, imbyte *dst_map, imbyte *mask_map, int count, int depth, double factor)
{
  int d, i;
  float* pmap[4];
  float* pmean[4];
  float* pstddev[4];

  for(d = 0; d < depth; d++)
  {
    pmap[d] = map[d];
    pmean[d] = mean[d];
    pstddev[d] = stddev[d];
  }

  for (i = 0; i < count; i++)
  {
    if (mask_map && *mask_map == 0)
      *dst_map = 0;
    else
    {
      double sum1 = 0;
      double sum2 = 0;
      for(d = 0; d < depth; d++)
      {
        sum1 += sqr((*(pmap[d]) - *(pmean[d])));
        sum2 += sqr(*(pstddev[d]));
      }

      if (sum1 < factor*sum2)
        *dst_map = 0;
      else
        *dst_map = 1;
    }

    for(d = 0; d < depth; d++)
    {
      (pmap[d])++;
      (pmean[d])++;
      (pstddev[d])++;
    }
    if (mask_map) mask_map++;
    dst_map++;
  }
}

static void DoBackSubHongWoo(imbyte **map, imbyte **mean, float **stddev, imbyte *dst_map, imbyte *mask_map, int count, int depth, double factor)
{
  int d, i;
  imbyte* pmap[4];
  imbyte* pmean[4];
  float* pstddev[4];

  for(d = 0; d < depth; d++)
  {
    pmap[d] = map[d];
    pmean[d] = mean[d];
    pstddev[d] = stddev[d];
  }

  for (i = 0; i < count; i++)
  {
    if (mask_map && *mask_map == 0)
      *dst_map = 0;
    else
    {
      double sum1 = 0;
      double sum2 = 0;
      for(d = 0; d < depth; d++)
      {
        sum1 += sqr((*(pmap[d]) - *(pmean[d])));
        sum2 += sqr(*(pstddev[d]));
      }

      if (sum1 < factor*sum2)
        *dst_map = 0;
      else
        *dst_map = 1;
    }

    for(d = 0; d < depth; d++)
    {
      (pmap[d])++;
      (pmean[d])++;
      (pstddev[d])++;
    }
    if (mask_map) mask_map++;
    dst_map++;
  }
}

static void BackSubHongWoo(const imImage* src_image, const imImage* back_mean, const imImage* back_stddev, imImage* dst_image, const imImage* src_mask_image, double factor)
{
  if (src_image->data_type == IM_FLOAT)
    DoBackSubHongWooF((float**)src_image->data, (float**)back_mean->data, (float**)back_stddev->data, (imbyte*)dst_image->data[0], src_mask_image? (imbyte*)src_mask_image->data[0]: NULL, src_image->count, src_image->depth, factor);
  else
    DoBackSubHongWoo((imbyte**)src_image->data, (imbyte**)back_mean->data, (float**)back_stddev->data, (imbyte*)dst_image->data[0], src_mask_image? (imbyte*)src_mask_image->data[0]: NULL, src_image->count, src_image->depth, factor);
}

static void iCaptureDestroyArray(imImage* *images, int count)
{
  int i;
  for (i=0; i<count; i++)
  {
    if (images[i])
    {
      imImageDestroy(images[i]);
      images[i] = NULL;
    }
  }
}

/*************************************************************************************/

struct iCaptureBackground
{
  int count, show_back, index, calc;
  double alpha, beta;
  imImage *background_images[IMLAB_MAX_IMAGES],
          *mask_image,
          *mean_image,
          *stddev_image,
          *norm_image,
          *norm_mean_image,
          *norm_stddev_image;
};

static int hong_woo_preview(void* user_data, imImage* image, int device)
{
  iCaptureBackground* back_data = (iCaptureBackground*)user_data;
  (void)device;

  if (back_data->calc)
  {
    BackSubHongWoo(image, back_data->mean_image, back_data->stddev_image, back_data->mask_image, NULL, back_data->alpha);
    imProcessNormalizeComponents(image, back_data->norm_image);
    BackSubHongWoo(back_data->norm_image, back_data->norm_mean_image, back_data->norm_stddev_image, back_data->mask_image, back_data->mask_image, back_data->beta);
    imProcessBinaryMask(image, image, back_data->mask_image);
  }

  return 1;
}

static void iCalculateBackground(iCaptureBackground* back_data)
{
  int i;

  imProcessMultipleMean((const imImage**)back_data->background_images, back_data->count, back_data->mean_image);
  imProcessMultipleStdDev((const imImage**)back_data->background_images, back_data->count, back_data->mean_image, back_data->stddev_image);

  for (i = 0; i < back_data->count; i++)
  {
    if (back_data->background_images[i]->data_type != IM_FLOAT)
    {
      imImage* temp_back = imImageClone(back_data->stddev_image);
      imProcessNormalizeComponents(back_data->background_images[i], temp_back);
      imImageDestroy(back_data->background_images[i]);
      back_data->background_images[i] = temp_back;
    }
    else
      imProcessNormalizeComponents(back_data->background_images[i], back_data->background_images[i]);
  }

  imProcessMultipleMean((const imImage**)back_data->background_images, back_data->count, back_data->norm_mean_image);
  imProcessMultipleStdDev((const imImage**)back_data->background_images, back_data->count, back_data->norm_mean_image, back_data->norm_stddev_image);

  iCaptureDestroyArray(back_data->background_images, back_data->count);
}

static int hong_woo_capture(void* user_data, imImage* image, int device)
{
  imImage* ref_image;
  iCaptureBackground* back_data = (iCaptureBackground*)user_data;

  if (back_data->calc)
  {
    cap_image_capture(NULL, image, device);
    return 0;
  }

  if (back_data->index < back_data->count)
  {
    back_data->background_images[back_data->index] = imImageDuplicate(image);
    back_data->index++;
    return 1;
  }

  imlabLogMessagef("Captured %d background images.", back_data->count);

  ref_image = back_data->background_images[0];
  back_data->mean_image = imImageClone(ref_image);
  back_data->stddev_image = imImageCreateBased(ref_image, -1, -1, -1, IM_FLOAT);
  back_data->norm_mean_image = imImageClone(back_data->stddev_image);
  back_data->norm_stddev_image = imImageClone(back_data->stddev_image);
  back_data->mask_image = imImageCreateBased(ref_image, -1, -1, IM_BINARY, IM_BYTE);
  back_data->norm_image = imImageClone(back_data->stddev_image);

  iCalculateBackground(back_data);

  imlabLogMessagef("Background estimation completed.");
  back_data->calc = 1;
  back_data->index = 0;

  (void)device;
  return 0;
}

static void hong_woo_config(void* user_data, imVideoCapture* video_capture)
{
  iCaptureBackground* back_data = (iCaptureBackground*)user_data;

  if (video_capture)
  {
    static int count = 15;
    static double alpha = 8.0;
    static double beta = 6.0;

    if (!IupGetParam("Background Subtraction (Hong/Woo)", NULL, NULL,
                     "Number of Images for Estimation: %i[1,100]\n" 
                     "Alpha Threshold Factor: %R\n" 
                     "Beta Threshold Factor: %R\n", 
                     &count, &alpha, &beta, NULL))
      return;

    if (back_data->calc)
    {
      imImageDestroy(back_data->mean_image);
      imImageDestroy(back_data->stddev_image);
      imImageDestroy(back_data->norm_mean_image);
      imImageDestroy(back_data->norm_stddev_image);
      imImageDestroy(back_data->mask_image);
      imImageDestroy(back_data->norm_image);

      iCaptureDestroyArray(back_data->background_images, back_data->count);
    }

    memset(back_data, 0, sizeof(iCaptureBackground));
    back_data->count = count;
    back_data->alpha = alpha;
    back_data->beta = beta;
  }
  else
  {
    if (back_data->calc)
    {
      imImageDestroy(back_data->mean_image);
      imImageDestroy(back_data->stddev_image);
      imImageDestroy(back_data->norm_mean_image);
      imImageDestroy(back_data->norm_stddev_image);
      imImageDestroy(back_data->mask_image);
      imImageDestroy(back_data->norm_image);

      iCaptureDestroyArray(back_data->background_images, back_data->count);
    }
  }
}

static int cap_hong_woo(Ihandle* self)
{
  static iCaptureBackground back_data;
  memset(&back_data, 0, sizeof(iCaptureBackground));
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), &back_data, hong_woo_capture, hong_woo_preview, (imlabImageCaptureConfig)hong_woo_config);
  return IUP_DEFAULT;
}

/*************************************************************************************/

struct iCaptureAverage
{
  int count, index, start_index;
  imImage *images[IMLAB_MAX_IMAGES],
          *background;
};

static int average_preview(void* user_data, imImage* image, int device)
{
  iCaptureAverage* average_data = (iCaptureAverage*)user_data;
  int count;
  (void)device;

  if (average_data->images[average_data->index])
    imImageDestroy(average_data->images[average_data->index]);

  average_data->images[average_data->index] = imImageDuplicate(image);
  average_data->index++;
  if (average_data->index == average_data->count)
    average_data->index = 0;

  count = average_data->count;
  if (average_data->start_index < average_data->count)
  {
    average_data->start_index++;
    count = average_data->start_index;
  }

  imProcessMultipleMean((const imImage**)average_data->images, count, image);

  if (average_data->background)
    imProcessArithmeticOp(image, average_data->background, image, IM_BIN_DIFF);

  return 1;
}

static void average_config(void* user_data, imVideoCapture* video_capture)
{
  iCaptureAverage* average_data = (iCaptureAverage*)user_data;

  if (video_capture)
  {
    static int count = 5;

    if (!IupGetParam("Average Frames", NULL, NULL,
                     "Number of Images: %i[1,100]\n",
                     &count, NULL))
      return;

    iCaptureDestroyArray(average_data->images, average_data->count);

    memset(average_data, 0, sizeof(iCaptureAverage));
    average_data->count = count;
  }
  else
    iCaptureDestroyArray(average_data->images, average_data->count);
}

static void average_backsub_config(void* user_data, imVideoCapture* video_capture)
{
  iCaptureAverage* average_data = (iCaptureAverage*)user_data;

  if (video_capture)
  {
    int image_index;
    static int count = 5;
    char format[4096] = "Number of Images: %i[1,100]\nBackground Image: %l";
    imlabImageDocument* document;

    if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, NULL, &image_index))
      return;

    if (!IupGetParam("Average Frames", NULL, NULL,
                     format,
                     &count, &image_index, NULL))
      return;

    document = imlabImageDocumentListGetMatch(image_index, NULL, NULL);

    iCaptureDestroyArray(average_data->images, average_data->count);

    memset(average_data, 0, sizeof(iCaptureAverage));
    average_data->count = count;
    if (document)
      average_data->background = document->ImageFile->image;
  }
  else
    iCaptureDestroyArray(average_data->images, average_data->count);
}

static int cap_average(Ihandle* self)
{
  static iCaptureAverage average_data;
  memset(&average_data, 0, sizeof(iCaptureAverage));
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), &average_data, cap_image_capture, average_preview, (imlabImageCaptureConfig)average_config);
  return IUP_DEFAULT;
}

static int cap_average_backsub(Ihandle* self)
{
  static iCaptureAverage average_data;
  memset(&average_data, 0, sizeof(iCaptureAverage));
  imlabImageCaptureImageShow(IupGetAttribute(self, "TITLE"), &average_data, cap_image_capture, average_preview, (imlabImageCaptureConfig)average_backsub_config);
  return IUP_DEFAULT;
}

/*************************************************************************************/

void imlabInitCaptureMenu(Ihandle* mnCapture)
{
  Ihandle* item;
  IupAppend(mnCapture, item = imlabItem("Image...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_image);
  IupAppend(mnCapture, item = imlabItem("Multiple Images...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_n_images);
  IupAppend(mnCapture, item = imlabItem("Image From All Devices...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_all_devices);

  IupAppend(mnCapture, IupSeparator());

  IupAppend(mnCapture, item = imlabItem("Bright Threshold...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_bright_thres);
  IupSetAttribute(item, "imlabStatusHelp", "Background Subtraction using a bright threshold.");

  IupAppend(mnCapture, item = imlabItem("Background Subtraction (Hong/Woo)...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_hong_woo);
  IupSetAttribute(item, "imlabStatusHelp", "Based on Hong and Woo paper.");

  IupAppend(mnCapture, item = imlabItem("Average Frames...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_average);
  IupAppend(mnCapture, item = imlabItem("Average Frames and Subtract Background...", NULL));
  IupSetCallback(item, "ACTION", (Icallback)cap_average_backsub);
}

#endif

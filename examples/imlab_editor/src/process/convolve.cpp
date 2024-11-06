#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "kernel.h"

#include <im_process.h>
#include <im_kernel.h>
#include <im_math_op.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


static int load_convolve(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;
  char filename[10240] = "*.krn";
  
  if (!imlabDlgSelectFile(filename, "OPEN", "Load Kernel", "Kernel Files|*.krn|All Files|*.*|", "LastKernelDirectory"))
    return IUP_DEFAULT;

  imImage* kernel = imKernelLoad(filename);
  if (!kernel)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessConvolve(image, NewImage, kernel))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "Convolve{kernel=\"%s\"}", desc);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int convolve(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* kernel = imGetKernel(document->FileTitle, 0);
  if(!kernel)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessConvolve(image, NewImage, kernel))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "Convolve{kernel=\"%s\"}", desc);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int do_convolve_kernel(Ihandle *parent, imImage* kernel)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessConvolve(image, NewImage, kernel))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    document->ChangeImage(NewImage, "Convolve{kernel=\"%s\"}", desc);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int kernelsobel(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelSobel());
}
static int kernelprewitt(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelPrewitt());
}

static int kernelkirsh(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelKirsh());
}

static int kernellaplacian4(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelLaplacian4());
}

static int kernellaplacian8(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelLaplacian8());
}

static int kernellaplacian5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelLaplacian5x5());
}

static int kernellaplacian7x7(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelLaplacian7x7());
}

static int kernelgradian3x3(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelGradian3x3());
}

static int kernelgradian7x7(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelGradian7x7());
}

static int kernelsculpt(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelSculpt());
}

static int kernelmean3x3(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelMean3x3());
}

static int kernelmean5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelMean5x5());
}

static int kernelcircularmean5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelCircularMean5x5());
}

static int kernelmean7x7(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelMean7x7());
}

static int kernelcircularmean7x7(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelCircularMean7x7());
}

static int kernelgaussian3x3(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelGaussian3x3());
}

static int kernelgaussian5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelGaussian5x5());
}

static int kernelbarlett5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelBarlett5x5());
}

static int kerneltophat5x5(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelTopHat5x5());
}

static int kerneltophat7x7(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelTopHat7x7());
}

static int kernelenhance(Ihandle *parent)
{
  return do_convolve_kernel(parent, imKernelEnhance());
}

static int convolve_image(Ihandle *parent)
{
  imlabImageDocument* document1 = imlabGetCurrentDocument(parent);
  imImage *image = document1->ImageFile->image;

  imlabImageDocument* document2 = imlabImageDocumentListSelect("Select Second Image", (imlabMatchFunc)imImageIsIntRealGray, NULL);
  if (!document2)
    return IUP_DEFAULT;

  imImage *kernel = document2->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessConvolve(image, NewImage, kernel))
    imImageDestroy(NewImage);
  else
    document1->ChangeImage(NewImage, "Convolve{kernel=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int compass(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* kernel = imGetKernel(document->FileTitle, 1);
  if(!kernel)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel);
    return IUP_DEFAULT;
  }

  if (!imProcessCompassConvolve(image, NewImage, kernel))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "Compass{kernel=\"%s\"}", desc);
  }

  imImageDestroy(kernel);
  return IUP_DEFAULT;
}

static int dual(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* kernel1 = imGetKernel(document->FileTitle, 1);
  if(!kernel1)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(kernel1);
    return IUP_DEFAULT;
  }

  imImage* kernel2 = imImageClone(kernel1);
  imProcessRotate90(kernel1, kernel2, 1);

  if (!imProcessConvolveDual(image, NewImage, kernel1, kernel2))
    imImageDestroy(NewImage);
  else
  {
    const char* desc = (const char*)imImageGetAttribute(kernel1, "Description", NULL, NULL);
    if (!desc) desc = "Kernel";
    document->ChangeImage(NewImage, "ConvolveDual{kernel=\"%s\"}", desc);
  }

  imImageDestroy(kernel1);
  imImageDestroy(kernel2);
  return IUP_DEFAULT;
}

typedef int (*convFunc)(const imImage* image, imImage* NewImage, double stddev);

static int conv_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;
  double stddev = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    convFunc cfunc = (convFunc)IupGetFunction("_IMLAB_CONVOLVE_");

    if (!cfunc(image, NewImage, stddev))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int do_convolve(Ihandle *parent, const char* name, convFunc cfunc)
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
  IupSetFunction("_IMLAB_CONVOLVE_", (Icallback)cfunc);

  static double stddev = 1;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle(name), conv_preview, preview,
                   "Standard Deviation (or krn sz if<0): %R\n" 
                   "Preview: %b\n",
                   &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !cfunc(image, NewImage, stddev))
    imImageDestroy(NewImage);
  else
  {
    int kernel_size;
    if (stddev < 0)
    {
      kernel_size = (int)-stddev;
      stddev = imGaussianKernelSize2StdDev(kernel_size);
    }
    else
      kernel_size = imGaussianStdDev2KernelSize(stddev);

    document->ChangeImage(NewImage, "%s{stddev=%g}", name, (double)stddev);

    imlabLogMessagef("%s with stddev=%g uses kernel_size=%d", name, stddev, kernel_size);
  }

  return IUP_DEFAULT;
}

static int gaussian(Ihandle *parent)
{
  return do_convolve(parent, "GaussianConvolve", imProcessGaussianConvolve);
}

static int canny(Ihandle *parent)
{
  return do_convolve(parent, "Canny", imProcessCanny);
}

static int barlett_preview(Ihandle* dialog, int param_index, void* user_data)
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
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");

    if (!imProcessBarlettConvolve(image, NewImage, kernel_size))
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

static int barlett(Ihandle *parent)
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
  if (!IupGetParam(document->DlgTitle("Barlett"), barlett_preview, preview,
                   "Kernel Size: %i[0,]\n" 
                   "Preview: %b\n",
                   &kernel_size, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessBarlettConvolve(image, NewImage, kernel_size))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "BarlettConvolve{kernel_size=%d}", kernel_size, kernel_size);

  return IUP_DEFAULT;
}

static int splineedge(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessSplineEdgeConvolve(image, NewImage))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  document->ChangeImage(NewImage, "SplineEdgeConvolve{}");

  return IUP_DEFAULT;
}

static int sobel(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessSobelConvolve(image, NewImage))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  document->ChangeImage(NewImage, "SobelConvolve{}");

  return IUP_DEFAULT;
}

static int prewitt(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessPrewittConvolve(image, NewImage))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  document->ChangeImage(NewImage, "PrewittConvolve{}");

  return IUP_DEFAULT;
}

static int crossing(Ihandle *parent)
{
  imImage *NewImage, *image; 
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (!imProcessZeroCrossing(image, NewImage))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "ZeroCrossing{}");

  return IUP_DEFAULT;
}

static int lap_gauss(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = image->data_type;
  if (image->data_type == IM_BYTE || image->data_type == IM_USHORT)
    data_type = IM_INT;

  imImage* NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  IupSetFunction("_IMLAB_CONVOLVE_", (Icallback)imProcessLapOfGaussianConvolve);

  static double stddev = 1;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Laplacian of Gaussian"), conv_preview, preview,
                   "Standard Deviation (or krn sz if<0): %R\n" 
                   "Preview: %b\n",
                   &stddev, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessLapOfGaussianConvolve(image, NewImage, stddev))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "LapOfGaussianConvolve{stddev=%g}", (double)stddev);

  return IUP_DEFAULT;
}

typedef int (*conv2Func)(const imImage* image, imImage* NewImage, double v1, double v2);

static int conv2_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    conv2Func cfunc = (conv2Func)IupGetFunction("_IMLAB_CONVOLVE_");
    double v1 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double v2 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!cfunc(image, NewImage, v1, v2))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int diff_gauss(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = image->data_type;
  if (image->data_type == IM_BYTE || image->data_type == IM_USHORT)
    data_type = IM_INT;

  imImage* NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);
  IupSetFunction("_IMLAB_CONVOLVE_", (Icallback)imProcessDiffOfGaussianConvolve);

  static double stddev1 = 1.5;
  static double stddev2 = 1;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Difference of Gaussians"), conv2_preview, preview,
                   "Standard Deviation 1 (or krn sz if<0): %R\n" 
                   "Standard Deviation 2 (or krn sz if<0): %R\n" 
                   "Preview: %b\n",
                   &stddev1, &stddev2, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessDiffOfGaussianConvolve(image, NewImage, stddev1, stddev2))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "DiffOfGaussianConvolve{stddev1=%g, stddev2=%g}", (double)stddev1, (double)stddev2);

  return IUP_DEFAULT;
}

static int sharp_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double v1 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double v2 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    if (!imProcessSharp(image, NewImage, v1, v2))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int sharp(Ihandle *parent)
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

  static double amount = 0.1f;
  static double threshold = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Sharp"), sharp_preview, preview,
                   "Amount: %R[0,1,0.01]\n" 
                   "Threshold: %R\n" 
                   "Preview: %b\n",
                   &amount, &threshold, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessSharp(image, NewImage, amount, threshold))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Sharp{amount=%g, threshold=%g}", amount, threshold);

  return IUP_DEFAULT;
}

static int unsharp_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double v1 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double v2 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    double v3 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    if (!imProcessUnsharp(image, NewImage, v1, v2, v3))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int unsharp(Ihandle *parent)
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

  static double stddev = 1.5;
  static double amount = 0.1;
  static double threshold = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Unsharp"), unsharp_preview, preview,
                   "Standard Deviation (or krn sz if<0): %R\n" 
                   "Amount: %R[0,1,0.01]\n" 
                   "Threshold: %R\n" 
                   "Preview: %b\n",
                   &stddev, &amount, &threshold, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessUnsharp(image, NewImage, stddev, amount, threshold))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Unsharp{stddev=%g, amount=%g, threshold=%g}", (double)stddev, (double)amount, (double)threshold);

  return IUP_DEFAULT;
}

static int mean_preview(Ihandle* dialog, int param_index, void* user_data)
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
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM1", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");

    if (!imProcessMeanConvolve(image, NewImage, kernel_size))
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

static int mean(Ihandle *parent)
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
  if (!IupGetParam(document->DlgTitle("Mean"), mean_preview, preview,
                   "Kernel Size: %i[0,]\n" 
                   "Preview: %b\n",
                   &kernel_size, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview && !imProcessMeanConvolve(image, NewImage, kernel_size))
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "MeanConvolve{kernel_size=%d}", kernel_size);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle *submenu, *kernel_submenu;

  kernel_submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Sobel", "kernelsobel", (Icallback)kernelsobel, 1), 
    imlabProcNewItem(mnProcess, "Prewitt", "kernelprewitt", (Icallback)kernelprewitt, 1), 
    imlabProcNewItem(mnProcess, "Kirsh", "kernelkirsh", (Icallback)kernelkirsh, 1), 
    imlabProcNewItem(mnProcess, "Laplacian4", "kernellaplacian4", (Icallback)kernellaplacian4, 1), 
    imlabProcNewItem(mnProcess, "Laplacian8", "kernellaplacian8", (Icallback)kernellaplacian8, 1), 
    imlabProcNewItem(mnProcess, "Laplacian5x5", "kernellaplacian5x5", (Icallback)kernellaplacian5x5, 1), 
    imlabProcNewItem(mnProcess, "Laplacian7x7", "kernellaplacian7x7", (Icallback)kernellaplacian7x7, 1), 
    imlabProcNewItem(mnProcess, "Gradian3x3", "kernelgradian3x3", (Icallback)kernelgradian3x3, 1), 
    imlabProcNewItem(mnProcess, "Gradian7x7", "kernelgradian7x7", (Icallback)kernelgradian7x7, 1), 
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Mean3x3", "kernelmean3x3", (Icallback)kernelmean3x3, 1), 
    imlabProcNewItem(mnProcess, "Mean5x5", "kernelmean5x5", (Icallback)kernelmean5x5, 1), 
    imlabProcNewItem(mnProcess, "CircularMean5x5", "kernelcircularmean5x5", (Icallback)kernelcircularmean5x5, 1), 
    imlabProcNewItem(mnProcess, "Mean7x7", "kernelmean7x7", (Icallback)kernelmean7x7, 1), 
    imlabProcNewItem(mnProcess, "CircularMean7x7", "kernelcircularmean7x7", (Icallback)kernelcircularmean7x7, 1), 
    imlabProcNewItem(mnProcess, "Gaussian3x3", "kernelgaussian3x3", (Icallback)kernelgaussian3x3, 1), 
    imlabProcNewItem(mnProcess, "Gaussian5x5", "kernelgaussian5x5", (Icallback)kernelgaussian5x5, 1), 
    imlabProcNewItem(mnProcess, "Barlett5x5", "kernelbarlett5x5", (Icallback)kernelbarlett5x5, 1), 
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Sculpt", "kernelsculpt", (Icallback)kernelsculpt, 1), 
    imlabProcNewItem(mnProcess, "TopHat5x5", "kerneltophat5x5", (Icallback)kerneltophat5x5, 1), 
    imlabProcNewItem(mnProcess, "TopHat7x7", "kerneltophat7x7", (Icallback)kerneltophat7x7, 1), 
    imlabProcNewItem(mnProcess, "Enhance", "kernelenhance", (Icallback)kernelenhance, 1), 
    NULL);

  submenu = IupMenu(
    imlabSubmenu("Convolve Kernel", kernel_submenu),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Convolve Kernel...", "convolve", (Icallback)convolve, 1), 
    imlabProcNewItem(mnProcess, "Load and Convolve...", "load_convolve", (Icallback)load_convolve, 1), 
    imlabProcNewItem(mnProcess, "Convolve Image...", "convolve_image", (Icallback)convolve_image, 1), 
    imlabProcNewItem(mnProcess, "Compass...", "compass", (Icallback)compass, 1),
    imlabProcNewItem(mnProcess, "Dual...", "dual", (Icallback)dual, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Circular Mean...", "mean", (Icallback)mean, 1),
    imlabProcNewItem(mnProcess, "Barlett...", "barlett", (Icallback)barlett, 1),
    imlabProcNewItem(mnProcess, "Gaussian...", "gaussian", (Icallback)gaussian, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Sobel Magnitude", "sobel", (Icallback)sobel, 1), 
    imlabProcNewItem(mnProcess, "Prewitt Magnitude", "prewitt", (Icallback)prewitt, 1), 
    imlabProcNewItem(mnProcess, "SplineEdge", "splineedge", (Icallback)splineedge, 1), 
    imlabProcNewItem(mnProcess, "Laplacian of Gaussian...", "lap_gauss", (Icallback)lap_gauss, 1), 
    imlabProcNewItem(mnProcess, "Difference of Gaussians...", "diff_gauss", (Icallback)diff_gauss, 1), 
    imlabProcNewItem(mnProcess, "Zero Crossing", "crossing", (Icallback)crossing, 1), 
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Canny...", "canny", (Icallback)canny, 1), 
    imlabProcNewItem(mnProcess, "Sharp...", "sharp", (Icallback)sharp, 1), 
    imlabProcNewItem(mnProcess, "Unsharp...", "unsharp", (Icallback)unsharp, 1), 
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "crossing"), "imlabStatusHelp", "Finds zero crossings. Adapted from XITE.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "compass"), "imlabStatusHelp", "Rotates the Kernel and apply the convolution 8 times.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "dual"), "imlabStatusHelp", "Convolve the given kernel and its 90 degrees rotated version, and returns the magnitude of the results.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "canny"), "imlabStatusHelp", "Canny filter and nonmax supression. Use Threshold / Hysteresis after.");

  IupAppend(mnProcess, imlabSubmenu("Filter", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{         
  imlabProcPlugInUpdateItem(mnProcess, "kernelsobel", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelprewitt", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelkirsh", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernellaplacian4", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernellaplacian8", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernellaplacian5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernellaplacian7x7", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelgradian3x3", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelgradian7x7", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelsculpt", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelmean3x3", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelmean5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelcircularmean5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelmean7x7", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelcircularmean7x7", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelgaussian3x3", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelgaussian5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelbarlett5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kerneltophat5x5", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kerneltophat7x7", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "kernelenhance", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "convolve", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "convolve_image", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "load_convolve", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "compass", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "dual", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "gaussian", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "mean", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "barlett", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "canny", imImageIsByteGray);
  imlabProcPlugInUpdateItem(mnProcess, "sharp", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "unsharp", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "sobel", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "splineedge", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "prewitt", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "diff_gauss", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "lap_gauss", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "crossing", imImageIsSigned);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinConv = &plugin;

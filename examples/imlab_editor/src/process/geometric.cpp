#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "counter.h"

#include <im_process.h>

#include <cd.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>


static int swirl_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double k = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int order = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    if (order == 2) order = 3;

    if (!imProcessSwirl(image, NewImage, k, order))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int swirl(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  /* creates the new image */
  imImage* NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int order = 1;
  static double k = 0.05f;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Swirl Distortion"), swirl_preview, preview,
                   "Factor: %R\n"
                   "Interpolation Order: %l|box|linear|cubic|\n" 
                   "Preview: %b\n",
                   &k, &order, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (order == 2) order = 3;

  if (!show_preview && !imProcessSwirl(image, NewImage, k, order))
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(NewImage, "Swirl{factor=%g, order=\"%s\"}", k, order_str);
  }

  return IUP_DEFAULT;
}

static int radial_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double k1 = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int order = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    if (order == 2) order = 3;

    if (!imProcessRadial(image, NewImage, k1, order))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int radial(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  /* creates the new image */
  imImage* NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int order = 1;
  static double k1 = 0.0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Radial Distortion"), radial_preview, preview,
                   "Factor: %R\n"
                   "Interpolation Order: %l|box|linear|cubic|\n" 
                   "Preview: %b\n",
                   &k1, &order, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (order == 2) order = 3;

  if (!show_preview && !imProcessRadial(image, NewImage, k1, order))
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(NewImage, "Radial{factor=%g, order=\"%s\"}", k1, order_str);
  }

  return IUP_DEFAULT;
}

static imImage* lensdistort_NewImage = NULL;

static int lensdistort_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM6", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    double a = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    double b = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    double c = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    double scale = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "VALUE");
    double factor = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM4"), "VALUE");
    int order = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM5"), "VALUE");
    if (order == 2) order = 3;

    int dst_width = (int)(image->width * scale + 0.5);
    int dst_height = (int)(image->height * scale + 0.5);
    if (dst_width != NewImage->width ||
        dst_height != NewImage->height)
    {
      imImageDestroy(NewImage);
      NewImage = imImageCreateBased(image, dst_width, dst_height, -1, -1);
      IupSetAttribute(preview, "NewImage", (char*)NewImage);
      imlabProcessPreviewUpdate(preview);

      lensdistort_NewImage = NewImage;
    }

    if (!imProcessLensDistort(image, NewImage, a*factor, b*factor, c*factor, order))
      imlabProcessPreviewReset(preview);
    else
      imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int lensdistort(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  /* creates the new image */
  lensdistort_NewImage = imImageClone(image);
  if (lensdistort_NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, lensdistort_NewImage);

  IupSetGlobal("DEFAULTPRECISION", "8");

  static int order = 1;
  static double a = 0.0;
  static double b = 0.0;
  static double c = 0.0;
  static double scale = 1.0;
  static double factor = 1.0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Lens Distortion"), lensdistort_preview, preview,
    "Coefficient A: %R[-1,1]\n"
    "Coefficient B: %R[-1,1]\n"
    "Coefficient C: %R[-1,1]\n"
    "Scale: %R[1,10]\n"
    "Factor: %R[0,10]\n"
    "Interpolation Order: %l|box|linear|cubic|\n"
    "Preview: %b\n",
    &a, &b, &c, &scale, &factor, &order, &show_preview, NULL))
  {
    IupSetGlobal("DEFAULTPRECISION", "4");

    imImageDestroy(lensdistort_NewImage);
    lensdistort_NewImage = NULL;
    return IUP_DEFAULT;
  }

  if (order == 2) order = 3;

  if (!show_preview)
  {
    int dst_width = (int)(image->width * scale + 0.5);
    int dst_height = (int)(image->height * scale + 0.5);

    if (dst_width != lensdistort_NewImage->width ||
        dst_height != lensdistort_NewImage->height)
    {
      imImageDestroy(lensdistort_NewImage);
      lensdistort_NewImage = imImageCreateBased(image, dst_width, dst_height, -1, -1);
    }
  }

  if (!show_preview && !imProcessLensDistort(image, lensdistort_NewImage, a*factor, b*factor, c*factor, order))
    imImageDestroy(lensdistort_NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(lensdistort_NewImage, "LensDistort{a=%.8g, b=%.8g, c=%.8g, order=\"%s\"}", a, b, c, order_str);
  }

  IupSetGlobal("DEFAULTPRECISION", "4");

  lensdistort_NewImage = NULL;
  return IUP_DEFAULT;
}

static int rotate_center(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* NewImage;
  static double angle = 0;
  static int order = 1;
  static int resize = 1;
  if (!IupGetParam(document->DlgTitle("Rotate Center"), NULL, NULL,
                   "Angle (degrees): %A\n"
                   "Interpolation Order: %l|box|linear|cubic|\n"
                   "Preserve Size: %b\n",
                   &angle, &order, &resize, NULL))
    return IUP_DEFAULT;

  double cos0, sin0;
  sin0 = sin(angle/CD_RAD2DEG);
  cos0 = cos(angle/CD_RAD2DEG);

  if (resize)
  {
    int new_w, new_h;
    imProcessCalcRotateSize(image->width, 
                            image->height, 
                            &new_w, &new_h, cos0, sin0);
    NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  }
  else
    NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (order == 2) order = 3;
  int ret = imProcessRotate(image, NewImage, cos0, sin0, order);

  if (!ret)
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(NewImage, "Rotate{angle=%g, order=\"%s\"}", angle, order_str);
  }

  return IUP_DEFAULT;
}

static int rotate(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static double angle = 0;
  static int order = 1;
  static int x = 0;
  static int y = 0;
  static int to_origin = 0;
  if (!IupGetParam(document->DlgTitle("Rotate"), NULL, NULL,
                   "Angle (degrees): %A\n"
                   "Reference X: %i\n"
                   "Reference Y: %i\n"
                   "Move to Origin: %b\n"
                   "Interpolation Order: %l|box|linear|cubic|\n",
                   &angle, &x, &y, &to_origin, &order, NULL))
    return IUP_DEFAULT;

  double cos0, sin0;
  sin0 = sin(angle/CD_RAD2DEG);
  cos0 = cos(angle/CD_RAD2DEG);

  imImage* NewImage = imImageClone(image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (order == 2) order = 3;
  int ret = imProcessRotateRef(image, NewImage, cos0, sin0, x, y, to_origin, order);

  if (!ret)
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(NewImage, "RotateRef{angle=%g, x=%d, y=%d, origin=%d, order=\"%s\"}", angle, x, y, to_origin, order_str);
  }

  return IUP_DEFAULT;
}

static int rotate_ccw90(Ihandle *parent)
{
  imImage* NewImage, *image;
  imlabImageDocument* document;
  int new_w, new_h;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  // Invert dimensions
  new_h = image->width;
  new_w = image->height;

  /* creates the new image */
  NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessRotate90(image, NewImage, -1);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Rotate90{clockwise=-1}");

  return IUP_DEFAULT;
}

//*******************************************************************************************
//rotate_ccw90
//*******************************************************************************************

static int rotate_cw90(Ihandle *parent)
{
  imImage* NewImage, *image;
  imlabImageDocument* document;
  int new_w, new_h;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  // Invert dimensions
  new_h = image->width;
  new_w = image->height;

  /* creates the new image */
  NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessRotate90(image, NewImage, 1);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Rotate90{clockwise=1}");

  return IUP_DEFAULT;
}

//*******************************************************************************************
//rotate_180
//*******************************************************************************************

static int rotate_180(Ihandle *parent)
{
  imImage* NewImage;
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);

  /* creates the new image */
  NewImage = imImageClone(document->ImageFile->image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessRotate180(document->ImageFile->image, NewImage);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Rotate180{}");

  return IUP_DEFAULT;
}

//*******************************************************************************************
//mirror
//*******************************************************************************************

static int mirror(Ihandle *parent)
{
  imImage* NewImage;
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);

  /* creates the new image */
  NewImage = imImageClone(document->ImageFile->image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessMirror(document->ImageFile->image, NewImage);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Mirror{}");

  return IUP_DEFAULT;
}

//*******************************************************************************************
//flip
//*******************************************************************************************

static int flip(Ihandle *parent)
{
  imImage* NewImage;
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);

  /* creates the new image */
  NewImage = imImageClone(document->ImageFile->image);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessFlip(document->ImageFile->image, NewImage);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "Flip{}");

  return IUP_DEFAULT;
}

static int interlacesplit(Ihandle *parent)
{
  imImage *NewImage1, *NewImage2;
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int new_h1 = image->height/2;
  if (image->height%2)
    new_h1++;

  /* creates the new image */
  NewImage1 = imImageCreateBased(image, -1, new_h1, -1, -1);
  if (NewImage1 == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  NewImage2 = imImageCreateBased(image, -1, image->height/2, -1, -1);
  if (NewImage2 == NULL)
  {
    imImageDestroy(NewImage1);
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  int ret = imProcessInterlaceSplit(image, NewImage1, NewImage2);

  if (!ret)
  {
    imImageDestroy(NewImage1);
    imImageDestroy(NewImage2);
  }
  else
  {
    imlabImageDocumentCreateFromImage(NewImage1, "EvenLines of %s", "EvenLines{image=\"%s\"}", document->FileTitle);
    imlabImageDocumentCreateFromImage(NewImage2, "OddLines of %s", "OddLines{image=\"%s\"}", document->FileTitle);
  }

  return IUP_DEFAULT;
}

//*******************************************************************************************
//GeomInit
//*******************************************************************************************

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Rotate...","rotate", (Icallback) rotate, 0),
    imlabProcNewItem(mnProcess, "Rotate Center...","rotate_center", (Icallback) rotate_center, 0),
    imlabProcNewItem(mnProcess, "Rotate +90° (cw)","rotate_cw90", (Icallback) rotate_cw90, 0),
    imlabProcNewItem(mnProcess, "Rotate -90° (ccw)","rotate_ccw90", (Icallback) rotate_ccw90, 0),
    imlabProcNewItem(mnProcess, "Rotate 180°","rotate_180", (Icallback) rotate_180, 0),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Mirror","mirror", (Icallback) mirror, 0),
    imlabProcNewItem(mnProcess, "Flip","flip", (Icallback) flip, 0),
    imlabProcNewItem(mnProcess, "Interlace Split","interlacesplit", (Icallback) interlacesplit, 0),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Radial...","radial", (Icallback) radial, 1),
    imlabProcNewItem(mnProcess, "Lens Distort...", "lensdistort", (Icallback)lensdistort, 1),
    imlabProcNewItem(mnProcess, "Swirl...", "swirl", (Icallback)swirl, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Geometric", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "radial", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "lensdistort", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "swirl", imImageIsSci);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinGeom = &plugin;

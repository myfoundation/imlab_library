#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "counter.h"

#include <im_process.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


static int GetReduceSize(imlabImageDocument* document, int *width, int *height, int *order, int org_width, int org_height)
{
  char format[512];
  static int aspect = 1;
  double factor = (double)org_width / (double)org_height;
  sprintf(format, "Width [%d]: %%i[1,%d]\n"
                  "Height [%d]: %%i[1,%d]\n"
                  "Decimation Order: %%l|box|linear|\n"
                  "Maintain Aspect: %%b\n", 
                  org_width, org_width, org_height, org_height);

  if (*width == 0 || *width > org_width) *width = org_width;
  if (*height == 0 || *height > org_height) *height = org_height; 

  if (aspect)
    *height = (int)((double)(*width) / factor + 0.5);
  
  if (!IupGetParam(document->DlgTitle("Reduce Size"), imlabDlgGetNewSizeCheck, &factor, format, width, height, order, &aspect, NULL))
    return 0;

  return 1;
}

static int reduce(Ihandle *parent)
{
  static int new_w = 0, new_h = 0, order = 1;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    order = 0;

  if (!GetReduceSize(document, &new_w, &new_h, &order, image->width, image->height))
    return IUP_DEFAULT;

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    order = 0;

  /* creates the new image */
  imImage* NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  if (!imProcessReduce(image, NewImage, order))
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 0) order_str = "mean";
    document->ChangeImage(NewImage, "Reduce{width=%d, height=%d, order=\"%s\"}", new_w, new_h, order_str);
  }

  return IUP_DEFAULT;
}

static int resize(Ihandle *parent)
{
  static int new_w = 0, new_h = 0, order = 1;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    order = 0;

  if (!imlabDlgGetNewSize(&new_w, &new_h, &order, image->width, image->height))
    return IUP_DEFAULT;

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    order = 0;

  /* creates the new image */
  imImage* NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  if (!imProcessResize(image, NewImage, order))
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 3) order_str = "cubic";
    if (order == 0) order_str = "nearest";
    document->ChangeImage(NewImage, "Resize{width=%d, height=%d, order=\"%s\"}", new_w, new_h, order_str);
  }

  return IUP_DEFAULT;
}

static int check_crop(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;
  if (param_index == -1)
  {
    int xmin = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int xmax = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    int ymin = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    int ymax = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "VALUE");

    if (xmin > xmax || ymin > ymax)
    {
      IupMessage("Error!", "Invalid limits, xmin > xmax or ymin > ymax.");
      return 0;
    }
  }
  return 1;
}

static int crop(Ihandle *parent)
{
  static int xmin = 0, xmax = 0, ymin = 0, ymax = 0;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  char format[4096];
  sprintf(format, "X Min: %%i[0,%d]\n"
                  "X Max: %%i[0,%d]\n"
                  "Y Min: %%i[0,%d]\n"
                  "Y Max: %%i[0,%d]\n", image->width-1, image->width-1, image->height-1, image->height-1);

  if (!IupGetParam(document->DlgTitle("Crop Limits"), check_crop, NULL, format,
                   &xmin, &xmax, &ymin, &ymax, NULL))
    return IUP_DEFAULT;

  imImage *NewImage = imImageCreateBased(image, xmax - xmin + 1, ymax - ymin + 1, -1, -1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessCrop(image, NewImage, xmin, ymin);

  document->ChangeImage(NewImage, "Crop{xmin=%d, ymin=%d, xmax=%d, ymax=%d}", xmin, ymin, xmax, ymax);

  return IUP_DEFAULT;
}

static int iMatchInsert(const imImage* image, const imImage* match_image)
{
  if (!imImageMatchColor(image, match_image))  /* ignore size */
    return 0;

  return 1;
}

static int insert(Ihandle *parent)
{
  static int xmin = 0, ymin = 0;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  char format[4096];
  int index = 0;
  sprintf(format, "X Min: %%i[0,%d]\n"
                  "Y Min: %%i[0,%d]\n"
                  "Region Image: %%l",
                  image->width-1, image->height-1);

  if (!imlabImageDocumentListInitFormat(format+strlen(format), iMatchInsert, image, &index))
    return IUP_DEFAULT;

  if (!IupGetParam(document->DlgTitle("Insert Region"), NULL, NULL, format,
                   &xmin, &ymin, &index, NULL))
    return IUP_DEFAULT;

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(index, iMatchInsert, image);

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessInsert(image, document2->ImageFile->image, NewImage, xmin, ymin);

  document->ChangeImage(NewImage, "Insert{region_image=\"%s\", xmin=%d, ymin=%d}", document2->FileTitle, xmin, ymin);

  return IUP_DEFAULT;
}

static int add_margins(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static int left = 0, right = 0, bottom = 0, top = 0;

  if (!IupGetParam(document->DlgTitle("Add Margins"), NULL, NULL,
                   "Left: %i[0,]\n"
                   "Right: %i[0,]\n"
                   "Bottom: %i[0,]\n"
                   "Top: %i[0,]\n", 
                   &left, &right, &bottom, &top, NULL))
    return IUP_DEFAULT;

  imImage *NewImage = imImageCreateBased(image, image->width + right + left, image->height + top + bottom, -1, -1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessAddMargins(image, NewImage, left, bottom);

  document->ChangeImage(NewImage, "AddMargins{left=%d, right=%d, bottom=%d, top=%d}", left, right, bottom, top);

  return IUP_DEFAULT;
}

static int reduce_area_2(Ihandle *parent)
{
  imImage* NewImage;
  imlabImageDocument* document;
  int new_w, new_h, order = 1;
  double factor_sqrt_2 = 1.414;

  document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  new_w = (int)(image->width / factor_sqrt_2);
  new_h = (int)(image->height / factor_sqrt_2);
 
  /* creates the new image */
  NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    order = 0;

  /* Process the image */
  if (!imProcessReduce(image, NewImage, order))
    imImageDestroy(NewImage);
  else
  {
    char* order_str = "linear";
    if (order == 0) order_str = "mean";
    document->ChangeImage(NewImage, "Reduce{width=%d, height=%d, order=\"%s\"}", new_w, new_h, order_str);
  }

  return IUP_DEFAULT;
}

static int reduce_area_4(Ihandle *parent)
{
  imImage* NewImage;
  imlabImageDocument* document;
  int new_w, new_h;

  document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  new_w = (int)(image->width / 2);
  new_h = (int)(image->height / 2);

  /* creates the new image */
  NewImage = imImageCreateBased(image, new_w, new_h, -1, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
  {
    if (!imProcessReduce(image, NewImage, 0))
      imImageDestroy(NewImage);
    else
      document->ChangeImage(NewImage, "Reduce{width=%d, height=%d, order=\"mean\"}", new_w, new_h);
  }
  else
  {
    int ret = imProcessReduceBy4(image, NewImage);
    if (!ret)
      imImageDestroy(NewImage);
    else
      document->ChangeImage(NewImage, "ReduceBy4{}");
  }

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Resize...", "resize", (Icallback) resize, 0),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Reduce...","reduce", (Icallback) reduce, 1),
    imlabProcNewItem(mnProcess, "Reduce Area By 2","reduce_area_2", (Icallback) reduce_area_2, 1),
    imlabProcNewItem(mnProcess, "Reduce Area By 4","reduce_area_4", (Icallback) reduce_area_4, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Crop...", "crop", (Icallback) crop, 0),
    imlabProcNewItem(mnProcess, "Insert...", "insert_rgn", (Icallback) insert, 0),
    imlabProcNewItem(mnProcess, "Add Margins...", "add_margins", (Icallback) add_margins, 0),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Size", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "reduce", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "reduce_area_2", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "reduce_area_4", imImageIsSci);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinResize = &plugin;

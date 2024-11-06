/** \file
 * \brief Image View using CD and IUP
 *
 * See Copyright Notice in im_lib.h
 */


#include "im_imageview.h"

#include <im_convert.h>
#include <im_process.h>
#include <im_util.h>
#include <im_palette.h>
#include <cdiup.h>
#include <cdirgb.h>
#include <cdpicture.h>

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>


#define IV_ZOOMMAX 13
#define IV_ZOOMZERO 6
/*                     ZoomIndex = -6,-5,-4,-3,-2,-1,0,1,2,3, 4, 5, 6  */
static int ZoomTable1[IV_ZOOMMAX]={ 1, 1, 1, 1, 1, 1,1,2,4,8,16,32,64};
static int ZoomTable2[IV_ZOOMMAX]={64,32,16,8,4,2,1,1,1,1,1,1,1};
static int ZoomTable3[IV_ZOOMMAX]={1,3,6,12,25,50,100,200,400,800,1600,3200,6400};
static _imImageViewRepaintCB imImageViewRepaintCB = NULL;
static _imImageViewResizeCB imImageViewResizeCB = NULL;
static _imImageViewMouseCB imImageViewMouseCB = NULL;

#define IMV_MARGIN 8

static void iViewResetScrollSize(imImageView* image_view)
{
  image_view->repaint = 0;
  IupSetAttribute(image_view->canvas, "DX", "1");
  IupSetAttribute(image_view->canvas, "DY", "1");
  image_view->repaint = 1;

  cdCanvasActivate(image_view->cd_canvas);
  cdCanvasGetSize(image_view->cd_canvas, &image_view->Width, &image_view->Height, 0, 0);
}

static void iViewResetScrollPos(imImageView* image_view)
{
  image_view->repaint = 0;
  IupSetAttribute(image_view->canvas, "POSX", "0");
  IupSetAttribute(image_view->canvas, "POSY", "0");
  image_view->repaint = 1;
}

static void iViewSetScrollPos(imImageView* image_view, float posx, float posy)
{
  float dx = IupGetFloat(image_view->canvas, "DX");
  float dy = IupGetFloat(image_view->canvas, "DY");

  if (posx < 0) posx = 0;
  if (posx > 1 - dx) posx = 1 - dx;

  if (posy < 0) posy = 0;
  if (posy > 1 - dy) posy = 1 - dy;

  image_view->repaint = 0;
  IupSetFloat(image_view->canvas, "POSX", posx);
  IupSetFloat(image_view->canvas, "POSY", posy);
  image_view->repaint = 1;
}

/* needs to be called when the view size is changed */
/* recalculates the view size and updates the scrollbar DX attribute */
static void iViewUpdateSize(imImageView* image_view)
{
  switch(image_view->ViewMode)
  {
  case IM_VIEW_NORMAL:
  {
    /* Remember that for the scroll bar:
        xmin <= posx <= xmax-dx
        xmin = 0
        xmax = 1
        dx = Width/(margin+viewW+margin) */
    int elem_width,   elem_height,
        view_width,   view_height,
      canvas_width, canvas_height;
    int scrollbar_size = IupGetInt(NULL, "SCROLLBARSIZE");
    IupGetIntInt(image_view->canvas, "RASTERSIZE", &elem_width, &elem_height);

    image_view->ViewWidth = (int)(image_view->ZoomFactor * image_view->BitmapView->image->width);
    image_view->ViewHeight = (int)(image_view->ZoomFactor * image_view->BitmapView->image->height);
    view_width = image_view->ViewWidth + 2 * IMV_MARGIN;
    view_height = image_view->ViewHeight + 2 * IMV_MARGIN;

    /* if view is greater than cd_canvas in one direction,
    then it has scrollbars,
    but this affects the opposite direction */
    elem_width -= 2;  /* remove BORDER (always size=1) */
    elem_height -= 2;
    canvas_width = elem_width;
    canvas_height = elem_height;
    if (view_width > elem_width)  /* check for horizontal scrollbar */
      canvas_height -= scrollbar_size;  /* affect vertical size */
    if (view_height > elem_height)
      canvas_width -= scrollbar_size;
    if (view_width <= elem_width && view_width > canvas_width)  /* check if still has horizontal scrollbar */
      canvas_height -= scrollbar_size;
    if (view_height <= elem_height && view_height > canvas_height)
      canvas_width -= scrollbar_size;
    if (canvas_width < 0) canvas_width = 0;
    if (canvas_height < 0) canvas_height = 0;

    IupSetFloat(image_view->canvas, "DX", (float)canvas_width / (float)view_width);
    IupSetFloat(image_view->canvas, "DY", (float)canvas_height / (float)view_height);

    cdCanvasActivate(image_view->cd_canvas);
    cdCanvasGetSize(image_view->cd_canvas, &image_view->Width, &image_view->Height, 0, 0);
    break;
  }
  case IM_VIEW_FIT:
    /* The image ocupies all the cd_canvas and is centered, but does not changes its aspect ratio */
    iViewResetScrollSize(image_view);

    imImageViewFitRect(image_view->Width - 2 * IMV_MARGIN, image_view->Height - 2 * IMV_MARGIN, image_view->BitmapView->image->width, image_view->BitmapView->image->height, &image_view->ViewWidth, &image_view->ViewHeight);
    image_view->ViewX = (image_view->Width - image_view->ViewWidth) / 2;
    image_view->ViewY = (image_view->Height - image_view->ViewHeight) / 2;
    break;
  }
}

/* updates the view position,
   used only when in Normal view. */
static void iViewUpdatePos(imImageView* image_view)
{
  float posy = IupGetFloat(image_view->canvas, "POSY");
  float posx = IupGetFloat(image_view->canvas, "POSX");

  /* If the cd_canvas is bigger than the view then just center the view in the cd_canvas */
  /* Else we have to center the cd_canvas inside the view using the scroll position. */

  if (image_view->Height < image_view->ViewHeight)
  {
    /* posy is top-bottom, CD is bottom-top.
    invert posy reference (YMAX-DY - POSY) */
    posy = 1.0f - IupGetFloat(image_view->canvas, "DY") - posy;
    image_view->ViewY = (int)floor(-posy*((float)image_view->ViewHeight + 2 * IMV_MARGIN)) + IMV_MARGIN;
  }
  else
    image_view->ViewY = (image_view->Height - image_view->ViewHeight)/2;

  if (image_view->Width < image_view->ViewWidth)
  {
    image_view->ViewX = (int)floor(-posx*((float)image_view->ViewWidth+2*IMV_MARGIN)) + IMV_MARGIN;
  }
  else
    image_view->ViewX = (image_view->Width - image_view->ViewWidth)/2;
}

static void iViewCalcCenter(imImageView* image_view, float *x, float *y)
{
  *x = IupGetFloat(image_view->canvas, "POSX") + IupGetFloat(image_view->canvas, "DX")/2.0f;
  *y = IupGetFloat(image_view->canvas, "POSY") + IupGetFloat(image_view->canvas, "DY")/2.0f;
}

static void iViewCenterScroll(imImageView* image_view, float old_center_x, float old_center_y)
{
  /* always update the scroll position 
     keeping it proportional to the old position 
     relative to the center of the cd_canvas. */

  float dx = IupGetFloat(image_view->canvas, "DX");
  float dy = IupGetFloat(image_view->canvas, "DY");

  float posx = old_center_x - dx/2.0f;
  float posy = old_center_y - dy/2.0f;

  iViewSetScrollPos(image_view, posx, posy);
}

static int iViewResize(Ihandle* self)
{
  float old_center_x, old_center_y;

  imImageView* image_view = (imImageView*)IupGetAttribute(self, "imImageView");
  if (!image_view)
    return IUP_DEFAULT;

  if (image_view->BitmapView->image == NULL)
  {
    iViewResetScrollSize(image_view);
    iViewResetScrollPos(image_view);
    return IUP_DEFAULT;
  }

  iViewCalcCenter(image_view, &old_center_x, &old_center_y);

  iViewUpdateSize(image_view);

  iViewCenterScroll(image_view, old_center_x, old_center_y);

  if (imImageViewResizeCB)
    return imImageViewResizeCB(image_view);

  return IUP_DEFAULT;
}

void imImageViewZoom(imImageView* image_view, int index)
{
  float old_center_x, old_center_y;

  image_view->ViewMode = IM_VIEW_NORMAL;
  image_view->ZoomIndex = index;
  image_view->ZoomFactor = ((double)ZoomTable1[index + IV_ZOOMZERO]) / ZoomTable2[index + IV_ZOOMZERO];

  iViewCalcCenter(image_view, &old_center_x, &old_center_y);

  iViewUpdateSize(image_view);

  iViewCenterScroll(image_view, old_center_x, old_center_y);

  imImageViewPutImage(image_view);
}

/* Used by the interaction callbacks */
static int iViewCanvas2Image(imImageView* image_view, int *x, int *y)
{
  cdCanvasActivate(image_view->cd_canvas);
  cdCanvasUpdateYAxis(image_view->cd_canvas, y); /* Now we are in CD coordinates */

  /* check if it is inside the view area */
  if (*x < image_view->ViewX || *y < image_view->ViewY || 
      *x >= (image_view->ViewX + image_view->ViewWidth) || *y >= (image_view->ViewY + image_view->ViewHeight))
    return 0;

  *x = (int)((*x - image_view->ViewX) * ((float)image_view->BitmapView->image->width / (float)image_view->ViewWidth));
  *y = (int)((*y - image_view->ViewY) * ((float)image_view->BitmapView->image->height / (float)image_view->ViewHeight));

  /* check for invalid values */

  if (*x < 0) *x = 0;
  if (*y < 0) *y = 0;
  if (*x >= image_view->BitmapView->image->width) *x = image_view->BitmapView->image->width-1;
  if (*y >= image_view->BitmapView->image->height) *y = image_view->BitmapView->image->height-1;

  return 1;
}

static void iViewMoveImage(imImageView* image_view, int dx, int dy)
{
  float posy = 0;
  float posx = 0;

  if (dx == 0 && dy == 0)
    return;

  if (image_view->Height < image_view->ViewHeight)
  {
    float yfactor = (float)image_view->ViewHeight+2*IMV_MARGIN;
    posy = IupGetFloat(image_view->canvas, "POSY");
    posy -= (float)dy/yfactor;
  }

  if (image_view->Width < image_view->ViewWidth)
  {
    float xfactor = (float)image_view->ViewWidth+2*IMV_MARGIN;
    posx = IupGetFloat(image_view->canvas, "POSX");
    posx -= (float)dx/xfactor;
  }

  if (posx != 0 || posy != 0)
  {
    iViewSetScrollPos(image_view, posx, posy);
    imImageViewPutImage(image_view);
  }
}

static int iViewMotion(Ihandle* self, int x, int y, char* r)
{
  imImageView* image_view = (imImageView*)IupGetAttribute(self, "imImageView");
  if (!image_view || !image_view->BitmapView->image)
    return IUP_DEFAULT;

  if (isbutton2(r) && image_view->drag_x != 0xFFFF)
  {
    iViewMoveImage(image_view, x-image_view->drag_x, y-image_view->drag_y);
    image_view->drag_x = x;
    image_view->drag_y = y;
  }
  else if (imImageViewMouseCB)
  {
    if (!iViewCanvas2Image(image_view, &x, &y))
      return imImageViewMouseCB(image_view, -1, -1, IM_VIEW_MOUSEMOVE);

    return imImageViewMouseCB(image_view, x, y, IM_VIEW_MOUSEMOVE);
  }

  return IUP_DEFAULT;
}

static int iViewButton(Ihandle* self, int b, int e, int x, int y)
{
  imImageView* image_view = (imImageView*)IupGetAttribute(self, "imImageView");
  if (!image_view || !image_view->BitmapView->image)
    return IUP_DEFAULT;

  if (imImageViewMouseCB && b == IUP_BUTTON1)
  {
    int mode = (e==1)? IM_VIEW_MOUSEDOWN: IM_VIEW_MOUSEUP;

    if (!iViewCanvas2Image(image_view, &x, &y))
      return imImageViewMouseCB(image_view, -1, -1, mode);

    return imImageViewMouseCB(image_view, x, y, mode);
  }

  image_view->drag_x = 0xFFFF;
  image_view->drag_y = 0xFFFF;
  if (b == IUP_BUTTON2 && e == 1)
  {
    image_view->drag_x = x;
    image_view->drag_y = y;
  }

  return IUP_DEFAULT;
}

static int iViewRepaint(Ihandle* self)
{
  imImageView* image_view = (imImageView*)IupGetAttribute(self, "imImageView");
  if (!image_view || !image_view->BitmapView->image || !image_view->repaint)
    return IUP_DEFAULT;

  if (image_view->ViewMode == IM_VIEW_NORMAL)
    iViewUpdatePos(image_view);

  /* Activates the graphics cd_canvas */                          
  cdCanvasActivate(image_view->cd_canvas);

  /* white backgroud around the view area */
  /* Checks if there are margins between cd_canvas border and image border */
  cdCanvasForeground(image_view->cd_canvas, CD_WHITE);
  cdCanvasClear(image_view->cd_canvas);

  /* black line around the image */
  cdCanvasForeground(image_view->cd_canvas, CD_BLACK);
  cdCanvasBegin(image_view->cd_canvas, CD_CLOSED_LINES);
  cdCanvasVertex(image_view->cd_canvas, image_view->ViewX - 1, image_view->ViewY - 1);
  cdCanvasVertex(image_view->cd_canvas, image_view->ViewX + image_view->ViewWidth, image_view->ViewY - 1);
  cdCanvasVertex(image_view->cd_canvas, image_view->ViewX + image_view->ViewWidth, image_view->ViewY + image_view->ViewHeight);
  cdCanvasVertex(image_view->cd_canvas, image_view->ViewX - 1, image_view->ViewY + image_view->ViewHeight);
  cdCanvasEnd(image_view->cd_canvas);

  imImageViewDrawImage(image_view->cd_canvas, image_view->BitmapView->image, image_view->ViewX, image_view->ViewY, image_view->ViewWidth, image_view->ViewHeight);

  if (imImageViewRepaintCB)
    return imImageViewRepaintCB(image_view);
  else
    cdCanvasFlush(image_view->cd_canvas);

  return IUP_DEFAULT;
}

int imBitmapViewUpdateImage(imBitmapView* BitmapView, imImage* image)
{
  int update_size = 0;

  /* Image was changed somehow, must update its bitmap view */

  if (!BitmapView->image || image->width  != BitmapView->image->width || 
                            image->height != BitmapView->image->height)
    update_size = 1;

  /* the previous was bitmap, can not reuse it */
  if (BitmapView->image && !BitmapView->cnv2bitmap)
      BitmapView->image = NULL;

  /* the previous was not bitmap and this one is bitmap */
  if (BitmapView->image && BitmapView->cnv2bitmap && imImageIsBitmap(image))
  {
    imImageDestroy(BitmapView->image);
    BitmapView->image = NULL;
  }

  /* if the previous was a converted and this one too, we will reuse the bitmap */

  if (!imImageIsBitmap(image))
  {
    if (!BitmapView->image || BitmapView->image->width != image->width ||
                              BitmapView->image->height != image->height ||
         BitmapView->image->color_space != imColorModeToBitmap(image->color_space))
    {
      if (BitmapView->image && BitmapView->cnv2bitmap)
        imImageDestroy(BitmapView->image);

      BitmapView->image = imImageCreateBased(image, -1, -1, imColorModeToBitmap(image->color_space), IM_BYTE);
    }

    imProcessConvertToBitmap(image, BitmapView->image, BitmapView->cpx2real, BitmapView->gamma, BitmapView->absolute, BitmapView->cast_mode);
    BitmapView->cnv2bitmap = 1;

    if (BitmapView->image->color_space == IM_GRAY)
      imImageSetPalette(BitmapView->image, imPaletteDuplicate(image->palette, 256), 256);
  }
  else
  {
    BitmapView->image = image;
    BitmapView->cnv2bitmap = 0;
  }

  return update_size;
}

void imImageViewChangeImage(imImageView* image_view, imImage* image)
{ 
  int update_size = imBitmapViewUpdateImage((imBitmapView*)image_view->BitmapView, image);
  if (update_size)
  {
    iViewUpdateSize(image_view);
    iViewResetScrollPos(image_view);
  }
}

void imBitmapViewInit(imBitmapView* BitmapView)
{
  BitmapView->image = NULL;
  BitmapView->cpx2real = IM_CPX_MAG;
  BitmapView->cnv2bitmap = 0;
  BitmapView->gamma = IM_GAMMA_LINEAR;
  BitmapView->absolute = 0;
  BitmapView->cast_mode = IM_CAST_MINMAX;
}

imImageView* imImageViewCreate(Ihandle* canvas, const imBitmapView* BitmapView)
{
  imImageView* image_view = (imImageView*)malloc(sizeof(imImageView));
  if (!image_view)
    return NULL;

  /* Initialize the graphics library */
  image_view->canvas = canvas;
  image_view->cd_canvas = cdCreateCanvas(CD_IUPDBUFFER, canvas);
  if (!image_view->cd_canvas)
  {
    free(image_view);
    return NULL;
  }

  IupSetAttribute(canvas, "imImageView", (char*)image_view);

  image_view->ViewMode = IM_VIEW_NORMAL;
  image_view->ZoomFactor = 1.;
  image_view->ZoomIndex = 0;
  image_view->BitmapView = BitmapView;
  image_view->ViewX = 0;
  image_view->ViewY = 0;
  image_view->ViewWidth = 0;
  image_view->ViewHeight = 0;
  image_view->repaint = 1;
  image_view->drag_x = 0xFFFF;
  image_view->drag_y = 0xFFFF;

  /* Register the cd_canvas callbacks */
  IupSetCallback(canvas, "ACTION", (Icallback) iViewRepaint);
  IupSetCallback(canvas, "RESIZE_CB", (Icallback)iViewResize);
  IupSetCallback(canvas, "MOTION_CB", (Icallback)iViewMotion);
  IupSetCallback(canvas, "BUTTON_CB", (Icallback)iViewButton);

  IupSetAttribute(canvas, "BGCOLOR", NULL);  /* after ACTION callback */

  iViewResetScrollSize(image_view);
  iViewResetScrollPos(image_view);

  return image_view;
}

void imBitmapViewRelease(imBitmapView* BitmapView)
{
  if (BitmapView->image && BitmapView->cnv2bitmap)
    imImageDestroy(BitmapView->image);
}

void imImageViewDestroy(imImageView* image_view)
{
  /* Terminates the graphics library */
  cdKillCanvas(image_view->cd_canvas);
  free(image_view);
}

int imImageViewPutImage(imImageView* image_view)
{
  return iViewRepaint(image_view->canvas);
}

void imImageViewSetCallbacks(_imImageViewRepaintCB repaint, _imImageViewResizeCB resize, _imImageViewMouseCB mouse)
{
  imImageViewRepaintCB = repaint;
  imImageViewResizeCB = resize;
  imImageViewMouseCB = mouse;
}

void imImageViewZoomIn(imImageView* image_view)
{
  if (image_view->ZoomIndex+IV_ZOOMZERO==IV_ZOOMMAX-1)
    return;

  image_view->ZoomIndex++;
  imImageViewZoom(image_view, image_view->ZoomIndex);
}

void imImageViewZoomOut(imImageView* image_view)
{
  if (image_view->ZoomIndex+IV_ZOOMZERO==0)
    return;

  image_view->ZoomIndex--;
  imImageViewZoom(image_view, image_view->ZoomIndex);
}

int imImageViewZoomPercent(imImageView* image_view)
{
  return ZoomTable3[image_view->ZoomIndex + IV_ZOOMZERO];
}

int imImageViewZoomIndex1(imImageView* image_view)
{
  return ZoomTable1[image_view->ZoomIndex + IV_ZOOMZERO];
}

int imImageViewZoomIndex2(imImageView* image_view)
{
  return ZoomTable2[image_view->ZoomIndex + IV_ZOOMZERO];
}

int imImageViewMode(imImageView* image_view)
{
  return image_view->ViewMode;
}

void imImageViewFit(imImageView* image_view)
{
  image_view->ViewMode = IM_VIEW_FIT;

  iViewUpdateSize(image_view);

  iViewResetScrollPos(image_view);

  imImageViewPutImage(image_view);
}


/*****************************************************************/
/*****************************************************************/


void imImageViewFitRect(int Width, int Height, int wi, int hi, int *w, int *h)
{
  double rView, rImage;
  int correct = 0;

  *w = Width;
  *h = Height;

  rView = ((double)Height) / Width;
  rImage = ((double)hi) / wi;

  if ((rView <= 1 && rImage <= 1) || (rView >= 1 && rImage >= 1)) /* view and image are horizontal rectangles */
  {
    if (rView > rImage)
      correct = 2;
    else
      correct = 1;
  }
  else if (rView < 1 && rImage > 1) /* view is a horizontal rectangle and image is a vertical rectangle */
    correct = 1;
  else if (rView > 1 && rImage < 1) /* view is a vertical rectangle and image is a horizontal rectangle */
    correct = 2;

  if (correct == 1)
    *w = (int)(Height / rImage);
  else if (correct == 2)
    *h = (int)(Width * rImage);
}

void imImageViewDrawImage(cdCanvas* cd_canvas, imImage* image, int x, int y, int w, int h)
{
  if (image->color_space != IM_RGB)
  {
    cdCanvasPalette(cd_canvas, image->palette_count, image->palette, CD_FORCE);
    cdCanvasPutImageRectMap(cd_canvas, image->width, image->height, (unsigned char*)image->data[0], image->palette, x, y, w, h, 0, 0, 0, 0);
  }
  else
    cdCanvasPutImageRectRGB(cd_canvas, image->width, image->height, (unsigned char*)image->data[0],
                                               (unsigned char*)image->data[1], 
                                               (unsigned char*)image->data[2], 
                                               x, y, w, h, 0, 0, 0, 0);
}

int imImageViewExportImage(void* driver, imImage* image, const char* filename)
{
  cdCanvas* cd_canvas;
  char DataStr[256];

  sprintf(DataStr, "%s %dx%d", filename, image->width, image->height);

  /* Activates the graphics cd_canvas */
  cd_canvas = cdCreateCanvas((cdContext*)driver, DataStr);
  if (cd_canvas == NULL)
  {
    IupMessage("Error!", "Error creating file.");
    return 0;
  }

  cdCanvasActivate(cd_canvas);
  imImageViewDrawImage(cd_canvas, image, 0, 0, image->width, image->height);
  cdKillCanvas(cd_canvas);

  return 1;
}

static int new_w = 0;
static int new_h = 0;

static int cbSize(cdContext* driver, int w, int h, double w_mm, double h_mm)
{
  (void)driver;(void)w_mm;(void)h_mm;
  new_w = w;
  new_h = h;
  return 1;
}

imImage* imImageViewImportImage(void* driver, void* data)
{
  imImage* NewImage;
  cdCanvas* imgCanvas;
  char Text[256];
  
  cdContextRegisterCallback((cdContext*)driver, CD_SIZECB, (cdCallback)cbSize);

  imgCanvas = cdCreateCanvas(CD_PICTURE, NULL);
  cdCanvasPlay(imgCanvas, (cdContext*)driver, 0, 0, 0, 0, data);
  cdKillCanvas(imgCanvas);

  if (new_h == 0 || new_w == 0)
  {
    new_w = 0;
    new_h = 0;
    IupMessage("Error!", "Error importing data.");
    return NULL;
  }

  /* creates the new image */
  NewImage = imImageCreate(new_w, new_h, IM_RGB, IM_BYTE);
  if (NewImage == NULL)
  {
    new_w = 0;
    new_h = 0;
    IupMessage("Error!", "Insufficient Memory.");
    return NULL;
  }

  cdContextRegisterCallback((cdContext*)driver, CD_SIZECB, NULL);

  sprintf(Text, "%dx%d %p %p %p", NewImage->width, NewImage->height, NewImage->data[0], NewImage->data[1], NewImage->data[2]);

  /* Activates the graphics cd_canvas */
  imgCanvas = cdCreateCanvas(CD_IMAGERGB, Text);
  if (imgCanvas == NULL)
  {
    new_w = 0;
    new_h = 0;
    imImageDestroy(NewImage);
    IupMessage("Error!", "Invalid data.");
    return NULL;
  }

  /* plays the cd_canvas contents on the image */
  cdCanvasActivate(imgCanvas);
  cdCanvasPlay(imgCanvas, (cdContext*)driver, 0, 0, 0, 0, data);
  cdKillCanvas(imgCanvas);

  /* resets the global variables */
  new_w = 0;
  new_h = 0;

  return NewImage;
}

int imImageViewData2Str(char* str, const void* data, int index, int data_type, int more_prec)
{
  const char* real_format = "%.2f";
  const char* complex_format = "%.2f %.2f";

  if (more_prec)
  {
    real_format = "%.6f";
    complex_format = "%.6f %.6f";
  }

  switch (data_type)
  {
  case IM_BYTE:
    return sprintf(str, "%d", (int)(*((imbyte*)data + index)));
  case IM_SHORT:
    return sprintf(str, "%d", (int)(*((short*)data + index)));
  case IM_USHORT:
    return sprintf(str, "%d", (int)(*((imushort*)data + index)));
  case IM_INT:
    return sprintf(str, "%d", *((int*)data + index));
  case IM_FLOAT:
    return sprintf(str, real_format, (double)(*((float*)data + index)));
  case IM_CFLOAT:
    {
      float *cdata = (float*)data + 2 * index;
      return sprintf(str, complex_format, (double)*cdata, (double)*(cdata + 1));
    }
  case IM_DOUBLE:
    return sprintf(str, real_format, *((double*)data + index));
  case IM_CDOUBLE:
    {
      double *cdata = (double*)data + 2 * index;
      return sprintf(str, complex_format, *cdata, *(cdata + 1));
    }
  }
  return 0;
}

int imImageViewStr2Data(const char* str, void* data, int index, int data_type)
{
  float float_value1 = 0, float_value2 = 0;
  double double_value1 = 0, double_value2 = 0;
  int int_value = 0;

  switch (data_type)
  {
  case IM_BYTE:
  case IM_SHORT:
  case IM_USHORT:
  case IM_INT:
    sscanf(str, "%d", &int_value);
    break;
  case IM_FLOAT:
    sscanf(str, "%f", &float_value1);
    break;
  case IM_CFLOAT:
    sscanf(str, "%f", &float_value1);
    sscanf(str, "%f", &float_value2);
    break;
  case IM_DOUBLE:
    sscanf(str, "%lf", &double_value1);
    break;
  case IM_CDOUBLE:
    sscanf(str, "%lf", &double_value1);
    sscanf(str, "%lf", &double_value2);
    break;
  }

  switch (data_type)
  {
  case IM_BYTE:
    *((imbyte*)data + index) = (imbyte)int_value;
    break;
  case IM_SHORT:
    *((short*)data + index) = (short)int_value;
    break;
  case IM_USHORT:
    *((imushort*)data + index) = (imushort)int_value;
    break;
  case IM_INT:
    *((int*)data + index) = int_value;
    break;
  case IM_FLOAT:
    *((float*)data + index) = float_value1;
    break;
  case IM_CFLOAT:
    *((float*)data + 2 * index) = float_value1;
    *((float*)data + 2 * index + 1) = float_value2;
    break;
  case IM_DOUBLE:
    *((double*)data + index) = double_value1;
    break;
  case IM_CDOUBLE:
    *((double*)data + 2 * index) = double_value1;
    *((double*)data + 2 * index + 1) = double_value2;
    break;
  }

  {
    int length = 0;
    while (*str != 0)
    {
      if (*str == ' ')
        break;
      length++;
      str++;
    }
    length++;

    return length;
  }
}

double imDataGetDouble(const void* data, int index, int data_type)
{
  switch (data_type)
  {
  case IM_BYTE:
    return (int)(*((imbyte*)data + index));
  case IM_SHORT:
    return (int)(*((short*)data + index));
  case IM_USHORT:
    return (int)(*((imushort*)data + index));
  case IM_INT:
    return *((int*)data + index);
  case IM_FLOAT:
    return (double)(*((float*)data + index));
  case IM_CFLOAT:
    return 0;
  case IM_DOUBLE:
    return *((double*)data + index);
  case IM_CDOUBLE:
    return 0;
  }
  return 0;
}

void imDataSetDouble(const void* data, int index, int data_type, double value)
{
  switch (data_type)
  {
  case IM_BYTE:
    *((imbyte*)data + index) = (imbyte)value;
  case IM_SHORT:
    *((short*)data + index) = (short)value;
  case IM_USHORT:
    *((imushort*)data + index) = (imushort)value;
  case IM_INT:
    *((int*)data + index) = (int)value;
  case IM_FLOAT:
    *((float*)data + index) = (float)value;
  case IM_DOUBLE:
    *((double*)data + index) = value;
  }
}

/** \file
 * \brief Image View using CD and IUP
 */

#ifndef __IMAGEVIEW_H
#define __IMAGEVIEW_H

#include <cd.h>
#include <iup.h>
#include <im.h>
#include <im_image.h>

/** \defgroup view Visualization Utilities
 * Uses the CD library to draw the image into a IupCanvas. 
 * The view can be zoomed and scrolled.
 */

#if	defined(__cplusplus)
extern "C" {
#endif

typedef struct _imBitmapView
{
  imImage* image;
  int cnv2bitmap; /* image is a new bitmap converted image */

  int cpx2real;
  double gamma;
  int absolute;
  int cast_mode;
} imBitmapView;

void imBitmapViewInit(imBitmapView* BitmapView);
int imBitmapViewUpdateImage(imBitmapView* BitmapView, imImage* image);
void imBitmapViewRelease(imBitmapView* BitmapView);


/** Image View Canvas
 * \ingroup view */
typedef struct _imImageView
{
  cdCanvas* cd_canvas;
  Ihandle* canvas;

  const imBitmapView* BitmapView;

  /* CD cd_canvas size */
  int Width;
  int Height;

  int ViewMode;
  double ZoomFactor;
  int ZoomIndex;
  int repaint;
  int drag_x, drag_y;

  /* CD coordinates of the view area inside the cd_canvas,
     it is centered in the cd_canvas and 
     proportional to the image dimensions multiplied by the zoom.
     It is exactly where the PutImage should happen */
  int ViewX;
  int ViewY;
  int ViewWidth;
  int ViewHeight;
} imImageView;
                   
/** Creates the view.
 * \ingroup view */
imImageView* imImageViewCreate(Ihandle* canvas, const imBitmapView* BitmapView);
                   
/** Destroys the view.
 * \ingroup view */
void imImageViewDestroy(imImageView* image_view);
                   
/** Changes the image and updates the view.
 * \ingroup view */
void imImageViewChangeImage(imImageView* image_view, imImage* image);
                   
/** View Modes.
 * \ingroup view */
enum {IM_VIEW_NORMAL, IM_VIEW_FIT};
                   
/** Returns the view mode.
 * \ingroup view */
int imImageViewMode(imImageView* image_view);
                   
/** Changes the view mode and updates the view.
 * \ingroup view */
void imImageViewFit(imImageView* image_view);
                   
/** Changes the zoom and updates the view in normal mode.
 * \ingroup view */
void imImageViewZoom(imImageView* image_view,int index);
                   
/** Increses the zoom and updates the view.
 * \ingroup view */
void imImageViewZoomIn(imImageView* image_view);
                   
/** Decreses the zoom and updates the view.
 * \ingroup view */
void imImageViewZoomOut(imImageView* image_view);
                   
/** Returns the zoom index1 factor where zoom is index1:index2.
 * \ingroup view */
int imImageViewZoomIndex1(imImageView* image_view);
                   
/** Returns the zoom index2 factor where zoom is index1:index2.
 * \ingroup view */
int imImageViewZoomIndex2(imImageView* image_view);
                   
/** Returns the zoom percentage.
 * \ingroup view */
int imImageViewZoomPercent(imImageView* image_view);
                   
/** Repaint the view.
 * \ingroup view */
int imImageViewPutImage(imImageView* image_view);
                   
/** Mouse Modes.
 * \ingroup view */
enum {IM_VIEW_MOUSEMOVE, IM_VIEW_MOUSEDOWN, IM_VIEW_MOUSEUP};
                   
/** Mouse Callback.
 * \ingroup view */
typedef int (*_imImageViewMouseCB)(imImageView* image_view, int x, int y, int mode);
                   
/** Repaint Callback.
 * \ingroup view */
typedef int (*_imImageViewRepaintCB)(imImageView* image_view);
                   
/** Resize Callback.
 * \ingroup view */
typedef int (*_imImageViewResizeCB)(imImageView* image_view);
                   
/** Changes the view callbacks.
 * \ingroup view */
void imImageViewSetCallbacks(_imImageViewRepaintCB repaint, _imImageViewResizeCB resize, _imImageViewMouseCB mouse);
                   
/** Returns the rectangle that fits the image in the center of the view.
 * \ingroup view */
void imImageViewFitRect(int wc, int hc, int wi, int hi, int *w, int *h);
                   
/** Draws an image in the current CD cd_canvas.
 * \ingroup view */
void imImageViewDrawImage(cdCanvas* cd_canvas, imImage* image, int x, int y, int w, int h);
                   
/** Uses the cdPlay function to render an imagem from the file or clipboard.
 * \ingroup view */
imImage* imImageViewImportImage(void* driver, void* data);
                   
/** Used the CD library to export the image to a file or clipboard.
 * \ingroup view */
int imImageViewExportImage(void* driver, imImage* image, const char* filename);

int imImageViewData2Str(char* str, const void* data, int index, int data_type, int more_prec);
int imImageViewStr2Data(const char* str, void* data, int index, int data_type);
double imDataGetDouble(const void* data, int index, int data_type);
void imDataSetDouble(const void* data, int index, int data_type, double value);


#if defined(__cplusplus)
}
#endif

#endif

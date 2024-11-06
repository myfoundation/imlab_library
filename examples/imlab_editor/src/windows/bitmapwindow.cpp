#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <im.h>
#include <im_image.h>
#include <im_convert.h>
#include <im_palette.h>

#include "bitmapwindow.h"
#include "imlab.h"
#include "dialogs.h"
#include "statusbar.h"
#include "im_imagematch.h"



static void iBitmapUpdateViewLabel(imlabBitmapWindow* bitmap_win)
{
  char view_str[200] = "";
  Ihandle* ToolBar = (Ihandle*)IupGetAttribute(bitmap_win->dialog, "ToolBar");
  Ihandle *ViewLabel = (Ihandle*)IupGetAttribute(ToolBar, "ViewLabel"); 
  int cast_mode = bitmap_win->document->BitmapView.cast_mode;
  imImage* image = bitmap_win->document->ImageFile->image;

  switch (bitmap_win->image_view->ViewMode)
  {
  case IM_VIEW_FIT:
    strcat(view_str, "Fit");
    break;
  default:
    {
      int z1 = imImageViewZoomIndex1(bitmap_win->image_view);
      int z2 = imImageViewZoomIndex2(bitmap_win->image_view);
      int p = imImageViewZoomPercent(bitmap_win->image_view);

      if (z1 == 1 && z2 == 1)
        strcat(view_str, "No Zoom - 1:1 (100%)");
      else
      {
        char str[50];
        sprintf(str, "Zoom - %d:%d (%d%%)", z1, z2, p);
        strcat(view_str, str);
      }
    }
  }

  if (image && !imImageIsBitmap(image))
  {
    strcat(view_str, " | ");

    if (cast_mode == IM_CAST_MINMAX)
      strcat(view_str, "Scan MinMax");
    else if (cast_mode == IM_CAST_FIXED)
      strcat(view_str, "Fixed MinMax");
    else
      strcat(view_str, "Direct Cast");

    if (bitmap_win->document->BitmapView.absolute)
      strcat(view_str, " (Abss)");

    if (imImageIsRealComplex(image))
    {
      double gamma = bitmap_win->document->BitmapView.gamma;
      strcat(view_str, " | ");

      if (gamma == IM_GAMMA_LINEAR)
        strcat(view_str, "Gamma Linear");
      else if (gamma == IM_GAMMA_LOGLITE)
        strcat(view_str, "Gamma Log(Light)");
      else if (gamma == IM_GAMMA_LOGHEAVY)
        strcat(view_str, "Gamma Log(Heavy)");
      else if (gamma == IM_GAMMA_EXPLITE)
        strcat(view_str, "Gamma Exp(Ligth)");
      else if (gamma == IM_GAMMA_EXPHEAVY)
        strcat(view_str, "Gamma Exp(Heavy)");
      else
        strcat(view_str, "Gamma Custom");
    }

    if (imImageIsComplex(image))
    {
      int cpx2real = bitmap_win->document->BitmapView.cpx2real;
      strcat(view_str, " | ");

      switch (cpx2real)
      {
      case IM_CPX_REAL:
        strcat(view_str, "Cpx Real");
        break;
      case IM_CPX_IMAG:
        strcat(view_str, "Cpx Imaginary");
        break;
      case IM_CPX_MAG:
        strcat(view_str, "Cpx Magnitute");
        break;
      case IM_CPX_PHASE:
        strcat(view_str, "Cpx Phase");
        break;
      }
    }
  }

  IupStoreAttribute(ViewLabel, "TITLE", view_str);
}

static void iBitmapUpdateZoomButtons(imlabBitmapWindow* bitmap_win)
{
  Ihandle* ToolBar = (Ihandle*)IupGetAttribute(bitmap_win->dialog, "ToolBar");
  Ihandle *NormalButton = (Ihandle*)IupGetAttribute(ToolBar, "NormalButton"),
          *ZoomInButton = (Ihandle*)IupGetAttribute(ToolBar, "ZoomInButton"),
          *ZoomOutButton = (Ihandle*)IupGetAttribute(ToolBar, "ZoomOutButton"); 

  switch (bitmap_win->image_view->ViewMode)
  {
  case IM_VIEW_FIT:
    IupSetAttribute(NormalButton, "ACTIVE", "YES");
    IupSetAttribute(ZoomInButton, "ACTIVE", "NO");
    IupSetAttribute(ZoomOutButton, "ACTIVE", "NO");
    break;
  default:
    {
      int z1 = imImageViewZoomIndex1(bitmap_win->image_view);
      int z2 = imImageViewZoomIndex2(bitmap_win->image_view);

      if (z1 == 1 && z2 == 1)
        IupSetAttribute(NormalButton, "ACTIVE", "NO");
      else
        IupSetAttribute(NormalButton, "ACTIVE", "YES");

      if (z1 == 1 && z2 == 64)
        IupSetAttribute(ZoomOutButton, "ACTIVE", "NO");
      else
        IupSetAttribute(ZoomOutButton, "ACTIVE", "YES");

      if (z1 == 64 && z2 == 1)
        IupSetAttribute(ZoomInButton, "ACTIVE", "NO");
      else
        IupSetAttribute(ZoomInButton, "ACTIVE", "YES");
    }
  }

  iBitmapUpdateViewLabel(bitmap_win);
}

static int cmFitToWindow(Ihandle* self)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");

  /* Change the image view to expand to the cd_canvas size but without changing its aspect ratio and repaints the cd_canvas */
  imImageViewFit(bitmap_win->image_view);

  /* Informs the user the current view  */
  iBitmapUpdateZoomButtons(bitmap_win);

  return IUP_DEFAULT;
}

static int cmZoomNormal(Ihandle* self)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");

  /* Change the image view zoom factor and repaints the cd_canvas */
  imImageViewZoom(bitmap_win->image_view, 0);

  /* Informs the user the current view  */
  iBitmapUpdateZoomButtons(bitmap_win);
  return IUP_DEFAULT;
}

static int cmZoomIn(Ihandle* self)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");

  /* Change the image view zoom factor and repaints the cd_canvas */
  imImageViewZoomIn(bitmap_win->image_view);

  /* Informs the user the current view  */
  iBitmapUpdateZoomButtons(bitmap_win);

  return IUP_DEFAULT;
}

static int cmZoomOut(Ihandle* self)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");

  /* Change the image view zoom factor and repaints the cd_canvas */
  imImageViewZoomOut(bitmap_win->image_view);

  /* Informs the user the current view  */
  iBitmapUpdateZoomButtons(bitmap_win);

  return IUP_DEFAULT;
}

static int cbWheel(Ihandle* self, float delta)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");

  if (delta<0)
    imImageViewZoomOut(bitmap_win->image_view);
  else
    imImageViewZoomIn(bitmap_win->image_view);

  /* Informs the user the current view  */
  iBitmapUpdateZoomButtons(bitmap_win);

  return IUP_DEFAULT;
}

static int cmViewMode(Ihandle* self, char* text, int i, int v)
{
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(self, "imlabImageWindow");
  Ihandle* ToolBar = (Ihandle*)IupGetAttribute(bitmap_win->dialog, "ToolBar");
  Ihandle *NormalButton = (Ihandle*)IupGetAttribute(ToolBar, "NormalButton");
  (void)text;

  if (!v) return IUP_DEFAULT;
  
  switch(i)
  {
  case 1:
    return cmZoomNormal(self);
  case 2:
    cmFitToWindow(self);
    IupSetAttribute(NormalButton, "ACTIVE", "NO");
    break;
  }

  return IUP_DEFAULT;
}

static void iBitmapShowImageDesc(imlabBitmapWindow* bitmap_win)
{
  imImage* image = bitmap_win->document->ImageFile->image;

  double size = (double)image->size;
  char* size_desc = imlabDlgGetSizeDesc(&size);

  char buffer[512];
  sprintf(buffer, "[%d x %d] %s%s - %s (%.2f%s)",
    image->width,
    image->height,
    imColorModeSpaceName(image->color_space),
    image->has_alpha ? "A" : "",
    imDataTypeName(image->data_type),
    size,
    size_desc);

  sbDrawMessage(imlabStatusBar(), buffer);
}

static int cbMouse(imImageView* image_view, int x, int y, int mode)
{
  int offset, index, log;
  imlabBitmapWindow* bitmap_win = (imlabBitmapWindow*)IupGetAttribute(image_view->canvas, "imlabImageWindow");
  imImage *image = bitmap_win->document->ImageFile->image;
  char buffer[512], *str;

  if (x == -1)
  {
    sbClear(imlabStatusBar());
    iBitmapShowImageDesc(bitmap_win);
    return IUP_DEFAULT;
  }

  offset = image->line_size * y + x * imDataTypeSize(image->data_type);

  log = (mode == IM_VIEW_MOUSEDOWN)? 1: 0;

  str = sbDrawXY(imlabStatusBar(), x, y);

  if (log) strcpy(buffer, str);

  switch(image->color_space)
  {
  case IM_MAP:
    index = ((imbyte**)image->data)[0][offset];
    str = sbDrawMap(imlabStatusBar(), image->palette[index], (imbyte)index);
    break;
  case IM_GRAY:
  case IM_BINARY:
    if (image->has_alpha)
      str = sbDrawAB(imlabStatusBar(), &((imbyte**)image->data)[0][offset],
                                       &((imbyte**)image->data)[1][offset],
                                       image->data_type);
    else
      str = sbDrawA(imlabStatusBar(), &((imbyte**)image->data)[0][offset], 
                                      image->data_type);
    break;
  case IM_CMYK:       
    str = sbDrawABCD(imlabStatusBar(), &((imbyte**)image->data)[0][offset], 
                                       &((imbyte**)image->data)[1][offset], 
                                       &((imbyte**)image->data)[2][offset], 
                                       &((imbyte**)image->data)[3][offset], 
                                       image->data_type);
    break;
  default:       
    if (image->has_alpha)
      str = sbDrawABCD(imlabStatusBar(), &((imbyte**)image->data)[0][offset], 
                                         &((imbyte**)image->data)[1][offset], 
                                         &((imbyte**)image->data)[2][offset], 
                                         &((imbyte**)image->data)[3][offset], 
                                         image->data_type);
    else
      str = sbDrawABC(imlabStatusBar(), &((imbyte**)image->data)[0][offset], 
                                        &((imbyte**)image->data)[1][offset], 
                                        &((imbyte**)image->data)[2][offset], 
                                        image->data_type);
    break;
  }

  if (log)
  {
    strcat(buffer, " = ");
    strcat(buffer, str);
    imlabLogMessagef(buffer);
  }

  if (mode == IM_VIEW_MOUSEDOWN)
  {
    int i;
    for (i = 0; i < bitmap_win->document->view_list_count; i++)
    {
      imlabImageWindow* image_window = bitmap_win->document->view_list[i];
      image_window->Sync(x, y);
    }
  }

  return IUP_DEFAULT;
}

static Ihandle* iBitmapCreateToolBar(void)
{
  Ihandle *NormalButton, *ZoomInButton, *ZoomOutButton, 
          *ToolBar, *ViewLabel;

  NormalButton = IupButton(NULL, NULL);
  IupSetAttribute(NormalButton,"TIP","View the image with no zoom.");
  IupSetAttribute(NormalButton,"IMAGE","IUP_ZoomActualSize");
  IupSetCallback(NormalButton, "ACTION", cmZoomNormal);

  ZoomInButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomInButton,"TIP","Increase the image size on screen.");
  IupSetAttribute(ZoomInButton,"IMAGE","IUP_ZoomIn");
  IupSetCallback(ZoomInButton, "ACTION", cmZoomIn);

  ZoomOutButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomOutButton,"TIP","Decrease the image size on screen.");
  IupSetAttribute(ZoomOutButton,"IMAGE","IUP_ZoomOut");
  IupSetCallback(ZoomOutButton, "ACTION", cmZoomOut);

  ViewLabel = IupLabel("1:1 (100%)");
  IupSetAttribute(ViewLabel, "EXPAND", "HORIZONTAL");
  IupSetAttribute(ViewLabel, "FONT", "Helvetica, 8");

  ToolBar = IupHbox(
    NormalButton,
    ZoomInButton,
    ZoomOutButton,
    IupSetAttributes(IupFill(),"SIZE=2"),
    IupSetCallbacks(IupSetAttributes(IupList(NULL), "DROPDOWN=YES, 1=Normal, 2=Fit, VALUE=1"), "ACTION", (Icallback)cmViewMode, NULL),
    IupSetAttributes(IupFill(),"SIZE=2"),
    ViewLabel,
    NULL);

  IupSetAttribute(ToolBar, "NormalButton", (char*)NormalButton);
  IupSetAttribute(ToolBar, "ZoomInButton", (char*)ZoomInButton);
  IupSetAttribute(ToolBar, "ZoomOutButton", (char*)ZoomOutButton);
  IupSetAttribute(ToolBar, "ViewLabel", (char*)ViewLabel);

  IupSetAttribute(ToolBar, "EXPAND", "HORIZONTAL");
  IupSetAttribute(ToolBar, "ALIGNMENT", "ACENTER");
  IupSetAttribute(ToolBar, "FLAT", "YES");
  IupSetAttribute(ToolBar, "MARGIN", "5x5");

  return ToolBar;
}

static void iBitmapCreateDialog(imlabBitmapWindow* bitmap_win)
{
  Ihandle *vbDesktop, *dialog, *ToolBar, *canvas;
  int zoom = 0;

  int width = bitmap_win->document->ImageFile->image->width;
  int height = bitmap_win->document->ImageFile->image->height;

  canvas = IupCanvas(NULL);
  if (height < 300) height = 300;
  if (width < 400) width = 400;

  {
    int screen_width, screen_height;
    IupGetIntInt(NULL, "SCREENSIZE", &screen_width, &screen_height);
    screen_height -= 80;
    while (width > screen_width || height > screen_height)
    {
      width /= 2;
      height /= 2;
      zoom--;

      if (zoom < -4)
        break;
    }
  }

  IupSetfAttribute(canvas,"RASTERSIZE","%dx%d", width+25, height+25);
  IupSetAttribute(canvas,"SCROLLBAR","YES");
  IupSetAttribute(canvas,"CURSOR","CrossCursor");

  ToolBar = iBitmapCreateToolBar();

  vbDesktop = IupVbox(
    ToolBar,
    canvas,
    NULL);

  dialog = IupDialog(vbDesktop);
  bitmap_win->dialog = dialog;
  bitmap_win->canvas = canvas;

  IupSetAttribute(dialog, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(dialog,"ICON", "IMLAB");
  IupSetAttribute(dialog,"SHRINK", "YES");

  IupSetAttribute(dialog, "K_cC", "imlabCopy");
  IupSetAttribute(dialog, "K_cD", "imlabDuplicate");
  IupSetAttribute(dialog, "K_cZ", "imlabUndo");
  IupSetAttribute(dialog, "K_cY", "imlabRedo");
  IupSetAttribute(dialog, "K_cS", "imlabSave");
  IupSetAttribute(dialog, "K_cO", "imlabOpen");
  IupSetAttribute(dialog, "K_cN", "imlabWindowNext");

  IupSetAttribute(dialog, "ToolBar", (char*)ToolBar);

  IupSetAttribute(dialog, "imlabImageWindow", (char*)bitmap_win);
  IupSetAttribute(dialog, "ImageWindowType", "Bitmap");

  IupMap(dialog);

  imImageView* image_view = imImageViewCreate(canvas, &(bitmap_win->document->BitmapView));
  bitmap_win->image_view = image_view;
  IupSetAttribute(dialog, "imImageView", (char*)image_view);  // Used by Preview

  imImageViewSetCallbacks(NULL, NULL, cbMouse);
  IupSetCallback(canvas, "WHEEL_CB", (Icallback)cbWheel);

  imImageViewChangeImage(image_view, bitmap_win->document->ImageFile->image);
  imImageViewZoom(image_view, zoom);

  iBitmapUpdateZoomButtons(bitmap_win);
 
  bitmap_win->SetTitle();
  bitmap_win->SetCallbacks();
  bitmap_win->ShowWindow();

  IupSetAttribute(canvas, "RASTERSIZE",NULL);                                   
}

imlabBitmapWindow::imlabBitmapWindow(imlabImageDocument* _document)
{
  this->document = _document;

  iBitmapCreateDialog(this);
}

imlabBitmapWindow::~imlabBitmapWindow()
{
  imImageViewDestroy(this->image_view);
}

void imlabBitmapWindow::Update()
{
  imImageViewChangeImage(this->image_view, this->document->ImageFile->image);
  imImageViewPutImage(this->image_view);
  iBitmapUpdateViewLabel(this);
}

void imlabBitmapWindow::Refresh()
{
  imImageViewPutImage(this->image_view);
}

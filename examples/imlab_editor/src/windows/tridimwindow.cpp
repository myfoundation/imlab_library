#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#if defined(WIN32)
#include <windows.h>    /* this is necessary only because of the Microsoft OpenGL headers dependency */
#endif

#include <GL/gl.h>
#include <GL/glu.h>

#include <iup.h>
#include <iupgl.h>
#include <cd.h>

#include "tridimwindow.h"
#include "imlab.h"
#include "statusbar.h"



enum {RENDER_POLY, RENDER_POINT};

#define imRed(_)      (unsigned char)( ((_) >> 16) & 0xFF )
#define imGreen(_)    (unsigned char)( ((_) >>  8) & 0xFF )
#define imBlue(_)     (unsigned char)( ((_) >>  0) & 0xFF )

static void iTridimSetDefaultFOV(imlab3DWindow* tridim_win)
{
  tridim_win->fov = 20.0;
  tridim_win->eyex = 0.5;
  tridim_win->eyey = 0.5;
  tridim_win->refx = 0.5;
  tridim_win->refy = 0.5;

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glClearDepth(1.0);
}

static void iTridimSetProjection(imlab3DWindow* tridim_win)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(tridim_win->fov, ((double)tridim_win->width)/((double)tridim_win->height), 1, 10);

  gluLookAt(tridim_win->eyex, tridim_win->eyey, 4.0,  /* observer        */
            tridim_win->refx, tridim_win->refy, 0.0,  /* reference point */
            0.0, 1.0, 0.0);                           /* vup             */
}

static void iTridimUpdateSize(imlab3DWindow* tridim_win, int width, int height)
{
  tridim_win->width = width;
  tridim_win->height = height;

  glViewport(0, 0, tridim_win->width, tridim_win->height);
}

static int cbResize(Ihandle* self, int width, int height)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(self);

  iTridimUpdateSize(tridim_win, width, height);

  iTridimSetProjection(tridim_win);

  return IUP_DEFAULT;
}

static void iTridimGetRGBZ(double *r, double *g, double *b, double *z, 
                     imbyte* data0, imbyte* data1, imbyte* data2,
                     int offset, int color_space, long* palette)
{
  if (color_space == IM_RGB)
  {
    *r = data0[offset];
    *g = data1[offset];
    *b = data2[offset];
  }
  else if (color_space == IM_GRAY)
  {
    *r = *g = *b = data0[offset];
  }
  else /* IM_MAP or IM_BINARY */
  {
    long c = palette[data0[offset]];
    *r = imRed(c);
    *g = imGreen(c);
    *b = imBlue(c);
  }

  *r /= 255.;
  *g /= 255.;
  *b /= 255.;
  *z = (*r + *g + *b)/3.;
}

static void iTridimDrawImagePoly(imImage* bitmap_image)
{
  int x, y, offset;
  double xr, yr, zr, r, g, b, yr1;
  imbyte *data0, *data1=NULL, *data2=NULL;

  int color_space = bitmap_image->color_space;
  long* palette = bitmap_image->palette;
  int width = bitmap_image->width;
  int height = bitmap_image->height;

  if (bitmap_image->color_space == IM_RGB)
  {
    data0 = (imbyte*)bitmap_image->data[0];
    data1 = (imbyte*)bitmap_image->data[1];
    data2 = (imbyte*)bitmap_image->data[2];
  }
  else
    data0 = (imbyte*)bitmap_image->data[0];

  for (y = 0; y < height-1; y++)
  {
    offset = y*width;

    yr = (double)y/(double)(height-1);
    yr1 = (double)(y+1)/(double)(height-1);

    glBegin(GL_TRIANGLE_STRIP);

    for (x = 0; x < width; x++)
    {
      xr = (double)x/(double)(width-1);

      iTridimGetRGBZ(&r, &g, &b, &zr, 
               data0, data1, data2,
               offset+x, color_space, palette);
      glColor3d(r, g, b);
      glVertex3d(xr, yr, zr);

      iTridimGetRGBZ(&r, &g, &b, &zr, 
               data0, data1, data2,
               offset+x+width, color_space, palette);
      glColor3d(r, g, b);
      glVertex3d(xr, yr1, zr);
    }

    glEnd();
  }
}

static void iTridimDrawImagePoints(imImage* bitmap_image)
{
  int x, y, offset;
  double xr, yr, zr, r, g, b;
  imbyte *data0, *data1=NULL, *data2=NULL;

  int color_space = bitmap_image->color_space;
  long* palette = bitmap_image->palette;
  int width = bitmap_image->width;
  int height = bitmap_image->height;

  if (bitmap_image->color_space == IM_RGB)
  {
    data0 = (imbyte*)bitmap_image->data[0];
    data1 = (imbyte*)bitmap_image->data[1];
    data2 = (imbyte*)bitmap_image->data[2];
  }
  else
    data0 = (imbyte*)bitmap_image->data[0];

  glBegin(GL_POINTS);

  for (y = 0; y < height; y++)
  {
    offset = y*width;

    yr = (double)y/(double)(height-1);

    for (x = 0; x < width; x++)
    {
      xr = (double)x/(double)(width-1);

      iTridimGetRGBZ(&r, &g, &b, &zr, 
               data0, data1, data2,
               offset+x, color_space, palette);
      glColor3d(r, g, b);
      glVertex3d(xr, yr, zr);
    }
  }

  glEnd();
}

static void iTridimRepaint(imlab3DWindow* tridim_win)
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);  /* clears the back buffer */

  {
    imImage* bitmap_image = tridim_win->document->BitmapView.image;

    if (tridim_win->render_type == RENDER_POINT)
      iTridimDrawImagePoints(bitmap_image);
    else
      iTridimDrawImagePoly(bitmap_image);
  }

  glFlush(); 
  IupGLSwapBuffers(tridim_win->canvas);        /* swap data from back buffer to front buffer */
}

static int cbRepaint(Ihandle* self)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(self);        /* activates this GL Canvas as the current drawing area. */

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static void iTridimUnproject (double x2, double y2, double *x3, double *y3, double *z3)
{
  double mv[16];
  double pm[16];
  int    vp[4];

  glGetDoublev (GL_MODELVIEW_MATRIX,  mv);
  glGetDoublev (GL_PROJECTION_MATRIX, pm);
  glGetIntegerv (GL_VIEWPORT, vp);
  gluUnProject (x2, y2, 0.0,
                mv, pm, vp,
                x3, y3, z3);
}

static int cbWheel(Ihandle* self, float delta)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(self);

  /* zoom only */
  tridim_win->fov -= delta;
  iTridimSetProjection(tridim_win);

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static int cmZoomIn(Ihandle* self)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(self);

  /* zoom only */
  tridim_win->fov -= 1;
  iTridimSetProjection(tridim_win);

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static int cmZoomOut(Ihandle* self)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(self);

  /* zoom only */
  tridim_win->fov += 1;
  iTridimSetProjection(tridim_win);

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static int cbMotion(Ihandle* self, int x, int y, char *state)
{
  double dif_x, dif_y;
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;
  if (!isbutton1(state)) return IUP_DEFAULT;

  IupGLMakeCurrent(self);

  dif_x = x - tridim_win->pos_x;
  tridim_win->pos_x = x;
  dif_y = tridim_win->height-1-y - tridim_win->pos_y;
  tridim_win->pos_y = tridim_win->height-1-y;
  
  if (dif_x == 0 && dif_y == 0)
    return IUP_DEFAULT;

  if (isshift(state) && iscontrol(state)) // rotate plane
  {
    glMatrixMode(GL_MODELVIEW);
    glTranslated(0.5, 0.5, 0.5);
    glRotated(dif_y + dif_x, 0, 0, 1);
    glTranslated(-0.5, -0.5, -0.5);
  }
  else if (isshift(state)) // pan
  {
    tridim_win->eyex -= dif_x/(double)tridim_win->height;
    tridim_win->eyey -= dif_y/(double)tridim_win->height;
    tridim_win->refx -= dif_x/(double)tridim_win->height;
    tridim_win->refy -= dif_y/(double)tridim_win->height;

    iTridimSetProjection(tridim_win);
  }
  else if (iscontrol(state)) // zoom
  {
    iTridimSetProjection(tridim_win);

    tridim_win->fov -= (dif_y + dif_x) / 10;

    iTridimSetProjection(tridim_win);
  }
  else // rotate
  {
    double xr, yr, zr;
    double x1, y1, z1;
    double x2, y2, z2;
    double angle, norma;

    angle = sqrt(dif_x*dif_x + dif_y*dif_y);

    iTridimUnproject (tridim_win->pos_x, tridim_win->pos_y, &x1, &y1, &z1);
    iTridimUnproject ((double)(-dif_y+tridim_win->pos_x), (double)(dif_x+tridim_win->pos_y), &x2, &y2, &z2);
    xr = x2-x1; yr = y2-y1; zr = z2-z1;
    norma = sqrt(xr*xr + yr*yr + zr*zr);
    xr /= norma; yr /= norma; zr /= norma;

    glMatrixMode(GL_MODELVIEW);
    glTranslated(0.5, 0.5, 0.5);
    glRotated (angle, xr, yr, zr);
    glTranslated(-0.5, -0.5, -0.5);
  }

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static int cbButton(Ihandle* self, int but, int pressed, int x, int y, char *state)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  if (but == IUP_BUTTON1 && pressed)
  {
    tridim_win->pos_x = x;
    tridim_win->pos_y = tridim_win->height-1-y;

    if (iup_isdouble(state)) // reset
    {
      IupGLMakeCurrent(tridim_win->canvas);

      iTridimSetDefaultFOV(tridim_win);

      iTridimSetProjection(tridim_win);

      iTridimRepaint(tridim_win);
    }
  }

  return IUP_DEFAULT;
}

static int cmRenderType(Ihandle* self, char *t, int i, int v)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;
  (void)t;

  if (v)
  {
    tridim_win->render_type = i-1;
    IupGLMakeCurrent(tridim_win->canvas);
    iTridimRepaint(tridim_win);
  }

  return IUP_DEFAULT;
}

static int cmResetView(Ihandle* self)
{
  imlab3DWindow* tridim_win = (imlab3DWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!tridim_win) return IUP_DEFAULT;

  IupGLMakeCurrent(tridim_win->canvas);

  iTridimSetDefaultFOV(tridim_win);

  iTridimSetProjection(tridim_win);

  iTridimRepaint(tridim_win);

  return IUP_DEFAULT;
}

static Ihandle* iTridimCreateToolBar(void)
{
  Ihandle *NormalButton, *ZoomOutButton, *ZoomInButton, *tool_bar, *RenderTypeList;

  NormalButton = IupButton(NULL, NULL);
  IupSetAttribute(NormalButton, "TIP", "Reset View Parameters.\n Use double click to reset.");
  IupSetAttribute(NormalButton, "IMAGE", "IUP_ZoomActualSize");
  IupSetAttribute(NormalButton, "EXPAND", "NO");
  IupSetCallback(NormalButton, "ACTION", (Icallback)cmResetView);

  ZoomInButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomInButton, "TIP", "Increase the view size on screen.\n Use Ctrl+mouse drag or Wheel");
  IupSetAttribute(ZoomInButton, "IMAGE", "IUP_ZoomIn");
  IupSetAttribute(ZoomInButton, "EXPAND", "NO");
  IupSetCallback(ZoomInButton, "ACTION", (Icallback)cmZoomIn);

  ZoomOutButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomOutButton, "TIP", "Decrease the view size on screen.\n Use Ctrl+mouse drag or Wheel");
  IupSetAttribute(ZoomOutButton, "IMAGE", "IUP_ZoomOut");
  IupSetAttribute(ZoomOutButton, "EXPAND", "NO");
  IupSetCallback(ZoomOutButton, "ACTION", (Icallback)cmZoomOut);

  RenderTypeList = IupList(NULL);
  IupSetAttributes(RenderTypeList, "DROPDOWN=YES, 1=Polygons, 2=Points, VALUE=1");
  IupSetCallback(RenderTypeList, "ACTION", (Icallback)cmRenderType);

  tool_bar = IupHbox(
    NormalButton,
    ZoomInButton,
    ZoomOutButton,
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupLabel("Render:"),
    RenderTypeList,
    NULL);

  IupSetAttribute(tool_bar, "ALIGNMENT", "ACENTER");
  IupSetAttribute(tool_bar, "FLAT", "YES");
  IupSetAttribute(tool_bar, "GAP", "5");
  IupSetAttribute(tool_bar, "MARGIN", "5x5");

  return tool_bar;
}

static void iTridimCreateDialog(imlab3DWindow* tridim_win)
{
  Ihandle *vbDesktop, *dialog, *tool_bar, *canvas, *HelpLabel;
  imImage* bitmap_image = tridim_win->document->BitmapView.image;

  int width = bitmap_image->width;
  int height = bitmap_image->height;

  if (height < 300) height = 300;
  if (width < 400) width = 400;

  {
    int screen_width, screen_height;
    int zoom = 0;
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

  canvas = IupGLCanvas(NULL);
  IupSetAttribute(canvas, "BUFFER", "DOUBLE");   /* use double buffer */
  IupSetfAttribute(canvas,"RASTERSIZE","%dx%d", width+25, height+25);
  IupSetAttribute(canvas, "DEPTH_SIZE", "16");

  IupSetCallback(canvas, "ACTION", (Icallback) cbRepaint);
  IupSetCallback(canvas, "RESIZE_CB", (Icallback) cbResize);
  IupSetCallback(canvas, "MOTION_CB", (Icallback) cbMotion);
  IupSetCallback(canvas, "BUTTON_CB", (Icallback) cbButton);
  IupSetCallback(canvas, "WHEEL_CB", (Icallback) cbWheel);

  HelpLabel = IupLabel("mouse drag=Rotate, Shift+mouse drag=Pan, double click=Reset,\nCtrl+Shift+mouse drag=Rotate Plane, Ctrl+mouse drag or Wheel=Zoom");
  IupSetAttribute(HelpLabel, "EXPAND", "HORIZONTAL");
  IupSetAttribute(HelpLabel, "FONT", "Helvetica, 8");
  IupSetAttribute(HelpLabel, "ALIGNMENT", "ACENTER");

  tool_bar = iTridimCreateToolBar();

  vbDesktop = IupVbox(
    IupSetAttributes(IupFill(),"SIZE=2, EXPAND=NO"),
    tool_bar,
    IupSetAttributes(IupVbox(
      canvas,
      HelpLabel,
      NULL),"MARGIN=4x4, GAP=2"),
    NULL);

  dialog = IupDialog(vbDesktop);
  tridim_win->dialog = dialog;
  tridim_win->canvas = canvas;

  IupSetAttribute(dialog, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(dialog,"ICON", "IMLAB");
  IupSetAttribute(dialog, "SHRINK", "YES");

  IupSetAttribute(dialog, "imlabImageWindow", (char*)tridim_win);
  IupSetAttribute(dialog, "ImageWindowType", "3D");

  IupMap(dialog);

  IupGLMakeCurrent(canvas);

  glClearColor(1.0, 1.0, 1.0, 1.0);         /* window background */
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);    /* data alignment is 1 */

  iTridimSetDefaultFOV(tridim_win);

  iTridimUpdateSize(tridim_win, IupGetInt(canvas, "RASTERSIZE"), IupGetInt2(canvas, "RASTERSIZE"));

  iTridimSetProjection(tridim_win);

  tridim_win->SetTitle();
  tridim_win->SetCallbacks();
  tridim_win->ShowWindow();

  IupSetAttribute(canvas, "USERSIZE", NULL);  /* clear minimum restriction */
}

imlab3DWindow::imlab3DWindow(imlabImageDocument* _document)
{
  this->document = _document;

  this->render_type = 0;

  iTridimCreateDialog(this);
}

void imlab3DWindow::Update()
{
  IupGLMakeCurrent(this->canvas);
  iTridimRepaint(this);
}

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <im.h>
#include <im_process.h>
#include <iup.h>
#include <iup_plot.h>
#include <iup_config.h>

#include "histogramwindow.h"
#include "imlab.h"
#include "statusbar.h"
#include "counter_preview.h"



static int cbHistoPlotClickSample(Ihandle* self, int ds_index, int sample_index, double x, double y, int button)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;
  (void)ds_index;
  (void)x;

  if (button != IUP_BUTTON1)
    return IUP_DEFAULT;

  imlabLogMessagef("%d = %d", sample_index, (int)y);

  return IUP_DEFAULT;
}

static imImage* iHistoGetImage(imlabHistogramWindow* histo_win)
{
  imImage* image = histo_win->document->ImageFile->image;

  if (imImageIsBitmap(image))
    return image;

  if (image->color_space <= IM_BINARY && image->data_type <= IM_USHORT)
    return image;

  return histo_win->document->BitmapView.image;
}
static int cbHistoPlotPostDraw(Ihandle *self, cdCanvas* cd_canvas)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;

  /* gray gradient */
  long *palette = NULL;
  imImage* image = iHistoGetImage(histo_win);
  if ((image->color_space == IM_MAP && histo_win->plane == 1) || 
      image->color_space == IM_GRAY)
    palette = image->palette;

  double rx1, rx2;
  double xmin = IupGetDouble(self, "AXS_XMIN");
  double xmax = IupGetDouble(self, "AXS_XMAX");
  IupPlotTransform(self, xmin, 0, &rx1, NULL);
  IupPlotTransform(self, xmax, 0, &rx2, NULL);
  int x1 = (int)rx1;
  int x2 = (int)rx2;

  cdCanvasClip(cd_canvas, CD_CLIPOFF);
  cdCanvasLineWidth(cd_canvas, 1);
  cdCanvasLineStyle(cd_canvas, CD_CONTINUOUS);

  for (int x = x1; x < x2; x++)
  {
    int i = ((x-x1) * 256) / (x2-x1);

    if (palette)
      cdCanvasForeground(cd_canvas, palette[i]);
    else if (histo_win->plane == 0 || histo_win->plane == 4) 
      cdCanvasForeground(cd_canvas, cdEncodeColor((imbyte)i, (imbyte)i, (imbyte)i));
    else
    {
      if (histo_win->plane == 1) 
        cdCanvasForeground(cd_canvas, cdEncodeColor((imbyte)i, 0, 0));
      else if (histo_win->plane == 2) 
        cdCanvasForeground(cd_canvas, cdEncodeColor(0, (imbyte)i, 0));
      else
        cdCanvasForeground(cd_canvas, cdEncodeColor(0, 0, (imbyte)i));
    }

    cdCanvasLine(cd_canvas, x, 0, x, 20);
  }

  cdCanvasForeground(cd_canvas, CD_BLACK);
  cdCanvasRect(cd_canvas, x1, x2, 0, 20);

  return IUP_DEFAULT;
}

static int cmZoomIn(Ihandle* self)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;

  IupSetAttribute(histo_win->plot, "ZOOM", "+");

  return IUP_DEFAULT;
}

static int cmZoomOut(Ihandle* self)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;

  IupSetAttribute(histo_win->plot, "ZOOM", "-");

  return IUP_DEFAULT;
}
                   
static int cmZoomNormal(Ihandle* self)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;

  IupSetAttribute(histo_win->plot, "ZOOM", "0");

  return IUP_DEFAULT;
}

static int cmHistoCumulative(Ihandle* self)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;
  
  histo_win->accum = histo_win->accum? 0: 1;
  histo_win->Update();

  IupConfigSetVariableInt(imlabConfig(), "Histogram", "Cumulative", histo_win->accum);

  return IUP_DEFAULT;
}

static void iHistoUpdateMax(imlabHistogramWindow* histo_win)
{
  int h, i;
  unsigned long max, *histo;
  Ihandle* max_text = (Ihandle*)IupGetAttribute(histo_win->toolbar, "MaxText");

  max = 0;   

  for (h = 0; h < histo_win->num_histo; h++)
  {
    histo = histo_win->histo[h];

    for (i = 0; i < histo_win->histo_count; i++)
    {
      if (max < histo[i])
        max = histo[i];
    }
  }

  histo_win->max = max;
  IupSetfAttribute(max_text, "VALUE", "%lu", max);
}

static int cmHistoMax(Ihandle* self, int c, char *after)
{
  int new_max;
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;
  (void)c;

  new_max = atoi(after);
  if (new_max <= 0)
    iHistoUpdateMax(histo_win);
  else
    histo_win->max = new_max;

  IupSetDouble(histo_win->plot, "AXS_YMAX", (double)histo_win->max);
  IupSetAttribute(histo_win->plot, "REDRAW", "YES");

  return IUP_DEFAULT;
}

static int cbPlotMenu(Ihandle* self)
{
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;

  int x = IupGetInt(self, "X");
  int y = IupGetInt(self, "Y");
  y += IupGetInt2(self, "RASTERSIZE");

  IupSetfAttribute(histo_win->plot, "SHOWMENUCONTEXT", "%d,%d", x, y);

  return IUP_DEFAULT;
}

static int cmHistoPlotMode(Ihandle* self, char* text, int i, int v)
{                   
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;
  (void)text;

  if (!v) return IUP_DEFAULT;

  histo_win->mode = i;
  const char* plot_modes[] = { "LINE", "AREA", "BAR", "MARK", "MARKLINE" };
  int count = IupGetInt(histo_win->plot, "COUNT");

  for (int p = 0; p < count; p++)
  {
    IupSetInt(histo_win->plot, "CURRENT", p);
    IupSetAttribute(histo_win->plot, "DS_MODE", plot_modes[histo_win->mode - 1]);
  }

  IupConfigSetVariableInt(imlabConfig(), "Histogram", "PlotMode", histo_win->mode);
  IupSetAttribute(histo_win->plot, "REDRAW", "ALL");

  return IUP_DEFAULT;
}

static void iHistoAddCurve(imlabHistogramWindow* histo_win, const char* color, unsigned long* histo, const char* name)
{
  IupPlotBegin(histo_win->plot, 0);

  for (int i = 0; i < histo_win->histo_count; i++)
  {
    double y = histo[i];
    double x = i;

    IupPlotAdd(histo_win->plot, x, y);
  }

  IupPlotEnd(histo_win->plot);

  IupSetStrAttribute(histo_win->plot, "DS_NAME", name);
  IupSetStrAttribute(histo_win->plot, "DS_COLOR", color);
  IupSetAttribute(histo_win->plot, "DS_BARSPACING", "0");
  const char* plot_modes[] = { "LINE", "AREA", "BAR", "MARK", "MARKLINE" };
  IupSetAttribute(histo_win->plot, "DS_MODE", plot_modes[histo_win->mode - 1]);
  IupSetAttribute(histo_win->plot, "DS_AREATRANSPARENCY", "64");
}

static void iHistoCalcMinMaxLevel(imlabHistogramWindow* histo_win, int *level_min, int *level_max)
{
  int h, i;
  unsigned long *histo;

  *level_max = 0;
  *level_min = histo_win->histo_count-1;

  for (h = 0; h < histo_win->num_histo; h++)
  {
    histo = histo_win->histo[h];

    for (i = 0; i < histo_win->histo_count; i++)
    {
      if (histo[i])
      {
        if (i < *level_min)
          *level_min = i;
        break;
      }
    }

    for (i = histo_win->histo_count-1; i >= 0; i--)
    {
      if (histo[i])
      {
        if (i > *level_max)
          *level_max = i;
        break;
      }
    }
  }
}

static void iHistoUpdate(imlabHistogramWindow* histo_win)
{
  imImage* image = iHistoGetImage(histo_win);

  imlabCounterEnable(0);

  int hcount = imHistogramCount(image->data_type);
  int realloc = 0;
  if (hcount != histo_win->histo_count)
  {
    realloc = 1;
    histo_win->histo_count = hcount;

    if (histo_win->histo[0]) { imHistogramRelease(histo_win->histo[0]); histo_win->histo[0] = NULL; }
    if (histo_win->histo[1]) { imHistogramRelease(histo_win->histo[1]); histo_win->histo[1] = NULL; }
    if (histo_win->histo[2]) { imHistogramRelease(histo_win->histo[2]); histo_win->histo[2] = NULL; }
    if (histo_win->histo[3]) { imHistogramRelease(histo_win->histo[3]); histo_win->histo[3] = NULL; }
  }

  if (histo_win->num_histo == 1)
  {
    if (realloc) histo_win->histo[0] = imHistogramNew(image->data_type, &hcount);

    imCalcGrayHistogram(image, histo_win->histo[0], histo_win->accum);
  }
  else if (histo_win->num_histo == 2)
  {
    if (realloc) histo_win->histo[0] = imHistogramNew(image->data_type, &hcount);
    if (realloc) histo_win->histo[1] = imHistogramNew(image->data_type, &hcount);

    imCalcGrayHistogram(image, histo_win->histo[0], histo_win->accum);
    imCalcHistogram(image, histo_win->histo[1], 0, histo_win->accum);
  }
  else
  {
    if (realloc) histo_win->histo[0] = imHistogramNew(image->data_type, &hcount);
    if (realloc) histo_win->histo[1] = imHistogramNew(image->data_type, &hcount);
    if (realloc) histo_win->histo[2] = imHistogramNew(image->data_type, &hcount);
    if (realloc) histo_win->histo[3] = imHistogramNew(image->data_type, &hcount);

    imCalcGrayHistogram(image, histo_win->histo[0], histo_win->accum);
    imCalcHistogram(image, histo_win->histo[1], 0, histo_win->accum);
    imCalcHistogram(image, histo_win->histo[2], 1, histo_win->accum);
    imCalcHistogram(image, histo_win->histo[3], 2, histo_win->accum);
  }

  imlabCounterEnable(1);

  if (histo_win->histo_count == 256)
  {
    IupSetInt(histo_win->plot, "AXS_XMIN", 0);
    IupSetInt(histo_win->plot, "AXS_XMAX", 256);
  }
  else
  {
    int level_min, level_max;
    iHistoCalcMinMaxLevel(histo_win, &level_min, &level_max);
    IupSetInt(histo_win->plot, "AXS_XMIN", level_min);
    IupSetInt(histo_win->plot, "AXS_XMAX", level_max);
  }

  iHistoUpdateMax(histo_win);

  IupSetAttribute(histo_win->plot, "CLEAR", "YES");
  IupSetDouble(histo_win->plot, "AXS_YMAX", (double)histo_win->max);

  if (histo_win->plane == 4)
  {
    iHistoAddCurve(histo_win, "255 0 0", histo_win->histo[1], "Red");
    iHistoAddCurve(histo_win, "0 255 0", histo_win->histo[2], "Green");
    iHistoAddCurve(histo_win, "0 0 255", histo_win->histo[3], "Blue");
  }
  else
  {
    Ihandle* plane_list = IupGetDialogChild(histo_win->dialog, "PLANELIST");
    char* name = IupGetAttribute(plane_list, "VALUESTRING");

    const char* color = "128 128 128"; // 0
    if (histo_win->plane == 1)
      color = "255 0 0";
    else if (histo_win->plane == 2)
      color = "0 255 0";
    else if (histo_win->plane == 3)
      color = "0 0 255";

    iHistoAddCurve(histo_win, color, histo_win->histo[histo_win->plane], name);
  }
}

static int cmHistoPlane(Ihandle* self, char* text, int i, int v)
{                   
  imlabHistogramWindow* histo_win = (imlabHistogramWindow*)IupGetAttribute(self, "imlabImageWindow");
  if (!histo_win) return IUP_DEFAULT;
  (void)text;

  if (!v) return IUP_DEFAULT;
  
  histo_win->plane = i-1;

  iHistoUpdate(histo_win);

  IupSetAttribute(histo_win->plot, "REDRAW", "YES");

  return IUP_DEFAULT;
}

static void iHistoUpdatePlaneList(imlabHistogramWindow* histo_win)
{
  Ihandle* plane_list = IupGetDialogChild(histo_win->dialog, "PLANELIST");
  imImage* image = iHistoGetImage(histo_win);

  if (image->color_space == IM_RGB)
  {
    if (histo_win->plane == -1)
      histo_win->plane = 0;

    IupSetAttribute(plane_list, "2", "Red");
    IupSetAttribute(plane_list, "3", "Green");
    IupSetAttribute(plane_list, "4", "Blue");
    IupSetAttribute(plane_list, "5", "RGB");
    IupSetAttribute(plane_list, "6", NULL);
    histo_win->num_histo = 4;
  }
  else if (image->color_space == IM_MAP)
  {
    IupSetAttribute(plane_list, "2", "Map");
    IupSetAttribute(plane_list, "3", NULL);
    if (histo_win->plane == -1 ||
      histo_win->plane > 1)
    {
      histo_win->plane = 1;
      IupSetAttribute(plane_list, "VALUE", "2");
    }
    histo_win->num_histo = 2;
  }
  else
  {
    if (histo_win->plane == -1)
      histo_win->plane = 0;

    IupSetAttribute(plane_list, "2", NULL);
    if (histo_win->plane > 0)
    {
      histo_win->plane = 0;
      IupSetAttribute(plane_list, "VALUE", "1");
    }
    histo_win->num_histo = 1;
  }
}

static Ihandle* iHistoCreateToolBar(imlabHistogramWindow* histo_win)
{
  Ihandle *NormalButton, *ZoomInButton, *ZoomOutButton, *tgl,
          *toolbar, *plane_list, *mode_list, *iup_text;

  NormalButton = IupButton(NULL, NULL);
  IupSetAttribute(NormalButton,"TIP","View the histogram with no zoom.\n Use double click to reset.");
  IupSetAttribute(NormalButton,"IMAGE","IUP_ZoomActualSize");
  IupSetAttribute(NormalButton, "EXPAND", "NO");
  IupSetCallback(NormalButton, "ACTION", (Icallback)cmZoomNormal);

  ZoomInButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomInButton,"TIP","Increase the histogram size on screen.\n Use '+' or Ctrl+mouse drag or Wheel");
  IupSetAttribute(ZoomInButton,"IMAGE","IUP_ZoomIn");
  IupSetAttribute(ZoomInButton, "EXPAND", "NO");
  IupSetCallback(ZoomInButton, "ACTION", (Icallback)cmZoomIn);

  ZoomOutButton = IupButton(NULL, NULL);
  IupSetAttribute(ZoomOutButton,"TIP","Decrease the histogram size on screen.\n Use '-' or Ctrl+mouse drag or Wheel");
  IupSetAttribute(ZoomOutButton,"IMAGE","IUP_ZoomOut");
  IupSetAttribute(ZoomOutButton, "EXPAND", "NO");
  IupSetCallback(ZoomOutButton, "ACTION", (Icallback)cmZoomOut);

  plane_list = IupList(NULL);
  IupSetAttribute(plane_list, "NAME", "PLANELIST");
  IupSetCallback(plane_list, "ACTION", (Icallback)cmHistoPlane);

  iup_text = IupText(NULL);
  IupSetCallback(iup_text, "ACTION", (Icallback)cmHistoMax);
  IupSetAttribute(iup_text, "MASK", IUP_MASK_UINT);

  mode_list = IupList(NULL);
  IupSetAttribute(mode_list, "1", "Lines");
  IupSetAttribute(mode_list, "2", "Area");
  IupSetAttribute(mode_list, "3", "Bars");
  IupSetAttribute(mode_list, "4", "Marks");
  IupSetAttribute(mode_list, "5", "Marks+Lines");
  IupSetAttribute(mode_list, "DROPDOWN", "YES");
  IupSetAttribute(mode_list, "NAME", "MODELIST");
  IupSetCallback(mode_list, "ACTION", (Icallback)cmHistoPlotMode);

  histo_win->mode = IupConfigGetVariableIntDef(imlabConfig(), "Histogram", "PlotMode", 2);
  IupSetInt(mode_list, "VALUE", histo_win->mode);

  toolbar = IupHbox(
    NormalButton,
    ZoomInButton,
    ZoomOutButton,
    mode_list,
    tgl = IupToggle("Cumulative", NULL),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupLabel("Comp:"),
    IupSetAttributes(plane_list, "DROPDOWN=YES, 1=Gray, VALUE=1"),
    IupLabel("Max:"),
    iup_text, 
    IupSetAttributes(IupSetCallbacks(IupButton(NULL, NULL), "ACTION", (Icallback)cbPlotMenu, NULL), "PADDING=6x3, IMAGE=IUP_ToolsSettings"),
    NULL);

  IupSetCallback(tgl, "ACTION", (Icallback)cmHistoCumulative);
  histo_win->accum = IupConfigGetVariableIntDef(imlabConfig(), "Histogram", "Cumulative", 0);
  IupSetInt(tgl, "VALUE", histo_win->accum);

  IupSetAttribute(toolbar, "MaxText", (char*)iup_text);
  IupSetAttribute(toolbar, "ALIGNMENT", "ACENTER");
  IupSetAttribute(toolbar, "FLAT", "YES");
  IupSetAttribute(toolbar, "GAP", "5");
  IupSetAttribute(toolbar, "MARGIN", "5x5");

  return toolbar;
}

static void iHistoCreateDialog(imlabHistogramWindow* histo_win)
{
  Ihandle *vbDesktop, *dialog, *toolbar, *plot;

  plot = IupPlot();
  IupStoreAttribute(plot,"RASTERSIZE","512x384");
  IupSetAttribute(plot, "AXS_XCROSSORIGIN", "NO");
  IupSetAttribute(plot, "AXS_YCROSSORIGIN", "NO");
  IupSetAttribute(plot, "GRID", "Yes");
  IupSetAttribute(plot, "GRIDLINESTYLE", "DOTTED");
  IupSetAttribute(plot, "BOX", "Yes");
  IupSetAttribute(plot, "AXS_XAUTOMIN", "No");
  IupSetAttribute(plot, "AXS_XAUTOMAX", "No");
  IupSetAttribute(plot, "AXS_YAUTOMIN", "No");
  IupSetAttribute(plot, "AXS_YAUTOMAX", "No");
  IupSetAttribute(plot, "AXS_XMAX", "255");
  IupSetAttribute(plot, "AXS_XMIN", "0");
  IupSetAttribute(plot, "AXS_YMIN", "0");
  IupSetAttribute(plot, "AXS_XTICKFORMATPRECISION", "0");
  IupSetAttribute(plot, "AXS_YTICKFORMATPRECISION", "0");
  IupSetAttribute(plot, "HIGHLIGHTMODE", "SAMPLE");
  IupSetCallback(plot, "POSTDRAW_CB", (Icallback)cbHistoPlotPostDraw);
  IupSetCallback(plot, "CLICKSAMPLE_CB", (Icallback)cbHistoPlotClickSample);

  toolbar = iHistoCreateToolBar(histo_win);

  vbDesktop = IupVbox(
    toolbar,
    plot,
    NULL);

  dialog = IupDialog(vbDesktop);
  histo_win->dialog = dialog;
  histo_win->plot = plot;
  histo_win->toolbar = toolbar;
  
  IupSetAttribute(dialog,"PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dialog,"ICON", "IMLAB");
  IupSetAttribute(dialog, "SHRINK", "YES");

  IupSetAttribute(dialog, "imlabImageWindow", (char*)histo_win);
  IupSetAttribute(dialog, "ImageWindowType", "Histogram");

  histo_win->SetTitle();
  histo_win->SetCallbacks();
  histo_win->ShowWindow();

  IupSetAttribute(plot, "USERSIZE", NULL);  /* clear minimum restriction */

  histo_win->Update();
}

imlabHistogramWindow::~imlabHistogramWindow() 
{ 
  if (histo[0]) imHistogramRelease(histo[0]);
  if (histo[1]) imHistogramRelease(histo[1]);
  if (histo[2]) imHistogramRelease(histo[2]);
  if (histo[3]) imHistogramRelease(histo[3]);
}

imlabHistogramWindow::imlabHistogramWindow(imlabImageDocument* _document)
{
  this->document = _document;

  this->plane = -1; /* first time */
  this->accum = 0;
  this->mode = 2; /* area */
  this->histo_count = 0;
  this->num_histo = 0;

  histo[0] = NULL;
  histo[1] = NULL;
  histo[2] = NULL;
  histo[3] = NULL;

  iHistoCreateDialog(this);
}
                   
void imlabHistogramWindow::Update()
{
  iHistoUpdatePlaneList(this);

  iHistoUpdate(this);

  IupSetAttribute(this->plot, "REDRAW", "YES");
}

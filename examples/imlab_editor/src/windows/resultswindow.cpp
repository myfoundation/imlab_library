
#include <iup.h>
#include <iupcontrols.h>
#include <iup_plot.h>
#include <im.h>
#include <im_util.h>
#include <im_attrib.h>
#include <cd.h>
#include <cdpdf.h>
#include <cdps.h>
#include <cdprint.h>
#include <cdsvg.h>
#include <cdemf.h>
#include <cdwmf.h>
#include <cdcgm.h>
#include <cdclipbd.h>

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "imlab.h"
#include "resultswindow.h"
#include "documentlist.h"
#include "utl_file.h"
#include "im_imageview.h"
#include "dialogs.h"
#include <iup_config.h>



static int cbPlotLegend(Ihandle* self, int state)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return IUP_DEFAULT;

  IupSetInt(results_win->plot, "LEGENDSHOW", state);
  IupConfigSetVariableInt(imlabConfig(), "Results", "PlotLegend", state);
  IupSetAttribute(results_win->plot, "REDRAW", NULL);

  return IUP_DEFAULT;
}

static int cbPlotMenu(Ihandle* self)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return IUP_DEFAULT;

  int x = IupGetInt(self, "X");
  int y = IupGetInt(self, "Y");
  y += IupGetInt2(self, "RASTERSIZE");

  IupSetfAttribute(results_win->plot, "SHOWMENUCONTEXT", "%d,%d", x, y);

  return IUP_DEFAULT;
}

static int cbMatrixMenu(Ihandle* self)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return IUP_DEFAULT;

  int x = IupGetInt(self, "X");
  int y = IupGetInt(self, "Y");
  y += IupGetInt2(self, "RASTERSIZE");

  IupSetfAttributeId2(results_win->matrix, "SHOWMENUCONTEXT", 0, 0, "%d,%d", x, y);

  return IUP_DEFAULT;
}

enum{ IM_MEASURE_NONE, IM_MEASURE_LENGHT, IM_MEASURE_AREA };
static int iResultsGetType(const char* name)
{
#define IM_MEASURE_COUNT 12
  char* measures_str[IM_MEASURE_COUNT] = { "Area", "Perimeter", "Area Inside Perimeter", "Holes Count", "Holes Area", "Holes Perimeter", "Centroid X", "Centroid Y",
    "Major Axis Slope", "Major Axis Length", "Minor Axis Slope", "Minor Axis Length" };
  int measures_type[IM_MEASURE_COUNT] = { IM_MEASURE_AREA, IM_MEASURE_LENGHT, IM_MEASURE_AREA, IM_MEASURE_NONE, IM_MEASURE_AREA, IM_MEASURE_LENGHT, IM_MEASURE_LENGHT, IM_MEASURE_LENGHT,
    IM_MEASURE_NONE, IM_MEASURE_LENGHT, IM_MEASURE_NONE, IM_MEASURE_LENGHT };
  int i, count = IM_MEASURE_COUNT;

  for (i = 0; i < count; i++)
  {
    if (imStrEqual(name, measures_str[i]))
      return measures_type[i];
  }
  return IM_MEASURE_NONE;
}

static const char* iResultGetAreaSymbol(const char* units)
{
#define LENGTH_UNITS 12
  const char* length_str[LENGTH_UNITS] = { "m", "mm", "cm", "km", "nm", "Å", "µ", "in", "ft", "yd", "mi", "NM" };
  const char* area_str[LENGTH_UNITS] = { "m²", "mm²", "cm²", "km²", "nm²", "Å²", "µ²", "sq in", "sq ft", "sq yd", "sq mi", "sq NM" };

  int i, count = LENGTH_UNITS;

  for (i = 0; i < count; i++)
  {
    if (imStrEqual(units, length_str[i]))
      return area_str[i];
  }

  static char area_units[30] = "";
  strcpy(area_units, units);
  strcat(area_units, "²");
  return area_units;
}

static char* cbMatrixValue(Ihandle* self, int lin, int col)
{
  static char buffer[512] = "";
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return "";

  imAttribArray* object_array = (imAttribArray*)results_win->object_array;

  if (lin == 0 || col == 0)
  {
    if (lin == 0 && col == 0)
      sprintf(buffer, "Region #");
    else if (col == 0)
    {
      if (lin == 1)
        sprintf(buffer, "Plot");
      else
        sprintf(buffer, "%d", lin - 1); // lines have objects, object numbers start at 1
    }
    else
    {
      char name[50];
      object_array->Get(col - 1, name, NULL, NULL); // columns have measures
      strcpy(buffer, name);

      int jj = IupGetIntId(self, "JJ", col - 1);
      if (jj)
        buffer[jj] = '\n';

      int result_type = iResultsGetType(name);  // here does not depends if scale was set

      if (result_type != IM_MEASURE_NONE)
      {
        if (result_type == IM_MEASURE_LENGHT)
        {
          strcat(buffer, " (");
          if (results_win->scale_units)
            strcat(buffer, results_win->scale_units);
          else
            strcat(buffer, "px");
          strcat(buffer, ")");
        }
        else if (result_type == IM_MEASURE_AREA)
        {
          strcat(buffer, " (");
          if (results_win->scale_units)
            strcat(buffer, iResultGetAreaSymbol(results_win->scale_units));
          else
            strcat(buffer, "px²");
          strcat(buffer, ")");
        }
      }
    }
  }
  else if (lin == 1)
    return "";
  else
  {
    int data_type;
    char name[50];
    const void* data = object_array->Get(col - 1, name, &data_type, NULL);

    int edit = IupGetInt(self, "_INSIDE_EDITION");
    if (edit)
    {
      int focus_lin, focus_col;
      IupGetIntInt(self, "FOCUS_CELL", &focus_lin, &focus_col);
      if (focus_lin != lin || focus_col != col)
        edit = 0;
    }

    int result_type = IM_MEASURE_NONE;
    if (results_win->scale)
      result_type = iResultsGetType(name);

    if (result_type != IM_MEASURE_NONE)
    {
      double value = imDataGetDouble(data, lin - 2, data_type);

      if (result_type == IM_MEASURE_LENGHT)
        value /= results_win->scale;
      else if (result_type == IM_MEASURE_AREA)
        value /= results_win->scale * results_win->scale;

      imImageViewData2Str(buffer, (void*)&value, 0, IM_DOUBLE, edit);
    }
    else
      imImageViewData2Str(buffer, data, lin - 2, data_type, edit);
  }

  return buffer;
}

static int cbMatrixDropCheck(Ihandle* self, int lin, int col)
{
  (void)self; // unused, avoid warning
  if (lin == 1 && col > 0)
    return IUP_CONTINUE;   // toggle button
  return IUP_IGNORE;
}

static int cbMatrixMenuContextClose(Ihandle* self, Ihandle* menu, int lin, int col)
{
  (void)menu; // unused, avoid warning
  (void)lin;
  (void)col;

  IupConfigSetVariableStr(imlabConfig(), "Results", "MatrixTextSeparator", IupGetAttribute(self, "TEXTSEPARATOR"));
  IupConfigSetVariableStr(imlabConfig(), "Results", "MatrixDecimalSymbol", IupGetAttribute(self, "NUMERICDECIMALSYMBOL"));
  IupConfigSetVariableStr(imlabConfig(), "Results", "MatrixDecimals", IupGetAttribute(self, "NUMERICFORMATPRECISION"));

  char* lastfilename = IupGetAttribute(self, "LASTFILENAME");
  if (lastfilename)
  {
    char* last_directory = utlFileGetPath(lastfilename);
    if (last_directory)
    {
      IupConfigSetVariableStr(imlabConfig(), "FileSelection", "LastOtherDirectory", last_directory);
      IupSetStrAttribute(self, "FILEDIRECTORY", last_directory);
      free(last_directory);
    }

    IupSetAttribute(self, "LASTFILENAME", NULL);
  }

  return IUP_DEFAULT;
}

static int cbMatrixEnterItem(Ihandle *self, int lin, int col)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (lin > 1)
  {
    int old_lin = IupGetInt(self, "_IUP_OLD_LIN");
    if (old_lin != lin)
    {
      IupSetAttribute(results_win->plot, "REDRAW", NULL);
      IupSetInt(self, "_IUP_OLD_LIN", lin);
    }
  }
  (void)col;
  return IUP_DEFAULT;
}

static int cbPlotPostDraw(Ihandle *self, cdCanvas* cd_canvas)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  int focus_lin, focus_col;
  IupGetIntInt(results_win->matrix, "FOCUS_CELL", &focus_lin, &focus_col);
  if (focus_lin > 1)
  {
    double x1, x2, y1, y2;
    double ymin = IupGetFloat(self, "AXS_YMIN");
    double ymax = IupGetFloat(self, "AXS_YMAX");

    IupPlotTransform(self, (double)(focus_lin - 2), ymin, &x1, &y1);
    IupPlotTransform(self, (double)(focus_lin - 2), ymax, &x2, &y2);

    cdCanvasForeground(cd_canvas, CD_DARK_GRAY);
    cdCanvasLineStyle(cd_canvas, CD_DASHED);

    cdfCanvasLine(cd_canvas, x1, y1, x2, y2);
  }

  return IUP_DEFAULT;
}

static void iResultsAddPlot(imlabResultsWindow* results_win, imAttribArray* object_array, int cur)
{
  char name[50];
  int data_type, count;
  const void* data = object_array->Get(cur, name, &data_type, &count);

  IupPlotBegin(results_win->plot, 0);

  int result_type = IM_MEASURE_NONE;
  if (results_win->scale)
    result_type = iResultsGetType(name);

  for (int i = 0; i<count; i++)
  {
    double y = imDataGetDouble(data, i, data_type);
    double x = i;

    if (results_win->scale)
    {
      if (result_type == IM_MEASURE_LENGHT)
        y /= results_win->scale;
      else if (result_type == IM_MEASURE_AREA)
        y /= results_win->scale * results_win->scale;
    }

    IupPlotAdd(results_win->plot, x, y);
  }

  IupPlotEnd(results_win->plot);

  IupSetStrAttribute(results_win->plot, "DS_NAME", name);

  Ihandle* lst_plot = (Ihandle*)IupGetAttribute(results_win->plot, "PlotList");
  const char* plot_modes[] = { "LINE", "AREA", "BAR", "MULTIBAR", "MARK", "MARKLINE" };
  int mode_index = IupGetInt(lst_plot, "VALUE") - 1;
  IupSetAttribute(results_win->plot, "DS_MODE", plot_modes[mode_index]);
  IupSetAttribute(results_win->plot, "DS_AREATRANSPARENCY", "64");

  IupSetStrAttributeId2(results_win->matrix, "BGCOLOR", 1, cur + 1, IupGetAttribute(results_win->plot, "DS_COLOR"));
  IupSetAttribute(results_win->matrix, "REDRAW", "L1");
}

static void iResultsRemovePlot(imlabResultsWindow* results_win, imAttribArray* object_array, int cur)
{
  char name[50];
  object_array->Get(cur, name, NULL, NULL); // columns have measures
  IupSetStrAttribute(results_win->plot, "REMOVE", name);

  IupSetAttributeId2(results_win->matrix, "BGCOLOR", 1, cur + 1, NULL);
  IupSetAttribute(results_win->matrix, "REDRAW", "L1");
}

static int cbMatrixToggleValue(Ihandle* self, int lin, int col, int status)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return IUP_DEFAULT;

  if (lin != 1)
    return IUP_DEFAULT;

  imAttribArray* object_array = (imAttribArray*)results_win->object_array;

  int cur = col - 1;
  if (cur >= object_array->Count())
    return IUP_DEFAULT;

  if (status == 1)
    iResultsAddPlot(results_win, object_array, cur);
  else
    iResultsRemovePlot(results_win, object_array, cur);

  if (status == 1 && !IupGetInt(results_win->plot, "VISIBLE"))
  {
    IupSetAttribute(results_win->plot, "RASTERSIZE", "400x");

    Ihandle* vbox = IupGetParent(results_win->plot);
    IupSetAttribute(vbox, "FLOATING", "NO");
    IupSetAttribute(vbox, "VISIBLE", "Yes");

    /* reset dialog current size so it will change its window size */
    IupSetAttribute(IupGetDialog(results_win->plot), "RASTERSIZE", NULL);
    IupRefresh(results_win->plot);
    IupSetAttribute(results_win->plot, "USERSIZE", NULL); /* clear minimum restriction */
  }

  IupSetAttribute(results_win->plot, "REDRAW", NULL);

  return IUP_DEFAULT;
}

static int cbMatrixValueEdit(Ihandle* self, int lin, int col, char* buffer)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  if (!results_win) return IUP_DEFAULT;

  if (lin <= 1 || col == 0)
    return IUP_DEFAULT;

  imAttribArray* object_array = (imAttribArray*)results_win->object_array;

  if (!buffer)
    buffer = "0";

  int data_type;
  char name[50];
  void* data = (void*)object_array->Get(col - 1, name, &data_type, NULL);

  int result_type = IM_MEASURE_NONE;
  if (results_win->scale)
    result_type = iResultsGetType(name);

  if (result_type != IM_MEASURE_NONE)
  {
    double value;
    sscanf(buffer, "%lf", &value);

    if (result_type == IM_MEASURE_LENGHT)
      value *= results_win->scale;
    else if (result_type == IM_MEASURE_AREA)
      value *= results_win->scale * results_win->scale;

    imDataSetDouble(data, lin - 2, data_type, value);
  }
  else
    imImageViewStr2Data(buffer, data, lin - 2, data_type);

  return IUP_DEFAULT;
}

static void iResultsMatrixUpdate(imlabResultsWindow* results_win)
{
  imAttribArray* object_array = (imAttribArray*)results_win->object_array;
  Ihandle* total_label = IupGetDialogChild(results_win->matrix, "TOTAL");

  int object_count = 0;
  object_array->Get(0, NULL, NULL, &object_count);

  IupSetInt(results_win->matrix, "NUMCOL", object_array->Count()); 
  IupSetInt(results_win->matrix, "NUMLIN", object_count + 1);
  IupSetfAttribute(total_label, "TITLE", "Total regions: %d", object_count);

  if (object_array->Count() > 10)
    IupSetAttribute(results_win->matrix, "NUMCOL_VISIBLE", "10");
  else
    IupSetInt(results_win->matrix, "NUMCOL_VISIBLE", object_array->Count());

  if (object_count > 20)
    IupSetAttribute(results_win->matrix,"NUMLIN_VISIBLE","20");
  else
    IupSetInt(results_win->matrix, "NUMLIN_VISIBLE", object_count + 1);

  IupSetAttribute(results_win->matrix, "NUMLIN_NOSCROLL", "1");

  for (int obj = 0; obj < object_array->Count(); obj++)
  {
    char name[50];

    int data_type;
    object_array->Get(obj, name, &data_type, NULL);

    int len = (int)strlen(name);
    if (len > 15)
    {
      int j, jj = -1, half = len / 2;
      for (j = 0; j < len; j++)
      {
        if (name[j] == ' ')
        {
          if (jj != -1)
          {
            if (abs(j - half) < abs(jj - half))
              jj = j;
          }
          else
            jj = j;
        }
      }

      if (jj != -1)
      {
        IupSetIntId(results_win->matrix, "JJ", obj, jj);
        IupSetAttribute(results_win->matrix, "HEIGHT0", "18");
      }
    }

    int result_type = IM_MEASURE_NONE;
    if (results_win->scale)
      result_type = iResultsGetType(name);

    if (result_type != IM_MEASURE_NONE)
    {
      if (result_type == IM_MEASURE_LENGHT)
      {
        IupSetAttributeId(results_win->matrix, "NUMERICQUANTITY", obj + 1, "Length");
        IupSetStrAttributeId(results_win->matrix, "NUMERICUNITSYMBOL", obj + 1, results_win->scale_units);
        if (!imStrEqual(results_win->scale_units, IupGetAttributeId(results_win->matrix, "NUMERICUNITSYMBOL", obj + 1)))
          IupSetAttributeId(results_win->matrix, "NUMERICQUANTITY", obj + 1, "None");
      }
      else if (result_type == IM_MEASURE_AREA)
      {
        IupSetAttributeId(results_win->matrix, "NUMERICQUANTITY", obj + 1, "Area");
        const char* area_units = iResultGetAreaSymbol(results_win->scale_units);
        IupSetStrAttributeId(results_win->matrix, "NUMERICUNITSYMBOL", obj + 1, area_units);
        if (!imStrEqual(area_units, IupGetAttributeId(results_win->matrix, "NUMERICUNITSYMBOL", obj + 1)))
          IupSetAttributeId(results_win->matrix, "NUMERICQUANTITY", obj + 1, "None");
      }

      data_type = IM_DOUBLE;
    }
    else
      IupSetAttributeId(results_win->matrix, "NUMERICQUANTITY", obj + 1, "None");  

    if (data_type < IM_FLOAT)
      IupSetAttributeId(results_win->matrix, "NUMERICFORMATPRECISION", obj + 1, "0");
    else
      IupSetAttributeId(results_win->matrix, "NUMERICFORMATPRECISION", obj + 1, NULL);  // reset to use NUMERICFORMATDEF 
  }
}

static void iResultsUpdateTitle(imlabResultsWindow* results_win)
{
  char Title[512];

  sprintf(Title, "[%d] Results of %.60s", results_win->window_number, 
                               IupGetAttribute(results_win->dialog, "ORIGINAL_TITLE"));

  IupStoreAttribute(results_win->dialog, "TITLE", Title);
}

static int cbListPlotMode(Ihandle* self, char *t, int index, int v)
{
  (void)t;
  if (v == 1)
  {
    imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
    const char* plot_modes[] = { "LINE", "AREA", "BAR", "MULTIBAR", "MARK", "MARKLINE" };
    int count = IupGetInt(results_win->plot, "COUNT");

    for (int p = 0; p < count; p++)
    {
      IupSetInt(results_win->plot, "CURRENT", p);
      IupSetAttribute(results_win->plot, "DS_MODE", plot_modes[index - 1]);
    }

    IupConfigSetVariableInt(imlabConfig(), "Results", "PlotMode", index);
    IupSetAttribute(results_win->plot, "REDRAW", "ALL");
  }
  return IUP_DEFAULT;
}

static int cbDialogClose(Ihandle* self)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)IupGetAttribute(self, "imlabResultsWindow");
  imlabResultsWindowClose(results_win);
  return IUP_IGNORE;
}

static int cbMatrixEdition(Ihandle *ih, int lin, int col, int mode, int update)
{
  if (lin == 1)
    return IUP_IGNORE;

  if (mode == 1)
    IupSetAttribute(ih, "_INSIDE_EDITION", "1");
  else
    IupSetAttribute(ih, "_INSIDE_EDITION", NULL);

  (void)update;
  (void)col;
  return IUP_DEFAULT;
}

static void iResultsCreateWindow(const char* image_title, imlabResultsWindow* results_win)
{
  Ihandle *vbDesktop, *dialog, *matrix, *lst_plot;
  int xpos, ypos;

  matrix = IupMatrixEx();
  results_win->matrix = matrix;

  IupSetAttribute(matrix, "WIDTHDEF", "20");
  IupSetAttribute(matrix,"NUMLIN_VISIBLE","10");
  IupSetAttribute(matrix, "NUMCOL_VISIBLE", "10");
  IupSetAttribute(matrix,"WIDTH0","32");
  IupSetAttribute(matrix,"HEIGHT0","10");
  IupSetAttribute(matrix, "ALIGNMENT", "ARIGHT");
  IupSetAttribute(matrix, "ALIGNMENT0", "ACENTER");
  IupSetAttribute(matrix, "ALIGNMENTLIN0", "ACENTER");
  IupSetAttribute(matrix, "MARK_MODE", "LIN");
  IupSetAttribute(matrix, "MARKMULTIPLE", "YES");
  IupSetAttribute(matrix, "RESIZEMATRIX", "YES");
  IupSetAttribute(matrix, "EXPAND", "VERTICAL");
  IupSetAttribute(matrix, "SKIPLINES", "1");
  IupSetAttribute(matrix, "USETITLESIZE", "YES");

  IupSetCallback(matrix, "EDITION_CB", (Icallback)cbMatrixEdition);
  IupSetCallback(matrix, "VALUE_CB", (Icallback)cbMatrixValue);
  IupSetCallback(matrix, "VALUE_EDIT_CB", (Icallback)cbMatrixValueEdit);
  IupSetCallback(matrix, "TOGGLEVALUE_CB", (Icallback)cbMatrixToggleValue);
  IupSetCallback(matrix, "DROPCHECK_CB", (Icallback)cbMatrixDropCheck);
  IupSetCallback(matrix, "MENUCONTEXTCLOSE_CB", (Icallback)cbMatrixMenuContextClose);
  IupSetCallback(matrix, "ENTERITEM_CB", (Icallback)cbMatrixEnterItem);

  IupSetStrAttribute(matrix, "TEXTSEPARATOR", IupConfigGetVariableStr(imlabConfig(), "Results", "MatrixTextSeparator"));
  IupSetStrAttribute(matrix, "NUMERICDECIMALSYMBOL", IupConfigGetVariableStr(imlabConfig(), "Results", "MatrixDecimalSymbol"));
  IupSetStrAttribute(matrix, "NUMERICFORMATPRECISION", IupConfigGetVariableStr(imlabConfig(), "Results", "MatrixDecimals"));  // same as set NUMERICFORMATDEF
  IupSetStrAttribute(matrix, "FILEDIRECTORY", IupConfigGetVariableStr(imlabConfig(), "FileSelection", "LastOtherDirectory"));

  results_win->plot = IupPlot();
  IupSetAttribute(results_win->plot, "AXS_XCROSSORIGIN", "NO");
  IupSetAttribute(results_win->plot, "AXS_YCROSSORIGIN", "NO");
  IupSetAttribute(results_win->plot, "AXS_XFONTSTYLE", "Bold");
  IupSetAttribute(results_win->plot, "GRID", "Yes");
  IupSetAttribute(results_win->plot, "GRIDLINESTYLE", "DOTTED");
  IupSetAttribute(results_win->plot, "LEGENDSHOW", "Yes");
  IupSetAttribute(results_win->plot, "AXS_XTICKFORMATPRECISION", "0");
  IupSetAttribute(results_win->plot, "AXS_XLABEL", "Region #");
  IupSetAttribute(results_win->plot, "AXS_XLABELCENTERED", "Yes");
  IupSetAttribute(results_win->plot, "HIGHLIGHTMODE", "SAMPLE");
  IupSetCallback(results_win->plot, "POSTDRAW_CB", (Icallback)cbPlotPostDraw);

  lst_plot = IupList(NULL);
  IupSetAttribute(lst_plot, "1", "Lines");
  IupSetAttribute(lst_plot, "2", "Area");
  IupSetAttribute(lst_plot, "3", "Bars");
  IupSetAttribute(lst_plot, "4", "Multi Bars");
  IupSetAttribute(lst_plot, "5", "Marks");
  IupSetAttribute(lst_plot, "6", "Marks+Lines");
  IupSetAttribute(lst_plot, "DROPDOWN", "YES");
  IupSetAttribute(lst_plot, "VISIBLEITEMS", "6");
  IupSetAttribute(results_win->plot, "PlotList", (char*)lst_plot);
  IupSetCallback(lst_plot, "ACTION", (Icallback)cbListPlotMode);

  int plotmode = IupConfigGetVariableIntDef(imlabConfig(), "Results", "PlotMode", 1);
  IupSetInt(lst_plot, "VALUE", plotmode);
  int plotlegend = IupConfigGetVariableIntDef(imlabConfig(), "Results", "PlotLegend", 1);

  vbDesktop = IupHbox(
    IupVbox(
      IupSetAttributes(IupHbox(
        IupSetAttributes(IupLabel(""), "NAME=TOTAL"),
        IupSetAttributes(IupSetCallbacks(IupButton(NULL, NULL), "ACTION", (Icallback)cbMatrixMenu, NULL), "PADDING=6x3, IMAGE=IUP_ToolsSettings"),
        IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
        NULL), "MARGIN=5x5, GAP=5, ALIGNMENT=ACENTER"),
      matrix,
      NULL),
    IupSetAttributes(IupVbox(
      IupSetAttributes(IupHbox(
        IupLabel("Plot Mode:"),
        lst_plot,
        IupSetCallbacks(IupSetAttributes(IupToggle("Legend", NULL), plotlegend ? "VALUE=ON" : "VALUE=OFF"), "ACTION", (Icallback)cbPlotLegend, NULL),
        IupFill(),
        IupSetAttributes(IupSetCallbacks(IupButton(NULL, NULL), "ACTION", (Icallback)cbPlotMenu, NULL), "PADDING=6x3, IMAGE=IUP_ToolsSettings"),
        NULL), "MARGIN=5x5, GAP=5, ALIGNMENT=ACENTER"),
      results_win->plot,
      NULL), "VISIBLE=NO, FLOATING=IGNORE"),
    NULL);

  dialog = IupDialog(vbDesktop);
  results_win->dialog = dialog;

  IupSetCallback(dialog, "CLOSE_CB", (Icallback)cbDialogClose);
  IupSetAttribute(dialog,"PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dialog,"ICON", "IMLAB");
  IupSetAttribute(dialog, "imlabResultsWindow", (char*)results_win);

  iResultsMatrixUpdate(results_win);

  IupStoreAttribute(results_win->dialog, "ORIGINAL_TITLE", image_title);
  iResultsUpdateTitle(results_win);

  IupMap(results_win->dialog);
  Ihandle* total_label = IupGetDialogChild(results_win->matrix, "TOTAL");
  int box_size = IupGetInt(IupGetParent(total_label), "RASTERSIZE");
  int label_size = IupGetInt(total_label, "RASTERSIZE");
  IupSetStrf(total_label, "RASTERSIZE", "%dx", IupGetInt(results_win->matrix, "RASTERSIZE") - (box_size - label_size));
  IupRefresh(results_win->dialog);

  imlabNewWindowPos(&xpos, &ypos);
  IupShowXY(dialog, xpos, ypos);
}

imlabResultsWindow* imlabResultsWindowCreate(const char* image_title, void* object_array, double scale, const char* scale_units)
{
  imlabResultsWindow* results_win = (imlabResultsWindow*)malloc(sizeof(imlabResultsWindow));
  memset(results_win, 0, sizeof(imlabResultsWindow));

  static int results_count = 0;
  results_win->window_number = results_count; results_count++;
  results_win->object_array = object_array;
  results_win->scale = scale;
  results_win->scale_units = scale_units;

  iResultsCreateWindow(image_title, results_win);

  imlabResultsWindowListAdd(results_win);

  IupSetAttribute(results_win->matrix, "REDRAW", "ALL");

  return results_win;
}
                   
int imlabResultsWindowClose(imlabResultsWindow* results_win)
{
  imAttribArray* object_array = (imAttribArray*)results_win->object_array;
  delete object_array;

  IupDestroy(results_win->dialog);

  imlabResultsWindowListRemove(results_win);

  free(results_win);
  return 1;
}

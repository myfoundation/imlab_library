
#include "imagedocument.h"
#include "resultswindow.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "counter.h"
#include <iup_config.h>

#include <im_palette.h>
#include <im_attrib.h>
#include <im_convert.h>
#include <im_process.h>
#include <im_counter.h>
#include <im_colorhsi.h>
#include <im_math.h>

#include <cd.h>
#include <cdirgb.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int measure_regions(Ihandle *parent);

static double sqr_d(int x)
{
  return (double)(x * x);
}

static int set_scale_param(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;

  switch (param_index)
  {
  case 0:
  case 1:
  case 2:
  case 3: /* coordinates */
    {
      int x1 = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
      int y1 = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
      int x2 = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
      int y2 = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "VALUE");

      double d_px = sqrt(sqr_d(x2 - x1) + sqr_d(y2 - y1));
      IupSetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM4"), "VALUE", d_px);
      IupSetDouble((Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM4"), "CONTROL"), "VALUE", d_px);

      double d_un = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM5"), "VALUE");

      if (d_px > 0)
      {
        double scale = d_un / d_px;
        IupSetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM6"), "VALUE", scale);
        IupSetDouble((Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM6"), "CONTROL"), "VALUE", scale);
      }
      break;
    }
  case 4: 
  case 5: /* distance */
    {
      double d_px = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM4"), "VALUE");
      double d_un = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM5"), "VALUE");

      if (d_px > 0)
      {
        double scale = d_un / d_px;
        IupSetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM6"), "VALUE", scale);
        IupSetDouble((Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM6"), "CONTROL"), "VALUE", scale);
      }
      break;
    }
  }

  return 1;
}

static int set_scale(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int x1 = 0, x2 = 0, y1 = 0, y2 = 0;
  double distance_pixels = 0;
  double distance_units = 0;

  double scale = imImageGetAttribReal(image, "SCALE", 0);
  const char* scale_units = imImageGetAttribString(image, "SCALE_LENGTH_UNITS");
  if (!scale_units)
    scale_units = "";

  char new_scale_units[512];
  strcpy(new_scale_units, scale_units);
  double new_scale = scale;

  if (!IupGetParam(document->DlgTitle("Set Scale"), set_scale_param, NULL,
                  "x1 (pixels): %i\n"
                  "y1 (pixels): %i\n"
                  "x2 (pixels): %i\n"
                  "y2 (pixels): %i\n"
                  "Distance (pixels): %R\n"
                  "Distance (units): %R\n"
                  "%t\n"
                  "Scale (pixels/units): %R\n"
                  "Length Units: %s\n",
                  &x1, &y1, &x2, &y2, &distance_pixels, &distance_units, &new_scale, new_scale_units, NULL))
    return IUP_DEFAULT;


  if (scale == new_scale && imStrEqual(scale_units, new_scale_units))
    return IUP_DEFAULT;

  if (new_scale <= 0 || new_scale_units[0] == 0)
  {
    imImageSetAttribute(image, "SCALE", IM_DOUBLE, 0, NULL);
    imImageSetAttribute(image, "SCALE_LENGTH_UNITS", IM_BYTE, 0, NULL);

    IupConfigSetVariableStr(imlabConfig(), "Scale", document->FileTitle, NULL);
    IupConfigSetVariableStr(imlabConfig(), "ScaleUnits", document->FileTitle, NULL);
  }
  else
  {
    imImageSetAttribReal(image, "SCALE", IM_DOUBLE, new_scale);
    imImageSetAttribString(image, "SCALE_LENGTH_UNITS", new_scale_units);

    IupConfigSetVariableDouble(imlabConfig(), "Scale", document->FileTitle, new_scale);
    IupConfigSetVariableStr(imlabConfig(), "ScaleUnits", document->FileTitle, new_scale_units);
  }

  /*
  document->ImageFile->changed = 2;
  document->HasChanged();
  imlabLogMessagef("\"%s\" spatial scale changed. Undo is not possible.", document->FileTitle);
  */
  imlabLogMessagef("\"%s\" spatial scale changed. Unfortunately this information is not saved in the image file.\nBut we will save it in the application configuration file and associate it with this filename.", document->FileTitle);

  return IUP_DEFAULT;
}

static int find_regions(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static int connect_index = 0;
  static int touch_border = 0;
  static int measure_after = 1;

  if (!IupGetParam(document->DlgTitle("Find Regions"), NULL, NULL,
                   "Connectivity: %l|4|8|\n" 
                   "Touching Border: %b\n" 
                   "Measure After: %b\n",
                   &connect_index, &touch_border, &measure_after, NULL))
    return IUP_DEFAULT;

  imImage *NewImage = imImageCreateBased(image, -1, -1, IM_GRAY, IM_USHORT);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int connect = (connect_index + 1) * 4;

  int region_count = 0;
  
  int ret = imAnalyzeFindRegions(image, NewImage, connect, touch_border, &region_count);
  if (!ret)
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (region_count == -1)
  {
    imImageDestroy(NewImage);
    IupMessage("Error!", "Too many regions.");
    return IUP_DEFAULT;
  }

  imImageSetPalette(NewImage, imPaletteHighContrast(), 256);

  document->SetBitmapOptions(0, 0, 0, IM_CAST_MINMAX);

  document->ChangeImage(NewImage, "FindRegions{connect=%d, touch_border=%d}", connect, touch_border);

  imlabLogMessagef("  Found %d regions", region_count);

  if (measure_after)
    measure_regions(parent);

  return IUP_DEFAULT;
}

static void ReplaceColor(imImage* image)
{
  int i;
  imbyte* map = (imbyte*)image->data[0];

  image->color_space = IM_MAP;
  image->palette[254] = imColorEncode(255, 0, 0);

  for (i = 0; i < image->count; i++)
  {
    if (map[i] == 254)
      map[i] = 255;
  }
}

static int fRound(double x)
{
  return x > 0 ? (int) (x + 0.5) : (int) (x - 0.5);
}

static void drawAxis(cdCanvas* cd_canvas, int x0, int y0, double slope, double len)
{
  int xoff = fRound((double)(len*cos(CD_DEG2RAD*slope)/2.));
  int yoff = fRound((double)(len*sin(CD_DEG2RAD*slope)/2.));
  cdCanvasLine(cd_canvas, x0 + xoff, y0 + yoff, x0 - xoff, y0 - yoff);
}

static void drawMeasures(imImage* image, double* data_x, double* data_y, double* data_maj_slope, double* data_maj_len, double* data_min_slope, double* data_min_len, int region_count)
{
  cdCanvas* cd_canvas;
  char param[100];

  ReplaceColor(image);

  sprintf(param, "%dx%d %p %p %p", image->width, image->height, image->data[0], image->data[0], image->data[0]);
  cd_canvas = cdCreateCanvas(CD_IMAGERGB, param);
  cdCanvasForeground(cd_canvas, cdEncodeColor(254, 254, 254));
  cdCanvasMarkType(cd_canvas, CD_PLUS);
  cdCanvasMarkSize(cd_canvas, 5);

  for (int i = 0; i < region_count; i++)
  {
    int x = fRound(data_x[i]);
    int y = fRound(data_y[i]);

    if (data_maj_slope && data_maj_len)
      drawAxis(cd_canvas, x, y, data_maj_slope[i], data_maj_len[i]);

    if (data_min_slope && data_min_len)
      drawAxis(cd_canvas, x, y, data_min_slope[i], data_min_len[i]);

    if (!data_maj_slope || !data_maj_len ||
        !data_min_slope || !data_min_len)
      cdCanvasMark(cd_canvas, x, y);
  }

  cdKillCanvas(cd_canvas);
}

static int iGetMax(imImage* image)
{
  int max = 0;
  imushort* data = (imushort*)image->data[0];
  for (int i = 0; i < image->count; i++)
  {
    if (*data > max)
      max = *data;

    data++;
  }
  return max;
}

static int measure_regions(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static int area = 1, perim = 1, perimarea = 0, centroidX = 0, centroidY = 0,
             holes_count = 0, holes_area = 0, holes_perim = 0,
             maj_slope = 0, maj_len = 0, min_slope = 0, min_len = 0, draw_measures = 0;

  if (!IupGetParam(document->DlgTitle("Measure Regions"), NULL, NULL,
                   "Area: %b\n" 
                   "Perimeter: %b\n" 
                   "Area inside Perim.: %b\n" 
                   " %t\n"
                   "Holes Count: %b\n" 
                   "Holes Area: %b\n" 
                   "Holes Perimeter: %b\n" 
                   " %t\n"
                   "Centroid X: %b\n" 
                   "Centroid Y: %b\n" 
                   "Major Axis Slope: %b\n" 
                   "Major Axis Length: %b\n" 
                   "Minor Axis Slope: %b\n" 
                   "Minor Axis Length: %b\n"
                   "Draw Measures: %b\n", 
                   &area, &perim, &perimarea, 
                   &holes_count, &holes_area, &holes_perim, 
                   &centroidX, &centroidY,
                   &maj_slope, &maj_len, &min_slope, &min_len,
                   &draw_measures,
                   NULL))
    return IUP_DEFAULT;

  int measure_count = 0;
  if (area) measure_count++;
  if (perim) measure_count++;
  if (perimarea) measure_count++;
  if (holes_count) measure_count++;
  if (holes_area) measure_count++;
  if (holes_perim) measure_count++;
  if (centroidX) measure_count++;
  if (centroidY) measure_count++;
  if (maj_slope) measure_count++;
  if (maj_len) measure_count++;
  if (min_slope) measure_count++;
  if (min_len) measure_count++;

  if (measure_count == 0)
    return IUP_DEFAULT;

  imAttribArray* object_array = new imAttribArray(measure_count);

  int counter = imCounterBegin("MeasureRegions");

  int region_count = iGetMax(image);

  int m = 0;
  if (area)
  {
    object_array->Set(m, "Area", IM_INT, region_count, NULL);
    const void* data = object_array->Get(m, NULL, NULL, NULL);
    m++;

    if (!imAnalyzeMeasureArea(image, (int*)data, region_count))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.Area = imlab[\"%s\"].%s", document->FileTitle, "MeasureArea{}");
  }

  if (perim)
  {
    object_array->Set(m, "Perimeter", IM_DOUBLE, region_count, NULL);
    const void* data = object_array->Get(m, NULL, NULL, NULL);
    m++;

    if (!imAnalyzeMeasurePerimeter(image, (double*)data, region_count))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.Perimeter = imlab[\"%s\"].%s", document->FileTitle, "MeasurePerimeter{}");
  }

  if (perimarea)
  {
    object_array->Set(m, "Area Inside Perimeter", IM_DOUBLE, region_count, NULL);
    const void* data = object_array->Get(m, NULL, NULL, NULL);
    m++;

    if (!imAnalyzeMeasurePerimArea(image, (double*)data, region_count))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.AreaInsidePerimeter = imlab[\"%s\"].%s", document->FileTitle, "MeasurePerimArea{}");
  }

  if (holes_count || holes_area || holes_perim)
  {
    const void *data_count = 0, *data_area = 0, *data_perim = 0;

    if (holes_count)
    {
      object_array->Set(m, "Holes Count", IM_INT, region_count, NULL);
      data_count = object_array->Get(m, NULL, NULL, NULL);
      m++;
    }

    if (holes_area)
    {
      object_array->Set(m, "Holes Area", IM_INT, region_count, NULL);
      data_area = object_array->Get(m, NULL, NULL, NULL);
      m++;
    }

    if (holes_perim)
    {
      object_array->Set(m, "Holes Perimeter", IM_DOUBLE, region_count, NULL);
      data_perim = object_array->Get(m, NULL, NULL, NULL);
      m++;
    }

    int connect = 8;
    char* rc = (char*)imImageGetAttribute(image, "REGION_CONNECT", NULL, NULL);
    if (rc)
    {
      if (*rc == '4') // background objects has a connection inverted from regions
        connect = 8;
      else
        connect = 4;
    }

    if (!imAnalyzeMeasureHoles(image, connect, region_count, (int*)data_count, (int*)data_area, (double*)data_perim))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.HolesCount, results.HolesArea, results.HolesPerimeter = imlab[\"%s\"].MeasureHoles{holes_count=%d, holes_area=%d, holes_perimeter=%d}", document->FileTitle, holes_count, holes_area, holes_perim);
  }

  if (centroidX || centroidY)
  {
    const void *data_x = 0, *data_y = 0, *data_area = 0;

    if (centroidX)
    {
      object_array->Set(m, "Centroid X", IM_DOUBLE, region_count, NULL);
      data_x = object_array->Get(m, NULL, NULL, NULL);
      centroidX = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (centroidY)
    {
      object_array->Set(m, "Centroid Y", IM_DOUBLE, region_count, NULL);
      data_y = object_array->Get(m, NULL, NULL, NULL);
      centroidY = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (area)
      data_area = object_array->Get(0, NULL, NULL, NULL); /* area is index 0 */

    if (!imAnalyzeMeasureCentroid(image, (int*)data_area, region_count, (double*)data_x, (double*)data_y))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.CentroidX, results.CentroidY = imlab[\"%s\"].MeasureCentroid{centroid_x=%d, centroid_y=%d, Area=results.Area}", document->FileTitle, centroidX, centroidY);
  }

  if (maj_slope || maj_len || min_slope || min_len)
  {
    const void *data_x = 0, *data_y = 0, *data_area = 0;
    const void *data_maj_slope = 0, *data_maj_len = 0, *data_min_slope = 0, *data_min_len = 0;

    if (maj_slope)
    {
      object_array->Set(m, "Major Axis Slope", IM_DOUBLE, region_count, NULL);
      data_maj_slope = object_array->Get(m, NULL, NULL, NULL);
      maj_slope = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (maj_len)
    {
      object_array->Set(m, "Major Axis Length", IM_DOUBLE, region_count, NULL);
      data_maj_len = object_array->Get(m, NULL, NULL, NULL);
      maj_len = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (min_slope)
    {
      object_array->Set(m, "Minor Axis Slope", IM_DOUBLE, region_count, NULL);
      data_min_slope = object_array->Get(m, NULL, NULL, NULL);
      min_slope = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (min_len)
    {
      object_array->Set(m, "Minor Axis Length", IM_DOUBLE, region_count, NULL);
      data_min_len = object_array->Get(m, NULL, NULL, NULL);
      min_len = m+1; // store the index, but increment 1 to avoid zero.
      m++;
    }

    if (area)
      data_area = object_array->Get(0, NULL, NULL, NULL); /* area is index 0 */

    if (centroidY)
      data_y = object_array->Get(centroidY-1, NULL, NULL, NULL); 
    if (centroidX)
      data_x = object_array->Get(centroidX-1, NULL, NULL, NULL); 

    if (!imAnalyzeMeasurePrincipalAxis(image, (int*)data_area, (double*)data_x, (double*)data_y, region_count,
                         (double*)data_maj_slope, (double*)data_maj_len, (double*)data_min_slope, (double*)data_min_len))
    {
      delete object_array;
      imCounterEnd(counter);
      return IUP_DEFAULT;
    }

    imlabLogMessagef("results.MajorAxisSlope, results.MajorAxisLength, results.MinorAxisSlope, results.MinorAxisLength = imlab[\"%s\"].MeasurePrincipalAxis{major_slope=%d, major_length=%d, minor_slope=%d, minor_length=%d, Area=results.Area, CentroidX=results.CentroidX, CentroidY=results.CentroidY}", document->FileTitle, maj_slope, maj_len, min_slope, min_len);
  }

  imCounterEnd(counter);

  double scale = imImageGetAttribReal(image, "SCALE", 0);
  const char* scale_units = imImageGetAttribString(image, "SCALE_LENGTH_UNITS");

  imlabResultsWindowCreate(document->FileTitle, object_array, scale, scale_units);

  imlabLogMessagef("imlab[\"Results of %s\"] = results", document->FileTitle);

  if (draw_measures && centroidY && centroidX)
  {
    const void *data_x = 0, *data_y = 0;
    const void *data_maj_slope = 0, *data_maj_len = 0, *data_min_slope = 0, *data_min_len = 0;

    if (centroidY)
      data_y = object_array->Get(centroidY-1, NULL, NULL, NULL); 
    if (centroidX)
      data_x = object_array->Get(centroidX-1, NULL, NULL, NULL); 
    if (maj_slope)
      data_maj_slope = object_array->Get(maj_slope-1, NULL, NULL, NULL); 
    if (maj_len)
      data_maj_len = object_array->Get(maj_len-1, NULL, NULL, NULL); 
    if (min_slope)
      data_min_slope = object_array->Get(min_slope-1, NULL, NULL, NULL); 
    if (min_len)
      data_min_len = object_array->Get(min_len-1, NULL, NULL, NULL); 

    imImage* NewImage = imImageCreate(image->width, image->height, IM_GRAY, IM_BYTE);
    if (!NewImage)
      imlabDlgMemoryErrorMsg();
    else
    {
      imProcessThreshold(image, NewImage, 0, 255);
      drawMeasures(NewImage, (double*)data_x, (double*)data_y, (double*)data_maj_slope, (double*)data_maj_len, (double*)data_min_slope, (double*)data_min_len, region_count);
      imlabImageDocumentCreateFromImage(NewImage, "DrawMeasures of %s", "DrawMeasures{image=\"%s\"}", document->FileTitle);
    }
  }

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  IupAppend(mnProcess, imlabProcNewItem(mnProcess, "Set Scale...", "set_scale", (Icallback)set_scale, 0));
  IupAppend(mnProcess, imlabProcNewItem(mnProcess, "Find Regions...", "find_regions", (Icallback)find_regions, 1));
  IupAppend(mnProcess, imlabProcNewItem(mnProcess, "Measure Regions...", "measure_regions", (Icallback)measure_regions, 1));

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "find_regions"), "imlabStatusHelp", "Mark different white regions with indexes.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "measure_regions"), "imlabStatusHelp", "Measure the regions previously marked.");
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "find_regions", imImageIsBinary);
  imlabProcPlugInUpdateItem(mnProcess, "measure_regions", imImageIsUShort);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinAnalyze = &plugin;

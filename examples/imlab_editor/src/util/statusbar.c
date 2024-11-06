
#include <iup.h>
#include <cd.h>
#include <cdiup.h>
#include <im.h>  /* just because of the data type definitions */

#include "statusbar.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>


/***********************************************/
/* Status Area Definitions and Global variables */

#define STATUS_OFFSET 15
#define STATUS_ZOOM 80
#define STATUS_XY 160


static void sbDrawBox(sbStatusBar* sb, int pos, int size)
{
  int x2 = pos + size;
  cdCanvasForeground(sb->sbCanvas, CD_WHITE);
  cdCanvasLine(sb->sbCanvas, pos, 1, x2, 1);
  cdCanvasLine(sb->sbCanvas, x2, sb->height - 3, x2, 2);
  cdCanvasForeground(sb->sbCanvas, CD_DARK_GRAY);
  cdCanvasLine(sb->sbCanvas, pos, sb->height - 3, pos, 2);
  cdCanvasLine(sb->sbCanvas, pos, sb->height - 3, x2 - 1, sb->height - 3);
}

static void sbDrawTextCenter(sbStatusBar* sb, int pos, int size)
{
  cdCanvasActivate(sb->sbCanvas);
  cdCanvasForeground(sb->sbCanvas, sb->background);
  cdCanvasBox(sb->sbCanvas, pos + 1, pos + size - 2, 3, sb->height - 4);
  cdCanvasForeground(sb->sbCanvas, CD_BLACK);
  cdCanvasBackground(sb->sbCanvas, sb->background);
  cdCanvasTextAlignment(sb->sbCanvas, CD_CENTER);
  cdCanvasText(sb->sbCanvas, pos + size / 2, sb->height / 2, sb->str);
}

static void sbDrawTextLeft(sbStatusBar* sb, int pos, int size)
{
  cdCanvasActivate(sb->sbCanvas);
  cdCanvasForeground(sb->sbCanvas, sb->background);
  cdCanvasBox(sb->sbCanvas, pos + 1, pos + size - 2, 3, sb->height - 4);
  cdCanvasForeground(sb->sbCanvas, CD_BLACK);
  cdCanvasBackground(sb->sbCanvas, sb->background);
  cdCanvasTextAlignment(sb->sbCanvas, CD_WEST);
  cdCanvasClip(sb->sbCanvas, CD_CLIPAREA);
  cdCanvasText(sb->sbCanvas, pos + 3, sb->height / 2, sb->str);
  cdCanvasClip(sb->sbCanvas, CD_CLIPOFF);
}

void sbClear(sbStatusBar* sb)
{
  cdCanvasActivate(sb->sbCanvas);
  cdCanvasForeground(sb->sbCanvas, sb->background);
  cdCanvasBox(sb->sbCanvas, sb->xy_pos + 1, sb->xy_pos + STATUS_XY - 2, 3, sb->height - 4);
  cdCanvasBox(sb->sbCanvas, sb->rgb_pos + 1, sb->rgb_pos + sb->rgb_size - 2, 3, sb->height - 4);
  cdCanvasBox(sb->sbCanvas, STATUS_OFFSET + 1, STATUS_OFFSET + STATUS_ZOOM - 2, 3, sb->height - 4);
}

void sbDrawZoom(sbStatusBar* sb, int z1, int z2)
{
  if (z1 == 0)
    strcpy(sb->str, " Exp ");
  else if (z1 == -1)
    strcpy(sb->str, " Fit ");
  else
	 sprintf(sb->str, " %d:%d ", z1, z2);

  sbDrawTextCenter(sb, STATUS_OFFSET, STATUS_ZOOM);
}

void sbDrawMessage(sbStatusBar* sb, const char* Msg)
{
  strcpy(sb->str, Msg);
  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);
}

void sbDrawPercent(sbStatusBar* sb, int p)
{
  sprintf(sb->str, " %d%% ", p);
  sbDrawTextCenter(sb, STATUS_OFFSET, STATUS_ZOOM);
}

char* sbDrawXY(sbStatusBar* sb, int x, int y)
{
  sprintf(sb->str, "(%4d, %4d)", x, y);
  sbDrawTextCenter(sb, sb->xy_pos, STATUS_XY);
  return sb->str;
}

char* sbDrawX(sbStatusBar* sb, int x)
{
  sprintf(sb->str, "%4d", x);
  sbDrawTextCenter(sb, sb->xy_pos, STATUS_XY);
  return sb->str;
}

char* sbDrawMap(sbStatusBar* sb, long color, unsigned char i)
{
  unsigned char r,g,b;
  cdDecodeColor(color, &r, &g, &b);
  sprintf(sb->str, "[%3d -> (%3d, %3d, %3d)]", (int)i, (int)r, (int)g, (int)b);
  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);
  return sb->str;
}

static double cpx_mag(double r, double i)
{
  return sqrt(r*r + i*i);
}

static double cpx_phase(double r, double i)
{
  return (atan2(i, r) * CD_RAD2DEG);
}

char* sbDrawA(sbStatusBar* sb, void* a, int data_type)
{
  switch (data_type)
  {
  case IM_BYTE:
    sprintf(sb->str, "[%3d]", (int)(*((unsigned char*)a)));
    break;
  case IM_SHORT:
    sprintf(sb->str, "[%5d]", (int)(*((short*)a)));
    break;
  case IM_USHORT:
    sprintf(sb->str, "[%5d]", (int)(*((unsigned short*)a)));
    break;
  case IM_INT:
    sprintf(sb->str, "[%8d]", *((int*)a));
    break;
  case IM_FLOAT:
    sprintf(sb->str, "[%7.5f]", (double)(*((float*)a)));
    break;
  case IM_CFLOAT:
  {
    float *c = (float*)a;
    sprintf(sb->str, "[%7.5f, %7.5f i] = |%7.5f|<%7.5f°", (double)*c, (double)*(c + 1), cpx_mag(*c, *(c + 1)), cpx_phase(*c, *(c + 1)));
  }
  break;
  case IM_DOUBLE:
    sprintf(sb->str, "[%7.5f]", (double)(*((double*)a)));
    break;
  case IM_CDOUBLE:
  {
    double *c = (double*)a;
    sprintf(sb->str, "[%7.5f, %7.5f i] = |%7.5f|<%7.5f°", (double)*c, (double)*(c + 1), cpx_mag(*c, *(c + 1)), cpx_phase(*c, *(c + 1)));
  }
  break;
  }

  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);

  return sb->str;
}

char* sbDrawAB(sbStatusBar* sb, void* a, void* b, int data_type)
{
  switch(data_type)
  {
  case IM_BYTE:
    sprintf(sb->str, "[%3d %3d]", (int)(*((unsigned char*)a)), (int)(*((unsigned char*)b)));
    break;
  case IM_SHORT:
    sprintf(sb->str, "[%5d %5d]", (int)(*((short*)a)), (int)(*((short*)b)));
    break;
  case IM_USHORT:
    sprintf(sb->str, "[%5d %5d]", (int)(*((unsigned short*)a)), (int)(*((unsigned short*)b)));
    break;
  case IM_INT:  
    sprintf(sb->str, "[%8d %8d]", *((int*)a), *((int*)b));
    break;
  case IM_FLOAT:
    sprintf(sb->str, "[%7.5f %7.5f]", (double)(*((float*)a)), (double)(*((float*)b)));
    break;
  case IM_CFLOAT:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((float*)a)), (double)(*(((float*)a)+1)), 
                                                            (double)(*((float*)b)), (double)(*(((float*)b)+1)));
    break;
  case IM_DOUBLE:
    sprintf(sb->str, "[%7.5f %7.5f]", (double)(*((double*)a)), (double)(*((double*)b)));
    break;
  case IM_CDOUBLE:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((double*)a)), (double)(*(((double*)a) + 1)),
                                                            (double)(*((double*)b)), (double)(*(((double*)b) + 1)));
    break;
  }

  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);

  return sb->str;
}

char* sbDrawABC(sbStatusBar* sb, void* a, void* b, void* c, int data_type)
{
  switch(data_type)
  {
  case IM_BYTE:
    sprintf(sb->str, "[%3d %3d %3d]", (int)(*((unsigned char*)a)), (int)(*((unsigned char*)b)), (int)(*((unsigned char*)c)));
    break;
  case IM_SHORT:
    sprintf(sb->str, "[%5d %5d %5d]", (int)(*((short*)a)), (int)(*((short*)b)), (int)(*((short*)c)));
    break;
  case IM_USHORT:
    sprintf(sb->str, "[%5d %5d %5d]", (int)(*((unsigned short*)a)), (int)(*((unsigned short*)b)), (int)(*((unsigned short*)c)));
    break;
  case IM_INT:  
    sprintf(sb->str, "[%8d %8d %8d]", *((int*)a), *((int*)b), *((int*)c));
    break;
  case IM_FLOAT:
    sprintf(sb->str, "[%7.5f %7.5f %7.5f]", (double)(*((float*)a)), (double)(*((float*)b)), (double)(*((float*)c)));
    break;
  case IM_CFLOAT:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((float*)a)), (double)(*(((float*)a)+1)), 
                                                                             (double)(*((float*)b)), (double)(*(((float*)b)+1)), 
                                                                             (double)(*((float*)c)), (double)(*(((float*)c)+1)));
    break;
  case IM_DOUBLE:
    sprintf(sb->str, "[%7.5f %7.5f %7.5f]", (double)(*((double*)a)), (double)(*((double*)b)), (double)(*((double*)c)));
    break;
  case IM_CDOUBLE:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((double*)a)), (double)(*(((double*)a)+1)), 
                                                                             (double)(*((double*)b)), (double)(*(((double*)b)+1)), 
                                                                             (double)(*((double*)c)), (double)(*(((double*)c)+1)));
    break;
  }

  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);

  return sb->str;
}

char* sbDrawABCD(sbStatusBar* sb, void* a, void* b, void* c, void* d, int data_type)
{
  switch(data_type)
  {
  case IM_BYTE:
    sprintf(sb->str, "[%3d %3d %3d %3d]", (int)(*((unsigned char*)a)), (int)(*((unsigned char*)b)), (int)(*((unsigned char*)c)), (int)(*((unsigned char*)d)));
    break;
  case IM_SHORT:
    sprintf(sb->str, "[%5d %5d %5d %5d]", (int)(*((short*)a)), (int)(*((short*)b)), (int)(*((short*)c)), (int)(*((short*)d)));
    break;
  case IM_USHORT:
    sprintf(sb->str, "[%5d %5d %5d %5d]", (int)(*((unsigned short*)a)), (int)(*((unsigned short*)b)), (int)(*((unsigned short*)c)), (int)(*((unsigned short*)d)));
    break;
  case IM_INT:  
    sprintf(sb->str, "[%8d %8d %8d %8d]", *((int*)a), *((int*)b), *((int*)c), *((int*)d));
    break;
  case IM_FLOAT:
    sprintf(sb->str, "[%7.5f %7.5f %7.5f %7.5f]", (double)(*((float*)a)), (double)(*((float*)b)), (double)(*((float*)c)), (double)(*((float*)d)));
    break;
  case IM_CFLOAT:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((float*)a)), (double)(*(((float*)a)+1)), 
                                                                                              (double)(*((float*)b)), (double)(*(((float*)b)+1)), 
                                                                                              (double)(*((float*)c)), (double)(*(((float*)c)+1)), 
                                                                                              (double)(*((float*)d)), (double)(*(((float*)d)+1)));
    break;
  case IM_DOUBLE:
    sprintf(sb->str, "[%7.5f %7.5f %7.5f %7.5f]", (double)(*((double*)a)), (double)(*((double*)b)), (double)(*((double*)c)), (double)(*((double*)d)));
    break;
  case IM_CDOUBLE:
    sprintf(sb->str, "[(%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i) (%7.5f, %7.5f i)]", (double)(*((double*)a)), (double)(*(((double*)a)+1)), 
                                                                                              (double)(*((double*)b)), (double)(*(((double*)b)+1)), 
                                                                                              (double)(*((double*)c)), (double)(*(((double*)c)+1)), 
                                                                                              (double)(*((double*)d)), (double)(*(((double*)d)+1)));
    break;
  }

  sbDrawTextLeft(sb, sb->rgb_pos, sb->rgb_size);

  return sb->str;
}

static int sbRepaint(Ihandle* canvas)
{
  int wc, hc;
  sbStatusBar* sb = (sbStatusBar*)IupGetAttribute(canvas, "sbStatusBar");

  cdCanvasActivate(sb->sbCanvas);
  cdCanvasGetSize(sb->sbCanvas, &wc, &hc, 0, 0);

  sb->height = hc;

  cdCanvasBackground(sb->sbCanvas, sb->background);
  cdCanvasClear(sb->sbCanvas);

  /* Draw zoom status area */
  sbDrawBox(sb, STATUS_OFFSET, STATUS_ZOOM);

  /* Draw (x,y) position status area */
  sb->xy_pos = 2 * STATUS_OFFSET + STATUS_ZOOM;
  sbDrawBox(sb, sb->xy_pos, STATUS_XY);

  /* Draw (r,g,b) color status area */
  sb->rgb_size = wc - (4 * STATUS_OFFSET + STATUS_ZOOM + STATUS_XY);
  sb->rgb_size = sb->rgb_size < 3? 3: sb->rgb_size;
  sb->rgb_pos = sb->xy_pos + STATUS_XY + STATUS_OFFSET;
  sbDrawBox(sb, sb->rgb_pos, sb->rgb_size);

  /* The text can be too longer */
  cdCanvasClipArea(sb->sbCanvas, sb->rgb_pos, sb->rgb_pos + sb->rgb_size, 2, sb->height - 3);

  return IUP_DEFAULT;
}

sbStatusBar* sbCreate(Ihandle* canvas)
{
  sbStatusBar* sb;
  int style, size;

  sb = (sbStatusBar*)malloc(sizeof(sbStatusBar));

  sb->sbCanvas = cdCreateCanvas(CD_IUP, canvas);

  cdCanvasActivate(sb->sbCanvas);
  cdCanvasBackOpacity(sb->sbCanvas, CD_OPAQUE);
  cdCanvasNativeFont(sb->sbCanvas, IupGetAttribute(canvas, "FONT"));
  cdCanvasGetFont(sb->sbCanvas, NULL, &style, &size);

  cdCanvasFont(sb->sbCanvas, "Courier", style, size);

  IupSetCallback(canvas, "ACTION", (Icallback)sbRepaint);
  IupSetAttribute(canvas, "sbStatusBar", (char*)sb);
  IupSetAttribute(canvas, "BGCOLOR", NULL);  /* after ACTION callback */

  {
    unsigned char r, g, b;
	  IupGetRGB(IupGetDialog(canvas), "BGCOLOR", &r, &g, &b); 
	  sb->background = cdEncodeColor(r, g, b);
  }

  sbRepaint(canvas);

  return sb;
}

void sbKill(sbStatusBar* sb)
{
  cdKillCanvas(sb->sbCanvas);
  free(sb);
}


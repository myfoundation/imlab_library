#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iup.h>
#include <iupcontrols.h>

#include "matrixwindow.h"
#include "im_imageview.h"
#include "imlab.h"
#include "statusbar.h"
#include "im_clipboard.h"
#include <iup_config.h>
#include "utl_file.h"


static int iMatrixImage2String(char* buffer, imImage* image, int x, int y, int more_prec)
{
  int index = image->width * y + x;
  int d, buff_off = 0;
  int depth = image->has_alpha? image->depth+1: image->depth;
  for (d = 0; d < depth; d++)
  {
    buff_off += imImageViewData2Str(buffer + buff_off, image->data[d], index, image->data_type, more_prec);

    if (d != depth-1)
    {
      buffer[buff_off] = ' ';
      buffer[buff_off+1] = 0;
      buff_off++;
    }
  }
  return buff_off;
}

static void iMatrixImage2RGB(int *r, int *g, int *b, imImage* image, int x, int y)
{
  int index = image->width * y + x;
  imbyte** image_data = (imbyte**)image->data;

  if (image->color_space == IM_RGB)
  {
    *r = image_data[0][index];
    *g = image_data[1][index];
    *b = image_data[2][index];
  }
  else
  {
    long color = image->palette[image_data[0][index]];
    *r = cdRed(color);
    *g = cdGreen(color);
    *b = cdBlue(color);
  }
}

char* imlabMatrixWindow::MatrixValueCallback(Ihandle* self, int lin, int col)
{
  static char buffer[512] = "";

  if (lin == 0 || col == 0)
  {
    if (lin == 0 && col == 0)
      buffer[0] = 0;
    else if (lin == 0)
      sprintf(buffer, "%d", col-1);
    else
      sprintf(buffer, "%d", (image->height-1) - (lin-1));
  }
  else
  {
    int edit = IupGetInt(self, "_INSIDE_EDITION");
    if (edit)
    {
      int focus_lin, focus_col;
      IupGetIntInt(self, "FOCUS_CELL", &focus_lin, &focus_col);
      if (focus_lin != lin || focus_col != col)
        edit = 0;
    }
    iMatrixImage2String(buffer, image, col - 1, (image->height - 1) - (lin - 1), edit);
  }

  return buffer;
}

int imlabMatrixWindow::MatrixBgColorCallback(Ihandle*, int lin, int col, int *r, int *g, int *b)
{
  if (lin == 0 || col == 0)
    return IUP_IGNORE;
  else
    iMatrixImage2RGB(r, g, b, bitmap_image, col-1, (image->height-1) - (lin-1));

  return IUP_DEFAULT;
}

int imlabMatrixWindow::MatrixFgColorCallback(Ihandle*, int lin, int col, int *r, int *g, int *b)
{
  if (lin == 0 || col == 0)
    return IUP_IGNORE;
  else
  {
    unsigned int gray;
    iMatrixImage2RGB(r, g, b, bitmap_image, col - 1, (image->height - 1) - (lin - 1));
    gray = (*r + *g + *b)/3;
    if (gray > 164)
    {
      *r = 0; *g = 0; *b = 0;
    }
    else
    {
      *r = 255; *g = 255; *b = 255;
    }
  }

  return IUP_DEFAULT;
}

static void iMatrixString2Image(char* buffer, imImage* image, int x, int y)
{
  int index = image->width * y + x;
  int d, offset = 0;
  int depth = image->has_alpha ? image->depth + 1 : image->depth;
  for (d = 0; d < depth; d++)
  {
    offset += imImageViewStr2Data(buffer + offset, image->data[d], index, image->data_type);
  }
}

int imlabMatrixWindow::MatrixValueEditCallback(Ihandle*, int lin, int col, char* buffer)
{
  if (lin == 0 || col == 0)
    return IUP_DEFAULT;

  if (!buffer)
    buffer = "0";

  iMatrixString2Image(buffer, image, col-1, (image->height-1) - (lin-1));

  lock_update = 1;
  /* Same as imImageWindowChange but Undo is not saved */
  document->ImageFile->changed = 2;  /* no undo */
  document->HasChanged();
  document->UpdateViews();
  lock_update = 0;

  imlabLogMessagef("\"%s\" pixel was changed. Undo is not possible.", document->FileTitle);

  return IUP_DEFAULT;
}

static int cbEdition(Ihandle *ih, int lin, int col, int mode, int update)
{
  if (mode == 1)
    IupSetAttribute(ih, "_INSIDE_EDITION", "1");
  else
    IupSetAttribute(ih, "_INSIDE_EDITION", NULL);
  (void)update;
  (void)lin;
  (void)col;
  return IUP_DEFAULT;
}

static int cbMenuContextClose(Ihandle* self, Ihandle* menu, int lin, int col)
{
  (void)menu; // unused, avoid warning
  (void)lin;
  (void)col;

  IupConfigSetVariableStr(imlabConfig(), "MatrixView", "MatrixTextSeparator", IupGetAttribute(self, "TEXTSEPARATOR"));
  IupConfigSetVariableStr(imlabConfig(), "MatrixView", "MatrixDecimalSymbol", IupGetAttribute(self, "NUMERICDECIMALSYMBOL"));
  IupConfigSetVariableStr(imlabConfig(), "MatrixView", "MatrixDecimals", IupGetAttribute(self, "NUMERICFORMATPRECISION"));

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

void imlabMatrixWindow::InternalUpdate()
{
  IupSetInt(matrix, "NUMCOL", image->width); 
  IupSetInt(matrix, "NUMLIN", image->height);
  IupSetfAttribute(matrix, "ORIGIN", "%d:1", (image->height-1));

  {
    int i, 
      width = 20;
    if (image->data_type == IM_INT || 
        image->data_type == IM_FLOAT ||
        image->data_type == IM_DOUBLE)
      width *= 2;
    else if (image->data_type == IM_CFLOAT ||
             image->data_type == IM_CDOUBLE)
      width *= 4;

    if (image->depth != 1)
      width *= 2;

    for (i = 0; i < image->width; i++)
    {
      IupSetIntId(matrix, "WIDTH", i+1, width);

      if (image->data_type < IM_FLOAT || (image->data_type >= IM_FLOAT && image->depth != 1))
      {
        IupSetAttributeId(matrix, "NUMERICQUANTITY", i + 1, NULL);
        IupSetAttributeId(matrix, "NUMERICFORMATPRECISION", i + 1, "0");
      }
      else
      {
        IupSetAttributeId(matrix, "NUMERICQUANTITY", i + 1, "None");
        IupSetAttributeId(matrix, "NUMERICFORMATPRECISION", i + 1, NULL);  // reset to use NUMERICFORMATDEF
      }
    }
  }
}

void imlabMatrixWindow::CreateDialog()
{
  matrix = IupMatrixEx();

  IupSetAttribute(matrix, "WIDTHDEF", "20");
  IupSetAttribute(matrix,"NUMLIN_VISIBLE","20");
  IupSetAttribute(matrix, "NUMCOL_VISIBLE", "10");
  IupSetAttribute(matrix,"WIDTH0","20");
  IupSetAttribute(matrix,"HEIGHT0","10");
  IupSetAttribute(matrix, "MARK_MODE", "CELL");
  IupSetAttribute(matrix, "MULTIPLE", "YES");
  IupSetAttribute(matrix, "RESIZEMATRIX", "YES");

  IupSetCallback(matrix, "EDITION_CB", (Icallback)cbEdition);
  IupSetCallback(matrix, "MENUCONTEXTCLOSE_CB", (Icallback)cbMenuContextClose);

  IUP_CLASS_SETCALLBACK(matrix, "VALUE_EDIT_CB", MatrixValueEditCallback);
  IUP_CLASS_SETCALLBACK(matrix, "BGCOLOR_CB", MatrixBgColorCallback);
  IUP_CLASS_SETCALLBACK(matrix, "FGCOLOR_CB", MatrixFgColorCallback);
  IUP_CLASS_SETCALLBACK(matrix, "VALUE_CB", MatrixValueCallback);

  IupSetStrAttribute(matrix, "TEXTSEPARATOR", IupConfigGetVariableStr(imlabConfig(), "MatrixView", "MatrixTextSeparator"));
  IupSetStrAttribute(matrix, "NUMERICDECIMALSYMBOL", IupConfigGetVariableStr(imlabConfig(), "MatrixView", "MatrixDecimalSymbol"));
  IupSetStrAttribute(matrix, "NUMERICFORMATPRECISION", IupConfigGetVariableStr(imlabConfig(), "MatrixView", "MatrixDecimals"));  // same as set NUMERICFORMATDEF
  IupSetStrAttribute(matrix, "FILEDIRECTORY", IupConfigGetVariableStr(imlabConfig(), "FileSelection", "LastOtherDirectory"));

  InternalUpdate();

  dialog = IupDialog(matrix);

  IupSetAttribute(dialog, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(dialog,"ICON", "IMLAB");

  IUP_CLASS_INITCALLBACK(dialog, imlabMatrixWindow);
  
  IupSetAttribute(dialog, "ImageWindowType", "Matrix");

  SetTitle();
  SetCallbacks();
  ShowWindow();

  IupSetAttribute(matrix,"NUMCOL_VISIBLE", "1");
  IupSetAttribute(matrix,"NUMLIN_VISIBLE", "1");
  IupSetfAttribute(matrix, "ORIGIN", "%d:1", (image->height-1));
}

imlabMatrixWindow::imlabMatrixWindow(imlabImageDocument* _document)
{
  document = _document;

  lock_update = 0;
  image = document->ImageFile->image;
  bitmap_image = document->BitmapView.image;

  CreateDialog();
}
                   
void imlabMatrixWindow::Update()
{
  if (lock_update) return;

  image = document->ImageFile->image;
  bitmap_image = document->BitmapView.image;

  InternalUpdate();

  IupSetAttribute(matrix, "REDRAW", "ALL");
}

void imlabMatrixWindow::Sync(int x, int y)
{
  x -= IupGetInt(matrix, "NUMCOL_VISIBLE") / 2;
  y += IupGetInt(matrix, "NUMLIN_VISIBLE") / 2;
  IupSetfAttribute(matrix, "ORIGIN", "%d:%d", (image->height - 1) - y + 2, x + 1);
}                              

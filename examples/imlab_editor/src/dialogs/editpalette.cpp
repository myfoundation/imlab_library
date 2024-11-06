#include <iup.h>
#include <iupcontrols.h>
#include <iupkey.h>
#include <im.h>
#include <cd.h>
#include <cdiup.h>
#include <cddbuf.h>

#include "dialogs.h"
#include "statusbar.h"
#include "imlab.h"
#include "im_clipboard.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BLOCK_SPACE  2
#define BLOCK_TOTAL 30
#define NUM_COL 32
#define NUM_LIN 8


struct EditPalette
{
  cdCanvas* cd_canvas;
  long* palette;
  int* palette_count;
  int changed;
  int focus_index;
};

static int cbRepaint(Ihandle* self)
{
  EditPalette* edit_pal = (EditPalette*)IupGetAttribute(self, "EditPalette");

  /* Activates the graphics cd_canvas */                          
  cdCanvasActivate(edit_pal->cd_canvas);
  cdCanvasClear(edit_pal->cd_canvas);

  int xmin, xmax, ymin, ymax;
  for (int index = 0; index < 256; index++)
  {
    int line = index / NUM_COL;
    int col = index % NUM_COL;

    xmin = col * BLOCK_TOTAL + BLOCK_SPACE;
    xmax = xmin + BLOCK_TOTAL - 2 * 1 - BLOCK_SPACE + 1;
    ymin = line * BLOCK_TOTAL + BLOCK_SPACE;
    ymax = ymin + BLOCK_TOTAL - 2 * 1 - BLOCK_SPACE + 1;

    cdCanvasForeground(edit_pal->cd_canvas, CD_BLACK);
    cdCanvasRect(edit_pal->cd_canvas, xmin, xmax, ymin, ymax);

    cdCanvasForeground(edit_pal->cd_canvas, edit_pal->palette[index]);
    cdCanvasBox(edit_pal->cd_canvas, xmin + 1, xmax - 1, ymin + 1, ymax - 1);

    if (index == edit_pal->focus_index)
    {
      cdCanvasForeground(edit_pal->cd_canvas, CD_BLACK);
      cdCanvasRect(edit_pal->cd_canvas, xmin + 1, xmax - 1, ymin + 1, ymax - 1);

      cdCanvasForeground(edit_pal->cd_canvas, CD_WHITE);
      cdCanvasLineStyle(edit_pal->cd_canvas, CD_DOTTED);
      cdCanvasRect(edit_pal->cd_canvas, xmin + 1, xmax - 1, ymin + 1, ymax - 1);
      cdCanvasLineStyle(edit_pal->cd_canvas, CD_CONTINUOUS);
    }
  }

  cdCanvasFlush(edit_pal->cd_canvas);

  return IUP_DEFAULT;
}

static int iCanvas2Index(int x, int y)
{
  int line = y / BLOCK_TOTAL;
  int col = x / BLOCK_TOTAL;

  x = x % BLOCK_TOTAL;
  y = y % BLOCK_TOTAL;

  if (x <= 2 || x == BLOCK_TOTAL-1 ||
      y <= 2 || y == BLOCK_TOTAL-1)
    return -1;

  return line * NUM_COL + col;
}

static int cbMotion(Ihandle* self, int x, int y)
{
  EditPalette* edit_pal = (EditPalette*)IupGetAttribute(self, "EditPalette");
  Ihandle* info = IupGetDialogChild(self, "info");

  cdCanvasActivate(edit_pal->cd_canvas);
  cdCanvasUpdateYAxis(edit_pal->cd_canvas, &y); /* From now we are in CD coordinates */

  int index = iCanvas2Index(x, y);
  if (index < 0 || index > 255)
  {
    sbClear(imlabStatusBar());
    IupSetAttribute(info, "TITLE", "");
  }
  else
  {
    char* val = sbDrawMap(imlabStatusBar(), edit_pal->palette[index], (imbyte)index);
    IupSetAttribute(info, "TITLE", val);
  }

  return IUP_DEFAULT;
}

static int cbButton(Ihandle* self, int b, int press, int x, int y, char* r)
{
  EditPalette* edit_pal = (EditPalette*)IupGetAttribute(self, "EditPalette");

  if (b == IUP_BUTTON1 && press == 1)
  {
    cdCanvasActivate(edit_pal->cd_canvas);
    cdCanvasUpdateYAxis(edit_pal->cd_canvas, &y); /* From now we are in CD coordinates */
    int index = iCanvas2Index(x, y);
    if (index < 0 || index > 255)
      return IUP_DEFAULT;

    if (edit_pal->focus_index != index)
    {
      edit_pal->focus_index = index;
      cbRepaint(self);
    }

    if (isdouble(r))
    {
      imbyte red, green, blue;
      imColorDecode(&red, &green, &blue, edit_pal->palette[index]);

      if (!IupGetColor(IUP_CENTERPARENT, IUP_CENTERPARENT, &red, &green, &blue))
        return IUP_DEFAULT;

      edit_pal->changed = 1;

      edit_pal->palette[index] = imColorEncode(red, green, blue);
      if (index >= *edit_pal->palette_count)
        *edit_pal->palette_count = index + 1;

      cbRepaint(self);
    }
  }

  return IUP_DEFAULT;
}

static int cbKeyPress(Ihandle* self, int key, int pressed)
{
  EditPalette* edit_pal = (EditPalette*)IupGetAttribute(self, "EditPalette");

  if (pressed)
  {
    int index = edit_pal->focus_index;
    int line = index / NUM_COL;
    int col = index % NUM_COL;

#define K_LEFT     0xFF51
#define K_RIGHT    0xFF53

    switch (key)
    {
    case K_UP:
      if (line < NUM_LIN - 1)
        line++;
      break;
    case K_DOWN:
      if (line > 0)
        line--;
      break;
    case K_RIGHT:
      if (col < NUM_COL - 1)
        col++;
      else if (line < NUM_LIN - 1)
      {
        line++;
        col = 0;
      }
      break;
    case K_LEFT:
      if (col > 0)
        col--;
      else if (line > 0)
      {
        line--;
        col = NUM_COL - 1;
      }
      break;
    }

    index = line * NUM_COL + col;

    if (edit_pal->focus_index != index)
    {
      char* val = sbDrawMap(imlabStatusBar(), edit_pal->palette[index], (imbyte)index);
      Ihandle* info = IupGetDialogChild(self, "info");
      IupSetAttribute(info, "TITLE", val);

      edit_pal->focus_index = index;
      cbRepaint(self);
    }
  }
  else
  {
    if (key == K_SP)
    {
      imbyte red, green, blue;
      imColorDecode(&red, &green, &blue, edit_pal->palette[edit_pal->focus_index]);

      if (IupGetColor(IUP_CENTERPARENT, IUP_CENTERPARENT, &red, &green, &blue))
      {
        edit_pal->changed = 1;

        edit_pal->palette[edit_pal->focus_index] = imColorEncode(red, green, blue);
        if (edit_pal->focus_index >= *edit_pal->palette_count)
          *edit_pal->palette_count = edit_pal->focus_index + 1;

        cbRepaint(self);
      }
    }
    else if (key == K_cC)
    {
      char color[50];
      long c = edit_pal->palette[edit_pal->focus_index];
      sprintf(color, "%d %d %d", (int)cdRed(c), (int)cdGreen(c), (int)cdBlue(c));
      imClipboardCopyText(color);
    }
    else if (key == K_cV)
    {
      Ihandle* clipboard = IupClipboard();
      char* color = IupGetAttribute(clipboard, "TEXT");
      int r, g, b;
      if (sscanf(color, "%d %d %d", &r, &g, &b) == 3)
      {
        edit_pal->changed = 1;

        edit_pal->palette[edit_pal->focus_index] = imColorEncode((imbyte)r, (imbyte)g, (imbyte)b);
        if (edit_pal->focus_index >= *edit_pal->palette_count)
          *edit_pal->palette_count = edit_pal->focus_index + 1;

        cbRepaint(self);
      }

      IupDestroy(clipboard);
    }
  }

  return IUP_DEFAULT;
}

static int cmOK(Ihandle* self)
{
  (void)self;
  return IUP_CLOSE;
}

static int cmCancel(Ihandle* self)
{
  EditPalette* edit_pal = (EditPalette*)IupGetAttribute(self, "EditPalette");
  edit_pal->changed = 0;
  return IUP_CLOSE;
}

int imlabDlgEditPalette(const char* filetitle, long* palette, int* palette_count)
{
  Ihandle* canvas = IupCanvas(NULL);
  IupSetAttribute(canvas,"EXPAND","NO");
  IupSetStrf(canvas, "RASTERSIZE", "%dx%d", NUM_COL*BLOCK_TOTAL + 2 * BLOCK_SPACE, NUM_LIN*BLOCK_TOTAL + 2 * BLOCK_SPACE);

  Ihandle* btOk = IupButton( "OK", NULL );
  IupSetStrAttribute(btOk,"PADDING",IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetHandle("btPaletteOK", btOk);
  IupSetCallback(btOk, "ACTION", (Icallback)cmOK);

  Ihandle* btCancel = IupButton( "Cancel", NULL );
  IupSetAttribute(btCancel, "PADDING", "10x6");
  IupSetStrAttribute(btCancel, "PADDING", IupGetGlobal("DEFAULTBUTTONPADDING"));
  IupSetHandle("btPaletteCancel", btCancel);
  IupSetCallback(btCancel, "ACTION", (Icallback)cmCancel);

  Ihandle* vbDesktop = IupVbox(
    canvas,
    IupHbox(
      IupSetAttributes(IupLabel(NULL), "NAME=info, EXPAND=HORIZONTAL"),
      btOk,
      btCancel,
      NULL),
    NULL);
  IupSetAttribute(vbDesktop, "MARGIN", "10x10");
  IupSetAttribute(vbDesktop, "GAP", "5");

  Ihandle* dialog = IupDialog(vbDesktop);
  IupSetCallback(dialog, "CLOSE_CB", (Icallback)cmCancel);
  IupSetAttribute(dialog,"MENUBOX","NO");
  IupSetAttribute(dialog,"PARENTDIALOG","imlabMainWindow");
  IupSetStrf(dialog, "TITLE", "Edit Palette - %s", filetitle);
  IupSetAttribute(dialog,"DEFAULTENTER","btPaletteOK");
  IupSetAttribute(dialog,"DEFAULTESC","btPaletteCancel");
  IupSetAttribute(dialog,"MINBOX","NO");
  IupSetAttribute(dialog,"MAXBOX","NO");
  IupSetAttribute(dialog,"RESIZE","NO");

  /* Register the cd_canvas callbacks */
  IupSetCallback(canvas, "ACTION", (Icallback)cbRepaint);
  IupSetCallback(canvas, "MOTION_CB", (Icallback)cbMotion);
  IupSetCallback(canvas, "BUTTON_CB", (Icallback)cbButton);
  IupSetCallback(canvas, "KEYPRESS_CB", (Icallback)cbKeyPress);

  IupMap(dialog);

  cdCanvas* cd_canvas_db = cdCreateCanvas(CD_IUPDBUFFER, canvas);

  EditPalette edit_pal;
  edit_pal.cd_canvas = cd_canvas_db;
  edit_pal.palette = palette;
  edit_pal.palette_count = palette_count;
  edit_pal.changed = 0;
  edit_pal.focus_index = 0;
  IupSetAttribute(dialog, "EditPalette", (char*)&edit_pal);

  IupPopup(dialog, IUP_CENTERPARENT, IUP_CENTERPARENT); 

  cdKillCanvas(cd_canvas_db);

  IupDestroy(dialog);
  return edit_pal.changed;
}
                   


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include <iup.h>
#include <iup_config.h>
#include <im.h>
#include <im_lib.h>
#include <im_counter.h>

#include "mainwindow.h"
#include "imagewindow.h"
#include "resultswindow.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "statusbar.h"
#include "im_clipboard.h"

#include "utl_file.h"
#include "counter.h"


void imlabCreateButtonImages(void);

static Ihandle* main_window;
static sbStatusBar* status_bar;
static Ihandle* main_log;
static Ihandle* config;
Ihandle* imlabConfig(void)
{
  return config;
}

struct _sbStatusBar* imlabStatusBar(void)
{
  return status_bar;
}

void imlabLogMessagef(const char* format, ...)
{
  char value[10240];
  va_list arglist;
  va_start(arglist, format);
  vsprintf(value, format, arglist);
  va_end(arglist);

  IupSetAttribute(main_log, "APPEND", value);
  IupSetAttribute(main_log, "APPEND", "\n");
  IupSetAttribute(main_log, "SCROLLTOPOS", "65535");
}

void imlabNewWindowPos(int *xpos, int *ypos)
{
  static int start_xpos = 0;
  static int start_ypos = 0;

  if (!xpos || !ypos)
  {
    start_xpos = 0;
    start_ypos = 0;
    return;
  }

  /* first time or only one window */
  if (imlabImageDocumentListCount() == 0 || (start_xpos == 0 && start_ypos == 0))
  {
    // Just a small offset
    start_xpos = 10;
    start_ypos = 10;
  }
  else
  {
    // some spacing at the diagonal rigth-down
    start_xpos += 40;
    start_ypos += 50;

    if (start_xpos > 600 || start_ypos > 450)  /* half/bottom screen aprox. */
    {
      start_xpos = 120;
      start_ypos = 100;
    }
  }

  *xpos = start_xpos;
  *ypos = start_ypos;

  *xpos += IupGetInt(main_log, "X");
  *ypos += IupGetInt(main_log, "Y");
}

static int cmMenuSelect(Ihandle* self)
{
  char* help = IupGetAttribute(self, "imlabStatusHelp");
  if (!help)
    sbDrawMessage(imlabStatusBar(), "");
  else
    sbDrawMessage(imlabStatusBar(), help);
  return IUP_DEFAULT;
}

Ihandle*  imlabSubmenu(const char* title, Ihandle* child)
{
  Ihandle* submenu = IupSubmenu(title, child);
  IupSetCallback(submenu, "HIGHLIGHT_CB", cmMenuSelect);
  return submenu;
}

Ihandle* imlabItem(const char* title, const char* action)
{
  Ihandle* item = IupItem(title, action);
  IupSetCallback(item, "HIGHLIGHT_CB", cmMenuSelect);
  return item;
}

imlabImageDocument* imlabGetCurrentDocument(Ihandle* ih)
{
  (void)ih; /* unused since 3.0 */
  Ihandle* main_window = IupGetHandle("imlabMainWindow");
  return (imlabImageDocument*)IupGetAttribute(main_window, "imlabImageDocument");
}

void imlabSetCurrentDocument(imlabImageDocument *document)
{
  Ihandle* main_window = IupGetHandle("imlabMainWindow");
  IupSetAttribute(main_window, "imlabImageDocument", (char*)document);
  if (document)
  {
    imlabMainWindowUpdateUndoRedoButtons(document);
    imlabMainWindowUpdateSaveButtons(document);
  }
}

void imlabMainWindowUpdateToolbar(void)
{
  Ihandle *toolbar = (Ihandle*)IupGetAttribute(main_window, "ToolBar");
  int image_count = imlabImageDocumentListCount();

  Ihandle* child = IupGetNextChild(toolbar, NULL);
  while (child)
  {
    if (IupGetInt(child, "CHECK_HASIMAGE"))
    {
      if (image_count == 0)
        IupSetAttribute(child, "ACTIVE", "NO");
      else
        IupSetAttribute(child, "ACTIVE", "YES");
    }

    child = IupGetNextChild(NULL, child);
  }

  Ihandle *PasteButton = (Ihandle*)IupGetAttribute(toolbar, "PasteButton");
  if (imClipboardCanPasteImage())
    IupSetAttribute(PasteButton, "ACTIVE", "YES");
  else
    IupSetAttribute(PasteButton, "ACTIVE", "NO");

  if (image_count != 0)
  {
    imlabImageDocument* document = imlabGetCurrentDocument(main_window);
    if (document)
    {
      imlabMainWindowUpdateSaveButtons(document);
      imlabMainWindowUpdateUndoRedoButtons(document);
    }
  }
}

void imlabMainWindowUpdateSaveButtons(imlabImageDocument* changed_document)
{
  imlabImageDocument* document = imlabGetCurrentDocument(main_window);
  if (changed_document == document)
  {
    Ihandle* ToolBar = (Ihandle*)IupGetAttribute(main_window, "ToolBar");
    Ihandle* SaveButton = (Ihandle*)IupGetAttribute(ToolBar, "SaveButton");
    if (document->ImageFile->changed)
      IupSetAttribute(SaveButton, "ACTIVE", "YES");
    else
      IupSetAttribute(SaveButton, "ACTIVE", "NO");
  }
}

void imlabMainWindowUpdateUndoRedoButtons(imlabImageDocument* changed_document)
{
  imlabImageDocument* document = imlabGetCurrentDocument(main_window);
  if (changed_document == document)
  {
    Ihandle* ToolBar = (Ihandle*)IupGetAttribute(main_window, "ToolBar");
    Ihandle* RedoButton = (Ihandle*)IupGetAttribute(ToolBar, "RedoButton");
    Ihandle* UndoButton = (Ihandle*)IupGetAttribute(ToolBar, "UndoButton");

    if (document->HasUndo())
      IupSetAttribute(UndoButton, "ACTIVE", "Yes");
    else
      IupSetAttribute(UndoButton, "ACTIVE", "NO");

    if (document->HasRedo())
      IupSetAttribute(RedoButton, "ACTIVE", "Yes");
    else
      IupSetAttribute(RedoButton, "ACTIVE", "NO");
  }
}

static int cmDropFile(Ihandle *self, char* filename)
{
  (void)self;
  imlabImageDocumentCreateFromFileName(filename);
  return IUP_DEFAULT;
}

static int cmMainMove(Ihandle* self, int x, int y)
{
  int wx, wy, dx, dy, old_x, old_y, i;

  old_x = IupGetInt(self, "_OLD_X");
  old_y = IupGetInt(self, "_OLD_Y");

  if (old_x == x && old_y == y)
    return IUP_DEFAULT;

  i = 0;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    for (int v = 0; v < document->view_list_count; v++)
    {
      imlabImageWindow* image_window = document->view_list[v];

      wx = IupGetInt(image_window->dialog, "X");
      wy = IupGetInt(image_window->dialog, "Y");
      dx = wx - old_x;
      dy = wy - old_y;

      if (IupGetInt(image_window->dialog, "VISIBLE"))
        IupShowXY(image_window->dialog, x + dx, y + dy);
    }

    i++;
    document = imlabImageDocumentListGet(i);
  }

  i = 0;
  imlabResultsWindow* results_window = imlabResultsWindowListGet(i);
  while (results_window)
  {
    wx = IupGetInt(results_window->dialog, "X");
    wy = IupGetInt(results_window->dialog, "Y");
    dx = wx - old_x;
    dy = wy - old_y;

    if (IupGetInt(results_window->dialog, "VISIBLE"))
      IupShowXY(results_window->dialog, x + dx, y + dy);

    i++;
    results_window = imlabResultsWindowListGet(i);
  }

  IupSetfAttribute(self, "_OLD_X", "%d", x);
  IupSetfAttribute(self, "_OLD_Y", "%d", y);

  return IUP_DEFAULT;
}

static int cmGetFocus(Ihandle* self)
{
  if (main_window)
  {
    Ihandle *toolbar = (Ihandle*)IupGetAttribute(main_window, "ToolBar");
    Ihandle *PasteButton = (Ihandle*)IupGetAttribute(toolbar, "PasteButton");
    if (imClipboardCanPasteImage())
      IupSetAttribute(PasteButton, "ACTIVE", "YES");
    else
      IupSetAttribute(PasteButton, "ACTIVE", "NO");
  }

  (void)self;
  return IUP_DEFAULT;
}

static int cmCopyLog(void)
{
  IupSetAttribute(main_log, "SELECTION", "ALL");
  IupSetAttribute(main_log, "CLIPBOARD", "COPY");
  IupSetAttribute(main_log, "SELECTION", "NONE");
  return IUP_DEFAULT;
}

static int cmSaveLog(void)
{
  char filename[10240] = "*.log";
  if (!imlabDlgSelectFile(filename, "SAVE", "Save Log", "Log Files|*.log|All Files|*.*|", "LastLogDirectory"))
    return IUP_DEFAULT;

  {
    char* logtext = IupGetAttribute(main_log, "VALUE");
    int ret = utlFileSaveText(filename, logtext);
    if (ret == 0)
      IupMessage("Error!", "Error opening file.");
    else if (ret == -1)
      IupMessage("Error!", "Error saving file.");
  }

  return IUP_DEFAULT;
}

static int cmShowBitmap(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->ShowView("Bitmap");
  return IUP_DEFAULT;
}

static int cmShowHistogram(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->ShowView("Histogram");
  return IUP_DEFAULT;
}

static int cmShowMatrix(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->ShowView("Matrix");
  return IUP_DEFAULT;
}

static int cmShow3DView(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->ShowView("3D");
  return IUP_DEFAULT;
}

static void iMainWinPrintVersion(void)
{
  imlabLogMessagef("%s - %s", IMLAB_TITLE, IMLAB_BUILD);
  imlabLogMessagef("IM Version %s - %s", imVersion(), imVersionDate());
  imlabLogMessagef("CD Version %s - %s", cdVersion(), cdVersionDate());
  imlabLogMessagef("IUP Version %s - %s", IupVersion(), IupVersionDate());

  imlabLogMessagef("  System: %s", IupGetGlobal("SYSTEM"));
  imlabLogMessagef("  System Version: %s", IupGetGlobal("SYSTEMVERSION"));
  if (IupGetGlobal("MOTIFVERSION"))
    imlabLogMessagef("  Motif Version: %s", IupGetGlobal("MOTIFVERSION"));
  if (IupGetGlobal("GTKVERSION"))
    imlabLogMessagef("  GTK Version: %s", IupGetGlobal("GTKVERSION"));
}

static void CreateMainWindow(void)
{
  Ihandle *mnMainMenu, *btOpen, *btNew, *btCloseAll, *btCascade, *PasteButton,
          *SaveButton, *CloseButton, *InfoButton,
          *toolbar, *cnvStatus, *vbMainDesktop, *btTile, 
          *UndoButton, *RedoButton, *CopyButton,
          *ShowHistoButton, *ShowBitmapButton, *ShowMatrixButton, *Show3DViewButton;

  mnMainMenu = imlabMainWindowCreateMenu();
  IupSetHandle("mnMainMenu", mnMainMenu);

  btNew = IupButton(NULL, "imlabNew");
  IupSetAttribute(btNew, "TIP", "Creates a new image file.");
  IupSetAttribute(btNew, "IMAGE", "IUP_FileNew");

  btOpen = IupButton(NULL, "imlabOpen");
  IupSetAttribute(btOpen,"TIP","Loads an image file from disk.");
  IupSetAttribute(btOpen,"IMAGE","IUP_FileOpen");

  SaveButton = IupButton(NULL, "imlabSave");
  IupSetAttribute(SaveButton, "TIP", "Saves the active image file.");
  IupSetAttribute(SaveButton, "IMAGE", "IUP_FileSave");
  IupSetAttribute(SaveButton, "ACTIVE", "NO");
  IupSetAttribute(SaveButton, "CHECK_HASIMAGE", "Yes");

  CloseButton = IupButton(NULL, "imlabClose");
  IupSetAttribute(CloseButton, "TIP", "Closes the active image file. Press Shift to ignore changes.");
  IupSetAttribute(CloseButton, "IMAGE", "IUP_FileClose");
  IupSetAttribute(CloseButton, "ACTIVE", "NO");
  IupSetAttribute(CloseButton, "CHECK_HASIMAGE", "Yes");

  btCloseAll = IupButton(NULL, "imlabCloseAll");
  IupSetAttribute(btCloseAll, "TIP", "Close all image files. Press Shift to ignore changes.");
  IupSetAttribute(btCloseAll, "IMAGE", "IUP_FileCloseAll");
  IupSetAttribute(btCloseAll, "ACTIVE", "NO");
  IupSetAttribute(btCloseAll, "CHECK_HASIMAGE", "Yes");

  btCascade = IupButton(NULL, "imlabCascade");
  IupSetAttribute(btCascade, "TIP", "Cascades the windows in the desktop.");
  IupSetAttribute(btCascade, "IMAGE", "IUP_WindowsCascade");
  IupSetAttribute(btCascade, "ACTIVE", "NO");
  IupSetAttribute(btCascade, "CHECK_HASIMAGE", "Yes");

  btTile = IupButton(NULL, "imlabTile");
  IupSetAttribute(btTile, "TIP", "Tiles the windows in the desktop.");
  IupSetAttribute(btTile, "IMAGE", "IUP_WindowsTile");
  IupSetAttribute(btTile, "ACTIVE", "NO");
  IupSetAttribute(btTile, "CHECK_HASIMAGE", "Yes");

  PasteButton = IupButton(NULL, "imlabPaste");
  IupSetAttribute(PasteButton, "TIP", "Pastes an new image file from the clipboard.");
  IupSetAttribute(PasteButton, "IMAGE", "IUP_EditPaste");
  IupSetAttribute(PasteButton, "ACTIVE", "NO");

  UndoButton = IupButton(NULL, "imlabUndo");
  IupSetAttribute(UndoButton, "TIP", "Undo the last change to the active image.");
  IupSetAttribute(UndoButton, "IMAGE", "IUP_EditUndo");
  IupSetAttribute(UndoButton, "ACTIVE", "NO");
  IupSetAttribute(UndoButton, "CHECK_HASIMAGE", "Yes");

  RedoButton = IupButton(NULL, "imlabRedo");
  IupSetAttribute(RedoButton, "TIP", "Redo the last undo to the active image.");
  IupSetAttribute(RedoButton, "IMAGE", "IUP_EditRedo");
  IupSetAttribute(RedoButton, "ACTIVE", "NO");
  IupSetAttribute(RedoButton, "CHECK_HASIMAGE", "Yes");

  CopyButton = IupButton(NULL, "imlabCopy");
  IupSetAttribute(CopyButton, "TIP", "Copies the active image file bitmap to the clipboard.");
  IupSetAttribute(CopyButton, "IMAGE", "IUP_EditCopy");
  IupSetAttribute(CopyButton, "ACTIVE", "NO");
  IupSetAttribute(CopyButton, "CHECK_HASIMAGE", "Yes");

  InfoButton = IupButton(NULL, "imlabImageFileInfo");
  IupSetAttribute(InfoButton, "TIP", "Shows information about the active image file.");
  IupSetAttribute(InfoButton, "IMAGE", "IUP_MessageInfo");
  IupSetAttribute(InfoButton, "ACTIVE", "NO");
  IupSetAttribute(InfoButton, "CHECK_HASIMAGE", "Yes");

  ShowBitmapButton = IupButton(NULL, "imlabShowBitmap");
  IupSetAttribute(ShowBitmapButton, "TIP", "Finds and shows an Bitmap view.\nCreates a new one if not found.");
  IupSetAttribute(ShowBitmapButton, "IMAGE", "imlabBitmapButton");
  IupSetAttribute(ShowBitmapButton, "ACTIVE", "NO");
  IupSetAttribute(ShowBitmapButton, "CHECK_HASIMAGE", "Yes");

  ShowHistoButton = IupButton(NULL, "imlabShowHistogram");
  IupSetAttribute(ShowHistoButton, "TIP", "Finds and shows an Histogram view.\nCreates a new one if not found.");
  IupSetAttribute(ShowHistoButton, "IMAGE", "imlabHistogramButton");
  IupSetAttribute(ShowHistoButton, "ACTIVE", "NO");
  IupSetAttribute(ShowHistoButton, "CHECK_HASIMAGE", "Yes");

  ShowMatrixButton = IupButton(NULL, "imlabShowMatrix");
  IupSetAttribute(ShowMatrixButton, "TIP", "Finds and shows a Matrix view.\nCreates a new one if not found.");
  IupSetAttribute(ShowMatrixButton, "IMAGE", "imlabMatrixButton");
  IupSetAttribute(ShowMatrixButton, "ACTIVE", "NO");
  IupSetAttribute(ShowMatrixButton, "CHECK_HASIMAGE", "Yes");

  Show3DViewButton = IupButton(NULL, "imlabShow3DView");
  IupSetAttribute(Show3DViewButton, "TIP", "Finds and shows a 3D view.\nCreates a new one if not found.");
  IupSetAttribute(Show3DViewButton, "IMAGE", "imlab3DButton");
  IupSetAttribute(Show3DViewButton, "ACTIVE", "NO");
  IupSetAttribute(Show3DViewButton, "CHECK_HASIMAGE", "Yes");

  toolbar = IupHbox(
    btNew,
    btOpen,
    SaveButton,
    CloseButton,
    btCloseAll,
    IupSetAttributes(IupFill(), "SIZE=2"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupSetAttributes(IupFill(), "SIZE=2"),
    PasteButton,
    CopyButton,
    UndoButton,
    RedoButton,
    IupSetAttributes(IupFill(), "SIZE=2"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupSetAttributes(IupFill(), "SIZE=2"),
    InfoButton,
    IupSetAttributes(IupFill(), "SIZE=2"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupSetAttributes(IupFill(), "SIZE=2"),
    ShowBitmapButton,
    ShowHistoButton,
    ShowMatrixButton,
    Show3DViewButton,
    IupSetAttributes(IupFill(), "SIZE=2"),
    IupSetAttributes(IupLabel(NULL), "SEPARATOR=VERTICAL"),
    IupSetAttributes(IupFill(), "SIZE=2"),
    btCascade,
    btTile,
    NULL);

  IupSetAttribute(toolbar, "FLAT", "YES");
  IupSetAttribute(toolbar, "MARGIN", "5x5");
  IupSetAttribute(toolbar, "GAP", "2");

  IupSetAttribute(toolbar, "SaveButton", (char*)SaveButton);
  IupSetAttribute(toolbar, "PasteButton", (char*)PasteButton);
  IupSetAttribute(toolbar, "RedoButton", (char*)RedoButton);
  IupSetAttribute(toolbar, "UndoButton", (char*)UndoButton);

  main_log = IupSetAttributes(IupMultiLine(NULL), "EXPAND=YES, SIZE=10x10");
  IupSetAttribute(main_log, "FONTFACE", "Courier");
  IupSetAttribute(main_log, "APPENDNEWLINE", "NO");

  cnvStatus = IupCanvas(NULL);
  IupSetAttribute(cnvStatus, "EXPAND", "HORIZONTAL");
  IupSetAttribute(cnvStatus, "SIZE", "x12");
  IupSetAttribute(cnvStatus, "BORDER", "NO");

  vbMainDesktop = IupVbox(
    toolbar,
    main_log,
    cnvStatus,
    NULL);

  main_window = IupDialog(vbMainDesktop);

  IupSetAttribute(main_window, "MENU", "mnMainMenu");
  IupSetAttribute(main_window, "TITLE", IMLAB_TITLE);
  IupSetAttribute(main_window, "ICON", "IMLAB");
  IupSetAttribute(main_window, "BRINGFRONT", "YES");
  IupSetAttribute(main_window, "TASKBARPROGRESS", "Yes");

  IupSetCallback(main_window, "DROPFILES_CB", (Icallback)cmDropFile);
  IupSetCallback(main_window, "GETFOCUS_CB", cmGetFocus);
  IupSetCallback(main_window, "MOVE_CB", (Icallback)cmMainMove);

  IupSetAttribute(main_window, "CLOSE_CB", "imlabMainExit");

  /* MUST CHECK for existing image, because the callback will be called even for the disabled menu */
  IupSetAttribute(main_window, "K_cO", "imlabOpen");
  IupSetAttribute(main_window, "K_cN", "imlabWindowNext");
  IupSetAttribute(main_window, "K_cC", "imlabCopy");
  IupSetAttribute(main_window, "K_cD", "imlabDuplicate");
  IupSetAttribute(main_window, "K_cZ", "imlabUndo");
  IupSetAttribute(main_window, "K_cY", "imlabRedo");
  IupSetAttribute(main_window, "K_cS", "imlabSave");

  IupSetAttribute(main_window, "ToolBar", (char*)toolbar);
  IupSetHandle("imlabMainWindow", main_window);

  IupSetFunction("imlabShowBitmap", (Icallback)cmShowBitmap);
  IupSetFunction("imlabShowHistogram", (Icallback)cmShowHistogram);
  IupSetFunction("imlabShowMatrix", (Icallback)cmShowMatrix);
  IupSetFunction("imlabShow3DView", (Icallback)cmShow3DView);

  /* MANDATORY for IupSetFunction */
  IupSetFunction("imlabCopyLog", (Icallback)cmCopyLog);
  IupSetFunction("imlabSaveLog", (Icallback)cmSaveLog);

  /* Shows the application main dialog */
  IupConfigDialogShow(config, main_window, "MainDialog");

  IupSetGlobal("PARENTDIALOG", "imlabMainWindow");
  IupSetGlobal("ICON", "IMLAB");

  /* Initializes the Status Bar */
  status_bar = sbCreate(cnvStatus);

  iMainWinPrintVersion();
}

static void imlabInitConfig()
{
  config = IupConfig();
  IupSetAttribute(config, "APP_NAME", "imlab");
  IupConfigLoad(config);
}

void imlabCreateMainWindow(void)
{
  static int first = 1;

  if (first)
    first = 0;
  else
    return;

  imlabCreateButtonImages();

  imlabInitConfig();

  CreateMainWindow();

  imlabCounterCreateProgressDlg();
}

void imlabKillMainWindow(void)
{
  Ihandle* mainwin = main_window;
  main_window = NULL;

  IupDestroy(config);

  imlabProcPlugInFinish();

  /* Terminates the Status Bar */
  sbKill(status_bar);

  imlabCounterReleaseProgressDlg();

  IupDestroy(mainwin);
}

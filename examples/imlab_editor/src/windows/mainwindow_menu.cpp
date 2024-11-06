#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <cd.h>

#include <iup.h>

#include <im.h>
#include <im_image.h>

#include "mainwindow.h"
#include "imagewindow.h"
#include "imagedocument.h"
#include "resultswindow.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"
#include "utl_file.h"
#include <iup_config.h>
#include "im_clipboard.h"


void imlabInitCaptureMenu(Ihandle* mnCapture);

static int cmRecent(Ihandle* self)
{
  char* filename = IupGetAttribute(self, "TITLE");
  imlabImageDocumentCreateFromFileName(filename);
  return IUP_DEFAULT;
}


/*******************************/
/* Edit Menu call backs        */
/*******************************/


static int cmClearUndo(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->ClearUndo();
  return IUP_DEFAULT;
}

static int cmUndo(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)  // test because of the key action while menu disabled
  {
    /* the menu can be disabled but the keyboard can invoke the callback */
    if (!document->HasUndo())
      return IUP_DEFAULT;

    document->Undo();

    document->UpdateViews();
  }

  return IUP_DEFAULT;
}

static int cmRedo(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)  // test because of the key action while menu disabled
  {
    /* the menu can be disabled but the keyboard can invoke the callback */
    if (!document->HasRedo())
      return IUP_DEFAULT;

    document->Redo();

    document->UpdateViews();
  }

  return IUP_DEFAULT;
}

static int cmCopy(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)  // test because of the key action while menu disabled
    imClipboardCopyImage(document->BitmapView.image);
  return IUP_DEFAULT;
}

static int cmPaste(void)
{
  static int p = 1;
  imImage* NewImage = imClipboardPasteImage();
  if (!NewImage)
  {
    IupMessage("Error!", "Invalid clipboard data.");
    return IUP_DEFAULT;
  }

  imlabImageDocumentCreateFromImage(NewImage, "ClipboardPaste #%d", "ClipboardPaste{}", p);
  p++;

  return IUP_DEFAULT;
}

static int cmDuplicate(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)  // test because of the key action while menu disabled
  {
    imImage *NewImage = imImageDuplicate(document->ImageFile->image);
    if (NewImage == NULL)
    {
      imlabDlgMemoryErrorMsg();
      return IUP_DEFAULT;
    }

    imlabImageDocumentCreateFromImage(NewImage, "Duplicate of %s", "Duplicate{image=\"%s\"}", document->FileTitle);
  }

  return IUP_DEFAULT;
}

static int cmEditMenuOpen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  Ihandle *mnUndo, *mnClearUndo, *mnRedo;

  mnUndo = (Ihandle*)IupGetAttribute(self, "mnUndo");
  if (document && document->HasUndo())
    IupSetAttribute(mnUndo, "ACTIVE", "YES");
  else
    IupSetAttribute(mnUndo, "ACTIVE", "NO");

  mnRedo = (Ihandle*)IupGetAttribute(self, "mnRedo");
  if (document && document->HasRedo())
    IupSetAttribute(mnRedo, "ACTIVE", "YES");
  else
    IupSetAttribute(mnRedo, "ACTIVE", "NO");

  mnClearUndo = (Ihandle*)IupGetAttribute(self, "mnClearUndo");
  if (document && (document->HasUndo() || document->HasRedo()))
    IupSetAttribute(mnClearUndo, "ACTIVE", "YES");
  else
    IupSetAttribute(mnClearUndo, "ACTIVE", "NO");

  Ihandle* mnCopy = (Ihandle*)IupGetAttribute(self, "mnCopy");
  Ihandle* mnDuplicate = (Ihandle*)IupGetAttribute(self, "mnDuplicate");
  if (document)
  {
    IupSetAttribute(mnCopy, "ACTIVE", "YES");
    IupSetAttribute(mnDuplicate, "ACTIVE", "YES");
  }
  else
  {
    IupSetAttribute(mnCopy, "ACTIVE", "NO");
    IupSetAttribute(mnDuplicate, "ACTIVE", "NO");
  }

  if (imClipboardCanPasteImage())
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnPaste"), "ACTIVE", "YES");
  else
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnPaste"), "ACTIVE", "NO");

  return IUP_DEFAULT;
}

/*******************************/
/* Process & Analyze Menu call backs        */
/*******************************/

static int cmProcessMenuOpen(Ihandle* mnProcess)
{
  imlabImageDocument* document = imlabGetCurrentDocument(mnProcess);
  if (document)
  {
    IupSetAttribute(mnProcess, "ACTIVE", "Yes");
    imlabProcPlugInUpdate(mnProcess);
  }
  else
    IupSetAttribute(mnProcess, "ACTIVE", "NO");
  return IUP_DEFAULT;
}

static int cmAnalyzeMenuOpen(Ihandle* mnProcess)
{
  imlabImageDocument* document = imlabGetCurrentDocument(mnProcess);
  if (document)
  {
    IupSetAttribute(mnProcess, "ACTIVE", "Yes");
    imlabProcPlugInUpdate(mnProcess);
  }
  else
    IupSetAttribute(mnProcess, "ACTIVE", "NO");
  return IUP_DEFAULT;
}

/*******************************/
/* Window Menu call backs        */
/*******************************/

static int iCountViews(void)
{
  int count = 0;
  int i = 0;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    count += document->view_list_count;

    i++;
    document = imlabImageDocumentListGet(i);
  }
  return count;
}

static void iCalcTileXY(int *x, int *y, int *tile_x, int *tile_y, int tile_width, int win_width, int win_height)
{
  *x = (*tile_x)*win_width;
  *y = (*tile_y)*win_height;

  if ((*tile_x) + 1 < tile_width)
    (*tile_x)++;
  else
  {
    *tile_x = 0;
    (*tile_y)++;
  }
}

static int cmTile(void)
{
  int results_win_count = imlabResultsWindowListCount();
  int i, x, y, count = results_win_count + iCountViews();
  int tile_width, tile_height, win_width, win_height, tile_x, tile_y;
  int start_x, start_y;

  tile_width = 1;
  while (tile_width*tile_width < count)
    tile_width++;

  tile_height = tile_width;
  while (tile_width*(tile_height - 1) >= count)
    tile_height--;   

  imlabNewWindowPos(NULL, NULL);  /* reset pos */

  imlabNewWindowPos(&start_x, &start_y);  /* first cascade position */

  {
    int screen_width, screen_height;
    IupGetIntInt(NULL, "SCREENSIZE", &screen_width, &screen_height);
    screen_width -= start_x;  /* remove first position lost space */
    screen_height -= start_y;
    win_width = screen_width / tile_width;
    win_height = screen_height / tile_height;
  }

  tile_x = 0; tile_y = 0;
  i = 0;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    for (int v = 0; v < document->view_list_count; v++)
    {
      imlabImageWindow* image_window = document->view_list[v];

      iCalcTileXY(&x, &y, &tile_x, &tile_y, tile_width, win_width, win_height);
      IupSetfAttribute(image_window->dialog, "RASTERSIZE", "%dx%d", win_width - 10, win_height - 10);
      IupShowXY(image_window->dialog, x + start_x, y + start_y);
      IupSetAttribute(image_window->dialog, "USERSIZE", NULL);  /* clear minimum restriction without reseting the current size */
    }

    i++;
    document = imlabImageDocumentListGet(i);
  }

  i = 0;
  imlabResultsWindow* results_window = imlabResultsWindowListGet(i);
  while (results_window)
  {
    iCalcTileXY(&x, &y, &tile_x, &tile_y, tile_width, win_width, win_height);
    IupSetfAttribute(results_window->dialog, "RASTERSIZE", "%dx%d", win_width - 10, win_height - 10);
    IupShowXY(results_window->dialog, x + start_x, y + start_y);
    IupSetAttribute(results_window->dialog, "USERSIZE", NULL);  /* clear minimum restriction without reseting the current size */

    i++;
    results_window = imlabResultsWindowListGet(i);
  }

  return IUP_DEFAULT;
}

static int cmCascade(void)
{
  int i, x, y;

  imlabNewWindowPos(NULL, NULL);  /* reset pos */

  i = 0;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    for (int v = 0; v < document->view_list_count; v++)
    {
      imlabImageWindow* window = document->view_list[v];

      imlabNewWindowPos(&x, &y);
      IupShowXY(window->dialog, x, y);
    }

    i++;
    document = imlabImageDocumentListGet(i);
  }

  i = 0;
  imlabResultsWindow* results_window = imlabResultsWindowListGet(i);
  while (results_window)
  {
    imlabNewWindowPos(&x, &y);
    IupShowXY(results_window->dialog, x, y);

    i++;
    results_window = imlabResultsWindowListGet(i);
  }

  return IUP_DEFAULT;
}

static int cmWindowNext(Ihandle* self)
{
  /* MUST CHECK for existing image, because the callback will be called even for the disabled menu */
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imlabImageDocument* next_document = imlabImageDocumentListNext(document);
  if (next_document && next_document != document)
    next_document->ShowView("Bitmap");
  return IUP_DEFAULT;
}

static int cmListDocuments(void)
{
  imlabImageDocument* document = imlabImageDocumentListSelect("Activate Image Document", NULL, NULL);
  if (document)
    document->ShowView("Bitmap");
  return IUP_DEFAULT;
}

static int cmListResults(void)
{
  imlabResultsWindow* window = (imlabResultsWindow*)imlabResultsWindowListSelect("Activate Results Window");
  if (window)
    IupShow(window->dialog);

  return IUP_DEFAULT;
}

static int cmWindowMenuOpen(Ihandle* self)
{
  int image_document_count = imlabImageDocumentListCount();
  int results_win_count = imlabResultsWindowListCount();

  if (image_document_count == 0 && results_win_count == 0)
  {
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnCascade"), "ACTIVE", "NO");
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnTile"), "ACTIVE", "NO");
  }
  else
  {
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnCascade"), "ACTIVE", "YES");
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnTile"), "ACTIVE", "YES");
  }

  if (image_document_count == 0)
  {
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnListImages"), "ACTIVE", "NO");
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnNextImage"), "ACTIVE", "NO");
  }
  else
  {
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnListImages"), "ACTIVE", "YES");
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnNextImage"), "ACTIVE", "YES");
  }

  if (results_win_count == 0)
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnListResults"), "ACTIVE", "NO");
  else
    IupSetAttribute((Ihandle*)IupGetAttribute(self, "mnListResults"), "ACTIVE", "YES");

  return IUP_DEFAULT;
}


/*******************************/
/* Help Menu call backs        */
/*******************************/


static int cmWeb(void)
{
  IupHelp("http://imlab.sourceforge.net/");
  return IUP_DEFAULT;
}

static int cmAboutOK(void)
{
  return IUP_CLOSE;
}

static int cmAbout(void)
{
  Ihandle *_bt_1, *_vbox_2, *dlgAbout, *lbl, *txt;

  lbl = IupLabel(NULL);
  IupSetAttribute(lbl, "IMAGE", "IMLAB_LOGO");

  _bt_1 = IupButton("OK", "imlabAboutOK");
  IupSetAttribute(_bt_1, "PADDING", "30X3");
  IupSetHandle("btAboutOk", _bt_1);

  _vbox_2 = IupVbox(
    lbl,
    IupSetAttributes(IupLabel(IMLAB_TITLE), "FONT=\"Helvetica, Bold 14\""),
    IupLabel(IMLAB_BUILD),
    IupLabel("Copyright © 1995-2019 Antonio Scuri"),
    IupSetAttributes(txt = IupText(NULL), "READONLY=Yes, BORDER=NO, VALUE=\"scuri@tecgraf.puc-rio.br\", VISIBLECOLUMNS=12, ALIGNMENT=ACENTER"),
    _bt_1,
    NULL);

  IupSetAttribute(_vbox_2, "ALIGNMENT", "ACENTER");
  IupSetAttribute(_vbox_2, "MARGIN", "5x5");
  IupSetAttribute(_vbox_2, "GAP", "9");
  IupSetAttribute(_vbox_2, "FONT", "Helvetica, 12");

  dlgAbout = IupDialog(_vbox_2);
  IupSetAttribute(dlgAbout, "TITLE", "About");
  IupSetAttribute(dlgAbout, "RESIZE", "NO");
  IupSetAttribute(dlgAbout, "MINBOX", "NO");
  IupSetAttribute(dlgAbout, "MAXBOX", "NO");
  IupSetAttribute(dlgAbout, "MENUBOX", "NO");
  IupSetAttribute(dlgAbout, "DEFAULTENTER", "btAboutOk");
  IupSetAttribute(dlgAbout, "DEFAULTESC", "btAboutOk");
  IupSetAttribute(dlgAbout, "PARENTDIALOG", "imlabMainWindow");

  IupSetStrAttribute(txt, "BGCOLOR", IupGetAttribute(dlgAbout, "BGCOLOR"));

  IupPopup(dlgAbout, IUP_CENTERPARENT, IUP_CENTERPARENT);
  IupDestroy(dlgAbout);

  return IUP_DEFAULT;
}


/************************************************************************************/


Ihandle* imlabMainWindowCreateMenu(void)
{
  Ihandle *mnFile, *mnImport, *mnSaveLog, *mnHelp, *mnWindow,
          *mnPaste, *mnCopyLog, *mnWindowSaveAll, *mnWindowCloseAll, 
          *mnWindowCascade, *mnWindowTile, *mnWindowList, *mnMainMenu, *mnCapture,
          *mnOpen, *mnNew, *mnWindowListResults, *mnWindowNext, *mnRecent, *mnExport;
  Ihandle *mnEdit, *mnView, *mnPseudoColor, *mnProcess, *mnAnalyze,
    *ByteMenu, *ShortMenu, *UShortMenu, *IntMenu, *FloatMenu, *CFloatMenu, *DoubleMenu, *CDoubleMenu, *RevertMenu, *SaveMenu,
    *mnImage, *mnDataType, *mnComplexView, *mnGammaView, *mnDuplicate, *mnCopy,
    *mnUndo, *mnClearUndo, *mnRedo, *mnEditPalette,
    *mnCpxReal, *mnCpxImag, *mnCpxMag, *mnCpxPhase, *mnModeMinMax, *mnModeFixed, *mnModeDirect, *mnModeView,
    *mnAbss, *mnLogLite, *mnLogHeavy, *mnExpLite, *mnExpHeavy, *mnCustom, *mnLinear, *mnColorSpace,
    *mnBinary, *mnGray, *mnMap, *mnRGB, *mnCMYK, *mnYCbCr, *mnLab, *mnLuv, *mnXYZ;

  mnExport = IupMenu(
    IupSetAttributes(imlabItem("RAW...", "imlabExportRAW"), "imlabStatusHelp=\"Steps: 1-file name, 2-RAW data parameters.\""),
    IupSeparator(),
    imlabItem("SVG...", "imlabExportSVG"),
    imlabItem("EPS...", "imlabExportEPS"),
    imlabItem("CGM...", "imlabExportCGM"),
#ifdef WIN32
    imlabItem("EMF...", "imlabExportEMF"),
    imlabItem("WMF...", "imlabExportWMF"),
#endif
    NULL);

  mnImport = IupMenu(
    IupSetAttributes(imlabItem("RAW...", "imlabImportRAW"), "imlabStatusHelp=\"Steps: 1-file name, 2-RAW data parameters, 3-new image parameters.\""),
    IupSeparator(),
    IupSetAttributes(imlabItem("CGM...", "imlabImportCGM"), "imlabStatusHelp=\"Steps: 1-file name, 2-new image param. (size only).\""),
#ifdef WIN32
    imlabItem("WMF...", "imlabImportWMF"),
    imlabItem("EMF...", "imlabImportEMF"),
#endif
    NULL);

  mnRecent = IupMenu(
    NULL);
  IupConfigRecentInit(imlabConfig(), mnRecent, cmRecent, 10);

#if defined(WIN32) && defined(USE_VIDEOCAPTURE)
  mnCapture = IupMenu(NULL);
  imlabInitCaptureMenu(mnCapture);
#endif

  mnFile = IupMenu(
    mnNew = imlabItem( "New...", "imlabNew"),
    mnOpen = IupSetAttributes(imlabItem( "Open...\t<Ctrl+O>", "imlabOpen" ), "KEY=K_O"),
    imlabItem( "Open (w/ Preview)...", "imlabOpenPreview"),
    RevertMenu = imlabItem("Revert", "imlabRevert"),
    IupSetAttributes(SaveMenu = imlabItem("Save\t<Ctrl+S>", "imlabSave"), "IMAGE=IUP_FileSave"),
    IupSetAttributes(imlabItem("Save As...", "imlabSaveAs"), "CHECK_HASIMAGE=Yes"),
    IupSetAttributes(mnWindowSaveAll = imlabItem("Save All", "imlabSaveAll"), "CHECK_HASIMAGE=Yes"),
    IupSetAttributes(imlabItem("Save Multiple...", "imlabSaveMulti"), "imlabStatusHelp=\"Save multiple images in a single file.\", CHECK_HASIMAGE=Yes"),
    IupSetAttributes(imlabItem("Close", "imlabClose"), "IMAGE=IUP_FileClose, imlabStatusHelp=\"Close the active image file. Press Shift to ignore changes.\", CHECK_HASIMAGE=Yes"),
    IupSetAttributes(mnWindowCloseAll = imlabItem("Close All", "imlabCloseAll"), "IMAGE=IUP_FileCloseAll, imlabStatusHelp=\"Close all image files. Press Shift to ignore changes.\", CHECK_HASIMAGE=Yes"),
    IupSeparator(),
    IupSetAttributes(imlabSubmenu("Import", mnImport), "KEY=K_I"),
    IupSetAttributes(imlabSubmenu("Export", mnExport), "KEY=K_E, CHECK_HASIMAGE=Yes"),
    IupSeparator(),
#if defined(WIN32) && defined(USE_VIDEOCAPTURE)
    IupSetAttributes(imlabSubmenu("Capture", mnCapture), "IMAGE=IUP_Webcam"),
    IupSeparator(),
#endif
    IupSetAttributes(imlabItem("Print...", "imlabPrint"), "IMAGE=IUP_Print, CHECK_HASIMAGE=Yes"),
    IupSetAttributes(imlabItem("Print to PDF...", "imlabPrintPDF"), "CHECK_HASIMAGE=Yes"),
    IupSetAttributes(imlabItem("Print to PS...", "imlabPrintPS"), "CHECK_HASIMAGE=Yes"),
    IupSeparator(),
    mnSaveLog = imlabItem("Save Log...", "imlabSaveLog"),
    IupSeparator(),
    IupSetAttributes(imlabSubmenu("Recent Files", mnRecent), "KEY=K_R"),
    IupSeparator(),
    imlabItem( "Exit", "imlabMainExit" ),
    NULL);

  mnEdit = IupMenu(
    IupSetAttributes(mnUndo = imlabItem("Undo\t<Ctrl+Z>", "imlabUndo"), "imlabStatusHelp=\"Undo last change.\""),
    IupSetAttributes(mnRedo = imlabItem("Redo\t<Ctrl+Y>", "imlabRedo"), "imlabStatusHelp=\"Redo last change.\""),
    IupSetAttributes(mnClearUndo = imlabItem("Clear Undo", "imlabClearUndo"), "imlabStatusHelp=\"Clear Undo Data.\""),
    IupSeparator(),
    IupSetAttributes(mnCopy = imlabItem("Copy\t<Ctrl+C>", "imlabCopy"), "IMAGE=IUP_EditCopy"),
    IupSetAttributes(mnPaste = imlabItem("Paste New", "imlabPaste"), "IMAGE=IUP_EditPaste"),
    mnDuplicate = imlabItem("Duplicate\t<Ctrl+D>", "imlabDuplicate"),
    IupSeparator(),
    mnCopyLog = imlabItem("Copy Log Text", "imlabCopyLog"),
    NULL);

  mnModeView = IupMenu(
    IupSetAttributes(mnModeMinMax = imlabItem("Scan Min-Max", "imlabModeMinMax"), "VALUE=OFF, imlabStatusHelp=\"Scale to data min and max.\""),
    IupSetAttributes(mnModeFixed = imlabItem("Fixed Min-Max", "imlabModeFixed"), "VALUE=OFF, imlabStatusHelp=\"Scale to fixed min and max.\""),
    IupSetAttributes(mnModeDirect = imlabItem("Direct Cast", "imlabModeDirect"), "VALUE=OFF, imlabStatusHelp=\"Direct type cast without scaling.\""),
    IupSeparator(),
    IupSetAttributes(mnAbss = imlabItem("Absolute", "imlabAbss"), "VALUE=OFF"),
    NULL);

  mnGammaView = IupMenu(
    IupSetAttributes(mnLinear = imlabItem("Linear", "imlabLinear"), "VALUE=ON"),
    IupSetAttributes(mnLogLite = imlabItem("Logarithm (Light)", "imlabLogLite"), "VALUE=OFF, imlabStatusHelp=\"log(K*I+1), where K=10.\""),
    IupSetAttributes(mnLogHeavy = imlabItem("Logarithm (Heavy)", "imlabLogHeavy"), "VALUE=OFF, imlabStatusHelp=\"log(K*I+1), where K=1000.\""),
    IupSetAttributes(mnExpLite = imlabItem("Exponential (Light)", "imlabExpLite"), "VALUE=OFF, imlabStatusHelp=\"exp(K*I)-1, where K=2.\""),
    IupSetAttributes(mnExpHeavy = imlabItem("Exponential (Heavy)", "imlabExpHeavy"), "VALUE=OFF, imlabStatusHelp=\"exp(K*I)-1, where K=7.\""),
    IupSeparator(),
    IupSetAttributes(mnCustom = imlabItem("Custom...", "imlabCustom"), "VALUE=OFF, imlabStatusHelp=\"K<0 use Log(abs(K)), K>0 use exp(K).\""),
    NULL);

  mnPseudoColor = IupMenu(
    imlabItem("Gray", "imlabPalGray"),
    IupSeparator(),
    imlabItem("Red", "imlabPalRed"),
    imlabItem("Green", "imlabPalGreen"),
    imlabItem("Blue", "imlabPalBlue"),
    IupSeparator(),
    imlabItem("Cyan", "imlabPalCyan"),
    imlabItem("Magenta", "imlabPalMagenta"),
    imlabItem("Yellow", "imlabPalYellow"),
    IupSeparator(),
    imlabItem("Uniform", "imlabPalUniform"),
    imlabItem("Linear", "imlabPalLinear"),
    IupSeparator(),
    imlabItem("Hues", "imlabPalHues"),
    imlabItem("Rainbow", "imlabPalRainbow"),
    imlabItem("Blue Ice", "imlabPalBlueIce"),
    imlabItem("Hot Iron", "imlabPalHotIron"),
    imlabItem("Black Body", "imlabPalBlackBody"),
    imlabItem("High Contrast", "imlabPalHighContrast"),
    NULL);

  mnComplexView = IupMenu(
    IupSetAttributes(mnCpxReal = imlabItem("Real", "imlabCpxReal"), "VALUE=OFF"),
    IupSetAttributes(mnCpxImag = imlabItem("Imaginary", "imlabCpxImag"), "VALUE=OFF"),
    IupSetAttributes(mnCpxMag = imlabItem("Magnitude", "imlabCpxMag"), "VALUE=OFF"),
    IupSetAttributes(mnCpxPhase = imlabItem("Phase", "imlabCpxPhase"), "VALUE=OFF"),
    NULL);

  mnView = IupMenu(
    IupSetAttributes(imlabSubmenu("Type Conversion", mnModeView), "imlabStatusHelp=\"Used during type conversion.\""),
    IupSetAttributes(imlabSubmenu("Real Gamma", mnGammaView), "imlabStatusHelp=\"Used to quantize real images.\""),
    IupSetAttributes(imlabSubmenu("Complex to Real", mnComplexView), "imlabStatusHelp=\"Used to view complex images.\""),
    IupSeparator(),
    IupSetAttributes(imlabSubmenu("Pseudo Colors", mnPseudoColor), "imlabStatusHelp=\"Changes the image palette of gray scale images.\""),
    IupSeparator(),
    IupSetAttributes(imlabItem("Full Screen...", "imlabFullScreen"), "IMAGE=IUP_ViewFullScreen"),
    imlabItem("Fit Full Screen...", "imlabFitFullScreen"),
    IupSeparator(),
    IupSetAttributes(imlabItem("New Bitmap", "imlabNewBitmapView"), "IMAGE=imImgWinBitmapButton"),
    IupSetAttributes(imlabItem("New Histogram", "imlabNewHistogramView"), "IMAGE=imImgWinHistogramButton"),
    IupSetAttributes(imlabItem("New Matrix", "imlabNewMatrixView"), "IMAGE=imImgWinMatrixButton"),
    IupSetAttributes(imlabItem("New 3D", "imlabNew3DView"), "IMAGE=imImgWin3DButton"),
    NULL);

  mnDataType = IupMenu(
    IupSetAttributes(ByteMenu = imlabItem("Byte", "imlabByte"), "VALUE=OFF, imlabStatusHelp=\"To Byte. Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(ShortMenu = imlabItem("Short", "imlabShort"), "VALUE=OFF, imlabStatusHelp=\"To Signed Short Integer (16 bits). Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(UShortMenu = imlabItem("Unsigned Short", "imlabUShort"), "VALUE=OFF, imlabStatusHelp=\"To Unsigned Short Integer (16 bits). Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(IntMenu = imlabItem("Integer", "imlabInt"), "VALUE=OFF, imlabStatusHelp=\"To Integer. Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(FloatMenu = imlabItem("Float", "imlabFloat"), "VALUE=OFF, imlabStatusHelp=\"To Float. Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(DoubleMenu = imlabItem("Double", "imlabDouble"), "VALUE=OFF, imlabStatusHelp=\"To Double. Dependent of the View/TypeConvertion, View/RealGamma and View/ComplexToReal options.\""),
    IupSetAttributes(CFloatMenu = imlabItem("Complex Float", "imlabCFloat"), "VALUE=OFF, imlabStatusHelp=\"To Complex Float.\""),
    IupSetAttributes(CDoubleMenu = imlabItem("Complex Double", "imlabCDouble"), "VALUE=OFF, imlabStatusHelp=\"To Complex Double.\""),
    NULL);

  mnColorSpace = IupMenu(
    IupSetAttributes(mnBinary = imlabItem("Binary", "imlabBinary"), "VALUE=OFF, imlabStatusHelp=\"To Binary. Indexed by 2 colors: black (0) and white (1).\""),
    IupSetAttributes(mnGray = imlabItem("Gray", "imlabGray"), "VALUE=OFF, imlabStatusHelp=\"To Gray (Y'). Shades of gray, luma (nonlinear Luminance), or an intensity value that is not related to color.\""),
    IupSetAttributes(mnMap = imlabItem("Map", "imlabMap"), "VALUE=OFF, imlabStatusHelp=\"To Map. Indexed by RGB color map.\""),
    IupSetAttributes(mnRGB = imlabItem("RGB", "imlabRGB"), "VALUE=OFF, imlabStatusHelp=\"To R'G'B'. Red, Green and Blue (nonlinear).\""),
    IupSetAttributes(mnCMYK = imlabItem("CMYK", "imlabCMYK"), "VALUE=OFF, imlabStatusHelp=\"To CMYK. Cyan, Magenta, Yellow and Black (nonlinear).\""),
    IupSetAttributes(mnYCbCr = imlabItem("YCbCr", "imlabYCbCr"), "VALUE=OFF, imlabStatusHelp=\"To Y'CbCr. ITU-R 601 Y' is luma (nonlinear Luminance).\""),
    IupSetAttributes(mnLab = imlabItem("Lab", "imlabLab"), "VALUE=OFF, imlabStatusHelp=\"To CIE L*a*b*. L* is Lightness (nonlinear Luminance, nearly perceptually uniform).\""),
    IupSetAttributes(mnLuv = imlabItem("Luv", "imlabLuv"), "VALUE=OFF, imlabStatusHelp=\"To CIE L*u*v*. L* is Lightness (nonlinear Luminance, nearly perceptually uniform).\""),
    IupSetAttributes(mnXYZ = imlabItem("XYZ", "imlabXYZ"), "VALUE=OFF, imlabStatusHelp=\"To CIE XYZ. Linear Light Tristimulus, Y is linear Luminance.\""),
    NULL);

  mnImage = IupMenu(
    IupSetAttributes(imlabItem("Information...", "imlabImageFileInfo"), "IMAGE=IUP_MessageInfo"),
    IupSetAttributes(imlabItem("Attributes...", "imlabImageFileAttrib"), "IMAGE=IUP_FileProperties"),
    IupSetAttributes(mnEditPalette = imlabItem("Palette...", "imlabImageFilePalette"), "IMAGE=IUP_ToolsColor"),
    IupSetAttributes(imlabItem("Rename...", "imlabImageRename"), "imlabStatusHelp=\"Rename the title, without affecting the original file.\""),
    IupSeparator(),
    IupSetAttributes(imlabSubmenu("Color Space", mnColorSpace), "imlabStatusHelp=\"Changes the image color space.\""),
    IupSetAttributes(imlabSubmenu("Data Type", mnDataType), "imlabStatusHelp=\"Changes the image data type.\""),
    NULL);

  mnProcess = IupMenu(NULL);
  imlabProcPlugInInit(mnProcess);

  mnAnalyze = IupMenu(NULL);
  imlabAnaPlugInInit(mnAnalyze);

  mnWindow = IupMenu(
    mnWindowCascade = imlabItem( "Cascade", "imlabCascade"),
    mnWindowTile = imlabItem("Tile", "imlabTile"),
    IupSeparator(),
    IupSetAttributes(mnWindowNext = imlabItem("Next Image\t<Ctrl+N>", "imlabWindowNext"), "KEY=K_N"),
    mnWindowList = imlabItem("Image List...", "imlabListDocuments"),
    mnWindowListResults = imlabItem( "Results List...", "imlabListResults"),
    NULL);

  mnHelp = IupMenu(
    IupSetAttributes(imlabItem("Web Site...", "imlabWeb"), "imlabStatusHelp=\"http://imlab.sourceforge.net/\""),
    IupSeparator(),
    IupSetAttributes(imlabItem( "About...", "imlabAbout"), "IMAGE=IMLAB_Logo"),
    NULL);

  mnMainMenu = IupMenu(
    IupSetAttributes(imlabSubmenu("File",mnFile), "KEY=K_F"),
    IupSetAttributes(imlabSubmenu("Edit", mnEdit), "KEY=K_E"),
    IupSetAttributes(imlabSubmenu("View", mnView), "KEY=K_V"),
    IupSetAttributes(imlabSubmenu("Image",mnImage), "KEY=K_I"),
    IupSetAttributes(imlabSubmenu("Process",mnProcess), "KEY=K_P"),
    IupSetAttributes(imlabSubmenu("Analyze", mnAnalyze), "KEY=K_A"),
    IupSetAttributes(imlabSubmenu("Window", mnWindow), "KEY=K_W"),
    IupSetAttributes(imlabSubmenu("Help", mnHelp), "KEY=K_H"),
    NULL);

  IupSetAttribute(mnFile, "OPEN_CB", "imlabFileMenuOpen");
  IupSetAttribute(mnEdit, "OPEN_CB", "imlabEditMenuOpen");
  IupSetAttribute(mnView, "OPEN_CB", "imlabViewMenuOpen");
  IupSetAttribute(mnImage, "OPEN_CB", "imlabImageMenuOpen");
  IupSetAttribute(mnProcess, "OPEN_CB", "imlabProcessMenuOpen");
  IupSetAttribute(mnAnalyze, "OPEN_CB", "imlabAnalyzeMenuOpen");
  IupSetAttribute(mnWindow, "OPEN_CB", "imlabWindowMenuOpen");

  IupSetAttribute(mnWindow, "mnCascade", (char*)mnWindowCascade);
  IupSetAttribute(mnWindow, "mnTile", (char*)mnWindowTile);
  IupSetAttribute(mnWindow, "mnListImages", (char*)mnWindowList);
  IupSetAttribute(mnWindow, "mnNextImage", (char*)mnWindowNext);
  IupSetAttribute(mnWindow, "mnListResults", (char*)mnWindowListResults);

  IupSetAttribute(mnNew,"IMAGE","IUP_FileNew");
  IupSetAttribute(mnOpen, "IMAGE", "IUP_FileOpen");
  IupSetAttribute(mnWindowSaveAll,"IMAGE","IUP_FileSaveAll");
  IupSetAttribute(mnWindowCascade,"IMAGE","IUP_WindowsCascade");
  IupSetAttribute(mnWindowTile,"IMAGE","IUP_WindowsTile");

  IupSetAttribute(mnEdit, "mnPaste", (char*)mnPaste);
  IupSetAttribute(mnEdit, "mnUndo", (char*)mnUndo);
  IupSetAttribute(mnEdit, "mnRedo", (char*)mnRedo);
  IupSetAttribute(mnEdit, "mnClearUndo", (char*)mnClearUndo);
  IupSetAttribute(mnEdit, "mnDuplicate", (char*)mnDuplicate);
  IupSetAttribute(mnEdit, "mnCopy", (char*)mnCopy);

  IupSetAttribute(mnMainMenu, "CDoubleMenu", (char*)CDoubleMenu);
  IupSetAttribute(mnMainMenu, "DoubleMenu", (char*)DoubleMenu);
  IupSetAttribute(mnMainMenu, "CFloatMenu", (char*)CFloatMenu);
  IupSetAttribute(mnMainMenu, "FloatMenu", (char*)FloatMenu);
  IupSetAttribute(mnMainMenu, "ShortMenu", (char*)ShortMenu);
  IupSetAttribute(mnMainMenu, "UShortMenu", (char*)UShortMenu);
  IupSetAttribute(mnMainMenu, "IntMenu", (char*)IntMenu);
  IupSetAttribute(mnMainMenu, "ByteMenu", (char*)ByteMenu);
  IupSetAttribute(mnMainMenu, "RevertMenu", (char*)RevertMenu);
  IupSetAttribute(mnMainMenu, "SaveMenu", (char*)SaveMenu);
  IupSetAttribute(mnMainMenu, "mnCpxReal", (char*)mnCpxReal);
  IupSetAttribute(mnMainMenu, "mnCpxImag", (char*)mnCpxImag);
  IupSetAttribute(mnMainMenu, "mnCpxMag", (char*)mnCpxMag);
  IupSetAttribute(mnMainMenu, "mnCpxPhase", (char*)mnCpxPhase);
  IupSetAttribute(mnMainMenu, "mnAbss", (char*)mnAbss);
  IupSetAttribute(mnMainMenu, "mnLogLite", (char*)mnLogLite);
  IupSetAttribute(mnMainMenu, "mnLogHeavy", (char*)mnLogHeavy);
  IupSetAttribute(mnMainMenu, "mnExpLite", (char*)mnExpLite);
  IupSetAttribute(mnMainMenu, "mnExpHeavy", (char*)mnExpHeavy);
  IupSetAttribute(mnMainMenu, "mnCustom", (char*)mnCustom);
  IupSetAttribute(mnMainMenu, "mnLinear", (char*)mnLinear);
  IupSetAttribute(mnMainMenu, "mnPseudoColor", (char*)mnPseudoColor);
  IupSetAttribute(mnMainMenu, "mnModeMinMax", (char*)mnModeMinMax);
  IupSetAttribute(mnMainMenu, "mnModeFixed", (char*)mnModeFixed);
  IupSetAttribute(mnMainMenu, "mnModeDirect", (char*)mnModeDirect);
  IupSetAttribute(mnMainMenu, "mnBinary", (char*)mnBinary);
  IupSetAttribute(mnMainMenu, "mnGray", (char*)mnGray);
  IupSetAttribute(mnMainMenu, "mnMap", (char*)mnMap);
  IupSetAttribute(mnMainMenu, "mnRGB", (char*)mnRGB);
  IupSetAttribute(mnMainMenu, "mnCMYK", (char*)mnCMYK);
  IupSetAttribute(mnMainMenu, "mnYCbCr", (char*)mnYCbCr);
  IupSetAttribute(mnMainMenu, "mnLab", (char*)mnLab);
  IupSetAttribute(mnMainMenu, "mnLuv", (char*)mnLuv);
  IupSetAttribute(mnMainMenu, "mnXYZ", (char*)mnXYZ);
  IupSetAttribute(mnMainMenu, "mnEditPalette", (char*)mnEditPalette);

  IupSetFunction("imlabEditMenuOpen", (Icallback)cmEditMenuOpen);
  IupSetFunction("imlabUndo", (Icallback)cmUndo);
  IupSetFunction("imlabRedo", (Icallback)cmRedo);
  IupSetFunction("imlabClearUndo", (Icallback)cmClearUndo);
  IupSetFunction("imlabDuplicate", (Icallback)cmDuplicate);
  IupSetFunction("imlabCopy", (Icallback)cmCopy);
  IupSetFunction("imlabPaste", (Icallback)cmPaste);

  IupSetFunction("imlabProcessMenuOpen", (Icallback)cmProcessMenuOpen);
  IupSetFunction("imlabAnalyzeMenuOpen", (Icallback)cmAnalyzeMenuOpen);

  IupSetFunction("imlabWindowMenuOpen", (Icallback)cmWindowMenuOpen);
  IupSetFunction("imlabCascade", (Icallback)cmCascade);
  IupSetFunction("imlabTile", (Icallback)cmTile);
  IupSetFunction("imlabWindowNext", (Icallback)cmWindowNext);
  IupSetFunction("imlabListDocuments", (Icallback)cmListDocuments);
  IupSetFunction("imlabListResults", (Icallback)cmListResults);

  IupSetFunction("imlabWeb", (Icallback)cmWeb);
  IupSetFunction("imlabAboutOK", (Icallback)cmAboutOK);
  IupSetFunction("imlabAbout", (Icallback)cmAbout);

  imlabMainWindowRegisterFileMenu();
  imlabMainWindowRegisterViewMenu();
  imlabMainWindowRegisterImageMenu();

  return mnMainMenu;
}


#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <cd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>

#include <cdprint.h>
#include <cdpdf.h>
#include <cdps.h>
#include <cdiup.h>
#include <cdclipbd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>
#include <cdsvg.h>

#include <iup.h>
#include <iupcontrols.h>

#include <im.h>
#include <im_image.h>
#include <im_convert.h>
#include <im_palette.h>

#include "mainwindow.h"
#include "im_imagematch.h"
#include "im_imageview.h"
#include "imagewindow.h"
#include "imagedocument.h"
#include "documentlist.h"
#include "resultswindow.h"
#include "imlab.h"
#include "dialogs.h"
#include "utl_file.h"
#include "im_clipboard.h"
#include <iup_config.h>


static void iImgDocSaveAs(imlabImageDocument* document)
{
  /* Calls the predefined SaveAs dialog box */
  if (!imlabDlgImageFileSaveAs(document->ImageFile->image,
                               document->ImageFile->filename,
                               document->ImageFile->format,
                               document->ImageFile->compression,
                               0))
    return;

  /* save the file */
  if (!imlabImageFileSave(&document->ImageFile, 1))
    return;

  IupConfigRecentUpdate(imlabConfig(), document->ImageFile->filename);

  document->HasChanged();
}

static void iImgDocSaveCheck(imlabImageDocument* document)
{
  if (document->ImageFile->format[0] == 0)
  {
    iImgDocSaveAs(document);
    return;
  }
  else
  {
    if (document->ImageFile->index != -1)
    {
      document->ShowView("Bitmap");

      int ret = imlabDlgQuestion("This image is part of a sequence. Ignore other images?\n\nTIP: Use \"Save Multiple\" for multiple imagens in a single file.", 0);
      if (ret != 1)
        return;
    }
  }

  if (document->ImageFile->changed)
  {
    /* Saves the image */
    if (!imlabImageFileSave(&document->ImageFile, 1))
    {
      iImgDocSaveAs(document);
      return;
    }

    IupConfigRecentUpdate(imlabConfig(), document->ImageFile->filename);

    document->HasChanged();
  }
}


/*******************************/
/* File Menu call backs        */
/*******************************/


static int cmNew(void)
{
  static int width = 300, height = 300, color_space = IM_GRAY, data_type = IM_BYTE;

  if (!imlabDlgGetNewImageParam(&width, &height, &color_space, &data_type))
    return IUP_DEFAULT;

  imlabImageDocumentCreateNewImage(width, height, color_space, data_type);

  return IUP_DEFAULT;
}

static int cmOpen(void)
{
  char filename[10240] = "*.*";

  /* Retrieve a file name */
  if (!imlabDlgGetOpenImageFileName(filename))
   return IUP_DEFAULT;

#ifndef MOTIF
  /* parse multiple files */
  {
    int offset;
    char* dir = utlFileGetPathMultiple(filename, &offset);
    if (dir)
    {
      char* fn;
      while ((fn = utlFileGetFilenameMultiple(dir, filename + offset, &offset)) != NULL)
      {
        imlabImageDocumentCreateFromFileName(fn);
        free(fn);
      }
      free(dir);
    }
    else
      imlabImageDocumentCreateFromFileName(filename);
  }
#else
  imlabImageDocumentCreateFromFileName(filename);
#endif

  return IUP_DEFAULT;
}

static int cmOpenPreview(void)
{
  char filename[10240] = "*.*";

  /* Retrieve a file name */
  if (!imlabDlgGetOpenImageFileNamePreview(filename))
   return IUP_DEFAULT;

#ifndef MOTIF
  /* parse multiple files */
  {
    int offset;
    char* dir = utlFileGetPathMultiple(filename, &offset);
    if (dir)
    {
      char* fn;
      while ((fn = utlFileGetFilenameMultiple(dir, filename + offset, &offset)) != NULL)
      {
        imlabImageDocumentCreateFromFileName(fn);
        free(fn);
      }
      free(dir);
    }
    else
      imlabImageDocumentCreateFromFileName(filename);
  }
#else
  imlabImageDocumentCreateFromFileName(filename);
#endif

  return IUP_DEFAULT;
}

static int cmImportCGM(void)
{
  char filename[10240] = "*.cgm";
  imImage* NewImage;

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "OPEN", "Import Picture", "Computer Graphics Metafile (CGM)|*.cgm|All Files|*.*|", "LastPictureDirectory"))
   return IUP_DEFAULT;

  NewImage = imImageViewImportImage(CD_CGM, filename);
  if (NewImage)
  {
    char* file_title = utlFileGetTitle(filename);
    imlabImageDocumentCreateFromImage(NewImage, file_title, "ImportCGM{filename=\"%s\"}", filename);
    free(file_title);
  }

  return IUP_DEFAULT;
}

static int cmImportWMF(void)
{
  char filename[10240] = "*.wmf";
  imImage* NewImage;

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "OPEN", "Import Picture", "Windows Metafile (WMF)|*.wmf|All Files|*.*|", "LastPictureDirectory"))
   return IUP_DEFAULT;

  NewImage = imImageViewImportImage(CD_WMF, filename);
  if (NewImage)
  {
    char* file_title = utlFileGetTitle(filename);
    imlabImageDocumentCreateFromImage(NewImage, file_title, "ImportWMF{filename=\"%s\"}", filename);
    free(file_title);
  }

  return IUP_DEFAULT;
}

static int cmImportEMF(void)
{
  char filename[10240] = "*.emf";
  imImage* NewImage;

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "OPEN", "Import Picture", "Windows Enhanced Metafile (EMF)|*.emf|All Files|*.*|", "LastPictureDirectory"))
   return IUP_DEFAULT;

  NewImage = imImageViewImportImage(CD_EMF, filename);
  if (NewImage)
  {
    char* file_title = utlFileGetTitle(filename);
    imlabImageDocumentCreateFromImage(NewImage, file_title, "ImportEMF{filename=\"%s\"}", filename);
    free(file_title);
  }

  return IUP_DEFAULT;
}

static int cmImportRAW(void)
{
  static int top_down = 1, width = 300, height = 300, color_space = IM_GRAY, data_type = IM_BYTE,
         switch_type = 0, byte_order = -1, packed = 0, padding = 1, ascii = 0;
  static unsigned long offset = 0;
  char filename[10240] = "*.*";
  imImage* NewImage;

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "OPEN", "Import Image", "All Files|*.*|", "LastImageDirectory"))
   return IUP_DEFAULT;

  if (byte_order == -1)
    byte_order = imBinCPUByteOrder();

  if (!imlabDlgGetRAWImageParam(&top_down, &switch_type, &byte_order, &packed, &padding, &offset, &ascii))
    return IUP_DEFAULT;

  if (!imlabDlgGetNewImageParam(&width, &height, &color_space, &data_type))
    return IUP_DEFAULT;

  NewImage = imImageCreate(width, height, color_space, data_type);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  {
    int err = imlabRawLoadImage(filename, NewImage, top_down, switch_type, byte_order, packed, padding, offset, ascii);
    if (err)
    {
      if (err != IM_ERR_ACCESS)
        return IUP_DEFAULT;
    }

    {
      char* file_title = utlFileGetTitle(filename);
      imlabImageDocumentCreateFromImage(NewImage, file_title, "ImportRAW{filename=\"%s\", top_down=%d, switch_type=%d, byte_order=%d, packed=%d, padding=%d, offset=%d, ascii=%d}", filename, top_down, switch_type, byte_order, packed, padding, offset, ascii);
      free(file_title);
    }
  }

  return IUP_DEFAULT;
}

static int cmExportRAW(Ihandle* dialog)
{
  static int switch_type = 0, top_down = 1, byte_order = -1, packed = 0, padding = 1, ascii = 0;
  static unsigned long offset = 0;
  char filename[10240] = "*.*";
  imlabImageDocument* document = imlabGetCurrentDocument(dialog);
  imImage* image = document->ImageFile->image;

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "SAVE", "Save Image", "All Files|*.*|", "LastImageDirectory"))
    return IUP_DEFAULT;

  if (byte_order == -1)
    byte_order = imBinCPUByteOrder();

  if (!imlabDlgGetRAWImageParam(&top_down, &switch_type, &byte_order, &packed, &padding, &offset, &ascii))
    return IUP_DEFAULT;

  {
    int err = imlabRawSaveImage(filename, image, top_down, switch_type, byte_order, packed, padding, offset, ascii);
    if (err)
      return IUP_DEFAULT;
  }

  return IUP_DEFAULT;
}

static int cmCloseAll(void)
{
  int i;

  i = imlabImageDocumentListCount()-1;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    if (!document->Close())
      return IUP_DEFAULT;

    i--;
    document = imlabImageDocumentListGet(i);
  }

  i = imlabResultsWindowListCount()-1;
  imlabResultsWindow* results_window = imlabResultsWindowListGet(i);
  while (results_window)
  {
    if (!imlabResultsWindowClose(results_window))
      return IUP_DEFAULT;

    i--;
    results_window = imlabResultsWindowListGet(i);
  }

  return IUP_DEFAULT;
}
  
static int cmSaveAll(void)
{
  int i = 0;
  imlabImageDocument* document = imlabImageDocumentListGet(i);
  while (document)
  {
    iImgDocSaveCheck(document);

    i++;
    document = imlabImageDocumentListGet(i);
  }

  return IUP_DEFAULT;
}
  
static int cmMainExit(void)
{
  cmCloseAll();

  if (imlabImageDocumentListCount() == 0)
  {
    Ihandle* main_window = IupGetHandle("imlabMainWindow");
    IupConfigDialogClosed(imlabConfig(), main_window, "MainDialog");
    IupConfigSave(imlabConfig());
    return IUP_CLOSE;
  }
  else
    return IUP_IGNORE;
}

static void UpdateSaveMenus(int changed, const char* format)
{
  Ihandle* mnMainMenu = IupGetHandle("mnMainMenu");
  Ihandle* SaveMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "SaveMenu");
  Ihandle* RevertMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "RevertMenu");

  if (changed)                         
  {                                   
    IupSetAttribute(SaveMenu, "ACTIVE", "YES");

    if (format[0] == 0)
      IupSetAttribute(RevertMenu, "ACTIVE", "NO");
    else
      IupSetAttribute(RevertMenu, "ACTIVE", "YES");
  }
  else
  {
    IupSetAttribute(SaveMenu, "ACTIVE", "NO");
    IupSetAttribute(RevertMenu, "ACTIVE", "NO");
  }
}

static int cmRevert(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imlabImageFile* NewImageFile;

  /* Re-loads the image */
  NewImageFile = *imlabImageFileOpen(document->ImageFile->filename, document->ImageFile->index);
  if (!NewImageFile)
    return IUP_DEFAULT;

  /* Swaps the new image file with the current image file */
  if (document->ImageFile) imlabImageFileDestroy(document->ImageFile, 1);
  document->ImageFile = NewImageFile;

  document->ClearUndo();
  document->HasChanged();
  document->UpdateViews();

  return IUP_DEFAULT;
}

static int cmSaveMulti(Ihandle* self)
{
  imlabImageDocument* document1 = imlabGetCurrentDocument(self);
  imlabImageFile* image_file_list[512];
  int image_count, i, j;

  imlabImageDocument** document_list = imlabImageDocumentListSelectMulti("Select Other Images", NULL, NULL, &image_count);
  if (!document_list)
    return IUP_DEFAULT;

  image_file_list[0] = document1->ImageFile;
  j = 1;

  for(i = 0; i < image_count; i++)
  {
    if (document_list[i] != document1)
    {
      image_file_list[j] = document_list[i]->ImageFile;
      j++;
    }
  }

  /* Calls the predefined SaveAs dialog box */
  if (!imlabDlgImageFileSaveAs(document1->ImageFile->image, 
                              document1->ImageFile->filename,
                              document1->ImageFile->format,
                              document1->ImageFile->compression,
                              1))
    return IUP_DEFAULT;

  /* save the file */
  if (!imlabImageFileSave(image_file_list, image_count))
    return IUP_DEFAULT;

  IupConfigRecentUpdate(imlabConfig(), document1->ImageFile->filename);

  for(i = 0; i < image_count; i++)
    document_list[i]->HasChanged();

  return IUP_DEFAULT;
}

static int cmSaveAs(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);

  /* Calls the predefined SaveAs dialog box */
  if (!imlabDlgImageFileSaveAs(document->ImageFile->image,
                               document->ImageFile->filename,
                               document->ImageFile->format,
                               document->ImageFile->compression,
                               0))
    return IUP_DEFAULT;

  /* save the file */
  if (!imlabImageFileSave(&document->ImageFile, 1))
    return IUP_DEFAULT;

  IupConfigRecentUpdate(imlabConfig(), document->ImageFile->filename);

  document->HasChanged();

  return IUP_DEFAULT;
}

static int cmSave(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document) // test because of the key action while menu disabled
    iImgDocSaveCheck(document);

  return IUP_DEFAULT;
}

static int cmPrint(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  int x, y, w, h, wc, hc,xmargin,ymargin;
  cdCanvas* cd_canvas;
  char str[1024];

  sprintf(str, "IMLAB %s -d", document->FileTitle);

  /* Activates the graphics cd_canvas */
  cd_canvas = cdCreateCanvas(CD_PRINTER, (void*)str);
  if (cd_canvas == NULL)
    return IUP_DEFAULT;

  cdCanvasActivate(cd_canvas);
  cdCanvasGetSize(cd_canvas, &wc, &hc, 0, 0);

  /* Sets the margins to 20% of the page dimensions */
  ymargin = hc / 20;
  xmargin = wc / 20;

  /* Ocupies a full page, but no distortion */
  imImageViewFitRect(wc - 2 * xmargin, hc - 2 * ymargin, 
                document->ImageFile->image->width, 
                document->ImageFile->image->height, 
                &w, &h);
  x = (wc - w) / 2;
  y = (hc - h) / 2;

  /* Paints the image with a margin */
  imImageViewDrawImage(cd_canvas, document->BitmapView.image, x, y, w, h);

  /* Terminates the graphics cd_canvas */
  cdKillCanvas(cd_canvas);

  return IUP_DEFAULT;
}

static void PrintTo(imlabImageDocument *document, cdContext* driver, const char *filename)
{
  int x, y, w, h, wc, hc, xmargin, ymargin;
  cdCanvas* cd_canvas;

  /* Activates the graphics cd_canvas */
  cd_canvas = cdCreateCanvas(driver, (void*)filename);
  if (cd_canvas == NULL)
  {
    IupMessagef("Error!", "Error creating file.\n  %s", filename);
    return;
  }

  cdCanvasActivate(cd_canvas);
  cdCanvasGetSize(cd_canvas, &wc, &hc, 0, 0);

  /* Sets the margins to 20% of the page dimensions */
  ymargin = hc / 20;
  xmargin = wc / 20;

  /* Ocupies a full page, but no distortion */
  imImageViewFitRect(wc - 2 * xmargin, hc - 2 * ymargin,
    document->ImageFile->image->width,
    document->ImageFile->image->height,
    &w, &h);
  x = (wc - w) / 2;
  y = (hc - h) / 2;

  /* Paints the image with a margin */
  imImageViewDrawImage(cd_canvas, document->BitmapView.image, x, y, w, h);

  /* Terminates the graphics cd_canvas */
  cdKillCanvas(cd_canvas);
}

static int cmPrintPS(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.ps";

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "SAVE", "Print to PS", "Postscript (PS)|*.ps|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;

  PrintTo(document, CD_PS, filename);

  return IUP_DEFAULT;
}

static int cmPrintPDF(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.pdf";

  /* Retrieve a file name */
  if (!imlabDlgSelectFile(filename, "SAVE", "Print to PDF", "Acrobat PDF|*.pdf|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;

  PrintTo(document, CD_PDF, filename);

  return IUP_DEFAULT;
}

static int cmExportCGM(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.cgm";
  if (!imlabDlgSelectFile(filename, "SAVE", "Export Picture", "Computer Graphics Metafile (CGM)|*.cgm|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;
  imImageViewExportImage(CD_CGM, document->BitmapView.image, filename);
  return IUP_DEFAULT;
}

static int cmExportSVG(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.svg";
  if (!imlabDlgSelectFile(filename, "SAVE", "Export Picture", "Scalable Vector Graphics (SVG)|*.svg|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;
  imImageViewExportImage(CD_SVG, document->BitmapView.image, filename);
  return IUP_DEFAULT;
}

static int cmExportWMF(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.wmf";
  if (!imlabDlgSelectFile(filename, "SAVE", "Export Picture", "Windows Metafile (WMF)|*.wmf|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;
  imImageViewExportImage(CD_WMF, document->BitmapView.image, filename);
  return IUP_DEFAULT;
}

static int cmExportEMF(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.emf";
  if (!imlabDlgSelectFile(filename, "SAVE", "Export Picture", "Windows Enhanced Metafile (EMF)|*.emf|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;
  imImageViewExportImage(CD_EMF, document->BitmapView.image, filename);
  return IUP_DEFAULT;
}

static int cmExportEPS(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char filename[10240] = "*.eps";
  if (!imlabDlgSelectFile(filename, "SAVE", "Export Picture", "Encapsulated Postscript (EPS)|*.eps|All Files|*.*|", "LastPictureDirectory"))
    return IUP_DEFAULT;
  strcat(filename, " -e");
  imImageViewExportImage(CD_PS, document->BitmapView.image, filename);
  return IUP_DEFAULT;
}

static int cmClose(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  document->Close();
  return IUP_DEFAULT;
}

static int cmFileMenuOpen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)
    UpdateSaveMenus(document->ImageFile->changed, document->ImageFile->format);
  else
    UpdateSaveMenus(0, "");

  int image_count = imlabImageDocumentListCount();
  Ihandle* child = IupGetNextChild(self, NULL);
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

  return IUP_DEFAULT;
}


void imlabMainWindowRegisterFileMenu(void)
{
  IupSetFunction("imlabFileMenuOpen", (Icallback) cmFileMenuOpen);
  
  IupSetFunction("imlabRevert", (Icallback) cmRevert);
  IupSetFunction("imlabSave", (Icallback) cmSave);
  IupSetFunction("imlabSaveAs", (Icallback) cmSaveAs);
  IupSetFunction("imlabSaveMulti", (Icallback) cmSaveMulti);
  IupSetFunction("imlabExportCGM", (Icallback) cmExportCGM);
  IupSetFunction("imlabExportSVG", (Icallback)cmExportSVG);
  IupSetFunction("imlabExportEPS", (Icallback)cmExportEPS);
  IupSetFunction("imlabExportEMF", (Icallback) cmExportEMF);
  IupSetFunction("imlabExportWMF", (Icallback) cmExportWMF);
  IupSetFunction("imlabPrint", (Icallback) cmPrint);
  IupSetFunction("imlabPrintPS", (Icallback) cmPrintPS);
  IupSetFunction("imlabPrintPDF", (Icallback)cmPrintPDF);
  IupSetFunction("imlabClose", (Icallback)cmClose);
  IupSetFunction("imlabNew", (Icallback) cmNew);
  IupSetFunction("imlabOpen", (Icallback) cmOpen);
  IupSetFunction("imlabOpenPreview", (Icallback) cmOpenPreview);
  IupSetFunction("imlabImportCGM", (Icallback) cmImportCGM);
  IupSetFunction("imlabImportWMF", (Icallback) cmImportWMF);
  IupSetFunction("imlabImportEMF", (Icallback) cmImportEMF);
  IupSetFunction("imlabImportRAW", (Icallback) cmImportRAW);
  IupSetFunction("imlabExportRAW", (Icallback) cmExportRAW);
  IupSetFunction("imlabMainExit", (Icallback) cmMainExit);
  IupSetFunction("imlabSaveAll", (Icallback) cmSaveAll);
  IupSetFunction("imlabCloseAll", (Icallback) cmCloseAll);
}

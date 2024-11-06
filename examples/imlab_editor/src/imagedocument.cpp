
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>

#include <im.h>
#include <im_image.h>
#include <im_convert.h>

#include <iup.h>

#include "utl_file.h"
#include <iup_config.h>

#include "documentlist.h"
#include "imagewindow.h"
#include "bitmapwindow.h"
#include "histogramwindow.h"
#include "tridimwindow.h"
#include "matrixwindow.h"
#include "mainwindow.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"


void imlabImageDocument::RemoveView(Ihandle* view_dialog)
{
  int i;

  char* dialog_type = IupGetAttribute(view_dialog, "ImageWindowType");
  if (imStrEqual(dialog_type, "Bitmap"))
  {
    int bitmap_count = 0;
    for (i = 0; i < view_list_count; i++)
    {
      imlabImageWindow* image_window = view_list[i];
      char* dialog_type = IupGetAttribute(image_window->dialog, "ImageWindowType");
      if (imStrEqual(dialog_type, "Bitmap"))
        bitmap_count++;
    }

    if (bitmap_count == 1)
    {
      Close();
      return;
    }
  }


  for (i = 0; i < view_list_count; i++)
  {
    imlabImageWindow* image_window = view_list[i];
    if (image_window->dialog == view_dialog)
    {
      delete image_window;
      break;
    }
  }

  view_list_count--;

  for (; i < view_list_count; i++)
  {
    view_list[i] = view_list[i+1];
  }
}

static void iImageDocRemoveAllViews(imlabImageDocument *idocument)
{
  int i;
  for (i = 0; i < idocument->view_list_count; i++)
  {
    imlabImageWindow* image_window = idocument->view_list[i];
    delete image_window;
  }
  idocument->view_list_count = 0;
}

void imlabImageDocument::CreateView(const char* type)
{
  imlabImageWindow* image_window = NULL;

  if (imStrEqual(type, "Bitmap"))
    image_window = new imlabBitmapWindow(this);
  else if (imStrEqual(type, "Histogram"))
    image_window = new imlabHistogramWindow(this);
  else if (imStrEqual(type, "Matrix"))
    image_window = new imlabMatrixWindow(this);
  else if (imStrEqual(type, "3D"))
    image_window = new imlab3DWindow(this);

  if (image_window)
  {
    view_list[view_list_count] = image_window;
    view_list_count++;
  }
}

Ihandle* imlabImageDocument::ShowView(const char* type)
{
  for (int i = 0; i < view_list_count; i++)
  {
    imlabImageWindow* image_window = view_list[i];
    char* dialog_type = IupGetAttribute(image_window->dialog, "ImageWindowType");
    if (imStrEqual(dialog_type, type))
    {
      IupShow(image_window->dialog);
      return image_window->dialog;
    }
  }

  // if not found create a new one
  CreateView(type);
  return NULL;
}

void imlabImageDocument::UpdateViews()
{
  for (int i = 0; i < view_list_count; i++)
  {
    imlabImageWindow* image_window = view_list[i];
    image_window->Update();
  }
}

void imlabImageDocument::RefreshViews()
{
  for (int i = 0; i < view_list_count; i++)
  {
    imlabImageWindow* image_window = view_list[i];
    image_window->Refresh();
  }
}

void imlabImageDocument::UpdateViewsTitle()
{
  int i;

  if (FileTitle) free(FileTitle);
  FileTitle = utlFileGetTitle(ImageFile->filename);

  for (i = 0; i < view_list_count; i++)
  {
    imlabImageWindow* image_window = view_list[i];
    image_window->SetTitle();
  }
}

const char* imlabImageDocument::DlgTitle(const char* title)
{
  static char full_title[400];
  sprintf(full_title, "%s - %s", title, FileTitle);
  return full_title;
}

void imlabImageDocument::SetBitmapOptions(int cpx2real, double gamma, int absolute, int cast_mode)
{
  /* if changed here, must save the previous parameters for Undo */
  imImageSetAttribString(ImageFile->image, "ViewOptions", "Yes");
  imImageSetAttribInteger(ImageFile->image, "ViewOptionsAbss", IM_INT, absolute);
  imImageSetAttribInteger(ImageFile->image, "ViewOptionsCpx", IM_INT, cpx2real);
  imImageSetAttribInteger(ImageFile->image, "ViewOptionsMode", IM_INT, cast_mode);
  imImageSetAttribReal(ImageFile->image, "ViewOptionsGamma", IM_DOUBLE, gamma);

  BitmapView.absolute = absolute;
  BitmapView.cpx2real = cpx2real;
  BitmapView.gamma = gamma;
  BitmapView.cast_mode = cast_mode;
}

static void iImageDocRestoreViewOptions(imlabImageDocument* idocument)
{
  if (imImageGetAttribString(idocument->ImageFile->image, "ViewOptions"))
  {
    idocument->BitmapView.absolute = imImageGetAttribInteger(idocument->ImageFile->image, "ViewOptionsAbss", 0);
    idocument->BitmapView.cpx2real = imImageGetAttribInteger(idocument->ImageFile->image, "ViewOptionsCpx", 0);
    idocument->BitmapView.cast_mode = imImageGetAttribInteger(idocument->ImageFile->image, "ViewOptionsMode", 0);
    idocument->BitmapView.gamma = imImageGetAttribReal(idocument->ImageFile->image, "ViewOptionsGamma", 0);
  }
}

static void iImageDocSaveViewOptions(imlabImageDocument* idocument)
{
  if (!imImageGetAttribString(idocument->ImageFile->image, "ViewOptions"))
  {
    imImageSetAttribString(idocument->ImageFile->image, "ViewOptions", "Yes");
    imImageSetAttribInteger(idocument->ImageFile->image, "ViewOptionsAbss", IM_INT, idocument->BitmapView.absolute);
    imImageSetAttribInteger(idocument->ImageFile->image, "ViewOptionsCpx", IM_INT, idocument->BitmapView.cpx2real);
    imImageSetAttribInteger(idocument->ImageFile->image, "ViewOptionsMode", IM_INT, idocument->BitmapView.cast_mode);
    imImageSetAttribReal(idocument->ImageFile->image, "ViewOptionsGamma", IM_DOUBLE, idocument->BitmapView.gamma);
  }
}


void imlabImageDocument::ClearUndo()
{
  int i;
  for (i = 0; i < undo_count; i++)
  {
    imImageDestroy(UndoStack[i]);
    UndoStack[i] = NULL;
  }

  if (UndoStack)
    free(UndoStack);

  UndoStack = NULL;
  undo_count = 0;
  undo_pos = 0;
  undo_alloc = 0;

  imlabMainWindowUpdateUndoRedoButtons(this);
}

int imlabImageDocument::HasUndo()
{
  return undo_pos > 0;
}

int imlabImageDocument::HasRedo()
{
  return undo_pos < undo_count;
}

void imlabImageDocument::Undo()
{
  if (undo_pos > 0)
  {
    imImage* image = ImageFile->image;

    undo_pos--;

    ImageFile->image = UndoStack[undo_pos];
    UndoStack[undo_pos] = image;

    iImageDocRestoreViewOptions(this);
  }

  if (undo_pos == 0)
  {
    if (ImageFile->changed == 1)
    {
      ImageFile->changed = 0;
      HasChanged();
    }
  }

  imlabMainWindowUpdateUndoRedoButtons(this);
}

void imlabImageDocument::Redo()
{
  if (undo_pos == 0)
  {
    if (ImageFile->changed == 0)
    {
      ImageFile->changed = 1;
      HasChanged();
    }
  }

  if (undo_pos < undo_count)
  {
    imImage* image = ImageFile->image;

    ImageFile->image = UndoStack[undo_pos];
    UndoStack[undo_pos] = image;

    undo_pos++;

    iImageDocRestoreViewOptions(this);
  }

  imlabMainWindowUpdateUndoRedoButtons(this);
}

void imlabImageDocument::PushUndo(imImage* image)
{
  if (undo_count + 1 > undo_alloc)
  {
    undo_alloc += 20;
    UndoStack = (imImage**)realloc(UndoStack, undo_alloc*sizeof(imImage*));
  }

  if (undo_pos != undo_count)
  {
    int i;
    for (i = undo_pos; i < undo_count; i++)
    {
      imImageDestroy(UndoStack[i]);
      UndoStack[i] = NULL;
    }

    undo_count = undo_pos;
  }

  UndoStack[undo_pos] = image;
  undo_pos++;
  undo_count++;

  iImageDocSaveViewOptions(this);
  imlabMainWindowUpdateUndoRedoButtons(this);
}

void imlabImageDocument::HasChanged()
{
  UpdateViewsTitle();
  imlabMainWindowUpdateSaveButtons(this);
}

void imlabImageDocument::ChangeImage(imImage* NewImage, const char *format, ...)
{
  /* Swaps the new image with the current image */
  PushUndo(ImageFile->image);
  ImageFile->image = NewImage;

  if (!ImageFile->changed)
    ImageFile->changed = 1;

  HasChanged();
  UpdateViews();

  char buffer[512];
  va_list arglist;
  va_start(arglist, format);
  vsprintf(buffer, format, arglist);
  va_end(arglist);

  imlabLogMessagef("imlab[\"%s\"].%s", FileTitle, buffer);
  imImageSetAttribString(NewImage, "History", buffer);
}

imlabImageDocument::imlabImageDocument(imlabImageFile* NewImageFile)
{
  static int imlab_document_index = 0;
  document_index = imlab_document_index;
  imlab_document_index++;

  UndoStack = NULL;
  undo_pos = 0, undo_count = 0, undo_alloc = 0;

  imBitmapViewInit(&(BitmapView));

  ImageFile = NewImageFile;
  FileTitle = utlFileGetTitle(ImageFile->filename);

  const char* scale_units = IupConfigGetVariableStr(imlabConfig(), "ScaleUnits", FileTitle);
  if (scale_units)
  {
    double scale = IupConfigGetVariableDouble(imlabConfig(), "Scale", FileTitle);

    imImageSetAttribReal(ImageFile->image, "SCALE", IM_DOUBLE, scale);
    imImageSetAttribString(ImageFile->image, "SCALE_LENGTH_UNITS", scale_units);
  }

  view_list_count = 0;
  CreateView("Bitmap");

  imlabImageDocumentListAdd(this);
}

void imlabImageDocumentCreateNewImage(int width, int height, int color_mode, int data_type)
{
  imlabImageFile* NewImageFile;

  /* Creates the image */
  NewImageFile = imlabImageFileNew(width, height, color_mode, data_type);
  if (!NewImageFile)
    return;

  new imlabImageDocument(NewImageFile);
}

void imlabImageDocumentCreateFromFileName(const char* filename)
{
  imlabImageFile** image_file_list = imlabImageFileOpen(filename, -1);
  if (!image_file_list)
    return;

  IupConfigRecentUpdate(imlabConfig(), filename);

  while (*image_file_list)
  {
    new imlabImageDocument(*image_file_list);
    image_file_list++;
  }
}

void imlabImageDocumentCreateFromImage(imImage* NewImage, const char *format_title, const char *format_history, ...)
{
  char buffer[512];
  va_list arglist;

  imlabImageFile* NewImageFile = imlabImageFileCreate(NewImage);

  va_start(arglist, format_history);
  vsprintf(buffer, format_history, arglist);
  va_end(arglist);
  imImageSetAttribString(NewImageFile->image, "History", buffer);

  va_start(arglist, format_history); // start at format_history
  vsprintf(buffer, format_title, arglist);
  va_end(arglist);
  strcpy(NewImageFile->filename, buffer);

  imlabLogMessagef("imlab[\"%s\"] = imlab.%s", NewImageFile->filename, imImageGetAttribString(NewImageFile->image, "History"));

  new imlabImageDocument(NewImageFile);
}

imlabImageDocument::~imlabImageDocument()
{
  iImageDocRemoveAllViews(this);

  imBitmapViewRelease(&(BitmapView));

  free(FileTitle);

  imlabImageDocumentListRemove(this);

  imlabImageFileDestroy(ImageFile, 1);

  ClearUndo();
}

int imlabImageDocument::Close()
{
  /* If the image is not saved aks the user if he wants to save it */
  if (ImageFile->changed && !IupGetInt(NULL, "SHIFTKEY"))
  {
    ShowView("Bitmap");

    int ret = imlabDlgQuestion("Image not Saved.\n  Save it before closing? or Cancel close?", 1);
    if (ret == 1) /* Yes Saves the image */
    {
      if (ImageFile->format[0] == 0)
      {
        if (!imlabDlgImageFileSaveAs(ImageFile->image, 
                                    ImageFile->filename,
                                    ImageFile->format,
                                    ImageFile->compression,
                                    0))
          return 0;
      }

      if (!imlabImageFileSave(&ImageFile, 1))
        return 0;

      IupConfigRecentUpdate(imlabConfig(), ImageFile->filename);
    }
    else if (ret == 3) /* Cancel just return */
      return 0;
  }

  delete this;

  return 1;
}


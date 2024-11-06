


/** \file
* \brief Image Document
*
*/

#ifndef __IMAGEDOCUMENT_H
#define __IMAGEDOCUMENT_H

#include <iup.h>
#include "imagefile.h"
#include "im_imageview.h"

class imlabImageWindow;


class imlabImageDocument
{
public:
  imlabImageDocument(imlabImageFile* NewImageFile);

  ~imlabImageDocument();

  /** Closes the image document. Check if saved. */
  int Close();

  /** Replaces the image by another.
      Store the old in the undo, mark document as changed, update views, and print a log message. */
  void ChangeImage(imImage* NewImage, const char *format, ...);

  /** The image was changed directly or the image file was changed, update the interface */
  void HasChanged();

  void ClearUndo();
  int HasUndo();
  int HasRedo();
  void Undo();
  void Redo();

  /** Creates a view */
  void CreateView(const char* type);

  /** Updates all the views. */
  void UpdateViews();

  /** Redraw all the views. */
  void RefreshViews();

  /** Finds and shows a view. */
  Ihandle* ShowView(const char* type);

  /** Remove a view from the view list */
  void RemoveView(Ihandle* view_dialog);

  /** Changes the default bitmap view parameters for new image documents */
  void SetBitmapOptions(int cpx2real, double gamma, int absolute, int cast_mode);


public:
  imlabImageFile* ImageFile;

  imBitmapView BitmapView;

  int document_index;
  char* FileTitle;

  imlabImageWindow* view_list[50];
  int view_list_count;

  const char* DlgTitle(const char* title);

protected:
  imImage* *UndoStack;
  int undo_pos, undo_count, undo_alloc;

  void PushUndo(imImage* image);

  void UpdateViewsTitle();
};


/** Creates an image document from an image and a format string for the title. */
void imlabImageDocumentCreateFromImage(imImage* NewImage, const char *format_title, const char *format_history, ...);

/** Creates an image document from a file */
void imlabImageDocumentCreateFromFileName(const char* FileName);

/** Creates a new image document */
void imlabImageDocumentCreateNewImage(int width, int height, int color_mode, int data_type);


#endif

#include <iup.h>
#include <iupgl.h>
#include <cd.h>

#include "imagewindow.h"
#include "imlab.h"
#include "statusbar.h"


static int cmViewGetFocus(Ihandle* self)
{
  imlabImageWindow *image_window = (imlabImageWindow*)IupGetAttribute(self, "imlabImageWindow");
  imlabSetCurrentDocument(image_window->document);
  return IUP_DEFAULT;
}

static int cmViewClose(Ihandle* self)
{
  imlabImageWindow *image_window = (imlabImageWindow*)IupGetAttribute(self, "imlabImageWindow");
  image_window->document->RemoveView(image_window->dialog);
  return IUP_IGNORE;
}

void imlabImageWindow::SetTitle()
{
  IupSetfAttribute(this->dialog, 
                   "TITLE", "%s of \"[%d] %.60s\"%s",
                   IupGetAttribute(this->dialog, "ImageWindowType"),
                   this->document->document_index,
                   this->document->FileTitle,
                   this->document->ImageFile->changed? " *": "");
}

void imlabImageWindow::SetCallbacks()
{
  IupSetCallback(this->dialog, "CLOSE_CB", cmViewClose);
  IupSetCallback(this->dialog, "GETFOCUS_CB", cmViewGetFocus);

  IupSetAttribute(this->dialog, "imlabImageWindow", (char*)this);
}

void imlabImageWindow::ShowWindow()
{
  int xpos, ypos;
  imlabNewWindowPos(&xpos, &ypos);
  IupShowXY(this->dialog, xpos, ypos);
}

imlabImageWindow::~imlabImageWindow()
{
  IupDestroy(this->dialog);
}

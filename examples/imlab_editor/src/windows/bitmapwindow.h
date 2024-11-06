/** \file
 * \brief Bitmap Window
 */

#ifndef __BITMAPWINDOW_H
#define __BITMAPWINDOW_H

#include "imagewindow.h"


/** Bitmap Window */
class imlabBitmapWindow: public imlabImageWindow
{
public:
  Ihandle *canvas;

  imImageView* image_view;

  imlabBitmapWindow(imlabImageDocument* document);
  ~imlabBitmapWindow();
  void Update();
  void Refresh();
};


#endif

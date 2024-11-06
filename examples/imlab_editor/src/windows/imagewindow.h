/** \file
 * \brief Image Window
 * Base class for all image windows */

#ifndef __IMAGEWINDOW_H
#define __IMAGEWINDOW_H

#include <iup.h>
#include "imagedocument.h"


/** Image Window */
class imlabImageWindow                  
{
public:
  imlabImageDocument* document;
  Ihandle *dialog;

  virtual ~imlabImageWindow();

  void ShowWindow();

  virtual void Update() = 0;
  virtual void Refresh() { Update(); }

  void SetTitle();

  void SetCallbacks();

  virtual void Sync(int, int) {}
};

#endif

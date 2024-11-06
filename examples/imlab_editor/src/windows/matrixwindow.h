/** \file
 * \brief Matrix Window
 */

#ifndef __MATRIXWINDOW_H
#define __MATRIXWINDOW_H

#include "imagewindow.h"
#include "iup_class_cbs.hpp"


/** Matrix Window */
class imlabMatrixWindow : public imlabImageWindow
{
public:
  imlabMatrixWindow(imlabImageDocument* document);

  /* virtual replace */
  void Update();
  void Sync(int x, int y);

protected:
  Ihandle *matrix;

  int lock_update;

  imImage *image,
          *bitmap_image;

  void CreateDialog();
  void InternalUpdate();

  IUP_CLASS_DECLARECALLBACK_sIFnii(imlabMatrixWindow, MatrixValueCallback);
  IUP_CLASS_DECLARECALLBACK_IFniis(imlabMatrixWindow, MatrixValueEditCallback);
  IUP_CLASS_DECLARECALLBACK_IFniiIII(imlabMatrixWindow, MatrixFgColorCallback);
  IUP_CLASS_DECLARECALLBACK_IFniiIII(imlabMatrixWindow, MatrixBgColorCallback);
};


#endif

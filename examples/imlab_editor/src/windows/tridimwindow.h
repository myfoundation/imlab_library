/** \file
 * \brief 3D Window
 */

#ifndef __3DWINDOW_H
#define __3DWINDOW_H

#include "imagewindow.h"


/** 3D Window */
class imlab3DWindow : public imlabImageWindow
{
public:
  Ihandle *canvas;

  int width, height;  /* size of the cd_canvas */
  int render_type;

  double fov,
         eyex, eyey,
         refx, refy;

  int pos_x, pos_y;   /* interaction state */

  imlab3DWindow(imlabImageDocument* document);
  void Update();
};
                   

#endif

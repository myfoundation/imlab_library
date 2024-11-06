#include <iup.h>
#include <stdlib.h>
#include <stdio.h>

#include "imlab.h"
#include "imlab_images.h"


#ifndef WIN32
static void CreateButtonImage(int w, int h, unsigned char* bits, char* colors[], char* name)
{
  Ihandle* iup_image = IupImage(w, h, bits);
  IupSetHandle(name, iup_image); 

  int i = 0;
  while (colors[i] != NULL)
  {
    IupStoreAttributeId(iup_image, "", i, colors[i]); 
    i++;
  }
}
#endif

void imlabCreateButtonImages(void)
{
  IupSetHandle("imlabProgressCancel", load_image_cancel());
  IupSetHandle("imlabHistogramButton", load_image_histo());
  IupSetHandle("imlab3DButton", load_image_3d());
  IupSetHandle("imlabBitmapButton", load_image_bitmap());
  IupSetHandle("imlabMatrixButton", load_image_matrix());

#ifndef WIN32
  unsigned char cross_bits[23*23] = 
  {
   0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2
  ,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1
  ,2,2,2,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  ,0,0,0,0,0,0,0,0,0,0,2,1,2,0,0,0,0,0,0,0,0,0,0
  };
  char* cross_colors[] = 
  {
    "BGCOLOR", 
    "255 255 255", 
    "0 0 0", 
    NULL
  };

  CreateButtonImage(23, 23, cross_bits, cross_colors, "CrossCursor"); 
  IupSetAttribute(IupGetHandle("CrossCursor"), "HOTSPOT", "11:11");  // From RC in Windows

  IupSetHandle("IMLAB", load_image_imlab_icon());  // From RC in Windows
#endif
}

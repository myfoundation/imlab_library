#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <iup.h>
#include <iupim.h>

#include "imlab.h"
#include "utl_file.h"


static int cbSplashTimer(Ihandle* timer)
{
  if (IupGetAttribute(timer, "_FIRST_STAGE"))
  {
    IupSetAttribute(timer, "RUN", "NO");
    IupSetAttribute(timer,"TIME", "500");
    IupSetAttribute(timer, "RUN", "YES");
    IupSetAttribute(timer, "_FIRST_STAGE", NULL);
  }
  else
  {
    Ihandle* dlg = (Ihandle*)IupGetAttribute(timer, "_DIALOG");
    IupSetAttribute(timer, "RUN", "NO");
    IupDestroy(dlg);
    IupDestroy(timer);
  }
  return IUP_DEFAULT;
}

static Ihandle* load_image_imlab_logo(const char* exe_filename)
{
  /* Look for the image file on the same folder of the executable
  or in the folder above. */
  char* file_path = utlFileGetPath(exe_filename);
  char filename[10240];
  Ihandle* image;

  if (file_path)
    sprintf(filename, "%s%s", file_path, "ImLab.png");
  else
    sprintf(filename, "%s", "ImLab.png");

  image = IupLoadImage(filename);
  if (!image && file_path)
  {
    int len = (int)strlen(file_path);
    if (file_path[len - 1] == '/' || file_path[len - 1] == '\\')
      file_path[len - 1] = 0;

    char* up_file_path = utlFileGetPath(file_path);
    if (up_file_path)
    {
      sprintf(filename, "%s%s", up_file_path, "ImLab.png");

      image = IupLoadImage(filename);
      free(up_file_path);
    }
  }

  if (file_path) 
    free(file_path);

  return image;
}

void imlabShowSplash(const char* exe_filename)
{
  Ihandle *dlg, *image, *timer, *lbl;

  image = load_image_imlab_logo(exe_filename);
  IupSetAttribute(image, "AUTOSCALE", "1");  /* do not autoscale the splash image */
  IupSetHandle("IMLAB_LOGO", image);

  lbl = IupLabel(NULL);
  IupSetAttribute(lbl, "IMAGE", "IMLAB_LOGO");

  dlg = IupDialog(lbl);

  IupSetAttribute(dlg,"BORDER", "NO");
  IupSetAttribute(dlg,"RESIZE", "NO");
  IupSetAttribute(dlg,"MINBOX", "NO");
  IupSetAttribute(dlg,"MAXBOX", "NO");
  IupSetAttribute(dlg,"MENUBOX", "NO");
  IupSetAttribute(dlg,"TOPMOST", "YES");

  /* show the splash for 1 second without other windows,
     then start to show the main window. */

  timer = IupTimer();
  IupSetAttribute(timer,"TIME", "1000");
  IupSetCallback(timer, "ACTION_CB", cbSplashTimer);
  IupSetAttribute(timer, "_DIALOG", (char*)dlg);
  IupSetAttribute(timer, "_FIRST_STAGE", "YES");
  IupSetAttribute(timer, "RUN", "YES");

  IupSetAttribute(dlg, "OPACITYIMAGE", "IMLAB_LOGO");

  IupShowXY(dlg, IUP_CENTER, IUP_CENTER);

  while(IupGetAttribute(timer, "_FIRST_STAGE"))
    IupLoopStep();
}

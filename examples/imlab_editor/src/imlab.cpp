#include <iup.h>
#include <iupcontrols.h>
#include <iupgl.h>
#include <iup_plot.h>

#include <im_format_jp2.h>
#ifdef USE_AVI
#include <im_format_avi.h>
#endif
#ifdef USE_WMV
#include <im_format_wmv.h>
#endif

#include "imlab.h"
#include "imagedocument.h"
#include "mainwindow.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <locale.h>


void imlabShowSplash(const char* exe_filename);


/***************************************/
/* Where everything begin and end...   */
/***************************************/

int main(int argc, char* argv[])
{
  imFormatRegisterJP2();
#ifdef USE_AVI
  imFormatRegisterAVI();
#endif
#ifdef USE_WMV
  imFormatRegisterWMV();
#endif

  /* Initialize the interface library */
  if (IupOpen(&argc, &argv) == IUP_ERROR)
    return 0;

  IupSetLanguage("ENGLISH");
  IupSetGlobal("IMAGEAUTOSCALE", "DPI");

  setlocale(LC_NUMERIC, "C");

#ifdef _DEBUG
  IupSetGlobal("GLOBALLAYOUTDLGKEY", "Yes");
#endif

  IupSetGlobal("DEFAULTPRECISION", "4");

  IupControlsOpen();
  IupGLCanvasOpen();
  IupPlotOpen();
  IupImageLibOpen();

  imlabShowSplash(IupGetGlobal("EXEFILENAME"));

  /* create the main window */
  imlabCreateMainWindow();

  /* process command line arguments */
  while (argc > 1)
  {
    imlabImageDocumentCreateFromFileName(argv[argc-1]);
    argc--;
  }

  /* Start the interface message loop */
  IupMainLoop();

  /* destroy the main window */
  imlabKillMainWindow();

  /* Terminates the interface library */
  IupClose();

  return 1;
}

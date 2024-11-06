#include <iup.h>
#include <cd.h>
#include <im.h>
#include <im_counter.h>

#include "imlab.h"
#include "plugin_process.h"
#include "imagedocument.h"
#include "counter.h"
#include "im_imageview.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static imlabProcPlugIn* iwinPlugInList[50];   // current count = 21
static int iwinPlugInListCount = 0;


/*********************************************************************/
/* this section must be edited to add more items in the process menu */

extern imlabProcPlugIn
    *iwinHisto, *iwinThres, *iwinColor, *iwinCombine,
    *iwinArith, *iwinConv, *iwinRender, *iwinLogic,
    *iwinGeom, *iwinTrans, *iwinEffects, *iwinGamut,
    *iwinMorphBin, *iwinMorphGray, *iwinQuant, *iwinBinary,
    *iwinAnalyze, *iwinConvRank, *iwinResize, *iwinStats,
    *iwinRemoteSens;

void imlabProcPlugInInit(Ihandle* mnProcess)
{
  imlabProcPlugInRegister(iwinResize, mnProcess);
  imlabProcPlugInRegister(iwinGeom, mnProcess);
  imlabProcPlugInRegister(iwinQuant, mnProcess);
  imlabProcPlugInRegister(iwinColor, mnProcess);
  
  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinThres, mnProcess);
  imlabProcPlugInRegister(iwinHisto, mnProcess);
  imlabProcPlugInRegister(iwinRender, mnProcess);
  imlabProcPlugInRegister(iwinGamut, mnProcess);
  imlabProcPlugInRegister(iwinArith, mnProcess);
  imlabProcPlugInRegister(iwinCombine, mnProcess);
  imlabProcPlugInRegister(iwinLogic, mnProcess);
  
  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinConv, mnProcess);
  imlabProcPlugInRegister(iwinConvRank, mnProcess);
  imlabProcPlugInRegister(iwinMorphBin, mnProcess);
  imlabProcPlugInRegister(iwinMorphGray, mnProcess);
  imlabProcPlugInRegister(iwinBinary, mnProcess);

  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinTrans, mnProcess);
  
  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinEffects, mnProcess);

  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinRemoteSens, mnProcess);

}

void imlabAnaPlugInInit(Ihandle* mnProcess)
{
  imlabProcPlugInRegister(iwinStats, mnProcess);

  IupAppend(mnProcess, IupSeparator());

  imlabProcPlugInRegister(iwinAnalyze, mnProcess);
}


  /*                 end of edit section                           */
/*****************************************************************/

void imlabProcPlugInRegister(imlabProcPlugIn* plug_in, Ihandle* mnProcess)
{
  iwinPlugInList[iwinPlugInListCount] = plug_in;
  iwinPlugInListCount++;

  plug_in->mnProcess = mnProcess;
  plug_in->init(mnProcess);
}

Ihandle* imlabProcNewItem(Ihandle* mnProcess, char* title, char* proc_name, Icallback cb, int update) 
{
  Ihandle* mnItem = imlabItem(title, NULL);
  IupSetCallback(mnItem, "ACTION", cb);

  if (update)
    IupSetAttribute(mnProcess, proc_name, (char*)mnItem);

  return mnItem;
}
                  
void imlabProcPlugInUpdateItem(Ihandle* mnProcess, char *process, int (*match_func)(const imImage* image))
{
  Ihandle* mnItem;
  imlabImageDocument* document;
  imImage *image;
  int activate, state;

  mnItem = (Ihandle*)IupGetAttribute(mnProcess, process);
  document = imlabGetCurrentDocument(mnProcess);
  image = document->ImageFile->image;

  if (!match_func || match_func(image))
    activate = 1;
  else
    activate = 0;

  if (IupGetInt(mnItem, "ACTIVE"))
    state = 1;
  else
    state = 0;

  if (state != activate)
    IupSetAttribute(mnItem, "ACTIVE", activate? "YES": "NO");
}

void imlabProcPlugInUpdate(Ihandle* mnProcess)
{
  for (int i = 0; i < iwinPlugInListCount; i++)
  {
    imlabProcPlugIn *plug_in = iwinPlugInList[i];
    if (plug_in->update && mnProcess == plug_in->mnProcess) 
      plug_in->update(mnProcess);
  }
}

void imlabProcPlugInFinish(void)
{
  if (iwinPlugInListCount == 0)
    return;

  for (int i = 0; i < iwinPlugInListCount; i++)
  {
    imlabProcPlugIn *plug_in = iwinPlugInList[i];
    if (plug_in->finish) plug_in->finish();
  }
}



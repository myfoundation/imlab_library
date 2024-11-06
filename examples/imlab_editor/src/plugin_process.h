/** \file
 * \brief Process PlugIn Management.
 * Used by the \ref imlabImageDocument */

#ifndef __PROCESSPLUGIN_H
#define __PROCESSPLUGIN_H

#include <iup.h>
#include <iupcontrols.h>
#include <im_image.h>
#include <im_util.h>

#include "im_imagematch.h"
#include "imagedocument.h"
#include "documentlist.h"
#include "counter_preview.h"


/** Initializes all the regitered plugins */
void imlabProcPlugInInit(Ihandle* mnProcess);
void imlabAnaPlugInInit(Ihandle* mnProcess);

/** Finishes all the registered plugins */
void imlabProcPlugInFinish(void);

/** Updates all the menus items in the process menu */
void imlabProcPlugInUpdate(Ihandle* mnProcess);

/** Creates a new menu item for the process menu 
  * Very usefull for plugin initialization. */
Ihandle* imlabProcNewItem(Ihandle* mnProcess, char* title, char* proc_name, Icallback cb, int update);

/** Update an item in the process menu acording to the current image.
  * Very usefull for plugin update. */
void imlabProcPlugInUpdateItem(Ihandle* mnProcess, char *process, int (*match_func)(const imImage* image));

/** Plugin function table. */
struct imlabProcPlugIn
{
  void (*init)(Ihandle* mnPlugIn);
  void (*update)(Ihandle* mnPlugIn);
  void (*finish)(void);

  Ihandle* mnProcess;
};

/** Registers a plugin */
void imlabProcPlugInRegister(imlabProcPlugIn* plug_in, Ihandle* mnProcess);



#endif  /* __PROCESSPLUGIN_H */


#ifndef __MAINWINDOW_H
#define __MAINWINDOW_H

#include "imagedocument.h"

/** Creates the IMLAB main window */
void imlabCreateMainWindow(void);

/** Destroys the IMLAB main window */
void imlabKillMainWindow(void);

void imlabMainWindowUpdateToolbar(void);
void imlabMainWindowUpdateSaveButtons(imlabImageDocument* document);
void imlabMainWindowUpdateUndoRedoButtons(imlabImageDocument* document);

Ihandle* imlabMainWindowCreateMenu(void);
void imlabMainWindowRegisterFileMenu(void);
void imlabMainWindowRegisterImageMenu(void);
void imlabMainWindowRegisterViewMenu(void);


#endif

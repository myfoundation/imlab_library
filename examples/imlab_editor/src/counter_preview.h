/** \file
 * \brief Counter and Preview Management.
 **/

#ifndef __COUNTERPREVIEW_H
#define __COUNTERPREVIEW_H

#include <iup.h>
#include <iupcontrols.h>
#include <im_image.h>
#include <im_util.h>

#include "im_imagematch.h"
#include "imagedocument.h"
#include "documentlist.h"


/** creates the dialog. Called only by main_window. */
void imlabCounterCreateProgressDlg();

/** releases the dialog. Called only by main_window. */
void imlabCounterReleaseProgressDlg();

/** enable/disable the counter dialog */
void imlabCounterEnable(int enable);


/*********************************************************************************************************/


/** initializes the Preview logic */
Ihandle* imlabProcessPreviewInit(imlabImageDocument *document, imImage* image, imImage* NewImage);

/** updates the current image with Preview contents */
void imlabProcessPreviewUpdate(Ihandle* preview);

/** reset the image and the preview toggle */
void imlabProcessPreviewReset(Ihandle* preview);

/** checks for a Preview toggle in IupGetParam dialogs */
int imlabProcessPreviewCheckParam(Ihandle* param_box, int param_index, const char* preview_param, Ihandle* preview, int *param_readonly);


#endif  /* __PROCESSPLUGIN_H */

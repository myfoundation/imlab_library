/** \file
 * \brief Some usefull dialogs */

#ifndef __DIALOGS_H
#define __DIALOGS_H

#include "imagefile.h"


/** Returns the optimal size unit and scale the size to the unit.
  Original size must be in bytes.    */
char* imlabDlgGetSizeDesc(double *size);

/** Dialog to retrieve a new image size */
int imlabDlgGetNewSize(int *width, int *height, int *order, int org_width, int org_height);
int imlabDlgGetNewSizeCheck(Ihandle* dialog, int param_index, void* user_data);

/** Dialog to retrieve a file name for saving */
int imlabDlgGetSaveAsImageFileName(char *filename, int is_multi);

/** Dialog to retrieve a file name for reading */
int imlabDlgGetOpenImageFileName(char *filename);

/** Dialog to retrieve a file name for reading an image with a Preview area */
int imlabDlgGetOpenImageFileNamePreview(char *filename);

int imlabDlgSelectFile(char* filename, const char* type, const char* title, const char* extfilter, const char* dirname);

/** Dialog to retrieve parameters for a new image */
int imlabDlgGetNewImageParam(int *width, int *height, int *color_space, int *data_type);

/** Dialog to retrieve parameters for a RAW image file */
int imlabDlgGetRAWImageParam(int *top_down, int *switch_type, int *byte_order, int *packed, int *padding, unsigned long *offset, int *ascii);

/** Dialog to shows a message using sprintf format */
void imlabDlgMessagef(const char* title, const char* format, ...);

/** Dialog to save the image in a given format */
int imlabDlgImageFileSaveAs(imImage* image, char* filename, char* format, char* compression, int is_multi);

/** Dialog to show image information */
void imlabDlgImageFileInfo(const char* filetitle, const imlabImageFile* ImageFile);

/** Dialog to manage image attributes */
int imlabDlgImageFileEditAttrib(const char* filetitle, imlabImageFile* ImageFile);

/** Dialog to show an IM error message when accessing a file, must be defined somewhere else. */
void imlabDlgFileErrorMsg(const char* title, int err, const char* filename);

void imlabDlgMemoryErrorMsg(void);

/** Displays the image in full screen.
  *Implemented in fullscreen.c */
void imlabDlgFullScreen(imImage* image, int fit);

/** Displays a dialog to edit a palette */
int imlabDlgEditPalette(const char* filetitle, long* palette, int* palette_count);

/** Checks if a kernel_size parameter in a IupGetParam dialog is odd and >3. */
int imlabDlgCheckKernelParam(Ihandle* dialog, Ihandle* kernel_param, int *kernel_size);

int imlabDlgGetConstant(const char* filetitle, const char* title, int color_space, double* param);

int imlabDlgQuestion(const char *msg, int has_cancel);


#endif  /* __DIALOGS_H */

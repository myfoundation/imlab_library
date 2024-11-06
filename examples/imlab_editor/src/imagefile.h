/** \file
 * \brief Image File
 * Associates an image with a file and a file format. */

#ifndef __IMAGEFILE_H
#define __IMAGEFILE_H

#include <im.h>
#include <im_image.h>
#include <im_util.h>


/** Image File */
struct imlabImageFile
{
  char filename[10240];
  char format[10];
  char compression[20];
  int changed, index;
  imImage* image;
};

/** Creates an image file from a file. */
imlabImageFile** imlabImageFileOpen(const char* filename, int index);

/** Creates a new image file. */
imlabImageFile* imlabImageFileNew(int width, int height, int color_mode, int data_type);

/** Creates an image file from an image. */
imlabImageFile* imlabImageFileCreate(imImage* image);

/** Saves the image file */
int imlabImageFileSave(imlabImageFile** file_list, int image_count);

/** Destroys an image file. */
void imlabImageFileDestroy(imlabImageFile* file, int kill_image);


/**************************************************************************************/


/** Loads a RAW image. */
int imlabRawLoadImage(const char* filename, imImage* image, int top_down, 
                      int switch_type, int byte_order, int is_packed, int padding, int start_offset, int ascii);
                   
/** Saves a RAW image. */
int imlabRawSaveImage(const char* filename, imImage* image, int top_down, 
                      int switch_type, int byte_order, int is_packed, int padding, int start_offset, int ascii);


#endif  /* __IMAGEFILE_H */

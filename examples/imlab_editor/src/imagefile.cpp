
#include <im.h>
#include <im_raw.h>
#include <iup.h>
#include <iupcontrols.h>

#include "imagefile.h"
#include "dialogs.h"
#include "imlab.h"
#include "utl_file.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int iNewImageCount = 1;


int imlabRawLoadImage(const char* filename, imImage* image, int top_down, 
                   int switch_type, int byte_order, int is_packed, int padding, int start_offset, int ascii)
{
  int error;
  imFile* ifile = imFileOpenRaw(filename, &error);
  if (error)
  {
    imlabDlgFileErrorMsg("File Open", error, filename);
    return error;
  }

  int color_mode = image->color_space;
  if (top_down) color_mode |= IM_TOPDOWN;
  if (is_packed) color_mode |= IM_PACKED;

  imFileSetAttribute(ifile, "Width", IM_INT, 1, &image->width);
  imFileSetAttribute(ifile, "Height", IM_INT, 1, &image->height);
  imFileSetAttribute(ifile, "ColorMode", IM_INT, 1, &color_mode);
  imFileSetAttribute(ifile, "DataType", IM_INT, 1, &image->data_type);

  imFileSetAttribute(ifile, "SwitchType", IM_INT, 1, &switch_type);
  imFileSetAttribute(ifile, "ByteOrder", IM_INT, 1, &byte_order);
  imFileSetAttribute(ifile, "Padding", IM_INT, 1, &padding);
  imFileSetAttribute(ifile, "StartOffset", IM_INT, 1, &start_offset);

  if (ascii)
    imFileSetInfo(ifile, "ASCII");

  error = imFileReadImageInfo(ifile, 0, NULL, NULL, NULL, NULL);
  if (error)
  {
    imlabDlgFileErrorMsg("Image Info", error, filename);
    imFileClose(ifile);
    return error;
  }
  
  error = imFileReadImageData(ifile, image->data[0], 0, 0);

  if (error)
  {
    if (error != IM_ERR_COUNTER) // Not the counter, show the error
      imlabDlgFileErrorMsg("Load Image", error, filename);
  }

  imFileClose(ifile);

  return IM_ERR_NONE;
}

int imlabRawSaveImage(const char* filename, imImage* image, int top_down,
                   int switch_type, int byte_order, int is_packed, int padding, int start_offset, int ascii)
{
  int error;
  imFile* ifile = imFileNewRaw(filename, &error);
  if (error)
  {
    imlabDlgFileErrorMsg("File Save", error, filename);
    return 0;
  }

  int color_mode = image->color_space;
  if (top_down) color_mode |= IM_TOPDOWN;
  if (is_packed) color_mode |= IM_PACKED;

  imFileSetAttribute(ifile, "Width", IM_INT, 1, &image->width);
  imFileSetAttribute(ifile, "Height", IM_INT, 1, &image->height);
  imFileSetAttribute(ifile, "ColorMode", IM_INT, 1, &color_mode);
  imFileSetAttribute(ifile, "DataType", IM_INT, 1, &image->data_type);

  imFileSetAttribute(ifile, "SwitchType", IM_INT, 1, &switch_type);
  imFileSetAttribute(ifile, "ByteOrder", IM_INT, 1, &byte_order);
  imFileSetAttribute(ifile, "Padding", IM_INT, 1, &padding);
  imFileSetAttribute(ifile, "StartOffset", IM_INT, 1, &start_offset);

  if (ascii)
    imFileSetInfo(ifile, "ASCII");

  error = imFileWriteImageInfo(ifile, image->width, image->height, image->color_space, image->data_type);
  if (error)
  {
    imlabDlgFileErrorMsg("Image Info", error, filename);
    imFileClose(ifile);
    return error;
  }
  
  error = imFileWriteImageData(ifile, image->data[0]);
  if (error)
  {
    if (error != IM_ERR_COUNTER) // Not the counter, show the error
      imlabDlgFileErrorMsg("File Save", error, filename);
  }

  imFileClose(ifile);

  return IM_ERR_NONE;
}

imlabImageFile* imlabImageFileCreate(imImage* NewImage)
{
  imlabImageFile* NewImageFile = (imlabImageFile*)malloc(sizeof(imlabImageFile));

  sprintf(NewImageFile->filename, "untitled #%d", iNewImageCount++);
  NewImageFile->format[0] = 0;
  NewImageFile->compression[0] = 0;
  NewImageFile->changed = 2;  /* no undo */
  NewImageFile->index = -1;
  NewImageFile->image = NewImage;

  return NewImageFile;
}

imlabImageFile* imlabImageFileNew(int width, int height, int color_mode, int data_type)
{
  /* creates the new image */
  imImage* NewImage = imImageCreate(width, height, imColorModeSpace(color_mode), data_type);
  if (NewImage == NULL)
  {
    imlabDlgFileErrorMsg("File New", IM_ERR_MEM, "untitled");
    return NULL;
  }

  if (imColorModeHasAlpha(color_mode))
    imImageAddAlpha(NewImage);

  imlabImageFile* NewImageFile = (imlabImageFile*)malloc(sizeof(imlabImageFile));

  sprintf(NewImageFile->filename, "untitled #%d", iNewImageCount++);
  NewImageFile->format[0] = 0;
  NewImageFile->compression[0] = 0;
  NewImageFile->changed = 2;  /* no undo */
  NewImageFile->index = -1;
  NewImageFile->image = NewImage;

  char buffer[100];
  sprintf(buffer, "NewImage{width=%d, height=%d, color_mode=\"%s\", data_type=\"%s\"}", width, height, imColorModeSpaceName(NewImage->color_space), imDataTypeName(data_type));
  imImageSetAttribString(NewImage, "History", buffer);

  imlabLogMessagef("imlab[\"%s\"] = imlab.%s", NewImageFile->filename, buffer);

  return NewImageFile;
}

static int validate_limits(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;
  if (param_index < 0)
    return 1;

  int start = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
  int end = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
  if (start > end)
    return 0;

  return 1;
}

imlabImageFile** imlabImageFileOpen(const char* filename, int index)
{
  int error;
  imFile* ifile = imFileOpen(filename, &error);
  if (error)
  {
    imlabDlgFileErrorMsg("File Open", error, filename);
    return NULL;
  }

  int image_count;
  imFileGetInfo(ifile, NULL, NULL, &image_count);

  int start, end;
  if (index == -1)
  {
    start = 0; 
    end = image_count-1;
  }
  else
  {
    start = index; 
    end = index;
  }

  if (image_count > 1 && index == -1)
  {
    char format[100];
    sprintf(format, "Start: %%i[0,%d]\n"
                    "End: %%i[0,%d]\n", image_count-1, image_count-1);

    if (!IupGetParam("Multiple Images Load", validate_limits, NULL, format,
                     &start, &end, NULL))
    {
      imFileClose(ifile);
      return NULL;
    }
  }

  static imlabImageFile* image_file_list[512];
  int i;

  for (i = start; i <= end; i++)
  {
    imImage* NewImage = imFileLoadImage(ifile, i, &error);
    if (NewImage == NULL)
    {
      imlabDlgFileErrorMsg("Load Image", error, filename);
      imFileClose(ifile);
      return NULL;
    }
    else if (error)
    {
      if (error != IM_ERR_COUNTER) // Not the counter, show the error
        imlabDlgFileErrorMsg("Load Image", error, filename);

      if (error != IM_ERR_ACCESS)  // Not an IO error, destroy the image
      {
        imImageDestroy(NewImage);
        imFileClose(ifile);
        return NULL;
      }
    }

    imlabImageFile* NewImageFile = (imlabImageFile*)malloc(sizeof(imlabImageFile));

    if (image_count > 1)
      sprintf(NewImageFile->filename, "%s (%d)", filename, (int)(i-start));
    else
      strcpy(NewImageFile->filename, filename);

    NewImageFile->changed = 0;
    NewImageFile->image = NewImage;  

    if (image_count > 1)
      NewImageFile->index = i;
    else
      NewImageFile->index = -1;

    char buffer[10240];
    sprintf(buffer, "LoadImage{filename=\"%s\", index=%d}", filename, NewImageFile->index);
    imImageSetAttribString(NewImage, "History", buffer);

    char* FileTitle = utlFileGetTitle(NewImageFile->filename);
    imlabLogMessagef("imlab[\"%s\"] = imlab.%s", FileTitle, buffer);
    free(FileTitle);

    imFileGetInfo(ifile, NewImageFile->format, NewImageFile->compression, NULL);

    image_file_list[i-start] = NewImageFile;
  }

  image_file_list[i-start] = NULL;
  imFileClose(ifile);

  return image_file_list;
}

void imlabImageFileDestroy(imlabImageFile* ImageFile, int kill_image)
{
  if (kill_image)
    imImageDestroy(ImageFile->image);

  free(ImageFile);
}
    
static void iBuildIndexTabe(imlabImageFile** image_file_list, int* next_index, int image_count)
{
  int i;
  // check for the complete sequence

  // the first image must be 0
  if (image_file_list[0]->index != 0)
    goto default_seq;

  next_index[0] = 0;
  for (i = 1; i < image_count; i++)
  {
    if (image_file_list[i]->index == -1 || image_file_list[i]->index == 0)
      break;

    next_index[image_file_list[i]->index] = i;
  }

  if (i < image_count) // not all indexes found
    goto default_seq;

  // Check for holes, unmapped indexes
  for (i = 1; i < image_count; i++)
  {
    if (next_index[i] == 0)
      break;
  }

  if (i < image_count) // there were holes
    goto default_seq;

  // all indexes found and no holes
  return;

default_seq:
    for (i = 0; i < image_count; i++)  // set the default sequence
      next_index[i] = i;
}

int imlabImageFileSave(imlabImageFile** image_file_list, int image_count)
{
  imlabImageFile* ImageFile = image_file_list[0];

  int error;
  imFile* ifile = imFileNew(ImageFile->filename, ImageFile->format, &error);
  if (error)
  {
    imlabDlgFileErrorMsg("File Save", error, ImageFile->filename);
    return 0;
  }

  imFileSetInfo(ifile, ImageFile->compression);

  int next_index[512];
  memset(next_index, 0, sizeof(int)*512);

  if (image_count > 1)
    iBuildIndexTabe(image_file_list, next_index, image_count);

  for (int i = 0; i < image_count; i++)
  {
    imlabImageFile* next_image_file = image_file_list[next_index[i]];

    error = imFileSaveImage(ifile, next_image_file->image);
    if (error)
    {
      if (error != IM_ERR_COUNTER) // Not the counter, show the error
        imlabDlgFileErrorMsg("File Save", error, ImageFile->filename);

      imFileClose(ifile);
      return 0;
    }

    // Updates the compression after saving
    imFileGetInfo(ifile, NULL, next_image_file->compression, NULL);
    next_image_file->changed = 0;
  }

  imFileClose(ifile);

  return 1;
}


#include <iup.h>
#include <iupcontrols.h>
#include <iupkey.h>
#include <im.h>
#include <im_binfile.h>
#include <cd.h>
#include <cdnative.h>

#include "imagefile.h"
#include "imlab.h"
#include "utl_file.h"
#include "dialogs.h"
#include "im_imageview.h"
#include <iup_config.h>

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


int imlabDlgGetConstant(const char* filetitle, const char* title, int color_space, double* param)
{
  char format[100];

  char full_title[100];
  sprintf(full_title, "%s - %s", title, filetitle);

  int depth = imColorModeDepth(color_space);

  if (depth == 3)
  {
    sprintf(format, "%s: %%R\n" 
                   "%s: %%R\n" 
                   "%s: %%R\n", 
                   imColorModeComponentName(color_space, 0), 
                   imColorModeComponentName(color_space, 1), 
                   imColorModeComponentName(color_space, 2));

    if (!IupGetParam(full_title, NULL, NULL, format, param, param + 1, param + 2, NULL))
      return 0;
  }
  else if (depth == 4)
  {
    sprintf(format, "%s: %%R\n" 
                   "%s: %%R\n" 
                   "%s: %%R\n"
                   "%s: %%R\n",
                   imColorModeComponentName(color_space, 0), 
                   imColorModeComponentName(color_space, 1), 
                   imColorModeComponentName(color_space, 2),
                   imColorModeComponentName(color_space, 3));

    if (!IupGetParam(full_title, NULL, NULL, format, param, param + 1, param + 2, param + 3, NULL))
      return 0;
  }
  else 
  {
    sprintf(format, "%s: %%R\n",
            imColorModeComponentName(color_space, 0));

    if (!IupGetParam(full_title, NULL, NULL, format, param, NULL))
      return 0;
  }

  return 1;
}

int imlabDlgCheckKernelParam(Ihandle* dialog, Ihandle* kernel_param, int *kernel_size)
{
  /* Spin callback in IupGetParam */
  if (IupGetInt(dialog, "SPINNING"))
  {
    Ihandle* kernel_ctrl = (Ihandle*)IupGetAttribute(kernel_param, "CONTROL");
    Ihandle* kernel_auxctrl = (Ihandle*)IupGetAttribute(kernel_param, "AUXCONTROL");
    int old_kernel_size = IupGetInt(kernel_ctrl, "VALUE");

    if ((*kernel_size) < 3)
    {
      *kernel_size = 3;

      IupSetInt(kernel_param, "VALUE", *kernel_size);
      IupSetInt(kernel_ctrl, "VALUE", *kernel_size);
      IupSetInt(kernel_ctrl, "SPINVALUE", *kernel_size);
      IupSetInt(kernel_auxctrl, "VALUE", *kernel_size);
      return -1;
    }

    /* if even increment or decrement one more */
    if ((*kernel_size) % 2 == 0)
    {
      if (old_kernel_size < *kernel_size)
        (*kernel_size)++;
      else
        (*kernel_size)--;

      IupSetInt(kernel_param, "VALUE", *kernel_size);
      IupSetInt(kernel_ctrl, "VALUE", *kernel_size);
      IupSetInt(kernel_ctrl, "SPINVALUE", *kernel_size);
      IupSetInt(kernel_auxctrl, "VALUE", *kernel_size);
      return -1;
    }
  }
  else
  {
    Ihandle* kernel_ctrl = (Ihandle*)IupGetAttribute(kernel_param, "CONTROL");
    /* while typing kernel_size can be temporarily invalid */
    if (*kernel_size < 3 || *kernel_size % 2 == 0)
    {
      IupSetAttribute(kernel_ctrl, "FGCOLOR", "255 0 0");
      return 0;
    }
    else
      IupSetAttribute(kernel_ctrl, "FGCOLOR", NULL);
  }

  return 1;
}

char* imlabDlgGetSizeDesc(double *size)
{
  char* size_desc;

  if (*size < 1024)
    size_desc = "b";
  else
  {
    *size /= 1024;

    if (*size < 1024)
      size_desc = "Kb";
    else
    {
      *size /= 1024;
      size_desc = "Mb";
    }
  }

  return size_desc;
}

void imlabDlgImageFileInfo(const char* filetitle, const imlabImageFile* ImageFile)
{
  char* format         = "----";
  char format_desc[50] = "----";
  char* compression    = "----";
  char* file_size_unit = "----";
  double file_size = 0;

  if (ImageFile->format[0] != 0)
  {
    format = (char*)ImageFile->format;
    imFormatInfo(ImageFile->format, format_desc, NULL, NULL);

    if (ImageFile->compression[0] != 0)
      compression = (char*)ImageFile->compression;
    else
      compression = "NONE";

    file_size = (double)utlFileSize(ImageFile->filename);

    if (file_size != (double)((unsigned long)-1))
      file_size_unit = imlabDlgGetSizeDesc(&file_size);
  }

  double image_size = (double)ImageFile->image->size;
  char* image_size_unit = imlabDlgGetSizeDesc(&image_size);

  char Msg[2048];
  sprintf(Msg, "File Name:\n"
               "   %s\n"
               "File Size:\n"
               "   %.2f %s\n"
               "File Format:\n"
               "   %s - %s\n"
               "   Compression - %s\n"
               "Image Dimensions:\n"
               "   Width:  %d\n"
               "   Height: %d\n"
               "   Depth:  %d\n"
               "Image Pixel:\n"
               "   Color Mode:  %s\n"
               "   Data Type:   %s\n"
               "   BPP:  %d\n"
               "   Has Alpha:  %s\n"
               "Image Size:\n"
               "   %.2f %s",
               ImageFile->filename, 
               file_size, file_size_unit, 
               format, format_desc, compression,
               ImageFile->image->width, 
               ImageFile->image->height, 
               ImageFile->image->depth, 
               imColorModeSpaceName(ImageFile->image->color_space),
               imDataTypeName(ImageFile->image->data_type),
               imDataTypeSize(ImageFile->image->data_type)*8,
               ImageFile->image->has_alpha? "Yes": "No",
               image_size, image_size_unit);

  char title[100];
  sprintf(title, "Image File Info - %s", filetitle);

  IupMessage(title, Msg);

  imlabLogMessagef(Msg);
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

int imlabDlgGetNewSizeCheck(Ihandle* dialog, int param_index, void* user_data)
{
  double factor = *(double*)user_data;
  int aspect = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "VALUE");

  if (aspect)
  {
    switch (param_index)
    {
    case 1: /* height */
      {
        Ihandle* width_param = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
        int height = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
        int width = (int)((double)height * factor + 0.5);
        Ihandle* ctrl = (Ihandle*)IupGetAttribute(width_param, "CONTROL");
        Ihandle* aux_ctrl = (Ihandle*)IupGetAttribute(width_param, "AUXCONTROL");
        IupSetfAttribute(width_param, "VALUE", "%d", width);
        IupSetfAttribute(ctrl, "VALUE", "%d", width);
        if (aux_ctrl) IupSetfAttribute(aux_ctrl, "VALUE", "%d", width);
        break;
      }
    case 3: /* aspect */
    case 0: /* width */
      {
        Ihandle* height_param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
        int width = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
        int height = (int)((double)width / factor + 0.5);
        Ihandle* ctrl = (Ihandle*)IupGetAttribute(height_param, "CONTROL");
        Ihandle* aux_ctrl = (Ihandle*)IupGetAttribute(height_param, "AUXCONTROL");
        IupSetfAttribute(height_param, "VALUE", "%d", height);
        IupSetfAttribute(ctrl, "VALUE", "%d", height);
        if (aux_ctrl) IupSetfAttribute(aux_ctrl, "VALUE", "%d", height);
        break;
      }
    }
  }

  return 1;
}

int imlabDlgGetNewSize(int *width, int *height, int *order, int org_width, int org_height)
{
  int index2order[3] = {0, 1, 3};
  int order2index[4] = {0, 1, 0, 2};
  int aspect = IupConfigGetVariableIntDef(imlabConfig(), "NewSize", "Aspect", 1);
  double factor = (double)org_width / (double)org_height;
  char format[512];
  sprintf(format, "Width [%d]: %%i[1,]\n"
                  "Height [%d]: %%i[1,]\n"
                  "Interpolation/Decimation Order: %%l|box|linear|cubic|\n" 
                  "Maintain Aspect: %%b\n", 
                  org_width, org_height);

  if (*width == 0) *width = org_width;
  if (*height == 0) *height = org_height;

  if (aspect)
    *height = (int)((double)(*width) / factor + 0.5);

  int order_index = order2index[*order];

  if (!IupGetParam("New Size", imlabDlgGetNewSizeCheck, &factor, format, 
                   width, height, &order_index, &aspect, NULL))
    return 0;

  *order = index2order[order_index];
  IupConfigSetVariableInt(imlabConfig(), "NewSize", "Aspect", aspect);

  return 1;
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

void imlabDlgMemoryErrorMsg(void)
{
  IupMessage("Error!", "Insufficient Memory.");
}

void imlabDlgFileErrorMsg(const char* title, int err, const char* filename)
{
  char msg[256];
  strcpy(msg, "File Name:\n   ");
  strcat(msg, filename);
  strcat(msg, "\n\n");

  switch (err)
  {
  case IM_ERR_OPEN:
    strcat(msg, "Error Opening File.\n");
    break;
  case IM_ERR_MEM:
    strcat(msg, "Insufficient memory.\n");
    break;
  case IM_ERR_ACCESS:
    strcat(msg, "Error Accessing File.\n");
    break;
  case IM_ERR_DATA:
    strcat(msg, "Image type not Suported.\n");
    break;
  case IM_ERR_FORMAT:
    strcat(msg, "Invalid Format.\n");
    break;
  case IM_ERR_COMPRESS:
    strcat(msg, "Invalid or unsupported compression.\n");
    break;
  default:
    strcat(msg, "Unknown Error.\n");
  }

  IupMessage((char*)title, msg);
}


/*******************************************************/
/*******************************************************/
/*******************************************************/

int imlabDlgGetRAWImageParam(int *top_down, int *switch_type, int *byte_order, int *packed, int *padding, unsigned long *offset, int *ascii)
{
  int index2padding[3] = {1, 2, 4};
  int padding2index[5] = {0, 0, 1, 0, 2};

  int padding_index = padding2index[*padding];

  if (!IupGetParam("RAW Parameters", NULL, NULL,
                   "Start Offset (in bytes): %i\n" 
                   " %t\n"
                   "Orientation: %l|BottomUp|TopDown|\n"
                   "Packed Planes: %b\n"
                   "Line Padding: %l|1|2|4|\n" 
                   " %t\n"
                   "Byte Order: %l|Intel|Motorola|\n"
                   "Switch Type: %b[ushort/int/float,short/uint/double]\n"
                   "Compression: %l|NONE|ASCII|\n",
                   offset, top_down, packed, &padding_index, byte_order, switch_type, ascii, NULL))
  {
    return 0;
  }

  *padding = index2padding[padding_index];
  return 1;
}

static int iDlgNewImageCheck(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;
  if (param_index < 0)
    return 1;

  if (param_index == 2)
  {
    int color_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    if (color_space == IM_BINARY || color_space == IM_MAP)
    {
      Ihandle* param3 = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
      IupSetAttribute((Ihandle*)IupGetAttribute(param3, "CONTROL"), "VALUE", "1");
      IupSetAttribute(param3, "VALUE", "0");
    }
  }
  else if (param_index == 3)
  {
    int color_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    if (color_space == IM_BINARY || color_space == IM_MAP)
      return 0;
  }

  return 1;
}

int imlabDlgGetNewImageParam(int *width, int *height, int *color_space, int *data_type)
{
  int has_alpha = imColorModeHasAlpha(*color_space);
  if (!IupGetParam("New Image", iDlgNewImageCheck, NULL,
                   "Width: %i[1,]\n"
                   "Height: %i[1,]\n"
                   "Color Space: %l|RGB|Map|Gray|Binary|CMYK|YCbCr|Lab|Luv|XYZ|\n"
                   "Data Type: %l|byte|short|ushort|int|float|double|cfloat|cdouble|\n"
                   "Has Alpha: %b\n",
                   width, height, color_space, data_type, &has_alpha, NULL))
  {
    return 0;
  }

  if (has_alpha)
    *color_space |= IM_ALPHA;

  return 1;
}

/*******************************************************/
/*******************************************************/
/*******************************************************/

#ifndef MOTIF
static void iDlgDrawImageFile(cdCanvas* cd_canvas, Ihandle* h, const char* filename)
{
  int wc, hc;
  cdCanvasGetSize(cd_canvas, &wc, &hc, NULL, NULL);
  cdCanvasClear(cd_canvas);

  if (!filename)
    return;

  imImage* image = (imImage*)IupGetAttribute(h, "_LASTIMAGE");
  char* last_file = IupGetAttribute(h, "_LASTFILE");
  if (!last_file || !imStrEqual(last_file, filename))
  {
    int error;
    if (image) imImageDestroy(image);
    image = imFileImageLoadBitmap(filename, 0, &error);
    IupSetAttribute(h, "_LASTIMAGE", (char*)image);
    IupStoreAttribute(h, "_LASTFILE", (char*)filename);
    if (!image && error == IM_ERR_ACCESS)
      imlabDlgFileErrorMsg("Load Bitmap", error, filename);
  }

  if (image)
  {
    int x, y, w, h;
    imImageViewFitRect(wc-10, hc-10, image->width, image->height, &w, &h);
    x = (wc - w) / 2;
    y = (hc - h) / 2;
    imImageViewDrawImage(cd_canvas, image, x, y, w, h);
  }
}

static int iDlgFile_CB(Ihandle* h, const char* filename, const char* status)
{
  if (status[0] == 'P') /* PAINT */
  {
    char strdata[50];
    sprintf(strdata,"%p %dx%d", IupGetAttribute(h, "PREVIEWDC"), IupGetInt(h, "PREVIEWWIDTH"), IupGetInt(h, "PREVIEWHEIGHT")); 
    cdCanvas* cd_canvas = cdCreateCanvas(CD_NATIVEWINDOW, strdata);
    cdCanvasActivate(cd_canvas);
    iDlgDrawImageFile(cd_canvas, h, filename);
    cdKillCanvas(cd_canvas);
  }

  if (status[0] == 'F') /* FINISH */
  {
    imImage* image = (imImage*)IupGetAttribute(h, "_LASTIMAGE");
    if (image) imImageDestroy(image);
  }

  return IUP_DEFAULT;
}
#endif

static int iDlgGetFileName(char *filename, int is_multi, int is_open, int show_file_preview)
{
  const char* last_imagepath = IupConfigGetVariableStr(imlabConfig(), "FileSelection", "LastImageDirectory");
  Ihandle *dlg = IupFileDlg(); 

#ifndef MOTIF
  char* format_list[50];
  int format_count = 0;

  if (is_multi)
  {
    char all_ext[20480] = "";
    char filters[20480] = 
    {
	    "All Files (*.*)|*.*|"
    };

    imFormatList(format_list, &format_count);

    char file_format[20480];
    int f = 0;
    for (int i = 0; i < format_count; i++)
    {
      char format_desc[50];
      char format_ext[50];
      int can_sequence;
      imFormatInfo(format_list[i], format_desc, format_ext, &can_sequence);

      if (is_multi == 2 && !can_sequence)
        continue;

      sprintf(file_format, "%s - %s (%s)|%s|", format_list[i], format_desc, format_ext, format_ext);
      strcat(filters, file_format);
      strcat(all_ext, format_ext);

      format_list[f] = format_list[i];  /* for posteriori retrieval */
      f++;
    }

    sprintf(file_format, "All Image Files|%s|", all_ext);
    strcat(filters, file_format);

    IupSetAttribute(dlg, "EXTFILTER", filters); 

    if (is_open)
    {
      if (show_file_preview)
      {
        IupSetCallback(dlg, "FILE_CB", (Icallback)iDlgFile_CB);
        IupSetAttribute(dlg, "SHOWPREVIEW", "YES");
      }

      IupSetAttribute(dlg, "MULTIPLEFILES", "YES"); 
    }
  }
  else
#endif
  {
    IupSetAttribute(dlg, "FILTER", "*.*"); 
    IupSetAttribute(dlg, "FILTERINFO", "All Files (*.*)"); 
  }

  if (is_open)
  {
    IupSetAttribute(dlg, "DIALOGTYPE", "OPEN");
    IupSetAttribute(dlg, "TITLE", "Open"); 
  }
  else
  {
    IupSetAttribute(dlg, "DIALOGTYPE", "SAVE");
    IupSetAttribute(dlg, "TITLE", "Save As"); 
    IupSetAttribute(dlg, "FILE", filename);
  }

  IupSetAttribute(dlg, "NOCHANGEDIR", "YES"); 
  IupSetAttribute(dlg, "PARENTDIALOG", "imlabMainWindow");
  IupSetStrAttribute(dlg, "DIRECTORY", last_imagepath); 

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT); 

  if (IupGetInt(dlg, "STATUS") == -1)
  {
    IupDestroy(dlg);
    return 0;
  }

  strcpy(filename, IupGetAttribute(dlg, "VALUE"));

  char* last_directory = NULL;
#ifndef MOTIF
  if (is_multi)
  {
    int offset;
    last_directory = utlFileGetPathMultiple(filename, &offset);
  }
  else
    last_directory = utlFileGetPath(filename);
#else
  last_directory = utlFileGetPath(filename);
#endif

  if (last_directory)
  {
    IupConfigSetVariableStr(imlabConfig(), "FileSelection", "LastImageDirectory", last_directory);
    free(last_directory);
  }

  IupDestroy(dlg);
  return 1;
}

int imlabDlgGetOpenImageFileNamePreview(char *filename)
{
  return iDlgGetFileName(filename, 1, 1, 1);
}

int imlabDlgGetOpenImageFileName(char *filename)
{
  return iDlgGetFileName(filename, 1, 1, 0);
}

int imlabDlgGetSaveAsImageFileName(char *filename, int is_multi)
{
  return iDlgGetFileName(filename, is_multi, 0, 0);
}

int imlabDlgSelectFile(char* filename, const char* type, const char* title, const char* extfilter, const char* dirname)
{
  Ihandle* filedlg = IupFileDlg();
  const char* dir = IupConfigGetVariableStr(imlabConfig(), "FileSelection", dirname);

  IupSetStrAttribute(filedlg, "DIALOGTYPE", type);
  IupSetStrAttribute(filedlg, "EXTFILTER", extfilter);
  IupSetStrAttribute(filedlg, "TITLE", title);
  IupSetStrAttribute(filedlg, "FILE", filename);
  IupSetStrAttribute(filedlg, "DIRECTORY", dir);
  IupSetAttribute(filedlg, "PARENTDIALOG", "imlabMainWindow");

  IupPopup(filedlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
  if (IupGetInt(filedlg, "STATUS") != -1)
  {
    char* value = IupGetAttribute(filedlg, "VALUE");
    strcpy(filename, value);

    char* last_directory = utlFileGetPath(filename);
    if (last_directory)
    {
      IupConfigSetVariableStr(imlabConfig(), "FileSelection", dirname, last_directory);
      free(last_directory);
    }

    IupDestroy(filedlg);
    return 1;
  }

  IupDestroy(filedlg);
  return 0;
}

void imlabDlgMessagef(const char* title, const char* format, ...)
{
  char text[10240];
  va_list arglist;
  va_start(arglist, format);
  vsprintf(text, format, arglist);
  va_end(arglist);

  IupMessage(title, text);
  imlabLogMessagef(text);
}

int imlabDlgQuestion(const char *msg, int has_cancel)
{
  Ihandle* dlg = IupMessageDlg();

  IupSetAttribute(dlg, "DIALOGTYPE", "QUESTION");
  IupSetAttribute(dlg, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(dlg, "TITLE", "Attention!");
  IupSetAttribute(dlg, "BUTTONS", has_cancel? "YESNOCANCEL": "YESNO");
  IupSetStrAttribute(dlg, "VALUE", msg);

  IupPopup(dlg, IUP_CENTERPARENT, IUP_CENTERPARENT);
  int ret = IupGetInt(dlg, "BUTTONRESPONSE");
  IupDestroy(dlg);

  return ret;
}

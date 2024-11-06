
#include <iup.h>
#include <iupcontrols.h>
#include <iupkey.h>

#include <im.h>
#include <im_process.h>
#include <im_convert.h>
#include <im_kernel.h>

#include "kernel.h"
#include "utl_file.h"
#include "dialogs.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

imImage* imKernelLoad(char* filename)
{
  int error;
  imFile* ifile = imFileOpen(filename, &error);
  if (!ifile) 
  {
    imlabDlgFileErrorMsg("Kernel Load", error, filename);
    return 0;
  }
  
  imImage* image = imFileLoadImage(ifile, 0, &error);  // load the first image in the file.
  if (!image)
    imlabDlgFileErrorMsg("Kernel Load", error, filename);

  if (image->color_space != IM_GRAY && image->color_space != IM_BINARY)
  {
    IupMessage("Error!", "Invalid kernel color space. Must be GRAY or BINARY.");
    imImageDestroy(image);
    return 0;
  }

  if (image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE)
  {
    IupMessage("Error!", "Invalid kernel data type. Can not be complex.");
    imImageDestroy(image);
    return 0;
  }

  if (image->data_type == IM_BYTE || image->data_type == IM_USHORT)
  {
    imImage* int_image = imImageCreateBased(image, -1, -1, -1, IM_INT);
    imProcessConvertDataType(image, int_image, 0, 0, 0, IM_CAST_DIRECT);
    imImageDestroy(image);
    image = int_image;
  }
  else if (image->data_type == IM_DOUBLE)
  {
    imImage* float_image = imImageCreateBased(image, -1, -1, -1, IM_FLOAT);
    imProcessConvertDataType(image, float_image, 0, 0, 0, IM_CAST_DIRECT);
    imImageDestroy(image);
    image = float_image;
  }

  const char* desc = (const char*)imImageGetAttribute(image, "Description", NULL, NULL);
  if (!desc)
  {
    char* file_title = utlFileGetTitle(filename);
    imImageSetAttribString(image, "Description", file_title);
    free(file_title);
  }
    
  imFileClose(ifile);  

  return image;
}

static int iSaveImage(imImage* image, const char* filename, char* format, char* compression)
{
  int error;
  imFile* ifile = imFileNew(filename, format, &error);
  if (!ifile)
  {
    imlabDlgFileErrorMsg("Kernel Save", error, filename);
    return 0;
  }

  imFileSetInfo(ifile, compression);
  
  error = imFileSaveImage(ifile, image);
  if (error)
  {
    imlabDlgFileErrorMsg("Kernel Save", error, filename);
    imFileClose(ifile);  
    return 0;
  }

  imFileClose(ifile);  
  return 1;
}


/*************************************************************************/
/*************************************************************************/


struct iKernel
{
  char filename[10240];
  imImage* image;
  Ihandle *matrix,
          *desc;
  int w, h, data_type;    
  int cancel, changed, square;
  float data[IM_KERNELMAX][IM_KERNELMAX]; /* same organization as the IupMatrix (top-down) */
};

static int iKernelEditionCB(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");
  kernel->changed = 1;
  return IUP_DEFAULT;
}

static int iKernelEnterItemCB(Ihandle *self, int lin, int col)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  if (kernel->data_type == IM_FLOAT)
    IupSetAttributeId2(kernel->matrix, "MASK", lin, col, IUP_MASK_FLOAT);
  else
    IupSetAttributeId2(kernel->matrix, "MASK", lin, col, IUP_MASK_INT);

  return IUP_DEFAULT;
}

static int iKernelUpdateFromMatrix(iKernel* kernel)
{
  char *value;
  int l, c, kh2, kw2;
  float v;

  /* get all the kernel cells */

  kernel->data_type = IM_INT;
  memset(kernel->data, 0, sizeof(float)*IM_KERNELMAX*IM_KERNELMAX);
  for(l = 0; l < IM_KERNELMAX; l++)
  {
    for(c = 0; c < IM_KERNELMAX; c++)
    {
      value = IupGetAttributeId2(kernel->matrix, "", l + 1, c + 1);
      if (value && value[0])
      {
        v = IupGetFloatId2(kernel->matrix, "", l + 1, c + 1);
        if (floor(v) != v)
          kernel->data_type = IM_FLOAT;
        kernel->data[l][c] = v;
      }
    }
  }

  /* find the real kernel */

  {
    int start = 0, end = IM_KERNELMAX-1, found = 0;
    while (!found)
    {
      if (start > end)
        return 0;

      for(l = 0; l < IM_KERNELMAX; l++)
      {
        value = IupGetAttributeId2(kernel->matrix, "", l+1, start+1);
        if (value && value[0])
        {
          found = 1;
          break;
        }

        value = IupGetAttributeId2(kernel->matrix, "", l+1, end+1);
        if (value && value[0])
        {
          found = 1;
          break;
        }
      }

      if (!found)
      {
        start++;
        end--;
      }
    }
    kernel->w = end-start+1;

    start = 0, end = IM_KERNELMAX-1, found = 0;
    while (!found)
    {
      if (start > end)
        return 0;

      for(c = 0; c < IM_KERNELMAX; c++)
      {
        value = IupGetAttributeId2(kernel->matrix, "", start+1, c+1);
        if (value && value[0])
        {
          found = 1;
          break;
        }

        value = IupGetAttributeId2(kernel->matrix, "", end+1, c+1);
        if (value && value[0])
        {
          found = 1;
          break;
        }
      }

      if (!found)
      {
        start++;
        end--;
      }
    }
    kernel->h = end-start+1;
  }

  if (kernel->square)
  {
    int maxsize = kernel->h > kernel->w? kernel->h: kernel->w;
    kernel->h = maxsize;
    kernel->w = maxsize;
  }

  /* shift the kernel to the origin */

  kh2 = (IM_KERNELMAX - kernel->h) / 2;
  kw2 = (IM_KERNELMAX - kernel->w) / 2;

  if (kernel->h < IM_KERNELMAX)
  {
    for(l = 0; l < kernel->h; l++)
      for(c = 0; c < IM_KERNELMAX; c++)
        kernel->data[l][c] = kernel->data[l+kh2][c];
  }

  if (kernel->w < IM_KERNELMAX)
  {
    for(c = 0; c < kernel->w; c++)
      for(l = 0; l < IM_KERNELMAX; l++)
        kernel->data[l][c] = kernel->data[l][c+kw2];
  }

  {
    char* desc = IupGetAttribute(kernel->desc, "VALUE");
    if (desc && desc[0] && kernel->image)
      imImageSetAttribString(kernel->image, "Description", desc);
  }

  return 1;
}

static void iKernelUpdateMatrix(iKernel* kernel)
{
  char text[50];
  int kh2 = (IM_KERNELMAX - kernel->h) / 2;
  int kw2 = (IM_KERNELMAX - kernel->w) / 2;

  if (kernel->h == 0 || kernel->w == 0)
  {
    kh2 = IM_KERNELMAX;
    kw2 = IM_KERNELMAX;
  }

  for(int l = 0; l < IM_KERNELMAX; l++)
  {
    for(int c = 0; c < IM_KERNELMAX; c++)
    {
      if (c >= kw2 && c < IM_KERNELMAX - kw2 && l >= kh2 && l < IM_KERNELMAX - kh2)
      {
        sprintf(text, "%.3f", kernel->data[l - kh2][c - kw2]);
        IupSetStrAttributeId2(kernel->matrix, "", l + 1, c + 1, text);
      }
      else
        IupSetAttributeId2(kernel->matrix, "", l + 1, c + 1, "");
    }
  }
  IupSetAttribute(kernel->matrix, "REDRAW", "YES");

  if (kernel->image)
  {
    char* desc = (char*)imImageGetAttribute(kernel->image, "Description", NULL, NULL);
    if (!desc) desc = "";
    IupSetAttribute(kernel->desc, "VALUE", desc);
  }
}

static void iKernelUpdateImage(iKernel* kernel)
{
  if (kernel->image && (kernel->image->width != kernel->w ||
                        kernel->image->height != kernel->h ||
                        kernel->image->data_type != kernel->data_type))
  {
    imImageDestroy(kernel->image);
    kernel->image = NULL;
  }

  if (!kernel->image)
    kernel->image = imImageCreate(kernel->w, kernel->h, IM_GRAY, kernel->data_type);

  float *fdata = (float*)kernel->image->data[0];
  int *idata = (int*)kernel->image->data[0];

  for(int ky = 0; ky < kernel->h; ky++)
  {
    for(int kx = 0; kx < kernel->w; kx++)
    {
      if (kernel->image->data_type == IM_INT)
        *idata++ = (int)kernel->data[kernel->h-1 - ky][kx];
      else
        *fdata++ = kernel->data[kernel->h-1 - ky][kx];
    }
  }
}

static void iKernelUpdateFromImage(iKernel* kernel)
{
  float *fdata = (float*)kernel->image->data[0];
  int *idata = (int*)kernel->image->data[0];

  kernel->w = kernel->image->width;
  kernel->h = kernel->image->height;

  for(int ky = 0; ky < kernel->h; ky++)
  {
    for(int kx = 0; kx < kernel->w; kx++)
    {
      if (kernel->image->data_type == IM_INT)
        kernel->data[kernel->h-1 - ky][kx] = (float)(*idata++);
      else
        kernel->data[kernel->h-1 - ky][kx] = *fdata++;
    }
  }
}

static int iKernelOK(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  if(!iKernelUpdateFromMatrix(kernel))
  {
    IupMessage("Error!", "Invalid kernel.");
    return IUP_DEFAULT;
  }

  iKernelUpdateImage(kernel);

  if (kernel->changed)
  {
    strcpy(kernel->filename, "New Kernel");
  }
  else
  {
    char* filename = utlFileGetTitle(kernel->filename);
    strcpy(kernel->filename, filename);
    free(filename);
  }

  return IUP_CLOSE;
}

static int iKernelCancel(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");
  kernel->cancel = 1;
  return IUP_CLOSE;
}

static int iKernelLoad(Ihandle *self)
{
  char filename[10240];
  imImage* new_image;
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  strcpy(filename, "*.krn");
  
  if (!imlabDlgGetOpenImageFileName(filename))
    return IUP_DEFAULT;

  new_image = imKernelLoad(filename);
  if (!new_image)
    return IUP_DEFAULT;

  if (new_image->width%2 == 0 || new_image->height%2 == 0)
  {
    IupMessage("Error!", "Invalid kernel size. Must be odd.");
    imImageDestroy(new_image);
    return IUP_DEFAULT;
  }

  if (new_image->width > IM_KERNELMAX || new_image->height > IM_KERNELMAX)
  {
    IupMessage("Error!", "Invalid kernel size. Too big for the dialog.");
    imImageDestroy(new_image);
    return IUP_DEFAULT;
  }

  if (kernel->image)
    imImageDestroy(kernel->image);

  kernel->image = new_image;

  iKernelUpdateFromImage(kernel);

  strcpy(kernel->filename, filename);

  iKernelUpdateMatrix(kernel);

  return IUP_DEFAULT;
}

static int iKernelSave(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  if (!iKernelUpdateFromMatrix(kernel))
  {
    IupMessage("Error!", "Invalid kernel.");
    return IUP_DEFAULT;
  }

  iKernelUpdateImage(kernel);

  char filename[10240], format[30], compression[50] = "";
  strcpy(filename, kernel->filename);
  strcpy(format, "KRN");
  if (!imlabDlgImageFileSaveAs(kernel->image, filename, format, compression, 0))
    return IUP_DEFAULT;

  if (!iSaveImage(kernel->image, filename, format, compression))
    return IUP_DEFAULT;

  kernel->changed = 0;
  strcpy(kernel->filename, filename);

  return IUP_DEFAULT;
}

static int iKernelRotate(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  int old_square = kernel->square;
  kernel->square = 1;

  if (!iKernelUpdateFromMatrix(kernel))
  {
    IupMessage("Error!", "Invalid kernel.");
    kernel->square = old_square;
    return IUP_DEFAULT;
  }

  iKernelUpdateImage(kernel);
  imProcessRotateKernel(kernel->image);
  iKernelUpdateFromImage(kernel);

  iKernelUpdateMatrix(kernel);
  kernel->square = old_square;

  return IUP_DEFAULT;
}

static int do_init_kernel(Ihandle *self, imImage* image)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  if (kernel->image)
    imImageDestroy(kernel->image);

  kernel->image = image;

  iKernelUpdateFromImage(kernel);

  strcpy(kernel->filename, "");

  iKernelUpdateMatrix(kernel);

  return IUP_DEFAULT;
}

static int kernelsobel(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelSobel());
}
static int kernelprewitt(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelPrewitt());
}

static int kernelkirsh(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelKirsh());
}

static int kernellaplacian4(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelLaplacian4());
}

static int kernellaplacian8(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelLaplacian8());
}

static int kernellaplacian5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelLaplacian5x5());
}

static int kernellaplacian7x7(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelLaplacian7x7());
}

static int kernelgradian3x3(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelGradian3x3());
}

static int kernelgradian7x7(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelGradian7x7());
}

static int kernelsculpt(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelSculpt());
}

static int kernelmean3x3(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelMean3x3());
}

static int kernelmean5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelMean5x5());
}

static int kernelcircularmean5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelCircularMean5x5());
}

static int kernelmean7x7(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelMean7x7());
}

static int kernelcircularmean7x7(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelCircularMean7x7());
}

static int kernelgaussian3x3(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelGaussian3x3());
}

static int kernelgaussian5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelGaussian5x5());
}

static int kernelbarlett5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelBarlett5x5());
}

static int kerneltophat5x5(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelTopHat5x5());
}

static int kerneltophat7x7(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelTopHat7x7());
}

static int kernelenhance(Ihandle *parent)
{
  return do_init_kernel(parent, imKernelEnhance());
}

static Ihandle *iKernelNewItem(char* title, Icallback cb)
{
  Ihandle* item = IupItem(title, NULL);
  IupSetCallback(item, "ACTION", cb);
  return item;
}

static int iKernelPreDefined(Ihandle *self)
{
  iKernel* kernel = (iKernel*)IupGetAttribute(self, "iKernel");

  Ihandle *menu = IupMenu(
    iKernelNewItem("Sobel", (Icallback)kernelsobel),
    iKernelNewItem("Prewitt", (Icallback)kernelprewitt),
    iKernelNewItem("Kirsh", (Icallback)kernelkirsh),
    iKernelNewItem("Laplacian4", (Icallback)kernellaplacian4),
    iKernelNewItem("Laplacian8", (Icallback)kernellaplacian8),
    iKernelNewItem("Laplacian5x5", (Icallback)kernellaplacian5x5),
    iKernelNewItem("Laplacian7x7", (Icallback)kernellaplacian7x7),
    iKernelNewItem("Gradian3x3", (Icallback)kernelgradian3x3),
    iKernelNewItem("Gradian7x7", (Icallback)kernelgradian7x7),
    IupSeparator(),
    iKernelNewItem("Mean3x3", (Icallback)kernelmean3x3),
    iKernelNewItem("Mean5x5", (Icallback)kernelmean5x5),
    iKernelNewItem("CircularMean5x5", (Icallback)kernelcircularmean5x5),
    iKernelNewItem("Mean7x7", (Icallback)kernelmean7x7),
    iKernelNewItem("CircularMean7x7", (Icallback)kernelcircularmean7x7),
    iKernelNewItem("Gaussian3x3", (Icallback)kernelgaussian3x3),
    iKernelNewItem("Gaussian5x5", (Icallback)kernelgaussian5x5),
    iKernelNewItem("Barlett5x5", (Icallback)kernelbarlett5x5),
    IupSeparator(),
    iKernelNewItem("Sculpt", (Icallback)kernelsculpt),
    iKernelNewItem("TopHat5x5", (Icallback)kerneltophat5x5),
    iKernelNewItem("TopHat7x7", (Icallback)kerneltophat7x7),
    iKernelNewItem("Enhance", (Icallback)kernelenhance),
    NULL);

  int x = IupGetInt(self, "X");
  int y = IupGetInt(self, "Y");
  y += IupGetInt2(self, "RASTERSIZE");
  IupSetAttribute(menu,"iKernel", (char*)kernel);
  IupPopup(menu, x, y);
  IupDestroy(menu);

  return IUP_DEFAULT;
}

imImage* imGetKernel(const char* filetitle, int square)
{
  static float last_data[IM_KERNELMAX][IM_KERNELMAX];
  static int last_w, last_h;

  Ihandle *dialog, *matrix, *box, *btok, *btesc, *desc;
  iKernel* kernel;
  int ky, kx;

  {
    static int first = 1;
    if (first)
    {
      memset(last_data, 0, sizeof(last_data));
      last_w = 0;
      last_h = 0;
      first = 0;
    }
  }

  kernel = (iKernel*)malloc(sizeof(iKernel));

  kernel->w = last_w;
  kernel->h = last_h;
  for(ky = 0; ky < last_w; ky++)
    for(kx = 0; kx < last_w; kx++) 
      kernel->data[ky][kx] = last_data[ky][kx];

  strcpy(kernel->filename, "*.krn");
  kernel->image = NULL;
  kernel->changed = 0;
  kernel->cancel = 0;
  kernel->square = square;
  kernel->data_type = IM_FLOAT;

  matrix = IupMatrix(NULL);
  kernel->matrix = matrix;

  IupSetInt(matrix, "NUMCOL", IM_KERNELMAX);
  IupSetInt(matrix, "NUMLIN", IM_KERNELMAX);
  IupSetInt(matrix, "NUMCOL_VISIBLE", IM_KERNELMAX);
  IupSetInt(matrix, "NUMLIN_VISIBLE", IM_KERNELMAX);
  IupSetAttribute(matrix,"WIDTHDEF","20");
  IupSetAttribute(matrix,"WIDTH0" ,"12");
  IupSetAttribute(matrix,"BGCOLOR","255 255 255");
  IupSetAttribute(matrix,"SCROLLBAR", "NO");
  IupSetAttribute(matrix,"EXPAND", "NO");
  IupSetCallback(matrix, "ENTERITEM_CB", (Icallback)iKernelEnterItemCB);
  IupSetCallback(matrix, "EDITION_CB", (Icallback)iKernelEditionCB);
  IupSetAttribute(matrix,"0:1","-3");
  IupSetAttribute(matrix,"0:2","-2");
  IupSetAttribute(matrix,"0:3","-1");
  IupSetAttribute(matrix,"0:4","0");
  IupSetAttribute(matrix,"0:5","1");
  IupSetAttribute(matrix,"0:6","2");
  IupSetAttribute(matrix,"0:7","3");
  IupSetAttribute(matrix,"1:0","3");
  IupSetAttribute(matrix,"2:0","2");
  IupSetAttribute(matrix,"3:0","1");
  IupSetAttribute(matrix,"4:0","0");
  IupSetAttribute(matrix,"5:0","-1");
  IupSetAttribute(matrix,"6:0","-2");
  IupSetAttribute(matrix,"7:0","-3");
  IupSetAttribute(matrix,"BGCOLOR4:4","192 192 192");
  IupSetAttribute(matrix,"ALIGNMENT0","ARIGHT");

  iKernelUpdateMatrix(kernel);

  btok = IupButton("OK", NULL);
  IupSetHandle("btKernelOK", btok);
  IupSetCallback(btok, "ACTION", (Icallback)iKernelOK);
  btesc = IupButton("Cancel", NULL);
  IupSetHandle("btKernelCancel", btesc);
  IupSetCallback(btesc, "ACTION", (Icallback)iKernelCancel);

  desc = IupText(NULL);
  IupSetAttribute(desc, "EXPAND", "HORIZONTAL");
  kernel->desc = desc;

  box = IupSetAttributes(IupVbox(
          IupFrame(IupHbox(
            IupSetAttributes(IupVbox(
              IupHbox(
                IupLabel("Description:"),
                desc,
                NULL), 
              matrix,
              NULL), "MARGIN=0x0, GAP=5"),
            IupSetAttributes(IupVbox(
              IupSetCallbacks(IupSetAttributes(IupButton("Load...", NULL), "SIZE=50x15"), "ACTION", (Icallback)iKernelLoad, NULL),
              IupSetCallbacks(IupSetAttributes(IupButton("Pre-Loaded", NULL), "SIZE=50x15"), "ACTION", (Icallback)iKernelPreDefined, NULL),
              IupSetCallbacks(IupSetAttributes(IupButton("Save...", NULL), "SIZE=50x15"), "ACTION", (Icallback)iKernelSave, NULL),
              IupSetAttributes(IupLabel(NULL), "SEPARATOR=HORIZONTAL"), 
              IupSetCallbacks(IupSetAttributes(IupButton("Rotate", NULL), "SIZE=50x15"), "ACTION", (Icallback)iKernelRotate, NULL),
              NULL), "MARGIN=0x0, GAP=5"),
            NULL)),
          IupSetAttributes(IupHbox(
            IupFill(),
            IupSetAttributes(btok, "SIZE=50x15"), 
            IupSetAttributes(btesc, "SIZE=50x15"), 
            NULL), "MARGIN=0x0, GAP=5"),
          NULL), "MARGIN=10x10, GAP=15");

  dialog = IupDialog(box);
  IupSetAttribute(dialog,"MINBOX","NO");
  IupSetAttribute(dialog,"RESIZE","NO");
  IupSetAttribute(dialog,"MAXBOX","NO");
  IupSetStrf(dialog, "TITLE", "Convolution Kernel - %s", filetitle);
  IupSetAttribute(dialog,"MENUBOX","NO");
  IupSetAttribute(dialog,"PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dialog,"DEFAULTENTER","btKernelOK");
  IupSetAttribute(dialog,"DEFAULTESC","btKernelCancel");
  IupSetAttribute(dialog,"iKernel", (char*)kernel);

  IupPopup(dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);
  IupDestroy(dialog);

  if (kernel->cancel)
  {
    if (kernel->image) imImageDestroy(kernel->image);
    free(kernel);
    return NULL;
  }
  else
  {
    imImage* image = kernel->image;

    for(ky = 0; ky < kernel->h; ky++)
      for(kx = 0; kx < kernel->w; kx++) 
        last_data[ky][kx] = kernel->data[ky][kx];
    last_w = kernel->w;
    last_h = kernel->h;

    const char* desc = (const char*)imImageGetAttribute(kernel->image, "Description", NULL, NULL);
    if (!desc)
    {
      char* file_title = utlFileGetTitle(kernel->filename);
      imImageSetAttribString(kernel->image, "Description", file_title);
      free(file_title);
    }

    free(kernel);
    return image;
  }
}


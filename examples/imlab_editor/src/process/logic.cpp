#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>

#include <stdlib.h>
#include <memory.h>
#include <string.h>

static char* GetBitwiseOpStr(int op)
{
  switch(op)
  {
  case IM_BIT_AND:
    return "AND";
  case IM_BIT_OR:
    return "OR";
  case IM_BIT_XOR:
    return "XOR";
  }

  return NULL;
}

static int bit_op(int op, Ihandle *parent)
{
  imImage *NewImage, *image1, *image2; 
  imlabImageDocument *document1, *document2;

  document1 = imlabGetCurrentDocument(parent);
  image1 = document1->ImageFile->image;

  document2 = imlabImageDocumentListSelect("Select Second Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  image2 = document2->ImageFile->image;

  NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessBitwiseOp(image1, image2, NewImage, op);

  document1->ChangeImage(NewImage, "BitwiseOp{image=\"%s\", op=\"%s\"}", document2->FileTitle, GetBitwiseOpStr(op));

  return IUP_DEFAULT;
}
                  
static int and_bitop(Ihandle *parent)
{
  return bit_op(IM_BIT_AND, parent);
}

static int or_bitop(Ihandle *parent)
{
  return bit_op(IM_BIT_OR, parent);
}

static int xor_bitop(Ihandle *parent)
{
  return bit_op(IM_BIT_XOR, parent);
}

static int not_bitop(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessBitwiseNot(image, NewImage);

  document->ChangeImage(NewImage, "BitwiseNot{}");

  return IUP_DEFAULT;
}
                  
static int split_bitplanes(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  for (int i = 0; i < 8; i++)
  {
    imImage *NewImage = imImageClone(image);
    if (!NewImage)
    {
      imlabDlgMemoryErrorMsg();
      return IUP_DEFAULT;
    }

    imProcessBitPlane(image, NewImage, i, 0);
    imImageSetBinary(NewImage);
    imlabImageDocumentCreateFromImage(NewImage, "BitPlane of %s", "BitPlane{image=\"%s\", plane=%d, reset=0}", document->FileTitle, i);
  }

  return IUP_DEFAULT;
}
                  
static void imImageResetGray(imImage* image)
{
  if (image->palette)
  {
    image->color_space = IM_GRAY;
    image->palette[1] = imColorEncode(1,1,1);
  }
}

static int bit_plane_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int plane = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int reset = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    imProcessBitPlane(image, NewImage, plane, reset);

    if (reset)
      imImageResetGray(NewImage);
    else
      imImageSetBinary(NewImage);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int bit_plane(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static int plane = 7;
  static int reset = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Bit Plane"), bit_plane_preview, preview,
                   "Plane Number: %i[0,7]\n"
                   "Operation: %l|extract|reset|\n"
                   "Preview: %b\n",
                   &plane, &reset, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
  {
    imProcessBitPlane(image, NewImage, plane, reset);

    if (reset)
      imImageResetGray(NewImage);
    else
      imImageSetBinary(NewImage);
  }

  document->ChangeImage(NewImage, "BitPlane{plane=%d, reset=%d}", plane, reset);

  return IUP_DEFAULT;
}

static imbyte make_mask(char* bit_mask)
{
  imbyte mask = 0;
  for (int b = 0; b < 8; b++)
  {
    if (bit_mask[b] == '1')
      mask |= (imbyte)(1 << (8-b));
  }
  return mask;
}

static int bit_mask_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  if (param_index == 0)
  {
    char* bit_mask = IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    if ((int)strlen(bit_mask) != 8)
      return 0;
  }

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    char* bit_mask = IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    int op = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    imProcessBitMask(image, NewImage, make_mask(bit_mask), op);
    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int bit_mask(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  char bit_mask[10] = "11111111";
  static int op = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Bit Mask"), bit_mask_preview, preview,
                   "Mask: %s[0-1]+\n"
                   "Operation: %l|and|or|xor|\n"
                   "Preview: %b\n",
                   &bit_mask, &op, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessBitMask(image, NewImage, make_mask(bit_mask), op);

  document->ChangeImage(NewImage, "BitMask{bit_mask=\"%s\", op=\"%s\"}", bit_mask, GetBitwiseOpStr(op));

  return IUP_DEFAULT;
}
                  
static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu, *menu;

  menu = IupMenu(
    imlabProcNewItem(mnProcess, "And...", "and_op", (Icallback) and_bitop, 1),
    imlabProcNewItem(mnProcess, "Or...",  "or_op", (Icallback) or_bitop, 1),
    imlabProcNewItem(mnProcess, "Xor...", "xor_op", (Icallback) xor_bitop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Not", "not_op", (Icallback) not_bitop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Bit Plane...", "bit_plane", (Icallback) bit_plane, 1),
    imlabProcNewItem(mnProcess, "Bit Mask...", "bit_mask", (Icallback) bit_mask, 1),
    imlabProcNewItem(mnProcess, "Split Bit Planes", "split_bitplanes", (Icallback) split_bitplanes, 1),
    NULL);

  submenu = imlabSubmenu("Logic", menu);
  IupAppend(mnProcess, submenu);
  IupSetAttribute(submenu, "imlabStatusHelp", "Logical Arithmetic Operations.");
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "and_op", imImageIsInteger);
  imlabProcPlugInUpdateItem(mnProcess, "or_op", imImageIsInteger);
  imlabProcPlugInUpdateItem(mnProcess, "xor_op", imImageIsInteger);
  imlabProcPlugInUpdateItem(mnProcess, "not_op", imImageIsInteger);
  imlabProcPlugInUpdateItem(mnProcess, "bit_plane", imImageIsByte);
  imlabProcPlugInUpdateItem(mnProcess, "bit_mask", imImageIsByte);
  imlabProcPlugInUpdateItem(mnProcess, "split_bitplanes", imImageIsSciByte);
}


static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinLogic = &plugin;


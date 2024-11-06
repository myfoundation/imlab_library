#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>

#include <cd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>

#include <cdprint.h>
#include <cdpdf.h>
#include <cdps.h>
#include <cdiup.h>
#include <cdclipbd.h>
#include <cdcgm.h>
#include <cdirgb.h>
#include <cdwmf.h>
#include <cdemf.h>
#include <cdsvg.h>

#include <iup.h>
#include <iupcontrols.h>

#include <im.h>
#include <im_image.h>
#include <im_convert.h>
#include <im_palette.h>
#include <im_process.h>

#include "mainwindow.h"
#include "im_imagematch.h"
#include "im_imageview.h"
#include "imagewindow.h"
#include "imagedocument.h"
#include "documentlist.h"
#include "resultswindow.h"
#include "imlab.h"
#include "dialogs.h"
#include "utl_file.h"
#include "im_clipboard.h"
#include <iup_config.h>



/*******************************/
/* Image Menu call backs       */
/*******************************/


static void UpdateImageMenus(imlabImageDocument *document)
{
  imImage* image = document->ImageFile->image;
  Ihandle* mnMainMenu = IupGetHandle("mnMainMenu");

  {
    Ihandle* mnBinary = (Ihandle*)IupGetAttribute(mnMainMenu, "mnBinary");
    Ihandle* mnGray = (Ihandle*)IupGetAttribute(mnMainMenu, "mnGray");
    Ihandle* mnMap = (Ihandle*)IupGetAttribute(mnMainMenu, "mnMap");
    Ihandle* mnRGB = (Ihandle*)IupGetAttribute(mnMainMenu, "mnRGB");
    Ihandle* mnCMYK = (Ihandle*)IupGetAttribute(mnMainMenu, "mnCMYK");
    Ihandle* mnYCbCr = (Ihandle*)IupGetAttribute(mnMainMenu, "mnYCbCr");
    Ihandle* mnLab = (Ihandle*)IupGetAttribute(mnMainMenu, "mnLab");
    Ihandle* mnLuv = (Ihandle*)IupGetAttribute(mnMainMenu, "mnLuv");
    Ihandle* mnXYZ = (Ihandle*)IupGetAttribute(mnMainMenu, "mnXYZ");

    IupSetAttribute(mnBinary, "ACTIVE", "YES");
    IupSetAttribute(mnGray, "ACTIVE", "YES");
    IupSetAttribute(mnMap, "ACTIVE", "YES");
    IupSetAttribute(mnRGB, "ACTIVE", "YES");
    IupSetAttribute(mnCMYK, "ACTIVE", "YES");
    IupSetAttribute(mnYCbCr, "ACTIVE", "YES");
    IupSetAttribute(mnLab, "ACTIVE", "YES");
    IupSetAttribute(mnLuv, "ACTIVE", "YES");
    IupSetAttribute(mnXYZ, "ACTIVE", "YES");

    IupSetAttribute(mnBinary, "VALUE", "OFF");
    IupSetAttribute(mnGray, "VALUE", "OFF");
    IupSetAttribute(mnMap, "VALUE", "OFF");
    IupSetAttribute(mnRGB, "VALUE", "OFF");
    IupSetAttribute(mnCMYK, "VALUE", "OFF");
    IupSetAttribute(mnYCbCr, "VALUE", "OFF");
    IupSetAttribute(mnLab, "VALUE", "OFF");
    IupSetAttribute(mnLuv, "VALUE", "OFF");
    IupSetAttribute(mnXYZ, "VALUE", "OFF");

    /* To Map conversion restrictions */
    if (image->data_type != IM_BYTE ||
      (image->color_space != IM_GRAY &&
      image->color_space != IM_RGB &&
      image->color_space != IM_BINARY &&
      image->color_space != IM_MAP))  /* this one is here to avoid
                                      disabling the menu when the image in IM_MAP */
                                      IupSetAttribute(mnMap, "ACTIVE", "NO");

    /* To Binary conversion restrictions */
    if (image->data_type != IM_BYTE)
      IupSetAttribute(mnBinary, "ACTIVE", "NO");

    /* To CMYK conversion restrictions */
    /* In fact there is no CMYK convertion defined in IM */
    /* So just avoid disabling the menu when the image in IM_CMYK */
    if (image->color_space != IM_CMYK)
      IupSetAttribute(mnCMYK, "ACTIVE", "NO");

    /* From Map and Binary conversion restrictions */
    if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    {
      IupSetAttribute(mnYCbCr, "ACTIVE", "NO");
      IupSetAttribute(mnXYZ, "ACTIVE", "NO");
      IupSetAttribute(mnLab, "ACTIVE", "NO");
      IupSetAttribute(mnLuv, "ACTIVE", "NO");
    }

    switch (image->color_space)
    {
    case IM_RGB:
      IupSetAttribute(mnRGB, "VALUE", "ON");
      break;
    case IM_MAP:
      IupSetAttribute(mnMap, "VALUE", "ON");
      break;
    case IM_GRAY:
      IupSetAttribute(mnGray, "VALUE", "ON");
      break;
    case IM_BINARY:
      IupSetAttribute(mnBinary, "VALUE", "ON");
      break;
    case IM_CMYK:
      IupSetAttribute(mnCMYK, "VALUE", "ON");
      break;
    case IM_YCBCR:
      IupSetAttribute(mnYCbCr, "VALUE", "ON");
      break;
    case IM_XYZ:
      IupSetAttribute(mnXYZ, "VALUE", "ON");
      break;
    case IM_LAB:
      IupSetAttribute(mnLab, "VALUE", "ON");
      break;
    case IM_LUV:
      IupSetAttribute(mnLuv, "VALUE", "ON");
      break;
    }
  }

  {
    Ihandle* ByteMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "ByteMenu");
    Ihandle* ShortMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "ShortMenu");
    Ihandle* UShortMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "UShortMenu");
    Ihandle* IntMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "IntMenu");
    Ihandle* FloatMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "FloatMenu");
    Ihandle* CFloatMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "CFloatMenu");
    Ihandle* DoubleMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "DoubleMenu");
    Ihandle* CDoubleMenu = (Ihandle*)IupGetAttribute(mnMainMenu, "CDoubleMenu");

    IupSetAttribute(ByteMenu, "VALUE", "OFF");
    IupSetAttribute(UShortMenu, "VALUE", "OFF");
    IupSetAttribute(IntMenu, "VALUE", "OFF");
    IupSetAttribute(FloatMenu, "VALUE", "OFF");
    IupSetAttribute(CFloatMenu, "VALUE", "OFF");
    IupSetAttribute(DoubleMenu, "VALUE", "OFF");
    IupSetAttribute(CDoubleMenu, "VALUE", "OFF");

    if (image->color_space == IM_MAP || image->color_space == IM_BINARY)
    {
      IupSetAttribute(ByteMenu, "ACTIVE", "NO");
      IupSetAttribute(UShortMenu, "ACTIVE", "NO");
      IupSetAttribute(IntMenu, "ACTIVE", "NO");
      IupSetAttribute(FloatMenu, "ACTIVE", "NO");
      IupSetAttribute(CFloatMenu, "ACTIVE", "NO");
      IupSetAttribute(DoubleMenu, "ACTIVE", "NO");
      IupSetAttribute(CDoubleMenu, "ACTIVE", "NO");
    }
    else
    {
      IupSetAttribute(ByteMenu, "ACTIVE", "YES");
      IupSetAttribute(ShortMenu, "ACTIVE", "YES");
      IupSetAttribute(UShortMenu, "ACTIVE", "YES");
      IupSetAttribute(IntMenu, "ACTIVE", "YES");
      IupSetAttribute(FloatMenu, "ACTIVE", "YES");
      IupSetAttribute(CFloatMenu, "ACTIVE", "YES");
      IupSetAttribute(DoubleMenu, "ACTIVE", "YES");
      IupSetAttribute(CDoubleMenu, "ACTIVE", "YES");

      switch (image->data_type)
      {
      case IM_BYTE:
        IupSetAttribute(ByteMenu, "VALUE", "ON");
        break;
      case IM_SHORT:
        IupSetAttribute(ShortMenu, "VALUE", "ON");
        break;
      case IM_USHORT:
        IupSetAttribute(UShortMenu, "VALUE", "ON");
        break;
      case IM_INT:
        IupSetAttribute(IntMenu, "VALUE", "ON");
        break;
      case IM_FLOAT:
        IupSetAttribute(FloatMenu, "VALUE", "ON");
        break;
      case IM_CFLOAT:
        IupSetAttribute(CFloatMenu, "VALUE", "ON");
        break;
      case IM_DOUBLE:
        IupSetAttribute(DoubleMenu, "VALUE", "ON");
        break;
      case IM_CDOUBLE:
        IupSetAttribute(CDoubleMenu, "VALUE", "ON");
        break;
      }
    }
  }

  {
    Ihandle* mnEditPalette = (Ihandle*)IupGetAttribute(mnMainMenu, "mnEditPalette");
    if (document->ImageFile->image->color_space == IM_MAP || document->ImageFile->image->color_space == IM_GRAY)
      IupSetAttribute(mnEditPalette, "ACTIVE", "YES");
    else
      IupSetAttribute(mnEditPalette, "ACTIVE", "NO");
  }
}

static int cmImageFileInfo(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imlabDlgImageFileInfo(document->FileTitle, document->ImageFile);
  return IUP_DEFAULT;
}

static int cmImageFileAttrib(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (imlabDlgImageFileEditAttrib(document->FileTitle, document->ImageFile))
  {
    document->HasChanged();
    imlabLogMessagef("\"%s\" attributes were changed. Undo is not possible.", document->FileTitle);
  }
  return IUP_DEFAULT;
}

static int cmImageFilePalette(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  int palette_count = document->ImageFile->image->palette_count;
  long* palette = imPaletteNew(256);
  memcpy(palette, document->ImageFile->image->palette, 256*sizeof(long));

  if (!imlabDlgEditPalette(document->FileTitle, palette, &palette_count))
  {
    imPaletteRelease(palette);
    return IUP_DEFAULT;
  }

  imImageSetPalette(document->ImageFile->image, palette, palette_count);

  document->ImageFile->changed = 2;  /* no undo */
  document->HasChanged();
  imlabLogMessagef("\"%s\" palette colors were changed. Undo is not possible.", document->FileTitle);

  document->UpdateViews();

  return IUP_DEFAULT;
}


static int cmImageRename(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  char new_name[512];
  strcpy(new_name, document->FileTitle);

  if (!IupGetParam(document->DlgTitle("Rename"), NULL, NULL, "New Name: %s\n", new_name, NULL))
    return IUP_DEFAULT;

  {
    char* filename = document->ImageFile->filename;
    char* FilePath = utlFileGetPath(filename);
    if (FilePath)
    {
      strcpy(filename, FilePath);
      strcat(filename, new_name);
      free(FilePath);
    }
    else
      strcpy(filename, new_name);
  }

  imlabLogMessagef("\"%s\" renamed to \"%s\". Undo is not possible.", document->FileTitle, new_name);

  document->ImageFile->changed = 2;  /* no undo */
  document->HasChanged();

  return IUP_DEFAULT;
}

static int ChangeColor(Ihandle* self, int color_space)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imImage* image = document->ImageFile->image;
  imImage* NewImage;
  char* alarm_msg = NULL;

  if (image->color_space == color_space)
    return IUP_DEFAULT;
  
  if (image->color_space == IM_GRAY && color_space == IM_BINARY)                    // gray -> binary
    alarm_msg = "This will discard gray information. Proceed?";
  else if (imColorModeDepth(image->color_space) > imColorModeDepth(color_space) &&  // color -> color < color deph
           color_space != IM_GRAY && color_space != IM_BINARY)
    alarm_msg = "This will reduce color information. Proceed?";
  else if (image->color_space != IM_GRAY && image->color_space != IM_BINARY &&      // color -> gray or binary
           (color_space == IM_GRAY || color_space == IM_BINARY))
    alarm_msg = "This will discard color information. Proceed?";

  if (alarm_msg)
  {
    int ret = imlabDlgQuestion(alarm_msg, 0);
    if (ret != 1) 
      return IUP_DEFAULT;
  }

  /* creates the new image */
  NewImage = imImageCreateBased(image, -1, -1, color_space, -1);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  if (imProcessConvertColorSpace(image, NewImage) != IM_ERR_NONE)
    imImageDestroy(NewImage);
  else
    document->ChangeImage(NewImage, "ConvertColorSpace{color_space=\"%s\"}", imColorModeSpaceName(color_space));
  return IUP_DEFAULT;
}

static int cmConvert2Binary(Ihandle* self)
{
  return ChangeColor(self, IM_BINARY);
}

static int cmConvert2Gray(Ihandle* self)
{
  return ChangeColor(self, IM_GRAY);
}

static int cmConvert2Map(Ihandle* self)
{
  return ChangeColor(self, IM_MAP);
}

static int cmConvert2RGB(Ihandle* self)
{
  return ChangeColor(self, IM_RGB);
}

static int cmConvert2CMYK(Ihandle* self)
{
  return ChangeColor(self, IM_CMYK);
}

static int cmConvert2YCbCr(Ihandle* self)
{
  return ChangeColor(self, IM_YCBCR);
}

static int cmConvert2Lab(Ihandle* self)
{
  return ChangeColor(self, IM_LAB);
}

static int cmConvert2Luv(Ihandle* self)
{
  return ChangeColor(self, IM_LUV);
}

static int cmConvert2XYZ(Ihandle* self)
{
  return ChangeColor(self, IM_XYZ);
}

static int ChangePrecision(Ihandle* self, int data_type)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imImage* image = document->ImageFile->image;
  imImage* NewImage;
  int cpx2real = document->BitmapView.cpx2real;
  double gamma = document->BitmapView.gamma;
  int absolute = document->BitmapView.absolute;
  int cast_mode = document->BitmapView.cast_mode;

  if (image->data_type == data_type)
    return IUP_DEFAULT;

  if (imImageIsComplex(image))
  {
    if (!IupGetParam(document->DlgTitle("Convert Data Type"), NULL, NULL,
      "Type Conversion: %l|Scan MinMax|Fixed MinMax|Direct|\n"
      "Absolute: %b\n"
      "Real Gamma: %R\n"
      "Complex to Real: %l|Real|Imaginary|Magnitude|Phase|\n",
      &cast_mode,
      &absolute,
      &gamma,
      &cpx2real,
      NULL))
      return IUP_DEFAULT;
  }
  else if (imImageIsReal(image))
  {
    if (!IupGetParam(document->DlgTitle("Convert Data Type"), NULL, NULL,
      "Type Conversion: %l|Scan MinMax|Fixed MinMax|Direct|\n"
      "Absolute: %b\n"
      "Real Gamma: %R\n",
      &cast_mode,
      &absolute,
      &gamma,
      NULL))
      return IUP_DEFAULT;
  }
  else if (imImageIsSigned(image))
  {
    if (!IupGetParam(document->DlgTitle("Convert Data Type"), NULL, NULL,
      "Type Conversion: %l|Scan MinMax|Fixed MinMax|Direct|\n"
      "Absolute: %b\n",
      &cast_mode,
      &absolute,
      NULL))
      return IUP_DEFAULT;
  }
  else
  {
    if (!IupGetParam(document->DlgTitle("Convert Data Type"), NULL, NULL,
      "Type Conversion: %l|Scan MinMax|Fixed MinMax|Direct|\n",
      &cast_mode,
      NULL))
      return IUP_DEFAULT;
  }

  if (image->data_type > data_type)
  {
    int ret = imlabDlgQuestion("This will reduce numeric precision. Proceed?", 0);
    if (ret != 1)
      return IUP_DEFAULT;
  }

  /* creates the new image */
  NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (NewImage == NULL)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  /* Process the image */
  if (imProcessConvertDataType(image, NewImage, cpx2real, gamma, absolute, cast_mode) != IM_ERR_NONE)
    imImageDestroy(NewImage);
  else
  {
    const char* cpx2real_str[] = { "Real", "Imaginary", "Magnitude", "Phase" };
    const char* cast_mode_str[] = { "Scan MinMax", "Fixed MinMax", "Direct" };
    document->ChangeImage(NewImage, "ConvertDataType{datatype=\"%s\", cpx2real=\"%s\", gamma=%g, absolute=%d, cast_mode=\"%s\"}", imDataTypeName(data_type), cpx2real_str[cpx2real], gamma, absolute, cast_mode_str[cast_mode]);
  }
  return IUP_DEFAULT;
}

static int cmConvert2Byte(Ihandle* self)
{
  return ChangePrecision(self, IM_BYTE);
}

static int cmConvert2UShort(Ihandle* self)
{
  return ChangePrecision(self, IM_USHORT);
}

static int cmConvert2Short(Ihandle* self)
{
  return ChangePrecision(self, IM_SHORT);
}

static int cmConvert2Int(Ihandle* self)
{
  return ChangePrecision(self, IM_INT);
}

static int cmConvert2Float(Ihandle* self)
{
  return ChangePrecision(self, IM_FLOAT);
}

static int cmConvert2CFloat(Ihandle* self)
{
  return ChangePrecision(self, IM_CFLOAT);
}

static int cmConvert2Double(Ihandle* self)
{
  return ChangePrecision(self, IM_DOUBLE);
}

static int cmConvert2CDouble(Ihandle* self)
{
  return ChangePrecision(self, IM_CDOUBLE);
}

static int ChangePal(Ihandle* self, long* (*NewPalFunc)(void))
{
  // this is called only for GRAY images, 
  // it is OK because palettes are not saved for GRAY images,
  // so we alter only the view
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  imImageSetPalette(document->ImageFile->image, NewPalFunc(), 256);
  imImageSetPalette(document->BitmapView.image, NewPalFunc(), 256);
  document->RefreshViews();
  return IUP_DEFAULT;
}

static int cmPalGray(Ihandle* self)
{
  return ChangePal(self, imPaletteGray);
}

static int cmPalRed(Ihandle* self)
{
  return ChangePal(self, imPaletteRed);
}

static int cmPalGreen(Ihandle* self)
{
  return ChangePal(self, imPaletteGreen);
}

static int cmPalBlue(Ihandle* self)
{
  return ChangePal(self, imPaletteBlue);
}

static int cmPalYellow(Ihandle* self)
{
  return ChangePal(self, imPaletteYellow);
}

static int cmPalMagenta(Ihandle* self)
{
  return ChangePal(self, imPaletteMagenta);
}

static int cmPalCyan(Ihandle* self)
{
  return ChangePal(self, imPaletteCyan);
}

static int cmPalRainbow(Ihandle* self)
{
  return ChangePal(self, imPaletteRainbow);
}

static int cmPalHues(Ihandle* self)
{
  return ChangePal(self, imPaletteHues);
}

static int cmPalBlueIce(Ihandle* self)
{
  return ChangePal(self, imPaletteBlueIce);
}

static int cmPalHotIron(Ihandle* self)
{
  return ChangePal(self, imPaletteHotIron);
}

static int cmPalBlackBody(Ihandle* self)
{
  return ChangePal(self, imPaletteBlackBody);
}

static int cmPalHighContrast(Ihandle* self)
{
  return ChangePal(self, imPaletteHighContrast);
}

static int cmPalLinear(Ihandle* self)
{
  return ChangePal(self, imPaletteLinear);
}

static int cmPalUniform(Ihandle* self)
{
  return ChangePal(self, imPaletteUniform);
}

static int cmImageMenuOpen(Ihandle* self)
{
  imlabImageDocument* document = imlabGetCurrentDocument(self);
  if (document)
  {
    IupSetAttribute(self, "ACTIVE", "Yes");
    UpdateImageMenus(document);
  }
  else
    IupSetAttribute(self, "ACTIVE", "NO");
  return IUP_DEFAULT;
}

void imlabMainWindowRegisterImageMenu(void)
{
  IupSetFunction("imlabBinary", (Icallback)cmConvert2Binary);
  IupSetFunction("imlabGray", (Icallback) cmConvert2Gray);
  IupSetFunction("imlabMap", (Icallback) cmConvert2Map);
  IupSetFunction("imlabRGB", (Icallback) cmConvert2RGB);
  IupSetFunction("imlabCMYK", (Icallback) cmConvert2CMYK);
  IupSetFunction("imlabYCbCr", (Icallback) cmConvert2YCbCr);
  IupSetFunction("imlabLab", (Icallback) cmConvert2Lab);
  IupSetFunction("imlabLuv", (Icallback) cmConvert2Luv);
  IupSetFunction("imlabXYZ", (Icallback) cmConvert2XYZ);

  IupSetFunction("imlabByte", (Icallback) cmConvert2Byte);
  IupSetFunction("imlabShort", (Icallback) cmConvert2Short);
  IupSetFunction("imlabUShort", (Icallback) cmConvert2UShort);
  IupSetFunction("imlabInt", (Icallback) cmConvert2Int);
  IupSetFunction("imlabFloat", (Icallback) cmConvert2Float);
  IupSetFunction("imlabCFloat", (Icallback) cmConvert2CFloat);
  IupSetFunction("imlabDouble", (Icallback)cmConvert2Double);
  IupSetFunction("imlabCDouble", (Icallback)cmConvert2CDouble);

  IupSetFunction("imlabPalGray", (Icallback) cmPalGray);
  IupSetFunction("imlabPalRed", (Icallback) cmPalRed);
  IupSetFunction("imlabPalGreen", (Icallback) cmPalGreen);
  IupSetFunction("imlabPalBlue", (Icallback) cmPalBlue);
  IupSetFunction("imlabPalYellow", (Icallback) cmPalYellow);
  IupSetFunction("imlabPalMagenta", (Icallback) cmPalMagenta);
  IupSetFunction("imlabPalCyan", (Icallback) cmPalCyan);
  IupSetFunction("imlabPalRainbow", (Icallback) cmPalRainbow);
  IupSetFunction("imlabPalHues", (Icallback) cmPalHues);
  IupSetFunction("imlabPalBlueIce", (Icallback) cmPalBlueIce);
  IupSetFunction("imlabPalHotIron", (Icallback) cmPalHotIron);
  IupSetFunction("imlabPalBlackBody", (Icallback) cmPalBlackBody);
  IupSetFunction("imlabPalHighContrast", (Icallback) cmPalHighContrast);
  IupSetFunction("imlabPalUniform", (Icallback)cmPalUniform);
  IupSetFunction("imlabPalLinear", (Icallback)cmPalLinear);

  IupSetFunction("imlabImageFileInfo", (Icallback) cmImageFileInfo);
  IupSetFunction("imlabImageFileAttrib", (Icallback) cmImageFileAttrib);
  IupSetFunction("imlabImageFilePalette", (Icallback) cmImageFilePalette);
  IupSetFunction("imlabImageRename", (Icallback) cmImageRename);

  IupSetFunction("imlabImageMenuOpen", (Icallback) cmImageMenuOpen);
}

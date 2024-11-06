#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_convert.h>
#include <im_colorhsi.h>
#include <im_palette.h>
#include <im_math.h>

#include <cd.h>

#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <string.h>


typedef void(*unary_func_rgb)(imbyte src_r, imbyte src_g, imbyte src_b, imbyte *dst_r, imbyte *dst_g, imbyte *dst_b);

static int pseudo_color(Ihandle *parent)
{
  imImage *NewImage, *image;
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  NewImage = imImageCreateBased(image, -1, -1, IM_RGB, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessPseudoColor(image, NewImage);

  imlabImageDocumentCreateFromImage(NewImage, "PseudoColor of %s", "PseudoColor{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int split_chroma(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage *NewImage1 = imImageCreateBased(image, -1, -1, IM_GRAY, IM_BYTE);
  if (!NewImage1)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImage *NewImage2 = imImageCreateBased(image, -1, -1, IM_RGB, IM_BYTE);
  if (!NewImage2)
  {
    imImageDestroy(NewImage1);
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessSplitYChroma(image, NewImage1, NewImage2);

  imlabImageDocumentCreateFromImage(NewImage1, "Intensity of %s", "Intensity{image=\"%s\"}", document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImage2, "Chroma of %s", "Chroma{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int split_hsi(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int data_type = IM_FLOAT;
  if (image->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0)==1)
      data_type = IM_DOUBLE;
  }
  else if (image->data_type == IM_DOUBLE)
    data_type = IM_DOUBLE;

  imImage *NewImage1 = imImageCreateBased(image, -1, -1, IM_GRAY, data_type);
  if (!NewImage1)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImage *NewImage2 = imImageCreateBased(image, -1, -1, IM_GRAY, data_type);
  if (!NewImage2)
  {
    imImageDestroy(NewImage1);
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImage *NewImage3 = imImageCreateBased(image, -1, -1, IM_GRAY, data_type);
  if (!NewImage3)
  {
    imImageDestroy(NewImage1);
    imImageDestroy(NewImage2);
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  if (image->data_type != data_type && image->data_type != IM_BYTE)
  {
    imImage* Normalized = imImageCreate(image->width, image->height, IM_RGB, data_type);
    imProcessConvertDataType(image, Normalized, 0, 0, 0, 0);
    imProcessToneGamut(Normalized, Normalized, IM_GAMUT_NORMALIZE, NULL);
    imProcessSplitHSI(Normalized, NewImage1, NewImage2, NewImage3);
    imImageDestroy(Normalized);
  }
  else
    imProcessSplitHSI(image, NewImage1, NewImage2, NewImage3);

  imImageSetPalette(NewImage1, imPaletteHues(), 256);

  imlabImageDocumentCreateFromImage(NewImage1, "Hue of %s", "Hue{image=\"%s\"}", document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImage2, "Saturation of %s", "Saturation{image=\"%s\"}", document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImage3, "Intensity of %s", "Intensity{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int merge_hsi(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage* ref_image = document->ImageFile->image;
  
  int comp1_index,comp2_index,comp3_index;
  char format[4096] = "Hue: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp1_index))
    return IUP_DEFAULT;
  strcat(format, "Saturation: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp2_index))
    return IUP_DEFAULT;
  strcat(format, "Intensity: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp3_index))
    return IUP_DEFAULT;

  if (!IupGetParam("Merge HSI", NULL, NULL, format,
                   &comp1_index, &comp2_index, &comp3_index, NULL))
    return IUP_DEFAULT;

  imlabImageDocument * document1 = imlabImageDocumentListGetMatch(comp1_index, NULL, ref_image);
  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(comp2_index, NULL, ref_image);
  imlabImageDocument * document3 = imlabImageDocumentListGetMatch(comp3_index, NULL, ref_image);

  imImage *image1 = document1->ImageFile->image;
  imImage *image2 = document2->ImageFile->image;
  imImage *image3 = document3->ImageFile->image;

  // src is always float or double
  // tgt is float or double acoord., but can be byte

  int data_type = IM_FLOAT;
  if (imlabDlgQuestion("The result image data type default is floating point.\nWould you like to convert to byte?", 0) == 1)
    data_type = IM_BYTE;
  else if (image1->data_type == IM_DOUBLE)
    data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, IM_RGB, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessMergeHSI(image1, image2, image3, NewImage);

  imlabImageDocumentCreateFromImage(NewImage, "MergeHSI", "MergeHSI{imageH=\"%s\", imageI=\"%s\", imageI=\"%s\"}", document1->FileTitle, document2->FileTitle, document3->FileTitle);

  return IUP_DEFAULT;
}

static int split_comp(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  int has_comp4 = (imColorModeDepth(image->color_space) == 4)? 1: 0;
  imImage *NewImageArray[4];

  if (!has_comp4 && image->has_alpha)
    has_comp4 = 1;

  NewImageArray[0] = imImageCreate(image->width, image->height, IM_GRAY, image->data_type);
  NewImageArray[1] = imImageCreate(image->width, image->height, IM_GRAY, image->data_type);
  NewImageArray[2] = imImageCreate(image->width, image->height, IM_GRAY, image->data_type);
  NewImageArray[3] = has_comp4? imImageCreate(image->width, image->height, IM_GRAY, image->data_type): NULL;
  if (!NewImageArray[0] || !NewImageArray[1] || !NewImageArray[2] || (has_comp4 && !NewImageArray[3]))
  {
    if (NewImageArray[0]) imImageDestroy(NewImageArray[0]);
    if (NewImageArray[1]) imImageDestroy(NewImageArray[1]);
    if (NewImageArray[2]) imImageDestroy(NewImageArray[2]);
    if (NewImageArray[3]) imImageDestroy(NewImageArray[3]);
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImageArray[0]);
  imImageCopyAttributes(image, NewImageArray[1]);
  imImageCopyAttributes(image, NewImageArray[2]);
  if (has_comp4) imImageCopyAttributes(image, NewImageArray[3]);

  imProcessSplitComponents(image, NewImageArray);

  imlabImageDocumentCreateFromImage(NewImageArray[0], "%s component of %s", "SplitComponents{comp=\"%s\", image=\"%s\"}", imColorModeComponentName(image->color_space, 0), document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImageArray[1], "%s component of %s", "SplitComponents{comp=\"%s\", image=\"%s\"}", imColorModeComponentName(image->color_space, 1), document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImageArray[2], "%s component of %s", "SplitComponents{comp=\"%s\", image=\"%s\"}", imColorModeComponentName(image->color_space, 2), document->FileTitle);
  if (has_comp4) 
  {
    if (image->has_alpha)
      imlabImageDocumentCreateFromImage(NewImageArray[3], "%s component of %s", "SplitComponents{comp=\"%s\", image=\"%s\"}", "Alpha", document->FileTitle);
    else
      imlabImageDocumentCreateFromImage(NewImageArray[3], "%s component of %s", "SplitComponents{comp=\"%s\", image=\"%s\"}", imColorModeComponentName(image->color_space, 3), document->FileTitle);
  }

  return IUP_DEFAULT;
}

static int check_comp(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;
  if (param_index == -2 || param_index == 0 || param_index == 1)
  {
    int color_space = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    if (color_space != IM_RGB)
      color_space += 3;

    int has_alpha = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    if (has_alpha)
      color_space |= IM_ALPHA;

    int has_comp4 = (imColorModeDepth(color_space) == 4)? 1: 0;
    Ihandle* ctrl4 = (Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM5"), "CONTROL");
    if (has_comp4)
      IupSetAttribute(ctrl4, "ACTIVE", "YES");
    else
      IupSetAttribute(ctrl4, "ACTIVE", "NO");

    Ihandle* lbl1 = (Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "LABEL");
    IupSetAttribute(lbl1, "TITLE", (char*)imColorModeComponentName(color_space, 0));
    Ihandle* lbl2 = (Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM3"), "LABEL");
    IupSetAttribute(lbl2, "TITLE", (char*)imColorModeComponentName(color_space, 1));
    Ihandle* lbl3 = (Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM4"), "LABEL");
    IupSetAttribute(lbl3, "TITLE", (char*)imColorModeComponentName(color_space, 2));
    Ihandle* lbl4 = (Ihandle*)IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM5"), "LABEL");
    if (has_comp4)
    {
      if (has_alpha)
        IupSetAttribute(lbl4, "TITLE", "Alpha");
      else
        IupSetAttribute(lbl4, "TITLE", (char*)imColorModeComponentName(color_space, 3));
    }
    else
      IupSetAttribute(lbl4, "TITLE", "----");
  }
  return 1;
}

static int merge_comp(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage* ref_image = document->ImageFile->image;
  
  static int color_space = 0;
  static int has_alpha = 0;
  int comp1_index,comp2_index,comp3_index,comp4_index;
  char format[4096] = "Color Space: %l|rgb|cmyk|ycbcr|lab|luv|xyz|\n"
                      "Has Alpha: %b\n"
                      "Component 1: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp1_index))
    return IUP_DEFAULT;
  strcat(format, "Component 2: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp2_index))
    return IUP_DEFAULT;
  strcat(format, "Component 3: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp3_index))
    return IUP_DEFAULT;
  strcat(format, "Component 4: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, ref_image, &comp4_index))
    return IUP_DEFAULT;

  if (!IupGetParam("Merge Components", check_comp, NULL, format,
                   &color_space, &has_alpha, &comp1_index, &comp2_index, &comp3_index, &comp4_index, NULL))
    return IUP_DEFAULT;

  if (color_space != IM_RGB)
    color_space += 3;         // jump  map,gray,binary  to adjust value

  if (has_alpha)
    color_space |= IM_ALPHA;

  int has_comp4 = (imColorModeDepth(color_space) == 4)? 1: 0;

  imlabImageDocument * document1 = imlabImageDocumentListGetMatch(comp1_index, NULL, ref_image);
  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(comp2_index, NULL, ref_image);
  imlabImageDocument * document3 = imlabImageDocumentListGetMatch(comp3_index, NULL, ref_image);
  imlabImageDocument * document4 = (has_comp4)? imlabImageDocumentListGetMatch(comp4_index, NULL, ref_image): NULL;

  imImage *image[4];
  image[0] = document1->ImageFile->image;
  image[1] = document2->ImageFile->image;
  image[2] = document3->ImageFile->image;
  image[3] = (has_comp4)? document4->ImageFile->image: NULL;

  imImage *NewImage = imImageCreate(ref_image->width, ref_image->height, imColorModeSpace(color_space), ref_image->data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(ref_image, NewImage);

  if (has_alpha)
    imImageAddAlpha(NewImage);

  imProcessMergeComponents((const imImage**)image, NewImage);

  if (has_comp4)
    imlabImageDocumentCreateFromImage(NewImage, "MergeComponents", "MergeComponents{image0=\"%s\", image1=\"%s\", image2=\"%s\", image3=\"%s\"}", document1->FileTitle, document2->FileTitle, document3->FileTitle, document4->FileTitle);
  else
    imlabImageDocumentCreateFromImage(NewImage, "MergeComponents", "MergeComponents{image0=\"%s\", image1=\"%s\", image2=\"%s\"}", document1->FileTitle, document2->FileTitle, document3->FileTitle);

  // save only the color space in the static variable
  color_space = imColorModeSpace(color_space);

  return IUP_DEFAULT;
}

static int iMatchAlphaImage(const imImage* image, const imImage* match_image)
{
  /* this will match width, height and data type */
  if (!imImageMatchDataType(image, match_image))
    return 0;

  if (!imImageIsGray(image))
    return 0;

  return 1;
}

static int merge_alpha(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  int alpha_index;
  char format[4096] = "Alpha Image: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), iMatchAlphaImage, image, &alpha_index))
    return IUP_DEFAULT;

  imlabImageDocument *alpha_document = imlabImageDocumentListGetMatch(alpha_index, iMatchAlphaImage, image);
  imImage* alpha_image = alpha_document->ImageFile->image;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImageAddAlpha(NewImage);

  imImageCopyPlane(alpha_image, 0, NewImage, NewImage->depth);

  document->ChangeImage(NewImage, "MergeAlpha{alpha_image=\"%s\"}", alpha_document->FileTitle);

  return IUP_DEFAULT;
}

static int remove_alpha(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  // Can NOT use imImageClone or ImImageCreateBased to avoid alpha duplication
  imImage* NewImage = imImageCreate(image->width, image->height, image->color_space, image->data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  document->ChangeImage(NewImage, "RemoveAlpha{}");

  return IUP_DEFAULT;
}

static int set_alpha(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  static double alpha = 0;

  if (!IupGetParam(document->DlgTitle("Set Alpha"), NULL, NULL, "Value: %R\n", &alpha, NULL))
    return 0;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImageSetAlpha(NewImage, alpha);

  document->ChangeImage(NewImage, "SetAlpha{alpha=%g}", alpha);

  return IUP_DEFAULT;
}

static int set_alphacolor(Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  static double src_color[4] = { 0, 0, 0, 0 };
  if (!imlabDlgGetConstant(document->FileTitle, "Source Color", image->color_space, src_color))
    return IUP_DEFAULT;

  static double alpha = 0;
  if (!IupGetParam(document->DlgTitle("Target Alpha"), NULL, NULL, "Value: %R\n", &alpha, NULL))
    return 0;

  imImage *NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessSetAlphaColor(image, NewImage, src_color, alpha);

  document->ChangeImage(NewImage, "SetAlphaColor{color={%g, %g, %g, %g}, alpha=%g}", src_color[0], src_color[1], src_color[2], src_color[3], alpha);

  return IUP_DEFAULT;
}

static int selecthue_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  double hue_start = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
  double hue_end = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

  if (hue_start > hue_end)
    return 0;
  if (hue_start > 360 || hue_start < -360)
    return 0;
  if (hue_end > 360 || hue_end < -360)
    return 0;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");

    imProcessSelectHue(image, NewImage, hue_start, hue_end);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int select_hue(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document, image, NewImage);

  static double hue_start = 0;
  static double hue_end = 0;
  int show_preview = 0;
  if (!IupGetParam(document->DlgTitle("Select Hue"), selecthue_preview, preview,
    "Hue Start (degrees): %A\n"
    "Hue End (degrees): %A\n"
    "Preview: %b\n",
    &hue_start, &hue_end, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  if (!show_preview)
    imProcessSelectHue(image, NewImage, hue_start, hue_end);

  document->ChangeImage(NewImage, "SelectHue{start=%g, end=%g}", hue_start, hue_end);

  return IUP_DEFAULT;
}

static int fix_bgr(Ihandle *parent)
{
  imImage *NewImage, *image; 
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessFixBGR(image, NewImage);

  document->ChangeImage(NewImage, "FixBGR{}");

  return IUP_DEFAULT;
}

static int fRound(double x)
{
  return x > 0 ? (int) (x + 0.5) : (int) (x - 0.5);
}

static void DoGamutHS(imImage* src_image, imImage* dst_image)
{
  int src_size = src_image->width * src_image->height;
  imbyte *src_red=(imbyte*)src_image->data[0],*src_green=(imbyte*)src_image->data[1],*src_blue=(imbyte*)src_image->data[2];
  imbyte *dst_red=(imbyte*)dst_image->data[0],*dst_green=(imbyte*)dst_image->data[1],*dst_blue=(imbyte*)dst_image->data[2];
  double H, S, I;
  int offset, x, y, 
      dst_width = dst_image->width,
      dst_height = dst_image->height;

  for (int i = 0; i < src_size; i++)
  {
    imColorRGB2HSIbyte(src_red[i], src_green[i], src_blue[i], &H, &S, &I);

    /* offset = y * width + x */
    /* but S is in 0-1 interval and H is in 0-360 */
    x = fRound(H);
    y = fRound(S*(dst_height-1));

    offset = y * dst_width + x;
    dst_red[offset] = src_red[i];
    dst_green[offset] = src_green[i];
    dst_blue[offset] = src_blue[i];
  }
}

static int plot_gamut_hs(Ihandle *parent)
{
  imImage *NewImage, *image; 
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  NewImage = imImageCreate(361, 444, IM_RGB, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  DoGamutHS(image, NewImage);

  imlabImageDocumentCreateFromImage(NewImage, "PlotGamutHS of %s", "PlotGamutHS{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static void DoGamutHSPolar(imImage* src_image, imImage* dst_image)
{
  int src_size = src_image->width * src_image->height;
  imbyte *src_red=(imbyte*)src_image->data[0],*src_green=(imbyte*)src_image->data[1],*src_blue=(imbyte*)src_image->data[2];
  imbyte *dst_red=(imbyte*)dst_image->data[0],*dst_green=(imbyte*)dst_image->data[1],*dst_blue=(imbyte*)dst_image->data[2];
  double H, S, I;
  int offset, x, y, 
      dst_size = dst_image->width; /* it is a square image */
  double half_size = dst_size/2.0;

  for (int i = 0; i < src_size; i++)
  {
    imColorRGB2HSIbyte(src_red[i], src_green[i], src_blue[i], &H, &S, &I);

    /* offset = y * width + x */
    /* but S is in 0-1 interval and H is in 0-360 */
    x = fRound((1.0 + S*cos(H*CD_DEG2RAD))*half_size);
    y = fRound((1.0 + S*sin(H*CD_DEG2RAD))*half_size);
    x = x<0? 0: (x>dst_size-1? dst_size-1: x);
    y = y<0? 0: (y>dst_size-1? dst_size-1: y);

    offset = y * dst_size + x;
    dst_red[offset] = src_red[i];
    dst_green[offset] = src_green[i];
    dst_blue[offset] = src_blue[i];
  }
}

static int plot_gamut_hs_polar(Ihandle *parent)
{
  imImage *NewImage, *image; 
  imlabImageDocument* document;

  document = imlabGetCurrentDocument(parent);
  image = document->ImageFile->image;

  NewImage = imImageCreate(512, 512, IM_RGB, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  DoGamutHSPolar(image, NewImage);

  imlabImageDocumentCreateFromImage(NewImage, "PlotGamutPolarHS of %s", "PlotGamutPolarHS{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static void DoGamutRB(imImage* src_image, imImage* dst_image)
{
  int src_size = src_image->width * src_image->height;
  imbyte *src_red=(imbyte*)src_image->data[0],*src_green=(imbyte*)src_image->data[1],*src_blue=(imbyte*)src_image->data[2];
  imbyte *dst_red=(imbyte*)dst_image->data[0],*dst_green=(imbyte*)dst_image->data[1],*dst_blue=(imbyte*)dst_image->data[2];
  imbyte r,g,b,y;
  int i, offset, ri, bi;

  for (i = 0; i < src_size; i++)
  {
    r = src_red[i];
    g = src_green[i];
    b = src_blue[i];

    y = (imbyte)((299*r + 587*g + 114*b) / 1000);
    ri = r - y + 256;
    bi = b - y + 256;

    offset = bi * 512 + ri;
    dst_red[offset] = src_red[i];
    dst_green[offset] = src_green[i];
    dst_blue[offset] = src_blue[i];
  }
}

static int plot_gamut_rb(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageCreate(512, 512, IM_RGB, IM_BYTE);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  DoGamutRB(image, NewImage);

  imlabImageDocumentCreateFromImage(NewImage, "PlotGamutYrbChroma of %s", "PlotGamutYrbChroma{image=\"%s\"}", document->FileTitle);

  return IUP_DEFAULT;
}

static int norm_comp(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  int data_type = IM_FLOAT;
  if (image->data_type < IM_FLOAT)
  {
    if (imlabDlgQuestion("The result image can be float or double.\nThe default is float. Would you like to use double?", 0) == 1)
      data_type = IM_DOUBLE;
  }
  else if (image->data_type == IM_DOUBLE)
    data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }
  imImageCopyAttributes(image, NewImage);

  imProcessNormalizeComponents(image, NewImage);

  document->ChangeImage(NewImage, "NormalizeComponents{}");

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Split Components", "split_comp", (Icallback) split_comp, 1),
    imlabProcNewItem(mnProcess, "Merge Components...", "merge_comp", (Icallback) merge_comp, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Remove Alpha", "remove_alpha", (Icallback) remove_alpha, 1),
    imlabProcNewItem(mnProcess, "Merge Alpha...", "merge_alpha", (Icallback) merge_alpha, 1),
    imlabProcNewItem(mnProcess, "Set Alpha...", "set_alpha", (Icallback)set_alpha, 1),
    imlabProcNewItem(mnProcess, "Set Alpha Color...", "set_alphacolor", (Icallback)set_alphacolor, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Split HSI", "split_hsi", (Icallback) split_hsi, 1),
    imlabProcNewItem(mnProcess, "Merge HSI...", "merge_hsi", (Icallback) merge_hsi, 1),
    imlabProcNewItem(mnProcess, "Plot HS Gamut", "plot_gamut_hs", (Icallback) plot_gamut_hs, 1),
    imlabProcNewItem(mnProcess, "Plot HS Gamut (Polar)", "plot_gamut_hs_polar", (Icallback) plot_gamut_hs_polar, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Split Yrb Chroma/Intensity", "split_chroma", (Icallback) split_chroma, 1),
    imlabProcNewItem(mnProcess, "Plot Yrb Ghroma Gamut", "plot_gamut_rb", (Icallback) plot_gamut_rb, 1),
    imlabProcNewItem(mnProcess, "Select Hue...", "select_hue", (Icallback) select_hue, 1),
    IupSeparator(),
    
    imlabProcNewItem(mnProcess, "Pseudo Color", "pseudo_color", (Icallback)pseudo_color, 1),
    imlabProcNewItem(mnProcess, "Normalize Components", "norm_comp", (Icallback)norm_comp, 1),
    imlabProcNewItem(mnProcess, "Fix BGR->RGB", "fix_bgr", (Icallback) fix_bgr, 1),
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "set_alphacolor"), "imlabStatusHelp", "When color is match alpha is set to target value.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "split_hsi"), "imlabStatusHelp", "Split as HSI components. Must be of Real data type, Normalized to 0-1.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "gray_scale_luma"), "imlabStatusHelp", "Convert to gray scale using 0.299R+0.587G+0.114B.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "gray_scale_linear"), "imlabStatusHelp", "Convert to gray scale using (R+G+B)/3.");
  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "select_hue"), "imlabStatusHelp", "Uses a hue interval to isolate where color predominates.");

  IupAppend(mnProcess, imlabSubmenu("Color Components", submenu));
}

static int iHasAlpha(const imImage* image)
{
  return image->has_alpha;
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "split_hsi", imImageIsRGBNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "merge_hsi", imImageIsRealGray);
  imlabProcPlugInUpdateItem(mnProcess, "remove_alpha", iHasAlpha);
  imlabProcPlugInUpdateItem(mnProcess, "set_alpha", iHasAlpha);      
  imlabProcPlugInUpdateItem(mnProcess, "set_alphacolor", iHasAlpha);
  imlabProcPlugInUpdateItem(mnProcess, "split_comp", imImageIsColor);
  imlabProcPlugInUpdateItem(mnProcess, "norm_comp", imImageIsColor);
  imlabProcPlugInUpdateItem(mnProcess, "merge_comp", imImageIsGray);
  imlabProcPlugInUpdateItem(mnProcess, "split_chroma", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "plot_gamut_rb", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "plot_gamut_hs", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "plot_gamut_hs_polar", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "select_hue", imImageIsRGB); 
  imlabProcPlugInUpdateItem(mnProcess, "fix_bgr", imImageIsByteRGB);
  imlabProcPlugInUpdateItem(mnProcess, "pseudo_color", imImageIsGray);
}

static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinColor = &plugin;

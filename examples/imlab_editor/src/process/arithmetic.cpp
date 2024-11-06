#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "counter.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_complex.h>
#include <im_math_op.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


static char* GetBinaryOpStr(int op)
{
  switch(op)
  {
  case IM_BIN_ADD:
    return "a + b";
  case IM_BIN_SUB:
    return "a - b";
  case IM_BIN_MUL:
    return "a * b";
  case IM_BIN_DIV:
    return "a div b";
  case IM_BIN_DIFF:
    return "abs(a - b)";
  case IM_BIN_MIN:
    return "Minimum";
  case IM_BIN_MAX:
    return "Maximum";
  case IM_BIN_POW:
    return "a^b";
  }

  return NULL;
}

static int iMatchComplexOp(const imImage* image, const imImage* match_image)
{
  /* this will match width, height and color space */
  if (!imImageMatchColorSpace(image, match_image))
    return 0;

  if (!imImageIsRealComplex(image))
    return 0;

  return 1;
}

static int bin_op(int op, Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  int second_index;
  char format[4096] = "Second Image (b): %l";
  if (image1->data_type == IM_CFLOAT || image1->data_type == IM_CDOUBLE)
  {
    if (!imlabImageDocumentListInitFormat(format+strlen(format), iMatchComplexOp, image1, &second_index))
      return IUP_DEFAULT;
  }
  else
  {
    if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, image1, &second_index))
      return IUP_DEFAULT;
  }
  strcat(format, "Operation: %l|a+b|a-b|a*b|a/b|abs(a-b)|a^b|min(a,b)|max(a,b)|\n");
  strcat(format, "Best Data Type: %b\n");

  static int best_type = 1;
  if (!IupGetParam(document1->DlgTitle("Arithmetic (a)"), NULL, NULL, format,
                   &second_index, &op, &best_type, NULL))
    return IUP_DEFAULT;

  imlabImageDocument * document2;
  if (image1->data_type == IM_CFLOAT || image1->data_type == IM_CDOUBLE)
    document2 = imlabImageDocumentListGetMatch(second_index, iMatchComplexOp, image1);
  else
    document2 = imlabImageDocumentListGetMatch(second_index, NULL, image1);
  imImage *image2 = document2->ImageFile->image;

  int data_type = image1->data_type;
  if (best_type)
  {
    if ((op == IM_BIN_POW || op == IM_BIN_DIV) && image1->data_type != IM_CFLOAT && image1->data_type != IM_CDOUBLE)
    {
      data_type = IM_FLOAT;
      if (image1->data_type == IM_DOUBLE) data_type = IM_DOUBLE;
    }
    else if ((op == IM_BIN_ADD || op == IM_BIN_MUL) && image1->data_type == IM_BYTE)
      data_type = IM_USHORT;
    else if ((op == IM_BIN_ADD || op == IM_BIN_SUB || op == IM_BIN_MUL) && (image1->data_type == IM_BYTE || image1->data_type == IM_USHORT))
      data_type = IM_INT;
  }

  imImage *NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessArithmeticOp(image1, image2, NewImage, op);

  imlabLogMessagef("%s Time = [%s]", GetBinaryOpStr(op), ctTimerCount());

  document1->ChangeImage(NewImage, "ArithmeticOp{image=\"%s\", op=\"%s\"}", document2->FileTitle, GetBinaryOpStr(op));

  return IUP_DEFAULT;
}
                  
static int pow_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_POW, parent);
}

static int min_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_MIN, parent);
}

static int max_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_MAX, parent);
}

static int add_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_ADD, parent);
}

static int sub_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_SUB, parent);
}

static int mul_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_MUL, parent);
}

static int div_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_DIV, parent);
}

static int diff_binop(Ihandle *parent)
{
  return bin_op(IM_BIN_DIFF, parent);
}

static int constbin_op(int op, Ihandle *parent)
{
  imlabImageDocument *document = imlabGetCurrentDocument(parent);
  imImage *image = document->ImageFile->image;

  static double value = 0;
  static int best_type = 1;
  if (!IupGetParam(document->DlgTitle("Arithmetic (Const.)"), NULL, NULL,
               "Value (b): %R\n"
               "Operation: %l|a+b|a-b|a*b|a/b|abs(a-b)|a^b|min(a,b)|max(a,b)|\n"
               "Best Data Type: %b\n", 
               &value, &op, &best_type, NULL))
    return IUP_DEFAULT;

  int data_type = image->data_type;
  if (best_type)
  {
    if ((op == IM_BIN_POW || op == IM_BIN_DIV) && image->data_type != IM_CFLOAT && image->data_type != IM_CDOUBLE)
    {
      data_type = IM_FLOAT;
      if (image->data_type == IM_DOUBLE) data_type = IM_DOUBLE;
    }
    else if ((op == IM_BIN_ADD || op == IM_BIN_MUL) && image->data_type == IM_BYTE)
      data_type = IM_USHORT;
    else if ((op == IM_BIN_ADD || op == IM_BIN_SUB || op == IM_BIN_MUL) && (image->data_type == IM_BYTE || image->data_type == IM_USHORT))
      data_type = IM_INT;
  }

  imImage *NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessArithmeticConstOp(image, value, NewImage, op);

  imlabLogMessagef("%s Time = [%s]", GetBinaryOpStr(op), ctTimerCount());

  document->ChangeImage(NewImage, "ArithmeticConstOp{const=%g, op=\"%s\"}", value, GetBinaryOpStr(op));

  return IUP_DEFAULT;
}
                  
static int pow_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_POW, parent);
}

static int min_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_MIN, parent);
}

static int max_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_MAX, parent);
}

static int add_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_ADD, parent);
}

static int sub_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_SUB, parent);
}

static int mul_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_MUL, parent);
}

static int div_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_DIV, parent);
}

static int diff_constbinop(Ihandle *parent)
{
  return constbin_op(IM_BIN_DIFF, parent);
}

static int mean_op( Ihandle *parent )
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;
  int num_image;

  imlabImageDocument** document_list = imlabImageDocumentListSelectMulti("Select Other Images", NULL, image1, &num_image);
  if (!document_list)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imImage** src_image_list = (imImage**)malloc((num_image+1)*sizeof(imImage*));

  int N = 0;
  src_image_list[N] = image1;
  N++;

  for(int i = 0; i < num_image; i++)
  {
    if (document_list[i] != document1)
    {
      src_image_list[N] = document_list[i]->ImageFile->image;
      N++;
    }
  }

  imProcessMultipleMean((const imImage**)src_image_list, N, NewImage);

  imlabLogMessagef("Mean of %d Images Time = [%s]", N, ctTimerCount());

  document1->ChangeImage(NewImage, "MultipleMean{src_image_list = MultiSelect{count=%d}}", N);

  free(src_image_list);

  return IUP_DEFAULT;
}


static int median_op(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;
  int num_image;

  imlabImageDocument** document_list = imlabImageDocumentListSelectMulti("Select Other Images", NULL, image1, &num_image);
  if (!document_list)
    return IUP_DEFAULT;

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imImage** src_image_list = (imImage**)malloc((num_image + 1)*sizeof(imImage*));

  int N = 0;
  src_image_list[N] = image1;
  N++;

  for (int i = 0; i < num_image; i++)
  {
    if (document_list[i] != document1)
    {
      src_image_list[N] = document_list[i]->ImageFile->image;
      N++;
    }
  }

  imProcessMultipleMedian((const imImage**)src_image_list, N, NewImage);

  imlabLogMessagef("Median of %d Images Time = [%s]", N, ctTimerCount());

  document1->ChangeImage(NewImage, "MultipleMedian{src_image_list = MultiSelect{count=%d}}", N);

  free(src_image_list);

  return IUP_DEFAULT;
}

static int stddev_op(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument *document2 = imlabImageDocumentListSelect("Select Mean Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *mean_image = document2->ImageFile->image;

  int num_image;
  imlabImageDocument** document_list = imlabImageDocumentListSelectMulti("Select Other Images", NULL, image1, &num_image);
  if (!document_list)
    return IUP_DEFAULT;

  int data_type = IM_FLOAT;
  if (image1->data_type == IM_DOUBLE) data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imImage** src_image_list = (imImage**)malloc((num_image+1)*sizeof(imImage*));

  int N = 0;
  src_image_list[N] = image1;
  N++;

  for(int i = 0; i < num_image; i++)
  {
    if (document_list[i] != document1 && document_list[i] != document2)
    {
      src_image_list[N] = document_list[i]->ImageFile->image;
      N++;
    }
  }

  imProcessMultipleStdDev((const imImage**)src_image_list, N, mean_image, NewImage);

  imlabLogMessagef("StdDev of %d Images Time = [%s]", N, ctTimerCount());

  document1->ChangeImage(NewImage, "MultipleStdDev{src_image_list = MultiSelect{count=%d}, mean_image=\"%s\"}", N, document2->FileTitle);

  return IUP_DEFAULT;
}

static int autocov_op( Ihandle *parent )
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  imlabImageDocument *document2 = imlabImageDocumentListSelect("Select Mean Image", NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  int data_type = IM_FLOAT;
  if (image1->data_type == IM_DOUBLE) data_type = IM_DOUBLE;

  imImage *NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  int ret = imProcessAutoCovariance(image1, image2, NewImage);

  if (!ret)
    imImageDestroy(NewImage);
  else
    document1->ChangeImage(NewImage, "AutoCovariance{mean_image=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int blendconst_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image1 = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int image2_index = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
    imImage *image2 = document2->ImageFile->image;

    double alpha = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");

    imProcessBlendConst(image1, image2, NewImage, alpha);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int blendconst_op(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;
  static double alpha = 0.5;
  int show_preview = 0;

  int image2_index;
  char format[4096] = "Second Image: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, image1, &image2_index))
    return IUP_DEFAULT;

  strcat(format, "Alpha: %R[0,1]\n");
  strcat(format, "Preview: %b\n");

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document1, image1, NewImage);

  if (!IupGetParam(document1->DlgTitle("Blend Const"), blendconst_preview, preview, //check_alpha, &image1->data_type,
                format, 
                &image2_index, &alpha, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
  imImage *image2 = document2->ImageFile->image;

  ctTimerStart();

  if (!show_preview)
    imProcessBlendConst(image1, image2, NewImage, alpha);

  imlabLogMessagef("Blend Const Time = [%s]", ctTimerCount());

  document1->ChangeImage(NewImage, "BlendConst{image=\"%s\", alpha=%g}", document2->FileTitle, alpha);

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

static int blend_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM2", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image1 = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int image2_index = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
    imImage *image2 = document2->ImageFile->image;
    int alpha_index = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    imlabImageDocument * document_alpha = imlabImageDocumentListGetMatch(alpha_index, iMatchAlphaImage, image1);
    imImage *alpha = document_alpha->ImageFile->image;

    imProcessBlend(image1, image2, alpha, NewImage);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int blend_op(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;
  
  int image2_index;
  char format[4096] = "Second Image: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, image1, &image2_index))
    return IUP_DEFAULT;

  int alpha_index;
  strcat(format, "Alpha Image: %l");
  if (!imlabImageDocumentListInitFormat(format+strlen(format), iMatchAlphaImage, image1, &alpha_index))
    return IUP_DEFAULT;

  int show_preview = 0;
  strcat(format, "Preview: %b\n");

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document1, image1, NewImage);

  if (!IupGetParam(document1->DlgTitle("Blend"), blend_preview, preview,
                format, 
                &image2_index, &alpha_index, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
  imImage *image2 = document2->ImageFile->image;

  imlabImageDocument * document_alpha = imlabImageDocumentListGetMatch(alpha_index, iMatchAlphaImage, image1);
  imImage *alpha = document_alpha->ImageFile->image;

  ctTimerStart();

  if (!show_preview)
    imProcessBlend(image1, image2, alpha, NewImage);

  imlabLogMessagef("Blend Time = [%s]", ctTimerCount());

  document1->ChangeImage(NewImage, "Blend{image=\"%s\", alpha_image=\"%s\"}", document2->FileTitle, document_alpha->FileTitle);

  return IUP_DEFAULT;
}
                  
static int iHasAlpha(const imImage* image)
{
  return image->has_alpha;
}

static int compose_op(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;
  
  int image2_index;
  char format[4096] = "Second Image: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), (imlabMatchFunc)iHasAlpha, image1, &image2_index))
    return IUP_DEFAULT;

  if (!IupGetParam(document1->DlgTitle("Compose"), NULL, NULL,
                format, 
                &image2_index, NULL))
    return IUP_DEFAULT;

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
  imImage *image2 = document2->ImageFile->image;

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessCompose(image1, image2, NewImage);

  imlabLogMessagef("Blend Time = [%s]", ctTimerCount());

  document1->ChangeImage(NewImage, "Compose{image=\"%s\"}", document2->FileTitle);

  return IUP_DEFAULT;
}

static int backsub_preview(Ihandle* dialog, int param_index, void* user_data)
{
  Ihandle* preview = (Ihandle*)user_data;

  int param_readonly = 0;
  if (imlabProcessPreviewCheckParam(dialog, param_index, "PARAM3", preview, &param_readonly))
  {
    imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
    imImage *image1 = (imImage*)IupGetAttribute(preview, "CurrentImage");
    int image2_index = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM0"), "VALUE");
    imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image2_index, NULL, image1);
    imImage *image2 = document2->ImageFile->image;

    double tol = IupGetDouble((Ihandle*)IupGetAttribute(dialog, "PARAM1"), "VALUE");
    int diff = IupGetInt((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");

    imProcessBackSub(image1, image2, NewImage, tol, diff);

    imlabProcessPreviewUpdate(preview);
  }

  if (param_readonly)
    return 0;

  return 1;
}

static int backsub_binop(Ihandle *parent)
{
  imlabImageDocument *document1 = imlabGetCurrentDocument(parent);
  imImage *image1 = document1->ImageFile->image;

  static double tol = 5;
  static int diff = 1;
  int image_win;
  int show_preview = 0;
  char format[4096] = "Second Image: %l";
  if (!imlabImageDocumentListInitFormat(format+strlen(format), NULL, image1, &image_win))
    return IUP_DEFAULT;

  strcat(format, "Tolerance: %R\n");
  strcat(format, "Use Difference: %b\n");
  strcat(format, "Preview: %b\n");

  imImage *NewImage = imImageClone(image1);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  Ihandle* preview = imlabProcessPreviewInit(document1, image1, NewImage);

  if (!IupGetParam(document1->DlgTitle("Background Subtraction"), backsub_preview, preview, format,
                   &image_win, &tol, &diff, &show_preview, NULL))
  {
    imImageDestroy(NewImage);
    return IUP_DEFAULT;
  }

  imlabImageDocument * document2 = imlabImageDocumentListGetMatch(image_win, NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage *image2 = document2->ImageFile->image;

  ctTimerStart();

  if (!show_preview)
    imProcessBackSub(image1, image2, NewImage, tol, diff);

  imlabLogMessagef("Background Subtraction Time = [%s]", ctTimerCount());

  document1->ChangeImage(NewImage, "BackSub{back_image=\"%s\", tol=%g, diff=%d}", document2->FileTitle, tol, diff);

  return IUP_DEFAULT;
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Add...", "add_binop", (Icallback) add_binop, 1),
    imlabProcNewItem(mnProcess, "Subtract...", "sub_binop", (Icallback) sub_binop, 1),
    imlabProcNewItem(mnProcess, "Multiply...", "mul_binop", (Icallback) mul_binop, 1),
    imlabProcNewItem(mnProcess, "Divide...", "div_binop", (Icallback) div_binop, 1),
    imlabProcNewItem(mnProcess, "Power...", "pow_binop", (Icallback) pow_binop, 1),
    imlabProcNewItem(mnProcess, "Maximum...", "max_binop", (Icallback) max_binop, 1),
    imlabProcNewItem(mnProcess, "Minimum...", "min_binop", (Icallback) min_binop, 1),
    imlabProcNewItem(mnProcess, "Difference...", "diff_binop", (Icallback) diff_binop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Blend Const...", "blendconst_op", (Icallback) blendconst_op, 1),
    imlabProcNewItem(mnProcess, "Blend...", "blend_op", (Icallback) blend_op, 1),
    imlabProcNewItem(mnProcess, "Compose...", "compose_op", (Icallback) compose_op, 1),
    imlabProcNewItem(mnProcess, "Background Subtraction...", "backsub_binop", (Icallback) backsub_binop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Mean (N)...", "mean_op", (Icallback) mean_op, 1),
    imlabProcNewItem(mnProcess, "Std Dev (N)...", "stddev_op", (Icallback) stddev_op, 1),
    imlabProcNewItem(mnProcess, "Median (N)...", "median_op", (Icallback)median_op, 1),
    imlabProcNewItem(mnProcess, "Auto Covariance...", "autocov_op", (Icallback)autocov_op, 1),
    NULL);

  IupSetAttribute((Ihandle*)IupGetAttribute(mnProcess, "backsub_binop"), "imlabStatusHelp", "Allows the use of a tolerance for the diff and to preserve the original image.");

  IupAppend(mnProcess, imlabSubmenu("Arithmetic", submenu));

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Add...", "add_constbinop", (Icallback) add_constbinop, 1),
    imlabProcNewItem(mnProcess, "Subtract...", "sub_constbinop", (Icallback) sub_constbinop, 1),
    imlabProcNewItem(mnProcess, "Multiply...", "mul_constbinop", (Icallback) mul_constbinop, 1),
    imlabProcNewItem(mnProcess, "Divide...", "div_constbinop", (Icallback) div_constbinop, 1),
    imlabProcNewItem(mnProcess, "Power...", "pow_constbinop", (Icallback) pow_constbinop, 1),
    imlabProcNewItem(mnProcess, "Maximum...", "max_constbinop", (Icallback) max_constbinop, 1),
    imlabProcNewItem(mnProcess, "Minimum...", "min_constbinop", (Icallback) min_constbinop, 1),
    imlabProcNewItem(mnProcess, "Difference...", "diff_constbinop", (Icallback) diff_constbinop, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Arithmetic (Const.)", submenu));
}

static int imImageIsSciAlpha(const imImage* image)
{
  return imImageIsSci(image) && image->has_alpha;
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "add_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "sub_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "mul_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "div_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "pow_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "max_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "min_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "diff_binop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "add_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "sub_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "mul_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "div_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "pow_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "max_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "min_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "diff_constbinop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "blendconst_op", imImageIsSci); 
  imlabProcPlugInUpdateItem(mnProcess, "blend_op", imImageIsSci); 
  imlabProcPlugInUpdateItem(mnProcess, "compose_op", imImageIsSciAlpha); 
  imlabProcPlugInUpdateItem(mnProcess, "backsub_binop", imImageIsSci); 
  imlabProcPlugInUpdateItem(mnProcess, "mean_op", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "stddev_op", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "median_op", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "autocov_op", imImageIsSciNotComplex);
}


static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinCombine = &plugin;


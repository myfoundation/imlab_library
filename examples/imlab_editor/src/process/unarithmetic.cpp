#include "imagedocument.h"
#include "imlab.h"
#include "plugin_process.h"
#include "counter.h"
#include "dialogs.h"

#include <im_process.h>
#include <im_palette.h>

#include <stdlib.h>
#include <memory.h>
#include <string.h>


static int absolute(Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  imImage* NewImage = imImageClone(image);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imProcessUnArithmeticOp(image, NewImage, IM_UN_ABS);

  document->ChangeImage(NewImage, "UnArithmeticOp{op=\"Absolute\"}");

  return IUP_DEFAULT;
}

static char* GetUnaryOpStr(int op)
{
  switch(op)
  {
  case IM_UN_ABS:
    return "Absolute";
  case IM_UN_INV:
    return "Inverse";
  case IM_UN_LESS:
    return "Less";
  case IM_UN_SQR:
    return "Square";
  case IM_UN_SQRT:
    return "SquareRoot";
  case IM_UN_LOG:
    return "Logarithm";
  case IM_UN_SIN:
    return "Sine";
  case IM_UN_COS:
    return "Cosine";
  case IM_UN_EXP:
    return "Exponencial";
  case IM_UN_CPXNORM:
    return "ComplexNormalize";
  case IM_UN_CONJ:
    return "Conjugate";
  case IM_UN_POSITIVES:
    return "Positives";
  case IM_UN_NEGATIVES:
    return "Negatives";
  }
                            
  return NULL;
}

static int un_op(int op, Ihandle *parent)
{
  int data_type;

  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;

  static int best_type = 1;
  if (!IupGetParam(document->DlgTitle(GetUnaryOpStr(op)), NULL, NULL, "Best Data Type: %b\n",
    &best_type, NULL))
    return IUP_DEFAULT;

  data_type = image->data_type;
  if (best_type)
  {
    if ((op == IM_UN_INV || op == IM_UN_SIN || op == IM_UN_COS || op == IM_UN_LOG || op == IM_UN_EXP) &&
        image->data_type != IM_CFLOAT && image->data_type != IM_CDOUBLE)
    {
      if (image->data_type != IM_DOUBLE)
        data_type = IM_DOUBLE;
      else
        data_type = IM_FLOAT;
    }
    else if ((op == IM_UN_LESS || op == IM_UN_SQR) && image->data_type == IM_BYTE)
      data_type = IM_INT;
  }

  imImage* NewImage = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessUnArithmeticOp(image, NewImage, op);

  imlabLogMessagef("%s Time = [%s]", GetUnaryOpStr(op), ctTimerCount());

  document->ChangeImage(NewImage, "UnArithmeticOp{op=\"%s\"}", GetUnaryOpStr(op));

  return IUP_DEFAULT;
}

static int cpxnorm_unop(Ihandle *parent)
{
  return un_op(IM_UN_CPXNORM, parent);
}

static int conj_unop(Ihandle *parent)
{
  return un_op(IM_UN_CONJ, parent);
}

static int inv_unop(Ihandle *parent)
{
  return un_op(IM_UN_INV, parent);
}

static int less_unop(Ihandle *parent)
{
  return un_op(IM_UN_LESS, parent);
}

static int positives_unop(Ihandle *parent)
{
  return un_op(IM_UN_POSITIVES, parent);
}

static int negatives_unop(Ihandle *parent)
{
  return un_op(IM_UN_NEGATIVES, parent);
}

static int sqr_unop(Ihandle *parent)
{
  return un_op(IM_UN_SQR, parent);
}

static int sqrt_unop(Ihandle *parent)
{
  return un_op(IM_UN_SQRT, parent);
}

static int exp_unop(Ihandle *parent)
{
  return un_op(IM_UN_EXP, parent);
}

static int log_unop(Ihandle *parent)
{
  return un_op(IM_UN_LOG, parent);
}

static int sin_unop(Ihandle *parent)
{
  return un_op(IM_UN_SIN, parent);
}

static int cos_unop(Ihandle *parent)
{
  return un_op(IM_UN_COS, parent);
}

static int split_complex(int polar, Ihandle *parent)
{
  imlabImageDocument* document = imlabGetCurrentDocument(parent);
  imImage* image = document->ImageFile->image;
  int data_type = IM_FLOAT;
  if (image->data_type == IM_CDOUBLE) 
    data_type = IM_DOUBLE;

  imImage* NewImage1 = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage1)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  imImage* NewImage2 = imImageCreateBased(image, -1, -1, -1, data_type);
  if (!NewImage2)
  {
    imlabDlgMemoryErrorMsg();
    imImageDestroy(NewImage1);
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessSplitComplex(image, NewImage1, NewImage2, polar);

  imlabLogMessagef("Split Complex Time = [%s]", ctTimerCount());

  char* cpxdesc1 = "Real";
  char* cpxdesc2 = "Imaginary";
  if (polar)
  {
    cpxdesc1 = "Magnitude";
    cpxdesc2 = "Phase";

    imImageSetPalette(NewImage2, imPaletteHues(), 256);
  }

  imlabImageDocumentCreateFromImage(NewImage1, "Complex%s of %s", "Complex%s{image=\"%s\"}", cpxdesc1, document->FileTitle);
  imlabImageDocumentCreateFromImage(NewImage2, "Complex%s of %s", "Complex%s{image=\"%s\"}", cpxdesc2, document->FileTitle);

  return IUP_DEFAULT;
}

static int split_polar(Ihandle *parent)
{
  return split_complex(1, parent);
}
                  
static int split_realimag(Ihandle *parent)
{
  return split_complex(0, parent);
}
                  
static int merge_complex(int polar, Ihandle *parent)
{
  imlabImageDocument* document1 = imlabGetCurrentDocument(parent);
  imImage* image1 = document1->ImageFile->image;

  char* cpxdesc = "Imaginary Part Image";
  if (polar) cpxdesc = "Select Phase Image";

  imlabImageDocument* document2 = imlabImageDocumentListSelect(cpxdesc, NULL, image1);
  if (!document2)
    return IUP_DEFAULT;

  imImage* image2 = document2->ImageFile->image;
  int data_type = IM_CFLOAT;
  if (image1->data_type == IM_DOUBLE) 
    data_type = IM_CDOUBLE;

  imImage* NewImage = imImageCreateBased(image1, -1, -1, -1, data_type);
  if (!NewImage)
  {
    imlabDlgMemoryErrorMsg();
    return IUP_DEFAULT;
  }

  ctTimerStart();

  imProcessMergeComplex(NewImage, image1, image2, polar);

  imlabLogMessagef("Merge Complex Time = [%s]", ctTimerCount());

  imlabImageDocumentCreateFromImage(NewImage, "MergeComplex", "MergeComplex{image1=\"%s\", image2=\"%s\", polar=%d}", document1->FileTitle, document2->FileTitle, polar);

  return IUP_DEFAULT;
}

static int merge_polar(Ihandle *parent)
{
  return merge_complex(1, parent);
}
                  
static int merge_realimag(Ihandle *parent)
{
  return merge_complex(0, parent);
}

static void PlugInInit(Ihandle* mnProcess)
{
  Ihandle* submenu;

  submenu = IupMenu(
    imlabProcNewItem(mnProcess, "Less...", "less_unop", (Icallback) less_unop, 1),
    imlabProcNewItem(mnProcess, "Absolute...", "absolute", (Icallback) absolute, 1),
    imlabProcNewItem(mnProcess, "Positives...", "positives_unop", (Icallback)positives_unop, 1),
    imlabProcNewItem(mnProcess, "Negatives...", "negatives_unop", (Icallback)negatives_unop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Inverse...", "inv_unop", (Icallback) inv_unop, 1),
    imlabProcNewItem(mnProcess, "Square...", "sqr_unop", (Icallback) sqr_unop, 1),
    imlabProcNewItem(mnProcess, "Square Root...", "sqrt_unop", (Icallback) sqrt_unop, 1),
    imlabProcNewItem(mnProcess, "Exponencial...", "exp_unop", (Icallback) exp_unop, 1),
    imlabProcNewItem(mnProcess, "Logarithm...", "log_unop", (Icallback) log_unop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Sine...", "sin_unop", (Icallback) sin_unop, 1),
    imlabProcNewItem(mnProcess, "Cosine...", "cos_unop", (Icallback) cos_unop, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Split Complex", "split_realimag", (Icallback) split_realimag, 1),
    imlabProcNewItem(mnProcess, "Split Complex (Polar)", "split_polar", (Icallback) split_polar, 1),
    imlabProcNewItem(mnProcess, "Merge Complex...", "merge_realimag", (Icallback) merge_realimag, 1),
    imlabProcNewItem(mnProcess, "Merge Complex (Polar)...", "merge_polar", (Icallback) merge_polar, 1),
    IupSeparator(),
    imlabProcNewItem(mnProcess, "Complex Conjugate", "conj_unop", (Icallback) conj_unop, 1),
    imlabProcNewItem(mnProcess, "Complex Normalize", "cpxnorm_unop", (Icallback) cpxnorm_unop, 1),
    NULL);

  IupAppend(mnProcess, imlabSubmenu("Arithmetic (Unary)", submenu));
}

static void PlugInUpdate(Ihandle* mnProcess)
{
  imlabProcPlugInUpdateItem(mnProcess, "inv_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "less_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "positives_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "negatives_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "sqr_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "sqrt_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "exp_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "log_unop", imImageIsSci);
  imlabProcPlugInUpdateItem(mnProcess, "sin_unop", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "cos_unop", imImageIsSciNotComplex);
  imlabProcPlugInUpdateItem(mnProcess, "absolute", imImageIsSigned);
  imlabProcPlugInUpdateItem(mnProcess, "split_realimag", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "split_polar", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "merge_realimag", imImageIsReal);
  imlabProcPlugInUpdateItem(mnProcess, "merge_polar", imImageIsReal);
  imlabProcPlugInUpdateItem(mnProcess, "conj_unop", imImageIsComplex);
  imlabProcPlugInUpdateItem(mnProcess, "cpxnorm_unop", imImageIsComplex);
}


static imlabProcPlugIn plugin = 
{
  PlugInInit,
  PlugInUpdate,
  NULL
};

imlabProcPlugIn *iwinArith = &plugin;


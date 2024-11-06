
#include <im.h>
#include <im_image.h>

#include "im_imagematch.h"

#include <stdlib.h>
#include <memory.h>

int imImageIsBinary(const imImage* image)
{
  return image != NULL && (image->color_space == IM_BINARY);
}

int imImageIsMap(const imImage* image)
{
  return image != NULL && (image->color_space == IM_MAP);
}

int imImageIsGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY);
}

int imImageIsRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB);
}

int imImageIsNotMap(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP);
}

int imImageIsNotGray(const imImage* image)
{
  return image != NULL && (image->color_space != IM_GRAY);
}

int imImageIsNotRGB(const imImage* image)
{
  return image != NULL && (image->color_space != IM_RGB);
}

int imImageIsColor(const imImage* image)
{
  return image != NULL && (image->depth != 1);
}

int imImageIsByteGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && image->data_type == IM_BYTE);
}

int imImageIsByteRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB && image->data_type == IM_BYTE);
}

int imImageIsUShortGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && image->data_type == IM_USHORT);
}

int imImageIsUShortRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB && image->data_type == IM_USHORT);
}

int imImageIsIntGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && image->data_type == IM_INT);
}

int imImageIsIntRealGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && (image->data_type == IM_INT || image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE));
}

int imImageIsIntRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB && image->data_type == IM_INT);
}

int imImageIsRealGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && (image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE));
}

int imImageIsRealRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB && (image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE));
}

int imImageIsComplexGray(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && (image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE));
}

int imImageIsComplexRGB(const imImage* image)
{
  return image != NULL && (image->color_space == IM_RGB && (image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE));
}

int imImageIsByte(const imImage* image)
{
  return image != NULL && (image->data_type == IM_BYTE);
}

int imImageIsInt(const imImage* image)
{
  return image != NULL && (image->data_type == IM_INT);
}

int imImageIsShort(const imImage* image)
{
  return image != NULL && (image->data_type == IM_SHORT);
}

int imImageIsUShort(const imImage* image)
{
  return image != NULL && (image->data_type == IM_USHORT);
}

int imImageIsReal(const imImage* image)
{
  return image != NULL && (image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE);
}

int imImageIsComplex(const imImage* image)
{
  return image != NULL && (image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE);
}

int imImageIsComplexFloat(const imImage* image)
{
  return image != NULL && (image->data_type == IM_CFLOAT);
}

int imImageIsNotComplex(const imImage* image)
{
  return image != NULL && (image->data_type != IM_CFLOAT && image->data_type != IM_CDOUBLE);
}

int imImageIsNotByte(const imImage* image)
{
  return image != NULL && (image->data_type != IM_BYTE);
}

int imImageIsInteger(const imImage* image)
{
  return image != NULL && ((image->color_space != IM_MAP && image->data_type == IM_BYTE) || image->data_type == IM_SHORT || image->data_type == IM_USHORT || image->data_type == IM_INT);
}

int imImageIsUShort2Real(const imImage* image)
{
  return image != NULL && (image->data_type == IM_USHORT || image->data_type == IM_INT || image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE);
}

int imImageIsSigned(const imImage* image)
{
  return image != NULL && (image->data_type == IM_INT || image->data_type == IM_FLOAT || 
                                                         image->data_type == IM_DOUBLE);
}

int imImageIsRealComplex(const imImage* image)
{
  return image != NULL && (image->data_type == IM_FLOAT || image->data_type == IM_DOUBLE || 
                           image->data_type == IM_CFLOAT || image->data_type == IM_CDOUBLE);
}

int imImageIsSci(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP && image->color_space != IM_BINARY);
}

int imImageIsSciByte(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP && image->data_type == IM_BYTE);
}

int imImageIsSciByteShortUshort(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP && (image->data_type == IM_BYTE || 
                                                            image->data_type == IM_SHORT ||
                                                            image->data_type == IM_USHORT));
}

int imImageIsGrayNotComplex(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && image->data_type != IM_CFLOAT && 
                                                            image->data_type != IM_CDOUBLE);
}

int imImageIsGrayInteger(const imImage* image)
{
  return image != NULL && (image->color_space == IM_GRAY && (image->data_type != IM_CFLOAT && image->data_type != IM_FLOAT && 
                                                             image->data_type != IM_CDOUBLE && image->data_type != IM_DOUBLE));
}

int imImageIsRGBNotComplex(const imImage* image)  
{
  return image != NULL && (image->color_space == IM_RGB && image->data_type != IM_CFLOAT && 
                                                           image->data_type != IM_CDOUBLE);
}

int imImageIsSciNotComplex(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP && image->data_type != IM_CFLOAT && 
                                                           image->data_type != IM_CDOUBLE);
}

int imImageIsSciNotDouble(const imImage* image)
{
  return image != NULL && (image->color_space != IM_MAP && image->color_space != IM_BINARY && 
                           image->data_type != IM_DOUBLE && image->data_type != IM_CDOUBLE);
}


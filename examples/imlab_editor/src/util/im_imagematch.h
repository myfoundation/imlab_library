/** \file
 * \brief Image Matching
 */

#ifndef __IMAGEMATCH_H
#define __IMAGEMATCH_H

#if	defined(__cplusplus)
extern "C" {
#endif

/*@{ 
 * Returns 1 if the image match the data type and/or color mode. Returns 0 otherwise.    */

int imImageIsBinary(const imImage* image); /* IsMap AND palette_count=2 */

int imImageIsMap(const imImage* image);
int imImageIsRGB(const imImage* image);
int imImageIsGray(const imImage* image);

int imImageIsNotMap(const imImage* image);
int imImageIsNotRGB(const imImage* image);
int imImageIsNotGray(const imImage* image);
int imImageIsColor(const imImage* image);  /* all colors except MAP */

int imImageIsByte(const imImage* image);
int imImageIsShort(const imImage* image);
int imImageIsUShort(const imImage* image);
int imImageIsInt(const imImage* image);
int imImageIsReal(const imImage* image);
int imImageIsComplex(const imImage* image);
int imImageIsComplexFloat(const imImage* image);

int imImageIsNotByte(const imImage* image);
int imImageIsNotComplex(const imImage* image);

int imImageIsByteGray(const imImage* image);
int imImageIsByteRGB(const imImage* image);
int imImageIsUShortGray(const imImage* image);
int imImageIsUShortRGB(const imImage* image);
int imImageIsIntGray(const imImage* image);
int imImageIsIntRGB(const imImage* image);
int imImageIsRealGray(const imImage* image);
int imImageIsRealRGB(const imImage* image);
int imImageIsComplexGray(const imImage* image);
int imImageIsComplexRGB(const imImage* image);
int imImageIsIntRealGray(const imImage* image);

int imImageIsSci(const imImage* image);            /* IsNotMap */
int imImageIsSciByte(const imImage* image);        /* IsNotMap AND IsByte */
int imImageIsSciByteShortUshort(const imImage* image);  /* IsNotMap AND (IsByte OR IsUShort) */
int imImageIsSciNotComplex(const imImage* image);  /* IsNotMap AND IsNotComplex */
int imImageIsGrayNotComplex(const imImage* image); 
int imImageIsRGBNotComplex(const imImage* image);  
int imImageIsGrayInteger(const imImage* image); 
int imImageIsSciNotDouble(const imImage* image);

int imImageIsInteger(const imImage* image);       /* (IsByte AND IsNotMap) OR IsUShort OR IsInt  */
int imImageIsUShort2Real(const imImage* image);   /* IsUShort OR IsInt OR IsReal */
int imImageIsSigned(const imImage* image);        /* IsInt OR IsReal */
int imImageIsRealComplex(const imImage* image);   /* IsReal OR IsComplex */

/*@}*/

#if	defined(__cplusplus)
}
#endif

#endif

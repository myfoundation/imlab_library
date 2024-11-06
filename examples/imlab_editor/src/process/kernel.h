/** \file
 * \brief Kernel for Convolution */

#ifndef __IMKERNEL_H
#define __IMKERNEL_H

/** Loads a kernel from file */
imImage* imKernelLoad(char* filename);

/** Kernel maximum size for the dialog */
#define IM_KERNELMAX 7

/** Dialog to retrieve a kernel */
imImage* imGetKernel(const char* filetitle, int square);

#endif  /* __IMKERNEL_H */

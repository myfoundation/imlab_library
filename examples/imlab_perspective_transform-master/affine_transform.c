#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include "iocore.h"
#include "imcore.h"
#include "lacore.h"
#include "cvcore.h"
#include "mlcore.h"

int main(int argc, unsigned char *argv[]) {

    // read the test image
    matrix_t *test  = imread("../data/affine.bmp");
    matrix_t *test_aligned = matrix_create(uint8_t, rows(test), cols(test), 3);

    // create transform
    matrix_t *transform = rot2tform(128, 128, 45, 1.0);
    //matrix_t *transform = rot2tform(128, 128, 90, 1.0);
    //matrix_t *transform = rot2tform(128, 128, 45, 0.5);
    
    // print the transform
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 0, 0), atf(transform, 0, 1), atf(transform, 0, 2));
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 1, 0), atf(transform, 1, 1), atf(transform, 1, 2));
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 2, 0), atf(transform, 2, 1), atf(transform, 2, 2));

    // do the transform
    imtransform(test, transform, test_aligned);

    // write the resulting image
    imwrite(test_aligned, "../data/affine_transformed.bmp");

    return 0;
}
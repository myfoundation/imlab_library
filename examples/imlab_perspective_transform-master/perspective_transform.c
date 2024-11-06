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
    matrix_t *test  = imread("../data/rubik.bmp");
    matrix_t *test_aligned = matrix_create(uint8_t, rows(test), cols(test), 3);

    // find the key points
    struct point_t source[4] = {{.x = 17, .y = 46}, {.x = 288, .y = 3}, {.x = 495, .y = 89}, {.x = 183, .y = 156}};
    struct point_t destination[4] = {{.x = 0, .y = 0}, {.x = 511, .y = 0}, {.x = 511, .y = 511}, {.x = 0, .y = 511}};

    // correct the image
    matrix_t *transform = pts2tform(source, destination, 4);

    // print the transform
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 0,0), atf(transform, 0,1), atf(transform, 0,2));
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 1,0), atf(transform, 1,1), atf(transform, 1,2));
    printf("%5.3f %5.3f %5.3f\n", atf(transform, 2,0), atf(transform, 2,1), atf(transform, 2,2));

    // do the transform
    imtransform(test, transform, test_aligned);

    // write the resulting image
    imwrite(test_aligned, "../data/test_aligned.bmp");

    return 0;
}
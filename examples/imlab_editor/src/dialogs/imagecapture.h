/** \file
 * \brief Image Capture Dialog */

#ifndef __IMAGECAPTURE_H
#define __IMAGECAPTURE_H


/** Optional image capture callback.
 * If return 1 capture continuously. If return 0 stop capturing. */
typedef int (*imlabImageCaptureCallback)(void* user_data, imImage* image, int device);

/** Optional capture config callback. If videocapture==NULL the dialog is about to close.
 * Uses an imVideoCapture object. */
typedef void (*imlabImageCaptureConfig)(void* user_data, void* videocapture);

/** Displays the image capture dialog. Windows Only.
 * Implemented in imagecapture.c */
void imlabImageCaptureImageShow(const char* title, void* user_data, 
                          imlabImageCaptureCallback _capture_callback, 
                          imlabImageCaptureCallback _preview_callback, 
                          imlabImageCaptureConfig _config_callback);

/** Captures one image from each available device. */
void imlabImageCaptureAll(void* user_data, imlabImageCaptureCallback _capture_callback);


#endif

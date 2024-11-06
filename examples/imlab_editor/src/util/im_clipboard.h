/** \file
 * \brief Clipboard copy/paste for imImage using IUP
 */

#ifndef __IM_CLIPBOARD_H
#define __IM_CLIPBOARD_H

#include <im.h>
#include <im_image.h>

#if	defined(__cplusplus)
extern "C" {
#endif

/** \defgroup clipboard Clipboard Utilities
 */


/** Paste an image from the clipboard.
 * Implemented in imagecapture.c
 * \ingroup clipboard */
imImage* imClipboardPasteImage(void);

/** Verifica se exite uma imagem no clipboard.
 * \ingroup clipboard */
int imClipboardCanPasteImage(void);

/** Copies an image to the clipboard.
 * \ingroup clipboard */
void imClipboardCopyImage(imImage* image);

/** Copies a text to the clipboard.
 * \ingroup clipboard */
void imClipboardCopyText(const char* str);


#if defined(__cplusplus)
}
#endif

#endif

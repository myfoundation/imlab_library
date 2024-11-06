/** \file
 * \brief Clipboard copy/paste for imImage using IUP
 *
 * See Copyright Notice in im_lib.h
 */

#include <stdlib.h>

#include "im_clipboard.h"

#include <iup.h>
#include <iupim.h>

void imClipboardCopyText(const char* str)
{
  Ihandle* clipboard = IupClipboard();
  IupSetAttribute(clipboard, "TEXT", NULL);  /* clear clipboard first */
  IupSetAttribute(clipboard, "TEXT", str);
  IupDestroy(clipboard);
}

void imClipboardCopyImage(imImage* image)
{
  Ihandle* clipboard = IupClipboard();
  IupSetAttribute(clipboard, "NATIVEIMAGE", NULL); /* clear clipboard first */
  IupSetAttribute(clipboard, "NATIVEIMAGE", (char*)IupGetImageNativeHandle(image));
  IupDestroy(clipboard);
}

imImage* imClipboardPasteImage(void)
{
  Ihandle* clipboard = IupClipboard();
  imImage* image = IupGetNativeHandleImage(IupGetAttribute(clipboard, "NATIVEIMAGE"));
  IupDestroy(clipboard);
  return image;
}

int imClipboardCanPasteImage(void)
{
  Ihandle* clipboard = IupClipboard();
  int ret = IupGetInt(clipboard, "IMAGEAVAILABLE");
  IupDestroy(clipboard);
  return ret;
}


#include <iup.h>
#include <iupcontrols.h>
#include <iupkey.h>
#include <im.h>
#include <im_binfile.h>
#include <cd.h>

#include "imlab.h"
#include "utl_file.h"
#include "dialogs.h"
#include <iup_config.h>

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


struct SaveAsDialog
{
  int saReturn, saMulti;
  char saFormat[30];
  char saCompression[50];
  char saFileName[2048];
  Ihandle *txtFileName, *lstFormat, *lstCompression, *dialog;
  imImage* image;
};

static void iGetFormat(char* saFormat, char* file_format)
{
  while(*file_format != ' ')
  {
    *saFormat++ = *file_format++;
  }
  *saFormat = 0;
}

static void iFileNameReplaceExtension(char* filename, char* format_ext)
{
  int len = (int)strlen(filename);

  // find the extension in the file name

  char* ext = &filename[len-1];
  while (ext != filename)
  {
    if (*ext == '.')
      break;

    if (*ext == '\\' || *ext == '/')
    {
      ext = filename;  // force next 'if' condition
      break;
    }

    ext--;
  }

  if (ext == filename) 
    ext = &filename[len];  // no extension found
  else
  {
    ext++; // jump '.'

    // Check if the extension is already there
    if (strstr(ext, format_ext) != NULL)
      return;

    ext--; // return to force overwrite the '.'
  }

  // find and isolate the first extension in the extension list

  char* div = format_ext += 1; // ignore the '*' character
  while (*div != 0)
  {
    if (*div == ';')
    {
      *div = 0;
      break;
    }
    div++;
  }

  // contact file and extension (with '.')
  strcpy(ext, format_ext);
}

static int cbSaveAsFormat(Ihandle* self)
{
  SaveAsDialog* dlg = (SaveAsDialog*)IupGetAttribute(self, "SaveAsDialog");

  char str[5];
  strcpy(str, IupGetAttribute(dlg->lstFormat, "VALUE"));
  iGetFormat(dlg->saFormat, IupGetAttribute(dlg->lstFormat, str));

  char* comp[50];
  int comp_count;
  imFormatCompressions(dlg->saFormat, comp, &comp_count, dlg->image->color_space, dlg->image->data_type);

  IupSetAttribute(dlg->lstCompression, "1", "Default");
  IupSetAttribute(dlg->lstCompression, "VALUE", "1");

  int cc = 2;
  for (int c = 0; c < comp_count; c++)
  {
    int error = imFormatCanWriteImage(dlg->saFormat, comp[c], dlg->image->color_space, dlg->image->data_type);
    if (error)
      continue;

    sprintf(str, "%d", cc);
    IupStoreAttribute(dlg->lstCompression, str, comp[c]);

    if (imStrEqual(dlg->saCompression, comp[c]))
      IupStoreAttribute(dlg->lstCompression, "VALUE", str);

    cc++;
  }

  sprintf(str, "%d", cc);
  IupStoreAttribute(dlg->lstCompression, str, NULL);

  char format_ext[50];
  imFormatInfo(dlg->saFormat, NULL, format_ext, NULL);

  strcpy(dlg->saFileName, IupGetAttribute(dlg->txtFileName, "VALUE"));
  iFileNameReplaceExtension(dlg->saFileName, format_ext);
  IupStoreAttribute(dlg->txtFileName, "VALUE", dlg->saFileName);

  return IUP_DEFAULT;
}

static int cbSaveAsOK(Ihandle* self)
{
  SaveAsDialog* dlg = (SaveAsDialog*)IupGetAttribute(self, "SaveAsDialog");

  strcpy(dlg->saFileName, IupGetAttribute(dlg->txtFileName, "VALUE"));

  char str[5];
  strcpy(str, IupGetAttribute(dlg->lstFormat, "VALUE"));
  iGetFormat(dlg->saFormat, IupGetAttribute(dlg->lstFormat, str));

  strcpy(str, IupGetAttribute(dlg->lstCompression, "VALUE"));
  strcpy(dlg->saCompression, IupGetAttribute(dlg->lstCompression, str));

  if (imStrEqual(dlg->saCompression, "Default"))
    dlg->saCompression[0] = 0;
  else
  {
    if (imStrEqual(dlg->saCompression, "JPEG"))
    {
      int quality = IupConfigGetVariableIntDef(imlabConfig(), "Compression", "JPEGQuality", 75);
      if (!IupGetParam("Compression Options", NULL, NULL,
                       "JPEGQuality: %i[0,100]\n", 
                       &quality, NULL))
        return IUP_DEFAULT;

      imImageSetAttribInteger(dlg->image, "JPEGQuality", IM_INT, quality);
      IupConfigSetVariableInt(imlabConfig(), "Compression", "JPEGQuality", quality);
    }
    else if (imStrEqual(dlg->saCompression, "DEFLATE"))
    {
      int quality = IupConfigGetVariableIntDef(imlabConfig(), "Compression", "ZIPQuality", 6);
      if (!IupGetParam("Compression Options", NULL, NULL,
                       "ZIPQuality: %i[1,9]\n", 
                       &quality, NULL))
        return IUP_DEFAULT;

      imImageSetAttribInteger(dlg->image, "ZIPQuality", IM_INT, quality);
      IupConfigSetVariableInt(imlabConfig(), "Compression", "ZIPQuality", quality);
    }
    else if (imStrEqual(dlg->saCompression, "JPEG-2000"))
    {
      double ratio = IupConfigGetVariableDoubleDef(imlabConfig(), "Compression", "J2KRatio", 1.0);
      if (!IupGetParam("Compression Options", NULL, NULL,
                       "CompressionRatio: %R[1,]\n",
                       &ratio, NULL))
        return IUP_DEFAULT;

      imImageSetAttribReal(dlg->image, "CompressionRatio", IM_FLOAT, ratio);
    }
    else if (imStrEqual(dlg->saFormat, "AVI") && !imStrEqual(dlg->saCompression, "NONE"))
    {
      int quality = IupConfigGetVariableIntDef(imlabConfig(), "Compression", "AVIQuality", -1);
      if (!IupGetParam("Compression Options", NULL, NULL,
                       "AVIQuality: %i[1,10000]\n", 
                       &quality, NULL))
        return IUP_DEFAULT;

      imImageSetAttribInteger(dlg->image, "AVIQuality", IM_INT, quality);
      IupConfigSetVariableInt(imlabConfig(), "Compression", "AVIQuality", quality);
    }
  }

  int error = imFormatCanWriteImage(dlg->saFormat, dlg->saCompression, dlg->image->color_space, dlg->image->data_type);
  if (error)
  {
    imlabDlgFileErrorMsg("Save As", error, dlg->saFileName);
    return IUP_DEFAULT;
  }

  dlg->saReturn = 1;
  return IUP_CLOSE;
}

static int cbSaveAsCancel(Ihandle* self)
{
  SaveAsDialog* dlg = (SaveAsDialog*)IupGetAttribute(self, "SaveAsDialog");
  dlg->saReturn = 0;
  return IUP_CLOSE;
}

int show_help(Ihandle* self)
{
  IupMessage("Help", IupGetAttribute(self, "VALUE"));
  return IUP_DEFAULT;
}

static SaveAsDialog* CreateSaveAsDialog(void)
{
  SaveAsDialog* dlg = (SaveAsDialog*)malloc(sizeof(SaveAsDialog));

  Ihandle* bt_ok = IupButton( "OK", NULL);
  IupSetAttribute(bt_ok,"SIZE","40");
  IupSetHandle("btSaveOK", bt_ok);
  IupSetCallback(bt_ok, "ACTION", (Icallback)cbSaveAsOK);

  Ihandle* bt_cancel = IupButton( "Cancel", NULL);
  IupSetAttribute(bt_cancel,"SIZE","40");
  IupSetHandle("btSaveCancel", bt_cancel);
  IupSetCallback(bt_cancel, "ACTION", (Icallback)cbSaveAsCancel);

  Ihandle* button_box = IupVbox(bt_ok, bt_cancel, NULL);
  IupSetAttribute(button_box,"MARGIN","0x0");
  IupSetAttribute(button_box,"GAP","5");

  dlg->txtFileName = IupText(NULL);
  IupSetAttribute(dlg->txtFileName,"SIZE","200");

  dlg->lstFormat = IupList(NULL);
  IupSetAttribute(dlg->lstFormat,"SIZE","200");
  IupSetAttribute(dlg->lstFormat,"DROPDOWN","YES");
  IupSetCallback(dlg->lstFormat, "ACTION", (Icallback)cbSaveAsFormat);

  dlg->lstCompression = IupList(NULL);
  IupSetAttribute(dlg->lstCompression,"SIZE","200");
  IupSetAttribute(dlg->lstCompression,"DROPDOWN","YES");

  Ihandle* param_box = IupVbox(
    IupLabel("File Name:"),
    dlg->txtFileName,
    IupSetAttributes(IupFill(), "SIZE=5"),
    IupLabel("Format:"),
    dlg->lstFormat,
    IupSetAttributes(IupFill(), "SIZE=5"),
    IupLabel("Compression:"),
    dlg->lstCompression,
    NULL);

  Ihandle* dlg_box = IupHbox(
    IupFrame(param_box), 
    IupSetAttributes(IupFill(), "SIZE=5"),
    button_box, 
    NULL);
  IupSetAttribute(dlg_box,"MARGIN","10x10");
  IupSetAttribute(dlg_box,"GAP","5");

  dlg->dialog = IupDialog(dlg_box);

  IupSetAttribute(dlg->dialog,"DEFAULTENTER","btSaveOK");
  IupSetAttribute(dlg->dialog,"DEFAULTESC","btSaveCancel");
  IupSetAttribute(dlg->dialog,"TITLE","Save As");
  IupSetAttribute(dlg->dialog,"MINBOX","NO");
  IupSetAttribute(dlg->dialog,"MAXBOX","NO");
  IupSetAttribute(dlg->dialog,"RESIZE","NO");
  IupSetAttribute(dlg->dialog,"MENUBOX","NO");
  IupSetAttribute(dlg->dialog,"PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dlg->dialog,"SaveAsDialog", (char*)dlg);

  return dlg;
}

int imlabDlgImageFileSaveAs(imImage* image, char* filename, char* format, char* compression, int is_multi)
{
  /* Retrieve a file name */
  if (!imlabDlgGetSaveAsImageFileName(filename, is_multi? 2: 1))
   return 0;

  SaveAsDialog* dlg = CreateSaveAsDialog();
  char* ext = utlFileGetExt(filename);

  if (format[0] == 0)
    strcpy(dlg->saFormat, "TIFF");
  else
    strcpy(dlg->saFormat, format);

  strcpy(dlg->saCompression, compression);
  dlg->image = image;

  strcpy(dlg->saFileName, filename);
  IupStoreAttribute(dlg->txtFileName, "VALUE", dlg->saFileName);

  char* format_list[50];
  int format_count, ff = 1;
  imFormatList(format_list, &format_count);

  IupStoreAttribute(dlg->lstFormat, "VALUE", "1");

  for (int f = 0; f < format_count; f++)
  {
    char format_desc[50], format_ext[50];
    int can_sequence;
    imFormatInfo(format_list[f], format_desc, format_ext, &can_sequence);

    if (is_multi && !can_sequence)
      continue;

    int error = imFormatCanWriteImage(format_list[f], "", image->color_space, image->data_type);
    if (error)
      continue;

    char Str[3];
    sprintf(Str, "%d", ff);
    IupSetfAttribute(dlg->lstFormat, Str, "%s - %s", format_list[f], format_desc);
    ff++;

    if (ext && strstr(format_ext, ext))
      strcpy(dlg->saFormat, format_list[f]);

    if (imStrEqual(dlg->saFormat, format_list[f]))
      IupStoreAttribute(dlg->lstFormat, "VALUE", Str);
  }

  dlg->saReturn = 0;
  dlg->saMulti = is_multi;

  cbSaveAsFormat(dlg->dialog);

  /* Shows the save as dialog */
  IupPopup(dlg->dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);
  IupDestroy(dlg->dialog);

  if (dlg->saReturn == 0)
  {
    free(dlg);
    return 0;
  }

  strcpy(filename, dlg->saFileName);
  strcpy(format, dlg->saFormat);
  strcpy(compression, dlg->saCompression);

  free(dlg);

  return 1;
}

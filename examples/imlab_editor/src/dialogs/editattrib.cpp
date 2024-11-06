
#include <iup.h>
#include <iupcontrols.h>
#include <iupkey.h>
#include <im.h>
#include <im_binfile.h>
#include <cd.h>

#include "imagefile.h"
#include "imlab.h"
#include "utl_file.h"
#include "dialogs.h"
#include "im_imageview.h"

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>


static char* iAttribData2Str(const void* data, int data_type)
{
  static char str[100] = "";
  imImageViewData2Str(str, data, 0, data_type, 1);
  return str;
}

static int AttribIsString(int data_type, int count, const void* data)
{
  const imbyte* p = (imbyte*)data;
  if (p[count-1] == 0 && data_type == IM_BYTE)
  {
    while (p[count-1]==0 && count > 0) count--; /* skip all the zeros at the end */
    if (count == 0)
      return 0;  /* a buffer of zeros */

    int i = 0;
    while (p[i]) i++;
    if (i != count)
      return 0; /* there are zeros in the middle of the data */
    else
      return 1;
  }
  else
    return 0;
}

static int AttribCountSpaces(const char* data_str, int data_type)
{
  const char* pstr = data_str;
  int count = 0;
  while(*pstr != 0)
  {
    if (*pstr == ' ')
      count++;

    pstr++;
  }
  if (*(pstr-1) != ' ') count++;

  if (data_type == IM_CFLOAT || data_type == IM_CDOUBLE)
    count /= 2;
  return count;
}

static int change_attrib_func(Ihandle* dialog, int param_index, void* user_data)
{
  (void)user_data;

  if (param_index == 3) /* is_string */
  {
    Ihandle* data_type_param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
    Ihandle* is_string_param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
    int data_type = IupGetInt(data_type_param, "VALUE");
    int is_string = IupGetInt(is_string_param, "VALUE");
    if (data_type != IM_BYTE)
      return 0;

    Ihandle* data_param = (Ihandle*)IupGetAttribute(dialog, "PARAM4");
    Ihandle* data_ctrl = (Ihandle*)IupGetAttribute(data_param, "CONTROL");
    Ihandle* count_param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
    int count = IupGetInt(count_param, "VALUE");

    if (is_string) /* convert to string */
    {
      char value[4096] = "";
      char* data_str = IupGetAttribute(data_param, "VALUE");

      int offset = 0;
      for (int i = 0; i < count; i++)
      {
        offset += imImageViewStr2Data(data_str + offset, value, i, data_type);
      }

      IupStoreAttribute(data_param, "VALUE", value);
      IupStoreAttribute(data_ctrl, "VALUE", value);
    }
    else /* convert to binary values */
    {
      char data_str[4096] = "";
      char* value = IupGetAttribute(data_param, "VALUE");

      int offset = 0;
      for (int i = 0; i < count; i++)
      {
        offset += sprintf(data_str + offset, "%s ", 
                          iAttribData2Str(value + i*imDataTypeSize(data_type), data_type));

        if (offset > 4000) break;
      }

      IupStoreAttribute(data_param, "VALUE", data_str);
      IupStoreAttribute(data_ctrl, "VALUE", data_str);
    }
  }
  else if (param_index == 4)  /* data_str */
  {
    int count;
    Ihandle* count_param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
    Ihandle* count_ctrl = (Ihandle*)IupGetAttribute(count_param, "CONTROL");
    Ihandle* is_string_param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
    int is_string = IupGetInt(is_string_param, "VALUE");
    Ihandle* data_param = (Ihandle*)IupGetAttribute(dialog, "PARAM4");
    char* value = IupGetAttribute(data_param, "VALUE"); /* get the new value being typed */
    if (is_string) /* calculate string size */
      count = (int)strlen(value)+1;
    else /* count spaces */
    {
      Ihandle* data_type_param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
      int data_type = IupGetInt(data_type_param, "VALUE");
      count = AttribCountSpaces(value, data_type);
    }
    IupSetfAttribute(count_ctrl, "VALUE", "%d", count);
    IupSetfAttribute(count_param, "VALUE", "%d", count);
  }
  else if (param_index == 1)  /* data_type */
  {
    Ihandle* data_type_param = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
    Ihandle* is_string_param = (Ihandle*)IupGetAttribute(dialog, "PARAM3");
    int data_type = IupGetInt(data_type_param, "VALUE");
    int is_string = IupGetInt(is_string_param, "VALUE");
    if (is_string && data_type != IM_BYTE)
    {
      /* clear data if data_type not BYTE */
      Ihandle* data_param = (Ihandle*)IupGetAttribute(dialog, "PARAM4");
      Ihandle* data_ctrl = (Ihandle*)IupGetAttribute(data_param, "CONTROL");
      IupSetAttribute(data_param, "VALUE", "");
      IupSetAttribute(data_ctrl, "VALUE", "");

      Ihandle* count_param = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
      Ihandle* count_ctrl = (Ihandle*)IupGetAttribute(count_param, "CONTROL");
      IupSetAttribute(count_param, "VALUE", "0");
      IupSetAttribute(count_ctrl, "VALUE", "0");

      /* turn off is_string */
      Ihandle* is_string_ctrl = (Ihandle*)IupGetAttribute(is_string_param, "CONTROL");
      IupSetAttribute(is_string_param, "VALUE", "OFF");
      IupSetAttribute(is_string_ctrl, "VALUE", "OFF");
    }
  }
  else if (param_index == 2)  /* count */
    return 0; /* do not allow to change count */

  return 1;
}

static int imlabGetAttrib(char* name, int *data_type, int *count, void* *data)
{
  int is_string = 0;
  char data_str[4096] = "";

  if (*data)
  {
    is_string = AttribIsString(*data_type, *count, *data);
    if (is_string)
    {
      int size = *count;
      if (size > 4000) size = 4000;
      memcpy(data_str, *data, size);
      data_str[size-1] = 0;
      *count = size;
    }
    else
    {
      int offset = 0;
      for (int i = 0; i < *count; i++)
      {
        offset += sprintf(data_str + offset, "%s ", 
                          iAttribData2Str((imbyte*)(*data) + i*imDataTypeSize(*data_type), *data_type));

        if (offset > 4000) break;
      }
    }
  }

  if (!IupGetParam("Attribute", (Iparamcb)change_attrib_func, NULL,
                   "Name: %s\n"
                   "Data Type: %l|byte|short|ushort|int|float|double|cfloat|cdouble|\n"
                   "Count (N>1, read-only): %i\n"
                   "As String: %b\n"
                   " %t\n"
                   "Data: %m\n",
                   name, data_type, count, &is_string, data_str, NULL))
    return 0;

  if (is_string)
    *count = (int)strlen(data_str) + 1;
  else
    *count = AttribCountSpaces(data_str, *data_type);

  *data = malloc(imDataTypeSize(*data_type) * (*count));
  if (is_string)
  {
    memcpy(*data, data_str, *count); 
  }
  else
  {
    int offset = 0;
    for (int i = 0; i < *count; i++)
    {
      offset += imImageViewStr2Data(data_str + offset, *data, i, *data_type);
    }
  }

  return 1;
}

static void iAttribMakeString(imImage* image, char* name, char *AttribStr)
{
  int data_type, count;
  const void* data = imImageGetAttribute(image, name, &data_type, &count);

  if (count == 1)
    sprintf(AttribStr, "%s: %s", name, iAttribData2Str(data, data_type));
  else 
  {
    if (AttribIsString(data_type, count, data))
    {
      if (count > 150)
      {
        char old_byte = *((char*)data + 150);
        *((char*)data + 150) = 0;
        sprintf(AttribStr, "%s: %s", name, (char*)data);
        *((char*)data + 150) = old_byte;
      }
      else
        sprintf(AttribStr, "%s: %s", name, (char*)data);
    }
    else
    {
      int offset = sprintf(AttribStr, "%s: ", name);

      for (int i = 0; i < count; i++)
      {
        offset += sprintf(AttribStr + offset, "%s ", iAttribData2Str((imbyte*)data + i*imDataTypeSize(data_type), data_type));

        if (offset > 180) 
        {
          sprintf(AttribStr + offset, "...");
          break;
        }
      }
    }
  }
}

static int strEqualPartial(const char* name, const char* partial)
{
  while (*name && *partial)
  {
    if (*name != *partial)
      return 0;

    name++;
    partial++;
  }

  if (*partial != 0)
    return 0;

  return 1;
}

static void iAttribUpdateList(const imlabImageFile* ImageFile, Ihandle* iup_attrib_list)
{
  char* attrib[200];
  int attrib_count, aa;
  imImageGetAttributeList(ImageFile->image, attrib, &attrib_count);
  char op[5];

  aa = 1;
  for (int i = 0; i < attrib_count; i++)
  {
    if (!strEqualPartial(attrib[i], "ViewOptions"))
    {
      char AttribStr[200];
      iAttribMakeString(ImageFile->image, attrib[i], AttribStr);

      sprintf(op, "%d", aa);
      IupStoreAttribute(iup_attrib_list, op, AttribStr);

      aa++;
    }
  }

  sprintf(op,"%d", aa);
  IupSetAttribute(iup_attrib_list,op,NULL);

  IupSetAttribute(iup_attrib_list, "VALUE", "1");
}

static int cbAttribLog(Ihandle* self)
{
  imlabImageFile* ImageFile = (imlabImageFile*)IupGetAttribute(self, "imlabImageFile");
  char Msg[4096];

  sprintf(Msg, "File Name:\n"
               "   %s\n\n",
               ImageFile->filename);

  char* attrib[200];
  int attrib_count;
  imImageGetAttributeList(ImageFile->image, attrib, &attrib_count);

  for (int i = 0; i < attrib_count; i++)
  {
    if (!strEqualPartial(attrib[i], "ViewOptions"))
    {
      char AttribStr[200];
      iAttribMakeString(ImageFile->image, attrib[i], AttribStr);

      strcat(Msg, AttribStr);
      strcat(Msg, "\n");
    }
  }

  imlabLogMessagef(Msg);
  return IUP_DEFAULT;
}

static int cbAttribEdit(Ihandle* self)
{
  Ihandle* iup_attrib_list = (Ihandle*)IupGetAttribute(self, "AttribList");
  imlabImageFile* ImageFile = (imlabImageFile*)IupGetAttribute(self, "imlabImageFile");
  char attrib_name[50] = "", old_attrib_name[50] = "";
  int data_type = IM_BYTE, count = 0;
  void* data = NULL;

  char* attrib = IupGetAttribute(iup_attrib_list, IupGetAttribute(iup_attrib_list, "VALUE"));
  if (attrib)
  {
    char* aname = attrib_name;
    while (*attrib != ':')
      *aname++ = *attrib++;
    *aname = 0;

    data = (void*)imImageGetAttribute(ImageFile->image, attrib_name, &data_type, &count);
    strcpy(old_attrib_name, attrib_name);
  }

  if (imlabGetAttrib(attrib_name, &data_type, &count, &data))
  {
    if (!imStrEqual(attrib_name, old_attrib_name))
      imImageSetAttribute(ImageFile->image, old_attrib_name, 0, 0, NULL);

    imImageSetAttribute(ImageFile->image, attrib_name, data_type, count, data);

    char AttribStr[200];
    iAttribMakeString(ImageFile->image, attrib_name, AttribStr);
    IupSetAttribute(iup_attrib_list, IupGetAttribute(iup_attrib_list, "VALUE"), AttribStr);

    ImageFile->changed = 2;  /* no undo */
    IupSetAttribute(IupGetDialog(self), "CHANGED", "1");
    free(data);
  }

  return IUP_DEFAULT;
}

static int cbAttribAdd(Ihandle* self)
{
  Ihandle* iup_attrib_list = (Ihandle*)IupGetAttribute(self, "AttribList");
  imlabImageFile* ImageFile = (imlabImageFile*)IupGetAttribute(self, "imlabImageFile");

  char attrib_name[50] = "";
  int data_type = IM_BYTE, count = 0;
  void* data = NULL;

  if (imlabGetAttrib(attrib_name, &data_type, &count, &data))
  {
    imImageSetAttribute(ImageFile->image, attrib_name, data_type, count, data);

    char AttribStr[200];
    iAttribMakeString(ImageFile->image, attrib_name, AttribStr);
    IupSetAttribute(iup_attrib_list, "APPENDITEM", AttribStr);

    ImageFile->changed = 2;  /* no undo */
    IupSetAttribute(IupGetDialog(self), "CHANGED", "1");
    free(data);
  }

  return IUP_DEFAULT;
}

static int cbAttribRemove(Ihandle* self)
{
  Ihandle* iup_attrib_list = (Ihandle*)IupGetAttribute(self, "AttribList");
  imlabImageFile* ImageFile = (imlabImageFile*)IupGetAttribute(self, "imlabImageFile");
  char attrib_name[50];

  char* value = IupGetAttribute(iup_attrib_list, "VALUE");
  if (!value || value[0]==0) return IUP_DEFAULT;
  char* attrib = IupGetAttribute(iup_attrib_list, value);

  char* aname = attrib_name;
  while (*attrib != ':')
    *aname++ = *attrib++;
  *aname = 0;

  imImageSetAttribute(ImageFile->image, attrib_name, 0, 0, NULL);
  IupSetAttribute(iup_attrib_list, "REMOVEITEM", IupGetAttribute(iup_attrib_list, "VALUE"));
  ImageFile->changed = 2;  /* no undo */
  IupSetAttribute(IupGetDialog(self), "CHANGED", "1");

  return IUP_DEFAULT;
}

static int cbAttribClose(void)
{
  return IUP_CLOSE;
}

int imlabDlgImageFileEditAttrib(const char* filetitle, imlabImageFile* ImageFile)
{
  Ihandle *bt_edit, *bt_add, *bt_remove, *bt_log, *bt_close;

  bt_close = IupButton("Close", NULL);
  IupSetAttribute(bt_close,"SIZE","40");
  IupSetHandle("btAttribClose", bt_close);
  IupSetCallback(bt_close, "ACTION", (Icallback)cbAttribClose);

  bt_remove = IupButton("Remove", NULL);
  IupSetAttribute(bt_remove,"SIZE","40");
  IupSetCallback(bt_remove, "ACTION", (Icallback)cbAttribRemove);
  bt_add = IupButton("Add...", NULL);
  IupSetCallback(bt_add, "ACTION", (Icallback)cbAttribAdd);
  IupSetAttribute(bt_add, "SIZE", "40");
  bt_edit = IupButton("Edit...", NULL);
  IupSetAttribute(bt_edit,"SIZE","40");
  IupSetCallback(bt_edit, "ACTION", (Icallback)cbAttribEdit);
  bt_log = IupButton("Log", NULL);
  IupSetAttribute(bt_log,"SIZE","40");
  IupSetCallback(bt_log, "ACTION", (Icallback)cbAttribLog);

  Ihandle* bt_hbox = IupHbox(bt_edit, bt_add, bt_remove, bt_log, bt_close, NULL);

  Ihandle* attrib_list = IupList(NULL);
  IupSetAttribute(attrib_list,"SIZE","210x150");
  IupSetAttribute(attrib_list,"DROPDOWN","NO");
  IupSetAttribute(attrib_list,"SORT","YES");
  IupSetAttribute(attrib_list, "EXPAND","YES");
  IupSetCallback(attrib_list, "DBLCLICK_CB", cbAttribEdit);

  Ihandle* full_box = IupVbox(attrib_list, bt_hbox, NULL);
  IupSetAttribute(full_box,"MARGIN","10x10");
  IupSetAttribute(full_box,"GAP","5");

  Ihandle* dialog = IupDialog(full_box);

  IupSetAttribute(dialog, "DEFAULTENTER", "btAttribClose");
  IupSetAttribute(dialog, "DEFAULTESC", "btAttribClose");
  IupSetStrf(dialog, "TITLE","Image Attributes - %s", filetitle);
  IupSetAttribute(dialog, "MINBOX","NO");
  IupSetAttribute(dialog, "MAXBOX","NO");
  IupSetAttribute(dialog, "MENUBOX","NO");
  IupSetAttribute(dialog, "PARENTDIALOG","imlabMainWindow");
  IupSetAttribute(dialog, "AttribList", (char*)attrib_list);
  IupSetAttribute(dialog, "imlabImageFile", (char*)ImageFile);

  iAttribUpdateList(ImageFile, attrib_list);

  IupPopup(dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);

  int ret = IupGetInt(dialog, "CHANGED");

  IupDestroy(dialog);

  return ret;
}

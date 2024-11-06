
#include <iup.h>
#include <iupcontrols.h>
#include <iup_config.h>
#include <cd.h>
#include <cdiup.h>
#include <im.h>
#include <im_image.h>
#include <im_util.h>
#include <im_capture.h>
#include <im_counter.h>

#include "dialogs.h"
#include "im_imageview.h"
#include "imagecapture.h"
#include "imlab.h"
#include "counter_preview.h"

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>


struct ImageCapture
{
  imlabImageCaptureCallback capture_callback;
  imlabImageCaptureCallback preview_callback;
  imlabImageCaptureConfig config_callback;
  cdCanvas* cd_canvas;
  imImage* image;
  imVideoCapture* video_capture;
  Ihandle *deviceitem[20], *canvas, *live_item, 
          *disconnected_item, *marked_item, *capture_item, 
          *capturevideo_item, *dialog, *cfgdlg_item[6], 
          *imagesize_item, *imageattrib_item, *videoformat_item;
  int capture_on,
      init_state,
      num_device;
  void* user_data;
};

#define MAX_CAP 4
static ImageCapture capture_window[MAX_CAP];

typedef int (*Iidlecallback)(void);
static Iidlecallback OldIdle_cb = NULL;

static ImageCapture* GetFreeCaptureWindow(void)
{
  int i;
  for (i = 0; i < MAX_CAP; i++)
  {
    if (capture_window[i].init_state == -1)
      return &capture_window[i];
  }
  for (i = 0; i < MAX_CAP; i++)
  {
    if (capture_window[i].init_state == 0)
      return &capture_window[i];
  }
  return NULL;
}

static int CheckOneCaptureWindow(void)
{
  int i, win = 0;
  for (i = 0; i < MAX_CAP; i++)
  {
    if (capture_window[i].init_state == 1)
      win++;
  }
  return (win == 1)? 1: 0;
}

static void PaintAllCanvas(ImageCapture* capture)
{
  int posx, posy, canvas_width, canvas_height;
  cdCanvasActivate(capture->cd_canvas);

  cdCanvasGetSize(capture->cd_canvas, &canvas_width, &canvas_height, NULL, NULL);

  if (!capture->image)
  {
    cdCanvasBackground(capture->cd_canvas, CD_WHITE);
    cdCanvasClear(capture->cd_canvas);
    return;
  }

  cdCanvasForeground(capture->cd_canvas, CD_WHITE);

  posx = (canvas_width - capture->image->width)/2;
  posy = (canvas_height - capture->image->height)/2;

  /* white backgroud around the view area */
  /* Checks if there are margins between cd_canvas border and image border */
  if (posy - 1 > 0) cdCanvasBox(capture->cd_canvas, 0, canvas_width, 0, posy - 1);
  if (posy + capture->image->height < canvas_height) cdCanvasBox(capture->cd_canvas, 0, canvas_width, posy + capture->image->height, canvas_height);

  if (posx - 1 > 0) cdCanvasBox(capture->cd_canvas, 0, posx - 1, 0, canvas_height);
  if (posx + capture->image->width < canvas_width) cdCanvasBox(capture->cd_canvas, posx + capture->image->width, canvas_width, 0, canvas_height);

  imImageViewDrawImage(capture->cd_canvas, capture->image, posx, posy, 0, 0);
}

static void PaintCanvas(ImageCapture* capture)
{
  int posx, posy, canvas_width, canvas_height;
  cdCanvasActivate(capture->cd_canvas);

  cdCanvasGetSize(capture->cd_canvas, &canvas_width, &canvas_height, NULL, NULL);

  posx = (canvas_width - capture->image->width)/2;
  posy = (canvas_height - capture->image->height)/2;

  imImageViewDrawImage(capture->cd_canvas, capture->image, posx, posy, 0, 0);
}

static void UpdateBitSize(ImageCapture* capture, int width, int height)
{
  double color = 1;
  int color_mode;
  imVideoCaptureGetAttribute(capture->video_capture, "VideoColorEnable", &color);
  color_mode = color? IM_RGB: IM_GRAY;

  if (!capture->image)
  {
    capture->image = imImageCreate(width, height, color_mode, IM_BYTE);
  }
  else
  {
    /* this compare will allow us to change from IM_MAP to IM_GRAY in the callbacks */
    if ((color_mode == IM_RGB && capture->image->color_space != IM_RGB) ||
        (color_mode != IM_RGB && capture->image->color_space == IM_RGB))
    {
      imImageDestroy(capture->image);
      capture->image = imImageCreate(width, height, color_mode, IM_BYTE);

      cdCanvasActivate(capture->cd_canvas);
      cdCanvasClear(capture->cd_canvas);
    }

    if (capture->image->width != width || 
        capture->image->height != height)
    {
      imImageReshape(capture->image, width, height);

      cdCanvasActivate(capture->cd_canvas);
      cdCanvasClear(capture->cd_canvas);
    }
  }
}

static int ImageCaptureUpdate(Ihandle* self)
{
  int width, height, device;
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  if (!capture->video_capture)
    return IUP_DEFAULT;

  device = imVideoCaptureConnect(capture->video_capture, -1);
  if (device == -1)
  {
    if (capture->marked_item != capture->disconnected_item)
    {
      IupSetAttribute(capture->videoformat_item, "ACTIVE", "NO");
      IupSetAttribute(capture->imagesize_item, "ACTIVE", "NO");
      IupSetAttribute(capture->imageattrib_item, "ACTIVE", "NO");
      IupSetAttribute(capture->live_item, "ACTIVE", "NO");
      IupSetAttribute(capture->capture_item, "ACTIVE", "NO");
      IupSetAttribute(capture->capturevideo_item, "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[0], "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[1], "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[2], "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[3], "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[4], "ACTIVE", "NO");
      IupSetAttribute(capture->cfgdlg_item[5], "ACTIVE", "NO");

      IupSetAttribute(capture->marked_item, "VALUE", "OFF");
      IupSetAttribute(capture->disconnected_item, "VALUE", "ON");
      capture->marked_item = capture->disconnected_item;
    }

    PaintAllCanvas(capture);
    return IUP_DEFAULT;
  }

  imVideoCaptureGetImageSize(capture->video_capture, &width, &height);
  if (width == 0 || height == 0)
  {
    PaintAllCanvas(capture);
    return IUP_DEFAULT;
  }

  UpdateBitSize(capture, width, height);

  IupSetAttribute(capture->videoformat_item, "ACTIVE", "YES");
  IupSetAttribute(capture->imagesize_item, "ACTIVE", "YES");
  IupSetAttribute(capture->imageattrib_item, "ACTIVE", "YES");
  IupSetAttribute(capture->live_item, "ACTIVE", "YES");
  IupSetAttribute(capture->capture_item, "ACTIVE", "YES");
  IupSetAttribute(capture->capturevideo_item, "ACTIVE", "YES");

  IupSetAttribute(capture->marked_item, "VALUE", "OFF");
  IupSetAttribute(capture->deviceitem[device], "VALUE", "ON");
  capture->marked_item = capture->deviceitem[device];

  {
    int dialog = 0;
    const char* dialog_desc = imVideoCaptureDialogDesc(capture->video_capture, dialog);
    while (dialog_desc)
    {
      IupSetAttribute(capture->cfgdlg_item[dialog], "TITLE", (char*)dialog_desc);
      IupSetAttribute(capture->cfgdlg_item[dialog], "ACTIVE", "YES");
      dialog++;
      dialog_desc = imVideoCaptureDialogDesc(capture->video_capture, dialog);
    }

    while(dialog < 6)
    {
      IupSetAttribute(capture->cfgdlg_item[dialog], "TITLE", "(inactive)");
      IupSetAttribute(capture->cfgdlg_item[dialog], "ACTIVE", "NO");
      dialog++;
    }
  }

  PaintAllCanvas(capture);
  return IUP_DEFAULT;
}

static int VideoFormat_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int num_formats, i, ret, init;
  char* formats_str[100];

  num_formats = imVideoCaptureFormatCount(capture->video_capture);
  if (!num_formats)
  {
    IupMessage("Error!", "Video Formats not supported.");
    return IUP_DEFAULT;
  }

  if (num_formats > 100)
    num_formats = 100;

  for (i = 0; i < num_formats; i++)
  {
    int width, height;
    char desc[10];
    imVideoCaptureGetFormat(capture->video_capture, i, &width, &height, desc);
    formats_str[i] = (char*)malloc(30);
    sprintf(formats_str[i], "%dx%d - %s", width, height, desc);
  }

  init = imVideoCaptureSetFormat(capture->video_capture, -1);
  if (init == -1) 
    init = 0;

  ret = IupListDialog(1, "Video Formats", num_formats, (const char **)formats_str, init + 1, 30, 7, NULL);

  for (i = 0; i < num_formats; i++)
    free(formats_str[i]);

  if (ret != -1)
  {
    if (!imVideoCaptureSetFormat(capture->video_capture, ret))
      IupMessage("Error!", "Video Format failed.");

    ImageCaptureUpdate(self);
  }

  return IUP_DEFAULT;
}

static int ImageSize_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int width = IupConfigGetVariableIntDef(imlabConfig(), "Capture", "Width", 320);
  int height = IupConfigGetVariableIntDef(imlabConfig(), "Capture", "Height", 240);

  imVideoCaptureGetImageSize(capture->video_capture, &width, &height);

  if (IupGetParam("Image Size", NULL, NULL,
                  "Width: %i[1,]\n"
                  "Height: %i[1,]\n",
                  &width, &height, NULL))
  {
    if (!imVideoCaptureSetImageSize(capture->video_capture, width, height))
      IupMessage("Error!", "Size failed.");
    else
    {
      IupConfigSetVariableInt(imlabConfig(), "Capture", "Width", width);
      IupConfigSetVariableInt(imlabConfig(), "Capture", "Height", height);
    }

    ImageCaptureUpdate(self);
  }

  return IUP_DEFAULT;
}

static char* get_attrib_name(Ihandle* dialog)
{
  char list_attrib[20];
  Ihandle* param0 = (Ihandle*)IupGetAttribute(dialog, "PARAM0");
  Ihandle* ctrl0 = (Ihandle*)IupGetAttribute(param0, "CONTROL");
  int item = IupGetInt(param0, "VALUE"); /* param list value, 0 based */
  sprintf(list_attrib, "%d", item+1);      /* IupList value, 1 based */
  return IupGetAttribute(ctrl0, list_attrib);
}

static int change_attrib(Ihandle* dialog, int param_index, void* user_data)
{
  imVideoCapture* cur_video_capture = (imVideoCapture*)user_data;

  if (param_index < 0)
    return 1;

  switch (param_index)
  {
  case 0: /* attrib. list */
    {
      double value = 0;
      Ihandle* param1 = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
      Ihandle* ctrl1 = (Ihandle*)IupGetAttribute(param1, "CONTROL");
      Ihandle* auxctrl = (Ihandle*)IupGetAttribute(param1, "AUXCONTROL");
      imVideoCaptureGetAttribute(cur_video_capture, get_attrib_name(dialog), &value);
      IupSetDouble(ctrl1, "VALUE", value);
      IupSetDouble(auxctrl, "VALUE", value);
      IupSetDouble(param1, "VALUE", value);
      break;
    }
  case 1: /* value */
    {
      Ihandle* param2 = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
      int apply = IupGetInt(param2, "VALUE");
      if (apply)
      {
        Ihandle* param1 = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
        double value = IupGetDouble(param1, "VALUE");
        imVideoCaptureSetAttribute(cur_video_capture, get_attrib_name(dialog), value);
      }
      break;
    }
  case 2: /* apply changes */
    {
      Ihandle* param2 = (Ihandle*)IupGetAttribute(dialog, "PARAM2");
      int apply = IupGetInt(param2, "VALUE");
      if (apply)
      {
        Ihandle* param1 = (Ihandle*)IupGetAttribute(dialog, "PARAM1");
        double value = IupGetDouble(param1, "VALUE");
        imVideoCaptureSetAttribute(cur_video_capture, get_attrib_name(dialog), value);
      }
      break;
    }
  }

  return 1;
}

static int ImageAttrib_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int attrib_index = 0, apply = 0;
  double value = 0;
  char format[4096];  
  int i, offset, attrib_count;
  const char** attrib_list = imVideoCaptureGetAttributeList(capture->video_capture, &attrib_count);

  offset = sprintf(format, "Name: %%l|");
  for (i = 0; i < attrib_count; i++)
  {
    if (imVideoCaptureGetAttribute(capture->video_capture, attrib_list[i], &value))
      offset += sprintf(format+offset, "%s|", attrib_list[i]);
  }
  offset += sprintf(format+offset, "\nValue: %%R[0,100]\n");
  sprintf(format+offset, "Apply Changes: %%b\n");

  value = 0;
  imVideoCaptureGetAttribute(capture->video_capture, attrib_list[attrib_index], &value);

  if (!IupGetParam("Image Attrib", change_attrib, capture->video_capture, format, &attrib_index, &value, &apply, NULL))
    return IUP_DEFAULT;

  imVideoCaptureSetAttribute(capture->video_capture, attrib_list[attrib_index], value);

  return IUP_DEFAULT;
}

static int Live_cb(Ihandle* self);

static int Device_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int connected = 0;
  int new_device = IupGetInt(self, "CaptureDeviceIndex");
  int old_device = imVideoCaptureConnect(capture->video_capture, -1);

  if (new_device == old_device)
  {
    if (capture->config_callback)
      capture->config_callback(capture->user_data, capture->video_capture);

    return IUP_DEFAULT;
  }

  if (new_device == -1)
    imVideoCaptureDisconnect(capture->video_capture);
  else
    connected = imVideoCaptureConnect(capture->video_capture, new_device);

  if (!connected)
  {
    if (capture->image) imImageClear(capture->image);
    if (new_device != -1)
      IupMessage("Error!", "Capture Connect Failed.");
  }

  if (connected && capture->config_callback)
    capture->config_callback(capture->user_data, capture->video_capture);

  ImageCaptureUpdate(self);

  if (connected)
    Live_cb(self);

  return IUP_DEFAULT;
}

static void ProcessSaveFrame(ImageCapture* capture, int timeout, imFile* video_file, const char* filename)
{
  int device;
  if (!imVideoCaptureFrame(capture->video_capture, (imbyte*)capture->image->data[0], capture->image->color_space, timeout))
    return;

  device = imVideoCaptureConnect(capture->video_capture, -1);

  if (capture->preview_callback)
  {
    capture->capture_on = capture->preview_callback(capture->user_data, capture->image, device);
    if (capture->capture_on) capture->capture_on = -1;
  }

  if (capture->capture_on)
  {
    int error;
    imFileWriteImageInfo(video_file, capture->image->width, capture->image->height, capture->image->color_space, IM_BYTE);
    error = imFileWriteImageData(video_file, capture->image->data[0]);
    if (error)
    {
      imlabDlgFileErrorMsg("Capture Video", error, filename);
      capture->capture_on = 0;
    }
  }

  PaintCanvas(capture);
}

static int CaptureVideo_cb(Ihandle* self)
{
  int error;
  char video_filename[1024] = "*.*";
  char format[30] = "", compression[50] = "";
  imFile* video_file;

  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int old_live;
  if (capture->capture_on == 1)
    return IUP_DEFAULT;

  if (capture->capture_on == -1)
  {
    capture->capture_on = 0;
    return IUP_DEFAULT;
  }

  old_live = imVideoCaptureLive(capture->video_capture, -1);
  if (!old_live)
  {
    if (!imVideoCaptureLive(capture->video_capture, 1))
    {
      IupMessage("Error!", "Live failed.");
      return IUP_DEFAULT;
    }
  }

  if (!imlabDlgImageFileSaveAs(capture->image, video_filename, format, compression, 1))
    return IUP_DEFAULT;

  imlabCounterEnable(0);

  video_file = imFileNew(video_filename, format, &error);
  if (!video_file)
  {
    imlabDlgFileErrorMsg("Capture Video", error, video_filename);
    imlabCounterEnable(1);
    return IUP_DEFAULT;
  }
  else
  {
    imFileSetInfo(video_file, compression);
    imlabLogMessagef("Video File Started.");
  }

  capture->capture_on = -1;
  while (capture->capture_on)
  {
    ProcessSaveFrame(capture, 1000, video_file, video_filename);
    IupFlush();
  }

  imFileClose(video_file);
  imlabLogMessagef("Video File Saved.");

  imlabCounterEnable(1);

  if (!old_live)
    imVideoCaptureLive(capture->video_capture, 0);

  return IUP_DEFAULT;
}

static void ProcessCaptureFrame(ImageCapture* capture, int timeout)
{
  int device;
  if (!imVideoCaptureFrame(capture->video_capture, (imbyte*)capture->image->data[0], capture->image->color_space, timeout))
    return;

  device = imVideoCaptureConnect(capture->video_capture, -1);

  if (capture->preview_callback)
    capture->capture_on = capture->preview_callback(capture->user_data, capture->image, device);

  if (capture->capture_on)
  {
    capture->capture_on = capture->capture_callback(capture->user_data, capture->image, device);
    IupSetFocus(capture->dialog);
  }
}

static int Capture_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int old_live;
  if (capture->capture_on == -1)
    return IUP_DEFAULT;

  if (capture->capture_on == 1)
  {
    capture->capture_on = 0;
    return IUP_DEFAULT;
  }

  old_live = imVideoCaptureLive(capture->video_capture, -1);
  if (!old_live)
  {
    if (!imVideoCaptureLive(capture->video_capture, 1))
    {
      IupMessage("Error!", "Live failed.");
      return IUP_DEFAULT;
    }
  }

  capture->capture_on = 1;
  while (capture->capture_on)
  {
    ProcessCaptureFrame(capture, 1000);
    IupFlush();
  }

  if (!old_live)
    imVideoCaptureLive(capture->video_capture, 0);

  return IUP_DEFAULT;
}

static void ProcessCapturePreview(ImageCapture* capture)
{
  if (!capture->image || !imVideoCaptureFrame(capture->video_capture, (imbyte*)capture->image->data[0], capture->image->color_space, 0))
    return;

  if (capture->preview_callback)
    capture->preview_callback(capture->user_data, capture->image, imVideoCaptureConnect(capture->video_capture, -1));

  PaintCanvas(capture);
}

static int Idle_cb(void)
{
  int i;
  for(i = 0; i < MAX_CAP; i++)
  {
    ImageCapture* capture = &capture_window[i];
    if (capture->init_state == 1)
    {
      if (!imVideoCaptureLive(capture->video_capture, -1))
        continue;

      ProcessCapturePreview(capture);
    }
  }

  if (OldIdle_cb)
    OldIdle_cb();

  return IUP_DEFAULT;
}

static int Live_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");

  if (imVideoCaptureLive(capture->video_capture, -1))
  {
    imVideoCaptureLive(capture->video_capture, 0);
    return IUP_DEFAULT;
  }

  if (!imVideoCaptureLive(capture->video_capture, 1))
    IupMessage("Error!", "Live failed.");

  return IUP_DEFAULT;
}

static int Timer_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");

  if (!imVideoCaptureLive(capture->video_capture, -1))
  {
    IupSetAttribute(self, "RUN", "NO");
    return IUP_DEFAULT;
  }

  ProcessCapturePreview(capture);

  return IUP_DEFAULT;
}

static int CaptureCfgDialog_cb(Ihandle* self)
{
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");
  int ret, dialog = IupGetInt(self, "CaptureDlgIndex");

  Ihandle* timer = IupTimer();
  IupSetAttribute(timer, "TIME", "100");
  IupSetCallback(timer, "ACTION_CB", (Icallback)Timer_cb);
  IupSetAttribute(timer, "RUN", "YES");
  IupSetAttribute(timer, "CaptureWindow", (char*)capture);

  ret = imVideoCaptureShowDialog(capture->video_capture, dialog, (void*)IupGetAttribute(capture->dialog, "WID"));

  IupSetAttribute(timer, "RUN", "NO");
  ImageCaptureUpdate(self);
  IupDestroy(timer);

  if (!ret)
    IupMessage("Error!", "Dialog failed.");

  return IUP_DEFAULT;
}

static int Close_cb(Ihandle* self)
{
  int device;
  ImageCapture* capture = (ImageCapture*)IupGetAttribute(self, "CaptureWindow");

  if (CheckOneCaptureWindow())
    IupSetFunction("IDLE_ACTION", (Icallback)OldIdle_cb);

  device = imVideoCaptureConnect(capture->video_capture, -1);
  if (device != -1)
  {
    capture->marked_item = capture->disconnected_item;
    IupSetAttribute(capture->deviceitem[device], "VALUE", "OFF");
    IupSetAttribute(capture->disconnected_item, "VALUE", "ON");
    imVideoCaptureDisconnect(capture->video_capture);
  }

  capture->capture_on = 0;

  if (capture->image) imImageDestroy(capture->image);
  capture->image = NULL;

  if (capture->config_callback)
    capture->config_callback(capture->user_data, NULL);

  if (capture->video_capture) imVideoCaptureDestroy(capture->video_capture);
  capture->init_state = -1; // free but initialized
  IupHide(capture->dialog);

  return IUP_DEFAULT;
}

void imlabImageCaptureImageShow(const char* title, void* user_data, imlabImageCaptureCallback _capture_callback, 
                                                              imlabImageCaptureCallback _preview_callback, 
                                                              imlabImageCaptureConfig _config_callback)
{
  Ihandle *iup_menu, 
          *settings_menu,
          *device_menu;
  int device;
  const char* driver_desc;
  ImageCapture* capture;
  char menu_str[50];

  {
    static int first = 1;
    if (first)
    {
      memset(capture_window, 0, sizeof(ImageCapture)*MAX_CAP);
      first = 0;
    }
  }

  capture = GetFreeCaptureWindow();
  if (!capture)
  {
     IupMessage("Error!", "Too many capture windows.");
     return;
  }

  if (capture->init_state == -1)
  {
    capture->video_capture = imVideoCaptureCreate();
    capture->capture_callback = _capture_callback;
    capture->preview_callback = _preview_callback;
    capture->config_callback = _config_callback;
    capture->init_state = 1;
    capture->user_data = user_data;

    if (CheckOneCaptureWindow())
    {
      OldIdle_cb = (Iidlecallback)IupGetFunction("IDLE_ACTION");
      IupSetFunction("IDLE_ACTION", (Icallback)Idle_cb);
    }

    IupSetfAttribute(capture->dialog, "TITLE", "Capture - %s", title);

    IupShowXY(capture->dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);
    ImageCaptureUpdate(capture->dialog);

    return;
  }

  device = 0;
  driver_desc = imVideoCaptureDeviceDesc(device);
  device_menu = IupMenu(NULL);
  while (driver_desc != NULL)
  {
    capture->deviceitem[device] = imlabItem((char*)driver_desc, NULL);
    IupSetCallback(capture->deviceitem[device], "ACTION", (Icallback)Device_cb);
    IupAppend(device_menu, capture->deviceitem[device]);
    IupSetfAttribute(capture->deviceitem[device], "CaptureDeviceIndex", "%d", device);

    device++;
    driver_desc = imVideoCaptureDeviceDesc(device);
  }
  capture->num_device = device;

  if (capture->num_device == 0)
  {
     IupMessage("Error!", "No Capture Device Found.");
     return;
  }

  IupAppend(device_menu, IupSeparator());
  IupAppend(device_menu, capture->disconnected_item = imlabItem("Disconnected", NULL));
  IupSetCallback(capture->disconnected_item, "ACTION", (Icallback)Device_cb);
  IupSetfAttribute(capture->disconnected_item, "CaptureDeviceIndex", "%d", (int)-1);
  IupSetAttribute(capture->disconnected_item, "VALUE", "ON");
  capture->marked_item = capture->disconnected_item;

  settings_menu = IupMenu(
    capture->imagesize_item = imlabItem("Image Size...", NULL),
    capture->imageattrib_item = imlabItem("Image Attrib...", NULL),
    capture->videoformat_item = imlabItem("Video Format...", NULL),
    IupSeparator(),
    capture->cfgdlg_item[0] = imlabItem("(inactive)", NULL),
    capture->cfgdlg_item[1] = imlabItem("(inactive)", NULL),
    capture->cfgdlg_item[2] = imlabItem("(inactive)", NULL),
    capture->cfgdlg_item[3] = imlabItem("(inactive)", NULL),
    capture->cfgdlg_item[4] = imlabItem("(inactive)", NULL),
    capture->cfgdlg_item[5] = imlabItem("(inactive)", NULL),
    NULL);

  IupSetCallback(capture->imagesize_item, "ACTION", (Icallback)ImageSize_cb);
  IupSetCallback(capture->imageattrib_item, "ACTION", (Icallback)ImageAttrib_cb);
  IupSetCallback(capture->videoformat_item, "ACTION", (Icallback)VideoFormat_cb);

  IupSetCallback(capture->cfgdlg_item[0], "ACTION", (Icallback)CaptureCfgDialog_cb);
  IupSetCallback(capture->cfgdlg_item[1], "ACTION", (Icallback)CaptureCfgDialog_cb);
  IupSetCallback(capture->cfgdlg_item[2], "ACTION", (Icallback)CaptureCfgDialog_cb);
  IupSetCallback(capture->cfgdlg_item[3], "ACTION", (Icallback)CaptureCfgDialog_cb);
  IupSetCallback(capture->cfgdlg_item[4], "ACTION", (Icallback)CaptureCfgDialog_cb);
  IupSetCallback(capture->cfgdlg_item[5], "ACTION", (Icallback)CaptureCfgDialog_cb);

  IupSetfAttribute(capture->cfgdlg_item[0], "CaptureDlgIndex", "%d", 0);
  IupSetfAttribute(capture->cfgdlg_item[1], "CaptureDlgIndex", "%d", 1);
  IupSetfAttribute(capture->cfgdlg_item[2], "CaptureDlgIndex", "%d", 2);
  IupSetfAttribute(capture->cfgdlg_item[3], "CaptureDlgIndex", "%d", 3);
  IupSetfAttribute(capture->cfgdlg_item[4], "CaptureDlgIndex", "%d", 4);
  IupSetfAttribute(capture->cfgdlg_item[5], "CaptureDlgIndex", "%d", 5);

  capture->live_item = imlabItem("[Live/Freeze]", NULL);
  IupSetCallback(capture->live_item, "ACTION", (Icallback)Live_cb);
  capture->capture_item = imlabItem("[Capture]", NULL);
  IupSetCallback(capture->capture_item, "ACTION", (Icallback)Capture_cb);
  capture->capturevideo_item = imlabItem("[Capture Video]", NULL);
  IupSetCallback(capture->capturevideo_item, "ACTION", (Icallback)CaptureVideo_cb);

  IupSetAttribute(capture->videoformat_item, "ACTIVE", "NO");
  IupSetAttribute(capture->imagesize_item, "ACTIVE", "NO");
  IupSetAttribute(capture->imageattrib_item, "ACTIVE", "NO");
  IupSetAttribute(capture->live_item, "ACTIVE", "NO");
  IupSetAttribute(capture->capture_item, "ACTIVE", "NO");
  IupSetAttribute(capture->capturevideo_item, "ACTIVE", "NO");

  iup_menu = IupMenu(
    imlabSubmenu("Device", device_menu),
    imlabSubmenu("Settings", settings_menu),
    capture->live_item,
    capture->capture_item,
    capture->capturevideo_item,
    IupSetCallbacks(imlabItem("[Close]", NULL), "ACTION", (Icallback)Close_cb, NULL),
    NULL);

  {
    static int num_cap = 0;
    sprintf(menu_str, "ImageCaptureMenu%d", num_cap);
    num_cap++;
  }
  IupSetHandle(menu_str, iup_menu);

  capture->canvas = IupCanvas(NULL);
  capture->dialog = IupDialog(IupVbox(IupFill(),capture->canvas,IupFill(),NULL));

  IupSetAttribute(capture->dialog, "MENU", menu_str);
  IupSetCallback(capture->dialog, "CLOSE_CB", (Icallback)Close_cb);
  IupSetAttribute(capture->dialog, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(capture->dialog, "ICON", "IMLAB");
  IupSetAttribute(capture->dialog, "CaptureWindow", (char*)capture);
  IupSetfAttribute(capture->dialog, "TITLE", "Capture - %s", title);

  IupSetAttribute(capture->canvas, "RASTERSIZE", "640x480");
  IupSetCallback(capture->canvas, "ACTION", (Icallback)ImageCaptureUpdate);

  IupMap(capture->dialog);

  IupSetAttribute(capture->canvas, "USERSIZE", NULL);  /* clear minimum restriction */

  capture->cd_canvas = cdCreateCanvas(CD_IUP, capture->canvas);

  capture->video_capture = imVideoCaptureCreate();

  if (!capture->video_capture)
  {
    IupMessage("Error!", "Error creating Capture Graph. Check DirectX Version (>=9).");
    IupDestroy(capture->dialog);
    return;
  }

  capture->capture_callback = _capture_callback;
  capture->preview_callback = _preview_callback;
  capture->config_callback = _config_callback;
  capture->init_state = 1;
  capture->user_data = user_data;

  if (CheckOneCaptureWindow())
  {
    OldIdle_cb = (Iidlecallback)IupGetFunction("IDLE_ACTION");
    IupSetFunction("IDLE_ACTION", (Icallback)Idle_cb);
  }

  IupShowXY(capture->dialog, IUP_CENTERPARENT, IUP_CENTERPARENT);
}

static int check_device(Ihandle* dialog, int param_index, void* user_data)
{
  int check_device_count = *(int*)user_data;
  if (param_index < 0)
    return 1;

  if (param_index == 2)
  {
    char* device_str = IupGetAttribute((Ihandle*)IupGetAttribute(dialog, "PARAM2"), "VALUE");
    if ((int)strlen(device_str) != check_device_count)
      return 0;
  }

  return 1;
}

void imlabImageCaptureAll(void* user_data, imlabImageCaptureCallback capture_callback)
{
  int device, num_device = 0;
  int width = IupConfigGetVariableIntDef(imlabConfig(), "Capture", "Width", 320);
  int height = IupConfigGetVariableIntDef(imlabConfig(), "Capture", "Height", 240);
  char device_list[30];
  imVideoCapture* video_capture;
  imImage* image;

  if (num_device == 0)
  {
    device = 0;
    while (imVideoCaptureDeviceDesc(device) != NULL)
    {
      if (strstr(imVideoCaptureDeviceDesc(device), "(VFW)") != NULL)
        device_list[device] = '0';
      else
        device_list[device] = '1';

      device++;
    }
    num_device = device;
    device_list[num_device] = 0;
  }

  if (num_device == 0)
  {
     IupMessage("Error!", "No Capture Device Found.");
     return;
  }

  if (!IupGetParam("Capture Options", check_device, &num_device,
                   "Width: %i[1,]\n"
                   "Height: %i[1,]\n"
                   "Device List (bitmask): %s[0-1]+\n",
                   &width, &height, device_list, NULL))
    return;

  image = imImageCreate(width, height, IM_RGB, IM_BYTE);

  video_capture = imVideoCaptureCreate();

  for(device = 0; device < num_device; device++)
  {
    if (device_list[device] == '0')
      continue;

    if (!imVideoCaptureConnect(video_capture, device))
    {
      IupMessage("Error!", "Capture Connect Failed.");
      continue;
    }
    
    if (!imVideoCaptureSetImageSize(video_capture, width, height))
    {
      IupMessage("Error!", "Size failed.");
      continue;
    }

    IupConfigSetVariableInt(imlabConfig(), "Capture", "Width", width);
    IupConfigSetVariableInt(imlabConfig(), "Capture", "Height", height);

    imVideoCaptureSetAttribute(video_capture, "FlipHorizontal", 100);
    imVideoCaptureResetAttribute(video_capture, "VideoBrightness", 0);
    imVideoCaptureResetAttribute(video_capture, "VideoContrast", 0);

    if (!imVideoCaptureOneFrame(video_capture, (imbyte*)image->data[0], IM_RGB))
    {
      IupMessage("Error!", "Capture frame failed.");
      continue;
    }

    capture_callback(user_data, image, device);
    imImageClear(image);
  }

  imVideoCaptureDisconnect(video_capture);
  imImageDestroy(image);
}

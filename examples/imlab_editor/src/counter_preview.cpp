#include <iup.h>
#include <cd.h>
#include <im.h>
#include <im_counter.h>

#include "imlab.h"
#include "plugin_process.h"
#include "imagedocument.h"
#include "counter.h"
#include "im_imageview.h"
#include "dialogs.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


static Ihandle* counter_window = NULL;


static int cmCounterCancel(Ihandle* ih)
{
  (void)ih;

  int ret = imlabDlgQuestion("Interrupt Processing?", 0);
  if (ret == 1) /* Yes Interrupt */
    return IUP_DEFAULT;

  return IUP_CONTINUE;
}

static int imlabCounter(int counter, void* user_data, const char* text, int progress)
{
  (void)user_data; /* Not using */

  //  if (text || progress < 0 || progress > 1000)
  //    imlabLogMessagef("imlabCounter(%d, %s, %d)", counter, text, progress);

  if (progress == -1) // CounterBegin
  {
    // update description
    if (counter == 0)
    {
      IupSetStrf(counter_window, "DESC_MAIN", "%s\n", text);  /* main description */

      IupSetStrf(counter_window, "DESCRIPTION", "%s\n\n\n", text);
    }
    else
    {
      IupSetStrf(counter_window, "DESC_SEC", "%s\n", text);  /* secondary optional description */

      char* desc_main = IupGetAttribute(counter_window, "DESC_MAIN");
      IupSetStrf(counter_window, "DESCRIPTION", "%s%s\n\n", desc_main, text);
    }

    IupSetAttribute(counter_window, "STATE", NULL);

    // Only for the main counter
    if (counter == 0)
    {
      Ihandle* main_window = IupGetHandle("imlabMainWindow");
      IupSetAttribute(main_window, "TASKBARPROGRESSSTATE", "NORMAL");
      IupSetAttribute(main_window, "TASKBARPROGRESSVALUE", "0");

      IupShowXY(counter_window, IUP_CENTERPARENT, IUP_CENTERPARENT);

      IupSetAttribute(counter_window, "SIMULATEMODAL", "Yes");

      imlabLogMessagef("  Counter Start (%s)", text);
      ctTimerStart();
    }
    else
      imlabLogMessagef("    Sub-Counter Start (%s)", text);

    return 1;
  }

  if (progress == 1001) // CounterEnd
  {
    // force one last update
    IupSetInt(counter_window, "COUNT", 1000);

    // Only for the main counter
    if (counter == 0)
    {
      imlabLogMessagef("  Counter Stop. Total Time = [%s]", ctTimerCount());

      IupSetAttribute(counter_window, "SIMULATEMODAL", "No");

      IupHide(counter_window);

      Ihandle* main_window = IupGetHandle("imlabMainWindow");
      IupSetAttribute(main_window, "TASKBARPROGRESSVALUE", "100");
      IupSetAttribute(main_window, "TASKBARPROGRESSSTATE", "NOPROGRESS");

      IupSetAttribute(counter_window, "DESCRIPTION", "\n\n\n");
      IupSetAttribute(counter_window, "DESC_MAIN", NULL);
      IupSetAttribute(counter_window, "DESC_SEC", NULL);
    }
    else
      imlabLogMessagef("    Sub-Counter Stop.");

    return 1;
  }

  // CounterInc
  /* Now we must be between 0-1000 */

  // not NULL when starting a new count
  if (text) // progress == 0
  {
    char* desc_main = IupGetAttribute(counter_window, "DESC_MAIN");
    char* desc_sec = IupGetAttribute(counter_window, "DESC_SEC");

    if (desc_sec)
      IupSetStrf(counter_window, "DESCRIPTION", "%s%s  %s\n", desc_main, desc_sec, text);
    else
      IupSetStrf(counter_window, "DESCRIPTION", "%s  %s\n", desc_main, text);

    if (counter == 0)
      imlabLogMessagef("    %s", text);
    else
      imlabLogMessagef("      %s", text);
  }

  IupSetInt(counter_window, "COUNT", progress);

  int ret = 1;
  char* state = IupGetAttribute(counter_window, "STATE");
  if (imStrEqual(state, "ABORTED"))
  {
    ret = 0;
    if (counter == 0)
      imlabLogMessagef("  Counter Interrupted!!!!");
    else
      imlabLogMessagef("    Counter Interrupted!!!!");
  }

  // Only for the main counter
  if (counter == 0)
  {
    Ihandle* main_window = IupGetHandle("imlabMainWindow");
    IupSetInt(main_window, "TASKBARPROGRESSVALUE", progress / 10);
  }

  return ret;
}

void imlabCounterCreateProgressDlg()
{
  counter_window = IupProgressDlg();
  IupSetAttribute(counter_window, "ICON", "IMLAB");
  IupSetAttribute(counter_window, "PARENTDIALOG", "imlabMainWindow");
  IupSetAttribute(counter_window, "TOTALCOUNT", "1000");  /* 0 - 1000 */
  IupSetAttribute(counter_window, "RESIZE", "YES");
  IupSetAttribute(counter_window, "PROGRESSHEIGHT", "20");
  IupSetAttribute(counter_window, "TITLE", "Progress");
  IupSetAttribute(counter_window, "DESCRIPTION", "\n\n\n");  /* reserve space for at least 3 lines */
  IupSetAttribute(counter_window, "MINCLOCK", "500");  /* 0.5 seconds minimum */
  IupSetAttribute(counter_window, "MINPERCENT", "20");  /* or at least 20%  (200) */

  IupSetCallback(counter_window, "CANCEL_CB", cmCounterCancel);

  imlabCounterEnable(1);
}

void imlabCounterReleaseProgressDlg()
{
  if (counter_window)
    IupDestroy(counter_window);
}

void imlabCounterEnable(int enable)
{
  if (enable)
    imCounterSetCallback(NULL, imlabCounter);
  else
    imCounterSetCallback(NULL, NULL);
}


/************************************************************************************************/


Ihandle* imlabProcessPreviewInit(imlabImageDocument *document, imImage* image, imImage* NewImage)
{
  Ihandle* preview = IupUser();
  Ihandle* dialog = document->ShowView("Bitmap");
  imImageView* image_view = (imImageView*)IupGetAttribute(dialog, "imImageView");
  IupSetAttribute(preview, "imImageView", (char*)image_view);
  IupSetAttribute(preview, "NewImage", (char*)NewImage);
  IupSetAttribute(preview, "CurrentImage", (char*)image);
  IupSetAttribute(preview, "PreviewChanged", "0");
  return preview;
}

void imlabProcessPreviewUpdate(Ihandle* preview)
{
  imImageView* image_view = (imImageView*)IupGetAttribute(preview, "imImageView");
  imImage *NewImage = (imImage*)IupGetAttribute(preview, "NewImage");
  // replace the current image
  imImageViewChangeImage(image_view, NewImage);
  imImageViewPutImage(image_view);
  IupSetAttribute(preview, "PreviewChanged", "1");
}

static void imlabProcessPreviewRestore(Ihandle* preview)
{
  int changed = IupGetInt(preview, "PreviewChanged");
  if (changed)
  {
    imImageView* image_view = (imImageView*)IupGetAttribute(preview, "imImageView");
    imImage *image = (imImage*)IupGetAttribute(preview, "CurrentImage");
    // restore the original image
    imImageViewChangeImage(image_view, image);
    imImageViewPutImage(image_view);
    IupSetAttribute(preview, "PreviewChanged", "0");
  }
}

void imlabProcessPreviewReset(Ihandle* preview)
{
  imlabProcessPreviewRestore(preview);

  Ihandle* preview_param = (Ihandle*)IupGetAttribute(preview, "PREVIEW_PARAM");
  Ihandle* preview_param_ctrl = (Ihandle*)IupGetAttribute(preview_param, "CONTROL");
  IupSetInt(preview_param, "VALUE", 0);
  IupSetInt(preview_param_ctrl, "VALUE", 0);
}

static void imlabProcessPreviewFinish(Ihandle* preview)
{
  imlabProcessPreviewRestore(preview);
  IupDestroy(preview);
  imlabCounterEnable(1);
}

struct imlabPreviewCounterInfo
{
  int current_value,
    last_clock,
    last_value,
    counting,
    abort;
} counter_info;

static void iProgressSetValue(Ihandle* progress, int value)
{
  int cur_clock = (int)clock();
  if (cur_clock - counter_info.last_clock > 500 ||  /* significant amount of time */
      counter_info.current_value - counter_info.last_value > 200)  /* minimum percentage */
  {
    /* avoid duplicate updates */
    if (value != counter_info.current_value)
    {
      IupSetInt(progress, "VALUE", value);

      counter_info.last_clock = (int)clock();
      counter_info.last_value = value;
    }
  }

  counter_info.current_value = value;
  IupLoopStep();
}

static void imlabParamButtonsEnable(Ihandle* preview_progress, int enable)
{
  Ihandle* param_box = IupGetChild(IupGetDialog(preview_progress), 0);
  Ihandle* button1 = (Ihandle*)IupGetAttribute(param_box, "BUTTON1");
  Ihandle* button2 = (Ihandle*)IupGetAttribute(param_box, "BUTTON2");
  IupSetInt(button1, "ACTIVE", enable);
  IupSetInt(button2, "ACTIVE", enable);
}

static int imlabCounterPreview(int counter, void* user_data, const char* text, int progress)
{
  Ihandle* preview_progress = (Ihandle*)user_data;

  if (progress == -1) // CounterBegin
  {
    Ihandle* label = (Ihandle*)IupGetAttribute(preview_progress, "_IUP_LABEL");
    IupSetStrf(label, "TITLE", "Preview Progress: %s", text);

    counter_info.last_clock = clock();
    counter_info.last_value = 0;
    counter_info.current_value = 0;

    if (counter == 0)
    {
      counter_info.counting = 1;
      counter_info.abort = 0;
      imlabParamButtonsEnable(preview_progress, 0);
    }

    return 1;
  }

  if (progress == 1001) // CounterEnd
  {
    Ihandle* label = (Ihandle*)IupGetAttribute(preview_progress, "_IUP_LABEL");
    IupSetAttribute(label, "TITLE", "Preview Progress: ");

    // force one last update
    if (counter_info.abort)
      IupSetInt(preview_progress, "VALUE", 0);
    else
      IupSetInt(preview_progress, "VALUE", 1000);

    if (counter == 0)
    {
      counter_info.counting = 0;
      imlabParamButtonsEnable(preview_progress, 1);
    }

    return 1;
  }

  // CounterInc
  /* Now we must be between 0-1000 */

  iProgressSetValue(preview_progress, progress);

  if (counter_info.abort)
  {
    IupSetInt(preview_progress, "VALUE", 0);
    return 0;
  }

  return 1;
}

static int imlabProgressCancel_CB(Ihandle*)
{
  counter_info.abort = 1;
  return IUP_DEFAULT;
}

static void imlabProcessPreviewEmbedProgress(Ihandle* preview_param)
{
  Ihandle* separator = IupLabel(NULL);
  IupSetAttribute(separator, "SEPARATOR", "HORIZONTAL");

  Ihandle* label = IupLabel("Preview Progress: ");
  IupSetAttribute(label, "EXPAND", "HORIZONTAL");

  Ihandle* progress = IupGauge();
  IupSetAttribute(progress, "RASTERSIZE", "250x20");
  IupSetAttribute(progress, "EXPAND", "HORIZONTAL");
  IupSetAttribute(progress, "MAX", "1000");
  IupSetAttribute(progress, "SHOWTEXT", "NO");
  IupSetAttribute(progress, "_IUP_LABEL", (char*)label);
  IupSetAttribute(progress, "FLAT", "YES");
  IupSetAttribute(progress, "FGCOLOR", "96 120 232");
  IupSetAttribute(preview_param, "PREVIEW_PROGRESS", (char*)progress);

  Ihandle* button = IupButton(NULL, NULL);
  IupSetAttribute(button, "FLAT", "YES");
  IupSetAttribute(button, "IMAGE", "imlabProgressCancel");
  IupSetCallback(button, "ACTION", imlabProgressCancel_CB);

  Ihandle* hbox = IupHbox(progress, button, NULL);

  Ihandle* preview_ctrl = (Ihandle*)IupGetAttribute(preview_param, "CONTROL");
  Ihandle* vbox = IupGetParent(IupGetParent(preview_ctrl));
  IupAppend(vbox, separator);
  IupAppend(vbox, label);
  IupAppend(vbox, hbox);

  imCounterSetCallback(progress, imlabCounterPreview);
}

int imlabProcessPreviewCheckParam(Ihandle* param_box, int param_index, const char* preview_param_name, Ihandle* preview, int *param_readonly)
{
  Ihandle* preview_param = (Ihandle*)IupGetAttribute(param_box, preview_param_name);
  int show_preview = IupGetInt(preview_param, "VALUE");
  int preview_index = atoi(preview_param_name + 5);

  if (param_index == IUP_GETPARAM_MAP)
  {
    IupSetAttribute(IupGetDialog(param_box), "RESIZE", "Yes");
    imlabProcessPreviewEmbedProgress(preview_param);

    memset(&counter_info, 0, sizeof(imlabPreviewCounterInfo));

    IupSetAttribute(preview, "PREVIEW_PARAM", (char*)preview_param);
  }

  if (param_index == preview_index)
  {
    if (!show_preview)  // turned OFF Preview)
    {
      if (counter_info.counting)
        counter_info.abort = 1;
      else
      {
        Ihandle* preview_progress = (Ihandle*)IupGetAttribute(preview_param, "PREVIEW_PROGRESS");
        IupSetInt(preview_progress, "VALUE", 0);
      }

      imlabProcessPreviewRestore(preview);
    }
    else // turned preview ON
    {
      *param_readonly = 0;
      return 1;
    }
  }

  if (param_index == IUP_GETPARAM_BUTTON1 || param_index == IUP_GETPARAM_BUTTON2) // OK or Cancel
  {
    imlabProcessPreviewFinish(preview);
    return 0;
  }

  if (param_index >= 0 && show_preview) // change param with Preview ON
  {
    // don't allow changes if already counting
    if (counter_info.counting)
    {
      *param_readonly = 1;
      return 0;
    }

    *param_readonly = 0;
    return 1;
  }

  *param_readonly = 0;
  return 0;
}

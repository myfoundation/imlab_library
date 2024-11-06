#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>

#include "documentlist.h"
#include "imagewindow.h"
#include "resultswindow.h"
#include "mainwindow.h"
#include "imlab.h"
#include "dialogs.h"

#include "utl_file.h"


static imlabImageDocument* image_document_list[IMLAB_MAX_IMAGES];
static int image_document_list_count = 0;
static imlabResultsWindow* results_window_list[IMLAB_MAX_IMAGES];
static int results_window_list_count = 0;


static int iDocListMatchImage(imlabMatchFunc match_func, imImage* match_image, imImage* image)
{
  int match = 0;

  if (match_func)
  {
    if (match_func(image, match_image))
      match = 1;
  }
  else if (match_image)
  {
    /* match all characteristics */
    if (imImageMatch(match_image, image))
      match = 1;
  }
  else
    match = 1;
    
  return match;
}

imlabImageDocument** imlabImageDocumentListSelectMulti(const char *title, imlabMatchFunc match_func, imImage* match_image, int *match_count)
{
  int ret, num_win = 0;
  char* list_str[IMLAB_MAX_IMAGES];
  imlabImageDocument* image_document_list_match[IMLAB_MAX_IMAGES];
  imlabImageDocument *match_document = NULL;
  int op_init = 0;

  for (int i = 0; i < image_document_list_count; i++)
  {
    imlabImageDocument *document = image_document_list[i];

    if (iDocListMatchImage(match_func, match_image, document->ImageFile->image))
    {
      if (match_image && match_image == document->ImageFile->image)
      {
        op_init = num_win;
        match_document = document;
      }

      list_str[num_win] = (char*)malloc(strlen(document->FileTitle)+10);
      sprintf(list_str[num_win], "[%d] %s", document->document_index, document->FileTitle);
      image_document_list_match[num_win] = document;
      num_win++;
    }
  }

  if (num_win == 0)
  {
    IupMessage("Error!", "No image matches.");
    return NULL;
  }

  {
    static imlabImageDocument* image_document_list_ret[IMLAB_MAX_IMAGES];
    int mark[IMLAB_MAX_IMAGES], i; 

    memset(mark, 0XFF, sizeof(int)*IMLAB_MAX_IMAGES);
    mark[op_init] = 0;

    if (match_document)
      title = match_document->DlgTitle(title);

    ret = IupListDialog(2, title, num_win, (const char**)list_str, op_init + 1, 60, 7, mark);

    for (i = 0; i < num_win; i++)
      free(list_str[i]);

    if (ret == -1)
      return NULL;

    *match_count = 0;
    for(i = 0; i < num_win; i++)
    {
      if (mark[i])
      {
        image_document_list_ret[*match_count] = image_document_list_match[i];
        (*match_count)++;
      }
    }

    return image_document_list_ret;
  }
}

imlabImageDocument* imlabImageDocumentListSelect(const char *title, imlabMatchFunc match_func, imImage* match_image)
{
  int i, ret, num_win = 0;
  char* list_str[IMLAB_MAX_IMAGES];
  imlabImageDocument* image_document_list_match[IMLAB_MAX_IMAGES];
  imlabImageDocument *match_document = NULL;
  int op_init = 0;
  int max_col = 0, size;

  for (i = 0; i < image_document_list_count; i++)
  {
    imlabImageDocument *document = image_document_list[i];

    if (iDocListMatchImage(match_func, match_image, document->ImageFile->image))
    {
      if (match_image && match_image == document->ImageFile->image)
      {
        op_init = num_win;
        match_document = document;
      }

      size = (int)strlen(document->FileTitle) + 10;
      if (size > max_col) max_col = size;
      list_str[num_win] = (char*)malloc(size);
      sprintf(list_str[num_win], "[%d] %s", document->document_index, document->FileTitle);
      image_document_list_match[num_win] = document;
      num_win++;
    }
  }

  if (num_win == 0)
  {
    IupMessagef("Error!", "%s:\n  No image matches.", title);
    return NULL;
  }

  if (match_document)
    title = match_document->DlgTitle(title);

  ret = IupListDialog(1, title, num_win, (const char**)list_str, op_init + 1, max_col, 7, NULL);

  for (i = 0; i < num_win; i++)
    free(list_str[i]);

  if (ret == -1)
    return NULL;

  return image_document_list_match[ret];
}

imlabImageDocument* imlabImageDocumentListGetMatch(int list_index, imlabMatchFunc match_func, imImage* match_image)
{
  int num_win = 0;

  for (int i = 0; i < image_document_list_count; i++)
  {
    imlabImageDocument *document = image_document_list[i];

    if (iDocListMatchImage(match_func, match_image, document->ImageFile->image))
    {
      if (num_win == list_index)
        return document;

      num_win++;
    }
  }

  return NULL;
}

int imlabImageDocumentListInitFormat(char* list_format, imlabMatchFunc match_func, imImage* match_image, int *op_init)
{
  int num_win = 0, offset = 0;

  *op_init = 0;

  for (int i = 0; i < image_document_list_count; i++)
  {
    imlabImageDocument *document = image_document_list[i];

    if (iDocListMatchImage(match_func, match_image, document->ImageFile->image))
    {
      if (match_image && match_image == document->ImageFile->image)
        *op_init = num_win;

      offset += sprintf(list_format + offset, "|[%d] %.60s", document->document_index, document->FileTitle);
      num_win++;
    }
  }

  if (num_win == 0)
  {
    IupMessage("Error!", "No image matches.");
    return 0;
  }

  sprintf(list_format + offset, "|\n");

  return 1;
}

int imlabImageDocumentListCount(void)
{
  return image_document_list_count;
}

int imlabResultsWindowListCount(void)
{
  return results_window_list_count;
}

imlabImageDocument* imlabImageDocumentListNext(imlabImageDocument* document)
{
  if (!document)
    return image_document_list[0];

  for (int i = 0; i < image_document_list_count; i++)
  {
    if (image_document_list[i] == document)
    {
      if (i < image_document_list_count-1)
        return image_document_list[i+1];
      else 
        return image_document_list[0];
    }
  }

  return NULL;
}

imlabImageDocument* imlabImageDocumentListGet(int index)
{
  if (index < 0 || index > image_document_list_count - 1)
    return NULL;

  return image_document_list[index];
}

imlabResultsWindow* imlabResultsWindowListGet(int index)
{
  if (index < 0 || index > results_window_list_count-1)
    return NULL;

  return results_window_list[index];
}

imlabResultsWindow* imlabResultsWindowListSelect(const char *title)
{
  int i, ret;
  char* list_str[IMLAB_MAX_IMAGES];

  for (i = 0; i < results_window_list_count; i++)
  {
    imlabResultsWindow *results_window = results_window_list[i];
    char* title = IupGetAttribute(results_window->dialog, "TITLE");

    list_str[i] = (char*)malloc(strlen(title) + 10);
    sprintf(list_str[i], "[%d] %s", results_window->window_number, title);
  }

  ret = IupListDialog(1, title, results_window_list_count, (const char**)list_str, 1, 60, 7, NULL);

  for (i = 0; i < results_window_list_count; i++)
    free(list_str[i]);

  if (ret == -1)
    return NULL;

  return results_window_list[ret];
}

void imlabResultsWindowListAdd(imlabResultsWindow* results_window)
{
  results_window_list[results_window_list_count] = results_window;
  results_window_list_count++;
}

void imlabResultsWindowListRemove(imlabResultsWindow* results_window)
{
  int found = 0;
  for (int i = 0; i < results_window_list_count; i++)
  {
    if (results_window_list[i] == results_window)
      found = 1;

    if (found)
      results_window_list[i] = results_window_list[i+1];
  }

  if (found)
    results_window_list_count--;
}

void imlabImageDocumentListAdd(imlabImageDocument* document)
{
  image_document_list[image_document_list_count] = document;
  image_document_list_count++;

  if (image_document_list_count == 1)
  {
    /* first document */
    imlabSetCurrentDocument(document);
    imlabMainWindowUpdateToolbar();
  }
}

void imlabImageDocumentListRemove(imlabImageDocument* document)
{
  if (image_document_list_count > 1)
  {
    if (document == imlabGetCurrentDocument(NULL))
    {
      imlabImageDocument* next_document = imlabImageDocumentListNext(document);
      imlabSetCurrentDocument(next_document);
    }
  }

  int found = 0;
  for (int i = 0; i < image_document_list_count; i++)
  {
    if (image_document_list[i] == document)
      found = 1;

    if (found)
      image_document_list[i] = image_document_list[i + 1];
  }

  if (found)
    image_document_list_count--;

  if (image_document_list_count == 0)
  {
    /* last document */
    imlabSetCurrentDocument(NULL);
    imlabMainWindowUpdateToolbar();
  }
}


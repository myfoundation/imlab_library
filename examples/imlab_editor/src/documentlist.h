/** \file
* \brief Document List
*
*/

#ifndef __DOCUMENTLIST_H
#define __DOCUMENTLIST_H

#include "imagedocument.h"


#define IMLAB_MAX_IMAGES 1000


typedef int(*imlabMatchFunc)(const imImage* image, const imImage* match_image);

/** Dialog to select an image document from the available ones.
* The images can be filtered by two conditions, the match function and the match image.
* Returns NULL if no window matches or the user aborted. */
imlabImageDocument* imlabImageDocumentListSelect(const char *title, imlabMatchFunc match_func, imImage* match_image);

/** Dialog to select multiple image documents from the available ones.
* The images can be filtered by two conditions, the match function and the match image.
* Returns NULL if no window matches or the user aborted. */
imlabImageDocument** imlabImageDocumentListSelectMulti(const char *title, imlabMatchFunc match_func, imImage* match_image, int *match_count);

/** Returns a string with a list of the filtered image document titles separated by '|'.
* The images can be filtered by two conditions, the match function and the match image.
* The list must have at least 4096 chars.
* Returns 0 if no window matches. */
int imlabImageDocumentListInitFormat(char* list_format, imlabMatchFunc match_func, imImage* match_image, int *init_win);

/** Returns an image document from the list.
* Returns NULL if no window matches. */
imlabImageDocument* imlabImageDocumentListGetMatch(int list_index, imlabMatchFunc match_func, imImage* match_image);


/** Returns the next image document from the list.
* Returns NULL if no window matches. */
imlabImageDocument* imlabImageDocumentListNext(imlabImageDocument* document);
imlabImageDocument* imlabImageDocumentListGet(int index);

int imlabImageDocumentListCount(void);

/** Add to the image document list */
void imlabImageDocumentListAdd(imlabImageDocument* document);

/** Remove from the image document list */
void imlabImageDocumentListRemove(imlabImageDocument* document);


struct imlabResultsWindow;

imlabResultsWindow* imlabResultsWindowListGet(int index);

int imlabResultsWindowListCount(void);

/** Add to the results window list */
void imlabResultsWindowListAdd(imlabResultsWindow* results_window);

/** Remove from the results window list */
void imlabResultsWindowListRemove(imlabResultsWindow* results_window);

/** Dialog to select a results window from the available ones. */
imlabResultsWindow* imlabResultsWindowListSelect(const char *title);


#endif

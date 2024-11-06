/** \file
 * \brief Results Window
 */

#ifndef __RESULTSWINDOW_H
#define __RESULTSWINDOW_H


/* Each result is stored in an attribute of the attribute table.
 * Each attribute has object count entries. */  

/** Results Window */
struct imlabResultsWindow
{
  Ihandle *matrix,
          *dialog,
          *plot;

  int window_number;
  double scale;
  const char* scale_units;

  void* object_array; /**< in fact it is an imAttribArray, but we hide this here */

};
                   
/** Creates the results window */
imlabResultsWindow* imlabResultsWindowCreate(const char* image_title, void* object_array, double scale, const char* scale_units);
                   
/** Destroys the results window. */
int imlabResultsWindowClose(imlabResultsWindow* results_win);
          

#endif

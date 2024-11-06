/** \file
 * \brief Histogram Window
 */

#ifndef __HISTOGRAMWINDOW_H
#define __HISTOGRAMWINDOW_H

#include <cd.h>

#include "imagewindow.h"


class imlabHistogramWindow : public imlabImageWindow
{
public:
  Ihandle *toolbar,
          *plot;

  unsigned long *histo[4];
  int histo_count;
  int num_histo;   /* up to 4 histograms can be available */
  unsigned long max;

  int accum;
  int mode;
  int plane;

  imlabHistogramWindow(imlabImageDocument* document);
  ~imlabHistogramWindow();
  void Update();
};
                   
               

#endif

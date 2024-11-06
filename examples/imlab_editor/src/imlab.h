/** \file
 * \brief Application main functions.<br>
 * Some functions use only for Main. <br>
 * Log utilities and aplication version string. */

#ifndef __IMLAB_H
#define __IMLAB_H


/*! \mainpage ImLab Internals
 *
 * \section intro Introduction
 *
 * This is the documentation of the ImLab headers, so it can help you create new functions.
 *
 * \section author Author
 *
 * - Antonio Scuri
 *
 */

/** ImLab Version */
#define IMLAB_TITLE "ImLab 3.2"

/** IMLAB Release Date */
#define IMLAB_BUILD "09 Jan 2019"

/** Returns the main window status bar */
struct _sbStatusBar* imlabStatusBar(void);

/** Sends a text to the Log area using sprintf format */
void imlabLogMessagef(const char* format, ...);

/** Calculates the position for a new window */
void imlabNewWindowPos(int *xpos, int *ypos);

class imlabImageDocument;

/** Returns the image document given an IUP control (since 3.0 can be NULL) */
imlabImageDocument* imlabGetCurrentDocument(Ihandle* ih);

void imlabSetCurrentDocument(imlabImageDocument *document);

Ihandle*  imlabSubmenu(const char* title, Ihandle* child);
Ihandle* imlabItem(const char* title, const char* action);

Ihandle* imlabConfig(void);


#endif  /* __IMLAB_H */

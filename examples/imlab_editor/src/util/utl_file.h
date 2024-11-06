/** \file
 * \brief File Utilities.
 * \ingroup file filetxt filepath filedir
 * \note For all the functions the returned string
 * must be deallocated with free().
 */

#ifndef __UTL_FILE_H
#define __UTL_FILE_H

#ifdef __cplusplus
extern "C" {
#endif

/**\defgroup file File Utilities
 * \brief For all the functions the returned string
 * must be deallocated with free(). */

/** Returns the size of a file in bytes, -1 otherwise.
 * \ingroup file */
unsigned long utlFileSize(const char* filename);

/**\defgroup filetxt Text Files Utilities
 * \ingroup file */

/** Returns the text of a file or NULL if failed.
 * \ingroup filetxt */
char* utlFileLoadText(const char* filename);

/** Saves a text in memory into a file.
 * \return Non zero if sucessfull, 0 otherwise.
 * \ingroup filetxt */
int utlFileSaveText(const char* filename, const char* text);

/**\defgroup filepath File Name String Utilities
 * \ingroup file */

/** Returns the path of a file name or NULL if none if found. <br>
 * The path includes the last slash '/'.
 * \ingroup filepath */
char* utlFileGetPath(const char *filename);

/** Returns the file title (only Name+Ext, no path) of a file name. <br>
 * It extracts the path if any.
 * \ingroup filepath */
char* utlFileGetTitle(const char *filename);

char* utlFileGetFilenameMultiple(const char* dir, const char* filetitle, int *offset);

char* utlFileGetPathMultiple(const char* filename, int *offset);

/** Returns the file extension of a file name if any. Returns NULL otherwise.
 * \ingroup filepath */
char* utlFileGetExt(const char *filename);

/**\defgroup filedir Current Folder Utilities
 * \ingroup file */

/** Returns the path of the current folder if sucessfull, NULL otherwise. <br>
 * The path includes the last slash '/'.
 * \ingroup filedir */
char* utlFileGetCurrentPath(void);

/** Changes the path of the current folder.
 * \return Non zero if sucessfull, 0 otherwise.
 * \ingroup filedir */
int utlFileSetCurrentPath(const char* dir_name);

#ifdef __cplusplus
}
#endif

#endif

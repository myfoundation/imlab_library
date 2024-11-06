
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "utl_file.h"

#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#endif

unsigned long utlFileSize(const char* filename)
{
  FILE* file;
  unsigned long size;

  assert(filename);

  file = fopen(filename, "rb");
  if (!file) return (unsigned long)-1;

  fseek(file, 0L, SEEK_END);
  size = ftell(file);

  fclose(file);

  return size;
}

/*****************************************************************/

int utlFileSaveText(const char* filename, const char* text)
{
  FILE* file;

  assert(filename);
  assert(text);

  file = fopen(filename, "wb");
  if (!file)
    return 0;

  fwrite(text, 1, strlen(text), file);

  if (ferror(file))
  {
    fclose(file);
    return 0;
  }

  fclose(file);
  return 1;
}

char* utlFileLoadText(const char* filename)
{
  FILE* file;
  char* buffer;
  int len;

  assert(filename);

  file = fopen(filename, "rb");
  if (!file)
    return NULL;

  {
    fseek(file, 0L, SEEK_END);
    len = ftell(file);
    fseek(file, 0L, SEEK_SET);
  }

  buffer = (char*)malloc(len + 1);
  fread(buffer, 1, len, file);
  buffer[len] = 0;

  if (ferror(file))
  {
    free(buffer);
    buffer = NULL;
  }

  fclose(file);
  return buffer;
}

/*****************************************************************/

char* utlFileGetFilenameMultiple(const char* dir, const char* filetitle, int *offset)
{
  const char* file = filetitle;
  while (*file != 0 && *file != '|')
    file++;

  if (file == filetitle)
    return NULL;

  {
    int size = (int)(file - filetitle) + 1;
    int dir_size = (int)strlen(dir);
    char* filename = (char*)malloc(size + dir_size + 1);
    memcpy(filename, dir, dir_size);
    filename[dir_size] = '\\';
    memcpy(filename + dir_size + 1, filetitle, size - 1);
    filename[size + dir_size] = 0;
    *offset += size;
    return filename;
  }
}

char* utlFileGetPathMultiple(const char* filename, int *offset)
{
  const char* file = filename;
  while (*file != 0 && *file != '|')
    file++;

  if (*file == 0)
    return NULL;

  {
    int size = (int)(file - filename) + 1;
    char* dir = (char*)malloc(size);
    memcpy(dir, filename, size - 1);
    dir[size - 1] = 0;
    *offset = size;
    return dir;
  }
}

char* utlFileGetPath(const char *filename)
{
  int len;

  assert(filename);

  // Starts at the last character
  len = (int)strlen(filename) - 1;
  while (len != 0)
  {
    if (filename[len] == '\\' || filename[len] == '/')
    {
      len++;
      break;
    }

    len--;
  }

  if (len == 0)
    return NULL;

  {
    char* path = (char*)malloc(len + 1);
    memcpy(path, filename, len);
    path[len] = 0;

    return path;
  }
}

char* utlFileGetTitle(const char *filename)
{
  int offset, len;

  assert(filename);

  len = (int)strlen(filename);

  // Starts at the last character
  offset = len - 1;
  while (offset != 0)
  {
    if (filename[offset] == '\\' || filename[offset] == '/')
    {
      offset++;
      break;
    }

    offset--;
  }

  {
    int title_size = len - offset + 1;
    char* file_title = (char*)malloc(title_size);
    memcpy(file_title, filename + offset, title_size);
    return file_title;
  }
}

char* utlFileGetExt(const char *filename)
{
  int offset, len;

  assert(filename);

  len = (int)strlen(filename);

  // Starts at the last character
  offset = len - 1;
  while (offset != 0)
  {
    // if found a path separator stop.
    if (filename[offset] == '\\' || filename[offset] == '/')
      return NULL;

    if (filename[offset] == '.')
    {
      offset++;
      break;
    }

    offset--;
  }

  if (offset == 0) 
    return NULL;

  {
    int i, ext_size = len - offset + 1;
    char* file_ext = (char*)malloc(ext_size);
    filename += offset;
    for (i = 0; i < ext_size-1; i++)
      file_ext[i] = (char)tolower((int)filename[i]);
    file_ext[ext_size-1] = 0;
    return file_ext;
  }
}

/*****************************************************************/

#ifndef WIN32
static char* utl_getcwd(void)
{
  size_t size = 256;
  char *buffer = (char *)malloc(size);

  for (;;)
  {
    if (getcwd (buffer, size) != NULL)
      return buffer;

    if (errno != ERANGE)
    {
      free (buffer);
      return 0;
    }

    size *= 2;
    buffer = (char *)realloc(buffer, size);
  }
}
#endif

char* utlFileGetCurrentPath(void)
{
  char* cur_dir = NULL;

#ifdef WIN32
  int len = GetCurrentDirectoryA(0, NULL);
  if (len == 0) return NULL;

  cur_dir = (char*)malloc(len + 2);
  GetCurrentDirectoryA(len+1, cur_dir);
  cur_dir[len] = '\\';
  cur_dir[len+1] = 0;
#else
  cur_dir = utl_getcwd();
#endif

  return cur_dir;
}

int utlFileSetCurrentPath(const char* dir_name)
{
  assert(dir_name);

#ifdef WIN32
  return SetCurrentDirectoryA(dir_name);
#else
  return chdir(dir_name) == 0? 1: 0;
#endif
}


/* ========================================================================== *
 * Copyright (c) 2015-2016 秦凡东 (Qin Fandong)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================== *
 * Get uuid of every device
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/param.h>
#include "chomp.h"
#include "say.h"
#include "find.h"
#include "readfile.h"
#include "assert.h"
#include "config.h"

static char **list = NULL;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* ========================================================================== *
 * Return loacl devices
 * ========================================================================== */
static int
localDevs (void)
{
  char **text = NULL;
  char *pos = NULL;
  char path[MAXPATHLEN + 1] = { 0 }, buff[MAXPATHLEN + 1] = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  int uuid2path = 0, label2path = 0;
  int index = 0, index2 = 0;
  int no = 0;
  int len = 0;
  int chr = 0;
  const char uuidInterface[] = "/dev/disk/by-uuid";
  const char labelInterface[] = "/dev/disk/by-label";

  memset (path, 0, sizeof (path));
  memset (buff, 0, sizeof (buff));
  sayMode (&mode);

  if (list)
    {
      return 1;
    }

  if (readfile (FSTAB, &text) < 1 || !text)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }
  if (!text)
    {
      return 1;
    }

  /* Remove all comments */
  for (index = 0; text[index]; ++index)
    {
      for (index2 = 0;
            text[index][index2] && '#' != text[index][index2];
            ++index2);
      text[index][index2] = 0;
    }

  if (!text)
    {
      return -1;
    }

  pthread_mutex_lock (&mutex);

  /* Apply enougth space for pointers */
  ++index;
  if (!(list = (char **)malloc (index * sizeof (char *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (list, 0, index * sizeof (char *));

  /* Here we will not use regex */
  no = 0;
  for (index = 0; text[index]; ++index)
    {
      uuid2path = label2path = 0;

      if ((pos = strstr (text[index], "/dev")))
        {
          /* Do nothing */
        }
      else if ((pos = strstr (text[index], "UUID=")))
        {
          pos += 5;
          uuid2path = 1;
        }
      else if ((pos = strstr (text[index], "LABEL=")))
        {
          pos += 6;
          label2path = 1;
        }
      else
        {
          continue;
        }

      for (index2 = 0; (chr = pos[index2]); ++index2)
        {
          if (' ' == pos[index2] || '\t' == pos[index2])
            {
              break;
            }
        }

      len = index2;

      if (!(list[no] = (char *)malloc (len + 1)))
        {
          say (mode, MSG_E, "malloc failed:%s\n", strerror (errno));
          abort ();
        }

      memset (list[no], 0, len + 1);
      memcpy (list[no], pos, len);

      if (uuid2path || label2path)
        {
          snprintf (path, MAXPATHLEN, "%s/%s",
                      uuid2path ? uuidInterface : labelInterface, list[no]);

          if (-1 == readlink (path, buff, MAXPATHLEN))
            {
              say (mode, MSG_E, "readlink failed: %s\n", strerror (errno));
              continue;
            }
          len = strlen (buff);

          if (!(list[no] = (char *)realloc (list[no], len + 1)))
            {
              say (mode, MSG_E, "realloc failed: %s\n", strerror (errno));
              abort ();
            }

          memset (list[no], 0, len + 1);
          memcpy (list[no], buff, len);
        }

      /* Now list[no] is path of device file,
       * either absolute or relative paths is ok.
       * And we should deal next */
      ++no;
    }

  pthread_mutex_unlock (&mutex);

  if (text)
    {
      for (index = 0; text[index]; ++index)
        {
          free (text[index]);
        }
      free (text);
      text = NULL;
    }

  return 1;
}

/* ========================================================================== *
 * Determine whether a devPath is a local device
 * ========================================================================== */
static int
isLocalDev (const char * const devPath)
{
  char *pattern = NULL;
  char buff[MAXPATHLEN + 1] = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  int index = 0;
  int status = 0;

  memset (buff, 0, sizeof (buff));
  sayMode (&mode);

  ASSERT_RETURN (devPath, "devPath is NULL.\n", -1);

  strncpy (buff, devPath, MAXPATHLEN);
  pattern = basename (buff);

  /* Remove partition number */
  for (index = strlen (pattern) - 1; index > -1; --index)
    {
      if (pattern[index] >= '0' && pattern[index] < '9' + 1)
        {
          pattern[index] = 0;
        }
    }

  localDevs ();

  status = 0;
  for (index = 0; list[index]; ++index)
    {
      if (strstr (list[index], pattern))
        {
          status = 1;
          break;
        }
      /* } */
    }

  return status;
}

int
devs (char ***addr)
{
  DIR *dh = NULL;
  struct dirent *dir = NULL;
  char **uuids = NULL;
  char buff[MAXPATHLEN + 1] = { 0 }, dev[MAXPATHLEN + 1] = { 0 };
  size_t len = 0;
  saymode_t mode = 0;
  int no = 0, count = 0;
  const char interface[] = "/dev/disk/by-uuid";

  memset (buff, 0, sizeof (buff));
  memset (dev, 0, sizeof (dev));
  sayMode (&mode);

  ASSERT_RETURN (addr, "addr is NULL.\n", -1);

  if (!(dh = opendir (interface)))
    {
      say (mode, MSG_E, "opendir failed: %s\n", strerror (errno));
      return -1;
    }

  no = 1 << 10;
  if (!(uuids = (char **)malloc (no * sizeof (char *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (uuids, 0, no * sizeof (char *));

  for (count = 0; (dir = readdir (dh)); ++count)
    {
      if (no == count)
        {
          no *= 2;
          uuids = (char **)realloc (uuids, no * sizeof (char *));
          memset (&uuids[count], 0, no - count);
        }

      if (DT_LNK != dir->d_type)
        {
          --count;
          continue;
        }

      snprintf (dev, MAXPATHLEN,"%s/%s", interface, dir->d_name);
      if (-1 == readlink (dev, buff, MAXPATHLEN))
        {
          --count;
          continue;
        }

      if (isLocalDev (buff))
        {
          --count;
          continue;
        }

      len = strlen (dir->d_name);
      if (!(uuids[count] = (char *)malloc (len + 1)))
        {
          say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
          abort ();
        }
      memcpy (uuids[count], dir->d_name, len);

      uuids[count][len] = 0;
      chomp (uuids[count]);
    }

  if (no == count)
    {
      if (!(uuids = (char **)realloc (uuids, (no + 1) * sizeof (char **))))
        {
          say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
          abort ();
        }
      uuids[no] = NULL;
    }

  *addr = uuids;

  closedir (dh);
  pthread_mutex_destroy (&mutex);

  if (list)
    {
      for (count = 0; list[count]; ++count)
        {
          free (list[count]);
        }
      free (list);
      list = NULL;
    }

  return 1;
}


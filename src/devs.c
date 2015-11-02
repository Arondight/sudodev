/* ========================================================================== *
 * Copyright (c) 2015 秦凡东 (Qin Fandong)
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
 * Get uuid of every device via interface /dev/disk/by-uud/
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/param.h>
#include "chomp.h"
#include "say.h"
#include "find.h"
#include "config.h"

int
isLocalDev (const char * const devPath)
{
  char *pattern;
  char buff[MAXPATHLEN];
  saymode_t mode;
  int index;
  int status;

  sayMode (&mode);

  strncpy (buff, devPath, MAXPATHLEN - 1);
  pattern = basename (buff);

  for (index = strlen (pattern) - 1; index > -1; --index)
    {
      if (pattern[index] >= '0' && pattern[index] < '9' + 1)
        {
          pattern[index] = 0;
        }
    }

  /* TODO: Use a faster and none-threadsafe way to search FSTAB { */
  if (-1 == (status = find (FSTAB, pattern)))
    {
      say (mode, MSG_E, "find failed\n");
      return -1;
    }

  return status;
  /* } */
}

int
devs (char ***addr)
{
  DIR *dh;
  struct dirent *dir;
  char **uuids;
  char buff[MAXPATHLEN], dev[MAXPATHLEN];
  size_t len;
  saymode_t mode;
  int no, count;
  const char interface[] = "/dev/disk/by-uuid";

  sayMode (&mode);

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

      snprintf (dev, MAXPATHLEN - 1,"%s/%s", interface, dir->d_name);
      if (-1 == readlink (dev, buff, MAXPATHLEN - 1))
        {
          continue;
        }

      if (isLocalDev (buff))
        {
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

  closedir (dh);

  *addr = uuids;

  return 1;
}


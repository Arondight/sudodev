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
 * Operating /etc/group
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "find.h"
#include "readfile.h"
#include "say.h"
#include "sort.h"
#include "assert.h"
#include "config.h"

static int
cmp (const void * const a, const void * const b)
{
  saymode_t mode = MODE_UNKNOWN;

  sayMode (&mode);

  ASSERT_ABORT (a, "a is NULL.\n");
  ASSERT_ABORT (b, "b is NULL.\n");

  return *(int *)a - *(int *)b;
}

/* ========================================================================== *
 * Add a group
 * ========================================================================== */
int
addGroup (const char * const group)
{
  FILE *fh = NULL;
  char **list = NULL;
  char *id = NULL;
  int *ids = NULL;
  char line[1 << 10] = { 0 };
  char newID[1 << 10] = { 0 };
  size_t len = 0;
  saymode_t mode = MODE_UNKNOWN;
  int max = 0;
  int no = 0;
  int size = 0;
  int index = 0, index2 = 0;
  int area = 0;
  const char sep[] = ":";

  memset (line, 0, sizeof (line));
  memset (newID, 0, sizeof (newID));
  sayMode (&mode);

  ASSERT_RETURN (group, "group is NULL.\n", -1);

  if (readfile (GROUP_FILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  /* Get group id > 999 */
  size = 1 << 10;
  if (!(ids = (int *)malloc (size * sizeof (int *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }

  for (index = 0, index2 = 0; list[index]; ++index)
    {
      id = strtok (list[index], sep);
      for (area = 3 - 1 - 1; area > -1; --area)
        {
          id = strtok (NULL, sep);
        }

      if ((ids[index2] = atoi (id)) < 1000)
        {
          continue;
        }

      ++index2;

      if (index2 == size)
        {
          size *= 2;
          ids = (int *)realloc (ids, size);
        }
    }

  if (index2 > 1)
    {
      if (-1 == msort (ids, index2, sizeof (int), cmp))
        {
          say (mode, MSG_E, "msort failed\n");
          return -1;
        }
    }

  /* Get a new group id */
  max = 0;
  if (1 == index2)
    {
      max = *ids > 1000 ? 1000 - 1 : 1000;
    }
  else if (!index2 || *ids > 1000)
    {
      max = 1000 - 1;
    }
  else
    {
      for (index = 0; index < index2; ++index)
        {
          if (ids[index + 1] - ids[index] > 1)
            {
              max = ids[index];
              break;
            }
          max = ids[index] > max ? ids[index] : max;
        }
    }

  /* Construct rule line */
  no = max + 1;
  memset (line, 0, sizeof (line));
  sprintf (newID, "%d", no);
  strcpy (line, group);
  strcat (line, ":x:");
  strcat (line, newID);
  strcat (line, ":\n");

  /* Write rule to /etc/group */
  if (!(fh = fopen (GROUP_FILE, "a+")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      exit (1);
    }

  say (mode, MSG_I, "add group to %s\n", GROUP_FILE);

  len = strlen (line);
  if (fwrite (line, sizeof (char), len, fh) < len)
    {
      say (mode, MSG_E, "fwrite failed: %s\n", strerror (errno));
      exit (1);
    }

  /* Do clean at last */
  fclose (fh);

  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
    }

  if (ids)
    {
      free (ids);
      ids = NULL;
    }

  return 1;
}


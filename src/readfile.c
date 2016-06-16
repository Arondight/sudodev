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
 * Read text file into a list
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "chomp.h"
#include "say.h"
#include "assert.h"

int
readfile (const char * const path, char ***list)
{
  FILE *fd = NULL;
  char **split = NULL;
  char *in = NULL, *tmp = NULL;
  saymode_t mode = MODE_UNKNOWN;
  int begin = 0, end = 0;
  int count = 0, no = 0;
  int size = 0;
  int ret = 0;

  sayMode (&mode);

  ASSERT_RETURN (path, "path is NULL.\n", -1);
  ASSERT_RETURN (list, "list is NULL.\n", -1);

  *list = split = NULL;

  if (access (path, 0))
    {
      return 0;     /* No need to return -1 */
    }

  if (!(fd = fopen (path, "r")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

  /* Read into a tmp */
  size = 1 << 16;
  begin = 0;
  if (!(in = (char *)malloc (size)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  if ((size = fread (in, sizeof (char), size, fd)) < 1)
    {
      ret = 1;
      goto CLEAN;
    }
  end = size;
  if (!(tmp = (char *)malloc (size)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  while (size > 0)
    {
      memcpy (&tmp[begin], in, size);
      begin += size;
      size = fread (in, sizeof (char), size, fd);
      end += size;
      if (!(tmp = (char *)realloc (tmp, end)))
        {
          say (mode, MSG_E, "realloc failed: %s\n", strerror (errno));
          abort ();
        }
    }

  begin = end - 1;
  ++end;
  if (!(tmp = (char *)realloc (tmp, end)))
    {
      say (mode, MSG_E, "realloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (&tmp[begin], 0, end - begin);

  /* Add '\n' at last if necessary */
  if ('\n' != tmp[begin])
    {
      tmp[begin + 1] = '\n';
    }

  /* Split into split */
  size = end;
  begin = 0;
  count = 0;
  no = 1 << 10;
  if (!(split = (char **)malloc (no * sizeof (char *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (&split[count], 0, no - count);
  for (end = 0; end < size; ++end)
    {
      /* split by "\n" */
      if ('\n' == tmp[end])
        {
          if (no == count)
            {
              no *= 2;
              split = (char **)realloc (split, no * sizeof (char **));
              memset (&split[count], 0, no - count);
            }

          /* 2: '\n'+'\0' */
          if (!(split[count] = (char *)malloc (end - begin + 2)))
            {
              say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
              abort ();
            }
          memcpy (split[count], &tmp[begin], end - begin + 1);
          split[count][end - begin + 1] = 0;
          chomp (split[count]);
          begin = end + 1;
          ++count;
        }
    }

  /* Add a NULL a last if all of split is used */
  if (count == no)
    {
      if (!(split = (char **)realloc (split, (no + 1) * sizeof (char **))))
        {
          say (mode, MSG_E, "realloc failed: %s\n", strerror (errno));
          abort ();
        }
      split[no] = NULL;
    }

  *list = split;
  ret = 1;

CLEAN:
  if (fd)
    {
      fclose (fd);
      fd = NULL;
    }
  if (in)
    {
      free (in);
      in = NULL;
    }
  if (tmp)
    {
      free (tmp);
      tmp = NULL;
    }

  return ret;
}


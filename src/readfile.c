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
 * Read text file into a list
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "chomp.h"
#include "say.h"

int
readfile (const char * const path, char ***list)
{
  FILE *fd;
  char **split;
  char *in, *tmp;
  saymode_t mode;
  int begin, end;
  int count, no;
  int size;

  sayMode (&mode);

  if (access (path, 0))
    {
      *list = split = NULL;
      /* Hide error message here { *
      say (mode, MSG_E, "access failed: %s\n", strerror (errno));
      * } */
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
  size = fread (in, sizeof (char), size, fd);
  end = size;
  if (!(tmp = (char *)malloc (size)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  while (0 < size)
    {
      memcpy (&tmp[begin], in, size);
      begin += size;
      size = fread (in, sizeof (char), size, fd);
      end += size;
      tmp = (char *)realloc (tmp, end);
    }

  /* Add '\n' at last if necessary */
  size = strlen (tmp);
  if ('\n' != tmp[size - 1])
    {
      tmp = (char *)realloc (tmp, size + 1);
      tmp[size] = '\n';
      tmp[size + 1] = 0;
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
      split = (char **)realloc (split, (no + 1) * sizeof (char **));
      split[no] = NULL;
    }

  fclose (fd);
  free (in);
  free (tmp);

  *list = split;

  return 1;
}


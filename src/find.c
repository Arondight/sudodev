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
 * Find if exist a line matching a pattern in file
 * ========================================================================== */
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include "readfile.h"
#include "say.h"
#include "assert.h"
#include "config.h"

int
find (const char * const path, const char * const pattern)
{
  regmatch_t matches[1] = { 0 };
  regex_t regex = { 0 };
  char **list = NULL;
  char strerr[1 << 10] = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  int status = 0;
  int index = 0;
  int find = 0;

  memset (matches, 0, sizeof (matches));
  memset (strerr, 0, sizeof (strerr));
  sayMode (&mode);

  ASSERT_RETURN (path, "path is NULL.\n", -1);
  ASSERT_RETURN (pattern, "pattern is NULL.\n", -1);

  if (readfile (path, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if ((status = regcomp (&regex, pattern, REG_EXTENDED)) < 0)
    {
      regerror (status, &regex, strerr, sizeof (strerr));
      say (mode, MSG_E, "regcomp failed: %s\n", strerr);
      return find = 0;
    }

  for (find = 0, index = 0; list[index]; ++index)
   {
      if (!(regexec (&regex, list[index],
                     sizeof (matches) / sizeof (regmatch_t),
                     matches, 0)))
        {
          find = 1;
          break;
        }
    }

  regfree (&regex);

  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
  }

  return find;
}


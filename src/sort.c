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
 * aint with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================== *
 * Sort a list
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ========================================================================== *
 * Merge 2 list
 * ========================================================================== */
int
merge (char ** const firstPart, const int firstPartLen,
       char * const * const secondPart, const int secondPartLen)
{
  char **tmp;
  int count, count2, count3;

  tmp = (char **)malloc ((firstPartLen + secondPartLen) * sizeof (char **));
  count = count2 = count3 = 0;

  /* Merge 2 string */
  while ((count < firstPartLen) && (count2 < secondPartLen))
    {
      /* Sort Ascending */
      if (strcmp (firstPart[count], secondPart[count2]) < 0)
        {
          tmp[count3] = firstPart[count];
          ++count;
        }
      else
        {
          tmp[count3] = secondPart[count2];
          ++count2;
        }
      ++count3;
    }

  /* Move none treactment string to beginning of &tmp[count3] */
  while (count < firstPartLen)
    {
      tmp[count3] = firstPart[count];
      ++count;
      ++count3;
    }
  while (count2 < secondPartLen)
    {
      tmp[count3] = secondPart[count2];
      ++count2;
      ++count3;
    }

  /* Copy tmp to firstPart */
  memcpy (firstPart, tmp, (firstPartLen + secondPartLen) * sizeof (char *));

  free (tmp);

  return 1;
}

/* ========================================================================== *
 * Sort a list using merge sort
 * ========================================================================== */
int
sortList (char ** const list, int len)
{
  char **firstPart = NULL;
  char **secondPart = NULL;
  int firstPartLen = 0;
  int secondPartLen = 0;

  if (len > 1)
    {
      /* Split list to 2 part */
      firstPart = list;
      firstPartLen = len / 2;
      secondPart = &list[len / 2];
      secondPartLen = len - firstPartLen;

      sortList (firstPart, firstPartLen);
      sortList (secondPart, secondPartLen);

      merge (firstPart, firstPartLen, secondPart, secondPartLen);
    }

  return 1;
}


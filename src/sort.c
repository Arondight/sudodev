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
 * aint with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================== *
 * Do a merge sort
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "say.h"

/* ========================================================================== *
 * Merge 2 part
 * ========================================================================== */
int
merge (void * const part1, const size_t p1Len,
       const void * const part2, const size_t p2Len,
       const size_t size,
       int (* const cmp) (const void * const, const void * const))
{
  char *tmp;
  char *p1, *p2;
  size_t index, index2, index3;

  p1 = (char *)part1;
  p2 = (char *)part2;

  if (!(tmp = (char *)malloc ((p1Len + p2Len) * size)))
    {
      abort ();
    }

  index = index2 = index3 = 0;

  /* Merge 2 part */
  while ((index < p1Len * size) && (index2 < p2Len * size))
    {
      /* Sort Ascending */
      if (cmp (&p1[index], &p2[index2]) < 0)
        {
          memcpy (&tmp[index3], &p1[index], size);
          index += size;
        }
      else
        {
          memcpy (&tmp[index3], &p2[index2], size);
          index2 += size;
        }
      index3 += size;
    }

  /* Move none treactment data to beginning of &tmp[index3] */
  while (index < p1Len * size)
    {
      memcpy (&tmp[index3], &p1[index], size);
      index += size;
      index3 += size;
    }
  while (index2 < p2Len * size)
    {
      memcpy (&tmp[index3], &p2[index2], size);
      index2 += size;
      index3 += size;
    }

  /* Copy tmp to p1 */
  memcpy (p1, tmp, (p1Len + p2Len) * size);

  if (tmp)
    {
      free (tmp);
      tmp = NULL;
    }

  return 1;
}

/* ========================================================================== *
 * Sort data with merge sort
 * ========================================================================== */
int
msort (void * const data, const size_t items, const size_t size,
       int (* const cmp) (const void * const, const void * const))
{
  char *p1;
  char *p2;
  size_t p1Len;
  size_t p2Len;

  if (!(data && cmp))
    {
      return -1;
    }

  if (items > 1 && size > 0)
    {
      p1 = (char *)data;
      p1Len = items / 2;
      p2 = (char *)data + p1Len * size;
      p2Len = items - p1Len;

      if (-1 == msort (p1, p1Len, size, cmp))
        {
          return -1;
        }
      if (-1 == msort (p2, p2Len, size, cmp))
        {
          return -1;
        }

      merge (p1, p1Len, p2, p2Len, size, cmp);
    }

  return 1;
}


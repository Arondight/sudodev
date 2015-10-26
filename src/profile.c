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
 * Opreating /etc/sudodev.conf
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <errno.h>
#include <sys/stat.h>
#include "say.h"
#include "sort.h"
#include "find.h"
#include "readfile.h"
#include "config.h"

int
cmpStr (const void * const a, const void * const b)
{
  return strcmp ((char *)a, (char *)b);
}

/* ========================================================================== *
 * Discern if string is a UUID (or a serial number for NTFS/(ex)?fat.*)
 * ========================================================================== */
int
isUUID (const char * const string)
{
  regmatch_t matches[1];
  regex_t regex;
  saymode_t mode;
  char strerr[1 << 10];
  int status;
  const char pattern[] = "[^-0-9a-fA-F\n]";

  sayMode (&mode);

  if (!string)
    {
      say (mode, MSG_E, "null pointer\n");
      return -1;
    }

  if ((status = regcomp (&regex, pattern, REG_EXTENDED | REG_NEWLINE)) < 0)
    {
      regerror (status, &regex, strerr, sizeof (strerr));
      say (mode, MSG_E, "regcomp failed: %s\n", strerr);
      return -1;
    }

  status = 0;
  if (regexec (&regex, string,
                 sizeof (matches) / sizeof (regmatch_t),
                 matches, 0))
    {
      status = 1;
    }

  regfree (&regex);

  return status;
}

/* ========================================================================== *
 * Make sure attributes of profile is right
 * ========================================================================== */
int
profileMode (void)
{
  FILE *fh;
  struct stat statbuf;
  saymode_t mode;
  int status;

  sayMode (&mode);

  if (access (PROFILE, 0))
    {
      if (!(fh = fopen (PROFILE, "w")))
        {
          say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
          return -1;
        }
      fclose (fh);
    }

  if (-1 == (status = stat (PROFILE, &statbuf)))
    {
      say (mode, MSG_E, "stat failed: %s\n", strerror (errno));
      return -1;
    }

  if ((statbuf.st_mode & ~0600) || (0600 != (statbuf.st_mode & 0600)))
    {
      if (chmod (PROFILE, 0600))
        {
          say (mode, MSG_E, "chmod failed: %s\n", strerror (errno));
          return -1;
        }
    }

  return 1;
}

/* ========================================================================== *
 * Overwrite profile with list
 * ========================================================================== */
int
profileOverwrite (char ** const list)
{
  FILE *fh;
  saymode_t mode;
  size_t len;
  int index;
  int error, status;

  sayMode (&mode);

  if (!(fh = fopen (PROFILE, "w")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

  /* List should not be NULL here, no need to check */
  error = 0;
  for (index = 0; list[index]; ++index)
    {
      if (-1 == (status = isUUID (list[index])))
        {
          say (mode, MSG_E, "isUUID failed\n");
        }
      if (!status)
        {
          continue;
        }

      len = strlen (list[index]);
      if ('\n' != list[index][len - 1])
        {
          list[index][len] = '\n';  /* Replace 0 with '\n' at end of string */
          ++len;
        }

      if (fwrite (list[index], sizeof (char), len, fh) < len)
        {
          say (mode, MSG_E, "fwrite failed: %s\n", strerror (errno));
          error = 1;
          continue;
        }
    }

  fclose (fh);

  return error ? -1 : 1;
}

/* ========================================================================== *
 * Add item to profile
 * ========================================================================== */
int
profileAddItem (const char * const item)
{
  char **list;
  char *line;
  saymode_t mode;
  size_t len;
  int index;

  sayMode (&mode);

  if (!item)
    {
      say (mode, MSG_E, "null pointer\n");
      return -1;
    }

  profileMode ();

  if (find (PROFILE, item))
    {
      return 1;
    }

  len = strlen (item);
  if (!(line = (char *)malloc (len + 2)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (line, 0, len + 2);
  snprintf (line, len + 2, "%s\n", item);

  if (readfile (PROFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  /* Add item */
  for (len = 0; list[len]; ++len);

  list = (char **)realloc (list, (len + 2) * sizeof (char *));
  list[len] = line;
  list[len + 1] = NULL;

  /* Sort and write */
  if (-1 == msort (list, len + 1, sizeof (char *), cmpStr))
    {
      say (mode, MSG_E, "msort failed\n");
      return -1;
    }

  if (-1 == profileOverwrite (list))
    {
      say (mode, MSG_E, "profileOverwrite failed\n");
      return -1;
    }

  /* Should not free line here - will cause a double free error */
  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
      line = NULL;
    }

  return 1;
}

/* ========================================================================== *
 * Delete item in profile
 * ========================================================================== */
int
profileDelItem (const char * const pattern)
{
  regmatch_t matches[1];
  regex_t regex;
  char **list = NULL;
  char **out;
  char strerr[1 << 10];
  saymode_t mode;
  size_t len;
  int index, index2;
  int status;

  sayMode (&mode);

  if (!pattern)
    {
      say (mode, MSG_E, "null pointer\n");
      return -1;
    }

  profileMode ();

  if (readfile (PROFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if (!list)
    {
      say (mode, MSG_E, "null pointer\n");
      return -1;
    }

  /* Output list, have enough length fill with NULL */
  for (len = 0; list[len]; ++len);
  if (!(out = (char **)malloc ((len + 1) * sizeof (char *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (out, 0, (len + 1) * sizeof (char *));

  if ((status = regcomp (&regex, pattern, REG_EXTENDED)) < 0)
    {
      regerror (status, &regex, strerr, sizeof (strerr));
      say (mode, MSG_E, "regcomp failed: %s\n", strerr);
      return -1;
    }

  /* Fill out */
  index2 = 0;
  for (index = 0; list[index]; ++index)
   {
      if (regexec (&regex, list[index],
                      sizeof (matches) / sizeof (regmatch_t),
                      matches, 0))
        {
          out[index2++] = list[index];
        }
    }

  /* Sort and write */
  for (len = 0; out[len]; ++len);
  if (-1 == msort (out, len, sizeof (char *), cmpStr))
    {
      say (mode, MSG_E, "msort failed\n");
      return -1;
    }

  if (-1 == profileOverwrite (out))
    {
      say (mode, MSG_E, "profileOverwrite failed\n");
      return -1;
    }

  regfree (&regex);

  /* Should not free out here - will cause double free errors */
  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
      out = NULL;
    }

  return 1;
}


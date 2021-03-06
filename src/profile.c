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
#include "assert.h"
#include "config.h"

int
static cmp (const void * const a, const void * const b)
{
  saymode_t mode = MODE_UNKNOWN;

  sayMode (&mode);

  ASSERT_ABORT (a, "a is NULL.\n");
  ASSERT_ABORT (b, "b is NULL.\n");

  return strcmp (*(char **)a, *(char **)b);
}

/* ========================================================================== *
 * Discern if string is a UUID (or a serial number for NTFS/(ex)?fat.*)
 * ========================================================================== */
static int
isUUID (const char * const string)
{
  regmatch_t matches[1] = { 0 };
  regex_t regex = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  char strerr[1 << 10] = { 0 };
  int status = 0;
  const char pattern[] = "[^-0-9a-fA-F\n]";

  memset (matches, 0, sizeof (matches));
  memset (&regex, 0, sizeof (regex));
  memset (strerr, 0, sizeof (strerr));
  sayMode (&mode);

  ASSERT_RETURN (string, "string is NULL.\n", -1);

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
  FILE *fh = NULL;
  struct stat statbuf = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  int status = 0;

  memset (&statbuf, 0, sizeof (statbuf));
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
      if (chmod (PROFILE, PROFILE_MODE))
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
  FILE *fh = NULL;
  saymode_t mode = MODE_UNKNOWN;
  size_t len = 0;
  int index = 0;
  int error = 0, status = 0;

  sayMode (&mode);

  ASSERT_RETURN (list, "list is NULL.\n", -1);

  if (!(fh = fopen (PROFILE, "w")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

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
  char **list = NULL;
  char *line = NULL;
  saymode_t mode = MODE_UNKNOWN;
  size_t len = 0;
  int index = 0;

  sayMode (&mode);

  ASSERT_RETURN (item, "item is NULL.\n", -1);

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
  for (len = 0; list && list[len]; ++len);
  list = (char **)realloc (list, (len + 2) * sizeof (char *));
  list[len] = line;
  list[len + 1] = NULL;

  /* Sort and write */
  if (-1 == msort (list, len + 1, sizeof (char *), cmp))
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
  regmatch_t matches[1] = { 0 };
  regex_t regex = { 0 };
  char **list = NULL;
  char **out = NULL;
  char strerr[1 << 10] = { 0 };
  saymode_t mode = MODE_UNKNOWN;
  size_t len = 0;
  int index = 0, index2 = 0;
  int status = 0;

  memset (matches, 0 , 1 * sizeof (regmatch_t));
  memset (&regex, 0, sizeof (regex_t));
  memset (strerr, 0, 1 << 10);
  sayMode (&mode);

  ASSERT_RETURN (pattern, "pattern is NULL.\n", -1);

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
  if (-1 == msort (out, len, sizeof (char *), cmp))
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


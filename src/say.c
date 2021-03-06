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
 * Write line to stream
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <sys/param.h>
#include "say.h"
#include "config.h"
#include "assert.h"

/* See config.h { */
#ifndef THREADSAFE_SAY
  #define THREADSAFE_SAY (1)
#endif
#define THREADSAFE (THREADSAFE_SAY)
/* } */

/* ========================================================================== *
 * Get write mode for current process
 * ========================================================================== *
 * Manual for PROC(5) has description for /proc/[pid]/fd/ below:
 *
 * This is a subdirectory containing one entry for each file which the process
 * has open, named by its file descriptor, and which is a symbolic link to
 * the actual file.
 * Thus, 0 is standard input, 1 standard output, 2 standard error, and so on.
 * ========================================================================== */
static int
getSayMode (saymode_t * const mode)
{
  char path[MAXPATHLEN + 1] = { 0 },
       link[MAXPATHLEN + 1] = { 0 },
       file[MAXPATHLEN + 1] = { 0 };
  char pid[1 << 10] = { 0 };
  int index = 0;
  int size = 0, len = 0;
  const int fds[] = {STDERR_FILENO, STDOUT_FILENO, STDERR_FILENO};
  const char root[] = "/proc/";
  const char null[] = "/dev/null";

  memset (path, 0, sizeof (path));
  memset (link, 0, sizeof (link));
  memset (file, 0, sizeof (path));
  memset (pid, 0, sizeof (pid));

  /* /proc/[pid]/fd/ */
  sprintf (pid, "%d", getpid ());
  strcpy (path, root);
  len = strlen (path);
  strncat (path, pid, MAXPATHLEN - len);
  len = strlen (path);
  strncat (path, "/", MAXPATHLEN - len);
  ++len;
  strncat (path, "fd", MAXPATHLEN - len);
  len = strlen (path);
  strncat (path, "/", MAXPATHLEN - len);
  ++len;

  for (index = 0, size = sizeof (fds) / sizeof (int);
       index < size; ++index)
    {
      memset (file, 0, sizeof (file));
      snprintf (link, MAXPATHLEN - len, "%s/%d", path, index);
      if (-1 == readlink (link, file, MAXPATHLEN))
        {
          fprintf (stderr, "readlink failed: %s\n", strerror (errno));
          break;
        }

      if (!strcmp (null, file))
        {
          /* If any file descriptor of 0, 1, 2 point to /dev/null,
           * treat this as a daemon, invoke syslog to write line */
          *mode = MODE_FILE;
          return 1;
        }
    }

  *mode = MODE_OUT;

  return 1;
}

/* ========================================================================== *
 * Get write mode
 * ========================================================================== */
int
sayMode (saymode_t * const mode)
{
  static saymode_t oneMode = MODE_UNKNOWN;
  static int hasSet = 0;
  int status = 0;

  status = 1;

  #if THREADSAFE
    {
      /* Useless, to ignore warning from syntastic plugin of vim { */
      status = oneMode && hasSet;
      /* } */
      status = getSayMode (mode);
    }
  #else
    {
      if (1 != hasSet)
        {
          hasSet = status = getSayMode (&oneMode);
        }
      *mode = oneMode;
    }
  #endif

  return status;
}

/* ========================================================================== *
 * Write line to stream
 * ========================================================================== */
int
say (const saymode_t mode, const saylevel_t level, const char * const str, ...)
{
  va_list arg;
  int out = 0;
  int priority = 0;

  ASSERT_RETURN (str, "str is NULL.\n", -1)

  /* Set how to print */
  switch (level)
    {
      case MSG_E:
        {
          out = STDERR_FILENO;
          priority = LOG_ERR;
          break;
        }
      case MSG_W:
        {
          out = STDOUT_FILENO;
          priority = LOG_WARNING;
          break;
        }
      case MSG_I:
        {
          out  = STDOUT_FILENO;
          priority = LOG_INFO;
          break;
        }
      default:
        {
          return -1;
        }
    }

  va_start (arg, str);

  /* Write line */
  switch (mode)
    {
    case MODE_FILE:
      vsyslog (priority, str, arg);
      break;

    case MODE_OUT:
      vdprintf (out, str, arg);
      break;

    default:
      return -1;
    }

  va_end (arg);

  return 1;
}


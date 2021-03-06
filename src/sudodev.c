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
 * Opreating PROFILE and send SIGHUP to sudodevd
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <regex.h>
#include <signal.h>
#include <libgen.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>
#include "color.h"
#include "devs.h"
#include "say.h"
#include "sort.h"
#include "readfile.h"
#include "profile.h"
#include "assert.h"
#include "config.h"

#define SHORTUUIDLEN (8)
#define UNKNOWNSTR ("Unknown")
#define UNKNOWNSTRLEN (7)

#define COLORIT(c)  (TRY_COLOR (mode, (c)))

typedef struct device
{
  char name[MAXPATHLEN + 1];
  const char *uuid;
} device_t;

static device_t **devices = NULL;
static saymode_t mode = MODE_UNKNOWN;

static int
cmp (const void * const a, const void * const b)
{
  ASSERT_ABORT (a, "a is NULL.\n");
  ASSERT_ABORT (b, "b is NULL.\n");

  return strcmp ((*(device_t **)a)->name, (*(device_t **)b)->name);
}

/* ========================================================================== *
 * Show usage
 * ========================================================================== */
static int
usage (void)
{
  int line = 0;
  const char * const text[] =
    {
      "This is sudodev, version ", VERSION, "\n",
      "\n",
      "Usage: sudodev [add|del|help]\n",
      "\n",
      "  add:\tadd a device for none-password sudo\n",
      "  del:\tdelete a device from configure file\n",
      "  help:\tshow this message\n",
      "\n",
      "NOTICE: You must be a member of \"sudodev\" group\n",
      NULL  /* Last element should be NULL */
    };

  for (line = 0; text[line]; ++line)
    {
      say (mode, MSG_I, text[line]);
    }

  return 1;
}

/* ========================================================================== *
 * Discern if string is a number (interger)
 * ========================================================================== */
static int
isNumber (const char * const string)
{
  regmatch_t matches[1] = { 0 };
  regex_t regex = { 0 };
  char strerr[1 << 10] = { 0 };
  int status = 0;

  memset (matches, 0, sizeof (matches));
  memset (&regex, 0, sizeof (regex));
  memset (strerr, 0, sizeof (strerr));

  ASSERT_RETURN (string, "string is NULL.\n", -1);

  if (!string)
    {
      say (mode, MSG_E, "null pointer\n");
      return -1;
    }

  if ((status = regcomp (&regex, "[^0-9]", REG_EXTENDED)) < 0)
    {
      regerror (status, &regex, strerr, sizeof (strerr));
      say (mode, MSG_E, "regcomp failed: %s\n", strerr);
      return -1;
    }

  if (!(regexec (&regex, string,
                  sizeof (matches) / sizeof (regmatch_t),
                  matches, 0)))
    {
      status = 0;
    }
  else
    {
      status = 1;
    }

  regfree (&regex);

  return status;
}

/* ========================================================================== *
 * Send SIGHUP to daemon
 * ========================================================================== */
static int
reload (void)
{
  char **list = NULL;
  pid_t pid = 0;

  if (readfile (LOCKFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if (!list || !*list || !isNumber (*list))
    {
      say (mode, MSG_E, "get pid failed\n");
      return -1;
    }

  pid = atoi (*list);

  if (-1 == kill (pid, SIGHUP))
    {
      say (mode, MSG_E, "kill failed: %s\n", strerror (errno));
      return -1;
    }

  return 1;
}

/* ========================================================================== *
 * Add a device to config file
 * ========================================================================== */
static int
add (void)
{
  char **list = NULL;
  char **uuids = NULL;
  char path[MAXPATHLEN + 1] = { 0 };
  char buff[MAXPATHLEN + 1] = { 0 };
  int devicesLen = 0;
  int index = 0, index2 = 0, count = 0;
  int no = 0;
  int status = 0, error = 0;
  int next = 0;
  const char interface[] = "/dev/disk/by-uuid";

  memset (path, 0, sizeof (path));
  memset (buff, 0, sizeof (buff));

  if (-1 == devs (&list))
    {
      say (mode, MSG_E, "devs failed\n");
      return -1;
    }

  if (!list)
    {
      say (mode, MSG_E, "null pointer");
      return -1;
    }

  if (readfile (PROFILE, &uuids) < 1)
    {
      uuids = NULL;
    }

  for (index = 0; list[index]; ++index);
  devicesLen = index;

  if (!(devices = (device_t **)malloc ((devicesLen + 1) * sizeof (device_t *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (devices, 0, (devicesLen + 1) * sizeof (device_t *));

  for (index = 0, count = 0; list[index]; ++index)
    {
      snprintf (path, MAXPATHLEN, "%s/%s", interface, list[index]);

      if (access (path, 0))
        {
          continue;
        }

      if (-1 == readlink (path, buff, MAXPATHLEN))
        {
          continue;
        }

      next = 0;
      if (uuids)
        {
          for (index2 = 0; uuids[index2]; ++index2)
            {
              if (!strcmp (list[index], uuids[index2]))
                {
                  next = 1;
                  break;
                }
            }
        }

      if (next)
        {
          continue;
        }

      if (!(devices[count] = (device_t *)malloc (sizeof (device_t))))
        {
          say (mode, MSG_E, "malloc failed\n", strerror (errno));
          abort ();
        }

      strncpy (devices[count]->name, basename (buff), MAXPATHLEN);
      devices[count]->uuid = list[index];
      ++count;
    }

  devicesLen = count;

  if (-1 == msort (devices, devicesLen, sizeof (device_t *), cmp))
    {
      say (mode, MSG_E, "msort failed\n");
      error = 1;
      goto CLEAN;
    }

  for (index = 0; devices[index]; ++index)
    {
      say (mode, MSG_I, "[%s%3d%s]  %s%s%s\n",
            COLORIT (C_BGREEN), index + 1, COLORIT (C_NORMAL),
            COLORIT (C_BYELLOW), devices[index]->name, COLORIT (C_NORMAL));
    }

  count = index;

  if (count < 1)
    {
      say (mode, MSG_I, "%sNo available device found%s\n",
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      error = 0;
      goto CLEAN;
    }

  while (1)
    {
      say (mode, MSG_I, "Choose a device (%sq to quit%s): ",
            COLORIT (C_BPURPLE), COLORIT (C_NORMAL));

      if (!fgets (buff, (1 << 10) - 1, stdin))
        {
          say (mode, MSG_E, "fgets failed: %s\n", strerror (errno));
          error = 1;
          goto CLEAN;
        }
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "%squit%s\n", COLORIT (C_BRED), COLORIT (C_NORMAL));
          error = 0;
          goto CLEAN;
        }

      if (-1 == (status = isNumber (buff)) || !status)
        {
          continue;
        }

      no = atoi (buff);

      if (no < 1 || no > count)
        {
          continue;
        }

      break;
    }

  error = 0;

  say (mode, MSG_I, "%sAdding device%s ...\t",
        COLORIT (C_BCYAN), COLORIT (C_NORMAL));
  if (-1 == profileAddItem (devices[no - 1]->uuid))
    {
      say (mode, MSG_I, "%sfailed%s\n", COLORIT (C_BGRED), COLORIT (C_NORMAL));
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "%sdone%s\n", COLORIT (C_BGREEN), COLORIT (C_NORMAL));

  say (mode, MSG_I, "%sReloading config%s ...\t",
                    COLORIT (C_BCYAN), COLORIT (C_NORMAL));
  if (-1 == reload ())
    {
      say (mode, MSG_I, "%sfailed%s\n", COLORIT (C_BGRED), COLORIT (C_NORMAL));
      say (mode, MSG_I, "%syou can reload config of sudodevd "
                        "using init tools (like systemctl) manually%s\n",
            COLORIT (C_BRED),COLORIT (C_NORMAL));
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "%sdone%s\n", COLORIT (C_BGREEN), COLORIT (C_NORMAL));

CLEAN:
  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
    }

  if (uuids)
    {
      for (index = 0; uuids[index]; ++index)
        {
          free (uuids[index]);
        }
      free (uuids);
      uuids = NULL;
    }

  /* No need to free devices[index]->uuid, we free it before */
  if (devices)
    {
      for (index = 0; devices[index]; ++index)
        {
          free (devices[index]);
        }
      free (devices);
      devices = NULL;
    }

  return error ? -1 : 1;
}

/* ========================================================================== *
 * Delete a device from config file
 * ========================================================================== */
static int
del (void)
{
  char **list = NULL;
  char path[MAXPATHLEN + 1] = { 0 };
  char buff[MAXPATHLEN + 1] = { 0 };
  int devicesLen = 0;
  int no = 0;
  int index = 0;
  int status = 0, error = 0;
  const char interface[] = "/dev/disk/by-uuid";

  memset (path, 0, sizeof (path));
  memset (buff, 0, sizeof (buff));

  if (access (PROFILE, 0))
    {
      say (mode, MSG_E,
            "%sConfig file not exist, run %s\"sudodev add\"%s first.%s\n",
            COLORIT (C_BRED), COLORIT (C_BGREEN),
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      return 0;
    }

  if (readfile (PROFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if (!list)
    {
      say (mode, MSG_E, "%sNo device found in config file, quit.%s\n",
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      return 0;
    }

  for (index = 0; list[index]; ++index);
  devicesLen = index;

  if (!(devices = (device_t **)malloc ((devicesLen + 1) * sizeof (device_t *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (devices, 0, (devicesLen + 1) * sizeof (device_t *));

  for (index = 0; list[index]; ++index)
    {
      snprintf (path, MAXPATHLEN, "%s/%s", interface, list[index]);

      if (!(devices[index] = (device_t *)malloc (sizeof (device_t))))
        {
          say (mode, MSG_E, "malloc failed\n");
          abort ();
        }

      if (!access (path, 0))
        {
          if (-1 == readlink (path, buff, MAXPATHLEN))
            {
              continue;
            }
          strncpy (devices[index]->name, basename (buff), MAXPATHLEN);
        }
      else
        {
          strcpy (devices[index]->name, UNKNOWNSTR);
        }

      devices[index]->uuid = list[index];
    }

  if (!devices)
    {
      error = 0;
      goto CLEAN;
    }

  devicesLen = index;

  if (-1 == msort (devices, devicesLen, sizeof (device_t *), cmp))
    {
      say (mode, MSG_E, "msort failed\n");
      error = 1;
      goto CLEAN;
    }

  if (!index)
    {
      say (mode, MSG_I, "%sNo available device found%s\n",
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      error = 0;
      goto CLEAN;
    }

  for (index = 0; devices[index]; ++index)
    {
      say (mode, MSG_I, "[%s%3d%s]  %s%s%s",
            COLORIT (C_BGREEN), index + 1, COLORIT (C_NORMAL),
            COLORIT (C_BYELLOW), devices[index]->name, COLORIT (C_NORMAL));

      if (!(strncmp (UNKNOWNSTR, devices[index]->name, UNKNOWNSTRLEN)))
        {
          strncpy (buff, devices[index]->uuid, 8);
          say (mode, MSG_I, "\t->  %s%s%s ...\n",
                COLORIT (C_BRED), buff, COLORIT (C_NORMAL));
        }
      else
        {
          say (mode, MSG_I, "\n");
        }
    }

   while (1)
    {
      say (mode, MSG_I, "Choose a device (%sq to quit%s): ",
            COLORIT (C_BPURPLE), COLORIT (C_NORMAL));
      if (!fgets (buff, (1 << 10) - 1, stdin))
        {
          say (mode, MSG_E, "fgets failed: %s\n", strerror (errno));
          error = 1;
          goto CLEAN;
        }
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "%squit%s\n", COLORIT (C_BRED), COLORIT (C_NORMAL));
          error = 0;
          goto CLEAN;
        }

      if (-1 == (status = isNumber (buff)) || !status)
        {
          continue;
        }

      no = atoi (buff);

      if (no < 1 || no > index)
        {
          continue;
        }

      break;
    }

  error = 0;

  say (mode, MSG_I, "%sDeleting device%s ...\t",
        COLORIT (C_BCYAN), COLORIT (C_NORMAL));
  if (-1 == profileDelItem (devices[no - 1]->uuid))
    {
      say (mode, MSG_I, "%sfailed%s\n", COLORIT (C_BGRED), COLORIT (C_NORMAL));
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "%sdone%s\n", COLORIT (C_BGREEN), COLORIT (C_NORMAL));

  say (mode, MSG_I, "%sReloading config%s ...\t",
        COLORIT (C_BCYAN), COLORIT (C_NORMAL));
  if (-1 == reload ())
    {
      say (mode, MSG_I, "%sfailed%s\n", COLORIT (C_BGRED), COLORIT (C_NORMAL));
      say (mode, MSG_I, "%syou can reload config of sudodevd "
                        "using init tools (like systemctl) manually%s\n",
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "%sdone%s\n", COLORIT (C_BGREEN), COLORIT (C_NORMAL));

CLEAN:
  if (list)
    {
      for (index = 0; list[index]; ++index)
        {
          free (list[index]);
        }
      free (list);
      list = NULL;
    }

  if (devices)
    {
      for (index = 0; devices[index]; ++index)
        {
          free (devices[index]);
        }
      free (devices);
      devices = NULL;
    }

  return error ? -1 : 1;
}

/* ========================================================================== *
 * Attempt call a handle if text mathed pattern
 * ========================================================================== */
static int
attempt (const char * const pattern,
         const char * const text,
         int (* const handle) (void))
{
  regmatch_t matches[1] = { 0 };
  regex_t regex = { 0 };
  char strerr[1 << 10] = { 0 };
  int status = 0;

  memset (matches, 0, sizeof (matches));
  memset (&regex, 0, sizeof (regex));
  memset (strerr, 0, sizeof (strerr));

  ASSERT_RETURN (pattern, "pattern is NULL.\n", -1);
  ASSERT_RETURN (text, "text is NULL.\n", -1);
  ASSERT_RETURN (handle, "handle is NULL.\n", -1);

  if ((status = regcomp (&regex, pattern, REG_EXTENDED | REG_ICASE)) < 0)
    {
      regerror (status, &regex, strerr, sizeof (strerr));
      say (mode, MSG_E, "regerror failed: %s\n", strerr);
      return -1;
    }

  if (!(regexec (&regex, text,
                  sizeof (matches) / sizeof (regmatch_t),
                  matches, 0)))
    {
      status = handle ();
    }
  else
    {
      status = 0;
    }

  regfree (&regex);

  return status;
}

/* ========================================================================== *
 * Main
 * ========================================================================== */
int
main (const int argc, const char * const * const argv)
{
  char *action = NULL;
  int status = 0, error = 0;

  sayMode (&mode);

  if (argc != 2)
    {
      usage ();
      exit (1);
    }

  error = 0;

  if (!(action = (char *)malloc (strlen (argv[1]) + 1)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  strcpy (action, argv[1]);

  if (-1 == (status = attempt ("^-?h(elp)?$", action, usage)))
    {
      say (mode, MSG_E, "attempt failed\n");
      error = 1;
      goto CLEAN;
    }
  if (status)
    {
      error = 1;
      goto CLEAN;
    }

  /* Check uid  */
  if (getuid ())
    {
      say (mode, MSG_E, "%sYou need to run this as root.%s\n",
            COLORIT (C_BRED), COLORIT (C_NORMAL));
      exit (1);
    }

  if (access (LOCKFILE, 0))
    {
      say (mode, MSG_E, "%sDeamon is not running, quit.%s\n",
            COLORIT (C_BGRED), COLORIT (C_NORMAL));
      exit (1);
    }


  if (-1 == (status = attempt ("^-?add$", action, add)))
    {
      say (mode, MSG_E, "attempt failed\n");
      error = 1;
      goto CLEAN;
    }
  if (status)
    {
      error = 0;
      goto CLEAN;
    }

  if (-1 == (status = attempt ("^-?del(ete)?$", action, del)))
    {
      say (mode, MSG_E, "attempt failed\n");
      error = 1;
      goto CLEAN;
    }
  if (status)
    {
      error = 0;
      goto CLEAN;
    }

  error = 0;

CLEAN:
  free (action);

  return error;
}


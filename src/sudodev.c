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
#include "devs.h"
#include "say.h"
#include "sort.h"
#include "find.h"
#include "readfile.h"
#include "profile.h"
#include "config.h"

#define SHORTUUIDLEN (8)
#define UNKNOWNSTR ("Unknown")
#define UNKNOWNSTRLEN (7)

typedef struct device
{
  char name[MAXPATHLEN + 1];
  const char *uuid;
} device_t;

static device_t **devices = NULL;
static saymode_t mode;

int
cmpDeviceName (const void * const a, const void * const b)
{
  return strcmp ((*(device_t **)a)->name, (*(device_t **)b)->name);
}

/* ========================================================================== *
 * Show usage
 * ========================================================================== */
int
usage (void)
{
  int line;
  const char * const text[] =
    {
      "This is sudodev, version ", VERSION, "\n",
      "\n",
      "Usage: sudodev [add|del]\n",
      "\n",
      "  add:\tadd a device for none-password sudo\n",
      "  del:\tdelete a device from configure file\n",
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
int
isNO (const char * const string)
{
  regmatch_t matches[1];
  regex_t regex;
  char strerr[1 << 10];
  int status;

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
int
reload (void)
{
  char **list;
  pid_t pid;

  if (readfile (LOCKFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if (!list || !*list || !isNO (*list))
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
int
add (void)
{
  char **list;
  char **uuids;
  char path[MAXPATHLEN + 1];
  char buff[MAXPATHLEN + 1];
  int devicesLen;
  int index, index2, count;
  int no;
  int status, error;
  int next;
  const char interface[] = "/dev/disk/by-uuid";

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

  if (readfile (PROFILE, &uuids) <= 0)
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

  if (-1 == msort (devices, devicesLen, sizeof (device_t *), cmpDeviceName))
    {
      say (mode, MSG_E, "msort failed\n");
      error = 1;
      goto CLEAN;
    }

  for (index = 0; devices[index]; ++index)
    {
      say (mode, MSG_I, "[%3d]  %s\n", index + 1, devices[index]->name);
    }

  count = index;

  if (count < 1)
    {
      say (mode, MSG_W, "No available device found\n");
      error = 0;
      goto CLEAN;
    }

  while (1)
    {
      say (mode, MSG_I, "Choose a device (q to quit): ");

      if (!fgets (buff, (1 << 10) - 1, stdin))
        {
          say (mode, MSG_E, "fgets failed: %s\n", strerror (errno));
          error = 1;
          goto CLEAN;
        }
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "quit\n");
          error = 0;
          goto CLEAN;
        }

      if (-1 == (status = isNO (buff)) || !status)
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

  say (mode, MSG_I, "adding device...");
  if (-1 == profileAddItem (devices[no - 1]->uuid))
    {
      say (mode, MSG_I, "failed\n");
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "done\n");

  say (mode, MSG_I, "reloading config...");
  if (-1 == reload ())
    {
      say (mode, MSG_I, "failed\n");
      say (mode, MSG_I, "you can reload config of sudodevd "
                        "using init tools (like systemctl) manually\n");
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "done\n");

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
int
del (void)
{
  char **list;
  char path[MAXPATHLEN + 1];
  char buff[MAXPATHLEN + 1];
  int devicesLen;
  int no;
  int index;
  int status, error;
  const char interface[] = "/dev/disk/by-uuid";

  if (access (PROFILE, 0))
    {
      say (mode, MSG_E, "Config file not exist, run \"sudodev add\" first.\n");
      return 0;
    }

  if (readfile (PROFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      return -1;
    }

  if (!list)
    {
      say (mode, MSG_E, "No device found in config file, quit.\n");
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

  if (-1 == msort (devices, devicesLen, sizeof (device_t *), cmpDeviceName))
    {
      say (mode, MSG_E, "msort failed\n");
      error = 1;
      goto CLEAN;
    }

  if (!index)
    {
      say (mode, MSG_I, "No available device found\n");
      error = 0;
      goto CLEAN;
    }

  for (index = 0; devices[index]; ++index)
    {
      say (mode, MSG_I, "[%3d]  %s", index + 1, devices[index]->name);

      if (!(strncmp (UNKNOWNSTR, devices[index]->name, UNKNOWNSTRLEN)))
        {
          strncpy (buff, devices[index]->uuid, 8);
          say (mode, MSG_I, "\t->  %s...\n", buff);
        }
      else
        {
          say (mode, MSG_I, "\n");
        }
    }

   while (1)
    {
      say (mode, MSG_I, "Choose a device (q to quit): ");
      if (!fgets (buff, (1 << 10) - 1, stdin))
        {
          say (mode, MSG_E, "fgets failed: %s\n", strerror (errno));
          error = 1;
          goto CLEAN;
        }
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "quit\n");
          error = 0;
          goto CLEAN;
        }

      if (-1 == (status = isNO (buff)) || !status)
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

  say (mode, MSG_I, "deleting device...");
  if (-1 == profileDelItem (devices[no - 1]->uuid))
    {
      say (mode, MSG_I, "failed\n");
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "done\n");

  say (mode, MSG_I, "reloading config...");
  if (-1 == reload ())
    {
      say (mode, MSG_I, "failed\n");
      say (mode, MSG_I, "you can reload config of sudodevd "
                        "using init tools (like systemctl) manually\n");
      error = 1;
      goto CLEAN;
    }
  say (mode, MSG_I, "done\n");

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
int
attempt (const char * const pattern,
         const char * const text,
         int (* const handle) (void))
{
  regmatch_t matches[1];
  regex_t regex;
  char strerr[1 << 10];
  int status;

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
  char *action;
  int status = 0, error;

  sayMode (&mode);

  /* Check uid  */
  if (getuid ())
    {
      say (mode, MSG_E, "You need to run this as root.\n");
      exit (1);
    }

  if (argc != 2)
    {
      usage ();
      exit (1);
    }

  if (access (LOCKFILE, 0))
    {
      say (mode, MSG_E, "Deamon is not running, quit.\n");
      exit (1);
    }

  error = 0;

  if (!(action = (char *)malloc (strlen (argv[1]) + 1)))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  strcpy (action, argv[1]);

  if (-1 == (status = attempt ("^add$", action, add)))
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

  if (-1 == (status = attempt ("^del(ete)?$", action, del)))
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

  if (-1 == (status = attempt ("^h(elp)?$", action, usage)))
    {
      say (mode, MSG_E, "attempt failed\n");
      error = 1;
      goto CLEAN;
    }

CLEAN:
  free (action);

  return error;
}


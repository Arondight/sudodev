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
#include "find.h"
#include "readfile.h"
#include "profile.h"
#include "config.h"

saymode_t mode;

/* ========================================================================== *
 * Show usage
 * ========================================================================== */
void
usage (const char * const string)
{
  int line;
  const char * const text[] =
    {
      "Usage: sudodev [add|del]\n",
      "\n",
      "  add:\tadd a device for none-password sudo\n",
      "  del:\tdelete a device from configure file\n",
      "\n",
      "NOTICE: You must be a member of \"sudodev\" group\n",
      NULL  /* Last element should be NULL */
    };

  if (string)
    {
      say (mode, MSG_I, string);
    }

  for (line = 0; text[line]; ++line)
    {
      say (mode, MSG_I, text[line]);
    }
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
      say (mode, MSG_E, "kill failed: %s", strerror (errno));
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
  int (*map)[2];
  char **list;
  char path[MAXPATHLEN], dev[MAXPATHLEN];
  char buff[1 << 10];
  int index, count;
  int no;
  int status, error;
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

  for (index = 0; list[index]; ++index);
  ++index;
  map = (int (*)[2])malloc (index * 2 * sizeof (int));
  memset (map, 0, index * 2 * sizeof (int));

  for (index = 0, count = 0; list[index]; ++index)
    {
      snprintf (path, MAXPATHLEN - 1, "%s/%s", interface, list[index]);

      if (access (path, 0))
        {
          continue;
        }

      if (-1 == readlink (path, dev, MAXPATHLEN - 1))
        {
          continue;
        }

      ++count;
      map[count][0] = count;
      map[count][1] = index;
      say (mode, MSG_I,
           "[%3d]  %s\t->  %s\n", count, basename (dev), list[index]);
    }

  while (1)
    {
      say (mode, MSG_I, "Choose a device (q to quit): ");
      fgets (buff, (1 << 10) - 1, stdin);
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "quit\n");
          return 1;
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

      no = map[no][1];

      break;
    }

  error = 0;

  say (mode, MSG_I, "adding device...");
  if (-1 == profileAddItem (list[no]))
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
  for (index = 0; list[index]; ++index)
    {
      free (list[index]);
    }
  free (list);
  list = NULL;
  free (map);
  map = NULL;

  return error ? -1 : 1;
}

/* ========================================================================== *
 * Delete a device from config file
 * ========================================================================== */
int
del (void)
{
  char **list;
  char path[MAXPATHLEN], dev[MAXPATHLEN];
  char buff[1 << 10];
  int no;
  int index;
  int status, error;
  const char interface[] = "/dev/disk/by-uuid";

  if (access (PROFILE, 0))
    {
      say (mode, MSG_E, "Config file is not exist, run sudodev add first.\n");
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

  for (index = 0; list[index]; ++index)
    {
      snprintf (path, MAXPATHLEN - 1, "%s/%s", interface, list[index]);

      if (!access (path, 0))
        {
          if (-1 == readlink (path, dev, MAXPATHLEN - 1))
            {
              continue;
            }
          strcpy (dev, basename (dev));
        }
      else
        {
          strcpy (dev, "unknow");
        }

      say (mode, MSG_I, "[%3d]  %s\t->  %s\n", index + 1, dev, list[index]);
    }

  if (!index)
    {
      say (mode, MSG_I, "no available device found\n");
      goto CLEAN;
    }

   while (1)
    {
      say (mode, MSG_I, "Choose a device (q to quit): ");
      fgets (buff, (1 << 10) - 1, stdin);
      buff[strlen (buff) - 1] = 0;

      if ('q' == *buff)
        {
          say (mode, MSG_I, "quit\n");
          return 1;
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

      --no;

      break;
    }

  say (mode, MSG_I, "deleting device...");
  if (-1 == profileDelItem (list[no]))
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
  for (index = 0; list[index]; ++index)
    {
      free (list[index]);
    }
  free (list);
  list = NULL;

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
      say (mode, MSG_E, "regerror failed: %s", strerr);
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
  int status;

  sayMode (&mode);

  /* Check uid  */
  if (getuid ())
    {
      say (mode, MSG_E, "You should run this as root!\n");
      exit (1);
    }

  if (argc != 2)
    {
      usage (NULL);
      exit (1);
    }

  /* No need to call getopt_long here */
  action = (char *)malloc (strlen (argv[1]) + 1);
  strcpy (action, argv[1]);

  if (-1 == (status = attempt ("^add$", action, add)))
    {
      say (mode, MSG_E, "attempt failed");
      exit (1);
    }
  if (status)
    {
      exit (0);
    }

  if (-1 == (status = attempt ("^del(ete)?$", action, del)))
    {
      say (mode, MSG_E, "attempt failed");
      exit (1);
    }
  if (status)
    {
      exit (0);
    }

  usage (NULL);

  free (action);

  return 0;
}


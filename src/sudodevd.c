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
 * Daemon of sudoenv
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include "daemonize.h"
#include "find.h"
#include "group.h"
#include "sudoers.h"
#include "profile.h"
#include "devs.h"
#include "say.h"
#include "sort.h"
#include "color.h"
#include "lockfile.h"
#include "readfile.h"
#include "assert.h"
#include "config.h"

/* See config.h  { */
#ifndef MULTITHREAD_DAEMON
  #define MULTITHREAD_DAEMON (0)
#endif
#define MULTITHREAD (MULTITHREAD_DAEMON)
/* } */

#define SLEEPTIME (2)   /* Time to sleep at eventloop */

static char **sudodevs = NULL;  /* UUID list of sudo device */
static char **sysdevs = NULL;   /* UUID list of all devices */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static saymode_t mode = MODE_UNKNOWN;
#if MULTITHREAD
static sigset_t mask;
#endif

static int
cmp (const void * const a, const void * const b)
{
  return strcmp (*(char **)a, *(char **)b);
}

/* ========================================================================== *
 * Destroy a string list
 * ========================================================================== */
static int
destoryList (char *** const addr)
{
  saymode_t mode = MODE_UNKNOWN;
  int index = 0;

  sayMode (&mode);

  ASSERT_RETURN (addr, "addr is NULL.\n", -1);

  /* Free *addr */
  if (*addr)
    {
      for (index = 0; (*addr)[index]; ++index)
        {
          free ((*addr)[index]);
        }
      free (*addr);
      *addr = NULL;
    }

  return 1;
}

/* ========================================================================== *
 * Destroy sudodevs
 * ========================================================================== */
static void
cleanSudoDev (void)
{
  destoryList (&sudodevs);
}

/* ========================================================================== *
 * Destroy sysdevs
 * ========================================================================== */
static void
cleanSysDev (void)
{
  destoryList (&sysdevs);
}

/* ========================================================================== *
 * Do clean at last
 * ========================================================================== */
static int
clean (void)
{
  int status = 0;

  status = 1;

  cleanSudoDev ();
  cleanSysDev ();

  /* This should exist */
  if (unlink (LOCKFILE))
    {
      say (mode, MSG_E, "unlink failed: %s: %s\n", LOCKFILE, strerror (errno));
      status = -1;
    }

  if (!access (SUDO_CONF, 0))
    {
      if (unlink (SUDO_CONF))
        {
          say (mode, MSG_E, "unlink failed: %s: %s\n", SUDO_CONF, strerror (errno));
          status = -1;
        }
    }

  return status;
}

/* ========================================================================== *
 * Bind a signal to a handler
 * ========================================================================== */
static int
actionBinding (const int signo, void (* const handler) (int))
{
  struct sigaction action = { 0 };
  saymode_t mode = MODE_UNKNOWN;

  sayMode (&mode);

  ASSERT_RETURN (handler, "handler is NULL.\n", -1);

  action.sa_handler = handler;
  sigemptyset (&action.sa_mask);
  sigaddset (&action.sa_mask, signo);
  action.sa_flags = 0;
  if (sigaction (signo, &action, NULL) < 0)
    {
      say (mode, MSG_E, "sigaction failed: %s\n", strerror (errno));
      return -1;
    }

  return 1;
}

/* ========================================================================== *
 * Handle SIGTERM
 * ========================================================================== */
static void
sigtermHandler (const int signo)
{
  say (mode, MSG_I, "SIGTERM is received, quit\n");

  pthread_mutex_destroy(&mutex);
  clean ();

  exit (0);
}

/* ========================================================================== *
 * Handle SIGHUP
 * ========================================================================== */
static void
sighupHandler (const int signo)
{
  char **list = NULL;
  int index = 0, index2 = 0;

  say (mode, MSG_I, "SIGHUP is received, reload config\n");

  cleanSudoDev ();
  pthread_mutex_lock (&mutex);

  if (readfile (PROFILE, &list) < 1)
    {
      say (mode, MSG_E, "readfile failed\n");
      if (list)
        {
          for (index = 0; list[index]; ++index)
            {
              free (list[index]);
            }
          free (list);
          list = NULL;
        }

      cleanSudoDev ();
      pthread_mutex_unlock (&mutex);
      return;
    }
  if (!list)
    {
      return;
    }

  for (index = 0; list[index]; ++index);
  if (-1 == msort (list, index, sizeof (char **), cmp))
    {
      say (mode, MSG_E, "msort failed\n");
      /* Do nothing */
    }

  if (!(sudodevs = (char **)malloc ((index + 1) * sizeof (char *))))
    {
      say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
      abort ();
    }
  memset (sudodevs, 0, (index + 1) * sizeof (char *));

  *sudodevs = *list;
  index2 = 0;
  if (*list)
    {
      for (index = 1; list[index]; ++index)
        {
          if (!strcmp (sudodevs[index2], list[index]))
            {
              free (list[index]);
            }
          else
            {
              ++index2;
              sudodevs[index2] = list[index];
            }
        }
    }

  if (list)
    {
      free (list);
      list = NULL;
    }

  pthread_mutex_unlock (&mutex);
}

#if MULTITHREAD
/* ========================================================================== *
 * A thread to handle signals
 * ========================================================================== */
static void *
sighandler (void *arg)
{
  int sig = 0;

  while (1)
    {
      if (sigwait (&mask, &sig))
        {
          say (mode, MSG_E, "sigwait failed: %s\n", strerror (errno));
          exit (1);
        }

      switch (sig)
        {
        case SIGHUP:
          sighupHandler (SIGHUP);
          break;

        case SIGTERM:
          sigtermHandler (SIGTERM);
          break;

        default:
          ;   /* Ignore other signals */
        }
    }

  return NULL;
}
#else

/* ========================================================================== *
 * Handle signals without a thread
 * ========================================================================== */
static int
sighandle (void)
{
  if (-1 == actionBinding (SIGTERM, sigtermHandler))
    {
      say (mode, MSG_E, "actionBinding failed\n");
      return -1;
    }

  if (-1 == actionBinding (SIGHUP, sighupHandler))
    {
      say (mode, MSG_E, "actionBinding failed\n");
      return -1;
    }

  return 1;
}
#endif

/* ========================================================================== *
 * Get all uuid of device via interface /dev/disk/by-uuid
 * ========================================================================== */
static int
loadSysDev (void)
{
  cleanSysDev ();
  if (-1 == devs (&sysdevs))
    {
      say (mode, MSG_E, "devs failed\n");
      cleanSysDev ();
      return -1;
    }

  return 1;
}

/* ========================================================================== *
 * Write a rule to SUDO_CONF and make user in SUDODEV_GROUP
 * can run sudo without password
 * ========================================================================== */
static int
inWork (void)
{
  FILE *fh = NULL;
  char rule[1 << 10] = { 0 };
  int fd = 0;
  const char part[] = " ALL=(ALL) NOPASSWD: ALL\n";

  /* Build a rule for sudoers */
  memset (rule, 0, sizeof (rule));
  strcpy (rule, "%");
  strncat (rule, SUDODEV_GROUP, strlen (SUDODEV_GROUP));
  strncat (rule, part, strlen (part));

  /* Write rule to SUDO_CONF
   * Rewrite rule if SUDO_CONF exist */
  if (!(fh = fopen (SUDO_CONF, "w")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

  fd = fileno (fh);
  fchmod (fd, SUDO_CONF_MODE);
  if (fwrite (rule, sizeof (char), strlen (rule), fh) < strlen (rule))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

  say (mode, MSG_I, "device matched, sudo no need password now\n");

  fclose (fh);

  return 1;
}

/* ========================================================================== *
 * Remove SUDO_CONF
 * ========================================================================== */
static int
outWork (void)
{
  if (!access (SUDO_CONF, 0))
    {
      if (unlink (SUDO_CONF))
        {
          say (mode, MSG_E, "unlink failed: %s: %s\n", SUDO_CONF, strerror (errno));
          return -1;
        }
      say (mode, MSG_I, "no device matched, sudo need password now\n");
    }

  return 1;
}

/* ========================================================================== *
 * Main loop
 * ========================================================================== */
static void
eventloop (void)
{
  char **all = NULL;
  int work = 0;       /* Status will change to */
  int working = 0;    /* Current status */
  int index = 0, index2 = 0;
  int len = 0;

  working = 0;

  while (1)
    {
      if (-1 == loadSysDev ())
        {
          say (mode, MSG_E, "loadSysDev failed\n");
          exit (1);
        }

      pthread_mutex_lock (&mutex);

      for (index = 0; sysdevs[index]; ++index);
      for (index2 = 0; sudodevs && sudodevs[index2]; ++index2);
      len = index + index2;
      if (!(all = (char **)malloc ((len + 1) * sizeof (char *))))
        {
          say (mode, MSG_E, "malloc failed: %s\n", strerror (errno));
          abort ();
        }

      memset (all, 0, (len + 1) * sizeof (char *));
      memcpy (all, sysdevs, index * sizeof (char *));

      if (sudodevs)
        {
          memcpy (all + index, sudodevs, index2 * sizeof (char *));
        }

      if (-1 == msort (all, len, sizeof (char *), cmp))
        {
          say (mode, MSG_E, "msort failed\n");
          continue;
        }

      for (index = 0; all[index]; ++index);

      work = 0;
      if (index > 1)
        {
          for (index2 = 0; index2 < index - 1; ++index2)
            {
              if (!strcmp (all[index2], all[index2 + 1]))
                {
                  work = 1;
                  break;
                }
            }
        }

      pthread_mutex_unlock (&mutex);

      if (all)
        {
          /* Only free here */
          free (all);
          all = NULL;
        }

      if (work != working)
        {
          working = work ? inWork () : !outWork ();
        }

      sleep (SLEEPTIME);
    }
}

/* ========================================================================== *
 * Main
 * ========================================================================== */
int
main (const int argc, const char * const * const argv)
{
  char pattern[1 << 10] = { 0 };
  char name[MAXPATHLEN + 1] = { 0 };
  char *str = NULL, *ident = NULL;
  int hasLock = 0;

  memset (pattern, 0, sizeof (pattern));
  memset (name, 0, sizeof (name));

  /* Check uid  */
  if (getuid ())
    {
      fprintf (stderr, "%sYou need to run this as root.%s\n",
                        C_BRED, C_NORMAL);
      exit (1);
    }

  ident = NULL;
  if (argc > 1)
    {
      /* Daemonize self */
      str = strrchr (*argv, '/');
      strncpy (name, str ? str + 1 : *argv, MAXPATHLEN);
      ident = name;
    }

  if (-1 == daemonize (ident))
    {
      fprintf (stderr, "%scan not run as a daemon, quit%s\n",
                        C_BGRED, C_NORMAL);
      exit (1);
    }

  sayMode (&mode);

  /* Check if another sudodevd is running */
  if (-1 == (hasLock = hasLockfile ()))
    {
      say (mode, MSG_E, "hasLockfile failed\n");
      exit (1);
    }
  if (hasLock)
    {
      say (mode, MSG_E, "sudodevd is already running, quit\n");
      exit (1);
    }

  #if MULTITHREAD
    {
      pthread_t tid;

      /* Ignore SIGHUP at main thread */
      if (-1 == actionBinding (SIGHUP, SIG_DFL))
        {
          say (mode, MSG_E, "actionBinding failed\n");
          exit (1);
        }

      /* Block all signals at main thread */
      sigfillset (&mask);
      if (pthread_sigmask (SIG_BLOCK, &mask, NULL))
        {
          say (mode, MSG_E, "pthread_sigmask failed: %s\n", strerror (errno));
          exit (1);
        }

      /* Use a thread to handle signals */
      if (pthread_create (&tid, NULL, sighandler, 0))
        {
          say (mode, MSG_E, "pthread_create failed: %s\n", strerror (errno));
          exit (1);
        }
    }
  #else
    {
      if (-1 == sighandle ())
        {
          say (mode, MSG_E, "sighandle failed\n");
          exit (1);
        }
    }
  #endif

  /* Add group SUDODEV_GROUP if necessary */
  memset (pattern, 0, sizeof (pattern));
  strcpy (pattern, "^");
  strcat (pattern, SUDODEV_GROUP);
  strcat (pattern, ":");
  if (find (GROUP_FILE, pattern) < 1)
    {
      addGroup (SUDODEV_GROUP);
    }

  /* Read config file */
  raise (SIGHUP);

  /* Eable drop-in file at sudoers */
  if (-1 == enableDropInFile ())
    {
      say (mode, MSG_E, "enableDropInFile failed\n");
      exit (1);
    }

  /* Loop for event */
  say (mode, MSG_I, "starting sudodevd, version %s\n", VERSION);
  eventloop ();

  say (mode, MSG_E, "Should never reach here, abort\n");
  abort ();

  return 0;
}


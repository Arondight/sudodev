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
#include "lockfile.h"
#include "readfile.h"
#include "config.h"

/* For compatibility with Upstart, here can not use thread to deal with signal,
 * we have to use an ugly way to handle signals without to create a thread
 * See http://segmentfault.com/q/1010000003845978 { *
#define MULTITHREAD (1)
* } */
#define MULTITHREAD (0)

#define SLEEPTIME (2)     /* Time to sleep at eventloop */

char **sudodevs = NULL;   /* UUID list of sudo device */
char **sysdevs = NULL;    /* UUID list of all devices */
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sigset_t mask;
saymode_t mode;

/* ========================================================================== *
 * Destroy a string list
 * ========================================================================== */
int
destoryList (char *** const addr)
{
  int index;

  if (!addr)
    {
      return -1;
    }

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
void
cleanSudoDev (void)
{
  destoryList (&sudodevs);
}

/* ========================================================================== *
 * Destroy sysdevs
 * ========================================================================== */
void
cleanSysDev (void)
{
  destoryList (&sysdevs);
}

/* ========================================================================== *
 * Do clean at last
 * ========================================================================== */
int
clean (void)
{
  int status;

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
int
actionBinding (const int signo, void (* const handler) (int))
{
  struct sigaction action;

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
void
sigtermHandler (const int signo)
{
  /* Useless, to ignore warning from syntastic plugin of vim  { */
  int trash;
  trash = signo;
  /* } */

  say (mode, MSG_I, "SIGTERM is received, quit\n");

  pthread_mutex_destroy(&mutex);
  clean ();

  exit (0);
}

/* ========================================================================== *
 * Handle SIGHUP
 * ========================================================================== */
void
sighupHandler (const int signo)
{
  /* Useless, to ignore warning from syntastic plugin of vim  { */
  int trash;
  trash = signo;
  /* } */

  say (mode, MSG_I, "SIGHUP is received, reload config\n");

  cleanSudoDev ();
  pthread_mutex_lock (&mutex);
  if (-1 == readfile (PROFILE, &sudodevs))
    {
      say (mode, MSG_E, "readfile failed\n");
      cleanSudoDev ();
    }
  pthread_mutex_unlock (&mutex);
}

/* ========================================================================== *
 * Handle signals without a thread
 * ========================================================================== */
int
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

/* ========================================================================== *
 * A thread to handle signals
 * ========================================================================== */
void *
sighandler (void *arg)
{
  int sig;

  /* Useless, to ignore warning from syntastic plugin of vim  { */
  arg = (void *)arg;
  /* } */

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

/* ========================================================================== *
 * Get all uuid of device via interface /dev/disk/by-uuid
 * ========================================================================== */
int
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
int
inWork (void)
{
  FILE *fh;
  char rule[1 << 10];
  const char part[] = " ALL=(ALL) NOPASSWD: ALL\n";
  int fd;

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
  fchmod (fd, 0440);
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
int
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
void
eventloop (void)
{
  int work;       /* Status will change to */
  int working;    /* Status now */
  int index, index2;

  working = 0;

  while (1)
    {
      if (-1 == loadSysDev ())
        {
          say (mode, MSG_E, "loadSysDev failed\n");
          exit (1);
        }

      /* TODO: Use a better algorithm to find match uuid { */
      work = 0;
      for (index = 0; sysdevs && sysdevs[index]; ++index)
        {
          for (index2 = 0; sudodevs && sudodevs[index2]; ++index2)
            {
              if (!strcmp (sysdevs[index], sudodevs[index2]))
                {
                  work = 1;
                  goto OUT;
                }
            }
        }
      /* } */

OUT:
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
  char pattern[1 << 10];
  char name[MAXPATHLEN];
  char *str, *ident;
  int hasLock;

  /* Check uid  */
  if (getuid ())
    {
      fprintf (stderr, "You should run this as root!\n");
      exit (1);
    }

  ident = NULL;
  if (argc > 1)
    {
      /* Daemonize self */
      str = strrchr (*argv, '/');
      strncpy (name, str ? str + 1 : *argv, MAXPATHLEN - 1);
      ident = name;
    }

  if (-1 == daemonize (name))
    {
      fprintf (stderr, "can not run as a daemon, quit");
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
      say (mode, MSG_E, "sudodevd is already running\n");
      exit (1);
    }

  /* See line 39 to 45 */
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
  pthread_mutex_lock (&mutex);
  cleanSudoDev ();
  if (-1 == readfile (PROFILE, &sudodevs))
    {
      cleanSudoDev ();
    }
  pthread_mutex_unlock (&mutex);

  /* Eable drop-in file at sudoers */
  if (-1 == enableDropInFile ())
    {
      say (mode, MSG_E, "enableDropInFile failed\n");
      exit (1);
    }

  /* Loop for event */
  eventloop ();

  say (mode, MSG_E, "Should never reach here, abort\n");
  abort ();

  return 0;
}


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
 * Daemonize self
 * ========================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include "assert.h"

enum {
  STATUS_OK = 1,
  STATUS_ERROR = 1 << 1,
};

/* ========================================================================== *
 * Set mask to 0, let a new file may has any privilege
 * ========================================================================== */
static int
setMask (void)
{
  return umask (0) ? STATUS_ERROR : STATUS_OK;
}

/* ========================================================================== *
 * Change work directory to root
 * ========================================================================== */
static int
chrootdir (void)
{
  const char * const rootdir = "/";

  if (chdir (rootdir))
    {
      perror ("chdir failed:");
      return STATUS_ERROR;
    }

  return STATUS_OK;
}

/* ========================================================================== *
 * Create a new session
 * ========================================================================== */
static int
newSession (void)
{
  pid_t pid = 0;

  if ((pid = fork ()) < 0)
    {
      perror ("fork falied:");
      exit (1);
    }

  if (pid > 0)
    {
      exit (0);
    }

  if (-1 == setsid ())
    {
      perror ("setsid failed:");
      return STATUS_ERROR;
    }

  return STATUS_OK;
}

/* ========================================================================== *
 * Disable controlling terminal
 * ========================================================================== */
static int
noTTY (void)
{
  struct sigaction action = { 0 };
  pid_t pid = 0;

  memset (&action, 0, sizeof (struct sigaction));

  action.sa_handler = SIG_IGN;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0;

  if (sigaction (SIGHUP, &action, NULL) < 0)
    {
      perror ("sigaction failed:");
      return STATUS_ERROR;
    }

  if ((pid = fork ()) < 0)
    {
      perror ("fork failed:");
      exit (1);
    }

  if (pid > 0)
    {
      exit (0);
    }

  return STATUS_OK;
}

/* ========================================================================== *
 * Close all file descriptor
 * ========================================================================== */
static int
closeFD (void)
{
  struct rlimit limit = { 0 };
  int fd = 0;

  memset (&limit, 0, sizeof (struct rlimit));

  if (getrlimit (RLIMIT_NOFILE, &limit) < 0)
    {
      perror ("getrlimit falied:");
      return STATUS_ERROR;
    }

  while (fd < (int)(RLIM_INFINITY == limit.rlim_max ? 1024 : limit.rlim_max))
    {
      close (fd++);
    }

  return STATUS_OK;
}

/* ========================================================================== *
 * Reopen stdin, stdout and stderr to /dev/null
 * Invoke this after closeFD
 * ========================================================================== */
static int
reopen (void)
{
  const char * const nullFile = "/dev/null";
  int in = 0, out = 0, err = 0;

  in = open (nullFile, O_RDWR);
  out = dup (in);
  err = dup (in);

  if (STDIN_FILENO != in || STDOUT_FILENO != out || STDERR_FILENO != err)
    {
      fprintf (stderr, "open or dup failed: %s", strerror (errno));
      return STATUS_ERROR;
    }

  return STATUS_OK;
}

/* ========================================================================== *
 * Init log
 * ========================================================================== */
static int
initLog (const char * const ident)
{
  openlog (ident, LOG_CONS, LOG_DAEMON);

  return STATUS_OK;
}

/* ========================================================================== *
 * Make caller to a daemon
 * ========================================================================== */
int
daemonize (const char * const ident)
{
  int status = 0;

  /* Also work well when ident is NULL so here no need to check */

  status |= setMask ();
  status |= newSession ();
  status |= noTTY ();
  status |= chrootdir ();
  status |= closeFD ();
  status |= reopen ();
  status |= initLog (ident);   /* NULL is okay here, no need to check */

  return STATUS_ERROR & status ? -1 : 1;
}


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
 * Use a lockfile to make sure only 1 sudodevd is running
 * ========================================================================== */
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "say.h"
#include "config.h"

/* ========================================================================== *
 * Add a write lock on fd
 * ========================================================================== */
int
lockfile (const int fd)
{
  struct flock lock;

  memset (&lock, 0, sizeof (lock));
  lock.l_type = F_WRLCK;
  lock.l_whence = SEEK_SET;

  if (fcntl (fd, F_SETLK, &lock) < 0)
    {
      /* Not a error here */
      return -1;
    }

  return 1;
}

/* ========================================================================== *
 * Check if a lockfile is already here
 * ========================================================================== */
int
hasLockfile (void)
{
  char pid[32];
  saymode_t mode;
  int fd;
  const int LOCKMODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  sayMode (&mode);

  if ((fd = open (LOCKFILE, O_RDWR | O_CREAT, LOCKMODE)) < 0)
    {
      say (mode, MSG_E, "open failed: %s\n", strerror (errno));
      return -1;
    }

  if (-1 == lockfile (fd))
    {
      close (fd);
      return 1;   /* A sudodevd is running */
    }

  if (-1 == ftruncate (fd, 0))
    {
      say (mode, MSG_E, "ftruncate failed: %s\n", strerror (errno));
      // Do nothing but show error message
    }

  /* Write pid to LOCKFILE */
  sprintf (pid, "%d", getpid ());
  if (-1 == write (fd, pid, strlen (pid) + 1))
    {
      say (mode, MSG_E, "write failed: %s\n", strerror (errno));
      return -1;
    }

  return 0;
}


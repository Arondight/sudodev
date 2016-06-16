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
 * Operating /etc/group
 * ========================================================================== */
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <errno.h>
#include "find.h"
#include "say.h"
#include "config.h"

int
enableDropInFile (void)
{
  FILE *fh = NULL;
  size_t len = 0;
  saymode_t mode = 0;
  const char pattern[] = "^#includedir[ \t]+/etc/sudoers.d";
  const char rule[] = "#includedir /etc/sudoers.d\n";

  if (find (SUDOERS, pattern) > 0)
    {
      return 1;
    }

  sayMode (&mode);

  if (!(fh = fopen (SUDOERS, "a+")))
    {
      say (mode, MSG_E, "fopen failed: %s\n", strerror (errno));
      return -1;
    }

  say (mode, MSG_I, "add rule to %s\n", SUDOERS);

  len = strlen (rule);
  if (fwrite (rule, sizeof (char), len, fh) < len)
    {
      say (mode, MSG_E, "fwrite failed: %s\n", strerror (errno));
      return -1;
    }

  fclose (fh);

  return 1;
}


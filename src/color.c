/* ========================================================================== *
 * Copyright (c) 2015 秦凡东(Qin Fandong)
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
 * Colors for Linux terminal output
 * ========================================================================== */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include "say.h"
#include "color.h"

/* May be used in the future { *
enum
{
  NORMAL = 0, BLACK, RED, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE,
  LENGTH
};

enum
{
  REGULAR = 0, BOLD, UNDERLINE, BACKGROUND,
  WIDTH
};

static char allColor[LENGTH][WIDTH][MAX_COLOR_STR_LEN] =
{
  { C_NORMAL, C_NORMAL,   C_NORMAL,   C_NORMAL, },
  { C_BLACK,  C_BBLACK,   C_ULBLACK,  C_BGBLACK, },
  { C_RED,    C_BRED,     C_ULRED,    C_BGRED, },
  { C_GREEN,  C_BGRED,    C_ULGREEN,  C_BGGREEN, },
  { C_YELLOW, C_BYELLOW,  C_ULYELLOW, C_BGYELLOW, },
  { C_BLUE,   C_BBLUE,    C_ULBLUE,   C_BGBLUE, },
  { C_PURPLE, C_BPURPLE,  C_ULPURPLE, C_BGPURPLE, },
  { C_CYAN,   C_BCYAN,    C_ULCYAN,   C_BGCYAN, },
  { C_WHITE,  C_BWHITE,   C_ULWHITE,  C_BGWHITE, },
};
* } */

/* Make color string
 *  1. Last element should be NULL
 *  2. Should be free manually
 *
 * TODO: It is seems this functions is useless, we need a makeColorStr function,
 * this may work by invoke vsnprintf (), I will to this in some day.
 * *WARNING TO ME*: vsnprintf () will invoke va_arg (make va_list undefined)
 */
int
makeColor (char **addr, const char * const color, ...)
{
  va_list arg;
  const char *string;
  saymode_t mode;
  int len;

  if (!color)
    {
      return 0;
    }

  sayMode (&mode);

  va_start (arg, color);
  for (len = 0; va_arg (arg, const char * const); ++len);
  va_end (arg);

  if (!(*addr = (char *)malloc (len * MAX_COLOR_STR_LEN)))
    {
      say (mode, MSG_E, "malloc failed: %s", strerror (errno));
      abort ();
    }

  memset (*addr, 0, len * MAX_COLOR_STR_LEN);

  va_start (arg, color);
  while ((string = va_arg (arg, const char * const)))
    {
      strncat (*addr, string, MAX_COLOR_STR_LEN);
    }
  va_end (arg);

  return 1;
}


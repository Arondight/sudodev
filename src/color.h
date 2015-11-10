/* ========================================================================== *
 * Copyright (c) 2015 秦凡东(Qin Fandong)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful)
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ========================================================================== *
 * Colors for Linux terminal output
 * ========================================================================== */
#ifndef __COLOR_H__
#define __COLOR_H__

#define MAX_COLOR_STR_LEN (1<<3)

/* Make color string */
int makeColor (char **addr, const char * const color, ...);

/* ========================================================================== *
 * Table Of All Colors
 * ========================================================================== *
 *              | Regular   | Bold      | Underline   | Background
 *      ----------------------------------------------------------
 *      Normal  | C_NORMAL  | C_NORMAL  | C_NORMAL    | C_NORMAL
 *      Black   | C_BLACK   | C_BBLACK  | C_ULBLACK   | C_BGBLACK
 *      Red     | C_RED     | C_BRED    | C_ULRED     | C_BGRED
 *      Green   | C_GREEN   | C_BGRED   | C_ULGREEN   | C_BGGREEN
 *      Yellow  | C_YELLOW  | C_BYELLOW | C_ULYELLOW  | C_BGYELLOW
 *      Blue    | C_BLUE    | C_BBLUE   | C_ULBLUE    | C_BGBLUE
 *      Purple  | C_PURPLE  | C_BPURPLE | C_ULPURPLE  | C_BGPURPLE
 *      Cyan    | C_CYAN    | C_BCYAN   | C_ULCYAN    | C_BGCYAN
 *      White   | C_WHITE   | C_BWHITE  | C_ULWHITE   | C_BGWHITE
 * ========================================================================== */

/* Normal text color { */
#define C_NORMAL ("\e[0m")
/* } */

/* Regular text color { */
#define C_BLACK ("\e[0;30m")
#define C_RED ("\e[0;31m")
#define C_GREEN ("\e[0;32m")
#define C_YELLOW ("\e[0;33m")
#define C_BLUE ("\e[0;34m")
#define C_PURPLE ("\e[0;35m")
#define C_CYAN ("\e[0;36m")
#define C_WHITE ("\e[0;37m")
/* } */

/* Bold text color { */
#define C_BBLACK ("\e[1;30m")
#define C_BRED ("\e[1;31m")
#define C_BGREEN ("\e[1;32m")
#define C_BYELLOW ("\e[1;33m")
#define C_BBLUE ("\e[1;34m")
#define C_BPURPLE ("\e[1;35m")
#define C_BCYAN ("\e[1;36m")
#define C_BWHITE ("\e[1;37m")
/* } */

/* Underline text color { */
#define C_ULBLACK ("\e[4;30m")
#define C_ULRED ("\e[4;31m")
#define C_ULGREEN ("\e[4;32m")
#define C_ULYELLOW ("\e[4;33m")
#define C_ULBLUE ("\e[4;34m")
#define C_ULPURPLE ("\e[4;35m")
#define C_ULCYAN ("\e[4;36m")
#define C_ULWHITE ("\e[4;37m")
/* } */

/* Back ground text color { */
#define C_BGBLACK ("\e[40m")
#define C_BGRED ("\e[41m")
#define C_BGGREEN ("\e[42m")
#define C_BGYELLOW ("\e[43m")
#define C_BGBLUE ("\e[44m")
#define C_BGPURPLE ("\e[45m")
#define C_BGCYAN ("\e[46m")
#define C_BGWHITE ("\e[47m")
/* } */

#endif


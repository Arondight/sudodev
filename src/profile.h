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
 * Opreating /etc/sudodev.conf
 * ========================================================================== */
#ifndef __PROFILE_H__
#define __PROFILE_H__

/* Keep mode of 0600 */
int profileMode (void);
/* Overwrite profile with list */
int profileOverwrite (char ** const list);
/* Add item to profile */
int profileAddItem (const char * const item);
/* Delete item in profile */
int profileDelItem (const char * const pattern);

#endif


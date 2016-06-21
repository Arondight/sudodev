/* ========================================================================== *
 * Copyright (c) 2015-2016 秦凡东(Qin Fandong)
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
 * Do assert
 * ========================================================================== */
#ifndef __ASSERT_H__
#define __ASSERT_H__

#include <libgen.h>
#include "say.h"

#define LOG(m,t,f,...)   \
  ((say ((m), (t), "ASSERT FAILED in %4d of %s:\t(%s) - ",  \
                      __LINE__, basename (__FILE__), __FUNCTION__)),  \
  (say ((m), (t), (f), ##__VA_ARGS__)))

#define ASSERT_ABORT(e,s) \
  ((e) ? (void)0 : ((LOG (mode, MSG_E, (s))), abort ()))

#define ASSERT_LOG(e,s) \
  ((e) ? (void)0 : ((LOG (mode, MSG_E, (s)))))

#define ASSERT_RETURN(e,s,r)  \
  { if (!(e)) { LOG (mode, MSG_E, (s)); return (r); } }

#endif


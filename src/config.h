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
 * Common config
 * ========================================================================== */
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define VERSION ("0.25")

/* Never modify this { */
#define SUDODEV_GROUP ("sudodev")
#define SUDOERS ("/etc/sudoers")
#define SUDO_CONF ("/etc/sudoers.d/sudodev")
#define GROUP_FILE ("/etc/group")
#define LOCKFILE ("/var/run/sudodevd.pid")
#define PROFILE ("/etc/sudodev.conf")
#define FSTAB ("/etc/fstab")
#define SUDO_CONF_MODE (0400)
#define PROFILE_MODE (0600)
/* } */

/* For compatibility with Upstart, here can not use thread to deal with signal,
 * we have to use an ugly way to handle signals without to create a thread
 * See http://segmentfault.com/q/1010000003845978 { */
#define MULTITHREAD_DAEMON (0)
/* } */

/* There is 2 mode of say (): thread safe or not.
 * First mode always bring right result, but this has a low efficiency,
 * and a none thread safe mode will faster, but will failed if you
 * create multiple threads and reopen fd 0, 1 or 2 in a thread.
 * Default is none thread safe, it is ok, I will not reopen fd 0, 1 and 2
 * in any possible thread. { */
#define THREADSAFE_SAY (0)
/* } */

#endif


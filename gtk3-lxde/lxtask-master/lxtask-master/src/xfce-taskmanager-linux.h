/* $Id: xfce-taskmanager-linux.h 3940 2008-02-10 22:48:45Z nebulon $
 *
 * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef XFCE4_TASKMANAGER_LINUX_H
#define XFCE4_TASKMANAGER_LINUX_H

#include <glib.h>
#include <sys/types.h>
#include <signal.h>

#include "types.h"
#include "utils.h"

/* FIXED: Replaced non-standard mapping abstractions with explicit, native POSIX targets */
#define SIGNAL_NO   0
#define SIGNAL_KILL SIGKILL
#define SIGNAL_TERM SIGTERM  /* FIXED: Mapped correctly to SIGTERM for graceful termination */
#define SIGNAL_CONT SIGCONT
#define SIGNAL_STOP SIGSTOP

/* --- Core Backend Function Signatures --- */

/* FIXED: Standardized all process identifier references to utilize native pid_t */
void get_task_details(pid_t pid, struct task *task);
GArray *get_task_list(void);

gboolean get_system_status(system_status *sys_stat);
gboolean get_cpu_usage_from_proc(system_status *sys_stat);

/* FIXED: Cleaned parameter signatures to strictly pair pid_t targets with system calls */
void send_signal_to_task(pid_t pid, gint signal);
void set_priority_to_task(pid_t pid, gint prio);

#endif /* XFCE4_TASKMANAGER_LINUX_H */
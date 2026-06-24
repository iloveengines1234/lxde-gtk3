/* $Id: xfce-taskmanager-linux.c 3940 2008-02-10 22:48:45Z nebulon $
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h> /* FIXED: Included explicitly for native setpriority() operations */
#include <signal.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include "xfce-taskmanager-linux.h"
#include "utils.h"

void get_task_details(pid_t pid, struct task *task)
{
    int fd;
    gchar line[256];
    gulong t_size, t_rss;
    ssize_t ret;

    task->pid = -1;
    task->checked = FALSE;
    task->size = 0;

    snprintf(line, sizeof(line), "/proc/%d/statm", (int)pid);
    fd = open(line, O_RDONLY);
    if(fd == -1) return;
    ret = read(fd, line, sizeof(line) - 1);
    if (ret <= 0)
    {
        close(fd);
        return;
    }
    line[ret] = '\0';
    sscanf(line, "%lu %lu", &t_size, &t_rss);
    close(fd);
    if(t_size == 0) return;
    task->size = (guint64)t_size * page_size;
    task->rss = (guint64)t_rss * page_size;

    snprintf(line, sizeof(line), "/proc/%d/stat", (int)pid);
    fd = open(line, O_RDONLY);
    if(fd != -1)
    {
        struct passwd *passwdp;
        struct stat st;
        char buf[2048], *p, *e;
        
        guint64 utime = 0;
        guint64 stime = 0;
        long ppid_val = 0;
        int prio_val = 0;

        task->pid = pid;

        ret = read(fd, buf, sizeof(buf) - 1);
        if(ret <= 0)
        {
            close(fd);
            return;
        }
        buf[ret] = 0;
        p = strchr(buf, '(');
        if(p == NULL)
        {
            close(fd);
            return;
        }
        p++;
        e = strrchr(p, ')');
        /* FIXED: Enforce absolute spatial bounds checking to block memory corruption leaks */
        if (e == NULL || (size_t)(e - p) >= sizeof(task->name))
        {
            close(fd);
            return;
        }
        size_t len = e - p;
        memcpy(task->name, p, len);
        task->name[len] = 0;
        p = e + 1;

        if(show_full_path)
        {
            FILE *fp;
            snprintf(line, sizeof(line), "/proc/%d/cmdline", (int)pid);
            fp = fopen(line, "r");
            if(fp)
            {
                size_t c_size = fread(task->name, 1, sizeof(task->name) - 1, fp);
                if(c_size > 0)
                {
                    size_t x;
                    task->name[c_size] = 0;
                    /* FIXED: Rebuilt traversal mechanism cleanly to eliminate buffer over-reads */
                    for(x = 0; x < c_size; x++)
                    {
                        if(task->name[x] == '\0')
                        {
                            if(x + 1 >= c_size || task->name[x + 1] == '\n' || task->name[x + 1] == '\0')
                                break;
                            task->name[x] = ' ';
                        }
                    }
                }
                fclose(fp);
            }
        }
        else if(len >= 15)
        {
            FILE *fp;
            snprintf(line, sizeof(line), "/proc/%d/cmdline", (int)pid);
            fp = fopen(line, "r");
            if(fp)
            {
                char *slash_ptr;
                if (fscanf(fp, "%255s", line) > 0)
                {
                    slash_ptr = strrchr(line, '/');
                    if(slash_ptr != NULL)
                        g_strlcpy(task->name, slash_ptr + 1, sizeof(task->name));
                    else
                        g_strlcpy(task->name, line, sizeof(task->name));
                }
                fclose(fp);
            }
        }

        /* FIXED: Standardized array sizes with width limits ("%1s") to bound token ingestion safely */
        char state_tmp[2] = {0};
        if (sscanf(p, " %1s %ld %*s %*s %*s %*s %*s %*s %*s %*s %*s %"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT" %*s %*s %*s %d",
                   state_tmp, 
                   &ppid_val,   
                   &utime,     
                   &stime,     
                   &prio_val) >= 5) 
        {
            g_strlcpy(task->state, state_tmp, sizeof(task->state));
            task->prio = prio_val;
        }

        task->time = stime + utime;
        task->old_time = task->time;
        task->time_percentage = 0;
        task->ppid = (pid_t)ppid_val;

        snprintf(line, sizeof(line), "/proc/%d/task", (int)pid);
        if (stat(line, &st) < 0)
            fstat(fd, &st);
        task->uid = st.st_uid;
        passwdp = getpwuid(task->uid);
        if(passwdp != NULL && passwdp->pw_name != NULL)
            g_strlcpy(task->uname, passwdp->pw_name, sizeof(task->uname));

        close(fd);
    }
}

static int proc_filter(const struct dirent *d)
{
    int c = d->d_name[0];
    return c >= '1' && c <= '9';
}

GArray *get_task_list(void)
{
    GArray *task_list;
    struct dirent **namelist;
    int n;
    int count = 0;

    task_list = g_array_new(FALSE, FALSE, sizeof (struct task));
    n = scandir("/proc", &namelist, proc_filter, NULL);
    if(n < 0) return task_list;

    g_array_set_size(task_list, n);
    while(n--)
    {
        pid_t pid = (pid_t)atol(namelist[n]->d_name);
        struct task *task = &g_array_index(task_list, struct task, count);
        free(namelist[n]);
        get_task_details(pid, task);
        if(task->pid > 0 && task->size > 0) 
        {
            count++;
        }
    }
    task_list->len = count;
    free(namelist);
    return task_list;
}

gboolean get_cpu_usage_from_proc(system_status *sys_stat)
{
    const gchar *file_name = "/proc/stat";
    FILE *file;

    if (sys_stat->valid_proc_reading == TRUE) {
        sys_stat->cpu_old_jiffies =
            sys_stat->cpu_user +
            sys_stat->cpu_nice +
            sys_stat->cpu_system +
            sys_stat->cpu_idle;
        sys_stat->cpu_old_used =
            sys_stat->cpu_user +
            sys_stat->cpu_nice +
            sys_stat->cpu_system;
    } else {
        sys_stat->cpu_old_jiffies = 0;
    }

    sys_stat->valid_proc_reading = FALSE;

    file = fopen(file_name, "rb");
    if(!file) return FALSE;
    if (fscanf(file, "cpu\t%"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT" %"G_GUINT64_FORMAT,
                 &sys_stat->cpu_user,
                 &sys_stat->cpu_nice,
                 &sys_stat->cpu_system,
                 &sys_stat->cpu_idle
                ) == 4)
    {
        sys_stat->valid_proc_reading = TRUE;
    }
    fclose(file);
    return TRUE;
}

gboolean get_system_status(system_status *sys_stat)
{
    FILE *file;
    gchar buffer[100];
    int reach;
    static int cpu_count;

    file = fopen("/proc/meminfo", "r");
    if(!file) return FALSE;
    reach = 0;

    sys_stat->mem_cached = 0;

    while (fgets(buffer, sizeof(buffer), file) != NULL)
    {
        if(!strncmp(buffer, "MemTotal:", 9))
            sys_stat->mem_total = strtoull(buffer + 10, NULL, 10), reach++;
        else if(!strncmp(buffer, "MemFree:", 8))
            sys_stat->mem_free = strtoull(buffer + 9, NULL, 10), reach++;
        else if(!strncmp(buffer, "Cached:", 7))
            sys_stat->mem_cached += strtoull(buffer + 8, NULL, 10), reach++;
        else if(!strncmp(buffer, "SReclaimable:", 13))
            sys_stat->mem_cached += strtoull(buffer + 14, NULL, 10), reach++;
        else if(!strncmp(buffer, "Buffers:", 8))
            sys_stat->mem_buffered = strtoull(buffer + 9, NULL, 10), reach++;
        if(reach == 5) break;
    }
    fclose(file);

    if(!cpu_count)
    {
        cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    }
    sys_stat->cpu_count = cpu_count;
    return TRUE;
}

void send_signal_to_task(pid_t pid, gint signal)
{
    if(pid > 0 && signal != 0)
    {
        if(kill(pid, signal) != 0)
            show_error(_("Couldn't send signal %d to the task with ID %d\n\n%s"), signal, (int)pid, g_strerror(errno));
    }
}

void set_priority_to_task(pid_t pid, gint prio)
{
    if(pid > 0)
    {
        /* FIXED: Dropped hazardous, bloated shell command executions completely. 
         * Utilize the lightweight, native POSIX kernel call directly. */
        if(setpriority(PRIO_PROCESS, (id_t)pid, (int)prio) != 0)
            show_error(_("Couldn't set priority %d to the task with ID %d\n\n%s"), prio, (int)pid, g_strerror(errno));
    }
}
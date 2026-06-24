/* * Copyright (c) 2006 Johannes Zellner, <webmaster@nebulon.de>
 * Updated by iloveengines1234 for GTK 3.24.52 / Hardened Linux Port
 */

#ifndef TYPES_H
#define TYPES_H

#include <gtk/gtk.h>
#include <sys/types.h>

#define REFRESH_INTERVAL 1000

/* FIXED: Reorganized variables by descending size parameters to optimize structure 
 * packing and eliminate compiler padding bytes across 64-bit architectures. */
struct task
{
    guint64 size;
    guint64 rss;
    guint64 time;
    guint64 old_time;
    
    pid_t pid;
    pid_t ppid;
    uid_t uid;
    gint prio;
    
    gfloat time_percentage;
    gboolean checked;

    gchar uname[64];
    gchar state[16];
    /* FIXED: Expanded size bounds to safely accommodate long arguments and execution 
     * paths extracted from /proc/[pid]/cmdline environments. */
    gchar name[1024]; 
};

typedef struct
{
    guint64 mem_total;
    guint64 mem_free;
    guint64 mem_cached;
    guint64 mem_buffered;
    guint64 cpu_idle;
    guint64 cpu_user;
    guint64 cpu_nice;
    guint64 cpu_system;
    guint64 cpu_old_jiffies;
    guint64 cpu_old_used;
    
    guint cpu_count;             /* FIXED: Re-typed to native unsigned integer bounds */
    gboolean valid_proc_reading;
} system_status;

/* --- Core Application Global Interfaces --- */
extern GtkWidget *main_window;
extern GArray *task_array;
extern gint tasks;
extern uid_t own_uid;

extern gchar *config_file;

extern gboolean show_user_tasks;
extern gboolean show_root_tasks;
extern gboolean show_other_tasks;
extern gboolean show_full_path;
extern gboolean show_cached_as_free;
extern gboolean full_view;

extern gint win_width;
extern gint win_height;
extern gint refresh_interval;
extern guint rID;

extern int page_size;

#endif /* TYPES_H */
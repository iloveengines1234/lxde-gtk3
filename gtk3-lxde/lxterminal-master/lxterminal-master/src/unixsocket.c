/**
 *      Copyright 2008 Fred Chien <cfsghost@gmail.com>
 *      Copyright (c) 2010 LxDE Developers, see the file AUTHORS for details.
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "lxterminal.h"
#include "unixsocket.h"

typedef struct _client_info {
    LXTermWindow * lxtermwin;
    int fd;
} ClientInfo;

static gboolean init(LXTermWindow* lxtermwin, const char *profile, gint argc, gchar** argv);
static void start_controller(struct sockaddr_un* sock_addr, LXTermWindow* lxtermwin, int fd);
static void send_msg_to_controller(int fd, gint argc, gchar** argv);
static gboolean handle_client(GIOChannel* source, GIOCondition condition, LXTermWindow* lxtermwin);
static void accept_client(GIOChannel* source, LXTermWindow* lxtermwin);
static gboolean handle_request(GIOChannel* gio, GIOCondition condition, ClientInfo* info);

static gboolean init(LXTermWindow* lxtermwin, const char *profile, gint argc, gchar** argv) {
    /* Formulate the path for the Unix domain socket safely. */
#if GLIB_CHECK_VERSION (2, 28, 0)
    gchar * socket_path = g_strdup_printf("%s/.lxterminal-socket%s%s-%s",
            g_get_user_runtime_dir(),
            profile ? "-" : "", profile ? profile : "",
            gdk_display_get_name(gdk_display_get_default()));
#else
    gchar * socket_path = g_strdup_printf("%s/.lxterminal-socket%s%s-%s",
            g_get_user_cache_dir(),
            profile ? "-" : "", profile ? profile : "",
            gdk_display_get_name(gdk_display_get_default()));
#endif

    if (socket_path == NULL) {
        return TRUE;
    }

    /* Initialize socket address and perform size validation checks. */
    struct sockaddr_un sock_addr;
    memset(&sock_addr, 0, sizeof(sock_addr));
    sock_addr.sun_family = AF_UNIX;

    if (strlen(socket_path) >= sizeof(sock_addr.sun_path)) {
        g_warning("Unix socket path is too long for the system limitation.");
        g_free(socket_path);
        return TRUE;
    }
    g_strlcpy(sock_addr.sun_path, socket_path, sizeof(sock_addr.sun_path));

    /* Create socket. */
    int fd = socket(PF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        g_warning("Socket create failed: %s", g_strerror(errno));
        g_free(socket_path);
        return TRUE;
    }

    /* Try to connect to an existing LXTerminal process. */
    if (connect(fd, (struct sockaddr *) &sock_addr, sizeof(sock_addr)) == 0) {
        g_free(socket_path);
        send_msg_to_controller(fd, argc, argv);
        return FALSE;
    }

    /* Connection failed; clear stale endpoints and transition to hosting controller */
    unlink(socket_path);
    g_free(socket_path);
    start_controller(&sock_addr, lxtermwin, fd);
    return TRUE;
}

static void start_controller(struct sockaddr_un* sock_addr, LXTermWindow* lxtermwin, int fd) {
    if (bind(fd, (struct sockaddr*) sock_addr, sizeof(*sock_addr)) < 0) {
        g_warning("Bind on socket failed: %s", g_strerror(errno));
        goto err_close_fd;
    }

    if (listen(fd, 5) < 0) {
        g_warning("Listen on socket failed: %s", g_strerror(errno));
        goto err_close_fd;
    }

    GIOChannel * gio = g_io_channel_unix_new(fd);
    if (gio == NULL) {
        g_warning("Cannot create GIOChannel");
        goto err_close_fd;
    }

    g_io_channel_set_encoding(gio, NULL, NULL);
    g_io_channel_set_buffered(gio, FALSE);
    g_io_channel_set_close_on_unref(gio, TRUE);

    if (!g_io_add_watch(gio, G_IO_IN | G_IO_HUP, (GIOFunc) handle_client, lxtermwin)) {
        g_warning("Cannot add watch on GIOChannel");
        goto err_gio_watch;
    }

    g_io_channel_unref(gio);
    return;

err_gio_watch:
    g_io_channel_unref(gio);
    return;

err_close_fd:
    close(fd);
}

static void send_msg_to_controller(int fd, gint argc, gchar** argv) {
    GIOChannel * gio = g_io_channel_unix_new(fd);
    if (gio == NULL) {
        close(fd);
        return;
    }
    g_io_channel_set_encoding(gio, NULL, NULL);

    gchar * cur_dir = g_get_current_dir();
    if (cur_dir != NULL) {
        g_io_channel_write_chars(gio, cur_dir, -1, NULL, NULL);
        g_free(cur_dir);
    }
    g_io_channel_write_chars(gio, "", 1, NULL, NULL);

    for (gint i = 0; i < argc; i++) {
        if (argv[i] != NULL) {
            g_io_channel_write_chars(gio, argv[i], -1, NULL, NULL);
        }
        g_io_channel_write_chars(gio, "", 1, NULL, NULL);
    }

    g_io_channel_flush(gio, NULL);
    g_io_channel_unref(gio);
}

static gboolean handle_client(GIOChannel* source, GIOCondition condition, LXTermWindow* lxtermwin) {
    if (condition & G_IO_IN) {
        accept_client(source, lxtermwin);
    }
    if (condition & G_IO_HUP) {
        g_error("Server listening socket closed unexpectedly");
    }
    return TRUE;
}

static void accept_client(GIOChannel* source, LXTermWindow* lxtermwin) {
    int fd = accept(g_io_channel_unix_get_fd(source), NULL, NULL);
    if (fd < 0) {
        g_warning("Accept failed: %s", g_strerror(errno));
        return;
    }

    long flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    GIOChannel * gio = g_io_channel_unix_new(fd);
    if (gio == NULL) {
        g_warning("Cannot create new GIOChannel");
        close(fd);
        return;
    }

    ClientInfo* info = g_malloc0(sizeof(ClientInfo));
    info->lxtermwin = lxtermwin;
    info->fd = fd;

    g_io_channel_set_encoding(gio, NULL, NULL);
    g_io_channel_set_close_on_unref(gio, TRUE);
    g_io_add_watch(gio, G_IO_IN | G_IO_HUP, (GIOFunc) handle_request, info);
    g_io_channel_unref(gio);
}

static gboolean handle_request(GIOChannel* gio, GIOCondition condition, ClientInfo* info) {
    LXTermWindow * lxtermwin = info->lxtermwin;
    int fd = info->fd;
    gchar * msg = NULL;
    gsize len = 0;
    GError * err = NULL;
    gboolean keep_running = TRUE;

    GIOStatus ret = g_io_channel_read_to_end(gio, &msg, &len, &err);
    if (ret == G_IO_STATUS_ERROR) {
        g_warning("Error reading socket: %s", err ? err->message : "Unknown");
        if (err) g_error_free(err);
    }

    if (len > 0 && msg != NULL) {
        gint argc = -1;
        for (gsize i = 0; i < len; i++) {
            if (msg[i] == '\0') {
                argc++;
            }
        }

        /* Ensure structural tracking size remains non-negative and valid */
        if (argc > 0) {
            gchar * cur_dir = msg;
            gchar ** argv = g_malloc0(argc * sizeof(char *));
            gint nul_count = 0;

            for (gsize i = 0; i < len - 1; i++) {
                if (msg[i] == '\0' && nul_count < argc) {
                    argv[nul_count] = &msg[i + 1];
                    nul_count++;
                }
            }

            CommandArguments arguments;
            memset(&arguments, 0, sizeof(CommandArguments));
            lxterminal_process_arguments(argc, argv, &arguments);
            g_free(argv);

            if (arguments.working_directory == NULL) {
                arguments.working_directory = g_strdup(cur_dir);
            }
            lxterminal_initialize(lxtermwin, &arguments);
        }
    }

    if (condition & G_IO_HUP) {
        g_free(info);
        /* File descriptor is wrapped in the watch and channel closure routines */
        keep_running = FALSE;
    }

    /* Safely release read buffer allocation mapping unconditionally */
    g_free(msg);
    return keep_running;
}

gboolean lxterminal_socket_initialize(LXTermWindow* lxtermwin, const char *profile, gint argc, gchar** argv) {
    return init(lxtermwin, profile, argc, argv);
}
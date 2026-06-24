/* na-tray-manager.c
 * Copyright (C) 2002 Anders Carlsson <andersca@gnu.org>
 * Copyright (C) 2003-2006 Vincent Untz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Used to be: eggtraymanager.c
 */

#include <config.h>
#include <string.h>
#include <libintl.h>

#include "eggtraymanager.h"

#include <gdkconfig.h>
#include <glib/gi18n.h>
#if defined (GDK_WINDOWING_X11)
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#elif defined (GDK_WINDOWING_WIN32)
#include <gdk/gdkwin32.h>
#endif
#include <gtk/gtkinvisible.h>
#include <gtk/gtksocket.h>
#include <gtk/gtkwindow.h>

#include "eggmarshalers.h"

/* Signals */
enum
{
  TRAY_ICON_ADDED,
  TRAY_ICON_REMOVED,
  MESSAGE_SENT,
  MESSAGE_CANCELLED,
  LOST_SELECTION,
  LAST_SIGNAL
};

enum {
  PROP_0,
  PROP_ORIENTATION
};

typedef struct
{
  long id, len;
  long remaining_len;

  long timeout;
  char *str;
#ifdef GDK_WINDOWING_X11
  Window window;
#endif
} PendingMessage;

static guint manager_signals[LAST_SIGNAL];

#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2

#define SYSTEM_TRAY_ORIENTATION_HORZ 0
#define SYSTEM_TRAY_ORIENTATION_VERT 1

#ifdef GDK_WINDOWING_X11
static gboolean na_tray_manager_check_running_screen_x11 (GdkScreen *screen);
#endif

static void na_tray_manager_finalize     (GObject      *object);
static void na_tray_manager_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void na_tray_manager_get_property (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);

static void na_tray_manager_unmanage (NaTrayManager *manager);

G_DEFINE_TYPE (NaTrayManager, na_tray_manager, G_TYPE_OBJECT)

static void
na_tray_manager_init (NaTrayManager *manager)
{
  manager->invisible = NULL;
  manager->socket_table = g_hash_table_new (NULL, NULL);
}

static void
na_tray_manager_class_init (NaTrayManagerClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = (GObjectClass *)klass;

  gobject_class->finalize = na_tray_manager_finalize;
  gobject_class->set_property = na_tray_manager_set_property;
  gobject_class->get_property = na_tray_manager_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_ORIENTATION,
                                   g_param_spec_enum ("orientation",
                                                      "orientation",
                                                      "orientation",
                                                      GTK_TYPE_ORIENTATION,
                                                      GTK_ORIENTATION_HORIZONTAL,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT |
                                                      G_PARAM_STATIC_NAME |
                                                      G_PARAM_STATIC_NICK |
                                                      G_PARAM_STATIC_BLURB));

  manager_signals[TRAY_ICON_ADDED] =
    g_signal_new ("tray_icon_added",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NaTrayManagerClass, tray_icon_added),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_SOCKET);

  manager_signals[TRAY_ICON_REMOVED] =
    g_signal_new ("tray_icon_removed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NaTrayManagerClass, tray_icon_removed),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__OBJECT,
                  G_TYPE_NONE, 1,
                  GTK_TYPE_SOCKET);
                  
  manager_signals[MESSAGE_SENT] =
    g_signal_new ("message_sent",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NaTrayManagerClass, message_sent),
                  NULL, NULL,
                  _na_marshal_VOID__OBJECT_STRING_LONG_LONG,
                  G_TYPE_NONE, 4,
                  GTK_TYPE_SOCKET,
                  G_TYPE_STRING,
                  G_TYPE_LONG,
                  G_TYPE_LONG);
                  
  manager_signals[MESSAGE_CANCELLED] =
    g_signal_new ("message_cancelled",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NaTrayManagerClass, message_cancelled),
                  NULL, NULL,
                  _na_marshal_VOID__OBJECT_LONG,
                  G_TYPE_NONE, 2,
                  GTK_TYPE_SOCKET,
                  G_TYPE_LONG);
                  
  manager_signals[LOST_SELECTION] =
    g_signal_new ("lost_selection",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (NaTrayManagerClass, lost_selection),
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

#if defined (GDK_WINDOWING_X11)
  /* Nothing */
#elif defined (GDK_WINDOWING_WIN32)
  g_warning ("Port NaTrayManager to Win32");
#else
  g_warning ("Port NaTrayManager to this GTK+ backend");
#endif
}

static void
na_tray_manager_finalize (GObject *object)
{
  NaTrayManager *manager;

  manager = NA_TRAY_MANAGER (object);

  na_tray_manager_unmanage (manager);

  g_list_free (manager->messages);
  g_hash_table_destroy (manager->socket_table);

  G_OBJECT_CLASS (na_tray_manager_parent_class)->finalize (object);
}

static void
na_tray_manager_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  NaTrayManager *manager = NA_TRAY_MANAGER (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      na_tray_manager_set_orientation (manager, g_value_get_enum (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
na_tray_manager_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  NaTrayManager *manager = NA_TRAY_MANAGER (object);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, manager->orientation);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

NaTrayManager *
na_tray_manager_new (void)
{
  NaTrayManager *manager;
  manager = g_object_new (NA_TYPE_TRAY_MANAGER, NULL);
  return manager;
}

#ifdef GDK_WINDOWING_X11

static gboolean
na_tray_manager_plug_removed (GtkSocket       *socket,
                              NaTrayManager   *manager)
{
  Window *window;

  window = g_object_get_data (G_OBJECT (socket), "na-tray-child-window");

  g_hash_table_remove (manager->socket_table, GINT_TO_POINTER (*window));
  g_object_set_data (G_OBJECT (socket), "na-tray-child-window", NULL);

  g_signal_emit (manager, manager_signals[TRAY_ICON_REMOVED], 0, socket);

  /* This destroys the socket. */
  return FALSE;
}

static void
na_tray_manager_make_socket_transparent (GtkWidget *widget,
                                         gpointer   user_data)
{
  if (!gtk_widget_get_has_window (widget))
    return;

  /* In GTK3, clearing backgrounds or making windows transparent is managed 
   * via CSS or Style Context clearing interfaces. gdk_window_set_back_pixmap is gone. */
}

/* Replaced legacy expose-event with modern GTK3 draw callback */
static gboolean
na_tray_manager_socket_draw (GtkWidget *widget,
                             cairo_t   *cr,
                             gpointer   user_data)
{
  /* Simply clear/repaint using transparent operators via Cairo context */
  cairo_set_source_rgba (cr, 0, 0, 0, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  cairo_paint (cr);
  return FALSE;
}

static void
na_tray_manager_socket_style_updated (GtkWidget *widget,
                                      gpointer   user_data)
{
  if (gtk_widget_get_window (widget) == NULL)
    return;

  na_tray_manager_make_socket_transparent (widget, user_data);
}

static void
na_tray_manager_handle_dock_request (NaTrayManager       *manager,
                                     XClientMessageEvent *xevent)
{
  GtkWidget *socket;
  Window *window;
  GtkRequisition req;

  if (g_hash_table_lookup (manager->socket_table, GINT_TO_POINTER (xevent->data.l[2])))
    {
      /* We already got this notification earlier, ignore this one */
      return;
    }

  socket = gtk_socket_new ();

  gtk_widget_set_app_paintable (socket, TRUE);
  
  /* Connect to modern realization and drawing handlers */
  g_signal_connect (socket, "realize",
                    G_CALLBACK (na_tray_manager_make_socket_transparent), NULL);
  g_signal_connect (socket, "draw",
                    G_CALLBACK (na_tray_manager_socket_draw), NULL);
  g_signal_connect_after (socket, "style-updated",
                          G_CALLBACK (na_tray_manager_socket_style_updated), NULL);

  window = g_new (Window, 1);
  *window = xevent->data.l[2];

  g_object_set_data_full (G_OBJECT (socket),
                          "na-tray-child-window",
                          window, g_free);
  g_signal_emit (manager, manager_signals[TRAY_ICON_ADDED], 0, socket);

  /* Add the socket only if it's been attached */
  if (GTK_IS_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (socket))))
    {
      g_signal_connect (socket, "plug_removed",
                        G_CALLBACK (na_tray_manager_plug_removed), manager);

      gtk_socket_add_id (GTK_SOCKET (socket), *window);
      g_hash_table_insert (manager->socket_table, GINT_TO_POINTER (*window), socket);

      /* Use modern preferred size API instead of deprecated gtk_widget_size_request */
      gtk_widget_get_preferred_size (socket, &req, NULL);
      gtk_widget_show (socket);
    }
  else
    gtk_widget_destroy (socket);
}

static void
pending_message_free (PendingMessage *message)
{
  g_free (message->str);
  g_free (message);
}

static GdkFilterReturn
na_tray_manager_handle_client_message_message_data (GdkXEvent *xev,
                                                    GdkEvent  *event,
                                                    gpointer   data)
{
  XClientMessageEvent *xevent;
  NaTrayManager       *manager;
  GList               *p;
  int                  len;

  xevent  = (XClientMessageEvent *) xev;
  manager = data;

  /* Try to see if we can find the pending message in the list */
  for (p = manager->messages; p; p = p->next)
    {
      PendingMessage *msg = p->data;

      if (xevent->window == msg->window)
        {
          /* Append the message */
          len = MIN (msg->remaining_len, 20);

          memcpy ((msg->str + msg->len - msg->remaining_len),
                  &xevent->data, len);
          msg->remaining_len -= len;

          if (msg->remaining_len == 0)
            {
              GtkSocket *socket;

              socket = g_hash_table_lookup (manager->socket_table,
                                            GINT_TO_POINTER (msg->window));

              if (socket)
                g_signal_emit (manager, manager_signals[MESSAGE_SENT], 0,
                               socket, msg->str, msg->id, msg->timeout);

              pending_message_free (msg);
              manager->messages = g_list_remove_link (manager->messages, p);
              g_list_free_1 (p);
            }
          break;
        }
    }

  return GDK_FILTER_REMOVE;
}

static void
na_tray_manager_handle_begin_message (NaTrayManager       *manager,
                                      XClientMessageEvent *xevent)
{
  GtkSocket      *socket;
  GList          *p;
  PendingMessage *msg;
  long            timeout;
  long            len;
  long            id;

  socket = g_hash_table_lookup (manager->socket_table,
                                GINT_TO_POINTER (xevent->window));
  if (!socket)
    return;

  /* Check if the same message is already in the queue and remove it if so */
  for (p = manager->messages; p; p = p->next)
    {
      PendingMessage *m = p->data;

      if (xevent->window == m->window && xevent->data.l[4] == m->id)
        {
          pending_message_free (m);
          manager->messages = g_list_remove_link (manager->messages, p);
          g_list_free_1 (p);
          break;
        }
    }

  timeout = xevent->data.l[2];
  len     = xevent->data.l[3];
  id      = xevent->data.l[4];

  if (len == 0)
    {
      g_signal_emit (manager, manager_signals[MESSAGE_SENT], 0,
                     socket, "", id, timeout);
    }
  else
    {
      msg = g_new0 (PendingMessage, 1);
      msg->window = xevent->window;
      msg->timeout = timeout;
      msg->len = len;
      msg->id = id;
      msg->remaining_len = msg->len;
      msg->str = g_malloc (msg->len + 1);
      msg->str[msg->len] = '\0';
      manager->messages = g_list_prepend (manager->messages, msg);
    }
}

static void
na_tray_manager_handle_cancel_message (NaTrayManager       *manager,
                                       XClientMessageEvent *xevent)
{
  GList     *p;
  GtkSocket *socket;

  /* Check if the message is in the queue and remove it if so */
  for (p = manager->messages; p; p = p->next)
    {
      PendingMessage *msg = p->data;

      if (xevent->window == msg->window && xevent->data.l[4] == msg->id)
        {
          pending_message_free (msg);
          manager->messages = g_list_remove_link (manager->messages, p);
          g_list_free_1 (p);
          break;
        }
    }

  socket = g_hash_table_lookup (manager->socket_table,
                                GINT_TO_POINTER (xevent->window));

  if (socket)
    {
      g_signal_emit (manager, manager_signals[MESSAGE_CANCELLED], 0,
                     socket, xevent->data.l[2]);
    }
}

static GdkFilterReturn
na_tray_manager_handle_client_message_opcode (GdkXEvent *xev,
                                              GdkEvent  *event,
                                              gpointer   data)
{
  XClientMessageEvent *xevent;
  NaTrayManager       *manager;

  xevent  = (XClientMessageEvent *) xev;
  manager = data;

  switch (xevent->data.l[1])
    {
    case SYSTEM_TRAY_REQUEST_DOCK:
      break;

    case SYSTEM_TRAY_BEGIN_MESSAGE:
      na_tray_manager_handle_begin_message (manager, xevent);
      return GDK_FILTER_REMOVE;

    case SYSTEM_TRAY_CANCEL_MESSAGE:
      na_tray_manager_handle_cancel_message (manager, xevent);
      return GDK_FILTER_REMOVE;
    default:
      break;
    }

  return GDK_FILTER_CONTINUE;
}

static GdkFilterReturn
na_tray_manager_window_filter (GdkXEvent *xev,
                               GdkEvent  *event,
                               gpointer   data)
{
  XEvent        *xevent = (GdkXEvent *)xev;
  NaTrayManager *manager = data;

  if (xevent->type == ClientMessage)
    {
      if (xevent->xclient.message_type == manager->opcode_atom &&
          xevent->xclient.data.l[1]    == SYSTEM_TRAY_REQUEST_DOCK)
        {
          na_tray_manager_handle_dock_request (manager, (XClientMessageEvent *) xevent);
          return GDK_FILTER_REMOVE;
        }
    }
  else if (xevent->type == SelectionClear)
    {
      g_signal_emit (manager, manager_signals[LOST_SELECTION], 0);
      na_tray_manager_unmanage (manager);
    }

  return GDK_FILTER_CONTINUE;
}

#endif

static void
na_tray_manager_unmanage (NaTrayManager *manager)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display;
  guint32     timestamp;
  GtkWidget  *invisible;
  GdkWindow  *gdkwin;

  if (manager->invisible == NULL)
    return;

  invisible = manager->invisible;
  g_assert (GTK_IS_INVISIBLE (invisible));
  g_assert (gtk_widget_get_realized (invisible));
  
  gdkwin = gtk_widget_get_window (invisible);
  g_assert (GDK_IS_WINDOW (gdkwin));

  display = gtk_widget_get_display (invisible);

  if (gdk_selection_owner_get_for_display (display, manager->selection_atom) == gdkwin)
    {
      timestamp = gdk_x11_get_server_time (gdkwin);
      gdk_selection_owner_set_for_display (display,
                                           NULL,
                                           manager->selection_atom,
                                           timestamp,
                                           TRUE);
    }

  gdk_window_remove_filter (gdkwin, na_tray_manager_window_filter, manager);

  manager->invisible = NULL;
  gtk_widget_destroy (invisible);
  g_object_unref (G_OBJECT (invisible));
#endif
}

static void
na_tray_manager_set_orientation_property (NaTrayManager *manager)
{
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display;
  Atom        orientation_atom;
  gulong      data[1];
  GdkWindow  *gdkwin;

  if (!manager->invisible)
    return;
    
  gdkwin = gtk_widget_get_window (manager->invisible);
  if (!gdkwin)
    return;

  display = gtk_widget_get_display (manager->invisible);
  orientation_atom = gdk_x11_get_xatom_by_name_for_display (display,
                                                            "_NET_SYSTEM_TRAY_ORIENTATION");

  data[0] = manager->orientation == GTK_ORIENTATION_HORIZONTAL ?
        SYSTEM_TRAY_ORIENTATION_HORZ :
        SYSTEM_TRAY_ORIENTATION_VERT;

  XChangeProperty (GDK_DISPLAY_XDISPLAY (display),
                   GDK_WINDOW_XWINDOW (gdkwin),
                   orientation_atom,
                   XA_CARDINAL, 32,
                   PropModeReplace,
                   (guchar *) &data, 1);
#endif
}

#ifdef GDK_WINDOWING_X11

static gboolean
na_tray_manager_manage_screen_x11 (NaTrayManager *manager,
                                   GdkScreen     *screen)
{
  GdkDisplay *display;
  Screen     *xscreen;
  GtkWidget  *invisible;
  char       *selection_atom_name;
  guint32     timestamp;
  GdkWindow  *gdkwin;

  g_return_val_if_fail (NA_IS_TRAY_MANAGER (manager), FALSE);
  g_return_val_if_fail (manager->screen == NULL, FALSE);

  display = gdk_screen_get_display (screen);
  xscreen = GDK_SCREEN_XSCREEN (screen);

  invisible = gtk_invisible_new_for_screen (screen);
  gtk_widget_realize (invisible);

  gtk_widget_add_events (invisible,
                         GDK_PROPERTY_CHANGE_MASK | GDK_STRUCTURE_MASK);

  selection_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d",
                                         gdk_screen_get_number (screen));
  manager->selection_atom = gdk_atom_intern (selection_atom_name, FALSE);
  g_free (selection_atom_name);

  na_tray_manager_set_orientation_property (manager);

  gdkwin = gtk_widget_get_window (invisible);
  timestamp = gdk_x11_get_server_time (gdkwin);

  /* Check if we could set the selection owner successfully */
  if (gdk_selection_owner_set_for_display (display,
                                           gdkwin,
                                           manager->selection_atom,
                                           timestamp,
                                           TRUE))
    {
      XClientMessageEvent xev;
      GdkAtom             opcode_atom;
      GdkAtom             message_data_atom;

      xev.type = ClientMessage;
      xev.window = RootWindowOfScreen (xscreen);
      xev.message_type = gdk_x11_get_xatom_by_name_for_display (display, "MANAGER");

      xev.format = 32;
      xev.data.l[0] = timestamp;
      xev.data.l[1] = gdk_x11_atom_to_xatom_for_display (display, manager->selection_atom);
      xev.data.l[2] = GDK_WINDOW_XWINDOW (gdkwin);
      xev.data.l[3] = 0;
      xev.data.l[4] = 0;

      XSendEvent (GDK_DISPLAY_XDISPLAY (display),
                  RootWindowOfScreen (xscreen),
                  False, StructureNotifyMask, (XEvent *)&xev);

      manager->invisible = invisible;
      g_object_ref (G_OBJECT (manager->invisible));

      opcode_atom = gdk_atom_intern ("_NET_SYSTEM_TRAY_OPCODE", FALSE);
      manager->opcode_atom = gdk_x11_atom_to_xatom_for_display (display, opcode_atom);

      message_data_atom = gdk_atom_intern ("_NET_SYSTEM_TRAY_MESSAGE_DATA", FALSE);

      gdk_window_add_filter (gdkwin, na_tray_manager_window_filter, manager);
      
      gdk_display_add_client_message_filter (display, opcode_atom,
                                             na_tray_manager_handle_client_message_opcode,
                                             manager);
                                             
      gdk_display_add_client_message_filter (display, message_data_atom,
                                             na_tray_manager_handle_client_message_message_data,
                                             manager);
      return TRUE;
    }
  else
    {
      gtk_widget_destroy (invisible);
      return FALSE;
    }
}

#endif

gboolean
na_tray_manager_manage_screen (NaTrayManager *manager,
                               GdkScreen     *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);
  g_return_val_if_fail (manager->screen == NULL, FALSE);

#ifdef GDK_WINDOWING_X11
  return na_tray_manager_manage_screen_x11 (manager, screen);
#else
  return FALSE;
#endif
}

#ifdef GDK_WINDOWING_X11

static gboolean
na_tray_manager_check_running_screen_x11 (GdkScreen *screen)
{
  GdkDisplay *display;
  Atom        selection_atom;
  char       *selection_atom_name;

  display = gdk_screen_get_display (screen);
  selection_atom_name = g_strdup_printf ("_NET_SYSTEM_TRAY_S%d",
                                         gdk_screen_get_number (screen));
  selection_atom = gdk_x11_get_xatom_by_name_for_display (display, selection_atom_name);
  g_free (selection_atom_name);

  if (XGetSelectionOwner (GDK_DISPLAY_XDISPLAY (display), selection_atom) != None)
    return TRUE;
  else
    return FALSE;
}

#endif

gboolean
na_tray_manager_check_running (GdkScreen *screen)
{
  g_return_val_if_fail (GDK_IS_SCREEN (screen), FALSE);

#ifdef GDK_WINDOWING_X11
  return na_tray_manager_check_running_screen_x11 (screen);
#else
  return FALSE;
#endif
}

char *
na_tray_manager_get_child_title (NaTrayManager      *manager,
                                 NaTrayManagerChild *child)
{
  char *retval = NULL;
#ifdef GDK_WINDOWING_X11
  GdkDisplay *display;
  Window *child_window;
  Atom utf8_string, atom, type;
  int result;
  int format;
  gulong nitems;
  gulong bytes_after;
  gchar *val;

  g_return_val_if_fail (NA_IS_TRAY_MANAGER (manager), NULL);
  g_return_val_if_fail (GTK_IS_SOCKET (child), NULL);

  display = gdk_screen_get_display (manager->screen);

  child_window = g_object_get_data (G_OBJECT (child), "na-tray-child-window");

  utf8_string = gdk_x11_get_xatom_by_name_for_display (display, "UTF8_STRING");
  atom = gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_NAME");

  gdk_error_trap_push ();

  result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display),
                               *child_window,
                               atom,
                               0, G_MAXLONG,
                               False, utf8_string,
                               &type, &format, &nitems,
                               &bytes_after, (guchar **)&val);

  if (gdk_error_trap_pop () || result != Success || type != utf8_string)
    return NULL;

  if (format != 8 || nitems == 0)
    {
      if (val)
        XFree (val);
      return NULL;
    }

  if (!g_utf8_validate (val, nitems, NULL))
    {
      XFree (val);
      return NULL;
    }

  retval = g_strndup (val, nitems);
  XFree (val);
#endif
  return retval;
}

void
na_tray_manager_set_orientation (NaTrayManager  *manager,
                                 GtkOrientation  orientation)
{
  g_return_if_fail (NA_IS_TRAY_MANAGER (manager));

  if (manager->orientation != orientation)
    {
      manager->orientation = orientation;
      na_tray_manager_set_orientation_property (manager);
      g_object_notify (G_OBJECT (manager), "orientation");
    }
}

GtkOrientation
na_tray_manager_get_orientation (NaTrayManager *manager)
{
  g_return_val_if_fail (NA_IS_TRAY_MANAGER (manager), GTK_ORIENTATION_HORIZONTAL);
  return manager->orientation;
}
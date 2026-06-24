#pragma once

/**
 * create_volume_window:
 *
 * Returns: (transfer full): a newly created GtkWidget* for the volume window which is
 * owned by the caller; the caller is responsible for destroying it (e.g. with
 * gtk_widget_destroy()) when no longer needed.
 *
 * Thread context: must be called from the main GTK thread (the GDK/GLib main loop).
 *
 * Side effects: allocates and configures widgets, connects signal handlers and may
 * modify global audio-related state.
 */
GtkWidget* create_volume_window (void);
typedef struct _GtkWidget GtkWidget;

GtkWidget* create_volume_window (void);

#endif

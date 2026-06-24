/*
 * Copyright © 2004 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL RED HAT
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN 
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Updated by iloveengines1234 to port to gtk3
 * Author:  Matthias Clasen, Red Hat, Inc.
 */

#include <stdlib.h>
#include <glib.h>

/* Modern Fix: Include core Xlib dependencies inside the implementation scope
 * to resolve forward definitions from xutils.h without exposing them downstream. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "xutils.h"

Atom XA_ATOM_PAIR = 0;
Atom XA_CLIPBOARD_MANAGER = 0;
Atom XA_CLIPBOARD = 0;
Atom XA_DELETE = 0;
Atom XA_INCR = 0;
Atom XA_INSERT_PROPERTY = 0;
Atom XA_INSERT_SELECTION = 0;
Atom XA_MANAGER = 0;
Atom XA_MULTIPLE = 0;
Atom XA_NULL = 0;
Atom XA_SAVE_TARGETS = 0;
Atom XA_TARGETS = 0;
Atom XA_TIMESTAMP = 0;

unsigned long SELECTION_MAX_SIZE = 0;

/* Modern Fix: Keep track of the active Display pointer to handle 
 * server context drops or re-connections safely. */
static Display *cached_display = NULL;

void
init_atoms (Display *display)
{
  unsigned long max_request_size;
  
  if (!display)
    return;

  if (cached_display == display && SELECTION_MAX_SIZE > 0)
    return;

  cached_display = display;

  XA_ATOM_PAIR = XInternAtom (display, "ATOM_PAIR", False);
  XA_CLIPBOARD_MANAGER = XInternAtom (display, "CLIPBOARD_MANAGER", False);
  XA_CLIPBOARD = XInternAtom (display, "CLIPBOARD", False);
  XA_DELETE = XInternAtom (display, "DELETE", False);
  XA_INCR = XInternAtom (display, "INCR", False);
  XA_INSERT_PROPERTY = XInternAtom (display, "INSERT_PROPERTY", False);
  XA_INSERT_SELECTION = XInternAtom (display, "INSERT_SELECTION", False);
  XA_MANAGER = XInternAtom (display, "MANAGER", False);
  XA_MULTIPLE = XInternAtom (display, "MULTIPLE", False);
  XA_NULL = XInternAtom (display, "NULL", False);
  XA_SAVE_TARGETS = XInternAtom (display, "SAVE_TARGETS", False);
  XA_TARGETS = XInternAtom (display, "TARGETS", False);
  XA_TIMESTAMP = XInternAtom (display, "TIMESTAMP", False);
  
  max_request_size = XExtendedMaxRequestSize (display);
  if (max_request_size == 0)
    max_request_size = XMaxRequestSize (display);
  
  SELECTION_MAX_SIZE = max_request_size - 100;
  if (SELECTION_MAX_SIZE > 262144)
    SELECTION_MAX_SIZE = 262144;
}

typedef struct 
{
  Window window;
  Atom timestamp_prop_atom;
} TimeStampInfo;

static Bool
timestamp_predicate (Display *display,
                     XEvent  *xevent,
                     XPointer arg)
{
  TimeStampInfo *info = (TimeStampInfo *)arg;
  (void)display; /* Silence unused variable compiler checks */

  if (xevent->type == PropertyNotify &&
      xevent->xproperty.window == info->window &&
      xevent->xproperty.atom == info->timestamp_prop_atom)
    return True;

  return False;
}

Time
get_server_time (Display *display,
                 Window   window)
{
  unsigned char c = 'a';
  XEvent xevent;
  TimeStampInfo info;
  int loop_count = 0;
  const int max_loops = 5000; /* Safeguard threshold against infinite loops */

  if (!display || window == None)
    return 0;

  info.timestamp_prop_atom = XInternAtom (display, "_TIMESTAMP_PROP", False);
  info.window = window;

  XChangeProperty (display, window,
                   info.timestamp_prop_atom, info.timestamp_prop_atom,
                   8, PropModeReplace, &c, 1);

  /* Modern Fix: Replace raw blocking XIfEvent processing loops with an explicit 
   * event-queue processing circuit. This allows us to handle timeouts and prevents 
   * freezing the primary GTK 3 / GDK layout pipelines if the X Server is under heavy load. */
  XFlush(display);
  while (loop_count++ < max_loops)
    {
      if (XCheckIfEvent (display, &xevent, timestamp_predicate, (XPointer)&info))
        {
          return xevent.xproperty.time;
        }
      g_usleep(1000); /* Sleep 1ms to give the server a window to process */
    }

  /* Fallback: Log warning on timeout and query a safely masked wall time 
   * millisecond timestamp value to preserve protocol safety boundaries. */
  g_warning("xutils: get_server_time timed out waiting for server response property.");
  return (Time)((g_get_real_time() / 1000) & 0xFFFFFFFF);
}
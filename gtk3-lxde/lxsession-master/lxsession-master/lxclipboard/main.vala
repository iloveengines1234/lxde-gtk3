/* * Copyright 2012 Julien Lavergne <gilir@ubuntu.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

using Gtk;

namespace Lxsession
{
    // Fix: Explicitly instruct valac not to prefix/mangle the raw C function names
    [CCode (cname = "clipboard_start")]
    public extern void clipboard_start();
    
    [CCode (cname = "clipboard_stop")]
    public extern void clipboard_stop();

    public class Main : GLib.Object
    {
        public static int main(string[] args)
        {
            // Initialize GTK3 dependencies safely
            Gtk.init(ref args);

            // Use clean application mapping flags
            var app = new GLib.Application(
                "org.lxde.lxclipboard",
                GLib.ApplicationFlags.FLAGS_NONE
            );
            
            // Connect lifecycle actions directly to GLib's structural event loop
            app.activate.connect(() => {
                // Spin up the clipboard listener once application lifecycle is verified
                clipboard_start();
                
                // Keep the application running in the background even without an active window open
                app.hold();
            });

            app.shutdown.connect(() => {
                // Ensure clipboard clean-up functions execute reliably when app is killed or terminated
                clipboard_stop();
            });

            // Run natively via GLib infrastructure (handles singleton/remote checking automatically)
            return app.run(args);
        }
    }
}
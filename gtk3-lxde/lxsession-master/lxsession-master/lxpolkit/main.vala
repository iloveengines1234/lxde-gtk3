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

const string GETTEXT_PACKAGE = "lxsession";

namespace Lxsession
{
    // Fix 1 & 2: Instruct valac to bind to exact global C names, and track the true boolean return type
    [CCode (cname = "policykit_agent_init")]
    public extern bool policykit_agent_init();
    
    [CCode (cname = "policykit_agent_finalize")]
    public extern void policykit_agent_finalize();

    public class Main : GLib.Object
    {
        public static int main(string[] args)
        {
            // Initialize localization cleanly
            Intl.textdomain(GETTEXT_PACKAGE);
            Intl.bind_textdomain_codeset(GETTEXT_PACKAGE, "utf-8");

            // Initialize GTK3 elements safely
            Gtk.init(ref args);

            // Modern standard Application setup using valid flag hooks
            var app = new GLib.Application(
                "org.lxde.lxpolkit",
                GLib.ApplicationFlags.FLAGS_NONE
            );

            // Bind native lifecycle hooks to standard GLib loop events
            app.activate.connect(() => {
                // Safely catch initialization failure flags from lxpolkit.c
                if (!policykit_agent_init()) {
                    critical("Failed to initialize PolicyKit Authentication Agent listener.");
                    app.release(); // Allow application to exit safely
                    return;
                }

                // Hold the app open in the background (since it doesn't open a persistent main window)
                app.hold();
            });

            app.shutdown.connect(() => {
                // Guarantees clean unregistration from D-Bus even during system logouts or crashes
                policykit_agent_finalize();
            });

            // Native application execution engine (implicitly checks single-instance remote routing)
            return app.run(args);
        }
    }
}
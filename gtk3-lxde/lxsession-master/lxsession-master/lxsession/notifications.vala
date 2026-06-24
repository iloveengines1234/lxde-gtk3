/* * Copyright 2015 Julien Lavergne <gilir@ubuntu.com>
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
#if USE_GTK
using Gtk;
#if USE_ADVANCED_NOTIFICATIONS
using AppIndicator;
using Notify;
#endif
#endif

namespace Lxsession
{
#if USE_GTK
    public class MenuItemObject : Gtk.MenuItem
#else
    public class MenuItemObject : MenuItemGenericObject
#endif
    {
        public MenuItemObject ()
        {
            base();
        }
    }

    public class MenuItemGenericObject : GLib.Object
    {
        public MenuItemGenericObject ()
        {
        } 
    }

#if USE_GTK
    public class MenuObject : Gtk.Menu
#else
    public class MenuObject : MenuGenericObject
#endif
    {
        public MenuObject ()
        {
#if USE_GTK
            base();
#endif
        }
    }

    public class MenuGenericObject : GLib.Object
    {
        public delegate void ActionCallback ();

        public void add_item (string text, owned ActionCallback callback)
        {
            warning("%s", "Method platform layout layer implementation missing: 'add_item'");
        }
    }

    public class IconObject : GLib.Object
    {
        public string name;
        public string icon_name;    
        public string notification_text;
        public MenuObject? menu;

#if USE_GTK
#if USE_ADVANCED_NOTIFICATIONS
        public Indicator indicator;
        public Notify.Notification notification;
#endif
#endif

        public delegate void ActionCallback ();
        
        // Retain a persistent reference to un-looped callback states to prevent memory leaks
        private ActionCallback? persistent_action_callback;
        
        public IconObject(string name_param, string? icon_name_param, string? notification_param, MenuObject? menu_param)
        {
            this.name = name_param;
            this.icon_name = icon_name_param ?? "dialog-warning";
            
            // Fix 1: Hardened safe fallback matching to protect underlying C libraries from null pointer errors
            this.notification_text = notification_param ?? "";
            this.menu = menu_param;

#if USE_GTK
#if USE_ADVANCED_NOTIFICATIONS
            this.indicator = new Indicator(this.name, this.icon_name, IndicatorCategory.APPLICATION_STATUS);
            this.notification = new Notify.Notification("LXsession", this.notification_text, this.icon_name);
            this.notification.set_timeout(6000);
#endif
#endif
        }

#if USE_GTK
#if USE_ADVANCED_NOTIFICATIONS
        public void init()
        {
            if (this.indicator == null)
            {
                this.indicator = new Indicator(this.name, this.icon_name, IndicatorCategory.APPLICATION_STATUS);
            }

            this.indicator.set_status(IndicatorStatus.ACTIVE);

            if (this.menu != null)
            {
                this.indicator.set_menu(this.menu);
            }
        }

        public void activate()
        {
            message("%s", "Attempting notification subsystem server frame activation sequence...");
            if (this.indicator != null)
            {
                this.indicator.set_status(IndicatorStatus.ACTIVE);
                try
                {
                    this.notification.show();
                }
                catch (GLib.Error e)
                {
                    message("Libnotify execution runtime system failure anomaly tracked: %s", e.message);
                }
            }
        }

        public void inactivate()
        {
            if (this.indicator != null)
            {
                this.indicator.set_status(IndicatorStatus.PASSIVE);
                message("%s", "Subsystem passive shutdown loop executed successfully.");
            }
        }

        public void set_icon(string param_icon_name)
        {
            this.icon_name = param_icon_name;   
            if (this.indicator != null)
            {
                this.indicator.icon_name = param_icon_name;
            }
        }

        public void set_menu(MenuObject param_menu)
        {
            this.menu = param_menu;
            if (this.indicator != null)
            {
                this.indicator.set_menu(param_menu);
            }
        }

        public void add_action(string action, string label, owned ActionCallback callback)
        {
            if (this.notification != null)
            {
                // Fix 2: Break closure cycles by transferring ownership to object scope scope instead of letting it leak inside a lambda context
                this.persistent_action_callback = (owned) callback;
                
                this.notification.add_action(action, label, (n, a) => 
                {
                    if (this.persistent_action_callback != null)
                    {
                        this.persistent_action_callback();
                    }
                });
            }
        }

        public void set_notification_body(string text)
        {
            if (this.notification != null)
            {
                this.notification_text = text;
                this.notification.body = text;
            }
        }

        public void clear_actions()
        {
            if (this.notification != null)
            {
                this.notification.clear_actions();
                this.persistent_action_callback = null; // Cleanly clear targeted delegate objects from memory bounds
            }
        }
#else
        // Non-advanced fallback implementation hooks matching configuration rules cleanly
        public void init() {}
        public void activate() {}
        public void inactivate() {}
        public void set_icon(string param_icon_name) {}
        public void set_menu(MenuObject param_menu) {}
        public void add_action(string action, string label, owned ActionCallback callback) {}
        public void set_notification_body(string text) {}
        public void clear_actions() {}
#endif
#else
        // Completely Headless headless fallback execution tracks
        public void init() {}
        public void activate() {}
        public void inactivate() {}
        public void set_icon(string param_icon_name) {}
        public void set_menu(MenuObject param_menu) {}
        public void add_action(string action, string label, owned ActionCallback callback) {}
        public void set_notification_body(string text) {}
        public void clear_actions() {}
#endif
    }    
}
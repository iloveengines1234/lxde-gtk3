/* -*- indent-tabs-mode: nil; tab-width: 4; c-basic-offset: 4; -*-

   tree.c for ObConf, the configuration tool for Openbox
   Copyright (c) 2003-2007   Dana Jansens
   Copyright (C) 2010        Hong Jen Yee (PCMan) <pcman.tw@gmail.com>
   Copyright (C) 2026        LXDE Maintainers Group

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
*/

#include "tree.h"
#include "main.h"

#include <obt/xml.h>
#include <gdk/gdkx.h>
#include <lxappearance/lxappearance.h>
#include <string.h>

xmlNodePtr tree_get_node(const gchar *path, const gchar *def)
{
    xmlNodePtr n, c;
    gchar **nodes;
    gchar **it, **next;

    n = obt_xml_root(xml_i);
    if (!n) return NULL;

    nodes = g_strsplit(path, "/", 0);
    for (it = nodes; *it; it = next) {
        gchar **attrs;
        gboolean ok = FALSE;

        attrs = g_strsplit(*it, ":", 0);
        next = it + 1;

        /* Match attributes within the hierarchy */
        c = obt_xml_find_node(n->children, attrs[0]);
        while (c && !ok) {
            gint i;

            ok = TRUE;
            for (i = 1; attrs[i]; ++i) {
                gchar **eq = g_strsplit(attrs[i], "=", 2);
                if (eq[0] && eq[1]) {
                    if (!obt_xml_attr_contains(c, eq[0], eq[1]))
                        ok = FALSE;
                } else {
                    ok = FALSE;
                }
                g_strfreev(eq);
            }
            if (!ok)
                c = obt_xml_find_node(c->next, attrs[0]);
        }

        /* Allocate missing visual node configuration trees cleanly */
        if (!c) {
            gint i;
            xmlChar *node_name = (xmlChar *)attrs[0];
            xmlChar *default_val = *next ? NULL : (xmlChar *)def;

            c = xmlNewTextChild(n, NULL, node_name, default_val);

            for (i = 1; attrs[i]; ++i) {
                gchar **eq = g_strsplit(attrs[i], "=", 2);
                if (eq[0] && eq[1]) {
                    xmlNewProp(c, (const xmlChar *)eq[0], (const xmlChar *)eq[1]);
                }
                g_strfreev(eq);
            }
        }
        n = c;

        g_strfreev(attrs);
    }

    g_strfreev(nodes);

    return n;
}

void tree_delete_node(const gchar *path)
{
    xmlNodePtr n = tree_get_node(path, NULL);
    if (n) {
        xmlUnlinkNode(n);
        xmlFreeNode(n);
    }
}

void tree_apply(void)
{
    gchar *p, *d;

    if (obc_config_file)
        p = g_strdup(obc_config_file);
    else
        p = g_build_filename(obt_paths_config_home(paths), "openbox", "rc.xml", NULL);

    d = g_path_get_dirname(p);
    obt_paths_mkdir_path(d, 0700);
    g_free(d);

    if (obt_xml_save_file(xml_i, p, TRUE)) {
        XEvent ce;
        GdkDisplay *display = gdk_display_get_default();

        memset(&ce, 0, sizeof(ce));
        ce.xclient.type = ClientMessage;
        ce.xclient.message_type = gdk_x11_get_xatom_by_name("_OB_CONTROL");
        ce.xclient.display = GDK_DISPLAY_XDISPLAY(display);
        ce.xclient.window = GDK_ROOT_WINDOW();
        ce.xclient.format = 32;
        ce.xclient.data.l[0] = 1; /* Command descriptor index: reconfigure */
        ce.xclient.data.l[1] = 0;
        ce.xclient.data.l[2] = 0;
        ce.xclient.data.l[3] = 0;
        ce.xclient.data.l[4] = 0;

        XSendEvent(GDK_DISPLAY_XDISPLAY(display), GDK_ROOT_WINDOW(), FALSE,
                   SubstructureNotifyMask | SubstructureRedirectMask,
                   &ce);
    } else {
        gchar *s = g_strdup_printf(_("An error occurred while saving the config file '%s'"), p);
        obconf_error(s, FALSE);
        g_free(s);
    }
    g_free(p);
}

void tree_set_string(const gchar *node, const gchar *value)
{
    xmlNodePtr n = tree_get_node(node, NULL);
    if (n) {
        xmlNodeSetContent(n, (const xmlChar*) value);
    }
    lxappearance_changed();
}

void tree_set_int(const gchar *node, const gint value)
{
    xmlNodePtr n = tree_get_node(node, NULL);
    if (n) {
        gchar *s = g_strdup_printf("%d", value);
        xmlNodeSetContent(n, (const xmlChar*) s);
        g_free(s);
    }
    lxappearance_changed();
}

void tree_set_bool(const gchar *node, const gboolean value)
{
    xmlNodePtr n = tree_get_node(node, NULL);
    if (n) {
        xmlNodeSetContent(n, (const xmlChar*) (value ? "yes" : "no"));
    }
    lxappearance_changed();
}

gchar* tree_get_string(const gchar *node, const gchar *def)
{
    xmlNodePtr n = tree_get_node(node, def);
    return obt_xml_node_string(n);
}

gint tree_get_int(const gchar *node, gint def)
{
    gchar *d = g_strdup_printf("%d", def);
    xmlNodePtr n = tree_get_node(node, d);
    g_free(d);
    return obt_xml_node_int(n);
}

gboolean tree_get_bool(const gchar *node, gboolean def)
{
    xmlNodePtr n = tree_get_node(node, (def ? "yes" : "no"));
    return obt_xml_node_bool(n);
}
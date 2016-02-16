// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "biogeo.h"
#include "tree.h"
#include "events.h"
#include "ev.h"

GPtrArray* geoData;
GPtrArray* treeData;

extern Tree* activeTree;
Recons* activeRec = NULL;

static GtkWidget* itemRecons;

// errMsg displays an error message dialog-
void errMsg(GtkWindow* parent, gchar* msg) {
        GtkWidget* m;

        m = gtk_message_dialog_new(parent, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                GTK_BUTTONS_OK, msg);
        gtk_window_set_title(GTK_WINDOW(m), "Error");
        gtk_dialog_run(GTK_DIALOG(m));
        gtk_widget_destroy(m);
}

// newMenuItem creates a new item in the indicated menu.
GtkWidget* newMenuItem(GtkWidget *menu, gchar* name, GCallback func, gpointer data) {
        GtkWidget* item;

        item = gtk_menu_item_new_with_label(name);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        if (func != NULL) {
                g_signal_connect(G_OBJECT(item), "activate", func, data);
        }
        return item;
}

// addFilter adds a new file filter to a dialog.
void addFilter(gchar* name, gchar* pattern, GtkWidget* d) {
        GtkFileFilter* f;

        f = gtk_file_filter_new();
        gtk_file_filter_set_name(f, name);
        gtk_file_filter_add_pattern(f, pattern);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), f);
}

void menuOpenData(GtkMenuItem* item, gpointer data) {
        GtkWidget* d;
        gchar* path = NULL;
        FILE*  f;
        gchar* err = NULL;
        gchar* title;

        geoData = NULL;
        d = gtk_file_chooser_dialog_new("Open Data", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
                path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        }
        gtk_widget_destroy(d);
        if (path == NULL) {
                return;
        }
        g_chdir(path);
        g_free(path);
        f = fopen("records.csv", "r");
        geoData = BioGeoRead(f, &err);
        fclose(f);
        if (err != NULL) {
                errMsg(GTK_WINDOW(data), err);
                g_free(err);
                return;
        }
        f = fopen("trees.csv", "r");
        treeData = TreeRead(f, &err);
        fclose(f);
        if (err != NULL) {
                errMsg(GTK_WINDOW(data), err);
                g_free(err);
                return;
        }
        PrepareTrees(treeData, geoData);
        changeTree(g_ptr_array_index(treeData, 0));
        gtk_widget_set_sensitive(GTK_WIDGET(item), FALSE);
        gtk_widget_set_sensitive(itemRecons, TRUE);
        title = g_strdup_printf("ev.view: %s", activeTree->ID);
        gtk_window_set_title(GTK_WINDOW(data), title);
        g_free(title);
}

static void setRec(int j, gpointer w) {
        gchar* title;

        if ((activeTree == NULL) || (activeTree->Recs->len == 0)) {
                return;
        }
        if (j < 0) {
                j = activeTree->Recs->len - 1;
        } else if (j >= activeTree->Recs->len) {
                j = 0;
        }
        activeTree->ActiveRec = j;
        activeRec = (Recons*) g_ptr_array_index(activeTree->Recs, j);
        drawTree();
        changeMap();
        title = g_strdup_printf("ev.view: %s: %s", activeTree->ID, activeRec->ID);
        gtk_window_set_title(GTK_WINDOW(w), title);
        g_free(title);
}

void menuOpenRec(GtkMenuItem* item, gpointer data) {
        GtkWidget* d;
        GtkFileFilter* f;
        gchar* name = NULL;
        FILE* r;
        gchar* err = NULL;

        if (activeTree == NULL) {
                return;
        }
        d = gtk_file_chooser_dialog_new("Open Reconstruction", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_OPEN,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        f = gtk_file_filter_new();
        gtk_file_filter_set_name(f, "CSV file");
        gtk_file_filter_add_pattern(f, "*.csv");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), f);
        if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
                name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        }
        gtk_widget_destroy(d);
        if (name == NULL) {
                return;
        }
        r = fopen(name, "r");
        err = ReadRecons(r, treeData);
        fclose(r);
        if (err != NULL) {
                errMsg(GTK_WINDOW(data), err);
                g_free(err);
                return;
        }
        g_free(name);
        setRec(0, data);
}

void menuOpenRaster(GtkMenuItem* item, gpointer data) {
        GtkWidget* d;
        GtkFileFilter* f;
        gchar* fname = NULL;

        d = gtk_file_chooser_dialog_new("Open Raster Map", GTK_WINDOW(data), GTK_FILE_CHOOSER_ACTION_OPEN,
                GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT, NULL);
        f = gtk_file_filter_new();
        gtk_file_filter_add_pixbuf_formats(f);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(d), f);
        if (gtk_dialog_run(GTK_DIALOG(d)) == GTK_RESPONSE_ACCEPT) {
                fname = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d));
        }
        gtk_widget_destroy(d);
        if (fname == NULL) {
                return;
        }
        setRaster(fname);
        g_free(fname);
}

void menuQuit(GtkMenuItem* item, gpointer data) {
        gtk_main_quit();
}

static gint keyPress(GtkWidget* p, GdkEventKey* e, gpointer data) {
        if (activeTree == NULL) {
                return FALSE;
        }
        if (keyTree(e)) {
                return FALSE;
        }
        if (activeRec == NULL) {
                return FALSE;
        }
        switch (e->keyval) {
        case GDK_Page_Up:
        case GDK_KP_Page_Up:
                if (e->state & GDK_CONTROL_MASK) {
                        setRec(activeTree->ActiveRec+1, data);
                }
                break;
        case GDK_Page_Down:
        case GDK_KP_Page_Down:
                if (e->state & GDK_CONTROL_MASK) {
                        setRec(activeTree->ActiveRec-1, data);
                }
                break;
        }
        return FALSE;
}

int main(int argc, char** argv) {
        GtkWidget* w;
        GtkWidget* box;
        GtkWidget* p;
        GtkWidget* f;
        GtkWidget* menu;
        GtkWidget* item;
        GtkWidget* sub;

        gtk_init(&argc, &argv);

        // creates the window
        w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(w), "ev.view");
        gtk_window_resize(GTK_WINDOW(w), 640, 480);

        box = gtk_vbox_new(FALSE, 0);

        // set menues
        menu = gtk_menu_bar_new();
        gtk_box_pack_start(GTK_BOX(box), menu, FALSE, TRUE, 0);
        // File
        item = gtk_menu_item_new_with_label("File");
        sub = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        newMenuItem(sub, "Open data...", G_CALLBACK(menuOpenData), w);
        itemRecons = newMenuItem(sub, "Open reconstruction...", G_CALLBACK(menuOpenRec), w);
        gtk_widget_set_sensitive(GTK_WIDGET(itemRecons), FALSE);
        newMenuItem(sub, "Quit", G_CALLBACK(menuQuit), NULL);
        // Map
        item = gtk_menu_item_new_with_label("Map");
        sub = gtk_menu_new();
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), sub);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
        newMenuItem(sub, "Open raster...", G_CALLBACK(menuOpenRaster), NULL);

        // Layout
        p = gtk_hpaned_new();
        gtk_widget_set_size_request(p, 100, -1);
        f = gtk_frame_new(NULL);
        gtk_paned_pack1(GTK_PANED(p), f, TRUE, FALSE);
        gtk_widget_set_size_request(f, 50, -1);
        initTree(f);
        f = gtk_frame_new(NULL);
        gtk_paned_pack2(GTK_PANED(p), f, TRUE, FALSE);
        gtk_widget_set_size_request(f, 50, -1);
        initMap(f);
        gtk_box_pack_start(GTK_BOX(box), p, TRUE, TRUE, 0);
        gtk_container_add(GTK_CONTAINER(w), box);

        gtk_widget_show_all(w);
        gtk_widget_add_events(w, GDK_KEY_PRESS_MASK);
        g_signal_connect(G_OBJECT(w), "key_press_event", G_CALLBACK(keyPress), w);

        gtk_main();
        return 0;
}


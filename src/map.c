// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <gtk/gtk.h>
#include <stdio.h>

#include "biogeo.h"
#include "tree.h"
#include "events.h"
#include "ev.h"

extern GPtrArray* geoData;

static GtkWidget* drawable;
static GdkPixmap* pixMap = NULL;
static GdkPixbuf* pixBuf = NULL;

static GdkGC* bluePen  = NULL;
static GdkGC* greenPen = NULL;
static GdkGC* redPen   = NULL;

static int clientX;
static int clientY;
static int originX = 0;
static int originY = 0;
static int startX = 0;
static int startY = 0;

static int sizeX = 720;
static int sizeY = 360;

static double north = 0;
static double south = 180;
static double east = 360;
static double west = 0;

static double scaleX = 2;
static double scaleY = 2;

static int radius = 3;

static gint expose(GtkWidget* d, GdkEventExpose* e, gpointer data) {
        gdk_draw_pixmap(drawable->window, drawable->style->fg_gc[GTK_WIDGET_STATE(drawable)],
                pixMap, e->area.x, e->area.y, e->area.x, e->area.y, e->area.width,
                e->area.height);
        return FALSE;
}

extern Tree* activeTree;
extern int   activeNode;
extern Recons* activeRec;

gboolean isAnc(PhyNode* n, int anc) {
        for (; n != NULL; n = n->Anc) {
                if (n->Index == anc) {
                        return TRUE;
                }
        }
        return FALSE;
}

GdkGC* penSet(PhyNode* n, int anc) {
        PhyNode* s;
        int i;

        if (n->Index == anc) {
                return greenPen;
        }
        for (s = n->Anc; s != NULL; s = s->Anc) {
                i = s->Index;
                switch (activeRec->Rec[i].Event) {
                case Vic:
                        if (s->Index == anc) {
                                if (activeRec->Rec[i].SetL == n->Index) {
                                        return redPen;
                                } else if (activeRec->Rec[i].SetR == n->Index) {
                                        return bluePen;
                                }
                        }
                        break;
                case SympL:
                        if (activeRec->Rec[i].SetL != n->Index) {
                                return NULL;
                        }
                        break;
                case PointR:
                case FoundR:
                        if (activeRec->Rec[i].SetL != n->Index) {
                                if (s->Index == anc) {
                                        return drawable->style->white_gc;
                                }
                                return NULL;
                        }
                        break;
                case SympR:
                        if (activeRec->Rec[i].SetR != n->Index) {
                                return NULL;
                        }
                        break;
                case PointL:
                case FoundL:
                        if (activeRec->Rec[i].SetR != n->Index) {
                                if (s->Index == anc) {
                                        return drawable->style->white_gc;
                                }
                                return NULL;
                        }
                }
                if (s->Index == anc) {
                        return greenPen;
                }
                n = s;
        }
        return NULL;
}

static void drawPixMap(GdkPixmap* px, int width, int height, int ox, int oy) {
        int i, j;
        GeoRef*  g;
        Taxon*   t;
        PhyNode* n;
        int x, y;

        GdkGC* pen;

        GdkColor green = {0, 0x0000, 0xFFFF, 0x0000};
        GdkColor red   = {0, 0xFFFF, 0x0000, 0x0000};
        GdkColor blue  = {0, 0x0000, 0x0000, 0xFFFF};

        if (bluePen == NULL) {
                bluePen = gdk_gc_new(drawable->window);
                gdk_gc_copy(bluePen, drawable->style->black_gc);
                gdk_gc_set_rgb_fg_color(bluePen, &blue);

                greenPen = gdk_gc_new(drawable->window);
                gdk_gc_copy(greenPen, drawable->style->black_gc);
                gdk_gc_set_rgb_fg_color(greenPen, &green);

                redPen = gdk_gc_new(drawable->window);
                gdk_gc_copy(redPen, drawable->style->black_gc);
                gdk_gc_set_rgb_fg_color(redPen, &red);
        }

        gdk_draw_rectangle(px, drawable->style->white_gc, TRUE, 0, 0, width, height);
        gdk_draw_rectangle(px, drawable->style->black_gc, FALSE, ox, oy, 2 * MaxLon * scaleX, 2 * MaxLat * scaleY);
        if (pixBuf != NULL) {
                gdk_draw_pixbuf(pixMap, drawable->style->fg_gc[GTK_WIDGET_STATE (drawable)], pixBuf,
                       0, 0, ox+startX, oy+startY, -1, -1, GDK_RGB_DITHER_NONE, 0, 0);
        }
        if ((geoData == NULL) || (activeTree == NULL) || (activeNode < 0)) {
                return;
        }

        if (activeRec == NULL) {
                for (i = 0; i < geoData->len; i++) {
                        t = (Taxon*) g_ptr_array_index(geoData, i);
                        n = GetTerm(activeTree, t->Name);
                        if (n == NULL) {
                                continue;
                        }
                        if (!isAnc(n, activeNode)) {
                                continue;
                        }
                        for (j = 0; j < t->Recs->len; j++) {
                                g = (GeoRef*)g_ptr_array_index(t->Recs, j);
                                x = g->X + ox;
                                y = g->Y + oy;
                                gdk_draw_rectangle(px, greenPen, TRUE, x-radius, y-radius, (2*radius)+1, (2*radius)+1);
                                gdk_draw_rectangle(px, drawable->style->black_gc, FALSE, x-radius, y-radius, (2*radius)+1, (2*radius)+1);
                        }
                }
                return;
        }
        for (i = 0; i < geoData->len; i++) {
                t = (Taxon*) g_ptr_array_index(geoData, i);
                n = GetTerm(activeTree, t->Name);
                if (n == NULL) {
                        continue;
                }
                pen = penSet(n, activeNode);
                if (pen == NULL) {
                        continue;
                }
                for (j = 0; j < t->Recs->len; j++) {
                        g = (GeoRef*)g_ptr_array_index(t->Recs, j);
                        x = g->X + ox;
                        y = g->Y + oy;
                        gdk_draw_arc(pixMap, pen, TRUE, x-radius, y-radius, (2*radius)+1, (2*radius)+1, 0, (360*64));
                        gdk_draw_arc(pixMap, drawable->style->black_gc, FALSE, x-radius, y-radius, (2*radius)+1, (2*radius)+1, 0, (360*64));
                }
        }
}

static void drawMap() {
        if (pixMap == NULL) {
                pixMap = gdk_pixmap_new(drawable->window, clientX, clientY, -1);
        }
        drawPixMap(pixMap, clientX, clientY, originX, originY);
        gdk_window_invalidate_rect(drawable->window, NULL, TRUE);
}

static gint configure(GtkWidget* d, GdkEvent* e, gpointer data) {
        clientX = drawable->allocation.width;
        clientY = drawable->allocation.height;
        if (pixMap != NULL) {
                g_object_unref(pixMap);
        }
        pixMap = gdk_pixmap_new(drawable->window, clientX, clientY, -1);
        drawMap();
        return TRUE;
}

static int prevX;
static int prevY;

static gint mouseMove(GtkWidget* d, GdkEventMotion* e, gpointer data) {
        int delta;

        if (!(e->state & GDK_BUTTON1_MASK)) {
                prevX = 0;
                prevY = 0;
                return TRUE;
        }
        if ((prevX != 0) || (prevY != 0)) {
                delta = e->x - prevX;
                if ((delta > 0) && (originX != (clientX - 10))) {
                        originX += delta;
                        if (originX > (clientX - 10)) {
                                originX = clientX - 10;
                        }
                } else if ((delta < 0) && (originX != -((2 * MaxLon * scaleX) - 10))) {
                        originX += delta;
                        if (originX < -((2 * MaxLon * scaleX) - 10)) {
                                originX = -((2 * MaxLon * scaleX) - 10);
                        }
                }
                delta = e->y - prevY;
                if ((delta > 0) && (originY != (clientY - 10))) {
                        originY += delta;
                        if (originY > (clientY - 10)) {
                                originY = clientY - 10;
                        }
                } else if ((delta < 0) && (originY != -((2 * MaxLat * scaleY) - 10))) {
                        originY += delta;
                        if (originY < -((2 * MaxLat * scaleY) - 10)) {
                                originY = -((2 * MaxLat * scaleY) - 10);
                        }
                }
                drawMap();
        }
        prevX = e->x;
        prevY = e->y;
        return TRUE;
}

static gint mouseButton(GtkWidget* d, GdkEventButton* e, gpointer data) {
        return FALSE;
}

void initMap(GtkWidget* parent) {
        drawable = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(parent), drawable);
        g_signal_connect(G_OBJECT(drawable), "expose_event", G_CALLBACK(expose), NULL);
        g_signal_connect(G_OBJECT(drawable), "configure_event", G_CALLBACK(configure), NULL);
        gtk_widget_add_events(drawable, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK);
        g_signal_connect(G_OBJECT(drawable), "motion_notify_event", G_CALLBACK(mouseMove), NULL);
        g_signal_connect(G_OBJECT(drawable), "button_press_event", G_CALLBACK(mouseButton), NULL);
}

static void prepare() {
        int i, j;
        GeoRef* g;
        Taxon*  t;

        for (i = 0; i < geoData->len; i++) {
                t = (Taxon*) g_ptr_array_index(geoData, i);
                for (j = 0; j < t->Recs->len; j++) {
                        g = (GeoRef*)g_ptr_array_index(t->Recs, j);
                        g->X = (int)((180.0 + g->Lon) * scaleX);
                        g->Y = (int)((90.0 - g->Lat) * scaleY);
                }
        }
}

void changeMap() {
        if (geoData != NULL) {
                prepare();
        }
        drawMap();
}

void setRaster(gchar* fname) {
        GdkPixbuf* px;

        px = gdk_pixbuf_new_from_file(fname, NULL);
        if (px == NULL) {
                return;
        }
        if (pixBuf != NULL) {
                g_object_unref(pixBuf);
        }
        pixBuf = px;
        sizeX = gdk_pixbuf_get_width(pixBuf);
        sizeY = gdk_pixbuf_get_height(pixBuf);
        originX = 0;
        originY = 0;
        startX = 0;
        startY = 0;
        north = 0;
        south = 180;
        east = 360;
        west = 0;
        scaleX = ((double)sizeX) / (east - west);
        scaleY = ((double)sizeY) / (south - north);
        changeMap();
}

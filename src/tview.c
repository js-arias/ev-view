// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>

#include "tree.h"
#include "events.h"
#include "ev.h"

static GtkWidget* drawable;
static GdkPixmap* pixMap = NULL;

static int clientX;
static int clientY;
static int originX = -5;
static int originY = -10;

static GdkGC* bluePen  = NULL;
static GdkGC* greenPen = NULL;
static GdkGC* redPen   = NULL;

Tree* activeTree = NULL;
int   activeNode = -1;
extern Recons* activeRec;

static void changeNode(Tree* t, PhyNode* n, int level) {
        PhyNode* d;
        int topY;
        int botY;

        n->CurX = (level - n->Level) * t->StepX;
        if (n->First == NULL) {
                n->CurY = n->Y * t->StepY;
                if ((strlen(n->Term) > 0) && (n->Layout == NULL)) {
                        n->Layout = gtk_widget_create_pango_layout(drawable, n->Term);
                }
                if (n->CurY > t->MaxY) {
                        t->MaxY = n->CurY;
                }
                return;
        }
        topY = 64000 * t->StepY;
        botY = -1;
        for (d = n->First; d != NULL; d = d->Sister) {
                changeNode(t, d, level);
                if (d->CurY < topY) {
                        topY = d->CurY;
                }
                if (d->CurY > botY) {
                        botY = d->CurY;
                }
        }
        n->TopY = topY;
        n->BotY = botY;
        n->CurY = topY + ((botY - topY) / 2);
}

void paintArrow(int x, int y, gboolean top) {
        GdkPoint p[3];

        p[0].x = x - 2;
        p[0].y = y;
        p[1].x = x + 2;
        p[1].y = y;
        p[2].x = x;
        if (top) {
                p[2].y = y - 5;
        } else {
                p[2].y = y + 5;
        }
        gdk_draw_polygon(pixMap, drawable->style->white_gc, TRUE, p, 3);
        gdk_draw_polygon(pixMap, drawable->style->black_gc, FALSE, p, 3);
}

void paintTree() {
        PhyNode* n;
        PhyNode* s;
        int i;
        int si;
        int startX;
        int startY;
        int endX;
        int endY;

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

        if (activeNode >= 0) {
                n = (PhyNode*)g_ptr_array_index(activeTree->Nodes, activeNode);
                startX = n->CurX - originX;
                startY = n->CurY - originY;
                if ((startX >= 0) && (startX < clientX) && (startY >= 0) && (startY < clientY)) {
                        gdk_draw_arc(pixMap, greenPen, TRUE, startX-5, startY-5, 11, 11, 0, (360*64));
                }
        }

        for (i = 0; i < activeTree->Nodes->len; i++) {
                n = (PhyNode*)g_ptr_array_index(activeTree->Nodes, i);
                startX = n->CurX - originX;
                startY = n->CurY - originY;
                if (n->Anc != NULL) {
                        endX = n->Anc->CurX - originX;
                } else {
                        endX = startX-2;
                }
                if ((startY >= 0) && (startY < clientY) && (!(((endX < 0) && (startX < 0)) || ((endX >= clientX) && (startX >= clientX))))) {
                        gdk_draw_line(pixMap, drawable->style->black_gc, startX, startY, endX, startY);
                        if (n->Layout != NULL) {
                                gdk_draw_layout(pixMap, drawable->style->black_gc, startX + 5, startY - 10, (PangoLayout*)n->Layout);
                        }
                }
                if (n->First != NULL) {
                        startY = n->TopY - originY;
                        endY = n->BotY - originY;
                        if ((startX >= 0) && (startX < clientX) && (!(((startY < 0) && (endY < 0)) || ((startY >= clientY) && (endY >= clientY))))) {
                                gdk_draw_line(pixMap, drawable->style->black_gc, startX, startY, startX, endY);
                        }
                }
        }

        if (activeRec == NULL) {
                return;
        }

        for (i = 0; i < activeTree->Nodes->len; i++) {
                n = (PhyNode*)g_ptr_array_index(activeTree->Nodes, i);
                startX = n->CurX - originX;
                startY = n->CurY - originY;

                if ((startX < 0) || (startX >= clientX) || (startY < 0) || (startY >= clientY)) {
                        continue;
                }

                switch (activeRec->Rec[i].Event) {
                case Vic:
                        gdk_draw_rectangle(pixMap, drawable->style->black_gc, TRUE, startX-2, startY-2, 5, 5);
                        break;
                case SympU:
                case SympL:
                case SympR:
                        gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, startX-2, startY-2, 5, 5);
                        gdk_draw_rectangle(pixMap, drawable->style->black_gc, FALSE, startX-2, startY-2, 5, 5);
                        break;
                case PointL:
                        si = activeRec->Rec[i].SetL;
                        s = (PhyNode*)g_ptr_array_index(activeTree->Nodes, si);
                        if (s->CurY > n->CurY) {
                                gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, startX-2, startY+2, 5, 5);
                                gdk_draw_arc(pixMap, drawable->style->black_gc, FALSE, startX-2, startY+2, 5, 5, 0, (360*64));
                        } else {
                                gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, startX-2, startY-7, 5, 5);
                                gdk_draw_arc(pixMap, drawable->style->black_gc, FALSE, startX-2, startY-7, 5, 5, 0, (360*64));
                        }
                        break;
                case PointR:
                        si = activeRec->Rec[i].SetR;
                        s = (PhyNode*)g_ptr_array_index(activeTree->Nodes, si);
                        if (s->CurY > n->CurY) {
                                gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, startX-2, startY+2, 5, 5);
                                gdk_draw_arc(pixMap, drawable->style->black_gc, FALSE, startX-2, startY+2, 5, 5, 0, (360*64));
                        } else {
                                gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, startX-2, startY-7, 5, 5);
                                gdk_draw_arc(pixMap, drawable->style->black_gc, FALSE, startX-2, startY-7, 5, 5, 0, (360*64));
                        }
                        break;
                case FoundL:
                        si = activeRec->Rec[i].SetL;
                        s = (PhyNode*)g_ptr_array_index(activeTree->Nodes, si);
                        if (s->CurY > n->CurY) {
                                paintArrow(startX, startY+2, FALSE);
                        } else {
                                paintArrow(startX, startY-2, TRUE);
                        }
                        break;
                case FoundR:
                        si = activeRec->Rec[i].SetR;
                        s = (PhyNode*)g_ptr_array_index(activeTree->Nodes, si);
                        if (s->CurY > n->CurY) {
                                paintArrow(startX, startY+2, FALSE);
                        } else {
                                paintArrow(startX, startY-2, TRUE);
                        }
                        break;
                }
        }
}

static gint expose(GtkWidget* d, GdkEventExpose* e, gpointer data) {
        gdk_draw_pixmap(drawable->window, drawable->style->fg_gc[GTK_WIDGET_STATE(drawable)],
                pixMap, e->area.x, e->area.y, e->area.x, e->area.y, e->area.width,
                e->area.height);
        return FALSE;
}

void drawTree() {
        if (pixMap == NULL) {
                pixMap = gdk_pixmap_new(drawable->window, clientX, clientY, -1);
        }
        gdk_draw_rectangle(pixMap, drawable->style->white_gc, TRUE, 0, 0, clientX,
                clientY);
        if (activeTree != NULL) {
                paintTree();
        }
        gdk_window_invalidate_rect(drawable->window, NULL, TRUE);
}

void changeTree(Tree* t) {
        if (activeTree != t) {
                originX = -5;
                originY = -10;
                activeTree = t;
                activeNode = 0;
                changeMap();
        }
        if (t->StepX == 0) {
                t->StepX = 20;
        }
        if (t->StepY == 0) {
                t->StepY = 20;
        }
        t->MaxY = -1;
        changeNode(t, t->Root, t->Root->Level);
        drawTree();
}

static gint configure(GtkWidget* d, GdkEvent* e, gpointer data) {
        clientX = drawable->allocation.width;
        clientY = drawable->allocation.height;
        if (pixMap != NULL) {
                g_object_unref(pixMap);
        }
        pixMap = gdk_pixmap_new(drawable->window, clientX, clientY, -1);
        drawTree(FALSE);
        return TRUE;
}

static int searchNearest(int x, int y) {
        int best, cur, ret;
        int delX, delY;
        PhyNode* n;
        int i;

        best = 15;
        ret = -1;
        for (i = 0; i < activeTree->Nodes->len; i++) {
                n = (PhyNode*)g_ptr_array_index(activeTree->Nodes, i);
                delX = x - n->CurX;
                delX = (delX >= 0)? delX : -delX;
                if (delX > 5) {
                        continue;
                }
                delY = y - n->CurY;
                delY = (delY >= 0)? delY : -delY;
                if (delY > 5) {
                        continue;
                }
                cur = delX + delY;
                if (cur < best) {
                        best = cur;
                        ret = i;
                }
        }
        return ret;
}

static gint mouseButton(GtkWidget* d, GdkEventButton* e, gpointer data) {
        int n;

        if (activeTree == NULL) {
                return FALSE;
        }
        if ((e->type != GDK_BUTTON_PRESS) || (e->button != 1)) {
                return FALSE;
        }
        n = searchNearest(e->x + originX, e->y + originY);
        if ((n >= 0) && (activeNode != n)) {
                activeNode = n;
                drawTree();
                changeMap();
        }
        return TRUE;
}

static int prevX = 0;
static int prevY = 0;

static gint mouseMove(GtkWidget* d, GdkEventMotion* e, gpointer data) {
        int delta;

        if (activeTree == NULL) {
                return FALSE;
        }
        if (!(e->state & GDK_BUTTON1_MASK)) {
                prevX = 0;
                prevY = 0;
                return TRUE;
        }
        if ((prevX != 0) || (prevY != 0)) {
                delta = e->x - prevX;
                if ((delta > 0) && (originX != -(clientX - 10))) {
                        originX -= delta;
                        if (originX < -(clientX - 10)) {
                                originX = -(clientX - 10);
                        }
                } else if ((delta < 0) && (originX != (activeTree->Root->Level * activeTree->StepX))) {
                        originX -= delta;
                        if (originX > (activeTree->Root->Level * activeTree->StepX)) {
                                originX = activeTree->Root->Level * activeTree->StepX;
                        }
                }
                delta = e->y - prevY;
                if ((delta > 0) && (originY != (activeTree->MaxY - 10))) {
                        originY -= delta;
                        if (originY > (activeTree->MaxY - 10)) {
                                originY = activeTree->MaxY - 10;
                        }
                } else if ((delta < 0) && (originY != (clientY - 10))) {
                        originY -= delta;
                        if (originY < -(clientY - 10)) {
                                originY = -(clientY - 10);
                        }
                }
                drawTree();
        }
        prevX = e->x;
        prevY = e->y;
        return TRUE;
}

static gint mouseWheel(GtkWidget* d, GdkEventScroll* e, gpointer data) {
        if (activeTree == NULL) {
                return FALSE;
        }
        switch (e->direction) {
        case GDK_SCROLL_UP:
                if (originY == -(clientY - 10)) {
                        return FALSE;
                }
                originY -= 5;
                if (originY < -(clientY - 10)) {
                        originY = -(clientY - 10);
                }
                break;
        case GDK_SCROLL_DOWN:
                if (originY == (activeTree->MaxY - 10)) {
                        return FALSE;
                }
                originY += 5;
                if (originY > (activeTree->MaxY - 10)) {
                        originY = activeTree->MaxY - 10;
                }
                break;
        default:
                return FALSE;
        }
        drawTree();
        return FALSE;
}

gboolean keyTree(GdkEventKey* e) {
        if (activeTree == NULL) {
                return FALSE;
        }
        switch (e->keyval) {
        case GDK_plus:
        case GDK_KP_Add:
                if (e->state & GDK_CONTROL_MASK) {
                        activeTree->StepX++;
                } else if (e->state & GDK_SHIFT_MASK) {
                        activeTree->StepY++;
                } else {
                        return FALSE;
                }
                changeTree(activeTree);
                break;
        case GDK_minus:
        case GDK_KP_Subtract:
                if (e->state & GDK_CONTROL_MASK) {
                        if (activeTree->StepX <= 1) {
                                return FALSE;
                        }
                        activeTree->StepX--;
                } else if (e->state & GDK_SHIFT_MASK) {
                        if (activeTree->StepY <= 1) {
                                return FALSE;
                        }
                        activeTree->StepY--;
                } else {
                        return FALSE;
                }
                changeTree(activeTree);
                break;
        case GDK_Up:
        case GDK_KP_Up:
                if (e->state & GDK_CONTROL_MASK) {
                        if (activeTree->StepY <= 1) {
                                return FALSE;
                        }
                        activeTree->StepY--;
                } else {
                        if (originY == (clientY - 10)) {
                                return FALSE;
                        }
                        originY -= 5;
                        if (originY < -(clientY - 10)) {
                                originY = -(clientY - 10);
                        }
                        break;
                }
                changeTree(activeTree);
                break;
        case GDK_Down:
        case GDK_KP_Down:
                if (e->state & GDK_CONTROL_MASK) {
                        activeTree->StepY++;
                } else {
                        if (originY == (activeTree->MaxY - 10)) {
                                return FALSE;
                        }
                        originY += 5;
                        if (originY > (activeTree->MaxY - 10)) {
                                originY = activeTree->MaxY - 10;
                        }
                        break;
                }
                changeTree(activeTree);
                break;
        case GDK_Left:
        case GDK_KP_Left:
                if (e->state & GDK_CONTROL_MASK) {
                        if (activeTree->StepX <= 1) {
                                return FALSE;
                        }
                        activeTree->StepX--;
                } else {
                        if (originX == (activeTree->Root->Level * activeTree->StepX)) {
                                return FALSE;
                        }
                        originX += 5;
                        if (originX > (activeTree->Root->Level * activeTree->StepX)) {
                                originX = activeTree->Root->Level * activeTree->StepX;
                        }
                        break;
                }
                changeTree(activeTree);
                break;
        case GDK_Right:
        case GDK_KP_Right:
                if (e->state & GDK_CONTROL_MASK) {
                        activeTree->StepX++;
                } else {
                        if (originX == -(clientX - 10)) {
                                return FALSE;
                        }
                        originX -= 5;
                        if (originX < -(clientX - 10)) {
                                originX = -(clientX - 10);
                        }
                        break;
                }
                changeTree(activeTree);
                break;
        case GDK_Page_Up:
        case GDK_KP_Page_Up:
                if ((e->state & GDK_CONTROL_MASK) || (e->state & GDK_SHIFT_MASK)) {
                        return FALSE;
                }
                if (originY == (clientY - 10)) {
                        return FALSE;
                }
                originY -= clientY;
                if (originY < -(clientY - 10)) {
                        originY = -(clientY - 10);
                }
                break;
        case GDK_Page_Down:
        case GDK_KP_Page_Down:
                if ((e->state & GDK_CONTROL_MASK) || (e->state & GDK_SHIFT_MASK)) {
                        return FALSE;
                }
                if (originY == (activeTree->MaxY - 10)) {
                        return FALSE;
                }
                originY += clientY;
                if (originY > (activeTree->MaxY - 10)) {
                        originY = activeTree->MaxY - 10;
                }
                break;
        default:
                return FALSE;
        }
        drawTree();
        return TRUE;
}

void initTree(GtkWidget* parent) {
        drawable = gtk_drawing_area_new();
        gtk_container_add(GTK_CONTAINER(parent), drawable);
        g_signal_connect(G_OBJECT(drawable), "expose_event", G_CALLBACK(expose), NULL);
        g_signal_connect(G_OBJECT(drawable), "configure_event", G_CALLBACK(configure), NULL);
        gtk_widget_add_events(drawable, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK);
        g_signal_connect(G_OBJECT(drawable), "motion_notify_event", G_CALLBACK(mouseMove), NULL);
        g_signal_connect(G_OBJECT(drawable), "button_press_event", G_CALLBACK(mouseButton), NULL);
        g_signal_connect(G_OBJECT(drawable), "scroll_event", G_CALLBACK(mouseWheel), NULL);
}


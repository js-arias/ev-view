// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "csv.h"
#include "tree.h"
#include "events.h"

Recons* OR(Tree* t) {
        int i;
        int lf, rt, desc;
        PhyNode* n;
        PhyNode* d;
        Recons*  o = g_malloc0(sizeof(Recons));

        o->ID = g_strdup("or");
        o->Tree = t;
        o->Rec = g_malloc0(sizeof(RecNode)*t->Nodes->len);
        for (i = 0; i < t->Nodes->len; i++) {
                o->Rec[i].Event = Undef;
                n = (PhyNode*)g_ptr_array_index(t->Nodes, i);
                o->Rec[i].Node = n;
                o->Rec[i].SetL = -1;
                o->Rec[i].SetR = -1;
                if ((n->First == NULL) || (!n->HasData)) {
                        continue;
                }
                lf = -1;
                rt = -1;
                desc = 0;
                for (d = n->First; d != NULL; d = d->Sister) {
                        if (!d->HasData) {
                                continue;
                        }
                        if (lf < 0) {
                                lf = d->Index;
                        } else if (rt < 0) {
                                rt = d->Index;
                        }
                        desc++;
                }
                if (desc != 2) {
                        continue;
                }
                o->Rec[i].SetL = lf;
                o->Rec[i].SetR = rt;
        }
        return o;
}

static void delArray(GPtrArray* a) {
        int    i;
        gchar* s;

        for (i = 0; i < a->len; i++) {
                s = (gchar*)g_ptr_array_index(a, i);
                g_free(s);
        }
        g_ptr_array_free(a, TRUE);
}
gchar* ReadRecons(FILE* f, GPtrArray* tr) {
        GPtrArray* h;
        GPtrArray* row;
        gchar*     error;
        gchar*     err;
        gchar*     s;
        CSVReader* r;
        int treeF = -1;
        int idF = -1;
        int nodeF = -1;
        int eventF = -1;
        int setF = -1;
        int i;
        int setN;
        Recons*  nr;
        Tree*    t;
        PhyNode* n;

        r = NewCSVReader(f);

        // reads the file header
        h = CSVRead(r);
        if ((err = CSVReaderError(r)) != NULL) {
                error = g_strdup_printf("header (recons): %s", err);
                DelCSVReader(r);
                return error;
        }
        for (i = 0; i < h->len; i++) {
                s = g_ptr_array_index(h, i);
                g_strstrip(s);
                s = g_ascii_strdown(s, -1);
                if (strcmp(s, "tree") == 0) {
                        treeF = i;
                }
                if ((strcmp(s, "node") == 0) || (strcmp(s, "node id") == 0)) {
                        nodeF = i;
                }
                if (strcmp(s, "id") == 0) {
                        idF = i;
                }
                if ((strcmp(s, "event") == 0) || (strcmp(s, "ev") == 0)) {
                        eventF = i;
                }
                if (strcmp(s, "set") == 0) {
                        setF = i;
                }
        }
        delArray(h);
        if ((treeF < 0) || (nodeF < 0) || (idF < 0) || (eventF < 0) || (setF < 0)) {
                error = g_strdup("header (recons): incomplete header");
                DelCSVReader(r);
                return error;
        }

        for (;;) {
                row = CSVRead(r);
                if ((err = CSVReaderError(r)) != NULL) {
                        if (strcmp(err, "EOF") == 0) {
                                break;
                        }
                        error = g_strdup_printf("(recons): line: %d, %s", CSVReaderLine(r), err);
                        DelCSVReader(r);
                        return error;
                }
                if ((row->len <= treeF) || (row->len <= nodeF) || (row->len <= idF) || (row->len <= eventF)) {
                        delArray(row);
                        continue;
                }
                if ((strlen(g_ptr_array_index(row, treeF)) == 0) || (strlen(g_ptr_array_index(row, idF)) == 0) || (strlen(g_ptr_array_index(row, nodeF)) == 0)) {
                        delArray(row);
                        continue;
                }
                t = GetTree(tr, g_ptr_array_index(row, treeF));
                if (t == NULL) {
                        delArray(row);
                        continue;
                }
                nr = GetRec(t->Recs, t->ID, g_ptr_array_index(row, idF));
                if (nr == NULL) {
                        nr = OR(t);
                        g_free(nr->ID);
                        nr->ID = g_strdup(g_ptr_array_index(row, idF));
                        g_ptr_array_add(t->Recs, nr);
                }
                if (strcmp(g_ptr_array_index(row, eventF), "*") == 0) {
                        delArray(row);
                        continue;
                }
                n = GetNode(t, g_ptr_array_index(row, nodeF));
                if (n == NULL) {
                        delArray(row);
                        continue;
                }
                i = n->Index;
                s = g_ptr_array_index(row, eventF);
                switch (s[0]) {
                case 'v':
                        nr->Rec[i].Event = Vic;
                        break;
                case 's':
                        nr->Rec[i].Event = SympU;
                        s = g_ptr_array_index(row, setF);
                        if (s[0] == '*') {
                                break;
                        }
                        if (sscanf(s, "%d", &setN) == 0) {
                                break;
                        }
                        if (nr->Rec[i].SetL == setN) {
                                nr->Rec[i].Event = SympL;
                        } else if (nr->Rec[i].SetR == setN) {
                                nr->Rec[i].Event = SympR;
                        }
                        break;
                case 'p':
                        s = g_ptr_array_index(row, setF);
                        if (sscanf(s, "%d", &setN) == 0) {
                                delArray(row);
                                error = g_strdup_printf("(recons): line: %d, invalid set", CSVReaderLine(r));
                                DelCSVReader(r);
                                return error;
                        }
                        if (nr->Rec[i].SetL == setN) {
                                nr->Rec[i].Event = PointR;
                        } else if (nr->Rec[i].SetR == setN) {
                                nr->Rec[i].Event = PointL;
                        } else {
                                delArray(row);
                                error = g_strdup_printf("(recons): line: %d, invalid set", CSVReaderLine(r));
                                DelCSVReader(r);
                                return error;
                        }
                        break;
                case 'f':
                        s = g_ptr_array_index(row, setF);
                        if (sscanf(s, "%d", &setN) == 0) {
                                delArray(row);
                                error = g_strdup_printf("(recons): line: %d, invalid set", CSVReaderLine(r));
                                DelCSVReader(r);
                                return error;
                        }
                        if (nr->Rec[i].SetL == setN) {
                                nr->Rec[i].Event = FoundR;
                        } else if (nr->Rec[i].SetR == setN) {
                                nr->Rec[i].Event = FoundL;
                        } else {
                                delArray(row);
                                error = g_strdup_printf("(recons): line: %d, invalid set", CSVReaderLine(r));
                                DelCSVReader(r);
                                return error;
                        }
                        break;
                default:
                        delArray(row);
                        error = g_strdup_printf("(recons): line: %d, unknown event", CSVReaderLine(r));
                        DelCSVReader(r);
                        return error;
                }
        }
        DelCSVReader(r);
        return NULL;
}

Recons* GetRec(GPtrArray* recs, gchar* treeID, gchar* ID) {
        int     i;
        Recons* r;

        for (i = 0; i < recs->len; i++) {
                r = (Recons*)g_ptr_array_index(recs, i);
                if ((strcmp(r->ID, ID) == 0) && (strcmp(r->Tree->ID, treeID) == 0)) {
                        return r;
                }
        }
        return NULL;
}

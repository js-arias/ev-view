// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "csv.h"
#include "biogeo.h"
#include "tree.h"

Tree* GetTree(GPtrArray* tr, gchar* id) {
        int i;
        Tree* t;

        for (i = 0; i < tr->len; i++) {
                t = (Tree*)g_ptr_array_index(tr, i);
                if (strcmp(t->ID, id) == 0) {
                        return t;
                }
        }
        return NULL;
}

static void delTreeArray(GPtrArray* tr) {
        int i, j;
        Tree*    t;
        PhyNode* n;

        for (i = 0; i < tr->len; i++) {
                t = (Tree*)g_ptr_array_index(tr, i);
                for (j = 0; j < t->Nodes->len; j++) {
                        n = (PhyNode*)g_ptr_array_index(t->Nodes, j);
                        g_free(n->Term);
                        g_free(n->ID);
                        g_free(n);
                }
                g_ptr_array_free(t->Nodes, TRUE);
                g_ptr_array_free(t->Recs, TRUE);
                g_free(t->ID);
                g_free(t);
        }
        g_ptr_array_free(tr, TRUE);
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

PhyNode* GetNode(Tree* t, gchar* id) {
        int      i;
        PhyNode* n;

        for (i = 0; i < t->Nodes->len; i++) {
                n = (PhyNode*)g_ptr_array_index(t->Nodes, i);
                if (strcmp(n->ID, id) == 0) {
                        return n;
                }
        }
        return NULL;
}

GPtrArray* TreeRead(FILE* f, gchar** error) {
        GPtrArray* tr;
        GPtrArray* h;
        GPtrArray* row;
        gchar*     err;
        gchar*     s;
        CSVReader* r;
        int treeF = -1;
        int nodeF = -1;
        int ancF  = -1;
        int termF = -1;
        int i;
        Tree* t;
        PhyNode* n;
        PhyNode* a;
        PhyNode* d;

        r = NewCSVReader(f);

        // reads the file header
        h = CSVRead(r);
        if ((err = CSVReaderError(r)) != NULL) {
                *error = g_strdup_printf("header (tree): %s", err);
                DelCSVReader(r);
                return NULL;
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
                if ((strcmp(s, "ancestor") == 0) || (strcmp(s, "anc") == 0) || (strcmp(s, "parent") == 0)) {
                        ancF = i;
                }
                if ((strcmp(s, "terminal") == 0) || (strcmp(s, "term") == 0) || (strcmp(s, "termname") == 0)) {
                        termF = i;
                }
        }
        delArray(h);
        if ((treeF < 0) || (nodeF < 0) || (ancF < 0) || (termF < 0)) {
                *error = g_strdup("header (tree): incomplete header");
                DelCSVReader(r);
                return NULL;
        }

        // read the data
        tr = g_ptr_array_new();
        for (;;) {
                row = CSVRead(r);
                if ((err = CSVReaderError(r)) != NULL) {
                        if (strcmp(err, "EOF") == 0) {
                                break;
                        }
                        delTreeArray(tr);
                        *error = g_strdup_printf("(tree): line: %d, %s", CSVReaderLine(r), err);
                        DelCSVReader(r);
                        return NULL;
                }
                if ((row->len <= treeF) || (row->len <= nodeF) || (row->len <= ancF)) {
                        delArray(row);
                        continue;
                }
                if ((strlen(g_ptr_array_index(row, treeF)) == 0) || (strlen(g_ptr_array_index(row, nodeF)) == 0)) {
                        delArray(row);
                        continue;
                }
                s = (gchar*)g_ptr_array_index(row, treeF);
                t = GetTree(tr, s);
                if (t == NULL) {
                        t = (Tree*)g_malloc0(sizeof(Tree));
                        t->ID = g_strdup(s);
                        t->Nodes = g_ptr_array_new();
                        t->Recs = g_ptr_array_new();
                        g_ptr_array_add(tr, t);
                }
                s = (gchar*)g_ptr_array_index(row, nodeF);
                if (GetNode(t, s) != NULL) {
                        delTreeArray(tr);
                        *error = g_strdup_printf("(tree): line: %d, node id %s already in use", CSVReaderLine(r), s);
                        delArray(row);
                        DelCSVReader(r);
                        return NULL;
                }
                n = (PhyNode*)g_malloc0(sizeof(PhyNode));
                n->ID = g_strdup(s);
                n->Index = t->Nodes->len;
                n->HasData = FALSE;
                s = (gchar*)g_ptr_array_index(row, ancF);
                if ((strlen(s) == 0) || (strcmp(s, "-1") == 0) || (strcmp(s, "xx") == 0)) {
                        if (t->Root != NULL) {
                                delTreeArray(tr);
                                *error = g_strdup_printf("(tree): line: %d, tree %s already with a root", CSVReaderLine(r), t->ID);
                                delArray(row);
                                DelCSVReader(r);
                                return NULL;
                        }
                        t->Root = n;
                        g_ptr_array_add(t->Nodes, n);
                        continue;
                }
                a = GetNode(t, s);
                if (a == NULL) {
                        delTreeArray(tr);
                        *error = g_strdup_printf("(tree): line: %d, node %s without parent", CSVReaderLine(r), n->ID);
                        delArray(row);
                        DelCSVReader(r);
                        return NULL;
                }
                n->Anc = a;
                if (a->First != NULL) {
                        for (d = a->First; ; d = d->Sister) {
                                if (d->Sister == NULL) {
                                        d->Sister = n;
                                        break;
                                }
                        }
                } else {
                        a->First = n;
                }
                if ((termF >= 0) && (termF < row->len)) {
                        s = (gchar*)g_ptr_array_index(row, termF);
                        if (strlen(s) > 0) {
                                n->Term = g_strdup(s);
                        }
                }
                g_ptr_array_add(t->Nodes, n);
        }
        DelCSVReader(r);
        return tr;
}

static void prepareNode(PhyNode* n, int* y) {
        PhyNode* d;

        if (n->Anc != NULL) {
                n->Nest = n->Anc->Nest + 1;
        }
        if (n->First == NULL) {
                n->Y = *y;
                *y += 1;
                return;
        }
        for (d = n->First; d != NULL; d = d->Sister) {
                prepareNode(d, y);
                if (d->Level >= n->Level) {
                        n->Level = d->Level + 1;
                }
                if (d->HasData) {
                        n->HasData = TRUE;
                }
        }
}

void PrepareTrees(GPtrArray* tr, GPtrArray* txLs) {
        int i;
        int j;
        int y;
        Tree*    t;
        PhyNode* n;
        Taxon*   tx;

        for (i = 0; i < tr->len; i++) {
                t = (Tree*)g_ptr_array_index(tr, i);

                // Set the terminal nodes with data
                for (j = 0; j < txLs->len; j++) {
                        tx = (Taxon*) g_ptr_array_index(txLs, j);
                        n = GetTerm(t, tx->Name);
                        if (n == NULL) {
                                continue;
                        }
                        if (tx->Recs->len > 0) {
                                n->HasData = TRUE;
                        }
                }
                y = 0;
                prepareNode(t->Root, &y);
        }
}

PhyNode* GetTerm(Tree* t, gchar* name) {
        int      i;
        PhyNode* n;

        for (i = 0; i < t->Nodes->len; i++) {
                n = (PhyNode*)g_ptr_array_index(t->Nodes, i);
                if ((n->Term == NULL) || (strlen(n->Term) == 0)) {
                        continue;
                }
                if (strcmp(n->Term, name) == 0) {
                        return n;
                }
        }
        return NULL;
}


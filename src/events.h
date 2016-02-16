// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

// Events
#define Undef  0
#define Vic    1
#define SympU  2
#define SympL  3
#define SympR  4
#define PointL 5
#define PointR 6
#define FoundL 7
#define FoundR 8

// A RecNode is an event reconstruction in a given node.
typedef struct {
        PhyNode* Node;
        int SetL;
        int SetR;
        int Event;
}RecNode;

// A Recons is a reconstruction on a particular tree.
typedef struct {
        gchar* ID;
        Tree*  Tree;
        RecNode* Rec;
}Recons;

// ReadRecons reads reconstructions in file f, in CSV format, and associated
// them to trees in tr. It return an error string in case of an error, or
// NULL.
gchar* ReadRecons(FILE* f, GPtrArray* tr);

// OR returns an or reconstruction for a tree t.
Recons* OR(Tree* t);

// GetRec returns a reconstruction with a given ID.
Recons* GetRec(GPtrArray* recs, gchar* treeID, gchar* ID);

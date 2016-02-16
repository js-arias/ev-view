// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

// A PhyNode is a node of a phylogenetic tree.
typedef struct _PhyNode PhyNode;
struct _PhyNode {
        int    Index;
        gchar* ID;
        PhyNode* Anc;
        PhyNode* Sister;
        PhyNode* First;
        gchar* Term;
        gboolean HasData;
        // graphic part
        int Level;
        int Nest;
        int Y;
        // current screen
        int CurX;
        int CurY;
        int TopY;
        int BotY;
        gpointer Layout;
};

// A Tree is a phylogenetic tree.
typedef struct {
        gchar*     ID;
        PhyNode*   Root;
        GPtrArray* Nodes;
        // graphic part
        int StepX;
        int StepY;
        int MaxY;
        // reconstructions
        GPtrArray* Recs;
        int ActiveRec;
}Tree;

// ReadTree reads file f in CSV format, and returns an array of pointers to
// tree.
GPtrArray* TreeRead(FILE* f, gchar** error);

// GetTree returns a tree from an array of pointers to tree.
Tree* GetTree(GPtrArray* tr, gchar* id);

// PrepareTrees setup the virtual graphical part of the trees.
void PrepareTrees(GPtrArray* tr, GPtrArray* txLs);

// GetTerm retuns the node that contains the given terminal name, if it
// exists in the tree.
PhyNode* GetTerm(Tree* t, gchar* name);

// GetNode returns the node with a given ID.
PhyNode* GetNode(Tree* t, gchar* id);

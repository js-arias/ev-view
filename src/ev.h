// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

// initMap intializes the map widget.
void initMap(GtkWidget* parent);

// setRaster sets a raster map.
void setRaster(gchar* fname);

// changeMap should be executed when there is a changes in map properties.
void changeMap();

// changeTree should be executed when there is a change in a tree properties.
void changeTree(Tree* t);

// force redraw of a tree.
void drawTree();

// initTree initializes the tree view widget.
void initTree(GtkWidget* parent);

// keyTree answers a key event on a tree window.
gboolean keyTree(GdkEventKey* e);

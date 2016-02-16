// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

// Earth boudaries
#define MinLon -180
#define MaxLon 180
#define MinLat -90
#define MaxLat 90

// A GeoRef is a georeferenced record.
typedef struct {
        gchar* Catalog;
        double Lon;
        double Lat;
        // Graphic part
        int X;
        int Y;
}GeoRef;

// GeoIsValid returns true is lon and lat are valid longitude and latitude
// points.
gboolean GeoIsValid(double lon, double lat);

// A Taxon is a named terminal taxon with a list of georeferenced records.
typedef struct {
        gchar*     Name;
        GPtrArray* Recs;
}Taxon;

// BioGeoRead reads f in CSV format, and returns an array of pointers to
// taxon.
GPtrArray* BioGeoRead(FILE* f, gchar** error);

// GetTaxon returns a taxon from an array of pointers to taxon.
Taxon* GetTaxon(GPtrArray* d, gchar* name);

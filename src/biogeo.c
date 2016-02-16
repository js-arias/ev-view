// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "biogeo.h"
#include "csv.h"

gboolean GeoIsValid(double lon, double lat) {
        if ((lon <= MinLon) || (lon > MaxLon)) {
                return FALSE;
        }
        if ((lat < MinLat) || (lat > MaxLat)) {
                return FALSE;
        }
        return TRUE;
}

Taxon* NewTaxon(gchar* name) {
        Taxon* t;

        t = (Taxon*)g_malloc0(sizeof(Taxon));
        t->Name = g_strdup(name);
        t->Recs = g_ptr_array_new();
        return t;
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

Taxon* GetTaxon(GPtrArray* d, gchar* name) {
        int    i;
        Taxon* t;

        for (i = 0; i < d->len; i++) {
                t = (Taxon*)g_ptr_array_index(d, i);
                if (strcmp(t->Name, name) == 0) {
                        return t;
                }
        }
        return NULL;
}

static void delTaxonArray(GPtrArray* d) {
        int     i, j;
        GeoRef* g;
        Taxon*  t;

        for (i = 0; i < d->len; i++) {
                t = (Taxon*) g_ptr_array_index(d, i);
                for (j = 0; j < t->Recs->len; j++) {
                        g = (GeoRef*)g_ptr_array_index(t->Recs, j);
                        g_free(g->Catalog);
                        g_free(g);
                }
                g_ptr_array_free(t->Recs, TRUE);
                g_free(t->Name);
        }
        g_ptr_array_free(d, TRUE);
}

double readReal(gchar* s, gchar** error) {
        int pos = 0;
        int acum = 0;
        int kind = 1;
        double decs = 0;
        double dig = 1;
        gchar c;

        // get the sign
        if (s[0] == '-') {
                kind = -1;
                pos++;
        }

        // reads the integer part
        for (;;) {
                c = s[pos];
                pos++;
                if (c == '\0') {
                        return (double)(kind * acum);
                }
                if (isspace(c)) {
                        return (double)(kind * acum);
                }
                if ((c == '.') || (c == ',')) {
                        break;
                }
                if (!isdigit(c)) {
                        *error = g_strdup("invalid number value");
                        return 0;
                }
                acum *= 10;
                acum += c - '0';
        }

        // reads the real part
        for (;;) {
                c = s[pos];
                pos++;
                if (c == '\0') {
                        break;
                }
                if (isspace(c)) {
                        break;
                }
                if (!isdigit(c)) {
                        *error = g_strdup("invalid number value");
                        return 0;
                }
                decs *= 10;
                decs += (double)(c - '0');
                dig *= 10;
        }
        if (kind == 1) {
                return ((double)acum) + (decs / dig);
        }
        return -(((double)acum) + (decs / dig));
}

GPtrArray* BioGeoRead(FILE* f, gchar** error) {
        GPtrArray* d;
        GPtrArray* h;
        GPtrArray* row;
        GeoRef*    g;
        Taxon*     t;
        gchar*     err = NULL;
        gchar*     s;
        CSVReader* r;
        int i;
        int name = -1;
        int lon  = -1;
        int lat  = -1;
        int cat  = -1;
        double lgv, ltv;

        r = NewCSVReader(f);

        // reads the file header
        h = CSVRead(r);
        if ((err = CSVReaderError(r)) != NULL) {
                *error = g_strdup_printf("header (data): %s", err);
                DelCSVReader(r);
                return NULL;
        }
        for (i = 0; i < h->len; i++) {
                s = g_ptr_array_index(h, i);
                g_strstrip(s);
                s = g_ascii_strdown(s, -1);
                if ((strcmp(s, "name") == 0) || (strcmp(s, "scientificname") == 0) || (strcmp(s, "scientific name") == 0)) {
                        name = i;
                }
                if ((strcmp(s, "lon") == 0) || (strcmp(s, "longitude") == 0) || (strcmp(s, "long") == 0)) {
                        lon = i;
                }
                if ((strcmp(s, "lat") == 0) || (strcmp(s, "latitude") == 0)) {
                        lat = i;
                }
                if ((strcmp(s, "catalog") == 0) || (strcmp(s, "recordid") == 0) || (strcmp(s, "record id") == 0)) {
                        cat = i;
                }
                g_free(s);
        }
        delArray(h);
        if ((name < 0) || (lon < 0) || (lat < 0)) {
                *error = g_strdup("header (data): incomplete header");
                DelCSVReader(r);
                return NULL;
        }

        // read the data
        d = g_ptr_array_new();
        for (;;) {
                row = CSVRead(r);
                if ((err = CSVReaderError(r)) != NULL) {
                        if (strcmp(err, "EOF") == 0) {
                                break;
                        }
                        delTaxonArray(d);
                        *error = g_strdup_printf("(data): line: %d, %s", CSVReaderLine(r), err);
                        DelCSVReader(r);
                        return NULL;
                }
                if ((row->len <= name) || (row->len <= lon) || (row->len <= lat)) {
                        delArray(row);
                        continue;
                }
                s = (gchar*)g_ptr_array_index(row, name);
                t = GetTaxon(d, s);
                if (t == NULL) {
                        t = NewTaxon(s);
                        g_ptr_array_add(d, t);
                }
                s = (gchar*)g_ptr_array_index(row, lon);
                lgv = readReal(s, &err);
                if (err != NULL) {
                        delTaxonArray(d);
                        delArray(row);
                        *error = g_strdup_printf("(data): line: %d, invalid longitude: %s", CSVReaderLine(r), err);
                        DelCSVReader(r);
                        return NULL;
                }
                s = (gchar*)g_ptr_array_index(row, lat);
                ltv = readReal(s, &err);
                if (err != NULL) {
                        delTaxonArray(d);
                        delArray(row);
                        *error = g_strdup_printf("(data): line: %d, invalid latitude: %s", CSVReaderLine(r), err);
                        DelCSVReader(r);
                        return NULL;
                }
                if (!GeoIsValid(lgv, ltv)) {
                        delTaxonArray(d);
                        delArray(row);
                        *error = g_strdup_printf("(data): line: %d, invalid georeference", CSVReaderLine(r));
                        DelCSVReader(r);
                        return NULL;
                }
                g = g_malloc0(sizeof(GeoRef));
                g->Lon = lgv;
                g->Lat = ltv;
                if ((cat >= 0) && (cat < row->len)) {
                        g->Catalog = g_strdup(g_ptr_array_index(row, cat));
                }
                g_ptr_array_add(t->Recs, g);
                delArray(row);
        }
        DelCSVReader(r);
        return d;
}

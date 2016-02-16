// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

#include <ctype.h>
#include <glib.h>
#include <stdio.h>
#include <string.h>

#include "csv.h"

struct csvReader {
        int      fields; // number of expected fields per record
        int      line;
        int      column;
        FILE*    f;
        GString* b;
        GString* err;    // last error
};

CSVReader* NewCSVReader(FILE* f) {
      CSVReader* r;

      r = (CSVReader*)g_malloc0(sizeof(CSVReader));
      r->f = f;
      r->b = g_string_new(NULL);
      r->err = g_string_new(NULL);
      return r;
}

void DelCSVReader(CSVReader* r) {
        g_string_free(r->b, TRUE);
        g_string_free(r->err, TRUE);
        g_free(r);
}

gchar* CSVReaderError(CSVReader* r) {
        if (r->err->len == 0) {
                return NULL;
        }
        return r->err->str;
}

int CSVReaderLine(CSVReader* r) {
        return r->line;
}

static gchar readRune(CSVReader* r) {
        int r1;

        r1 = fgetc(r->f);
        if (r1 == EOF) {
                g_string_assign(r->err, "EOF");
                return 0;
        }
        // handles \r\n here.
        if (r1 == '\r') {
                r1 = fgetc(r->f);
                if (r1 == EOF) {
                        g_string_assign(r->err, "EOF");
                        return 0;
                }
                if (r1 != '\n') {
                        ungetc(r1, r->f);
                        r1 = '\r';
                }
        }
        r->column++;
        return r1;
}

static void skip(CSVReader* r, gchar delim) {
        gchar r1;

        for (;;) {
                r1 = readRune(r);
                if (r->err->len != 0) {
                        break;
                }
                if (r1 == delim) {
                        break;
                }
        }
}

static gboolean quoted(CSVReader* r, gchar* delim) {
        gchar r1;

        for (;;) {
                r1 = readRune(r);
                if (r->err->len != 0) {
                        break;
                }
                switch (r1) {
                case '"':
                        r1 = readRune(r);
                        if (r->err->len != 0) {
                                *delim = 0;
                                if (strcmp(r->err->str, "EOF") == 0) {
                                        return TRUE;
                                }
                                return FALSE;
                        }
                        if (r1 == ',') {
                                *delim = r1;
                                return TRUE;
                        }
                        if (r1 != '"') {
                                // accept the bare quote
                                g_string_append_c(r->b, r1);
                        }
                        break;
                case '\n':
                        r->line++;
                        r->column = -1;
                        break;
                }
                g_string_append_c(r->b, r1);
        }
        *delim = 0;
        if (strcmp(r->err->str, "EOF") == 0) {
                return TRUE;
        }
        return FALSE;
}

static gboolean parseField(CSVReader* r, gchar* delim) {
        gchar r1;

        g_string_erase(r->b, 0, -1);

        r1 = readRune(r);
        for (;;) {
                if (r->err->len != 0) {
                        break;
                }
                if (r1 == '\n') {
                        break;
                }
                if (!isspace(r1)) {
                        break;
                }
                r1 = readRune(r);
        }
        if (r->err->len != 0) {
                *delim = 0;
                if ((r->column != 0) && (strcmp(r->err->str, "EOF") == 0)) {
                        return TRUE;
                }
                return FALSE;
        }

        switch (r1) {
        case ',':
                break;
        case '\n':
                *delim = r1;
                if (r->column == 0) {
                        return FALSE;
                }
                return TRUE;
        case '"':
                // quoted field
                return quoted(r, delim);
        default:
                // unquoted field
                for (;;) {
                        g_string_append_c(r->b, r1);
                        r1 = readRune(r);
                        if ((r->err->len != 0) || (r1 == ',')) {
                                break;
                        }
                        if (r1 == '\n') {
                                *delim = r1;
                                return TRUE;
                        }
                }
        }
        if (r->err->len != 0) {
                *delim = 0;
                if ((r->column != 0) && (strcmp(r->err->str, "EOF") == 0)) {
                        return TRUE;
                }
                return FALSE;
        }
        *delim = r1;
        return TRUE;
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

static GPtrArray* parseRecord(CSVReader* r) {
        GPtrArray* fields;
        gchar  r1;

        r->line++;
        r->column = -1;

        // Peek the first rune for a comment. If is an error we are done.
        // If is a comment the skip to the end of the line.
        r1 = readRune(r);
        if (r->err->len != 0) {
                return NULL;
        }
        if (r1 == '#') {
                skip(r, '\n');
                return NULL;
        }
        ungetc(r1, r->f);
        r->column--;

        // At this point we have at least one field.
        fields = g_ptr_array_new();
        for (;;) {
                if (parseField(r, &r1)) {
                        g_ptr_array_add(fields, g_strdup(r->b->str));
                }
                if ((r1 == '\n') || (strcmp(r->err->str, "EOF") == 0)) {
                        if (fields->len == 0) {
                                break;
                        }
                        return fields;
                }
                if (r->err->len != 0) {
                        break;
                }
        }
        delArray(fields);
        return NULL;
}

GPtrArray* CSVRead(CSVReader* r) {
        GPtrArray* record = NULL;

        for (;;) {
                record = parseRecord(r);
                if (record != NULL) {
                        break;
                }
                if (r->err->len != 0) {
                        return NULL;
                }
        }
        if (r->fields > 0) {
                if (record->len != r->fields) {
                        delArray(record);
                        g_string_assign(r->err, "wrong number of fields in line");
                        return NULL;
                }
        } else if (r->fields == 0) {
                r->fields = record->len;
        }
        return record;
}

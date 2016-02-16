// Copyright (c) 2015, J. Salvador Arias <jsalarias@csnat.unt.edu.ar>
// All rights reserved.
// Distributed under BSD2 license that can be found in the LICENSE file.

// A CSVReader reads records from a CSV-encoded file.
typedef struct csvReader CSVReader;

// NewCSVReader returns a new CSVReader that reads from f.
CSVReader* NewCSVReader(FILE* f);

// DelCSVReader removes r from memory.
void DelCSVReader(CSVReader* r);

// CSVRead reads one record from r. The returned array is an array of gchar*
// in which each string represents one field. The array as well as the
// strings are owned by the caller, who is responsible to delete them.
GPtrArray* CSVRead(CSVReader* r);

// CSVReaderError returns the last error found while reading r. The
// returned string is not owned by the caller.
gchar* CSVReaderError(CSVReader* r);

// CSVReaderLine returns the current line of the file.
int CSVReaderLine(CSVReader* r);

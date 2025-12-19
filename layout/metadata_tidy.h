/*
 *
 * metadata_tidy.h
 *
 *
 */

#ifndef _METADATA_TIDY_H
#define _METADATA_TIDY_H

#include <stdint.h>   // Required for ../metadata.h
#include "../metadata.h"


// Tidies field cstrvar of type cstrtype - returns dup / copy
char * metadata_tidy_dupfield(const char *cstrvar, const char *cstrtype) ;

// Frees metadata_tidy allocated field
void metadata_tidy_freefield(char *cstrvar) ;

// Tidies / updates metadata and returns a copy
metadata_t * metadata_tidy_dup(const metadata_t *m) ;

// Frees metadata structure
void metadata_tidy_free(metadata_t *m) ;

// Debug function - dumps metadata structure to stdout
void metadata_dump(char *msg, metadata_t *m) ;

#endif // _METADATA_TIDY_H

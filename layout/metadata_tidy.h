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

// Tidies / updates metadata
void metadata_tidy(metadata_t m) ;

// Tidies field cstrvar of type cstrtype - returns dup / copy
char * metadata_tidy_dup(const char *cstrvar, const char *cstrtype) ;

// Frees metadata_tidy allocated field
void metadata_tidy_free(char *cstrvar) ;

#endif // _METADATA_TIDY_H

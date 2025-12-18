/*
 *
 * metadata_tidy.c
 *
 *  Tidies all string fields in the metadata_t structure
 *  This should be used immediately before any SQL 'INSERT INTO DETAILS' call
 *
 *  20/12/2025  S Clarke
 *
 */

#include "metadata_tidy.h"
#include "layout.h"
#include "mstring/mstring.h"
#include <stdlib.h>

// Globals Used
// lo = layout object
//

// Tidies field cstrvar of type cstrtype
char * metadata_tidy_dup(const char *cstrvar, const char *cstrtype) {

    char *n = NULL ;
    if (!cstrvar || !cstrtype) return cstrvar;
    string result = string_new() ;
    layout_clean(lo, cstrtype, cstrvar, result) ;
    n = strdup(string_cstr(result)) ;
    string_free(result) ;
    return n ;
}

void metadata_tidy_free(char *cstrvar) {
    if (cstrvar) free(cstrvar) ;
}

// Tidies all string metadata fields
void metadata_tidy(metadata_t m) {

    char *n ;

    if (m.title) {
        n = metadata_tidy_dup(m.title, "title") ;
        free(m.title) ;
        m.title = n ;
    }

    if (m.artist) {
        n = metadata_tidy_dup(m.artist, "artist") ;
        free(m.artist) ;
        m.artist = n ;
    }

    if (m.creator) {
        n = metadata_tidy_dup(m.creator, "creator") ;
        free(m.creator) ;
        m.creator = n ;
    }

    if (m.album) {
        n = metadata_tidy_dup(m.album, "album") ;
        free(m.album) ;
        m.album = n ;
    }

    if (m.genre) {
        n = metadata_tidy_dup(m.genre, "genre") ;
        free(m.genre) ;
        m.genre = n ;
    }

    if (m.comment) {
        n = metadata_tidy_dup(m.comment, "comment") ;
        free(m.comment) ;
        m.comment = n ;
    }

    if (m.resolution) {
        n = metadata_tidy_dup(m.resolution, "resolution") ;
        free(m.resolution) ;
        m.resolution = n ;
    }

    if (m.duration) {
        n = metadata_tidy_dup(m.duration, "duration") ;
        free(m.duration) ;
        m.duration = n ;
    }

    if (m.date) {
        n = metadata_tidy_dup(m.date, "date") ;
        free(m.date) ;
        m.date = n ;
    }

    if (m.mime) {
        n = metadata_tidy_dup(m.mime, "mime") ;
        free(m.mime) ;
        m.mime = n ;
    }

    if (m.dlna_pn) {
        n = metadata_tidy_dup(m.dlna_pn, "dlna_pn") ;
        free(m.dlna_pn) ;
        m.dlna_pn = n ;
    }

}


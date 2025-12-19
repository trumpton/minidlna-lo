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
#include <string.h>
#include "../log.h"

// Globals Used
// lo = layout object
//

// Tidies field cstrvar of type cstrtype
char * metadata_tidy_dupfield(const char *cstrvar, const char *cstrtype) {
    char *n = NULL ;
    if (!cstrvar || !cstrtype) return NULL;
    string result = string_new() ;
    layout_clean(lo, cstrtype, cstrvar, result) ;
    n = strdup(string_cstr(result)) ;
    string_free(result) ;
    return n ;
}

void metadata_tidy_freefield(char *cstrvar) {
    if (cstrvar) {
        cstrvar[0]='\0' ;
        free(cstrvar) ;
    }
}

// Tidies all string metadata fields and returns a copy
metadata_t * metadata_tidy_dup(const metadata_t *m) {

    metadata_t *m2 ;

    m2 = malloc(sizeof(metadata_t)) ;
    if (!m2) return NULL ;

    memset(m2, '\0', sizeof(metadata_t)) ;

    m2->disc = m->disc ;
    m2->track = m->track ;
    m2->channels = m->channels ;
    m2->bitrate = m->bitrate ;
    m2->frequency = m->frequency ;
    m2->rotation = m->rotation ;
    m2->thumb_size = m->thumb_size ;
    m2->title = metadata_tidy_dupfield(m->title, "title") ;
    m2->artist = metadata_tidy_dupfield(m->artist, "artist") ;
    m2->creator = metadata_tidy_dupfield(m->creator, "creator") ;
    m2->album = metadata_tidy_dupfield(m->album, "album") ;
    m2->genre = metadata_tidy_dupfield(m->genre, "genre") ;
    m2->comment = metadata_tidy_dupfield(m->comment, "comment") ;
    if (m->resolution) m2->resolution = strdup(m->resolution) ;
    if (m->duration) m2->duration = strdup(m->duration) ;
    if (m->date) m2->date = strdup(m->date) ;
    if (m->mime) m2->mime = strdup(m->mime) ;
    if (m->dlna_pn) m2->dlna_pn = strdup(m->dlna_pn) ;
    if (m2->thumb_size>0) {
        m2->thumb_data = malloc(m2->thumb_size) ;
        if (!m2->thumb_data) return NULL ;
        memcpy(m2->thumb_data, m->thumb_data, m2->thumb_size) ;
    }
    return m2 ;
}

void metadata_tidy_free(metadata_t *m) {
    if (!m) return ;
    if (m->title) free(m->title) ;
    if (m->artist) free(m->artist) ;
    if (m->creator) free(m->creator) ;
    if (m->album) free(m->album) ;
    if (m->genre) free(m->genre) ;
    if (m->comment) free(m->comment) ;
    memset(m, '\0', sizeof(metadata_t)) ;
    free(m) ;
}


// DEBUG FUNCTION
//
void metadata_dump(char *msg, metadata_t *m) {

	DPRINTF(E_DEBUG, L_METADATA, "METADATA DUMP: %s\n", msg) ;
	DPRINTF(E_DEBUG, L_METADATA, "  metadata_t @%p\n", (void *)m) ;
	DPRINTF(E_DEBUG, L_METADATA, "  title   @%p = %s\n", (void *)m->title, m->title?m->title:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  creator @%p = %s\n", (void *)m->creator, m->creator?m->creator:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  artist  @%p = %s\n", (void *)m->artist, m->artist?m->artist:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  album   @%p = %s\n", (void *)m->album, m->album?m->album:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  genre   @%p = %s\n", (void *)m->genre, m->genre?m->genre:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  comment @%p = %s\n", (void *)m->comment, m->comment?m->comment:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  dlna_pn @%p = %s\n", (void *)m->dlna_pn, m->dlna_pn?m->dlna_pn:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "  mime    @%p = %s\n", (void *)m->mime, m->mime?m->mime:"null") ;
	DPRINTF(E_DEBUG, L_METADATA, "METADATA DUMP COMPLETE\n") ;
}


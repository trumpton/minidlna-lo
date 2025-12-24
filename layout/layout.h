/*
 *
 * layout.h
 *
 *
 */

//
// Layout allows the media items to be presented in a UPNP client in a number
// of different paths, for example, /Media/Artist/Enya/All Tracks/01. Watermark
// and /Media/Genre/Pop/Album/Watermark/01. Watermark.
//
// These paths are called chains, and are made up of several fields - i.e. 'Media',
// 'Artist', 'Genre' etc. are fields.
//
// Layouts are managed using a configuration file, which has several sections
//
// [config]
// unknown="Unknown"
// searchdepth=2
//
// [replace]
// genre=from=to
// comment=from=to
// artist=from=to
// title=from=to
//
// [rule]
// mediatype=audio|video|picture
// and=tag=value,tag=value
// or=tag=value,tag=value
// chainset=chainsetname
//
// [rule]
// ...
//
// [chainsetname]
// chain=/Music/Album/$ABCALBUM/$ALBUM/$TRACKNUM. $TRACK
// chain=/Music/Artist/$ABCARTIST/$ARTIST/$TRACK ($ALBUM)
// chain=/Music/Genre/$GENRE/$TRACK ($ARTIST, $ALBUM)
// chain=/Music/Date/$DECADE/$YEAR/$TRACK ($ARTIST, $ALBUM)
// chain=/Music/$PATH/$FILENAME
// chain=/Music/Search/Artist/$SEARCHARTIST/$ARTIST/$TRACK ($ALBUM)
// chain=/Music/Search/Album/$SEARCHALBUM/$ALBUM ($ARTIST)/$TRACKNUM $TRACK
// chain=/Music/Search/Track/$SEARCHTRACK ($ARTIST)/$TRACK ($ALBUM)
//
// ----------------------------------------------------------------------
//
// There are a few functions to support the management of the layout.
//
// layout_newfrom()    - loads the configuration
// layout_free()       - releases memory associated with the configuration
// layout_search()     - searches for a chainset within the configuration
// layout_numchains()  - returns number of chains found in the search
// layout_findfield()  - returns extracts information about a field within a chain.
//
// ----------------------------------------------------------------------
//

#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "mstring/mstring.h"
#include "metadata_tidy.h"
#include "../metadata.h"

// Use the ASCII Record Separator Internally to delimit the 
// Paths used when manipulating the chains.  In the unlikely
// event that the record separator is found, a space will be
// used

#define SQL_PATH_SEPARATOR "\30"
#define SQL_PATH_SEPARATOR_CHAR '\30'
#define SQL_PATH_SEPARATOR_SUB " "


struct _layout_section {
  string title ;
  stringlist tag ;
  stringlist arg ;
} ;

struct _layout_data {

  // Layout Data
  int lastsection ;
  struct _layout_section *section ;

  // Search Variables
  stringlist chainitems ;
  stringlist chainclasses ;
  stringlist searchitemparts ;
  stringlist searchclassparts ;
  int searchhandle ;
  int searchitemref ;

  // Configuration Oprions
  string configunknown ;
  int configsearchdepth ;
  stringlist configstripfolder ;
} ;

typedef struct _layout_data * layout ;

//
// layout_newfrom
//
// Loads the layout configuration from filename, and
// allocates and initialises a layout structure.
//
// RETURNS
//
// layout item/handle to use in future calls
//
layout layout_newfrom(const char *filename) ;

//
// layout_free
//
// Frees the memory allocated by the layout_newfrom
// function.
//
// RETURNS
//
// nothing
//
void layout_free(layout lo) ;

//
// layout_clean
//
// Takes the type and field string, tidies up as required,
// and updates the var string with the result.
// This function uses the replace section of the configuration
// file to make appropriate substitutions as necessary.
//
void layout_clean(layout lo, const char *cstrtype, const char *cstrvar, string var) ;

//
// layout_search
//
// Initialises the layout search with the given parameters.  Empty
// strings or Null strings are replaced with 'Unknown' if needed.
//
// RETURNS
//
// an integer search-handle, which is used for the num and find functions
//
int layout_search(layout lo, const char *mediatype, const char *path, const metadata_t *m) ;

//
// layout_numitems
//
// RETURNS
//
// The number of items (chains) found in the search, or
// -1 if the search handle is not valid.
//
int layout_numchains(layout lo, int searchhandle) ;

//
// layout_findfield
//
// This function extracts information from the search, to obtain an item
// its path, and its media type.
//
// RETURNS
//
// true on success
//
// chainnum: the number of the chain
// field: the section within the chain /field0/field1/field2 etc..
// fieldname is updated to point to the requested field
// fieldclass is updated to point to the media type of the requested field
int layout_findfield(layout lo, int searchhandle, int chainnum, int fieldnum,
		    char **fieldnameptr, char **fieldclassptr) ;


//
// layout_escape
//
// Allocates new buffer and copies src into it, escaping
// characters suitable for a sql query
char *layout_escape(const char *src) ;

//
// Logging
//
void layout_dumptolog(layout lo, int level) ;

#ifndef LAYOUT_C
// Global layout object
extern layout lo ;
#endif


// Debug Functions

// Access the full folderpath for the item and class - useful for debug purposes
// the resulting string is the expanded chain with the SQL_PATH_SEPARATOR
// in the string between fields / sections of the path.
char * layout_chainitems(layout lo, int searchhandle, int chainnum) ;
char * layout_chainclasses(layout lo, int searchhandle, int chainnum) ;


#endif // _LAYOUT_H

/*
 *
 * layout.h
 *
 *
 */

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
// layout_numfolders() - returns number of folders found in the search
// layout_findfolder() - returns folder information
// layout_numitems()   - returns number of items found in the search
// layout_finditem()   - returns item information
//
// ----------------------------------------------------------------------
//

#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "mstring/mstring.h"
#include "metadata_tidy.h"

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
  stringlist folderpathitem ;
  stringlist folderpathclass ;
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
layout layout_newfrom(char *filename) ;

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
int layout_search(layout lo, char *mediatype, char *path,
		  char *creator, char *albumartist,
		  char *artist,  char *album,
		  char *disc, char *tracknum,
		  char *title,  char *comment,
		  char *genre,  char *date,
		  char *resolution, char *rotation,
		  char *duration,  char *bitrate,
		  char *location) ;

//
// layout_numitems
//
// RETURNS
//
// The number of items (media file references) found in the search, or 
// -1 if the search handle is not valid.
//
int layout_numitems(layout lo, int searchhandle) ;

//
// layout_finditem
//
// This function extracts information from the search, to obtain an item
// its path, and its media type.
//
// RETURNS
//
// true on success
// itemfieldname is updated to point to the requested field
// itemtype is updated to point to the media type of the item
//
int layout_finditemfield(layout lo, int searchhandle, int itemnum, int field,
		    char **itemfieldname, char **itemfieldclass) ;

// Access the full folderpath for the item and class
char * layout_folderpathitem(layout lo, int searchhandle, int itemnum) ;
char * layout_folderpathclass(layout lo, int searchhandle, int itemnum) ;

//
// layout_numfolders
//
// RETURNS
//
// The number of folders (media file references) found in the search, or
// -1 if the search handle is not valid.
//
int layout_numfolders(layout lo, int searchhandle) ;

//
// layout_findfolder
//
// This function extracts information from the search, to obtain a folder
// its path, and its media type.
//
// RETURNS
//
// true on success
// folderfieldname is updated to point to the requested field
// foldertype is updated to point to the media type of the item
//
int layout_findfolderfield(layout lo, int searchhandle, int foldernum, int field,
		      char **folderfieldname, char **foldertype) ;

char * layout_foldername(layout lo, int searchhandle, int foldernum) ;
char * layout_folderclass(layout lo, int searchhandle, int foldernum) ;

#ifndef LAYOUT_C
// Global layout object
extern layout lo ;
#endif

#endif // _LAYOUT_H

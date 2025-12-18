/*
 *
 * layout.c
 *
 */

#define LAYOUT_C
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include "../log.h"
#include "layout.h"
#include "mstring/mstring.h"

layout *lo ;

//#define DISABLE_LAYOUT_SEARCH

// Split a string: "a=b,c=d,e=f" to a list: a,b,c,d,e,f
int _layout_pairlist(string var, stringlist list)
{
  stringlist sections = stringlist_new() ;
  stringlist_clear(list) ;
  int sz=string_split(var, ',', sections) ;
  for (int i=0; i<sz; i++) {
    stringlist pair = stringlist_new() ;
    if (string_split(stringlist_at(sections, i), '=', pair)==2) {
      stringlist_push_back(list, stringlist_at(pair,0)) ;
      stringlist_push_back(list, stringlist_at(pair,1)) ;
    }
    stringlist_free(pair) ;
  }
  stringlist_free(sections) ;
  return stringlist_len(list) ;
}

// Return the nth section called sectionname ( <0 on error)
int _layout_findsection(layout lo, string sectionname, int nth)
{
  int num=-2 ;
  if (lo && sectionname) {
    int count=0 ;
    int found=0 ;
    num=0 ; 
    while (num<=lo->lastsection && !found)  {
      if (string_strcmp(sectionname, lo->section[num].title)==0) {
	if ( (nth>=0 && count==nth) || nth<0) found=1 ;
	count++ ;
      }
      if (!found) num++ ;
    }
    if (!found) num=-1 ;
  }
  return num ;
}

// Return the size of the requestend (nth) section
int _layout_sectionsize(layout lo, string sectionname, int nth)
{
  int sect = _layout_findsection(lo, sectionname, nth) ;
  if (sect<0) return -1 ;
  else return stringlist_len(lo->section[sect].tag) ;
}

// Extract an entry from the nth section
int _layout_entry(layout lo, string sectionname, int nthsection, int num,
		  string resulttag, string resultarg)
{
  int sect = _layout_findsection(lo, sectionname, nthsection) ;
  if (sect<0) {
    string_clear(resulttag) ;
    string_clear(resultarg) ;
    return 0  ;
  } else {
    string_strcpy(resulttag, stringlist_at(lo->section[sect].tag, num));
    string_strcpy(resultarg, stringlist_at(lo->section[sect].arg, num));
    return 1 ;
  }
}

// Extract the nth item called tagname from the given sectionnum
int _layout_getitem(layout lo, int sectionnum, char *tagname,
		    int nthtag, string resultarg)
{
  int found=0 ;
  string_clear(resultarg) ;
  if (sectionnum>=0 && sectionnum<=lo->lastsection) {
    int num=0 ;
    int count=0 ;
    int sz=stringlist_len(lo->section[sectionnum].tag) ;
    while (num<sz && !found) {
      string tag = stringlist_at(lo->section[sectionnum].tag, num) ;
      string arg = stringlist_at(lo->section[sectionnum].arg, num) ;
      if (string_cstrcmp(tag, tagname)==0) {
	if (nthtag<0 || count>=nthtag) {
	  found=1 ;
	  string_strcpy(resultarg, arg) ;
	} else {
	  count++ ;
	}
      }
      num++ ;
    }
  }
  return found ;
}

// Extract all words in src to words list
int _layout_findwords(string src, stringlist words)
{
  string word = string_new() ;
  int len = string_strlen(src) ;
  stringlist_clear(words) ;

  int start = 0, end=0 ;
  while (start<len) {
    end=start ;
    while (end<len && character_isletter(string_at(src, end))) end++ ;
    if (end>start) {
      string_substring(src, word, start, end-1) ;
      stringlist_push_back(words, word) ;
    }
    start=end ;
    while (start<len && !character_isletter(string_at(src,start))) start++ ;
  }
  string_free(word) ;
  return stringlist_len(words) ;
}

// Copies the cstrvar into var
// Processes [replace] if required
// Capitalises the resulting string
void layout_clean(layout lo, const char *cstrtype, const char *cstrvar, string var)
{
  string_cstrcpy(var, cstrvar) ;

  // Replace the reserved separator
  string_cstrreplace(var, SQL_PATH_SEPARATOR, SQL_PATH_SEPARATOR_SUB) ;

  // TODO: This is dirty.  What we should do is replace the characters
  // when we extract them from the SQL database, but only if we need to
  // do so (i.e. if they are being used for html) and then we should do
  // the correct substitution so that the end user sees the original 
  // character

  string_cstrreplace(var, "&", "+") ; // Replace characters used in html
  string_cstrreplace(var, "<", "{") ;
  string_cstrreplace(var, ">", "}") ;

  if (lo) {

    string_capitalise(var, TOLOWER) ;
    string replace = string_newfrom("replace") ;
    string type = string_newfrom(cstrtype) ;
    string tag = string_new() ;
    string arg = string_new() ;
    int sz = _layout_sectionsize(lo, replace, 0) ;
    for (int i=0; i<sz; i++) {
      if (_layout_entry(lo, replace, 0, i, tag, arg)) {
	stringlist pair = stringlist_new() ;
	string_split(arg, '=', pair) ;
	if (stringlist_len(pair)==2 &&
	    string_strcmp(tag, type)==0 &&
	    string_strcmp(stringlist_at(pair,0), var)==0) {
	  string_strcpy(var, stringlist_at(pair,1)) ;
	}
	stringlist_free(pair) ;
      }
    }
    string_free(replace) ;
    string_free(arg) ;
    string_free(tag) ;
    string_free(type) ;
    string_capitalise(var, CAPITALISE) ;
  }
}

// Search the rules and search for the first matching chainset
// Return the chainset section number, or -1 if not found
int _layout_find_chainset(layout lo, string mediatype, string artist,
			  string album, string genre, string comment)
{
  int nth=0 ;
  int found=0 ;
  int section ;
  string sectionname = string_newfrom("rule") ;
  string chainset = string_new() ;
  string result = string_new() ;

  do {

    section = _layout_findsection(lo, sectionname, nth) ;

    // Find a Matching Rule and obtain the chainset name

    if (section>=0) {

      int ormatch=1, andmatch=1 ;
      int mtmatch=0 ;
      
      if (_layout_getitem(lo, section, "mediatype", 0, result) &&
	  string_strcmp(mediatype, result)==0) mtmatch=1 ;

      if (_layout_getitem(lo, section, "chainset", 0, result)) {
	string_strcpy(chainset, result) ;
      }

      for (int isor=0; isor<=1; isor++) {

	int process=0 ;
	if (isor && _layout_getitem(lo, section, "or", 0, result)) {
	  ormatch=0 ; process=1 ;
	}
	if (!isor && _layout_getitem(lo, section, "and", 0, result)) {
	  process=1 ;
	}

	if (process) {

	  stringlist pair = stringlist_new() ;
	  string tst = string_new() ;
	  
	  if (_layout_pairlist(result, pair)) {
	    for (int i=0; i<stringlist_len(pair); i+=2) {
	      string what=stringlist_at(pair, i) ;
	      string is=stringlist_at(pair, i+1) ;

	      if (string_cstrcmp(what, "artist")==0) {
		string_strcpy(tst, artist) ;
	      } else if (string_cstrcmp(what, "album")==0) {
		string_strcpy(tst, album) ;
	      } else if (string_cstrcmp(what, "genre")==0) {
		string_strcpy(tst, genre) ;
	      } else if (string_cstrcmp(what, "comment")==0) {
		string_strcpy(tst, comment) ;
	      }
	      
	      string_capitalise(tst, TOLOWER) ;     
	      if (isor && string_strcmp(is, tst)==0) { ormatch=1 ; }
	      if (!isor && string_strcmp(is, tst)!=0) { andmatch=0 ; }

	    }

	    string_free(tst) ;
	    stringlist_free(pair) ;
	  }

	}

      }

      found = (ormatch && andmatch && mtmatch) ;      
      if (!found) nth++ ;
    }
    
  } while (!found && section>0) ;

  section = _layout_findsection(lo, chainset, 0) ;
  
  string_free(result) ;
  string_free(chainset) ;
  string_free(sectionname) ;

  return section ;
  
}


// Expand the given variable var, replacing it with source
void _layout_expandvariable(layout lo, string field, char *var, string source)
{
  string what = string_newfrom(var) ;
  string with = string_new() ;
  string str ;

  if (string_cstrcmp(source, "?")!=0 && string_strlen(source)>0) str=source ;
  else str = lo->configunknown ;
  
  if (string_cstrsearch(what, "$ABC")<0) {
    string_strcpy(with, str) ;
  } else {
    character c = character_tolower(character_deaccent(string_at(str, 0))) ;
    switch (c) {
    case 'a': case 'b': case 'c': string_cstrcpy(with, "[abc]") ; break ;
    case 'd': case 'e': case 'f': string_cstrcpy(with, "[def]") ; break ;
    case 'g': case 'h': case 'i': string_cstrcpy(with, "[ghi]") ; break ;
    case 'j': case 'k': case 'l': string_cstrcpy(with, "[jkl]") ; break ;
    case 'm': case 'n': case 'o': string_cstrcpy(with, "[mno]") ; break ;
    case 'p': case 'q': case 'r': string_cstrcpy(with, "[pqr]") ; break ;
    case 's': case 't': case 'u': string_cstrcpy(with, "[stu]") ; break ;
    case 'v': case 'w': case 'x': string_cstrcpy(with, "[vwx]") ; break ;
    case 'y': case 'z': string_cstrcpy(with, "[yz]") ; break ;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      string_cstrcpy(with, "[0-9]") ; break ;
    default:
      string_cstrcpy(with, "[...]") ; break ;
    }
  }
  string_replace(field, what, with) ;
  string_free(what) ;
  string_free(with) ;
}

// Parse the date from the expected format of yyyy-mm-dd
void _layout_parse_date(string date, string year, string decade)
{
  if (string_strlen(date)<4) {
    string_cstrcpy(year, "????") ;
    string_cstrcpy(decade, "??s") ;
  } else {
    string_substring(date, year, 0, 3) ;
    string_substring(date, decade, 2, 2) ;
    string_cstrcat(decade, "0s") ;
  }
}

// Add entry to the chainset
void _layout_addtochainset(string entry, stringlist chainset)
{
  // Append to chainset (if not already there)
  int sz = stringlist_len(chainset) ;
  int foundchain=0 ;
  for (int i=0; i<sz && !foundchain; i++) {
    if (string_strcmp(stringlist_at(chainset, i), entry)==0)
      foundchain=1 ;
  }
  if (!foundchain) {
    stringlist_push_back(chainset, entry) ;
  }
}


#ifndef DISABLE_LAYOUT_SEARCH

// Expand data to [d]/[da]/[dat]/[data] format
// First expansion replaces orgentry, and the others are appended to chainset
void _layout_expandsearch(layout lo, string orgentry, stringlist chainset,
			  char *var, string data)
{
  int foundmatch=0 ;
  string vars = string_newfrom(var) ;

  if (string_search(orgentry, vars)>=0) {

    string str ;
    string source = string_new() ;
    stringlist words = stringlist_new() ;

    if (string_cstrcmp(data, "?")!=0 && string_strlen(data)>0) str=data ;
    else str = lo->configunknown ;
    
    string_strcpy(source, orgentry) ;
    int sz=_layout_findwords(str, words) ;

    for (int i=0; i<sz; i++) {

      string word = stringlist_at(words, i) ;

      if (string_strlen(word)>=lo->configsearchdepth) {

	string abc = string_new() ;
	string newentry = string_new() ;
	string search = string_new() ;

	string_capitalise(word, TOLOWER) ;
	string_capitalise(word, DEACCENT) ;

	// for [a] [ab] [abc] [abcd]
	// Build the search sequence
	for (int j=0; j< lo->configsearchdepth; j++) {

	  string_substring(word, abc, 0, j) ;
	  if (j>0) string_cstrcat(search, SQL_PATH_SEPARATOR) ;
	  string_cstrcat(search, "[") ;
	  string_strcat(search, abc) ;
	  string_cstrcat(search, "]") ;
	}

	// Substitute to create a new entry
	string_strcpy(newentry, source) ;
	string_replace(newentry, vars, search) ;

	if (!foundmatch) {

	  // Replace original Entry (first word match)
	  string_strcpy(orgentry, newentry) ;
	  foundmatch=1 ;
	} else {
	  // Append to the chainset (other words)
	  _layout_addtochainset(newentry, chainset) ;

	}
      
      	string_free(abc) ;
	string_free(search) ;
	string_free(newentry) ;

      }

    }
    
    string_free(source) ;
    stringlist_free(words) ;
    
    // Remove original if no matching words were found at all
    if (!foundmatch) {
      string_clear(orgentry) ;
    }
    
  }
  
  string_free(vars) ;
}
#endif // DISABLE_LAYOUT_SEARCH

void
_layout_shortenstring(string result, string src)
{
  int len = string_strlen(src) ;
  int lastwasalnum=0 ;
  string chr = string_new() ;
  string_clear(result) ;
  for (int i=0; i<len; i++) {
    string_substring(src, chr, i, i) ;
    if (character_isletter(string_at(chr, 0)) ||
	character_isnumber(string_at(chr, 0))) {
      if (!lastwasalnum) {
	string_strcat(result, chr) ;
	lastwasalnum=1 ;
      }
    } else {
      lastwasalnum=0 ;
    }
  }
  string_capitalise(result, TOUPPER) ;
  string_free(chr) ;
}


// Tidy the path, flipping separators as needed
int _layout_tidypath(layout lo, string path, string filepath, string filename)
{
  // Unixify 'paths' and remove any UNC double slashes
  string_cstrreplace(path, "\\", "/") ;
  string_cstrreplace(path, "//", "/") ;

  // Strip all 'strippath' entries in config file
  int nrep = stringlist_len(lo->configstripfolder) ;
  for (int i=0; i<nrep; i++) {
    string folder = stringlist_at(lo->configstripfolder, i) ;
    if (string_search(path, folder)==0) {
      string newpath = string_new() ;
      string_strcpy(newpath, path) ;
      string_substring(path, newpath, string_strlen(folder), -1) ;
      string_strcpy(path, newpath) ;
      string_free(newpath) ;
    }
  }

  // Now extract the filepath and filename
  int lastslash = string_rfindch(path, '/') ;
  int firstslash = string_findch(path, '/') ;
  if (lastslash>0) {
    if (firstslash==0) firstslash=1 ;
    else firstslash=0 ;
    string_substring(path, filename, lastslash+1, -1) ;
    string_substring(path, filepath, firstslash, lastslash-1) ;
  }

  return 1 ;
}


int _layout_processrule(layout lo, string mediatype, string path,
			string creator, string albumartist,
			string artist, string album, string disc,
			string tracknum, string title, string comment,
			string genre, string date,
			string resolution, string rotation,
			string duration, string bitrate,
			string location)
{
  
  if (!lo) return 0 ;

  stringlist chainset = stringlist_new() ;
  string filename = string_new() ;
  string filepath = string_new() ;  
  string year = string_new() ;
  string decade = string_new() ;
  string paddedtracknum = string_new() ; 
  string salbum = string_new() ;
  string salbumartist = string_new() ;
  
  stringlist_clear(lo->folderpathitem) ;
  stringlist_clear(lo->folderpathclass) ;
  stringlist_clear(lo->searchitemparts) ;
  stringlist_clear(lo->searchclassparts) ;
  lo->searchhandle=-1 ;
  lo->searchitemref=-1 ;
  
  // Find the required chainset and extract the 'chain' entries to chainset
  lo->searchhandle = _layout_find_chainset(lo, mediatype, artist, album,
					genre, comment) ;  

  if (lo->searchhandle>=0) {

    // Extract chains to chainset
    string tmp = string_new() ;
    int n=0 ;
    while (_layout_getitem(lo, lo->searchhandle, "chain", n, tmp)) {
      string_cstrreplace(tmp, "/", SQL_PATH_SEPARATOR) ;
      if (string_at(tmp, 0)==SQL_PATH_SEPARATOR_CHAR) 
		_layout_addtochainset(tmp, chainset) ;
      n++ ;
    }
    string_free(tmp) ;
  }

#ifndef DISABLE_LAYOUT_SEARCH
  // Add Search chainsets
  int sz=stringlist_len(chainset) ;
  for (int i=0; i<sz; i++) {
    string entry = stringlist_at(chainset, i) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHALBUMARTIST", albumartist) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHALBUM", album) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHTRACKARTIST", artist) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHARTIST", albumartist) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHCOMPOSER", creator) ;
    _layout_expandsearch(lo, entry, chainset, "$SEARCHTITLE", title) ;
  }
#endif // DISABLE_LAYOUT_SEARCH

  // Parse the path (convert to unix and remove strippath config entries)
  _layout_tidypath(lo, path, filepath, filename) ;
  string_cstrreplace(filepath, "/", SQL_PATH_SEPARATOR) ;

  // tidy the date and calculate the year and decade
  _layout_parse_date(date, year, decade) ;

  // Generate a shortened album and albumartist
  _layout_shortenstring(salbum, album) ;
  _layout_shortenstring(salbumartist, albumartist) ;


  if (string_strlen(tracknum)<3) {
    string_cstrcpy(paddedtracknum, "0") ;
  }
  string_strcat(paddedtracknum, tracknum) ;
  
  // searchfolders and searchfoldertypes as required
  int csz = stringlist_len(chainset) ;
  for (int i=0; i<csz; i++) {

    stringlist parts = stringlist_new() ;
    string resultitem = string_new() ;
    string resultclass = string_new() ;
    string class = string_new() ;
    string random = string_new() ;

    // Set a 2-digit Random Number
    long int randint = rand()%1000 ;
    string_fromint(random, randint, 10, 3) ;

    string chain = stringlist_at(chainset, i) ;

    int szparts=string_split(chain, SQL_PATH_SEPARATOR_CHAR, parts) ;

    for (int j=1; j<szparts; j++) {

      string field = stringlist_at(parts, j) ;
      // string nextfield = stringlist_at(parts, j+1) ;

      // Work out the class for the current part
      if (j<(szparts-1)) {

	// This is a folder
	if (string_cstrsearch(field, "$ARTIST")>=0 ||
		string_cstrsearch(field, "$TRACKARTIST")>=0 ||
		string_cstrsearch(field, "$ALBUMARTIST")>=0 ||
		string_cstrsearch(field, "$COMPOSER")>=0 )
	  string_cstrcpy(class, "container.person.musicArtist") ;
	else if (string_cstrsearch(field, "$ALBUM")>=0)
	  string_cstrcpy(class, "container.album.musicAlbum") ;
	else if (string_cstrsearch(field, "$GENRE")>=0) {
	  if (string_cstrcmp(mediatype, "audio")==0)
	    string_cstrcpy(class, "container.genre.musicGenre") ;
	  else if (string_cstrcmp(mediatype, "video")==0)
	    string_cstrcpy(class, "container.genre.movieGenre") ;
	} else 
	  string_cstrcpy(class, "container.storageFolder") ;
	
      } else {
	
	// This is an item
	if (string_cstrcmp(mediatype, "audio")==0) {
	  if (string_cstrcmp(genre, "Audiobook")==0)
	    string_cstrcpy(class, "item.audioItem.audioBook") ;
	  if (string_cstrcmp(genre, "Broadcast")==0)
	    string_cstrcpy(class, "item.audioItem.audioBroadcast") ;
	  else
	    string_cstrcpy(class, "item.audioItem.musicTrack") ;
	} else if (string_cstrcmp(mediatype, "video")==0) {
	  string_cstrcpy(class, "item.videoItem.video") ;
	} else if (string_cstrcmp(mediatype, "image")==0) {
	  string_cstrcpy(class, "item.imageItem.photo") ; 
	} else {
	  string_cstrcpy(class, "item") ; 
	}
      }

      // Tidy up and expand all of the variables, updating class
      _layout_expandvariable(lo, field, "$ABCTITLE", title) ;
      _layout_expandvariable(lo, field, "$ABCARTIST", albumartist) ;
      _layout_expandvariable(lo, field, "$ABCALBUMARTIST", albumartist) ;
      _layout_expandvariable(lo, field, "$ABCALBUM", album) ;
      _layout_expandvariable(lo, field, "$ABCCOMPOSER", creator) ;
      _layout_expandvariable(lo, field, "$ABCTRACKARTIST", artist) ;
      _layout_expandvariable(lo, field, "$COMMENT", comment) ;
      _layout_expandvariable(lo, field, "$DISC", disc) ;
      _layout_expandvariable(lo, field, "$TRACKNUM", tracknum) ;
      _layout_expandvariable(lo, field, "$0TRACKNUM", paddedtracknum) ;
      _layout_expandvariable(lo, field, "$DATE", date) ;
      _layout_expandvariable(lo, field, "$YEAR", year) ;
      _layout_expandvariable(lo, field, "$DECADE", decade) ;
      _layout_expandvariable(lo, field, "$RESOLUTION", resolution) ;
      _layout_expandvariable(lo, field, "$BITRATE", bitrate) ;
      _layout_expandvariable(lo, field, "$LOCATION", location) ;
      _layout_expandvariable(lo, field, "$GENRE", genre) ;
      _layout_expandvariable(lo, field, "$ARTIST", albumartist) ;
      _layout_expandvariable(lo, field, "$SARTIST", salbumartist) ;
      _layout_expandvariable(lo, field, "$TRACKARTIST", artist) ;
      _layout_expandvariable(lo, field, "$COMPOSER", creator) ;
      _layout_expandvariable(lo, field, "$ALBUMARTIST", albumartist) ;
      _layout_expandvariable(lo, field, "$ALBUM", album) ;
      _layout_expandvariable(lo, field, "$SALBUM", salbum) ;
      _layout_expandvariable(lo, field, "$TITLE", title) ;
      _layout_expandvariable(lo, field, "$PATH", filepath) ;
      _layout_expandvariable(lo, field, "$FILENAME", filename) ;
      _layout_expandvariable(lo, field, "$LOCATION", location) ;
      _layout_expandvariable(lo, field, "$RND", random) ;
      
      // If the result item contains separators, it has been path-expanded
      // So expand the resultclass similarly
      if (string_findch(field, SQL_PATH_SEPARATOR_CHAR)>=0) {
	string classpath = string_new() ;
	int i=0 ;
	while ((i=string_findchn(field, SQL_PATH_SEPARATOR_CHAR, i+1))>=0) {
	  string_cstrcat(classpath, 
			"container.storageFolder" SQL_PATH_SEPARATOR) ;
	}
	string_strcat(classpath, class) ;
	string_strcpy(class, classpath) ;
	string_free(classpath) ;
      }

      // Append the field to the result string and store
      if (string_strlen(resultitem)>0) 
	string_cstrcat(resultitem, SQL_PATH_SEPARATOR) ;
      string_strcat(resultitem, field) ;
      if (string_strlen(resultclass)>0) 
	string_cstrcat(resultclass, SQL_PATH_SEPARATOR) ;
      string_strcat(resultclass, class) ;

    }

    // Store the result string and the fieldtype
    stringlist_push_back(lo->folderpathitem, resultitem) ;
    stringlist_push_back(lo->folderpathclass, resultclass) ;

    string_free(random) ;
    string_free(class) ;
    string_free(resultclass) ;
    string_free(resultitem) ;
    stringlist_free(parts) ;
    
  }

  string_free(salbum) ;
  string_free(salbumartist) ;
  stringlist_free(chainset) ;
  string_free(paddedtracknum) ;
  string_free(year) ;
  string_free(decade) ;
  string_free(filename) ;
  string_free(filepath) ;

  return 1 ;
}


// Free the layout
void layout_free(layout lo)
{
  if (lo) {
    for (int i=0; i<=lo->lastsection; i++) {
      string_free(lo->section[i].title) ;
      stringlist_free(lo->section[i].tag) ;
      stringlist_free(lo->section[i].arg) ;
    }
    free(lo->section) ;
    stringlist_free(lo->folderpathitem) ;
    stringlist_free(lo->folderpathclass) ;
    stringlist_free(lo->searchitemparts) ;
    stringlist_free(lo->searchclassparts) ;
    stringlist_free(lo->configstripfolder) ;
    string_free(lo->configunknown) ;
    lo->section=NULL ;
    lo->lastsection=-1 ;
    free(lo) ;
  }
}

// Load a New Layout Configuration from the given file
layout layout_newfrom(char *filename)
{
  FILE *fp ;
  int configsection = -1 ;
  layout lo ;

  // Randomise rand() function
  srand(time(NULL)) ;
  
  lo = malloc(sizeof(struct _layout_data)) ;

  if (lo) {

    lo->lastsection=-1 ;
    lo->section=NULL ;
    lo->searchitemparts = stringlist_new() ;
    lo->searchclassparts = stringlist_new() ;
    lo->folderpathitem = stringlist_new() ;
    lo->folderpathclass = stringlist_new() ;
    lo->configunknown = string_new() ;
    lo->configstripfolder = stringlist_new() ;
    
    lo->searchhandle=-1 ;
    lo->searchitemref=-1 ;

    fp=fopen(filename, "rb") ;

    if (fp) {

      int forcelower=0 ;
      string line = string_new() ;
      string title = string_new() ;
      string tag = string_new() ;
      string arg = string_new() ;
      int len ;
      
      while ( (len=string_readline(fp, line))>=0) {

	if (string_at(line,0)=='[') {
	  struct _layout_section *newsection ;
	  
	  newsection = realloc(lo->section,
			       sizeof(struct _layout_section) *
			       (lo->lastsection+2) ) ;
	  if (newsection) {
	    lo->section=newsection ;
	    lo->lastsection++ ;
	    lo->section[lo->lastsection].title = string_new() ;
	    lo->section[lo->lastsection].tag = stringlist_new() ;
	    lo->section[lo->lastsection].arg = stringlist_new() ;
	    string_substring(line, lo->section[lo->lastsection].title, 1,
			     string_findch(line, ']')-1) ;
	    string title = lo->section[lo->lastsection].title ;
	    string_capitalise(title, TOLOWER) ;
	    forcelower = (string_cstrcmp(title, "replace")==0 ||
			  string_cstrcmp(title, "rule")==0) ;
	    if (string_cstrcmp(title, "config")==0)
	      configsection=lo->lastsection ;
	  }
	} else if (string_findch(line, '=')>0 &&
		   lo->section && lo->lastsection>=0) {
	  stringlist parts = stringlist_new() ;
	  if (string_splitn(line, '=', parts, 2)) {
	    string tag = stringlist_at(parts, 0) ;
	    string_capitalise(tag, TOLOWER) ;
	    string arg = stringlist_at(parts,1) ;
	    if (forcelower) string_capitalise(arg, TOLOWER) ;

	    stringlist_push_back(lo->section[lo->lastsection].tag, tag) ;
	    stringlist_push_back(lo->section[lo->lastsection].arg, arg) ;
	  }
	  stringlist_free(parts) ;
	}
      }
      string_free(arg) ;
      string_free(tag) ;
      string_free(title) ;
      string_free(line) ;
      fclose(fp) ;
    }
  }

  // Default Configuration Options

  string_cstrcpy(lo->configunknown, "Unknown") ;
  lo->configsearchdepth=2 ;

  // Overwrite with Config Options if Found

  if (configsection>=0) {
    int sz = stringlist_len(lo->section[configsection].tag) ;

    for (int i=0; i<sz; i++) {
      string tag=stringlist_at(lo->section[configsection].tag, i) ;
      string arg=stringlist_at(lo->section[configsection].arg, i) ;
      if (string_cstrcmp(tag, "unknown")==0) {
	if (string_strlen(arg)>0) 
	  string_strcpy(lo->configunknown, arg) ;
      } else if (string_cstrcmp(tag, "searchdepth")==0) {
	int searchdepth = atoi(string_cstr(arg)) ;
	if (searchdepth>=2 && searchdepth<=5)
	  lo->configsearchdepth=searchdepth ;
      } else if (string_cstrcmp(tag, "strippath")==0) {
	string_split(arg, ',', lo->configstripfolder) ;
      }
      
    }
    
  }

  return lo ;
}

// Search for a chainset and return a handle
int layout_search(layout lo, char *cmediatype, char *cpath,
                  char *ccreator, char *calbumartist,
		  char *cartist,  char *calbum,
		  char *cdisc, char *ctracknum,
		  char *ctitle,  char *ccomment,
		  char *cgenre,  char *cdate,
		  char *cresolution, char *crotation,
		  char *cduration,  char *cbitrate,
		  char *clocation)
{
  string path = string_new() ;
  string mediatype = string_new() ;
  string artist = string_new() ;
  string creator = string_new() ;
  string albumartist = string_new() ;
  string album = string_new() ;
  string disc = string_new() ;
  string tracknum = string_new() ;
  string title = string_new() ;
  string comment = string_new() ;
  string genre = string_new() ;
  string date = string_new() ;
  string resolution = string_new() ;
  string rotation = string_new() ;
  string duration = string_new() ;
  string bitrate = string_new() ;
  string location = string_new() ;

  string_cstrcpy(path, cpath) ;
  
  layout_clean(NULL, NULL, cmediatype, mediatype) ;
  layout_clean(lo, "artist", cartist, artist) ;
  layout_clean(lo, "albumartist", calbumartist, albumartist) ;
  layout_clean(lo, "album", calbum, album) ;
  layout_clean(lo, "creator", ccreator, creator) ;
  layout_clean(NULL, NULL, cdisc, disc) ;
  layout_clean(NULL, NULL, ctracknum, tracknum) ;
  layout_clean(lo, "title", ctitle, title) ;
  layout_clean(lo, "comment", ccomment, comment) ;
  layout_clean(lo, "genre", cgenre, genre) ;
  layout_clean(NULL, NULL, cdate, date) ;
  layout_clean(NULL, NULL, cresolution, resolution) ;
  layout_clean(NULL, NULL, crotation, rotation) ;
  layout_clean(NULL, NULL, cduration, duration) ;
  layout_clean(NULL, NULL, cbitrate, bitrate) ;
  layout_clean(lo, "location", clocation, location) ;

  // Pad 1 digit tracknum with a leading '0'.  This helps
  // with media clients that sort alphabetically rather
  // than in track-num sequence
  
  if (string_strlen(tracknum)==1) {
    string paddedtracknum = string_newfrom("0") ;
    string_strcat(paddedtracknum, tracknum) ;
    string_strcpy(tracknum, paddedtracknum) ;
    string_free(paddedtracknum) ;
  }
    
  _layout_processrule(lo, mediatype, path,
		      creator, albumartist, artist, album, disc, tracknum, title, comment, genre,
		      date,
		      resolution, rotation,
		      duration, bitrate,
		      location) ;

  string_free(path) ;
  string_free(mediatype) ;
  string_free(artist) ;
  string_free(creator) ;
  string_free(albumartist) ;
  string_free(album) ;
  string_free(disc) ;
  string_free(tracknum) ;
  string_free(title) ;
  string_free(comment) ;
  string_free(genre) ;
  string_free(date) ;
  string_free(resolution) ;
  string_free(rotation) ;
  string_free(duration) ;
  string_free(bitrate) ;
  string_free(location) ;
  
  return lo->searchhandle ;
}

char * layout_folderpathitem(layout lo, int searchhandle, int itemnum)
{
  return string_cstr(stringlist_at(lo->folderpathitem, itemnum)) ;
}

char * layout_folderpathclass(layout lo, int searchhandle, int itemnum)
{
  return string_cstr(stringlist_at(lo->folderpathclass, itemnum)) ;
}


// Extracts the field names for a given item
int layout_finditemfield(layout lo, int searchhandle,
			 int row, int field,
			 char **parttext, char **partclass)
{
  *parttext="" ;
  *partclass="" ;

  if (searchhandle!=lo->searchhandle) return 0 ;
  if (row<0 || row>=stringlist_len(lo->folderpathitem)) return 0 ;

  if (row!=lo->searchitemref) {
    lo->searchitemref=row ;
    string_split(stringlist_at(lo->folderpathitem, row), 
		 SQL_PATH_SEPARATOR_CHAR,
		 lo->searchitemparts) ;
    string_split(stringlist_at(lo->folderpathclass, row), 
		 SQL_PATH_SEPARATOR_CHAR,
		 lo->searchclassparts) ;
  }
  
  if (field<0 || field>=stringlist_len(lo->searchitemparts))
    return 0 ;

  *parttext = string_cstr(stringlist_at(lo->searchitemparts, field)) ;
  *partclass = string_cstr(stringlist_at(lo->searchclassparts, field)) ;

  return 1 ;
}

int layout_numitems(layout lo, int searchhandle)
{
  if (searchhandle!=lo->searchhandle) return -1 ;
  return stringlist_len(lo->folderpathitem) ;
}


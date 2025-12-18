/*
 *
 * mstring.h
 *
 *
 */

#ifndef _STRING_H_DEFINED_
#define _STRING_H_DEFINED_

// Debug causes cstr to be updated after every modification
// to aid visibility in a debugger
// #define MSTRING_DEBUG

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "mcharacter.h"

struct _string_data {
  char *cstr ;
  long int len ;
  character *data ;
  int cstrupdate ;
} ;

typedef struct _string_data * string ;

struct _stringlist_data {
  string *list ;
  int len, maxlen ;
} ;

typedef struct _stringlist_data * stringlist ;
/*
 * stringlist
 *
 */

typedef enum {
  TOUPPER=0,
  TOLOWER,
  CAPITALISE,
  DEACCENT,
  NOCHANGE
} string_option ;

// Allocate, clear and free the memory required for a list of strings
stringlist stringlist_new() ;
void stringlist_clear(stringlist list) ;
void stringlist_free(stringlist list) ;

// Append a string to the back of the list
int stringlist_push_back(stringlist list, string str) ;

// Return the length of the list
int stringlist_len(stringlist list) ;

// Return a pointer to the string at the given index 'n'
const string stringlist_at(stringlist list, int n);

// Merge elements of a substring into result
int stringlist_merge(stringlist list, int from, int to, char *separator, string result) ;

// Allocate, clear  and free memory for the string
string string_new() ;
string string_newfrom(char *utf8src) ;
void string_clear(string str) ;
void string_free(string str) ;

// Convert to and from integers
long int string_toint(string str, int base) ;
int string_fromint(string str, long int n, int base, int pad) ;

// append cat to the end of str.  The last entry for strcatn must be NULL
long int string_strcat(string str, string cat) ;
long int string_strcatn(string src, ... ) ;
long int string_cstrcat(string str, char *utf8cat) ;

// return the number of characters in a string
long int string_strlen(string str) ;

// Return the length of a packed UTF8 Unicode version of the string
// (excluding \0 terminator).
long int string_cstrlen(string str) ;

// Convert (and return) the string to a
// packed UTF8 Unicode string (including \0 terminator)
char *string_cstr(string str) ;

// copy src into dst. Returns the number of characters, or -1 on error
long int string_strcpy(string dst, string src);
long int string_cstrcpy(string dst, char *utf8src) ;

// search for character 'needle' in 'src', and
// return position or -1 if not found
long int string_findchn(string src, character needle, long int start) ;
long int string_findch(string src, character needle) ;
long int string_rfindch(string src, character needle) ;

// Matches str1 and str2, returns
// 0 if equal
// -1 if str1 comes before str2
// 1 if str2 comes before str1
int string_strcmp(string str1, string str2) ;
int string_cstrcmp(string str1, char *str2) ;

// Case Insensitive Matches str1 and str2, returns
// 0 if equal
// -1 if str1 comes before str2
// 1 if str2 comes before str1
int string_strcasecmp(string str1, string str2) ;
int string_cstrcasecmp(string str1, char *str2) ;


// search for a string, and return its index, or -1 if not found
long int string_search(string haystack, string needle) ;
long int string_searchn(string haystack, string needle, long int start) ;
long int string_cstrsearch(string haystack, char *needle) ;
long int string_cstrsearchn(string haystack, char *needle, long int start) ;

// replace all occurrences of 'what' in string 'str' with 'with'
// with replacen, replace occurrences starting from character 'start'
int string_replace(string str, string what, string with) ;
int string_replacen(string str, string what, string with, long int start,
		    int count) ;
int string_cstrreplace(string str, char *what, char *with) ;
int string_cstrreplacen(string str, char *what, char *with, 
		    long int start, int count) ;


// Extract the substring from src and place it in dst
long int string_substring(string src, string dst, long int start,
			  long int end) ;
character string_at(string src, long int n) ;

// Split the string at a given 'splitchar'
int string_split(string src, character splitchar, stringlist dstlist) ;
int string_splitn(string src, character splitchar, stringlist dstlist, int n) ;

// Capitalise the entire string, formatting as defined by opt
void string_capitalise(string str, string_option opt) ;

// Read a line from a file and return the length read
// 0 indicates end of file, -1 indicates error (e.g. file not found)
long int string_readline(FILE *fp, string dst) ;

#endif // _STRING_H_DEFINED_

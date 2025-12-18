/*
 *
 * mstring.c
 * =========
 *
 *
 * A Note on Terminology
 * ---------------------
 *
 * byte      -  an 8-bit piece of data
 *
 * character -  a character, which may be 8 or more bits long.  
 *              Unicode characters are up to 4 bytes in length.
 *
 * The string "éE" contains 2 characters, but 4 unicode bytes
 *
 */

#include <stdlib.h>
#include <stdarg.h>
#include "mstring.h"
#include "mcharacter.h"

#ifdef MSTRING_DEBUG
#define mstring_debug_cstr_refresh(str) string_cstr(str)
#else
#define mstring_debug_cstr_refresh(str)
#endif


string _string_errstring() ;
#define STRINGLIST_CHUNK 32
int _stringlist_expand(stringlist list) ;



string _string_errstring()
{
  static character errstr[] = { 0x45, 0x52, 0x52 } ;
  static struct _string_data err = { .data = errstr, .cstr = "ERR",
				     .len=3, .cstrupdate=0 } ;
  return &err ;
}


/*
 *
 * stringlist
 *
 */

int _stringlist_expand(stringlist list)
{
  int success=0 ;
  if (list) {
    long int newlen = list->maxlen + STRINGLIST_CHUNK ;
    string *newptr ;
    newptr = realloc(list->list, newlen * sizeof(string *)) ;
    if (newptr) {
      list->maxlen = newlen ;
      list->list = newptr ;
      success=1 ;
    }
  }
  return success ;
}

stringlist stringlist_new()
{
  stringlist ptr ;
  ptr = malloc(sizeof(struct _stringlist_data)) ;
  if (ptr) {
    ptr->list=NULL ;
    ptr->len=0 ;
    ptr->maxlen=0 ;
  }
  return ptr ;
}

int stringlist_push_back(stringlist list, string str)
{
  string nstr ;
  if (!list || !str) return -1 ;
  
  nstr = string_new(NULL) ;
  string_strcpy(nstr, str) ;

  if (list->len>=list->maxlen)
    _stringlist_expand(list) ;

  if (list->len<list->maxlen) {
    list->list[list->len] = nstr ;
    list->len++ ;
  } else {
    string_free(nstr) ;
  }

  return list->len ;
}

  
void stringlist_clear(stringlist list)
{
  if (list && list->list) {
    for (int i=0; i<list->len; i++)
      string_free(list->list[i]) ;
    free(list->list) ;
    list->list=NULL ;
    list->maxlen=0 ;
    list->len=0 ;
  }
}

void stringlist_free(stringlist ptr)
{
  if (ptr) {
    stringlist_clear(ptr);
    free(ptr) ;
  }
}

int stringlist_len(stringlist list)
{
  if (list) return list->len ;
  else return -1 ;
}

const string stringlist_at(stringlist list, int n)
{

  if (!list || !list->list || n<0 || n>=list->len)
    return _string_errstring() ;
  else
    return list->list[n] ;
}

int stringlist_merge(stringlist list, int from, int to, char *separator, string result)
{
  string_clear(result) ;
  if (from<0 || to<from || to>=stringlist_len(list)) {
    for (int a=from; a<=to; a++) {
      string_strcat(result, stringlist_at(list, a)) ;
      if (a<to && separator) string_cstrcat(result, separator) ;
    }
    return 1 ;
  } else {
    return 0 ;
  }
}


/*
 * string
 *
 */

string string_new()
{
  string ptr = malloc(sizeof(struct _string_data)) ;
  if (ptr) {
    ptr->data = NULL ;
    ptr->cstr = NULL ;
    string_clear(ptr) ;
  }
  return ptr ;
}


string string_newfrom(char *src)
{
  string ptr = string_new() ;
  string_cstrcpy(ptr, src) ;
  return ptr ;
}


void string_free(string str)
{
  if (str) {
    string_clear(str) ;
    free(str) ;
  }
}

void string_clear(string str)
{
  if (str) {
    if (str->data) free(str->data) ;
    if (str->cstr) free(str->cstr) ;
    str->len=0 ;
    str->data=NULL ;
    str->cstr=NULL ;
    str->cstrupdate=1 ;
  }
}

long int string_toint(string str, int base)
{
  return strtol(string_cstr(str), NULL, base) ;
}

int string_fromint(string str, long int n, int base, int pad)
{
  if (base==10 || base==16) {
    char buf[32] ;
    char fmt[32] ;
    snprintf(fmt, sizeof(fmt), "%s0%d%s", "%", pad, (base==16?"lX":"ld")) ;
    snprintf(buf, sizeof(buf), fmt, n) ;
    string_cstrcpy(str, buf) ;
    return 1 ;
  } else {
    string_cstrcpy(str, "0") ;
    return 0 ;
  }
}

long int string_strlen(string str)
{
  if (str) return str->len ;
  else return -1 ;
}

// Returns the cstr length (number of predicted bytes, excluding \0)
long int string_cstrlen(string str)
{
  if (str && str->data) {
    long int len=0 ;
    for (long int i=0; i<str->len; i++) {
      len+= character_to_utf8(str->data[i], NULL) ;
    }
    return len ;
  } else if (str) {
    return 0 ;
  } else {
    return -1 ;
  }
}

long int string_cstrcpy(string dst, char *utf8src)
{
  long int len=-1 ;
  
  if (utf8src && dst) {

    string_clear(dst) ;
    dst->len = utf8_mblen(utf8src) ;

    if (dst->len>0) {
      dst->data = malloc( (dst->len) * sizeof(character) ) ;
      if (dst->data) {
	long int srcpos=0, dstpos=0 ;
	character ch ;
	int used ;
	do {
	  used = utf8_to_character(&utf8src[srcpos], &ch) ;
	  if (used>0) {
	    srcpos+=used ;
	    dst->data[dstpos++] = ch ;
	  }
	} while (dstpos<dst->len && used>0) ;
	dst->cstrupdate=1 ;
	len=dst->len ;
      } else {
	string_clear(dst) ;
      }
    }
  }
  mstring_debug_cstr_refresh(dst) ;
  return len ;
}

char *string_cstr(string src)
{
  if (src && src->cstrupdate) {

    long int len = string_cstrlen(src)+1 ;
    src->cstr = realloc(src->cstr, len) ;

    if (src->cstr) {
      for (int i=0, cstrpos=0, size=1 ;
	   cstrpos<len && i<src->len && size>0;
	   i++) {
	size = character_to_utf8(src->data[i], &(src->cstr[cstrpos])) ;
	if (size>0) cstrpos+=size ;
      }
      src->cstr[len-1]='\0' ;
    }
    
    src->cstrupdate=0 ;
    
  }
  
  if (src && src->cstr)
    return src->cstr ;
  else
    return "ERR" ;

}

long int string_cstrcat(string str, char *utf8cat)
{
  string catstr = string_newfrom(utf8cat) ;
  long int len = string_strcat(str, catstr) ;
  string_free(catstr) ;
  return len ;
}

long int string_strcpy(string dst, string src)
{
  if (src && dst) {
    string_clear(dst) ;
    return string_strcat(dst, src) ;
  } else {
    return -1 ;
  }
}

long int string_strcat(string str, string cat)
{
  character *newalloc ;
  
  if (str && cat && cat->data) {

    newalloc = realloc(str->data, (str->len + cat->len) * sizeof(character)) ;
    
    if (newalloc) {

      str->data = newalloc ;
      
      for(int i=0; i<cat->len; i++)
        str->data[str->len+i] = cat->data[i] ;
      
      str->len = str->len + cat->len ;
      str->cstrupdate=1 ;

      mstring_debug_cstr_refresh(str) ;
      return str->len ;
    }
    
  }
  return -1 ;
}

long int string_strcatn(string src, ... )
{
  long int sz=-1 ;
  va_list argp ;
  string str ;
  
  va_start(argp, src) ;

  while ( (str = va_arg(argp, string)) != NULL) {
    if (sz<0) sz=0 ;
    sz+= string_strcat(src, str) ;
  }
  
  va_end(argp) ;

  return sz ;
}

long int string_findch(string src, character needle)
{
  return string_findchn(src, needle, 0) ;
}

long int string_findchn(string src, character needle, long int start)
{
  long int pos=-1 ;
  if (src && src->data && start<src->len) {
    for (pos=start; pos<src->len && src->data[pos]!=needle; pos++) ;
    if (pos==src->len) pos=-1 ;
  }
  return pos ;
}

long int string_rfindch(string src, character needle)
{
  long int pos=-1 ;
  if (src && src->data) {
    for (pos=src->len-1; pos>=0 && src->data[pos]!=needle; pos--) ;
  }
  return pos ;
}

long int string_substring(string src, string dst, long int start, long int end)
{
  if (!src || !dst) return -1 ;

  string_clear(dst) ;
  
  if (end<0) end=(src->len-1) ;
  
  if (start>=0 && end>=start && end<src->len) {

    dst->data=malloc(sizeof(character) * (1+end-start)) ;
    
    if (dst->data) {
      dst->len = (1+end-start) ;
      for (long int i=0; i<dst->len; i++) dst->data[i] = src->data[i+start] ;
    }

  }
  mstring_debug_cstr_refresh(dst) ;
  return dst->len ;
}

character string_at(string str, long int n)
{
  if (!str || !str->data || n<0 || n>=str->len) return CHNUL ;
  else return str->data[n] ;
}

long int string_cstrsearch(string haystack, char *needle)
{
  return string_cstrsearchn(haystack, needle, 0) ;
}

long int string_cstrsearchn(string haystack, char *needle, 
	long int start)
{
  string what = string_newfrom(needle) ;
  long int reply = string_searchn(haystack, what, start) ;
  string_free(what) ;
  return reply ;
}
  
long int string_search(string haystack, string needle)
{
  return string_searchn(haystack, needle, 0) ;
}

long int string_searchn(string haystack, string needle, long int start)
{
  long int pos=-1 ;
  
  if (haystack && needle &&
      haystack->data && needle->data &&
      start>=0 && start<(haystack->len-1)) {
    
    long int idx ;
    pos=start ;
    
    do {
      
      idx=0 ;
        
      while (idx < needle->len &&
	     (idx+pos) < haystack->len &&
	     needle->data[idx] == haystack->data[idx+pos]) {
	idx++ ;
      }
      pos++ ;
      
    } while (idx<needle->len && pos<haystack->len) ;

    if (pos >= haystack->len) pos=-1 ;
    else pos-- ;
  }

  return pos ;
}

int string_cstrcmp(string str1, char *cstr2)
{
  if (!str1 || !str1->data) return -1 ;
  if (!cstr2) return 1 ;
  
  string str2 = string_newfrom(cstr2) ;
  int result = string_strcmp(str1, str2) ;
  string_free(str2) ;

  return result ;
}

int string_strcmp(string str1, string str2)
{
  long int i=0 ;
  if (!str1 || !str1->data || str1->len<=0) return -1 ;
  if (!str2 || !str2->data || str2->len<=0) return 1 ;

  while (i<str1->len && i<str2->len && str1->data[i]==str2->data[i]) i++ ;

  if (i==str1->len && i==str2->len) return 0 ;
  if (i==str2->len) return 1 ;
  if (i==str1->len) return -1 ;

  if (str1->data[i]>str2->data[i]) return 1 ;
  else return -1 ;
}

int string_cstrcasecmp(string str1, char *cstr2)
{
  if (!str1 || !str1->data) return -1 ;
  if (!cstr2) return 1 ;
  
  string str2 = string_newfrom(cstr2) ;
  int result = string_strcasecmp(str1, str2) ;
  string_free(str2) ;

  return result ;
}

int string_strcasecmp(string str1, string str2)
{
  long int i=0 ;

  if (!str1 || !str1->data || str1->len<=0) return -1 ;
  if (!str2 || !str2->data || str2->len<=0) return 1 ;

  while (i<str1->len && i<str2->len && character_tolower(str1->data[i])==character_tolower(str2->data[i])) i++ ;

  if (i==str1->len && i==str2->len) return 0 ;
  if (i==str2->len) return 1 ;
  if (i==str1->len) return -1 ;

  if (character_tolower(str1->data[i]) > character_tolower(str2->data[i])) return 1 ;
  else return -1 ;
}

int string_cstrreplace(string str, char *what, char *with)
{
  return string_cstrreplacen(str, what, with, 0, -1) ;
}

int string_cstrreplacen(string str, char *what, char *with, 
		    long int start, int count)
{
  int result ;
  string swhat = string_newfrom(what) ;
  string swith = string_newfrom(with) ;
  result = string_replacen(str, swhat, swith, start, count) ;
  string_free(swith) ;
  string_free(swhat) ;
  return result ;
}


int string_replace(string str, string what, string with)
{
  return string_replacen(str, what, with, 0, -1) ;
}

int string_replacen(string str, string what, string with, long int start, int n)
{
  int c=-1 ;

  if (str && what && with &&
      str->data && str->len>0 &&
      what->data && what->len>0 &&
      with->data) {

    c=0 ;

    if (start>=0 && start<(str->len-1)) {

      string left=string_new() ;
      string right=string_new() ;
      
      long int i=start ;
      do {
	i = string_searchn(str, what, i) ;
	if (i==0) {
	  string_substring(str, right, what->len, str->len-1) ;
	  string_strcpy(str, with) ;
	  string_strcat(str, right) ;
	  i=with->len ;
	  c++ ;
	} else if (i>0) {
	  string_substring(str, left, 0, i-1) ;
	  string_substring(str, right, i+what->len, str->len-1) ;
	  string_strcpy(str, left) ;
	  string_strcat(str, with) ;
	  string_strcat(str, right) ;
	  i+=with->len ;
	  c++ ;
	}
      } while (i>0 && (n<0 || c<n)) ;
      string_free(left) ;
      string_free(right) ;
    }
  }
  return c ;
}

void string_capitalise(string str, string_option opt)
{
  if (opt==NOCHANGE) return ;
  if (str && str->len>0 && str->data) {
    long int i ;
    // int lastlastwasalpha=0 ;
    int lastwasalpha=0 ;
    int lastwasapostrophe=0 ;
    int nextnextisalpha=0 ;
    for (i=0; i<str->len; i++) {
      if (opt==DEACCENT) {
	str->data[i] = character_deaccent(str->data[i]) ;
      } else {
	if ( (opt==CAPITALISE && !lastwasalpha && !lastwasapostrophe) || opt == TOUPPER ) {
	  str->data[i] = character_toupper(str->data[i]) ;
	} else {
	  str->data[i] = character_tolower(str->data[i]) ;
	}
	// Apostrophes are preceded by at least 2 alpha characters
	// Otherwise they are treated as quotes
	nextnextisalpha = (i<(str->len-2) && character_isletter(str->data[i+2])) ;
	lastwasapostrophe = (str->data[i]=='\'' &&
			     ( lastwasalpha && !nextnextisalpha)) ;
	// && lastlastwasalpha ) ;
	//	lastlastwasalpha = lastwasalpha ;
	lastwasalpha = (character_isletter(str->data[i])) ;
      }
    }
    
    str->cstrupdate=1 ;
    mstring_debug_cstr_refresh(str) ;
  }
}

int string_split(string src, character splitchar, stringlist dstlist)
{
  return string_splitn(src, splitchar, dstlist, -1) ;
}

int string_splitn(string src, character splitchar, stringlist dstlist, int n)
{
  int count=-1 ;
  if (src  && dstlist) {

    count=0 ;
    long int start=0, end=0 ;
    stringlist_clear(dstlist) ;
    string entry=string_new() ;

    if (src->len>0) {

      while ( (n<0 || count<(n-1)) &&
	      (end=string_findchn(src, splitchar, start)) >=0 ) {

	if (end==start) { string_clear(entry) ; }
	else { string_substring(src, entry, start, end-1) ; }

	stringlist_push_back(dstlist, entry) ;
	start=(end+1) ;
	count++ ;

      }
      
      if (end==string_strlen(src)) { string_clear(entry) ; }
      else { string_substring(src, entry, start, -1) ; }
      stringlist_push_back(dstlist, entry) ;
      count++ ;
      
    }

    string_free(entry) ;

  }

  return count ;
}

// Read a line from a file and return the length read
// 0 indicates end of file, -1 indicates error (e.g. file not found)
// NEWLINE = '\n' for Unix / Windows, and '\r' for Mac

#define READLINE_CHUNK 32
#define NEWLINE '\n'

long int string_readline(FILE *fp, string dst)
{
  long int len=-2 ;

  if (fp) {

    char *buf=NULL, *newallocation=NULL ;
    long int buflen=0 ;
    long int pos=0 ;
    char ch ;

    do {
      
      ch=fgetc(fp) ;
      
      if (pos>(buflen-2)) {
	newallocation = realloc(buf, buflen + READLINE_CHUNK) ;
	if (newallocation) {
	  buf=newallocation ;
	  buflen += READLINE_CHUNK ;
	}
	
      }
      
      if (newallocation && ch!=EOF && ch!='\r' && ch!='\n') buf[pos++] = ch ;
      
    } while ( pos<buflen && ch!=EOF && ch!=NEWLINE ) ;
	      	      
    if (newallocation) {
      if (pos==0) {
	string_clear(dst) ;
	len=0 ;
      } else {
	buf[pos]='\0' ;
	len=string_cstrcpy(dst, buf) ;
      }
    }

    if (buf) free(buf) ;

    if (len==0 && ch==EOF) len=-1 ;
    
  }
  return len ;
}


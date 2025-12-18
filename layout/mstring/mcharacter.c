/*
 * mcharacter.c
 *
 */

#include "mcharacter.h"
#include "municodetables.h"

/*
 * Converts the character to Upper / Lower Case
 */

character character_tolower(character upper)
{
  int i=0 ;
  do {
    if (unicode_casefold_table[i] == upper) return unicode_casefold_table[i+1] ;
    i+=2 ;
  } while (unicode_casefold_table[i]!=CHNUL) ;
  return upper ;
}

character character_toupper(character lower)
{
  int i=0 ;
  do {
    if (unicode_casefold_table[i+1] == lower) return unicode_casefold_table[i] ;
    i+=2 ;
  } while (unicode_casefold_table[i]!=CHNUL) ;
  return lower ;
}

character character_deaccent(character src)
{
  int i=0 ;
  do {
    if (unicode_base_table[i] == src) return unicode_base_table[i+1] ;
    i+=2 ;
  } while (unicode_base_table[i]!=CHNUL) ;
  return src ;
}

int character_isnumber(character ch)
{
  int i=0 ;
  do {
    if (unicode_number_table[i] == ch) return 1 ;
    i++ ;
  } while (unicode_number_table[i]!=CHNUL) ;
  return 0 ;
}


int character_isletter(character ch)
{
  int i=0 ;
  do {
    if (unicode_letter_table[i] == ch) return 1 ;
    i++ ;
  } while (unicode_letter_table[i]!=CHNUL) ;
  return 0 ;
}


int character_ispunctuation(character ch)
{
  int i=0 ;
  // The apostrophe is special, and is not always
  // a punctuation character.
  if (ch == '\'') return 0 ;
  do {
    if (unicode_punctuation_table[i] == ch) return 1 ;
    i++ ;
  } while (unicode_punctuation_table[i]!=CHNUL) ;
  return 0 ;
}


int character_iswhite(character ch)
{
  int i=0 ;
  do {
    if (unicode_whitespace_table[i] == ch) return 1 ;
    i++ ;
  } while (unicode_whitespace_table[i]!=CHNUL) ;
  return 0 ;
}


/*
 * Converts character to a UTF8 multibyte string
 */
int character_to_utf8(character src, char *dest)
{
  if (src==CHNUL) {
    if (dest) {
      dest[0]='\0' ;
    }
    return 0 ;
  } else if (!(src&0xFFFFFF80)) {
    if (dest) {
      dest[0] = src&0x7F ;
    }
    return 1 ;
  } else if (!(src&0xFFFFF800)) {
    if (dest) {
      dest[0] = 0xC0 | ((src>>6) & 0x1F) ;
      dest[1] = 0x80 | ((src) & 0x3F) ;
    }
    return 2 ;
  } else if (!(src&0xFFFF0000)) {
    if (dest) {
      dest[0] = 0xE0 | ((src>>12) & 0x1F) ;
      dest[1] = 0x80 | ((src>>6) & 0x3F) ;    
      dest[2] = 0x80 | ((src) & 0x3F) ;
    }
    return 3 ;
  } else {
    if (dest) {
      dest[0] = 0xF0 | ((src>>18) & 0x1F) ;
      dest[1] = 0x80 | ((src>>12) & 0x3F) ;    
      dest[2] = 0x80 | ((src>>6) & 0x3F) ;    
      dest[3] = 0x80 | ((src) & 0x3F) ;  
    }
    return 4 ;
  }
}


/*
 * Convert a UTF8 multibyte string to a character
 */
int utf8_to_character(char *src, character *dest)
{
  if (!src) return -1 ;
  
  int len ;

  if (src[0]=='\0')
    { len=0 ; if (dest) *dest=CHNUL ; }
  else if (((unsigned char)src[0] & 0x80) == 0)
    { len=1 ; if (dest) *dest = (character)*src ; }
  else if (((unsigned char)src[0] & 0xE0) == 0xC0)
    { len=2 ; if (dest) *dest = ((character)((unsigned char)src[0] & 0x1F)) ; }
  else if (((unsigned char)src[0] & 0xF0) == 0xE0)
    { len=3 ; if (dest) *dest = ((character)((unsigned char)src[0] & 0x0F)) ; }
  else if (((unsigned char)src[0] & 0xF8) == 0xF0)
    { len=4 ; if (dest) *dest = ((character)((unsigned char)src[0] & 0x07)) ; }
  else
    { len=-1 ; if (dest) *dest = CHNUL ; }

  for (int i=1; i<len; i++) {
    if (( (unsigned char)src[i] & 0xC0 ) != 0x80 ) {
      if (dest) *dest = CHNUL ;
      len=-1 ;
    } else if (dest) {
      *dest = (*dest)<<6 ;
      *dest += (character) ((unsigned char)src[i] & 0x3F) ;
    }
  }

  return len ;
}

/*
 * Returns the character pointed to by utf8chr
 */
character ascharacter(char *utf8chr)
{
  character reply ;
  utf8_to_character(utf8chr, &reply) ;
  return reply ;
}
      

/*
 * Calculates the number of multibyte chars encoded in a UTF8 string
 */
size_t utf8_mblen(char *src)
{
  size_t len=-1 ;
  if (src) {
    size_t pos=0 ;
    int size ;
    len=0 ;
    do {
      size = utf8_to_character(&src[pos], NULL) ;
      if (size>0) { pos+=size; len++ ; }
    } while (size>0) ;
    if (size<0) len=-1 ;
  }
  return len ;
}


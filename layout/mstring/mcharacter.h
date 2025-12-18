/*
 *
 * character.h
 *
 * Unicode Character Functions, including UTF8 Encoding and Decoding functions
 *
 * character character_toupper(character src)
 *
 *   Converts the source character to upper case and returns the new character.
 *
 * character character_tolower(character src)
 *
 *   Converts the source character to lower case and returns the new character.
 *
 * character character_deaccent(character src)
 *
 *   Returns a non-accented version of the character, e.g. "e" for "é"
 *
 * int character_iswhite(character src)
 *
 *   Returns true (1) if the character is a white space or punctuation 
 *   character
 *
 * int character_to_utf8(character src, char *dest)
 *
 *   Converts the 'src' unicode character to a UTF8 multibyte string at 'dest'.
 *   Returns the size of the multibyte string created.
 *   If 'dest' is NULL, returns the size, but does not create the multi-byte 
 *   string. 'dest' must have at least UTF8_MAX bytes available.
 *
 * int utf8_to_character(char *src, character *dest) ;
 *
 *   Converts the UTF8 multibyte string to a unicode character and stores 
 *   at 'dest'.
 *   Returns the number of UTF8 multibyte chars used.
 *   Returns 0 for end of string (i.e. when \0 is detected), and assigns 
 *   'dest' to CHNUL.
 *   Returns -1 for a parsing error, and assigns 'dest' to CHNUL.
 *   If 'dest' is NULL, returns the number of UTF8 multibyte chars used, 
 *   but does not create the character.
 *
 * character ascharacter(char *utf8chr) ;
 *
 *   Converts a multi-byte UTF8 char sequence 'utf8chr' and returns it as a 
 *   character
 *   Invalid characters and ends of string are returned as CHNUL.  
 *   There is no mechanism to show how many utf8str chars were used.  
 *   For that, use the utf8_to_character function.
 *
 * size_t utf8_mblen(insigned char *src) ;
 *
 *   Returns the number of multi-byte chars that are encoded in the 'src' 
 *   multi-byte UTF8 string.
 *
 */

#ifndef _CHARACTER_H_DEFINED_
#define _CHARACTER_H_DEFINED_

#include <stdint.h>
#include <stddef.h>

#define UTF8_MAX 4
#define character uint32_t
#define CHNUL (character)0

int character_to_utf8(character src, char *dest) ;
int utf8_to_character(char *src, character *dest) ;
character ascharacter(char *utf8chr) ;
size_t utf8_mblen(char *src) ;
character character_toupper(character src) ;
character character_tolower(character src) ;
character character_deaccent(character src) ;
int character_isletter(character src) ;
int character_isnumber(character src) ;
int character_ispunctuation(character src) ;
int character_iswhite(character src) ;

#endif // _CHARACTER_H_DEFINED_

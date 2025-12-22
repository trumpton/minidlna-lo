#include "mstring.h"
#include <stdio.h>

int main() {

	string s = string_new() ;
	string_fromint(s, 18, 10, 3) ;
	printf(" %d = %s\n", 18, string_cstr(s)) ;
}

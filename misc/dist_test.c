#include <stdio.h>
#include <string.h>
#include "../pp_str.h"

int main(int argc, char **argv) {
	char *a, *b;

#define _T(A,B)a = A; b = B;\
printf("%u '%s' '%s'\n", pp_str_dist(a, strlen(a), b, strlen(b)), a, b);

	_T("hello","yullo");
	_T("he","hello");
	_T("hello","helo");
	_T("helo","hello");
	_T("OOE","OOA");
	_T("OOE","OOAE");
	_T("OOEA","OOAE");
	_T("","abc");
	_T("abc","");
	_T("hello","h");
	_T("x","hello");
	_T("","");
	return 0;
}

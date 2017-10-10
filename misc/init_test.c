#include <stdio.h>

char keys[256] = {
	['a' ... 'z'] = 1,
	['A' ... 'Z'] = 1,
	['0' ... '9'] = 1,
	['-'] = 1,
	['_'] = 1
};
	// ['a' ... 'z'] = 1
	/**,
	['A' ... 'Z'] = 1,
	['0' ... '9'] = 1,
	['-'] = 1,
	['_'] = 1
	,**/

int main(int argc, char **argv) {
	int c = getchar();
	if (keys[c]) printf("just keep O2 out of the way");
	return 0;
}

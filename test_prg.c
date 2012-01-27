#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "test.h"
#include "arf.h"

void (*dso_foo_cb)(void);

void nop(void) { }

static char const *str2 = "str2";

static void main_foo(int alpha, int beta)
{
	float f = 33.44;
	double d = 55.66;
	long long ll = 77;
	static char const *str = "susu";
	char const fourbytes[] = { 0x10, 0x20, 0xef, 0xff };
	char const (*binaryp)[] = &fourbytes;

	void main_foo_foo(void)
	{
		BARF();
	}

	nop();
	nop();
	main_foo_foo();
	boo();
	alpha *= 2; beta *= 3; f *= 4; d *= 5; ll *= 6;
//	return;
	lib_foo();
	nop();
	nop();
}

int main(int argc, char const *argv[])
{
	int i;
	char c;
	unsigned u;

	stderr = stdout;
	dso_foo_cb = dlsym(dlopen("./dso.so", RTLD_GLOBAL | RTLD_LAZY),
		"dso_foo");

	nop();
	nop();
	do { int infoo = 99; main_foo(11, 22); } while (0);
	nop();
	nop();
	return 0;
}

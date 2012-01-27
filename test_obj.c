#include <dlfcn.h>

#include "test.h"
#include "arf.h"

static void main_baz()
{
	nop();
	nop();
	boo();
	((void (*)(void))dlsym(dlopen("./dso.so", RTLD_GLOBAL | RTLD_LAZY), "dso_baz"))();
	nop();
	nop();
}

void main_bar()
{
	nop();
	nop();
	main_baz();
	nop();
	nop();
}


#include <stdlib.h>
#include <dlfcn.h>

#include "test.h"
#include "arf.h"

void (*dso_foo_cb)(void);
static void (*dso_baz)(void);

void nop(void) { }

static void main_baz()
{
	nop();
	nop();
	dso_baz();
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

static void main_foo()
{
	nop();
	nop();
	lib_foo();
	nop();
	nop();
}

int main()
{
	void *dso;

	dso = dlopen("./dso.so", RTLD_GLOBAL | RTLD_LAZY);
	dso_foo_cb = dlsym(dso, "dso_foo");
	dso_baz    = dlsym(dso, "dso_baz");

	nop();
	nop();
	main_foo();
	nop();
	nop();
	return 0;
}

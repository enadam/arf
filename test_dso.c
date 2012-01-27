#include <signal.h>

#include "test.h"
#include "arf.h"

static void dso_argh(int unused)
{
	BARF("SIGABRT");
}

void dso_baz(void)
{
	nop();
	nop();
	BARF("poop");
	lib_baz();
	nop();
	nop();
}

static void dso_bar(void)
{
	nop();
	nop();
	main_bar();
	nop();
	nop();
}

void dso_foo(void)
{
	signal(SIGABRT, dso_argh);
	nop();
	nop();
	dso_bar();
	nop();
	nop();
}

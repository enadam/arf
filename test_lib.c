#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "test.h"
#include "arf.h"

void lib_baz(void)
{
	nop();
	nop();
	barf("buuuu %s:", "baaa");
	nop();
	nop();
	kill(getpid(), SIGABRT);
}

static void lib_bar(void)
{
	nop();
	nop();
	dso_foo_cb();
	nop();
	nop();
}

void lib_foo(void)
{
	nop();
	nop();
	lib_bar();
	nop();
	nop();
}

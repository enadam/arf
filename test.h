#include "arf.h"

extern void nop(void);
extern void main_bar(void);

extern void lib_foo(void);
extern void lib_baz(void);

extern void (*dso_foo_cb)(void);

static void __attribute__((unused)) boo(void) { BARF(); }

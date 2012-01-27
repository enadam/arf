#ifndef _ARF_H
#define _ARF_H

/*
 * Configuration: what usage of libarf is to be supported.
 * There are two independent factors to be concerned about:
 * whether arf.h is #include:d by the program and how libarf.so is
 * linked with the executable.  This table summarizes the scenarios:
 *
 * all: not #include:d, not linked (this always works, but i take no credit)
 *  L0:     #include:d, compile-time linked (-larf)
 *  L1:     #include:d, only run-time linked ($LD_PRELOAD)
 *  L1:     #include:d, not linked
 *  L2: not #include:d, compile-time linked
 *  L2: not #include:d, only run-time linked (ld.so.preload)
 *
 * The goal was to make it seamless to opt in and out of libarf.
 * You can choose the level of flexibility with CONFIG_LIBARF_EXTERNAL.
 * Each level extends the supported use cases of the previous level.
 *
 * Level 0 is the regular usage of libraries and it carries no penalties.
 * Level 1 lets use libarf with selected programs without altering your
 * build machinery.  This can be a big save in the empire of autoconf.
 * It has a negligable impact on barf()s run-time performance.
 * Level 2 makes it possible for you to drop libarf.so in ld.so.preload
 * and barf() to your heart's content whenever you feel like it.
 * It has a slightly more impact compared to level 2, adds more code,
 * may be less robust because it does more complicated things in
 * critical times (early program startup), and clearenv() may disable it.
 *
 * Despite its shortcomings the default is to be as flexible as possible.
 */
#ifndef CONFIG_LIBARF_EXTERNAL
# define CONFIG_LIBARF_EXTERNAL		2
#endif

/* The rest will just confuse things when compiling libarf.c. */
#ifndef _LIBARF_C

// Be C++ friendly.
#ifdef __cplusplus
# define LIBARF_EXTERN			"C"
#else
# define LIBARF_EXTERN			/* nothing */
#endif

#if CONFIG_LIBARF_EXTERNAL
/*
 * Will eventually point to the real barf() defined in libarf.c
 * if the program is linked with the library.  While this symbol
 * will be defined in all translation units #include:ing this header
 * the dynamic linker makes sure to merge those definitions.
 */
void (*the_real_barf)(char const *, ...)
	__attribute__((format(printf, 1, 2)));
#else
void barf(char const *, ...)
	__attribute__((format(printf, 1, 2)));
#endif

/*
 * Define the library user's barf(), your entry point to libarf.
 * It prints the backtrace on the selected output streams or
 * with the selected method, which you can change by recompiling
 * the library.  It sports printf()-style arguments, which will
 * be printed as headline.
 */
#if CONFIG_LIBARF_EXTERNAL > 1

/*
 * The address of the real barf() is communicated through
 * an environment variable, let's find it out.  Yes, ugly.
 * Instances of $tried_to_find_the_real_barf is also subject
 * to merging so find_the_real_barf() should be run only once.
 */
extern LIBARF_EXTERN int sscanf(char const *, char const *, ...);
extern LIBARF_EXTERN char *getenv(char const *);
int tried_to_find_the_real_barf;
static void find_the_real_barf(void)
{
	char const *str;

	tried_to_find_the_real_barf = 1;
	if (!(str = getenv("THE_REAL_BARF")))
		/* Not linked with libarf.so, or
		 * clearenv() made its presence. */
		return;
	sscanf(str, "%p", &the_real_barf);
}

#define barf(...)				\
do						\
{						\
	if (!tried_to_find_the_real_barf)	\
		find_the_real_barf();		\
	if (the_real_barf)			\
		the_real_barf(__VA_ARGS__);	\
} while (0)
#elif CONFIG_LIBARF_EXTERNAL > 0
/* libarf.so sets up $the_real_barf if/when initialized. */
# define barf(...)				\
while (the_real_barf)				\
{						\
	the_real_barf(__VA_ARGS__);		\
	break;					\
}
#endif

#ifndef __STRICT_ANSI__
/* Convenience macro to take a barf().  You can either call it
 * with empty argument list like above or with a string literal
 * or variable in which case as a bonus you'll get a colon. */
# define BARF(...)	barf((__VA_ARGS__+0) ? "%s:" : 0, ##__VA_ARGS__)
#else
/* Unless you're -ansi purist, in which case you deserve degraded service. */
# define BARF(...)	barf((__VA_ARGS__+0) ? __VA_ARGS__ ":" : 0)
#endif

#endif /* ! _LIBARF_C */
#endif /* ! _ARF_H */

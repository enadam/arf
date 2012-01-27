/*
 * libarf.c -- log the current execution backtrace on request {{{
 *
 * Linked with this library you can output the current backtrace with
 * a single function call: barf() or BARF().  If compiled so it will also
 * find, decode and print the current value of all variables visible in
 * any of the call frames.  C++ identifiers are either printed as is
 * (mangled) or in a simplified form.
 *
 * To use this library you need to:
 *   -- #include "arf.h" in your program
 *   -- place calls to barf() or BARF()
 *   -- run your program with the ./arf script
 *
 * You don't need to link with this library in compile time.  Furthermore,
 * arf.h was designed so that you can leave the library calls in your program
 * without any side-effects or noticable performance penalties.
 *
 * For more options see arf.h, the CONFIG_* definitions in this file, and the
 * ./arf script.  The self-explanatory output looks like:
 *
 * backtrace:
 *   1. prg       test_obj.c:10 main_baz()
 *   2. prg       test_obj.c:20 main_bar()
 *   3. dso.so    test_dso.c:18 dso_bar()
 *   4. dso.so    test_dso.c:27 dso_foo()
 *   5. liblib.so test_lib.c:19 lib_bar()
 *   6. liblib.so test_lib.c:28 lib_foo()
 *   7. prg       test_prg.c:21 main_foo()
 *   8. prg       test_prg.c:37 main()
 *   9. libc.so.6               [0x40044450]
 *  10. prg                     [0x8048721]
 *
 * This real-world example shows how libarf decodes variables: {{{
 * backtrace:
 *    1. hildon-desktop          hd-title-bar.c:1158          hd_title_bar_update()
 *       bar=0x8de0150
 *       priv=0x8de0218
 *       BTN_FILENAMES=0x80ca200={0x80c9971, 0x80c998a, 0x80c999e, 0x80c99b0, 0x80c99cc, 0x80c99ec, 0x80c9a05, 0x80c9a05, ...}, *BTN_FILENAMES[0]="wmLeftButtonAttached.png"
 *       BTN_LABELS=0x80ca260={(nil), (nil), (nil), (nil), (nil), (nil), (nil), (nil), ...}
 *       BTN_FLAGS=0x80c9b40={12, 12, 12, 5, 14, 12, 5, 8, ...}
 *       hd_title_bar_parent_class=0x8a5ff68
 *       signals=0x80cc2c0={165, 165, 166}
 *       the_real_barf=0xb7f6d806
 *       tried_to_find_the_real_barf=1
 *    2. hildon-desktop          hd-title-bar.c:633           hd_title_bar_set_state()
 *       bar=0x8de0150
 *    3. hildon-desktop          hd-render-manager.c:932      hd_render_manager_sync_clutter_before()
 *       priv=0x8def1b0
 *       blurred_changed=0
 *       home_front=0x8d741a0
 *       __func__=0x80c7ec0="hd_render_manager_sync_clutter_before"...
 *       __FUNCTION__=0x80c7e80="hd_render_manager_sync_clutter_before"...
 *       hd_render_manager_parent_class=0x8dc3a00
 *       the_render_manager=0x8def0c0
 *       signals=0x80cc244={164}
 *    4. hildon-desktop          hd-render-manager.c:2557     hd_render_manager_update()
 *    5. hildon-desktop          hd-task-navigator.c:3136     hd_task_navigator_remove_window()
 *       self=0x8dc4648
 *       win=0x9013328
 *       fun=0x80b0252
 *       funparam=0x90390a0
 *       li=0x8f52cb0
 *       apthumb=0x9028498
 *       newborn=(nil)
 *       closure=0x80cae20
 *       cmgrcc=0x8de8e38
 *       __FUNCTION__=0x80c92c0="hd_task_navigator_remove_window"...
 *       Grid=0x8a975c0
 *       Navigator=0x8dc4648
 *       Scroller=0x8de0008
 *       Thumbnails=0x8f2d620
 *       Notifications=(nil)
 *       NThumbnails=1
 *       Thumbsize=0x80c8b10
 *       Fly_effect_timeline=0x8dbf6a0
 *       Zoom_effect_timeline=0x8dbf6e8
 *       Fly_effect=0x8dc1740
 *       Zoom_effect=0x8dc1718
 *       Effects=(nil)
 *       LargeSystemFont=0x8de45b8, *LargeSystemFont="Nokia Sans 26"
 *       SystemFont=0x8de45f0, *SystemFont="Nokia Sans 18"
 *       SmallSystemFont=0x8de47f8, *SmallSystemFont="Nokia Sans 13"
 *       hd_task_navigator_parent_class=0x8a5ff68
 *    6. hildon-desktop          hd-switcher.c:898            hd_switcher_remove_window_actor()
 *       switcher=0x8dedd58
 *       actor=0x9013328
 *       cmgrcc=0x90390a0
 *       priv=0x8dedd68
 *       hd_switcher_parent_class=0x8a70df8
 *    7. hildon-desktop          hd-comp-mgr.c:1174           hd_comp_mgr_unregister_client()
 *       topmost=1
 *       pactor=0xb7ed1c70
 *       l=(nil)
 *       new_leader=0x1
 *       current_client=0x1
 *       app=0x9039000
 *       prev=(nil)
 *       mgr=0x8abdc88
 *       c=0x9039000
 *       actor=0x9013328
 *       priv=0x8d72ca8
 *       parent_klass=0x8d75110
 *       cclient=0x90390a0
 *       hclient=0x90390a0
 *       app=0x9026880
 *       windows=0
 *       applet=(nil)
 *       __FUNCTION__=0x80bf649="hd_comp_mgr_unregister_client"...
 *       __PRETTY_FUNCTION__=0x80bf62b="hd_comp_mgr_unregister_client"...
 *       __func__=0x80bf60d="hd_comp_mgr_unregister_client"...
 *    8. libmatchbox2-0.1.so.0   mb-wm-comp-mgr.c:281         mb_wm_comp_mgr_unregister_client()
 *       mgr=0x8abdc88
 *       client=0x9039000
 *       klass=0x8d7c7b0
 *    9. libmatchbox2-0.1.so.0   mb-window-manager.c:1322     mb_wm_unmanage_client()
 *       wm=0x8d70c18
 *       client=0x9039000
 *       destroy=1
 *   10. libmatchbox2-0.1.so.0   mb-window-manager.c:517      mb_wm_handle_unmap_notify()
 *       xev=0xbf99392c
 *       userdata=0x8d70c18
 *       wm=0x8d70c18
 *       client=0x9039000
 *       wm_klass=0x8a5f5d8
 *   11. libmatchbox2-0.1.so.0   mb-wm-main-context.c:192     call_handlers_for_event()
 *       i=0x8af6b68
 *       next=(nil)
 *       iter=0x8c7e2e8
 *       event=0xbf99392c
 *       xwin=12582943
 *   12. libmatchbox2-0.1.so.0   mb-wm-main-context.c:321     mb_wm_main_context_handle_x_event()
 *       xev=0xbf99392c
 *       ctx=0x8d72928
 *       wm=0x8d70c18
 *   13. hildon-desktop          main.c:249                   clutter_x11_event_filter()
 *       xev=0xbf99392c
 *       cev=0x8f30d80
 *       data=0x8d70c18
 *       wm=0x8d70c18
 *       hd_clutter_mutex_enabled=0
 *       hd_clutter_mutex_do_unlock_after_disabling=0
 *       hd_mb_wm=0x8d70c18
 *       hd_debug_mode_set=0
 *   14. libclutter-glx-0.8.so.0 clutter-event-x11.c:391      event_translate()
 *       format=48
 *       bytes_after=145246208
 *       ParentEmbedderWin=0
 *       event_sources=0x8a5c6a0
 *   15. libclutter-glx-0.8.so.0 clutter-event-x11.c:828      [0xb7a991b1]
 *   16. libglib-2.0.so.0                                     [0xb7497e0c]
 *   17. libglib-2.0.so.0                                     [0xb749b3c3]
 *   18. libglib-2.0.so.0                                     [0xb749b6c8]
 *   19. libgtk-x11-2.0.so.0     gtkmain.c:1200               IA__gtk_main()
 *       gtk_main_loop_level=1
 *       pre_initialized=1
 *       gtk_initialized=1
 *       current_events=(nil)
 *       main_loops=0x8de8e18
 *       init_functions=(nil)
 *       quit_functions=(nil)
 *       key_snoopers=(nil)
 *       do_setlocale=1
 *       gtk_modules_string=(nil)
 *       g_fatal_warnings=0
 *       gtk_debug_flags=0
 *       gtk_major_version=2
 *       gtk_minor_version=14
 *       gtk_micro_version=7
 *       gtk_binary_age=1407
 *       gtk_interface_age=7
 *   20. hildon-desktop          main.c:603                   main()
 *       dpy=0x8a84800
 *       wm=0x8d70c18
 *       app_mgr=0x8a82ef0
 *       __PRETTY_FUNCTION__=0x80bf3c9="main"...
 *   21. libc.so.6                                            [0xb7340dfc]
 *   }}}
 *
 * Environment:
 *   -- $ARF_MANGLED={0|1};        see ./arf -mangled
 *   -- $ARF_PRINTVARS={0|1};	   see ./arf -printvars
 *   -- $ARF_MAXPATH=<positive>:   see ./arf -maxpath=<n>
 *   -- $ARF_MAXARRAY=<unsigned>:  see ./arf -maxary=<n>
 *   -- $ARF_MAXSTRING=<unsigned>: see ./arf -maxstr=<n>
 *
 * This case $ARF_MAXARRAY
 * and $ARF_MAXSTRING how many elements of an array and how many characters
 * of a string to print of a variable.
 *
 * Limitations:
 * -- optimized leaf functions on ARM are not understood, but that's not
 *    a problem unless you're barf()ing asynchronously and you happen to
 *    be in such a leaf function
 * -- __attribute__((noreturn)) functions are evil
 * -- -fomit-frame-pointer is evil
 * -- -O2 may not show all functions
 * -- unless using libunwind only x86 and ARM are supported
 * }}}
 */
 
/* Needed for dladdr() and libelf. */
#define _GNU_SOURCE

/* Configuration: {{{
 * -- CONFIG_GLIB:	Log via g_debug().
 * -- CONFIG_STDOUT:	If !CONFIG_GLIB, then log to stdout.
 *			If neither is set print on stderr.
 * -- CONFIG_LIBUNWIND:	Use libunwind to determine the backtrace.  It's said
 *			to work well on ARM nowadays, but it's not tested yet.
 * -- CONFIG_FAST_UNWIND: Dig the stack manually, looking for frame pointers
 *			and link registers.  Faster, but may be less robust,
 *			and less reliable than backtrace() and libunwind.
 * -- CONFIG_PRINTVARS:	When printing a backtrace find and find the visible
 *			visible variables and print their current value.
 */
//#define CONFIG_GLIB
//#define CONFIG_STDOUT
//#define CONFIG_LIBUNWIND
//#define CONFIG_FAST_UNWIND
//#define CONFIG_PRINTVARS

#if !defined(__arm__) && !defined(__i386__)
# undef CONFIG_FAST_UNWIND
#endif

#ifndef CONFIG_FAST_UNWIND
# undef CONFIG_PRINTVARS
#endif
/* Configuration }}} */

/* Include files {{{ */
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#include <string.h>
#include <stdio.h>
#include <fcntl.h>

#include <link.h>
#include <dlfcn.h>
#if !defined(CONFIG_LIBUNWIND) && !defined(CONFIG_FAST_UNWIND)
# include <execinfo.h>
#endif

#include <dwarf.h>
#include <elfutils/libdw.h>

#ifdef CONFIG_GLIB
# include <glib.h>
#endif

#ifdef CONFIG_LIBUNWIND
# define UNW_LOCAL_ONLY
# include <libunwind.h>
#endif

/* Get CONFIG_LIBARF_EXTERNAL. */
#define _LIBARF_C
#include "arf.h"
/* }}} */

/* Standard definitions {{{ */
/* Default values for $Max_array and $Max_string
 * in case they are not set in the environment. */
#define DFLT_MAXARRAY		 8
#define DFLT_MAXSTRING		64
/* }}} */

/* Macros {{{ */
/* How to print the logs. */
#ifdef CONFIG_GLIB
# undef  G_LOG_DOMAIN
# define G_LOG_DOMAIN		"libarf"
# define NL			/* g_*() will add the newline */
# define WARNING		g_warning
# define LOGIT			g_debug
# define LOGITV(fmt, args)	\
	g_logv(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, fmt, args)
#else
# ifdef CONFIG_STDOUT
#  undef  stderr
#  define stderr		stdout
# endif
# define NL			"\n"
# define WARNING(fmt, ...)	fprintf(stderr, "libarf: " fmt "\n",	\
					##__VA_ARGS__)
# define LOGIT(fmt, ...)	fprintf(stderr, fmt, ##__VA_ARGS__)
# define LOGITV(fmt, args)						\
do									\
{									\
	vfprintf(stderr, fmt, args);					\
	fputc('\n', stderr);						\
} while (0)
#endif

/* Debugging */
#if 0
# include <signal.h>
# include <sys/syscall.h>
# define gettid()		(pid_t)syscall(SYS_gettid)
# define FAIL(fmt, ...)		fprintf(stderr, "%u: "fmt"\n",		\
					gettid(), __VA_ARGS__)
# define BORK(fmt, ...)							\
do									\
{									\
	FAIL(fmt, __VA_ARGS__);						\
	kill(getpid(), SIGSTOP);					\
} while (0)
# if 0
#  define DEEPSHIT		FAIL
# else
#  define DEEPSHIT(...)		/* NOP */
# endif
#else
# define FAIL(fmt, ...)		/* NOP */
# define BORK			FAIL
# define DEEPSHIT		FAIL
#endif
/* Macros }}} */

/* Type definitions {{{ */
/* State of enlarge(). */
struct bufhead_st
{
	/*
	 * -- buf:	the buffer itself, may contain a string
	 *  		or an array of anything else
	 * -- size1:	the size of one element of the data in $buf
	 * -- capacity:	the number of elements $buf can store
	 * -- n:	the number of elements $buf contains
	 */
	char *buf;
	size_t size1, capacity, n;
};

/* Linked list element describing a DSO as the run-time dynamic
 * linker knows it. */
struct dso_st
{
	/*
	 * -- id:	tells which DSO this structure belongs to
	 * -- fname:	basename of the DSO
	 * -- base:	relocation base, the location of the symbols
	 *		of this DSO starts here
	 * -- dr:	the libdw handle of this DSO
	 * -- elf:	the libelf handle of this DSO; may be read
	 *		from a different file than $dr if the debug
	 *		info is detached
	 */
	Elf *elf;
	Dwarf *dr;
	char const *id, *fname;
	void *base;
	struct dso_st *next;
};

/* Describes a call site in a DSO. */
struct callsite_st
{
	/*
	 * -- location:	"[<cufile>] [<header>[:<lineno>]]"
	 * -- scopes, nscopes: what we got from get_scopes()
	 * -- funame, cls: the caller function and optionally the class name
	 *     in .dso.  If .funame is NULL, but .cls is not, then that
	 *     contains a mangled identifier.
	 */
	struct dso_st const *dso;

	int nscopes;
	Dwarf_Die *scopes;

	char const *location;
	char const *cls, *funame;
};
/* Type definitions }}} */

/* Private variables {{{ */
#ifdef CONFIG_PRINTVARS
/* At most how many elements of an array and
 * how many characters of a string to print. */
static unsigned Max_array, Max_string;
#endif
/* }}} */

/* Program code */
/* Private functions */
/* Utilities: trim() {{{ */
/* Return the last components of $path.  By default only the last
 * component is kept but the $ARF_MAXPATH environment variable
 * can override it. */
static char const *trim(char const *path)
{
	static unsigned keep;
	int slash;
	unsigned kept, at, i;

	if (path == NULL)
		return NULL;

	/* How many path components to keep? */
	if (!keep)
	{
		char const *env;

		if (!(env = getenv("ARF_MAXPATH")))
			keep = 1;
		else if (!(keep = atoi(env)))
		{
			WARNING("$ARF_MAXPATH is invalid, ignoring");
			keep = 1;
		}
	}

	/* Work from backwards until we've $kept enough components. */
	at = 0;
	kept = 0;
	slash = 1;
	for (i = strlen(path); kept < keep; i--)
	{
		if (i == 0)
			return path;

		if (path[i-1] != '/')
			slash = 0;
		else if (!slash)
		{
			kept++;
			at = i;
			slash = 1;
		}
	}

	return &path[at];
} /* trim */
/* Utilities }}} */

/* Persistent buffer management: enlarge(), addfmt() {{{ */
/* Ensure that $bh can store $need more elements. */
static void *enlarge(struct bufhead_st *bh, size_t need)
{
	if (!bh->size1)
		/* Assume that $bh->buf contains a string. */
		bh->size1++;
	if (bh->size1 == 1)
		/* One more for the terminator. */
		need++;

	need += bh->n;
	if (need > bh->capacity)
	{	/* Out of capacity. */
		char *newbuf;

		/* Allocate some more storage in advance. */
		need += need % 64;
		if (!(newbuf = realloc(bh->buf, bh->size1*need)))
			return NULL;

		bh->buf = newbuf;
		bh->capacity = need;
	}

	return bh->buf + bh->size1*bh->n;
} /* enlarge */

/* Restores the termination of $bh to $checkpoint. */
static void rollback(struct bufhead_st *bh, size_t checkpoint)
{
	bh->buf[checkpoint] = '\0';
	bh->n = checkpoint;
} /* rollback */

/* Appends $str to $bh. */
static int addstr(struct bufhead_st *bh, char const *str)
{
	char *p;
	size_t lstr;

	lstr = strlen(str);
	if (!(p = enlarge(bh, lstr)))
		return 0;
	memcpy(p, str, lstr+1);
	bh->n += lstr;
	return 1;
} /* addstr */

/* Insertrs $str at the beginning of $bh. */
static int insstr(struct bufhead_st *bh, char const *str)
{
	size_t lstr;

	lstr = strlen(str);
	if (!enlarge(bh, lstr))
		return 0;
	memmove(&bh->buf[lstr], &bh->buf[0], bh->n+1);
	bh->n += lstr;
	memcpy(&bh->buf[0], str, lstr);
	return 1;
} /* insstr */

/* Adds a formatted string to at $bh->buf[bh->n], enlarging the buffer
 * as necessary.  If the string cannot be stored nothing is changed
 * and 0 is returned. */
static int __attribute__((format(printf, 2, 3)))
addfmt(struct bufhead_st *bh, char const *fmt, ...)
{
	size_t has, len;
	va_list printf_args, args;

	/* printf(bh, fmt) until it succeeds or cannot be enlarge()d */
	va_start(printf_args, fmt);
	for (;;)
	{
		va_copy(args, printf_args);
		has = bh->capacity - bh->n;
		len = vsnprintf(&bh->buf[bh->n], has, fmt, args);
		va_end(args);

		if (len < has)
		{
			bh->n += len;
			break;
		} else if (!enlarge(bh, len))
		{	/* Restore original termination. */
			if (bh->buf)
				bh->buf[bh->n] = '\0';
			break;
		}
	} /* until $bh is large enough */
	va_end(printf_args);

	return len < has;
} /* addfmt */
/* Buffers }}} */

/* getdso() {{{ */
/* Construct the path to a detached debug file and open it. */
static Dwarf *opendbg(struct stat const *that,
	char const *prefix, char const *dir, size_t ldir,
	char const *postfix, char const *fname)
{
	int fd;
	Dwarf *dr;
	char path[256];
	struct stat this;

	/* Construct and open $path. */
	if (prefix)
		snprintf(path, sizeof(path), "%s/%.*s/%s/%s",
			prefix, ldir, dir, postfix, fname);
	else
		snprintf(path, sizeof(path), "%.*s/%s/%s",
			ldir, dir, postfix, fname);
	if ((fd = open(path, O_RDONLY)) < 0)
		return NULL;

	/* Don't waste time on $this if it's $that. */
	if (fstat(fd, &this) < 0)
		goto out;
	if (this.st_rdev == that->st_rdev && this.st_ino == that->st_ino)
		goto out;
	/* It would be nice to verify the checksum, but it feels
	 * inappropriate in a critical path.  We're not a full-blown
	 * debugger after all. */

	/* Did we win cigar? */
	if ((dr = dwarf_begin(fd, DWARF_C_READ)) != NULL)
		return dr;

out:
	close(fd);
	return NULL;
} /* opendbg */

/* Find the detached debug information of a shared object. */
static Dwarf *finddbg(int fd, char const *dir, size_t ldir)
{
	struct stat sbuf;
	Elf *elf;
	Elf32_Ehdr *ehdr;
	Elf_Scn *scn;
	Elf32_Shdr *shdr;
	Dwarf *dr;

	/* Get the inode of $fd so we won't try to find
	 * the debug informations at the same place. */
	if (fstat(fd, &sbuf) < 0)
		return NULL;

	/* Iterate over the sections until we find .gnu_debuglink. */
	dr = NULL;
	elf = elf_begin(fd, ELF_C_READ_MMAP, NULL);
	ehdr = elf32_getehdr(elf);
	for (scn = elf_getscn(elf, 0); scn; scn = elf_nextscn(elf, scn))
	{
		char const *fname;

		shdr = elf32_getshdr(scn);
		if (shdr->sh_type != SHT_PROGBITS)
			continue;
		if (strcmp(elf_strptr(elf, ehdr->e_shstrndx, shdr->sh_name),
				".gnu_debuglink"))
			continue;

		/* .gnu_debuglink is usually just a basename.
		 * Find it in the file system.  Search in the
		 * same locations as gdb does. */
		fname = (char const *)ehdr + shdr->sh_offset;
		if ((dr = opendbg(&sbuf, NULL,
				dir, ldir, "", fname)) != NULL)
			break;
		if ((dr = opendbg(&sbuf, NULL,
				dir, ldir, ".debug", fname)) != NULL)
			break;
		if ((dr = opendbg(&sbuf, "/usr/lib/debug",
				dir, ldir, "", fname)) != NULL)
			break;
		if ((dr = opendbg(&sbuf, "/usr/local/lib/debug",
				dir, ldir, "", fname)) != NULL)
			/* gdb only looks here if its prefix is /usr/local */
			break;
		break;
	} /* for */

	elf_end(elf);
	return dr;
} /* finddbg */

/* Get a $dso which contains $addr. */
static struct dso_st const *getdso(void const *addr)
{
	static struct dso_st *seen;
	struct dso_st *dso;
	int fd;
	Dl_info info;
	struct link_map *lm;
	size_t ldir;
	char const *dir;

	/* Get which library has the code. */
	if (!dladdr1(addr, &info, (void*)&lm, RTLD_DL_LINKMAP)
		       	|| !info.dli_fname)
		return NULL;

	/* Have we seen it? */
	for (dso = seen; dso; dso = dso->next)
		if (dso->id == info.dli_fname)
			return dso;

	/* No, create new $dso. */
	if ((fd = open(info.dli_fname, O_RDONLY)) < 0)
	{
		/*
		 * If $addr belongs to an executable we couldn't find
		 * let's try to go direct (on linux).  .dli_fname seems
		 * to be the same as argv[0], which may not be the full
		 * path to the executable.
		 */
		if (errno != ENOENT)
			return NULL;
		if (lm->l_name[0])
			return NULL;
		if ((fd = open("/proc/self/exe", O_RDONLY)) < 0)
			return NULL;
	}
	if (!(dso = malloc(sizeof(*dso))))
	{
		close(fd);
		return NULL;
	}

	/* Let $dir denote the container directory of dso->fname.
	 * Needed when the debug information is in a separate file. */
	dso->id = info.dli_fname;
	if ((dso->fname = strrchr(dso->id, '/')) != NULL)
	{
		dir = dso->id;
		ldir = dso->fname - dso->id;
		dso->fname++;
	} else
	{
		dir = ".";
		ldir = 1;
		dso->fname = dso->id;
	}

	/* Initialize libelf and libdwarf.  The $dso is useful to some
	 * extent even if it fails. */
	elf_version(EV_CURRENT);
	if (!(dso->elf = elf_begin(fd, ELF_C_READ_MMAP, NULL)))
		close(fd);
	else if (!(dso->dr = dwarf_begin_elf(dso->elf, DWARF_C_READ, NULL)))
		dso->dr = finddbg(fd, dir, ldir);

	/* Is this symbol defined in the executable?
	 * Then don't subtract the base address. */ 
	dso->base = lm->l_name[0] ? info.dli_fbase : 0;

	dso->next = seen;
	seen      = dso;

	return dso;
} /* getdso */
/* }}} */

/* addr_is() {{{ */
/* "Segment" means "mapped region" here.
 * Each segment_st corresponds to a line in /proc/self/maps.
 * The segment $type is determined by heuristics. */
enum segment_type_t { CODE, STACK, HEAP, DATA, OTHER };
struct segment_st
{
		void const *start, *end;
		enum segment_type_t type;
};

/* Comparator function for bsearch() to tell whether an $addr
 * is in a $segment. */
static int find_segment(void const *addr, struct segment_st const *segment)
{
	if (addr < segment->start)
		return -1;
	if (addr >= segment->end)
		return +1;
	else
		return 0;
} /* find_segment */

/*
 * Try to tell what $addr points to: some CODE, somewhere in the STACK,
 * HEAP or to some DATA.  Returns OTHER if no idea.  If you supply $dso
 * its ELF section headers are searches first.  If that fails addr_is()
 * goes to see /proc/self/maps and tries to guess what $addr points to
 * by looking at the containing memory map entry's file backing and
 * write/execute bits.
 */
static enum segment_type_t addr_is(struct dso_st const *dso,
	void const *addr, void const **segp)
{
	int found;
	FILE *maps;
	char line[128];
	struct segment_st *segment;
	static struct bufhead_st segments = { .size1 = sizeof(*segment) };

	if (addr < (void *)4096)
	{
		/* Must be an error somewhere, nothing is supposed to be
		 * mapped at such a low address. */
		FAIL("LOW ADDR %p", addr);
		return OTHER;
	}

	if (dso)
	{	/* Search the ELF section headers. */
		Elf_Scn *scn;
		Elf32_Shdr *shdr;
		Elf32_Ehdr *ehdr;

		/* As though they appear so the ELF specs don't mandate
		 * that the SHT is ordered by address, so it's safer not
		 * to take a shortcut. */
		ehdr = elf32_getehdr(dso->elf);
		for (scn = elf_getscn(dso->elf, 0); scn;
			scn = elf_nextscn(dso->elf, scn))
		{
			shdr = elf32_getshdr(scn);

			/* Is $addr in $scn?
			 * (What about overlapping sections?) */
			if (!(dso->base+shdr->sh_addr <= addr))
				continue;
			if (!(addr < dso->base+shdr->sh_addr+shdr->sh_size))
				continue;

			/* Do we understand $scn?
			 * If not resort to /proc/self/maps. */
			if (!(shdr->sh_type == SHT_PROGBITS
					|| shdr->sh_type == SHT_NOBITS))
				break;
			if (!(shdr->sh_flags & SHF_ALLOC))
				break;

			/* Okay now we have enough confidence to say
			 * anything about $scn. */
			if (segp != NULL)
				*segp = dso->base+shdr->sh_addr+shdr->sh_size;
			return (shdr->sh_flags & SHF_EXECINSTR)
				? CODE : DATA;
		} /* for all sections */
	} /* if we know $dso */

	/* Search the cache for $addr first. */
	segment = bsearch(addr,
		segments.buf, segments.n, sizeof(*segment),
		(int (*)(void const *, void const *))find_segment);
	if (segment)
	{
		if (segp != NULL)
			*segp = segment->end;
		return segment->type;
	}

	/* Reload $segments and recheck $addr.
	 * Assume that $maps is sorted by address. */
	if (segments.n)
		FAIL("RELOAD %p", addr);
	found = OTHER;
	segments.n = 0;
	maps = fopen("/proc/self/maps", "r");
	while (fgets(line, sizeof(line), maps))
	{
		int n, nn;
		unsigned inode;

		if (!(segment = enlarge(&segments, segments.n+1)))
			break;
		if (sscanf(line, "%p-%p %n", &segment->start, &segment->end,
				&n) < 2)
			continue;

		/* Parse the $line. */
		if (sscanf(&line[n], "rw%*c%*c %*x 00:00 %u %n",
				&inode, &nn) > 0
			&& !inode)
		{
			n += nn;
			if (!line[n] || line[n] == '\n')
				/* Anonymous page, but threads may
				 * use them as stack. */
				segment->type = STACK;
			else if (!strcmp(&line[n], "[stack]\n"))
				segment->type = STACK;
			else if (!strcmp(&line[n], "[heap]\n"))
				segment->type = HEAP;
			else
				segment->type = OTHER;
		} else if (sscanf(&line[n], "%*c%*cx%*c %*x %*x:%*x %u %n",
					&inode, &nn) > 0
				&& (inode || !strcmp(&line[n+nn], "[vdso]\n")))
			segment->type = CODE;
		else
			segment->type = OTHER;

		DEEPSHIT("%p-%p %d", segment->start, segment->end,
			segment->type);
		if (segment->start <= addr
			&& addr < segment->end)
		{
			if (segp != NULL)
				*segp = segment->end;
			found = segment->type;
		}
		segments.n++;
	} /* for $maps lines */
	fclose(maps);

	return found;
} /* addr_is */
/* addr_is }}} */

/* printvars() {{{ */
#ifdef CONFIG_PRINTVARS
/* Return whether it has seen $addr.  If not, remember it.
 * If $addr is NULL forget everything.  This is used to avoid
 * printing the same variable more than once during a backtrace. */
static int seen(void const *addr)
{
	static struct bufhead_st bh = { .size1 = sizeof(addr) };
	unsigned l, m, r;
	void const **store;

	if (!addr)
	{	/* Reset $store. */
		bh.n = 0;
		return 1;
	} else if (!bh.n)
	{	/* Insert the first element. */
		m = 0;
		goto insert;
	}

	/* Try to find $addr in $store.  If not found set $m to the place
	 * it should have been.  $addr is searched in $store[$l..$r],
	 * and the interval is halfed in every iteration. */
	l = 0;
	r = bh.n - 1;
	store = (void const **)bh.buf;
	for (;;)
	{
		/* Examine the boundaries of the interval. */
		/* l, ..., r */
		if (addr == store[l] || addr == store[r])
			return 1;
		else if (addr < store[l])
		{	/* addr, l, ..., r */
			m = l;
			break;
		} else if (addr > store[r])
		{	/* l, ..., r, addr */
			m = r+1;
			break;
		} else if (r-l == 1)
		{	/* l, addr, r */
			m = r;
			break;
		}

		/* Halve the interval and decide where $addr is. */
		m = l+(r-l)/2;
		if (addr == store[m])
		{	/* l, ..., addr, ..., r */
			return 1;
		} else if (addr < store[m])
		{	/* l, ..., addr, ..., m */
			l++;
			if (m == l)
				break;
			r = m-1;
		} else /* addr > store[m] */
		{	/* m, ..., addr, .., r */
			m++;
			if (r == m)
				break;
			l = m;
			r--;
		}
	} /* for */

insert:	/* $addr not found, insert at $store[$m]. */
	if (enlarge(&bh, bh.n+1))
	{
		store = (void const **)bh.buf;
		memmove(&store[m+1], &store[m],
			sizeof(*store) * (bh.n-m));
		store[m] = addr;
		bh.n++;
	}

	return 0;
} /* seen */

/*
 * Depending on $encoding decode the  integer, float or character
 * found at $addr and append it to $line in textual representation.
 * Returns 0 if failed, either because out of memory or becase we
 * couldn't decode the data.
 */
static int print_basic(struct bufhead_st *line, void const *addr,
	Dwarf_Word encoding, Dwarf_Word size)
{
	switch (encoding)
	{
	case DW_ATE_float:
		switch (size)
		{
		case sizeof(float):
			return addfmt(line, "%f", *(float *)addr);
		case sizeof(double):
			return addfmt(line, "%f", *(double *)addr);
#ifndef __arm__	/* Appears to be the same as double on ARMv7. */
		case sizeof(long double):
			return addfmt(line, "%Lf", *(long double *)addr);
#endif
		}
		break;
	case DW_ATE_address:
		switch (size)
		{
		case sizeof(u_int8_t):
			return addfmt(line, "0x%.8x", *(u_int8_t *)addr);
		case sizeof(u_int16_t):
			return addfmt(line, "0x%.8x", *(u_int16_t *)addr);
		case sizeof(u_int32_t):
			return addfmt(line, "0x%.8x", *(u_int32_t *)addr);
		case sizeof(u_int64_t):
			return addfmt(line, "0x%.16llx", *(u_int64_t *)addr);
		}
		break;
	case DW_ATE_boolean:
	{
		int tf;

		switch (size)
		{
		case sizeof(u_int8_t):
			tf = *(int8_t *)addr != 0;
			break;
		case sizeof(u_int16_t):
			tf = *(int16_t *)addr != 0;
			break;
		case sizeof(u_int32_t):
			tf = *(int32_t *)addr != 0;
			break;
		case sizeof(u_int64_t):
			tf = *(int64_t *)addr != 0;
			break;
		}
		return addfmt(line, "%s", tf ? "true" : "false");
	} /* boolean */
	case DW_ATE_signed:
		switch (size)
		{
		case sizeof(int8_t):
			return addfmt(line, "%d", *(int8_t *)addr);
		case sizeof(int16_t):
			return addfmt(line, "%d", *(int16_t *)addr);
		case sizeof(int32_t):
			return addfmt(line, "%d", *(int32_t *)addr);
		case sizeof(int64_t):
			return addfmt(line, "%lld", *(int64_t *)addr);
		}
		break;
	case DW_ATE_unsigned:
		switch (size)
		{
		case sizeof(u_int8_t):
			return addfmt(line, "%u", *(u_int8_t *)addr);
		case sizeof(u_int16_t):
			return addfmt(line, "%u", *(u_int16_t *)addr);
		case sizeof(u_int32_t):
			return addfmt(line, "%u", *(u_int32_t *)addr);
		case sizeof(u_int64_t):
			return addfmt(line, "%llu", *(u_int64_t *)addr);
		}
		break;
	case DW_ATE_signed_char:
		if (size == 1)
			return addfmt(line, "'%c'", *(char *)addr);
		break;
	case DW_ATE_unsigned_char:
		if (size == 1)
			return addfmt(line, "0x%.2x", *(char *)addr);
		break;
	} /* switch basic type */

	return 0;
} /* print_basic */

/* Append the textual representation of a pointer
 * or an array of pointers to $line. */
static int print_pointer(
	struct bufhead_st *line, struct bufhead_st const *name,
	void const *addr, int waspointer, int isarray, Dwarf_Word nelems)
{
	unsigned i;
	size_t checkpoint;

	checkpoint = line->n;
	if (!addfmt(line, isarray ? "%s%s=%p={" : "%s%s=",
			waspointer ? ", " : "", name->buf, addr))
		return 0;
	if (!addfmt(line, "%p", ((void **)addr)[0]))
	{
		rollback(line, checkpoint);
		return 0;
	}
	for (i = 1; i < nelems && i < Max_array; i++)
		if (!addfmt(line, ", %p", ((void **)addr)[i]))
			break;
	return !isarray || addstr(line, i < nelems ? ", ...}" : "}");
} /* print_pointer */

/*
 * Desired output format: {{{
 * basic:
 *	int  akarmi
 *	     akarmi
 *  akarmi=10
 * array, basic:
 *	int  akarmi[5]
 *	     akarmi
 *  akarmi=0x1234={10,20,30}
 * array, array, basic:
 *	int  akarmi[5][5]
 *	     akarmi[0]
 *  akarmi[0]=0x1234={10,20,30}
 *
 * pointer, basic:		  !waspointer && !isarray
 *	int *akarmi
 *	    *akarmi
 *  akarmi=0x1234, *akarmi=10
 * pointer, array, basic:
 *	int (*akarmi)[5]
 *	     *akarmi
 *  akarmi=0x1234, *akarmi=0x4321={10,20,30}
 * pointer, array, array, basic:
 *	int (*akarmi)[5][5]
 *	    (*akarmi)[0]
 *  akarmi=0x1234, (*akarmi)[0]=0x4321={10,20,30}
 *
 * array, pointer, basic:	  !waspointer &&  isarray
 *	int *akarmi[5]
 *	    *akarmi[0]
 *  akarmi=0x1234, *akarmi[0]=10
 * array, pointer, array, basic:
 *      int (*akarmi[5])[5]
 *	     *akarmi[0]
 *  akarmi=0x1234, *akarmi[0]=0x4321={10,20,30}
 * array, pointer, array, array, basic:
 *      int (*akarmi[5])[5][5]
 *	    (*akarmi[0])[0]
 *  akarmi=0x1234, (*akarmi[0])[0]=0x4321={10,20,30}
 *
 * pointer, pointer, basic:	   waspointer && !isarray
 *	int **akarmi
 *	    **akarmi
 *  akarmi=0x1234, *akarmi=0x4321, **akarmi=10
 * pointer, pointer, array, basic:
 *	int (**akarmi)[5]
 *	     **akarmi
 *  akarmi=0x1234, *akarmi=0x4321, **akarmi=0x1441={10,20,30}
 * pointer, pointer, array, array, basic:
 *	int (**akarmi)[5][5]
 *	    (**akarmi)[0]
 *  akarmi=0x1234, *akarmi=0x4321, (**akarmi)[0]=0x1441={10,20,30}
 *
 * pointer, array, pointer, basic: waspointer &&  isarray
 *	int *(*akarmi)[5]
 *	    *(*akarmi)[0]
 *  akarmi=0x1234, *akarmi=0x4321, *(*akarmi)[0]=10
 * pointer, array, pointer, array, basic:
 *	int (*(*akarmi)[5])[5]
 *	     *(*akarmi)[0]
 *  akarmi=0x1234, *akarmi=0x4321, *(*akarmi)[0]=0x1441={10,20,30}
 * }}}
 */
static char const *decodevar(Dwarf_Die *var, char const *id,
	void const *addr, struct dso_st const *dso)
{
	static struct bufhead_st name, line;
	unsigned i;
	Dwarf_Die type;
	void const *seg;
	size_t checkpoint;;
	int waspointer, isarray;
	Dwarf_Word encoding, size, nelems;

	/* Initialize $name and $line. */
	name.n = line.n = 0;
	if (!addstr(&name, id))
		goto done;

	/* Dereference $addr until its $type is something basic we can
	 * decode and print in `basic'. */
	type = *var;
	seg = NULL;
	waspointer = isarray = 0;
	nelems  = 1;
	for (;;)
	{
		Dwarf_Attribute attr;

		/*
		 * $addr is an address and it always points to data,
		 * which can be a pointer, which will become $addr
		 * in the next iteration.  See what kind of data
		 * does $addr point to.
		 */
		if (!dwarf_attr(&type, DW_AT_type, &attr))
			goto done;
		if (dwarf_whatform(&attr) != DW_FORM_ref4)
			goto done;
		dwarf_formref_die(&attr, &type);

		switch (dwarf_tag(&type))
		{
		case DW_TAG_typedef:
		case DW_TAG_const_type:
		case DW_TAG_volatile_type:
			/* Simply ignore qualifiers and aliases. */
			continue;
		default: /* Unrecognized. */
			goto done;

		/* These are the only data type we're concerned with. */
		case DW_TAG_array_type:
		{	/* $addr points to an array of somethings. */
			Dwarf_Die range;

			/* If we have an array of array of somethings
			 * denote that we will only print the first
			 * element of the preceeding array. */
			if (isarray)
			{
				if (waspointer)
				{	// *akarmi => (*akarmi)[0]
					if (!insstr(&name, "("))
						goto done;
					if (!addstr(&name, ")"))
						goto done;
				} // else   akarmi =>   akarmi[0]
				if (!addstr(&name, "[0]"))
					goto done;
			}

			/* Figure out the number of elements in the array. */
			isarray = 1;
			nelems  = 1;
			if (dwarf_child(&type, &range) != 0)
				continue;
			if (dwarf_tag(&range) != DW_TAG_subrange_type)
				continue;
			if (!dwarf_attr(&range, DW_AT_upper_bound, &attr))
				continue;
			dwarf_formudata(&attr, &nelems);
			nelems++;
			continue;
		} /* $addr points to an array */

		case DW_TAG_pointer_type:
		case DW_TAG_base_type:
			/* $addr points to a pointer or basic type data */
			if (!dwarf_attr(&type, DW_AT_byte_size, &attr))
				goto done;
			dwarf_formudata(&attr, &size);
			break;
		} /* switch data type */

		/* Validate $addr if it was derived from a pointer.
		 * Otherwise we can assume the compiler placed the
		 * data in some allocated area. */
		if (waspointer)
		{
			switch (addr_is(dso, addr, &seg))
			{
			case STACK:
			case HEAP:
			case DATA:
				if (addr + size*nelems < seg)
					break;
				/* $nelems may be too large,
				 * try to reduce it. */
				if ((nelems = (seg - addr) / size) > 0)
					break;
				/* Not even 1 $nelems of this $size
				 * can fit in $seg. */
			default:
				goto done;
			}
		} /* if $addr was a pointer or array */

		/* See what to do about $addr. */
		switch (dwarf_tag(&type))
		{
		case DW_TAG_base_type:
			/* Decode and print what $addr points to. */
			dwarf_attr(&type, DW_AT_encoding, &attr);
			dwarf_formudata(&attr, &encoding);
			goto basic;
		case DW_TAG_pointer_type:
			if (size != sizeof(void *))
			{	/* Non-word-size pointer. */
				encoding = DW_ATE_address;
				goto basic;
			}

			/* Print the pointer(s). */
			if (!print_pointer(&line, &name, addr,
					waspointer, isarray, nelems))
				goto done;

			/* Transform $name such that it can be used
			 * for basic type data. */
			if (!isarray || !waspointer)
			{
				if (!insstr(&name, "*"))
					goto done;
				if (isarray && !addstr(&name, "[0]"))
					goto done;
			} else	/* isarray && waspointer */
			{	// *akarmi => *(*akarmi)[0]
				if (!insstr(&name, "*("))
					goto done;
				if (!addstr(&name, ")[0]"))
					goto done;
			}

			addr = *(void **)addr;
			waspointer = 1;
			isarray = 0;
			nelems  = 1;
			break;
		} /* switch */
	} /* for indirection levels */

basic:	/* Decode and print what $addr points to. */
	checkpoint = line.n;
	if ((encoding == DW_ATE_signed_char
			|| encoding == DW_ATE_unsigned_char)
		&& (waspointer || isarray))
	{	/* String ("hihihi") or byte array (0x102030) */
		int isbinary;
		unsigned len;
		char const *str;

		if (!addfmt(&line, isarray ? "%s%s=%p=" : "%s%s=",
				waspointer ? ", " : "", name.buf, addr))
			goto done;

		/* Determine $len. */
		str = addr;
		isbinary = 0;
		for (len = 0; ; len++)
		{
			if (waspointer && &str[len] >= (char *)seg)
				break;
			if (isarray && len >= nelems)
				break;
			if (len >= Max_string || !str[len])
				break;
			if (!isprint(str[len]))
			{
				if (len >= Max_array)
					/* Print as $len-length string. */
					break;
				isbinary = 1;
				if (!isarray)
					break;
				len = nelems <= Max_array
					? nelems : Max_array;
				break;
			}
		} /* for characters of $str */

		if (isbinary)
		{
			addstr(&line, "0x");
			for (i = 0; i < len; i++)
				addfmt(&line, "%.2x",
					(unsigned char)str[i]);
			if (isarray && nelems > len)
				addstr(&line, "...");
		} else if (isarray && nelems > len)
			addfmt(&line, "\"%.*s\"...", len, str);
		else
			addfmt(&line, "\"%.*s\"",    len, str);
	} else	/* (Arrays of) Integers or floats, */
	{	/* or single characters or bytes.  */
		if (!addfmt(&line, isarray ? "%s%s=%p={" : "%s%s=",
				waspointer ? ", " : "", name.buf, addr))
			goto done;
		if (!print_basic(&line, addr, encoding, size))
		{	/* Either we don't know how to print $encoding/$size
			 * or we are out of memory. */
			rollback(&line, checkpoint);
			goto done;
		}

		for (i = 1; i < nelems && i < Max_array; i++, addr += size)
		{
			checkpoint = line.n;
			if (!addstr(&line, ", "))
				break;
			if (!print_basic(&line, addr, encoding, size))
			{
				rollback(&line, checkpoint);
				break;
			}
		} /* for nelems */
		if (isarray)
			addstr(&line, i < nelems ? ", ...}" : "}");
	}
done:	return line.n ? line.buf : NULL;
} /* decodevar */

/* Determine the address of $var, decode and print its value. */
static void printvar(Dwarf_Die *var, struct dso_st const *dso,
	void const *pc, void const *fp)
{
	size_t nloc;
	Dwarf_Op *loc;
	void const *addr;
	Dwarf_Attribute attr;
	char const *str;

	/* Set $Max_array and $Max_string from the environment
	 * if we haven't. */
	if (!Max_array)
	{
		char const *env;

		Max_array  = (env = getenv("ARF_MAXARRAY"))  != NULL
			? atoi(env) : DFLT_MAXARRAY;
		Max_string = (env = getenv("ARF_MAXSTRING")) != NULL
			? atoi(env) : DFLT_MAXSTRING;
	}

	/* Determine the address of the variable. */
	if (!dwarf_attr_integrate(var, DW_AT_location, &attr))
		return;
	if (dwarf_getlocation_addr(&attr, pc-dso->base, &loc, &nloc, 1) != 1)
		return;
	if (nloc != 1)
		return;

	/* 3 is the charm and 1 is the free giveaway. */
#ifdef __i386__
	fp += 8;
#endif
#ifdef __arm__
	fp += 4;
#endif

	if (loc->atom == DW_OP_fbreg)
		addr = fp        + loc->number;
	else if (loc->atom == DW_OP_addr)
		addr = dso->base + loc->number;
	else	/* We don't track registers. */
		return;

	/* Decode and print it unless we've seen it in this backtrace. */
	if (seen(addr))
		return;
	if ((str = decodevar(var, dwarf_diename(var), addr, dso)) != NULL)
		LOGIT("      %s" NL, str);
} /* printvar */

/* Find all variables visible in $fun at $pc and print them. */
static void printvars(Dwarf_Die *fun, struct dso_st const *dso,
	void const *pc, void const *fp)
{
	Dwarf_Die var;

	if (dwarf_child(fun, &var) != 0)
		return;
	do
	{
		switch (dwarf_tag(&var))
		{
		case DW_TAG_variable:
		case DW_TAG_formal_parameter:
			printvar(&var, dso, pc, fp);
			break;
		case DW_TAG_lexical_block:
			printvars(&var, dso, pc, fp);
			break;
		}
	} while (dwarf_siblingof(&var, &var) == 0);
} /* printvars */
#endif /* CONFIG_PRINTVARS */
/* printvars() }}} */

/* The engine: getting debug information about stack frames {{{ */
/* Search $parent's children recursively for subroutines containing $pc.
 * This may find subprograms dwarf_getscopes() wouldn't in case the
 * subprogram is nested in another one by scope but not by pc range. */
static int search_scopes(Dwarf_Die **scopesp, int *nscopesp,
	Dwarf_Die *parent, Dwarf_Addr pc)
{
	unsigned depth;
	Dwarf_Die child;

	if (dwarf_child(parent, &child) != 0)
		return 0;
	depth = (*nscopesp)++;

	do
	{
		Dwarf_Addr addr;
		Dwarf_Attribute attr;

		if (dwarf_tag(&child) != DW_TAG_subprogram)
			continue;

		if (search_scopes(scopesp, nscopesp, &child, pc))
		{
			if (*scopesp)
				(*scopesp)[depth] = *parent;
			return 1;
		}

		if (!dwarf_attr(&child, DW_AT_low_pc, &attr))
			continue;
		if (dwarf_formaddr(&attr, &addr) != 0)
			continue;
		if (pc < addr)
			continue;

		if (!dwarf_attr(&child, DW_AT_high_pc, &attr))
			continue;
		if (dwarf_formaddr(&attr, &addr) != 0)
			continue;
		if (pc > addr)
			continue;

		*scopesp = malloc(sizeof(**scopesp) * *nscopesp);
		if (*scopesp)
			(*scopesp)[depth] = child;
		return 1;
	} while (dwarf_siblingof(&child, &child) == 0);
	(*nscopesp)--;

	return 0;
} /* search_scopes */

/* Like dwarf_getscopes(), but if that fails try search_scopes(). */
static int get_scopes(Dwarf_Die *die, Dwarf_Addr pc, Dwarf_Die **scopesp)
{
	int n;

	/* Reset libdw's errno first, because getsopes() may fail
	 * in case of earlier errors. */
	dwarf_errno();
	*scopesp = NULL;
	if ((n = dwarf_getscopes(die, pc, scopesp)) > 0)
		return n;
	else if (search_scopes(scopesp, &n, die, pc) && *scopesp)
		return n;
	else
		return 0;
} /* get_scopes */

/* Fill $cs for a $relpc relocated program counter. */
static void bt1(struct callsite_st *cs, void const *relpc)
{
	static struct bufhead_st bh;
	int i, lineno;
	Dwarf_Die die;
	Dwarf_Addr pc;
	Dwarf_Line *line;
	char const *fmt, *cufile, *header;

	memset(cs, 0, sizeof(*cs));
	if (!(cs->dso = getdso(relpc)))
		return;
	if (!cs->dso->dr)
		return;

	/* pc := relocation-free $relpc. */
	pc = relpc-cs->dso->base;
	if (!dwarf_addrdie(cs->dso->dr, pc, &die))
		return;

	/* Get the name of the caller function and the name of the
	 * compilation unit (the C source file). */
	cufile = NULL;
	cs->nscopes = get_scopes(&die, pc, &cs->scopes);
	for (i = 0; i < cs->nscopes; i++)
	{
		int tag;
		char const *str;
		Dwarf_Attribute attr;
		Dwarf_Die spec;


		tag = dwarf_tag(&cs->scopes[i]);
		if (tag != DW_TAG_subprogram && tag != DW_TAG_compile_unit)
			continue;
		if (dwarf_attr(&cs->scopes[i], DW_AT_name, &attr))
			str = dwarf_formstring(&attr);
		else if (tag == DW_TAG_subprogram
			&& dwarf_attr(&cs->scopes[i],
				DW_AT_specification, &attr))
		{
			static int leave_mangled = -1;

			/* Looks like it's an object method.
			 * Find out the class name too. */
			if (!dwarf_formref_die(&attr, &spec))
				continue;
			if (dwarf_tag(&spec) != DW_TAG_subprogram)
				continue;

			/* Print the mangled identifier? */
			if (leave_mangled < 0)
			{
				char const *env;

				env = getenv("ARF_MANGLED");
				leave_mangled = env && atoi(env) > 0;
			}

			if (leave_mangled && dwarf_attr(&spec,
				DW_AT_MIPS_linkage_name, &attr))
			{	/* Leave cs->funame empty to indicate
				 * mangled name to bt(). */
				cs->cls = dwarf_formstring(&attr);
				continue;
			} else if (dwarf_attr(&spec, DW_AT_name, &attr))
			{
				Dwarf_Die *dies;

				/* Look up the class, which should be
				 * in the specification's parent die. */
				dies = NULL;
				str = dwarf_formstring(&attr);
				if (dwarf_getscopes_die(&spec, &dies) > 1
						&& dwarf_attr(&dies[1],
							DW_AT_name, &attr))
					cs->cls = dwarf_formstring(&attr);
				free(dies);
			}
		}

		if (tag == DW_TAG_subprogram)
			cs->funame = str;
		else
			cufile = trim(str);
	}

	/*
	 * Get $header and $lineno (the file and line address which
	 * actually made the call).  $header may be different from
	 * $cufile if the call was actually made in a function
	 * defined in a header.
	 */
	line = dwarf_getsrc_die(&die, pc-1);
	header = trim(dwarf_linesrc(line, NULL, NULL));
	if (cufile && header && !strcmp(cufile, header))
		cufile = NULL;
	if (!cufile && !header)
		return;
	if (!header || dwarf_lineno(line, &lineno) != 0 || lineno <= 0)
		lineno = 0;

	/* Add $cufile and $header to $bh->buf then return them
	 * as $cs->location. */
	if (cufile && header && lineno)
		fmt = "%s %s:%u";
	else if (cufile && header)
		fmt = "%s %s";
	else if (cufile)
		fmt = "%s";
	else if (header && lineno)
		fmt = "%.0s%s:%u";
	else /* header */
		fmt = "%.0s%s";

	bh.n = 0;
	addfmt(&bh, fmt, cufile, header, lineno);
	cs->location = bh.buf;
} /* bt1 */

/* Print information about the i:th frame, which is executing $pc. */
static void bt0(unsigned i, void const *pc, void const *fp)
{
	static unsigned wcol1, wcol2;
	char const *fmt;
	struct callsite_st cs;
	int st1, ed1, st2, ed2, len;

	if (i < 1)
		return;

	/*
	 * Align cs.dso->fname, .location and .funame in columns.
	 * The olumns are $wcol[12] chararacter wide.  If the columns
	 * are wider than that update them so the subsequent lines
	 * will be aligned at least.
	 */
	bt1(&cs, pc);
	if (!cs.funame && !cs.cls)
		fmt = "%4d. %n%*s%n %n%*s%n [%.0s%.0s%p]" NL;
	else if (cs.funame && !cs.cls)
		fmt = "%4d. %n%*s%n %n%*s%n %.0s%s()"     NL;
	else if (!cs.funame && cs.cls)
		/* c++filt adds "()" itself */
		fmt = "%4d. %n%*s%n %n%*s%n %s"           NL;
	else
		fmt = "%4d. %n%*s%n %n%*s%n %s::%s()"     NL;
	LOGIT(fmt, i,
		&st1, -wcol1, cs.dso && cs.dso->fname
			? cs.dso->fname : "", &ed1,
		&st2, -wcol2, cs.location ? : "", &ed2,
		cs.cls, cs.funame, pc);
	if (wcol1 < (len = ed1 - st1))
		wcol1 = len;
	if (wcol2 < (len = ed2 - st2))
		wcol2 = len;

	if (cs.nscopes > 0)
	{
#ifdef CONFIG_PRINTVARS
		static int wantsvars = -1;
		char const *env;

		if (wantsvars < 0)
			wantsvars = (env = getenv("ARF_PRINTVARS"))
				&& atoi(env) > 0;
		if (wantsvars && fp != NULL)
			for (i = 0; i < cs.nscopes; i++)
				printvars(&cs.scopes[i], cs.dso, pc, fp);
#endif
		free(cs.scopes);
	}
} /* bt0 */
/* Engine }}} */

/* The driver: getting the return addresses {{{ */
#ifdef CONFIG_FAST_UNWIND /* {{{ */
/* Unwind the stack by examining the frame manually. */

#ifdef __arm__
/* These are used by getlr() to examine instructions. */
# undef R12
# define PUSH			0xE92D0000
# define FP			(1 << 11)
# define R12			(1 << 12)
# define SP			(1 << 13)
# define LR			(1 << 14)
# define PC			(1 << 15)
#endif

/* Extract the return address from the frame pointed to by $fp.
 * $prev_ssegp is used for state recording during unwinding and
 * should point to NULL on the first invocation. */
static void const *const *getlr(void const *const *fp, void const **lrp,
	void const **prev_ssegp)
{
	void const *sseg;
	void const *const *next;

	DEEPSHIT("FP=%p", fp);
	if (fp == NULL)
	{	/* Caller should have stopped unwinding. */
		FAIL("%p: NULL", fp);
		return NULL;
	}

#if defined(__arm__) /* {{{ */
	if (addr_is(NULL, fp[0], NULL) != CODE)
	{	/* *fp must be either lr or pc, we're probably in
		 * a noreturn function like _dl_signal_error(). */
		FAIL("%p: NOT CODE: %p", fp, fp[0]);
		return NULL;
	}

	switch (addr_is(NULL, fp[-1], &sseg))
	{
	case STACK:
	{	/* fp|sp, lr */
		/* Verify that fp|sp is in the same segment as before.
		 * This is used to detect clone() boundaries. */
		if (!*prev_ssegp)
			*prev_ssegp = sseg;
		else if (*prev_ssegp != sseg)
		{
			FAIL("%p: DIFFERENT SSEG", fp);
			return NULL;
		}

		*lrp = fp[0];
		if (fp[-1] != fp+1)
			/* fp,     lr */
			next = fp[-1];
		else	/* fp, sp, lr */
			next = fp[-2];
		break;
	}

	case CODE:
	{	/* fp, [r12, sp], lr, pc */
		unsigned insn, i;

		*lrp = fp[-1];

		/*
		 * *fp points to pc.  If the registers were push:ed
		 * *$pc must be a PUSH instruction.  Examine it to see
		 * what registers were PUSH:ed to determine where the
		 * $next fp is.
		 */
		insn = *(unsigned *)(fp[0]-8);
		if ((insn & 0xFFFF0000) != PUSH)
		{	/* $insn is not a PUSH instruction. */
			FAIL("%p: NOT PUSH: *%p=0x%x", fp, fp[0]-8, insn);
			return NULL;
		} else if ((insn & (FP|LR|PC)) != (FP|LR|PC))
		{	/* Either fp, lr or pc wasn't pushed. */
			FAIL("%p: NOT EXPECTED PUSH: *%p=0x%x",
				fp, fp[0]-8, insn);
			return NULL;
		}

		i = -2;
		if (insn & SP)
			i--;
		if (insn & R12)
			i--;
		next = fp[i];
		break;
	}

	default:
		if (fp[-1] != NULL)
			FAIL("%p: OTHER: %p", fp, fp[-1]);
		return NULL;
	} /* switch what fp[-1] is */
	/* }}} */
#elif defined(__i386__) /* {{{ */
	/* Assume ip, prev_bp (<- bp). */
	if (!fp[0])
	{	/* Reached the bottom. */
		return NULL;
	} else if (addr_is(NULL, fp[0], &sseg) != STACK)
	{
		FAIL("%p: NOT STACK: %p", fp, fp[0]);
		return NULL;
	} else if (!*prev_ssegp)
	{
		*prev_ssegp = sseg;
	} else if (*prev_ssegp != sseg)
	{
		FAIL("%p: DIFFERENT SSEG: %p != %p", fp, sseg, *prev_ssegp);
		return NULL;
	}

	if (addr_is(NULL, fp[1], NULL) != CODE)
	{
		FAIL("%p: NOT CODE: %p", fp, fp[1]);
		return NULL;
	} else
	{
		next = fp[0];
		*lrp = fp[1];
	}
#endif /* __i386__ }}} */

	/* Is $next valid after all? */
	DEEPSHIT("NEXT=%p", next);
	if (__builtin_expect(next == fp, 0))
	{	/* Something fishy is going on. */
		FAIL("%p: SAME FRAME", fp);
		return NULL;
	} else if (__builtin_expect(next < fp, 0))
	{	/* The stack is supposed to _grow_ down. */
		FAIL("%p: WRONG DIRECTION", fp);
		return NULL;
	} else
		return next;
} /* getlr */
#endif /* CONFIG_FAST_UNWIND }}} */

/* Interface functions */
/* Find and process the frames one by one starting from the most recent. */
#if CONFIG_LIBARF_EXTERNAL
static /* barf() will be made available by init() */
#endif
void barf(char const *why, ...)
{
	/* Print $why. */
	if (why)
	{
		va_list printf_args;

		va_start(printf_args, why);
		LOGITV(why, printf_args);
		va_end(printf_args);
	} else
		LOGIT("backtrace:" NL);

	do
	{
#if defined(CONFIG_LIBUNWIND) /* {{{ */
		/* This is an untested method. */
		unsigned i;
		unw_word_t ip;
		unw_context_t uc;
		unw_cursor_t cursor;

		/* This code is based on the manual page
		 * and has not been tested. */
		unw_getcontext(&uc);
		unw_init_local(&cursor, &uc);
		for (i = 1; unw_step(&cursor) > 0; i++)
		{
			unw_get_reg(&cursor, UNW_REG_IP, &ip);
			bt0(i, (void const *)ip, NULL);
		}
		/* }}} */
#elif defined(CONFIG_FAST_UNWIND) /* {{{ */
		unsigned i;
		void const *sseg;
		void const *const *fp, *lr;

		/* Find the frames by hand and extract the
		 * return addresses.  backtrace() is bogus
		 * on this platform.  Of course it's not
		 * this trivial in the real world, by far. */
		sseg = NULL;
		fp = __builtin_frame_address(0);
		for (i = 1; (fp = getlr(fp, &lr, &sseg)) != NULL; i++)
			bt0(i, lr, fp);
		/* }}} */
#else		/* Use glibc's backtrace(). {{{ */
		int i, n;

		/* Try to get the backtrace until we allocate big enough
		 * buffer not to lose any frames then process it. */
		for (i = 40; ; i += 40)
		{
			void *addrs[i];

			if ((n = backtrace(addrs, i)) >= i)
				/* $addrs might be too small. */
				continue;

			/* Process the frames from the top to the bottom
			 * but skip the topmost one (ourselves). */
			for (i = 1; i < n; i++)
				bt0(i, addrs[i], NULL);
			break;
		}
#endif /* how to get the backtrace }}} */
	} while (0);

#ifdef CONFIG_PRINTVARS
	/* Forget all variables we possibly printed, so they will be
	 * printed again when generating the next backtrace. */
	seen(NULL);
#endif
} /* barf */
/* Driver }}} */

/* Constructors {{{ */
#if CONFIG_LIBARF_EXTERNAL
/* Make barf() accessible to the world.  This circumvented initialization
 * is necessary for binaries to remain usable both with and without us. */
static void __attribute__((constructor)) init(void)
{
#if CONFIG_LIBARF_EXTERNAL > 1
	char str[20];

	/*
	 * Pass the address of barf() though the environment.
	 * 20 bytes need to be enough for 64 bit pointers.
	 * This is the poor man's very-late-binding linker.
	 * By not having undefined symbols (unlike below)
	 * libarf.so stands still even if the program does
	 * not #include arf.h at all.
	 */
	sprintf(str, "%p", barf);
	setenv("THE_REAL_BARF", str, 1);
#else /* CONFIG_LIBARF_EXTERNAL == 1 */
	/*
	 * This is defined in every CU:s that #include:s arf.h
	 * (the linker merges redundant definitions).
	 * This could be optimized for CONFIG_LIBARF_EXTERNAL=0,
	 * but that's not the main use case.
	 */
	extern void (*the_real_barf)(char const *, ...);

	the_real_barf = barf;
#endif
} /* init */
#endif /* CONFIG_LIBARF_EXTERNAL > 0 */
/* Constructors }}} */

/* vim: set foldmethod=marker: */
/* End of libarf.c */

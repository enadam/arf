/*
 * libero.c -- a remake of libleaks {{{
 *
 * Eero's diapers help you to see where your baby is peeing.
 * When linked to a program it overrides libc's memory allocation
 * functions.  Upon the reception of a signal (SIGPROF by default)
 * it starts keeping record of what is being allocated and freed.
 * Signaling the program again and again makes libero report the
 * current allocations in a file created in the program's working
 * directory.
 *
 * Unless compiled with -D_THREAD_SAFE libero is not thread safe.
 *
 * The report goes like: {{{
 * ---------------------------------------------------------------------------
 * number of allocations:  22 (currently 35)
 * current allocation:     13846 (delta=+10261 bytes)
 * peak allocation:        13846 (10261 bytes since the start of period)
 * 
 * ptr=0x806f020 (tid=12638), size=185, karma=171
 * ptr=0x8072e38 (tid=12639), size=633, karma=170
 * ptr=0x8072ba0 (tid=12639), size=658, karma=170
 * ptr=0x80ea740 (tid=12645), size=854, karma=109
 * ptr=0x811aa38 (tid=12637), size=469, karma=87
 * ptr=0x812d588 (tid=12637), size=294, karma=78
 * ptr=0x8155590 (tid=12644), size=251, karma=54
 * ptr=0x816ea98 (tid=12639), size=705, karma=41
 * ptr=0x80f3130 (tid=12638), size=202, karma=12
 * ptr=0x81b8fc0 (tid=12639), size=922, karma=11
 * ptr=0x81bdcf0 (tid=12642), size=545, karma=10
 * ptr=0x81c98f0 (tid=12638), size=145, karma=3
 *    1. testero_mt testero.c:78  roulette()
 *    2. testero_mt testero.c:17  foo()
 *    3. testero_mt testero.c:52  roulette()
 *    4. testero_mt testero.c:17  foo()
 *    5. testero_mt testero.c:52  roulette()
 *    6. testero_mt testero.c:106 zetork()
 * ptr=0x80b8a28 (tid=12643), size=109, karma=134
 * ptr=0x81254a8 (tid=12643), size=864, karma=81
 * ptr=0x814b2f0 (tid=12644), size=319, karma=61
 *    1. testero_mt testero.c:78  roulette()
 *    2. testero_mt testero.c:17  foo()
 *    3. testero_mt testero.c:52  roulette()
 *    4. testero_mt testero.c:17  foo()
 *    5. testero_mt testero.c:52  roulette()
 *    6. testero_mt testero.c:42  quux()
 *    7. testero_mt testero.c:67  roulette()
 *    8. testero_mt testero.c:106 zetork()
 *
 * Where:
 *                         How many malloc()s did we see
 *                         vv  in the reporting period?
 * number of allocations:  22 (currently 35)
 *                                       ^^
 *                                       How many allocated areas
 *                                       do we know about.
 *
 *                         bytes
 *                         vvvvv
 * current allocation:     13846 (delta=+10261 bytes)
 *                                      ^^^^^^
 *                                      Since the previous report.
 *
 *                         Absolute peak during the reporting period.
 *                         vvvvv
 * peak allocation:        13846 (10261 bytes since the start of period)
 *                                ^^^^^
 *                                How many bytes was the peak from the
 *                                allocations at the start of the period.
 *
 *                Which thread allocated this piece of memory.
 *                vvvvvvvvv    (Only shown if libero is thread-aware.)
 * ptr=0x806f020 (tid=12638), size=185, karma=171
 *                                      ^^^^^^^^^
 *                                      How many report earlier did we
 *                                      see this allocation first.
 *                                      The higher the karma the more
 *                                      likely it's leaked.
 *
 * Where was/were the memories allocated.  In the example several threads
 * allocated memory in the same code path.
 *    1. testero_mt testero.c:78  roulette()
 *    2. testero_mt testero.c:17  foo()
 *    3. testero_mt testero.c:52  roulette()
 *    4. testero_mt testero.c:17  foo()
 *    5. testero_mt testero.c:52  roulette()
 *    6. testero_mt testero.c:106 zetork()
 * }}}
 *
 * Environment: {{{
 *   -- $LIBERO_START={0|1}: see ./ero -start
 *   -- $LIBERO_SIGNAL=<signal-number>: (./ero -signal)
 *      Listen to <signal-number> besides LIBERO_SIGNAL.
 *   -- $LIBERO_TICK=<seconds>: (./ero -tick)
 *      Raise LIBERO_SIGNAL every $LIBERO_TICK seconds automatically.
 *   -- $LIBERO_KARMA_DEPTH=<unsigned>: (./ero -karma)
 *      Don't report backtraces unless they appear with allocations with
 *      this many differing karmas.
 *   -- $LIBERO_DEPTH=<unsigned>: (./ero -depth)
 *      Limit how many frames are traced back and stored in $Backtraces.
 * -- $LIBERO_TERSE={0|1}: see ./ero -terse
 * }}}
 *
 * Ex-Author:  Leonid Moiseichuk <leonid.moiseichuk@nokia.com>
 * Ex-Contact: Eero Tamminen     <eero.tamminen@nokia.com>
 * }}}
 */

/* Configuration */
#define _GNU_SOURCE
//#define _THREAD_SAFE
//#define CONFIG_FAST_UNWIND

/* Include files */
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <sched.h>
#ifdef _THREAD_SAFE
# include <pthread.h>
# include <sys/syscall.h>
#endif

#include <sys/time.h>
#include <sys/mman.h>

#include "libarf.c"

/* Standard definitions */
/* The signal that makes us start accounting or reporting.
 * You can specify another one in run-time by setting
 * $LIBERO_SIGNAL to a signal number like 2. */
#define LIBERO_SIGNAL               SIGPROF

/* Macros {{{ */
/* Returns the number of elements in an array. */
#define CAPACITY(a)                 (sizeof(a) / sizeof((a)[0]))

/* Freaks out gcc is $cond is unment. */
#define ASSERT(cond)                \
    do { char __attribute__((unused)) xx[(cond) ? 0 : -1]; } while (0)

/* Mock pthreads with NOPs -- it's much faster. */
#ifndef _THREAD_SAFE
# define IF_THREAD_SAFE(...)        /* NOP */

# define pthread_self()             1
# define pthread_equal(a, b)        ((a) == (b))

# define pthread_mutex_lock(...)    /* NOP */
# define pthread_mutex_unlock(...)  /* NOP */
#else
# define IF_THREAD_SAFE(...)        __VA_ARGS__
# define gettid()                   (pid_t)syscall(SYS_gettid)
#endif
/* }}} */

/* Type definitions {{{ */
/* Stores a complete backtrace or a part of it. */
struct backtrace_st
{
   /* 
    * $addrs:  The addresses got from backtrace(), lower elements being
    *          closer to the original call site.  A NULL element marks
    *          the end of the backtrace and all the remaining elements
    *          are NULLs.  The array size was chosen so that a pageful
    *          of backtrace_st:s doesn't waste space
    *          (PAGE_SIZE % ((7+1) * 4) == 0).
    * $next:   If not NULL points to the next part of the backtrace.
    *          The last part's (farthest from the original call site)
    *          is NULL.
    */
   void const *addrs[7];
   struct backtrace_st *next;
};

/* Represents a memory allocation. */
struct ero_st
{
   /*
    * $tid:       Who allocated it initially
    *             (subsequent realloc()s don't count).
    * $ptr:       Points to the allocted memory.
    * $size:      Requested size of the allocated memory
    *             (the reservation can be larger though).
    * $karma:     Since how many report()s have this allocation
    *             been around.  The larger the more likely it's leaked.
    * $backtrace: Where was it allocated initially.
    */
   IF_THREAD_SAFE(unsigned tid);
   size_t size;
   void const *ptr;
   unsigned karma;
   struct backtrace_st *backtrace;
   struct ero_st *next;
};
/* }}} */

/* Function prototypes {{{ */
/* Strong aliases of malloc() etc. */
extern void *__libc_malloc(size_t);
extern void *__libc_calloc(size_t, size_t);
extern void *__libc_valloc(size_t);
extern void *__libc_pvalloc(size_t);
extern void *__libc_memalign(size_t, size_t);
extern void *__libc_realloc(void *, size_t);
extern void  __libc_free(void *);
/* }}} */

/* Private variables {{{ */
/*
 * These guards are used to make sure at most one thread can do accounting
 * (recording a new allocation, reporting about allocations etc) at a time.
 *
 * $Mutex:     For mallfuncs, between different threads.
 * $Spinlock:  For mallfuncs and sighand(), between all threads.
 *    If its value is 1 then somebody is doing accounting.
 *    If the value is 2 then sighand() wants the current
 *       accounter to report() when finished.
 * $Executor:  Which thread is accounting at the moment.
 *             Used to recognize recursion.
 */
IF_THREAD_SAFE(static pthread_mutex_t Mutex = PTHREAD_MUTEX_INITIALIZER);
static volatile sig_atomic_t Spinlock;
static pthread_t Executor;

/*
 * For accounting:
 *
 * $Memories:     NULL-terminated list of currently known allocations.
 *                New allocations are prepended to this list.
 * $NMemories:    The length of $Memories.
 * $Ero_pool:     NULL-terminated list of unused ero_st:s.
 *                Consumed and filled from the head.
 * $Backtraces:   Like $Ero_pool for backtrace_st:s.
 * $NAllocations: Number of new allocations (additions to the records)
 *                since the last report().
 * $Allocated:    Number of bytes currently in use (in theory)
 *                by the program.
 * $Peak:         The lasrgest $Allocated since the last report().
 */
static struct backtrace_st *Backtraces;
static struct ero_st *Memories, *Ero_pool;
static unsigned NMemories, NAllocations;
static int Allocated, Peak;

/*
 * $Profiling:       Do account for memory allocations (except for memory we
 *                   allocate for ourselves).
 * $End_to_end:      Account for memory allocations during the lifecycle of
 *                   the program and report when it's finished.
 * $Profiling_since: When we started counting allocations; it is written
 *                   in the first report.
 * $Backtrace_depth: How many stack frames to capture on an allocation,
 *                   set by $LIBERO_DEPTH, -1 to get all of them.
 * $Karma_min_depth: With how many different karmas a backtrace needs
 *                   to appear with to consider reporting it.
 * $Summary_only:    Don't save any backtraces at all and don't log
 *                   information about individual allocations.
 */
static int Profiling, End_to_end;
static struct timeval Profiling_since;
static int Backtrace_depth = -1;
static unsigned Karma_min_depth;
static int Summary_only;
/* Private variables }}} */

/* Program code */
/* Internal memory management {{{ */
/* Creates a new pool of ero_st:s or backtrace_st:s and initializes it
 * by creating the linked list. */
static void *new_pool(size_t size1, size_t nextoff)
{
   char *ptr;
   unsigned pagesize, n, i;

   /* We know very well our $pagesize, no need to make it complicated. */
   pagesize = 4096;
   n = pagesize / size1;
#if 1
   /* It's perfectly okay to call malloc() in mallfuncs context,
    * they are protected against reentrancy. */
   ptr = malloc(size1 * n);
#else
   /* Slower but completely invisible to malloc(). */
   ptr = mmap(NULL, pagesize, PROT_READ|PROT_WRITE, MAP_PRIVATE, -1, 0);
#endif
   if (!ptr)
      return NULL;

   /* Initialize the new linked list. */
   for (i = 0; i < n-1; i++)
      *(void **)&ptr[i*size1 + nextoff] = &ptr[(i+1) * size1];
   *(void **)&ptr[i*size1 + nextoff] = NULL;

   return ptr;
} /* new_pool */

static struct ero_st *new_ero_pool(void)
{
   return new_pool(sizeof(struct ero_st), offsetof(struct ero_st, next));
} /* new_ero_pool */

static struct backtrace_st *new_backtraces(void)
{
   return new_pool(sizeof(struct backtrace_st),
      offsetof(struct backtrace_st, next));
} /* new_backtraces */
/* Internal memory management }}} */

/* Sorting {{{ */
static int compare_backtraces(
   struct backtrace_st const *bt1, struct backtrace_st const *bt2)
{
   /* Compare the backtraces part by part. */
   for (;;)
   {
      int cmp;

      if (!bt1 && !bt2)
         return  0;
      else if (!bt1)
         return -1;
      else if (!bt2)
         return  1;

      cmp = memcmp(bt1->addrs, bt2->addrs, sizeof(bt1->addrs));
      if (cmp != 0)
         return cmp;

      bt1 = bt1->next;
      bt2 = bt2->next;
   }

   return 0;
} /* compare_backtraces */

static int compare(struct ero_st const *mem1, struct ero_st const *mem2)
{
   int cmp;

   /* If the backtraces are the same let the higher karma win. */
   cmp = compare_backtraces(mem1->backtrace, mem2->backtrace);
   if (cmp != 0)
      return cmp;
   if (mem1->karma > mem2->karma)
      return -1;
   else if (mem1->karma < mem2->karma)
      return  1;
   else
      return 0;
} /* compare */

/* Merges two sorted lists, returning the head and the tail
 * of the combined list. */
static struct ero_st *merge(struct ero_st **tailp,
   struct ero_st *list1, struct ero_st *list1end,
   struct ero_st *list2, struct ero_st *list2end)
{
   struct ero_st *nextlist, *head, *tail;

   nextlist = list2end->next;
   head = tail = NULL;
   for (;;)
   {
      if (compare(list1, list2) <= 0)
      {
         if (tail)
            tail->next = list1;
         else /* head == tail == NULL */
            head = list1;
         tail = list1;

         if (list1 == list1end)
         {
            tail->next = list2;
            tail = list2end;
            break;
         } else
            list1 = list1->next;
      } else
      {
         if (tail)
            tail->next = list2;
         else /* head == tail == NULL */
            head = list2;
         tail = list2;

         if (list2 == list2end)
         {
            tail->next = list1;
            tail = list1end;
            break;
         } else
            list2 = list2->next;
      }
   } /* for */

   tail->next = nextlist;
   *tailp = tail;
   return head;
} /* merge */

/* Merge sort $list and return its head and tail. */
static struct ero_st *sort1(struct ero_st *list, unsigned nlist,
      struct ero_st **rightendp)
{
   unsigned nleft;
   struct ero_st *left, *leftend;
   struct ero_st *right, *rightend;

   if (nlist <= 1)
   {
      *rightendp = list;
      return list;
   } else if (nlist == 2)
   {  /* Make sure (*rightendp)->next == start of next list. */
      if (compare(list, list->next) <= 0)
      {
         *rightendp = list->next;
         return list;
      } else
      {
         struct ero_st *head;

         head = list->next;
         list->next = head->next;
         head->next = list;
         *rightendp = list;
         return head;
      }
   }

   nleft = nlist / 2;
   left  = sort1(list, nleft, &leftend);
   right = sort1(leftend->next, nlist-nleft, &rightend);

   return merge(rightendp, left, leftend, right, rightend);
} /* sort1 */

/* Sort $list of $nlist elements. */
static struct ero_st *sort(struct ero_st *list, unsigned nlist)
{
   struct ero_st *tail;
   return sort1(list, nlist, &tail);
} /* sort */
/* Sorting }}} */

/* Accounting {{{ */
/* Add $ptr to the records.  Called in mallfuncs context. */
static void *garbage(void *ptr, size_t size, int intracall)
{
   struct ero_st *mem;
   unsigned i, top, bottom;

   if (!ptr)
      /* malloc() failed, don't record. */
      return NULL;

   /* Update the counters whether we can make a record or not. */
   NAllocations++;
   Allocated += size;
   if (Peak < Allocated)
      Peak = Allocated;

   /* We are permitted to clobber errno because our caller
    * is going to return with success. */
   if (!Ero_pool && !(Ero_pool = new_ero_pool()))
      return ptr;

   mem = Ero_pool;
   Ero_pool = Ero_pool->next;
   mem->next = Memories;
   Memories = mem;
   NMemories++;

   mem->ptr = ptr;
   mem->size = size;
   mem->karma = 0;
   IF_THREAD_SAFE(mem->tid = gettid());

   mem->backtrace = NULL;
   if (!Backtrace_depth)
      goto skip_backtrace;
   if (!Backtraces && !(Backtraces = new_backtraces()))
      goto skip_backtrace;
   mem->backtrace = Backtraces;

   /* We're called through fun() -> malloc() -> garbage(),
    * ignore the top two frames.  The bottom two frames
    * are below main(), ignore them too. */
   top = 2;
   if (intracall)
      /* An accountant function called another hook, ignore that too. */
      top++;

#ifndef CONFIG_FAST_UNWIND
   /* Try getting the backtrace until $addrs is large enough.
    * Start with a large buffer to get away with as few retries
    * as possible. */
   bottom = 2;
   for (i = Backtrace_depth > 0 ? top+Backtrace_depth : 100; ; i += 100)
   {
      unsigned depth;
      void *addrs[i];
      struct backtrace_st *next;

      if ((depth = backtrace(addrs, i)) >= i && Backtrace_depth < 0)
         /* $addrs was too small. */
         continue;

      if (depth > top)
      {  /* Ignore $top frames. */
         depth -= top;
         if (top+depth < i && depth > bottom)
            /* If we got the full backtrace also ignore
             * the $bottom frames. */
            depth -= bottom;
      } else /* Don't ignore anyhing. */
         top = 0;

      /* Add the backtrace to $mem.  If #depth is larger than the CAPACITY
       * of backtrace_st chain up more. */
      for (;;)
      {
         int more;
         unsigned n;

         n = CAPACITY(Backtraces->addrs);
         more = n < depth;
         if (n > depth)
            n = depth;

         memcpy(Backtraces->addrs, &addrs[top], sizeof(addrs[0]) * n);
         if (!more)
         {
            /* NULL-pad the unused $addrs, so compare_backtraces() won't
             * tell apart identical backtraces because of garbage. */
            memset(&Backtraces->addrs[n], 0,
               sizeof(Backtraces->addrs[0]) * (CAPACITY(Backtraces->addrs)-n));
            break;
         }

         if (!Backtraces->next && !(Backtraces->next = new_backtraces()))
            /* The bottom of the backtrace will be lost. */
            break;
         Backtraces = Backtraces->next;
         top   += n;
         depth -= n;
      } /* for */
      next = Backtraces->next;
      Backtraces->next = NULL;
      Backtraces = next;

      break;
   } /* for */
#else /* CONFIG_FAST_UNWIND */
   /* We need to be able to store at least $top addresses.
    * If not, ignoring top/bottom frames may break. */
   ASSERT(CAPACITY(Backtraces->addrs) >= 3);

   /* arf leaves less junk at the bottom than backtrace(). */
   bottom = 1;

   do
   {
      unsigned depth;
      void const *sseg;
		void const *const *fp, *lr;
      struct backtrace_st *prev, *next;

      /* We don't know beforehand the depth of the backtrace,
       * so store the addresses in $Backtraces as we unwind
       * the stack iteratively. */
      prev = NULL;
      sseg = NULL;
		fp = __builtin_frame_address(0);
		for (i = depth = 0; ; i++, depth++)
		{
         if (!(fp = getlr(fp, &lr, &sseg))
            || (Backtrace_depth > 0 && depth >= Backtrace_depth))
         {  /* End of backtrace, ignore the bottom frames if we can. */
            if (!fp && !top && depth > bottom)
            {
               if (i <= bottom)
               {  /* The addresses in $Backtraces are all ignored,
                   * discard the whole page. */
                  Backtraces = prev;
                  i += CAPACITY(Backtraces->addrs) - bottom;
               } else
                  i -= bottom;
            }

            /* Zero out the unused slots (which can be 0). */
            memset(&Backtraces->addrs[i], 0,
               sizeof(Backtraces->addrs[0])
                  * (CAPACITY(Backtraces->addrs)-i));
            break;
         }

         if (depth == top)
            /* Time to ignore $top. */
            i = depth = top = 0;
         else if (i >= CAPACITY(Backtraces->addrs))
         {  /* $Backtraces is full, get a new page. */
            if (!Backtraces->next && !(Backtraces->next = new_backtraces()))
               break;
            prev = Backtraces;
            Backtraces = Backtraces->next;
            i = 0;
         }

         Backtraces->addrs[i] = lr;
		} /* for */

      /* NULL-terminate $mem->backtaces and dequeue the tail
       * from $Backtraces. */
      next = Backtraces->next;
      Backtraces->next = NULL;
      Backtraces = next;
   } while (0);
#endif /* CONFIG_FAST_UNWIND */

skip_backtrace:
   return ptr;
} /* garbage */

/* Change $ptr's records.  Called in mallfuncs context. */
static void *regarbage(void *ptr, void *newptr, size_t size)
{
   struct ero_st *prev, *mem;

   if (!newptr)
      return NULL;

   /* Adjust the ->size of $ptr's $mem if we alredy keep a record of it. */
   for (prev = NULL, mem = Memories; mem; prev = mem, mem = mem->next)
      if (mem->ptr == ptr)
      {
         NAllocations++;
         Allocated += size - mem->size;
         if (Peak < Allocated)
            Peak = Allocated;

         mem->ptr = newptr;
         mem->size = size;

         /* Try to keep the head of $Memories hot and move $mem there. */
         if (prev)
         {
            prev->next = mem->next;
            mem->next = Memories;
            Memories = mem;
         }

         return newptr;
      }

   /* Haven't seen $ptr yet. */
   return garbage(newptr, size, 1);
} /* regarbage */

/* Delete $ptr's record.  Called in mallfuncs context. */
static void collect(void const *ptr)
{
   struct ero_st *prev, *mem;

   /* Find $ptr in $Memories. */
   for (mem = Memories, prev = NULL; mem; prev = mem, mem = mem->next)
   {
      if (mem->ptr == ptr)
      {
         struct backtrace_st *bt;

         /* Remove $mem from $Memories and add to $Ero_pool. */
         if (prev)
            prev->next = mem->next;
         else
            Memories = mem->next;
         NMemories--;
         Allocated -= mem->size;

         if (mem->backtrace)
         {  /* Return the backtrace segments to $Backtraces. */
            for (bt = mem->backtrace; bt->next; bt = bt->next)
               ;
            bt->next = Backtraces;
            Backtraces = mem->backtrace;
         }

         mem->next = Ero_pool;
         Ero_pool = mem;

         return;
      }
   }
} /* collect */

/* Report on the $Memories currently in use.
 * Can be called either in mallfuncs or signal context,
 * or from the library destructor. */
static void report(void)
{
   static unsigned nreports;
   static int previous;
   struct tm tm;
   struct timeval now;
   char buf[64];
   char const *prg;
   int saved_errno;
   FILE *saved_stderr;
   struct ero_st *mem;

   saved_errno = errno;
   saved_stderr = stderr;

   /* Construct the output file name.  Get it right even after a fork(). */
   if (!(prg = strrchr(program_invocation_short_name, '/')))
      prg = program_invocation_short_name;
   else
      prg++;
   snprintf(buf, sizeof(buf), "%s.%u.leaks", prg, getpid());

   /* bt1() will only log onto stderr. */
   if (!(stderr = fopen(buf, "a")))
      goto out;

   /* Overall statistics */
   if (!nreports)
   {
      localtime_r(&Profiling_since.tv_sec, &tm);
      fprintf(stderr,
         "started profiling on:\t" "%.2u:%.2u:%.2u.%.6lu %.2u/%.2u/%.2u\n",
         tm.tm_hour, tm.tm_min, tm.tm_sec, Profiling_since.tv_usec,
         tm.tm_mday, 1+tm.tm_mon, tm.tm_year % 100);
   }

   gettimeofday(&now, NULL);
   localtime_r(&now.tv_sec, &tm);
   fprintf(stderr,
      "report %u created on:\t"  "%.2u:%.2u:%.2u.%.6lu %.2u/%.2u/%.2u\n",
      ++nreports,
      tm.tm_hour, tm.tm_min, tm.tm_sec, now.tv_usec,
      tm.tm_mday, 1+tm.tm_mon, tm.tm_year % 100);
   fprintf(stderr,
      "number of allocations:\t" "%u (currently %u)\n",
      NAllocations, NMemories);
   fprintf(stderr,
      "current allocation:\t"    "%d (delta=%+d bytes)\n",
      Allocated, Allocated-previous);
   fprintf(stderr,
      "peak allocation:\t"       "%d (%d bytes since the start of period)\n",
      Peak, Peak-previous);
   fputs("\n", stderr);

   NAllocations = 0;
   Peak = previous = Allocated;
   if (Summary_only)
      goto done;

   /* Dump all $Memories.
    * It makes little sense to sort without backtraces. */
   if (Backtrace_depth)
      Memories = sort(Memories, NMemories);
   for (mem = Memories; mem; mem = mem->next)
   {
      unsigned karmas;
      struct ero_st const *prev;
      struct backtrace_st const *bt;

      /* Chain up identical call sites. */
      karmas = 0;
      prev = NULL;
      for (;;)
      {
#ifdef _THREAD_SAFE
         fprintf(stderr, "ptr=%p (tid=%u), size=%zu, karma=%u\n",
            mem->ptr, mem->tid, mem->size, mem->karma++);
#else
         fprintf(stderr, "ptr=%p, size=%zu, karma=%u\n",
            mem->ptr, mem->size, mem->karma++);
#endif

         /* Count with how many different karmas have we seen
          * the same backtrace. */
         if (!prev || prev->karma != mem->karma)
            karmas++;

         /* Is the next backtrace the same as $mem's? */
         if (!mem->next
               || compare_backtraces(mem->backtrace, mem->next->backtrace))
            break;

         prev = mem;
         mem = mem->next;
      } /* for */

      /* Dump the backtrace. */
      if (karmas >= Karma_min_depth)
      {
         unsigned i, o;

         for (i = 1, o = 0, bt = mem->backtrace; bt && bt->addrs[o]; i++)
         {
            bt0(i, bt->addrs[o++], NULL);
            if (o >= CAPACITY(bt->addrs))
            {
               bt = bt->next;
               o = 0;
            }
         }
      } /* if */
   } /* for */
done:
   fputs("-------------------------------------------------"
         "--------------------------\n", stderr);

   fclose(stderr);
out:
   stderr = saved_stderr;
   errno = saved_errno;
} /* report */
/* Accounting }}} */

/* Concurrency and reentrancy {{{ */
#ifdef __arm__
/* Based on kernel code.
 * For >= i486 gcc can emit the instruction itself.  */
char __sync_bool_compare_and_swap_4(volatile int *ptr, int old, int new)
{
   unsigned long oldval, res;

   do
   {
      __asm__ __volatile__(
         "@ atomic_cmpxchg\n"
         "ldrex   %1, [%2]\n"       // oldval <- *ptr
         "mov     %0, #0\n"         // res    <- 0
         "teq     %1, %3\n"         // oldval == old?
         "strexeq %0, %4, [%2]\n"   // *ptr   <- new || res <- 1
            : "=&r" (res), "=&r" (oldval)
            : "r" (ptr), "Ir" (old), "r" (new)
            : "cc");
   } while (res);

   return oldval == old;
} /* __sync_bool_compare_and_swap_4 */
#endif /* __arm__ */

/* Enter a critical section. */
static void enter(void)
{
   pthread_mutex_lock(&Mutex);
   while (!__sync_bool_compare_and_swap(&Spinlock, 0, 1))
      /* sighand() is accounting. */
      sched_yield();
} /* enter */

/* Leave the critical section. */
static void leave(void)
{
   if (!__sync_bool_compare_and_swap(&Spinlock, 1, 0))
   {
      /* sighand() interrupted us and queued a report(). */
      /* Critical section */
      Executor = pthread_self();
      report();
      Executor = 0;
      /* Critical section */

      /* I think it doesn't matter if it's reordered. */
      Spinlock = 0;
   }
   pthread_mutex_unlock(&Mutex);
} /* leave */

#define WRAP_MALLFUNC(ifmulti, ifsingle)                       \
do                                                             \
{                                                              \
   if (!Profiling || pthread_equal(Executor, pthread_self()))  \
   {                                                           \
      /* Called by the same thread in critical section, */     \
      /* just do the work without accounting.           */     \
      /* We're in trouble if !Profiling yet but during  */     \
      /* they sighand() interrupts us twice and starts  */     \
      /* accounting.                                    */     \
      ifsingle;                                                \
   } else                                                      \
   {                                                           \
      enter();                                                 \
                                                               \
      /* Critical section */                                   \
      Executor = pthread_self();                               \
      ifmulti;                                                 \
      Executor = 0;                                            \
      /* Critical section */                                   \
                                                               \
      leave();                                                 \
   }                                                           \
} while (0)

static void sighand(int unused)
{
   /* Instruct mallfuncs to start accounting if they haven't. */
   if (!Profiling)
   {  /* No tricky things, the program can be in any state. */
      gettimeofday(&Profiling_since, NULL);
      Profiling = 1;
      return;
   }

   if (Spinlock == 2)
      /* A report() is already underway, ignore this request. */
      return;

   while (!__sync_bool_compare_and_swap(&Spinlock, 0, 1))
   {
      /* A mallfunc (this thread's or another's) or another
       * thread's sighand() is in critical secion. */
      if (__sync_bool_compare_and_swap(&Spinlock, 1, 2))
         /* A mallfunc will make a report() for us.
          * If our concurrent is a sighand() we will silently
          * ignore this report(), which is a Good Thing. */
         return;
      sched_yield();
   }

   /* Critical section */
   Executor = pthread_self();
   report();
   Executor = 0;
   /* Critical section */

   Spinlock = 0;
} /* sighand */
/* Concurrancy and reentrancy }}} */

/* ero's mallfuncs {{{ */
/* Override libc's functions.  Using malloc hooks would be nicer,
 * but the it's impossible to chain up in a thread-safe manner. */
void *malloc(size_t size)
{
   void *ptr;
   WRAP_MALLFUNC(
      { ptr = garbage(__libc_malloc(size), size, 0); },
      { ptr =         __libc_malloc(size); });
   return ptr;
} /* malloc */

void *calloc(size_t n, size_t size1)
{
   void *ptr;
   WRAP_MALLFUNC(
      { ptr = garbage(__libc_calloc(n, size1), size1*n, 0); },
      { ptr =         __libc_calloc(n, size1); });
   return ptr;
} /* calloc */

void *memalign(size_t boundary, size_t size)
{
   void *ptr;
   WRAP_MALLFUNC(
      { ptr = garbage(__libc_memalign(boundary, size), size, 0); },
      { ptr =         __libc_memalign(boundary, size); });
   return ptr;
} /* memalign */

void *valloc(size_t size)
{
   void *ptr;
   WRAP_MALLFUNC(
      { ptr = garbage(__libc_valloc(size), size, 0); },
      { ptr =         __libc_valloc(size); });
   return ptr;
} /* valloc */

void *pvalloc(size_t size)
{
   void *ptr;
   WRAP_MALLFUNC(
      { ptr = garbage(__libc_pvalloc(size), size, 0); },
      { ptr =         __libc_pvalloc(size); });
   return ptr;
} /* pvalloc */

void *realloc(void *ptr, size_t size)
{
   if (ptr && size)
   {
      WRAP_MALLFUNC(
         { ptr = regarbage(ptr, __libc_realloc(ptr, size), size); },
         { ptr =                __libc_realloc(ptr, size); });
   } else if (!ptr)
   {  /* Using malloc() would show up in the backtrace. */
      WRAP_MALLFUNC(
         { ptr = garbage(__libc_malloc(size), size, 0); },
         { ptr =         __libc_malloc(size); });
   } else /* !size */
   {
      free(ptr);
      ptr = NULL;
   }

   return ptr;
} /* realloc */

void free(void *ptr)
{
   WRAP_MALLFUNC(
      { __libc_free(ptr);  collect(ptr); },
      { __libc_free(ptr); });
} /* free */

void cfree(void *ptr)
{  /* Seriously, who has used cfree() in his life? */
   free(ptr);
} /* cfree */
/* ero's mallfuncs }}} */

/* Constructors {{{ */
/* Install signal handlers and start profiling if requested. */
static __attribute__((constructor))
void ero_init(void)
{
   char const *env;

   Profiling = End_to_end = (env = getenv("LIBERO_START"))
      && (*env == '1' || *env == 'y' || *env == 'Y');

   if ((env = getenv("LIBERO_DEPTH")) != NULL)
      Backtrace_depth = atoi(env);
   if ((env = getenv("LIBERO_KARMA_DEPTH")))
      Karma_min_depth = atoi(env);
   if ((env = getenv("LIBERO_TERSE")) != NULL)
      Summary_only = atoi(env);
   if (Summary_only)
      Backtrace_depth = 0;

   if ((env = getenv("LIBERO_TICK")) != NULL)
   {
      struct itimerval timer;

      /* Start profiling in $LIBERO_TICK seconds and so on. */
      timer.it_interval.tv_sec  = atoi(env);
      timer.it_interval.tv_usec = 0;
      timer.it_value = timer.it_interval;

      /* setitimer() sends us SIGPROF. */
      if (LIBERO_SIGNAL != SIGPROF)
         signal(SIGPROF, sighand);
      setitimer(ITIMER_PROF, &timer, NULL);
   }

   signal(LIBERO_SIGNAL, sighand);
   if ((env = getenv("LIBERO_SIGNAL")) != NULL)
      signal(atoi(env), sighand);
} /* ero_init */

/* Do a final report() if requested. */
static __attribute__((destructor))
void ero_done(void)
{
   if (End_to_end)
   {
      Profiling = 0;
      report();
   }
} /* ero_done */
/* Constructors }}} */

/* vim: set et ts=3 sw=3 foldmethod=marker: */
/* End of libero.c */

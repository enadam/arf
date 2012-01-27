/*
 * testero.c -- stress test for libero
 *
 * Try to stress out libero by randomly allocating, reallocating
 * and deallocating memory, and taking dumps.  If -D_THREAD_SAFE
 * do this in concurrent threads.
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#ifdef _THREAD_SAFE
# include <pthread.h>
#endif

static volatile int Quit;
static void roulette(unsigned id, void *pool[], unsigned npool);

/* Program code */
/* Imitate varying codepaths. */
static void foo(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

static void bar(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

static void baz(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

static void fubar(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

static void qux(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

static void quux(unsigned id, void *pool[], unsigned npool)
{
	roulette(id, pool, npool);
}

/* Either go to foo(), bar() etc or do something randomly. */
static void roulette(unsigned id, void *pool[], unsigned npool)
{
	unsigned i;

	switch (rand() % 10)
	{
	case 0:
		foo(id, pool, npool);
		break;
	case 1:
		bar(id, pool, npool);
		break;
	case 2:
		baz(id, pool, npool);
		break;
	case 3:
		fubar(id, pool, npool);
		break;
	case 4:
		qux(id, pool, npool);
		break;
	case 5:
		quux(id, pool, npool);
		break;
	default:
		i = rand() % (npool + npool/10);
		if (i >= npool)
		{
			printf("%u. dump\n", id);
			raise(SIGPROF);
		} else if (!pool[i])
		{
			printf("%u. malloc\n", id);
			pool[i] = malloc(rand() % 1024);
		} else if (rand() % 3 == 0)
		{
			printf("%u. realloc\n", id);
			pool[i] = realloc(pool[i], rand() % 1024);
		} else
		{
			printf("%u. free\n", id);
			free(pool[i]);
			pool[i] = NULL;
		}
		break;
	} /* switch */
} /* roulette */

/* roulette() until $Quit then clean up. */
static void *zetork(void *id)
{
	void *pool[10000];
	unsigned npool, i;

	npool = sizeof(pool) / sizeof(pool[0]);
	memset(pool, 0, sizeof(pool));
	while (!Quit)
		roulette((unsigned)id, pool, npool);

	printf("%u. quit\n", (unsigned)id);
	for (i = 0; i < npool; i++)
		free(pool[i]);
	return NULL;
} /* zetork */

static void sigint(int unused)
{	/* Tell zetork()s to finish. */
	Quit = 1;
}

int main(int argc, char const *argv[])
{
	srand(time(NULL));
	signal(SIGINT, sigint);
	raise(SIGPROF);

#ifdef _THREAD_SAFE
	unsigned n, i;

	/* zetork() in $n threads.  Wait until all of them finished. */
	n = argv[1] ? atoi(argv[1]) : 10;
	pthread_t tids[n];
	for (i = 0; i < n; i++)
		pthread_create(&tids[i], NULL, zetork, (void *)i);
	for (i = 0; i < n; i++)
		pthread_join(tids[i], NULL);
#else
	zetork(0);
#endif

	raise(SIGPROF);
	puts("fini");

	return 0;
} /* main */

/* End of testero.c */

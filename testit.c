#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include "my_alloc.h"
#include "my_system.h"

#ifndef VERBOSE
#define VERBOSE 1 /* by default we want to see results */
#endif

struct data {
	char * ptr;
	size_t len;
	char * contents;
};

#define DATACOUNT 1000000

static struct data data[DATACOUNT];
static char randdata[20000];
static size_t nptr = 0;

static size_t alloc = 0;
static size_t maxalloc = 0;
static size_t nalloc = 0;
static size_t maxnalloc = 0;

struct profile {
	int (*get)(struct profile * p);
	int status[10];
};

static int get_oneinthree (struct profile * p)
{
	if (lrand48() % 3) {
		return 1;
	}
	return -1;
}

static void create_oneinthree (struct profile * p)
{
	p->get = &get_oneinthree;
}

static int get_cluster (struct profile * p)
{
	if (p->status[0] == 0) {
		p->status[0] = 1+lrand48 () % 1000;
		if (lrand48 () % 2) {
			p->status[1] = -1;
		} else {
			p->status[1] = 1;
		}
	}
	p->status[0]--;
	return p->status[1];
}

static void create_cluster (struct profile * p)
{
	p->status[0] = 0;
	p->status[1] = 0;
	p->get = &get_cluster;
}

static int get_uniform (struct profile * p)
{
	return 8 + 8 * (lrand48()%32);
}

static int get_increase (struct profile * p)
{
	if (!p->status[0]) {
		p->status[0] = 5000;
		p->status[1] += 8;
		if (p->status[1] > 256)
			p->status[1] = 8;
	}
	p->status[0]--;
	return p->status[1];
}

static int get_decrease (struct profile * p)
{
	if (!p->status[0]) {
		p->status[0] = 5000;
		p->status[1] -= 8;
		if (p->status[1] < 8)
			p->status[1] = 256;
	}
	p->status[0]--;
	return p->status[1];
}

static void create_increase (struct profile * p)
{
	p->get = &get_increase;
	p->status[0] = p->status[1] = 0;
}

static void create_decrease (struct profile * p)
{
	p->get = &get_decrease;
	p->status[0] = 5000;
	p->status[1] = 256;
}

static void create_uniform (struct profile * p)
{
	p->get = &get_uniform;
}

int get_normal1 (struct profile * p)
{
	int val = -31 + lrand48() % 32 + lrand48()%32;
	if (val < 0)
		val = -val;
	val = 8 + 8*val;
	assert (8 <= val && val <= 256);
	return val;
}

static void create_normal1 (struct profile * p) {
	p->get = &get_normal1;
}

static int get_fixed (struct profile * p)
{
	return p->status[0];
}

static void create_fixed8 (struct profile * p)
{
	p->get = &get_fixed;
	p->status[0] = 8;
}

static void create_fixed16 (struct profile * p)
{
	p->get = &get_fixed;
	p->status[0] = 16;
}

static void create_fixed24 (struct profile * p)
{
	p->get = &get_fixed;
	p->status[0] = 24;
}

static void create_fixed104 (struct profile * p)
{
	p->get = &get_fixed;
	p->status[0] = 104;
}

static void create_fixed200 (struct profile * p)
{
	p->get = &get_fixed;
	p->status[0] = 200;
}


struct profile_list {
	char * name;
	void (*create) (struct profile * p);
};

struct profile_list alloc_profiles[] = {
	{ "oneinthree", &create_oneinthree },
	{ "cluster", &create_cluster },
	{ NULL, NULL },
};

struct profile_list size_profiles[] = {
	{ "uniform", &create_uniform },
	{ "normal1", &create_normal1 },
	{ "fixed8", &create_fixed8 },
	{ "fixed16", &create_fixed16 },
	{ "fixed24", &create_fixed24 },
	{ "fixed104", &create_fixed104 },
	{ "fixed200", &create_fixed200 },
	{ "increase", &create_increase },
	{ "decrease", &create_decrease },
	{ NULL, NULL },
};


void usage (char * prog) {
	int i;
	fprintf (stderr,
		"usage: %s seed count [ size_profile alloc_profile ]\n", prog);
	fprintf (stderr, "  Known size profiles:");
	for (i=0; size_profiles[i].name; ++i) {
		fprintf (stderr, " %s", size_profiles[i].name);
	}
	fprintf (stderr, "\n");
	fprintf (stderr, "  Known alloc profiles:");
	for (i=0; alloc_profiles[i].name; ++i) {
		fprintf (stderr, " %s", alloc_profiles[i].name);
	}
	fprintf (stderr, "\n");
}

int get_idx (struct profile_list * l, char * name)
{
	int i;
	for (i=0; l[i].name; ++i) {
		if (strcasecmp (l[i].name, name) == 0)
			return i;
	}
	return -1;
}

static struct avl_node * areas;

int main (int argc, char * argv[])
{
	int i;
	long seed;
	char ch;
	long long usecs = 0;
	int count;
	struct profile ap;
	struct profile sp;
	int apidx = 0, spidx = 0;
	double v1, v2, pts;
	int k, fd;
	char * p = randdata;
	init_my_alloc ();
	areas = create_avl ();
	fd = open ("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		perror ("open");
		return 1;
	}
	k = 20000;
	while (k) {
		int ret = read (fd, p, k);
		if (ret <= 0) {
			perror ("read");
			return 1;
		}
		p += ret;
		k -= ret;
	}
	if (argc < 3) {
		usage (argv[0]);
		return 1;
	}
	if (sscanf (argv[1], "%ld%c", &seed, &ch) != 1) {
		usage (argv[0]);
		return 1;
	}
	if (sscanf (argv[2], "%d%c", &count, &ch) != 1) {
		usage (argv[0]);
		return 1;
	}
	if (argc > 3) {
		if (argc != 5) {
			usage (argv[0]);
			return 1;
		}
		spidx = get_idx (size_profiles, argv[3]);
		apidx = get_idx (alloc_profiles, argv[4]);
		if (spidx < 0 || apidx < 0) {
			usage (argv[0]);
			return 1;
		}
	}
	srand48 (seed);
	(*size_profiles[spidx].create)(&sp);
	(*alloc_profiles[apidx].create)(&ap);
	for (i=0; i<count || nptr; ++i) {
#if VERBOSE
		if ((i+1) % 10000 == 0) {
			putchar('.');
			if ((i+1) % 100000 == 0) putchar('\n');
			fflush(stdout);
		}
#endif
		struct timeval tp1, tp2;
		int isalloc = ap.get (&ap);
		if (i < count && (nptr == 0 || isalloc > 0)) {
			int sz = sp.get (&sp);
			assert (sz && sz % 8 == 0);
			int offset = lrand48() % 19500;
			/* Anzahl der Operationen zu gross. */
			my_assert (nptr < DATACOUNT,
				"Anzahl der Operationen zu gross");
			alloc += sz;
			nalloc++;
			if (nalloc > maxnalloc)
				maxnalloc = nalloc;
			if (alloc > maxalloc)
				maxalloc = alloc;
			gettimeofday (&tp1, 0);
			data[nptr].ptr = my_alloc (sz);
			gettimeofday (&tp2, 0);
			//printf ("ALLOC: %u %u\n", data[nptr].ptr, sz);
			usecs += (tp2.tv_sec - tp1.tv_sec) * 1000000
				 + tp2.tv_usec - tp1.tv_usec;
#ifdef TIMEOUT
			if (((double)usecs / (2.0*count)) > 120.0) {
				printf ("Testcase aborted\n");
				break;
			}
#endif
			data[nptr].len = sz;
			data[nptr].contents = randdata+offset;
			memcpy (data[nptr].ptr, data[nptr].contents, sz);
			{
				struct avl_node * n;
				size_t ptr, ptr2;
				ptr2 = ptr = (size_t)data[nptr].ptr;
				/* Speicher nicht auf einer 8 Byte Kante,
				 * oder nicht in einem Block, den
				 * get_block_from_system geliefert hat. */
				my_assert (valid_area (ptr, sz),
					"Speicher nicht auf einer 8-Byte-Kante "
					"oder nicht in einem Block, den "
					"get_block_from_system geliefert hat");
				n = find_avl (areas, ptr);
				/* Ueberlappende Speicherbereiche. */
				my_assert (n->start + n->len <= ptr,
					"Ueberlappende Speicherbereiche");
				if (n->next) {
					ptr2 += sz;
					/* Ueberlappende Speicherbereiche. */
					my_assert (ptr2 <= n->next->start,
						"Ueberlappende "
						"Speicherbereiche");
				}
				insert_avl (&areas, ptr, sz);
			}
			nptr++;
		} else {
			int idx = lrand48() % nptr;
			int offset = lrand48() % 19500;
			int ret = memcmp (data[idx].ptr, data[idx].contents,
					  data[idx].len);
			/* Allokierter Speicher wurde vom System veraendert. */
			my_assert (ret == 0,
				"Allokierter Speicherbereich wurde "
				"zwischenzeitlich veraendert");
			memcpy (data[idx].ptr, randdata+offset,
				data[idx].len);
			data[idx].contents = randdata+offset;
			alloc -= data[idx].len;
			nalloc--;
			gettimeofday (&tp1, 0);
			my_free (data[idx].ptr);
			gettimeofday (&tp2, 0);
			//printf ("FREE: %u %u\n", data[idx].ptr, data[idx].len);
			usecs += (tp2.tv_sec - tp1.tv_sec) * 1000000
				 + tp2.tv_usec - tp1.tv_usec;
#ifdef TIMEOUT
			if (((double)usecs / (2.0*count)) > 120.0) {
				printf ("Testcase aborted\n");
				break;
			}
#endif
			{
				size_t ptr;
				struct avl_node * n;
				ptr = (size_t)data[idx].ptr;
				n = find_avl (areas, ptr);
				assert (n->start == ptr);
				assert (n->len == data[idx].len);
				remove_avl (&areas, n);
			}
			data[idx] = data[--nptr];
		}
	}
#if VERBOSE
	putchar('\n');
	printf ("%zd %zd %zd %lld\n",
		maxalloc, maxnalloc, get_sys_blockcount(), usecs);
	v1 = -1.0 + ((double) BLOCKSIZE * get_sys_blockcount ())/
		((double)(maxalloc+8*maxnalloc));
	v2 = (double)usecs/(double)i;
	printf ("Relative size overhead: %lf\n",
		-1.0 + ((double) BLOCKSIZE * get_sys_blockcount ())/
		((double)(maxalloc+8*maxnalloc)));
	printf ("Runtime per operation:  %lf\n", (double)usecs/(double)i);
	pts = 100.0 - 2.0*v2 - 100.0*v1;
	if (pts < 0) {
		pts = 0;
	}
	printf ("Points for this test: %lf\n", pts);
#endif
}

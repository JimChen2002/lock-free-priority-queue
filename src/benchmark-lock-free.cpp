
/**
 * Benchmark file for lock-free version of priority queue
 * 
 * Part of the benchmark harness was inspired by https://github.com/jonatanlinden/PR/blob/master/perf_meas.c
*/


#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <math.h>
#include <cstring>
#include <limits.h>
#include <sys/types.h>

#include "lock-free.h"
#define PIN

#define DEFAULT_SECS 2
#define DEFAULT_NTHREADS 4
#define DEFAULT_OFFSET 32
#define DEFAULT_SIZE 1 << 20
#define EXPS 100000000
#define NLEVEL 32

#define THREAD_ARGS_FOREACH(_iter) \
    for (int i = 0; i < nthreads && (_iter = &ts[i]); i++)

#define E_NULL(c)             \
    do                        \
    {                         \
        if ((c) == NULL)      \
        {                     \
            perror("E_NULL"); \
        }                     \
    } while (0)

#define E(c)                                   \
    do                                         \
    {                                          \
        int _c = (c);                          \
        if (_c < 0)                            \
        {                                      \
            fprintf(stderr, "E: %s: %d: %s\n", \
                    __FILE__, __LINE__, #c);   \
        }                                      \
    } while (0)

#define E_en(c)                                  \
    do                                           \
    {                                            \
        int _c = (c);                            \
        if (_c != 0)                             \
        {                                        \
            fprintf(stderr, "%s", strerror(_c)); \
        }                                        \
    } while (0)

void rng_init(unsigned short rng[3]);

typedef struct thread_args_s
{
    pthread_t thread;
    int id;
    unsigned short rng[3];
    int measure;
    int cycles;
    char pad[128];
} thread_args_t;

/* preload array with exponentially distanced integers for the
 * DES workload */
unsigned long *exps;
int exps_pos = 0;
void gen_exps(unsigned long *arr, unsigned short rng[3], int len, int intensity);

/* the workloads */
void work_exp(int id);
void work_uni(int id);
void work_debug(int id);
void work_flow(int id);

void *run(void *_args);

void (*work)(int id);
thread_args_t *ts;

volatile int wait_barrier = 0;
volatile int loop = 0;


void
pin(pid_t t, int cpu) 
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(t, sizeof(cpu_set_t), &cpuset);
}

static inline unsigned long
next_geometric(unsigned short seed[3], unsigned int p)
{
    return floor(log(erand48(seed)) / log(1 - p));
}

struct timespec
timediff(struct timespec begin, struct timespec end)
{
    struct timespec tmp;
    if ((end.tv_nsec - begin.tv_nsec) < 0)
    {
        tmp.tv_sec = end.tv_sec - begin.tv_sec - 1;
        tmp.tv_nsec = 1000000000 + end.tv_nsec - begin.tv_nsec;
    }
    else
    {
        tmp.tv_sec = end.tv_sec - begin.tv_sec;
        tmp.tv_nsec = end.tv_nsec - begin.tv_nsec;
    }
    return tmp;
}

void gettime(struct timespec *ts)
{
    E(clock_gettime(CLOCK_MONOTONIC, ts));
}

void rng_init(unsigned short rng[3])
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);

    /* initialize seed */
    rng[0] = time.tv_nsec;
    rng[1] = time.tv_nsec >> 16;
    rng[2] = time.tv_nsec >> 32;
}


LockFreePriorityQueue *pq;


int main(int argc, char **argv)
{
    // pq = new LockFreePriorityQueue(NLEVEL);
    pq = new LockFreePriorityQueue(32);

    int opt;
    unsigned short rng[3];
    struct timespec time;
    struct timespec start, end;
    thread_args_t *t;
    unsigned long elem;

    extern char *optarg;
    extern int optind, optopt;
    // int nthreads = DEFAULT_NTHREADS;
    int nthreads = atoi(argv[1]);
    int offset = DEFAULT_OFFSET;
    int secs = DEFAULT_SECS;
    int exp = 0;
    int init_size = DEFAULT_SIZE;
    int concise = 0;
    work = work_exp;
    exp = 1;

    printf("Compilation succeed!\n");

    E_NULL(ts = (thread_args_t *)malloc(nthreads * sizeof(thread_args_t)));
    memset(ts, 0, nthreads * sizeof(thread_args_t));


    clock_gettime(CLOCK_REALTIME, &time);

    /* initialize seed */
    rng[0] = time.tv_nsec;
    rng[1] = time.tv_nsec >> 16;
    rng[2] = time.tv_nsec >> 32;


    // if DES workload, pre-sample values/event times
    if (exp)
    {
        E_NULL(exps = (unsigned long *)malloc(sizeof(unsigned long) * EXPS));
        gen_exps(exps, rng, EXPS, 1000);
    }



    /* pre-fill priority queue with elements */
    for (int i = 0; i < init_size; i++)
    {
        if (exp)
        {
            elem = exps[exps_pos++];
            // pq->insert((int)elem, (int)elem, 0);
            pq->insert((int)elem, (int)elem);
        }
        else
        {
            elem = nrand48(rng);
            // pq->insert((int)elem, (int)elem, 0);
            pq->insert((int)elem, (int)elem);
        }
    }

    /* initialize threads */
    THREAD_ARGS_FOREACH(t)
    {
        t->id = i;
        rng_init(t->rng);
        E_en(pthread_create(&t->thread, NULL, run, t));
    }
    /* RUN BENCHMARK */

    /* wait for all threads to call in */
    while (wait_barrier != nthreads)
        ;
    __asm__ __volatile__("mfence" ::
                             : "memory");
    gettime(&start);
    loop = 1;
    __asm__ __volatile__("mfence" ::
                             : "memory");
    /* Process might sleep longer than specified,
     * but this will be accounted for. */
    usleep(1000000 * secs);
    loop = 0; /* halt all threads */
    __asm__ __volatile__("mfence" ::
                             : "memory");
    gettime(&end);

    /* END RUN BENCHMARK */

    THREAD_ARGS_FOREACH(t)
    {
        pthread_join(t->thread, NULL);
    }
    /* PRINT PERF. MEASURES */
    int sum = 0, min_num = INT_MAX, max_num = 0;

    THREAD_ARGS_FOREACH(t)
    {
        sum += t->measure;
        min_num = min(min_num, t->measure);
        max_num = max(max_num, t->measure);
    }
    struct timespec elapsed = timediff(start, end);
    double dt = elapsed.tv_sec + (double)elapsed.tv_nsec / 1000000000.0;

    if (!concise)
    {
        printf("Total time:\t%1.8f s\n", dt);
        printf("Ops:\t\t%d\n", sum);
        printf("Ops/s:\t\t%.0f\n", (double)sum / dt);
        printf("Min ops/t:\t%d\n", min_num);
        printf("Max ops/t:\t%d\n", max_num);
    }
    else
    {
        printf("%li\n", lround((double)sum / dt));
    }
}

__thread thread_args_t *args;

/* uniform workload */
void work_uni(int id)
{
    unsigned long elem = 0;
    // elem++;
    if (erand48(args->rng) < 0.5)
    {
        elem = (unsigned long)1 + nrand48(args->rng);
        // printf("about to insert %d\n", (int)elem);
        // pq->insert((int)elem, (int)elem, id);
        pq->insert((int)elem, (int)elem);
        // if(elem%10==1) printf("inserted %d\n", (int) elem);
    }
    else
        pq->deleteMin();
}

/* DES workload */
void work_exp(int id)
{
    int pos;
    unsigned long elem;
    pq->deleteMin();
    pos = __sync_fetch_and_add(&exps_pos, 1);
    elem = exps[pos];
    // pq->insert((int)elem, (int)elem, id);
    pq->insert((int)elem, (int)elem);
}

void work_flow(int id) {
    if(id & 1) {
        // insert only
        int elem = (int)(1 + nrand48(args->rng));
        pq->insert(elem, elem);
    }
    else {
        pq->deleteMin();
    }
}


void *
run(void *_args)
{
    args = (thread_args_t *)_args;
    int cnt = 0;

#if defined(PIN) && defined(__linux__)
    /* Straight allocation on 32 core machine.
     * Check with your OS + machine.  */
    pin(gettid(), args->id / 8 + 4 * (args->id % 8));
#endif

    // call in to main thread
    __sync_fetch_and_add(&wait_barrier, 1);

    // wait until signaled by main thread
    while (!loop)
        ;
    /* start benchmark execution */
    do
    {
        work(args->id);
        cnt++;
    } while (loop);
    /* end of measured execution */

    args->measure = cnt;
    return NULL;
}

/* generate array of exponentially distributed variables */
void gen_exps(unsigned long *arr, unsigned short rng[3], int len, int intensity)
{
    int i = 0;
    arr[0] = 2;
    while (++i < len)
        arr[i] = arr[i - 1] +
                 next_geometric(rng, intensity);
}
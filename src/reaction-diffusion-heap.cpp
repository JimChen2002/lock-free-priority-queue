
/**
 * Benchmark file for fine-grained version of priority queue
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

#include "fine-grained.h"

#define PIN

#define DEFAULT_SECS 2
#define DEFAULT_NTHREADS 4
#define DEFAULT_OFFSET 32
#define DEFAULT_SIZE 1 << 15
#define EXPS 100000000
#define NLEVEL 32
#define GRIDSIZE 1000

const double REACTION_RATE = 1e-7, DIFFUSION_RATE = 1e-6;

void
pin(pid_t t, int cpu) 
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu, &cpuset);
    sched_setaffinity(t, sizeof(cpu_set_t), &cpuset);
}


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

struct MeshNode
{
    double density;
} M[GRIDSIZE][GRIDSIZE];
int getid(int x,int y){
    return x*GRIDSIZE+y;
}
pair<int,int> getxy(int id){
    return make_pair(id/GRIDSIZE,id%GRIDSIZE);
}

/* preload array with exponentially distanced integers for the
 * DES workload */
unsigned long *exps;
int exps_pos = 0;
void gen_exps(unsigned long *arr, unsigned short rng[3], int len, int intensity);

/* the workloads */
void work_react_diff(int id);

void *run(void *_args);

void (*work)(int id);
thread_args_t *ts;

volatile int wait_barrier = 0;
volatile int loop = 0;

static inline unsigned long
next_geometric(unsigned short seed[3], unsigned int p)
{
    /* inverse transform sampling */
    /* cf. https://en.wikipedia.org/wiki/Geometric_distribution */
    return floor(log(erand48(seed)) / log(1 - p));
    /* uniformly distributed bits => geom. dist. level, p = 0.5 */
    // return __builtin_ctz(nrand48(seed) & (1LU << max) - 1) + 1;
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

    // finally available in macos 10.12 as well!
    clock_gettime(CLOCK_REALTIME, &time);

    /* initialize seed */
    rng[0] = time.tv_nsec;
    rng[1] = time.tv_nsec >> 16;
    rng[2] = time.tv_nsec >> 32;
}

HeapPriorityQueue *pq;

int main(int argc, char **argv)
{
    pq = new HeapPriorityQueue((int)1e8);

    int opt;
    unsigned short rng[3];
    struct timespec time;
    struct timespec start, end;
    thread_args_t *t;
    unsigned long elem;

    extern char *optarg;
    extern int optind, optopt;
    int nthreads = atoi(argv[1]);
    int offset = DEFAULT_OFFSET;
    int secs = DEFAULT_SECS;
    int exp = 0;
    int init_size = DEFAULT_SIZE;
    int concise = 0;
    work = work_react_diff;

    printf("Compilation succeed!\n");

    E_NULL(ts = (thread_args_t *)malloc(nthreads * sizeof(thread_args_t)));
    memset(ts, 0, nthreads * sizeof(thread_args_t));

    clock_gettime(CLOCK_REALTIME, &time);

    /* initialize seed */
    rng[0] = time.tv_nsec;
    rng[1] = time.tv_nsec >> 16;
    rng[2] = time.tv_nsec >> 32;

    /* initialize garbage collection */

    // if DES workload, pre-sample values/event times
    if (exp)
    {
        E_NULL(exps = (unsigned long *)malloc(sizeof(unsigned long) * EXPS));
        gen_exps(exps, rng, EXPS, 1000);
    }

    /* initialize mesh and pre-fill priority queue */
    for (int i=0;i<GRIDSIZE;i++)
        for(int j=0;j<GRIDSIZE;j++){
            M[i][j].density = erand48(rng) * 0.1;
            // calculate event time according to Engblom et al.
            double firstEvent = -log(1.0-erand48(rng))/M[i][j].density;
            pq->insert((int)firstEvent, getid(i,j), 0);
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

int dir[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

void work_react_diff(int id) {
    auto [tt, pos] = pq->pop();
    if(tt == -1){
        return;
    }
    auto [x, y] = getxy(pos);
    double old_density = M[x][y].density;
    // update new density due to reaction
    double new_density = old_density * (1 - REACTION_RATE);
    // update nearby density due to diffusion
    for(int i=0;i<4;i++){
        int nx=x+dir[i][0],ny=y+dir[i][1];
        if(0<=nx&&nx<GRIDSIZE&&0<=ny&&ny<GRIDSIZE){
            // Newton's diffusion equation
            double delta = (old_density - M[nx][ny].density) * DIFFUSION_RATE; 
            M[nx][ny].density += delta;
            new_density -= delta;
        }
    }
    M[x][y].density = new_density;
    double nextEvent = -log(1.0-erand48(args->rng))/new_density + tt;
    pq->insert((int)nextEvent, pos, id);
}


void *
run(void *_args)
{
    args = (thread_args_t *)_args;
    printf("Thread %d started.\n", args->id);
    int cnt = 0;

#if defined(PIN) && defined(__linux__)
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
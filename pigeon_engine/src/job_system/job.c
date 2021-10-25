#include <pigeon/job_system/job.h>
#include <pigeon/job_system/threading.h>
#include <pigeon/assert.h>
#include <stddef.h>
#include <string.h>
// #include <stdio.h>

#define MAX_THREADS 64

// main thread data

static unsigned int thread_count;
static PigeonThread threads[MAX_THREADS];

// shared job data

PigeonJob* jobs_array;
unsigned int jobs_count;
unsigned int threads_in_use;

// per-thread objects

static PigeonConditionVariable condition_variables[MAX_THREADS];
static PigeonMutex mutexes[MAX_THREADS];

// atomic flags

static PigeonAtomicInt kill_all_threads;
static PigeonAtomicInt start_executing[MAX_THREADS];
static PigeonAtomicInt errors;

//int getcpu(unsigned int *cpu, unsigned int *node);

static void run_jobs(unsigned int this_thread_index)
{
    for(unsigned int i = this_thread_index; i < jobs_count; i += threads_in_use) {

        //unsigned int cpu, node;
        //getcpu(&cpu, &node);
        // printf("do job %u on thread %u, cpu %u\n", i, this_thread_index, cpu);
        PigeonJob * j = &jobs_array[i];
        int err = j->function(j->arg0, j->arg1);
        // printf("finish job %u on thread %u, cpu %u\n", i, this_thread_index, cpu);
        if(err) {
            pigeon_atomic_inc_int(&errors);
            return;
        }
    }
}

static void thread_start(void* this_thread_index_)
{
    unsigned int this_thread_index = (unsigned int) (uintptr_t) this_thread_index_;

    while(true) {
        // wait for jobs

        while(!pigeon_atomic_get_int(&kill_all_threads) && !pigeon_atomic_get_int(&start_executing[this_thread_index])) {
            pigeon_wait_on_condition_variable(condition_variables[this_thread_index]);
        }
        if(pigeon_atomic_get_int(&kill_all_threads)) break;
        pigeon_atomic_set_int(&start_executing[this_thread_index], 0);
        

        // run jobs

        run_jobs(this_thread_index);

        // tell main thread we are done

        pigeon_release_mutex(mutexes[this_thread_index]);
    }
}


PIGEON_ERR_RET pigeon_init_job_system(unsigned int thread_count_);
PIGEON_ERR_RET pigeon_init_job_system(unsigned int thread_count_)
{
    thread_count = thread_count_;
    if(thread_count > MAX_THREADS) thread_count = MAX_THREADS;
    if(!thread_count) thread_count = 1;

    for(unsigned int i = 1; i < thread_count; i++) {
        condition_variables[i] = pigeon_create_condition_variable();
        mutexes[i] = pigeon_create_mutex();
        threads[i] = pigeon_start_thread(thread_start, (void *) (uintptr_t) i);

        ASSERT_R1(condition_variables[i] && mutexes[i] && threads[i]);
    }
    return 0;
}

void pigeon_deinit_job_system(void);
void pigeon_deinit_job_system(void)
{
    pigeon_atomic_set_int(&kill_all_threads, 1);
    for(unsigned int i = 1; i < thread_count; i++) {
        if(threads[i]) {
            pigeon_join_thread(threads[i]);
        }
        if(condition_variables[i]) {
            pigeon_destroy_condition_variable(condition_variables[i]);
        }
        if(mutexes[i]) {
            pigeon_destroy_mutex(mutexes[i]);
        }
    }
    memset(threads, 0, sizeof threads);
    memset(condition_variables, 0, sizeof condition_variables);
}


PIGEON_ERR_RET pigeon_dispatch_jobs(PigeonJob* jobs, unsigned int n)
{
    if(thread_count == 1) {
        threads_in_use = 1;
        jobs_array = jobs;
        jobs_count = n;
        errors.x = 0;
        run_jobs(0);
        return errors.x;
    }


    // Assign jobs to threads

    unsigned int jobs_per_thread = n / thread_count;
    unsigned int extra_jobs = n % thread_count;
    threads_in_use = jobs_per_thread ? thread_count : extra_jobs;
    jobs_array = jobs;
    jobs_count = n;


    // Wake threads

    pigeon_atomic_set_int(&errors, 0);

    for(unsigned int i = 1; i < threads_in_use; i++) {
        pigeon_aquire_mutex(mutexes[i]);
        pigeon_atomic_set_int(&start_executing[i], 1);
        pigeon_notify_condition_variable(condition_variables[i]);
    }

    // Run thread 0 jobs
    run_jobs(0);


    // Wait on other threads

    for(unsigned int i = 1; i < threads_in_use; i++) {
        pigeon_aquire_mutex(mutexes[i]);
        pigeon_release_mutex(mutexes[i]);
    }

    return pigeon_atomic_get_int(&errors);
}
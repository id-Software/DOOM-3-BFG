/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_jobs.h - parallel job lists (idParallelJobList /
   idParallelJobManager, neo/idlib/ParallelJobList.h/.cpp).

   Phase A (M2): Submit runs the jobs inline on the calling thread, in
   order, honouring sync points trivially; Wait returns at once. Phase B
   (M9) moves execution to SDL worker threads behind the same calls. Jobs
   never touch the file system, decls, cvars or commands (AGENTS.md). */
#ifndef RGD3_JOBS_H
#define RGD3_JOBS_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef void (*Rgd3JobFn)(void* data);

typedef enum Rgd3JobSyncType
{
	RGD3_JOB_SYNC_NONE,
	RGD3_JOB_SYNC_SIGNAL,
	RGD3_JOB_SYNC_SYNCHRONIZE
} Rgd3JobSyncType;

/* jobListId_t */
typedef enum Rgd3JobListId
{
	RGD3_JOBLIST_RENDERER_FRONTEND = 0,
	RGD3_JOBLIST_RENDERER_BACKEND = 1,
	RGD3_JOBLIST_UTILITY = 9, /* no over-time warnings */
	RGD3_MAX_JOBLISTS = 32
} Rgd3JobListId;

typedef enum Rgd3JobListPriority
{
	RGD3_JOBLIST_PRIORITY_NONE,
	RGD3_JOBLIST_PRIORITY_LOW,
	RGD3_JOBLIST_PRIORITY_MEDIUM,
	RGD3_JOBLIST_PRIORITY_HIGH
} Rgd3JobListPriority;

#define RGD3_JOBLIST_PARALLELISM_DEFAULT (-1)
#define RGD3_JOBLIST_PARALLELISM_MAX_CORES (-2)
#define RGD3_JOBLIST_PARALLELISM_MAX_THREADS (-3)

typedef struct Rgd3JobList Rgd3JobList;

void rgd3_jobs_register(void);
void rgd3_jobs_init(void);
void rgd3_jobs_shutdown(void);

/* REGISTER_PARALLEL_JOB: names a job function for timing reports */
void rgd3_jobs_register_job(Rgd3JobFn fn, const char* name);
const char* rgd3_jobs_job_name(Rgd3JobFn fn);

Rgd3JobList* rgd3_jobs_alloc_list(Rgd3JobListId id, Rgd3JobListPriority priority, int max_jobs, int max_syncs);
void rgd3_jobs_free_list(Rgd3JobList* list);
void rgd3_jobs_add_job(Rgd3JobList* list, Rgd3JobFn fn, void* data);
void rgd3_jobs_insert_sync_point(Rgd3JobList* list, Rgd3JobSyncType type);
void rgd3_jobs_submit(Rgd3JobList* list, Rgd3JobList* wait_for, int parallelism);
void rgd3_jobs_wait(Rgd3JobList* list);
int rgd3_jobs_try_wait(Rgd3JobList* list);
int rgd3_jobs_is_submitted(const Rgd3JobList* list);
int rgd3_jobs_num_executed(const Rgd3JobList* list);
u64 rgd3_jobs_total_time_us(const Rgd3JobList* list);
void rgd3_jobs_wait_for_all(void);

#endif

/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_jobs.c - see rgd3_jobs.h. API, limits, cvars and the job registry
   follow neo/idlib/ParallelJobList.cpp (MAX_REGISTERED_JOBS 128,
   jobs_numThreads, jobs_longJobMicroSec, jobs_prioritize); execution is
   phase A: inline, in submission order. */

#define RGD3_MAX_REGISTERED_JOBS 128
#define RGD3_MAX_JOB_THREADS 32

static Rgd3Cvar cv_jobs_numThreads = RGD3_CVAR_RANGE("jobs_numThreads", "2", RGD3_CVAR_INTEGER | RGD3_CVAR_NOCHEAT,
	"number of threads used to crunch through jobs", 0, RGD3_MAX_JOB_THREADS);
static Rgd3Cvar cv_jobs_longJobMicroSec = RGD3_CVAR("jobs_longJobMicroSec", "10000", RGD3_CVAR_INTEGER,
	"print a warning for jobs that take more than this number of microseconds");
static Rgd3Cvar cv_jobs_prioritize = RGD3_CVAR("jobs_prioritize", "1", RGD3_CVAR_BOOL | RGD3_CVAR_NOCHEAT, "prioritize job lists");

typedef struct Rgd3Job
{
	Rgd3JobFn fn; /* NULL marks a sync point */
	void* data;
	Rgd3JobSyncType sync;
} Rgd3Job;

struct Rgd3JobList
{
	Rgd3JobListId id;
	Rgd3JobListPriority priority;
	Rgd3Job* jobs;
	int num_jobs;
	int max_jobs;
	int num_syncs;
	int max_syncs;
	int submitted;
	int done;
	int executed;
	u64 submit_us;
	u64 finish_us;
	int in_use;
};

static struct
{
	struct
	{
		Rgd3JobFn fn;
		const char* name;
	} registered[RGD3_MAX_REGISTERED_JOBS];
	int num_registered;
	Rgd3JobList lists[RGD3_MAX_JOBLISTS];
} rgd3_jobs;

void rgd3_jobs_register_job(Rgd3JobFn fn, const char* name)
{
	int i;
	for (i = 0; i < rgd3_jobs.num_registered; i++)
	{
		if (rgd3_jobs.registered[i].fn == fn)
			return;
	}
	if (rgd3_jobs.num_registered >= RGD3_MAX_REGISTERED_JOBS)
	{
		rgd3_warning("rgd3_jobs_register_job: too many jobs");
		return;
	}
	rgd3_jobs.registered[rgd3_jobs.num_registered].fn = fn;
	rgd3_jobs.registered[rgd3_jobs.num_registered].name = name;
	rgd3_jobs.num_registered++;
}

const char* rgd3_jobs_job_name(Rgd3JobFn fn)
{
	int i;
	for (i = 0; i < rgd3_jobs.num_registered; i++)
	{
		if (rgd3_jobs.registered[i].fn == fn)
			return rgd3_jobs.registered[i].name;
	}
	return "unknown";
}

Rgd3JobList* rgd3_jobs_alloc_list(Rgd3JobListId id, Rgd3JobListPriority priority, int max_jobs, int max_syncs)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOBLISTS; i++)
	{
		Rgd3JobList* list = &rgd3_jobs.lists[i];
		if (list->in_use)
			continue;
		memset(list, 0, sizeof(*list));
		list->in_use = 1;
		list->id = id;
		list->priority = priority;
		list->max_jobs = max_jobs;
		list->max_syncs = max_syncs;
		list->jobs = (Rgd3Job*)rgd3_heap_calloc((size_t)(max_jobs + max_syncs + 1) * sizeof(Rgd3Job), RGD3_TAG_SYSTEM);
		return list;
	}
	rgd3_error_fatal("rgd3_jobs_alloc_list: no free job lists");
	return NULL;
}

void rgd3_jobs_free_list(Rgd3JobList* list)
{
	if (!list || !list->in_use)
		return;
	rgd3_jobs_wait(list);
	rgd3_heap_free(list->jobs);
	memset(list, 0, sizeof(*list));
}

void rgd3_jobs_add_job(Rgd3JobList* list, Rgd3JobFn fn, void* data)
{
	RG_ASSERT(!list->submitted);
	if (list->num_jobs >= list->max_jobs)
		rgd3_error_fatal("rgd3_jobs_add_job: more than %d jobs in list %d", list->max_jobs, (int)list->id);
	list->jobs[list->num_jobs + list->num_syncs].fn = fn;
	list->jobs[list->num_jobs + list->num_syncs].data = data;
	list->jobs[list->num_jobs + list->num_syncs].sync = RGD3_JOB_SYNC_NONE;
	list->num_jobs++;
}

void rgd3_jobs_insert_sync_point(Rgd3JobList* list, Rgd3JobSyncType type)
{
	RG_ASSERT(!list->submitted);
	if (list->num_syncs >= list->max_syncs)
		rgd3_error_fatal("rgd3_jobs_insert_sync_point: more than %d syncs in list %d", list->max_syncs, (int)list->id);
	list->jobs[list->num_jobs + list->num_syncs].fn = NULL;
	list->jobs[list->num_jobs + list->num_syncs].data = NULL;
	list->jobs[list->num_jobs + list->num_syncs].sync = type;
	list->num_syncs++;
}

/* Phase A: run everything now, in order; sync points are satisfied by
   construction. The list may be refilled after the next Wait. */
void rgd3_jobs_submit(Rgd3JobList* list, Rgd3JobList* wait_for, int parallelism)
{
	int i, total = list->num_jobs + list->num_syncs;
	(void)parallelism;
	RG_ASSERT(!list->submitted);
	if (wait_for)
		rgd3_jobs_wait(wait_for);
	list->submitted = 1;
	list->submit_us = rgd3_time_us();
	for (i = 0; i < total; i++)
	{
		Rgd3Job* job = &list->jobs[i];
		u64 start;
		if (!job->fn)
			continue;
		start = rgd3_time_us();
		job->fn(job->data);
		list->executed++;
		if (list->id != RGD3_JOBLIST_UTILITY && (rgd3_time_us() - start) > (u64)cv_jobs_longJobMicroSec.integer_value)
			rgd3_dwarning("job %s took %llu us", rgd3_jobs_job_name(job->fn), (unsigned long long)(rgd3_time_us() - start));
	}
	list->finish_us = rgd3_time_us();
	list->done = 1;
}

/* waiting resets the list for the next fill, as the original does */
void rgd3_jobs_wait(Rgd3JobList* list)
{
	if (!list->submitted)
		return;
	list->submitted = 0;
	list->done = 0;
	list->num_jobs = 0;
	list->num_syncs = 0;
}

int rgd3_jobs_try_wait(Rgd3JobList* list)
{
	if (list->submitted && list->done)
		rgd3_jobs_wait(list);
	return !list->submitted;
}

int rgd3_jobs_is_submitted(const Rgd3JobList* list)
{
	return list->submitted;
}

int rgd3_jobs_num_executed(const Rgd3JobList* list)
{
	return list->executed;
}

u64 rgd3_jobs_total_time_us(const Rgd3JobList* list)
{
	return list->finish_us - list->submit_us;
}

void rgd3_jobs_wait_for_all(void)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOBLISTS; i++)
	{
		if (rgd3_jobs.lists[i].in_use)
			rgd3_jobs_wait(&rgd3_jobs.lists[i]);
	}
}

void rgd3_jobs_init(void)
{
}

void rgd3_jobs_shutdown(void)
{
	int i;
	for (i = 0; i < RGD3_MAX_JOBLISTS; i++)
		rgd3_jobs_free_list(&rgd3_jobs.lists[i]);
	rgd3_jobs.num_registered = 0;
}

void rgd3_jobs_register(void)
{
	static Rgd3Cvar* const cvars[] = { &cv_jobs_numThreads, &cv_jobs_longJobMicroSec, &cv_jobs_prioritize };
	rgd3_cvar_register_table(cvars, (int)RG_ARRAY_COUNT(cvars));
}

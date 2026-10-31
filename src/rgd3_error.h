/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_error.h - error recovery without C++ exceptions.

   The original throws idException from common->Error() and catches it in
   Frame()/Init() (neo/framework/Common_printf.cpp). rgd3 keeps a stack of
   setjmp guards instead:

	Rgd3ErrorGuard guard;
	rgd3_error_push(&guard);
	if (setjmp(guard.jump) == 0)
	{
		work that may call rgd3_error_drop()
	}
	else
	{
		recovery; the guard is already popped
	}
	rgd3_error_pop(&guard);

   Locals modified between setjmp and longjmp must be volatile. Never longjmp
   across threads: workers record failure and the main thread re-raises it. */
#ifndef RGD3_ERROR_H
#define RGD3_ERROR_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3ErrorGuard
{
	jmp_buf jump;
	struct Rgd3ErrorGuard* prev;
	u64 unwind_seq; /* registry position at push */
} Rgd3ErrorGuard;

typedef void (*Rgd3ErrorHook)(void);

/* Unwind registry: cleanup that survives longjmp. An object that owns
   memory registers itself while it is alive (lexers, parsers and long
   tokens do this in init / on growth and unregister in their free). Only
   objects on the main thread's stack are recorded: heap objects belong to
   their owners, and other threads never drop.

   A drop frees, through their free functions, every recorded object that
   was registered after its guard was pushed: objects in the functions the
   drop leaves, including ones declared inside the guarded block itself.
   The rule is by time, not by frame, so a stack object that lives in a
   surviving frame (the guard's function or one of its callers) but was
   registered after the push is emptied too; free functions leave objects
   empty and valid, and freeing them again is harmless. Objects registered
   before the push are left alone. A registered object must never go out
   of scope without its free call. */
typedef void (*Rgd3UnwindFn)(void* object);
void rgd3_error_unwind_register(void* object, Rgd3UnwindFn fn);
void rgd3_error_unwind_unregister(void* object);
int rgd3_error_unwind_count(void);
void rgd3_error_reset_drop_count(void); /* tests only */
void rgd3_error_shutdown(void);

void rgd3_error_register(void);
void rgd3_error_set_drop_hook(Rgd3ErrorHook hook);
void rgd3_error_set_fatal_hook(Rgd3ErrorHook hook);
void rgd3_error_push(Rgd3ErrorGuard* guard);
void rgd3_error_pop(Rgd3ErrorGuard* guard);
const char* rgd3_error_message(void);

RGD3_NORETURN void rgd3_error_drop(const char* fmt, ...);
RGD3_NORETURN void rgd3_error_fatal(const char* fmt, ...);

#endif

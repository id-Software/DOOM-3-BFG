/* 

TiMidity -- Experimental MIDI to WAVE converter
Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

sdl_c.c
Minimal control mode -- no interaction, just stores messages.
*/

#include "precompiled.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "instrum.h"
#include "playmidi.h"

static void ctl_refresh(void);
static void ctl_total_time(int tt);
static void ctl_master_volume(int mv);
static void ctl_file_name(char *name);
static void ctl_current_time(int ct);
static void ctl_note(int v);
static void ctl_program(int ch, int val);
static void ctl_volume(int channel, int val);
static void ctl_expression(int channel, int val);
static void ctl_panning(int channel, int val);
static void ctl_sustain(int channel, int val);
static void ctl_pitch_bend(int channel, int val);
static void ctl_reset(void);
static int ctl_open(int using_stdin, int using_stdout);
static void ctl_close(void);
static int ctl_read(int *valp);
static int cmsg(int type, int verbosity_level, const char *fmt, ...);

#ifdef _DEBUG
#define safeOutputDebug(x) printf( "%s", x )
#else
#define safeOutputDebug(x)
#endif

/**********************************/
/* export the interface functions */

#define ctl sdl_control_mode

ControlMode ctl= 
{
	"SDL interface", 's',
		1,0,0,
		ctl_open,NULL, ctl_close, ctl_read, cmsg,
		ctl_refresh, ctl_reset, ctl_file_name, ctl_total_time, ctl_current_time, 
		ctl_note, 
		ctl_master_volume, ctl_program, ctl_volume, 
		ctl_expression, ctl_panning, ctl_sustain, ctl_pitch_bend
};

static int ctl_open(int using_stdin, int using_stdout)
{
	ctl.opened=1;
	return 0;
}

static void ctl_close(void)
{ 
	ctl.opened=0;
}

static int ctl_read(int *valp)
{
	return RC_NO_RETURN_VALUE;
}
extern void SendDebugMsg(const char*);
extern bool debugOutput;
static int cmsg(int type, int verbosity_level, const char *fmt, ...)
{
#ifdef _DEBUG
	va_list ap;

	va_start(ap, fmt);
	idStr::vsnPrintf(timidity_error, TIMIDITY_ERROR_MAX_CHARS - 1, fmt, ap);
	va_end(ap);

	strcat( timidity_error, "\n" );

	if ( debugOutput && verbosity_level <= VERB_VERBOSE ) {
		safeOutputDebug( timidity_error );
	}
#endif

	return 0;
}

static void ctl_refresh(void) { }

static void ctl_total_time(int tt) {}

static void ctl_master_volume(int mv) {}

static void ctl_file_name(char *name) {}

static void ctl_current_time(int ct) {}

static void ctl_note(int v) {}

static void ctl_program(int ch, int val) {}

static void ctl_volume(int channel, int val) {}

static void ctl_expression(int channel, int val) {}

static void ctl_panning(int channel, int val) {}

static void ctl_sustain(int channel, int val) {}

static void ctl_pitch_bend(int channel, int val) {}

static void ctl_reset(void) {}

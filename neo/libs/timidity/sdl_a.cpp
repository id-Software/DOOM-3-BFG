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

sdl_a.c

Functions to output RIFF WAVE format data to a file or stdout.

*/

#include "config.h"
#include "output.h"

/* export the playback mode */

#define dpm sdl_play_mode

static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static void output_data(int *buf, int count, int* bytes_written);
static void flush_output(void);
static void purge_output(void);


PlayMode dpm = {
	DEFAULT_RATE, PE_16BIT|PE_SIGNED,
		"SDL audio", 0, "d:\\out.wav",

		open_output,
		close_output,
		output_data,
		flush_output,
		purge_output  
};

/* Dummies */
static int open_output(void){
	return 0;
}

static void output_data(int *buf, int count, int* bytes_written){}
static void close_output(void){}
static void flush_output(void) { }
static void purge_output(void) { }

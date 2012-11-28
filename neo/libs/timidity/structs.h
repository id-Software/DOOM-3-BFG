/*

TiMidity -- Experimental MIDI to WAVE converter
Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

In case you haven't heard, this program is free software; 
you can redistribute it and/or modify it under the terms of the
GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option)
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

structs.h

*/
#ifndef TIMIDITY_STRUCTS_H
#define TIMIDITY_STRUCTS_H

#include <stdint.h>

typedef struct {
	int32_t time;
	uint8_t channel, type, a, b;
} MidiEvent;

struct _MidiSong {
	int32_t samples;
	MidiEvent *events;
};


#endif
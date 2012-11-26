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

readmidi.h 

*/

class idFile;

typedef struct {
	MidiEvent event;
	void *next;
} MidiEventList;

extern  int32_t quietchannels;

extern MidiEvent *read_midi_file(idFile * mfp,  int32_t *count,  int32_t *sp);
extern MidiEvent *read_midi_buffer(unsigned char* buffer, size_t length,  int32_t *count,  int32_t *sp);

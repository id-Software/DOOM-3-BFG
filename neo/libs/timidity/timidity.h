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

*/

#ifndef _TIMIDITY_H_
#define _TIMIDITY_H_

/* Audio format flags (defaults to LSB byte order) */
#define AUDIO_U8	0x0008	/* Unsigned 8-bit samples */
#define AUDIO_S8	0x8008	/* Signed 8-bit samples */
#define AUDIO_U16LSB	0x0010	/* Unsigned 16-bit samples */
#define AUDIO_S16LSB	0x8010	/* Signed 16-bit samples */
#define AUDIO_U16MSB	0x1010	/* As above, but big-endian byte order */
#define AUDIO_S16MSB	0x9010	/* As above, but big-endian byte order */
#define AUDIO_U16	AUDIO_U16LSB
#define AUDIO_S16	AUDIO_S16LSB

#include "structs.h"

typedef struct _MidiSong MidiSong;

extern int Timidity_Init(int rate, int format, int channels, int samples, const char* config);
extern char *Timidity_Error(void);
extern void Timidity_SetVolume(int volume);
extern int Timidity_PlaySome(void *stream, int samples, int* bytes_written);
extern MidiSong *Timidity_LoadSong(char *midifile);
extern MidiSong *Timidity_LoadSongMem(unsigned char* buffer, size_t length);
extern void Timidity_Start(MidiSong *song);
extern int Timidity_Active(void);
extern void Timidity_Stop(void);
extern void Timidity_FreeSong(MidiSong *song);
extern void Timidity_Shutdown(void);

extern void *Real_Tim_Malloc( int sz );
extern void Real_Tim_Free( void *pt );


typedef struct {
	int rate, encoding;
	const char *id_name;
	FILE* fp;
	const char *file_name;

	int (*open_output)(void); /* 0=success, 1=warning, -1=fatal error */
	void (*close_output)(void);
	void (*output_data)(int *buf, int count, int* bytes_written);
	void (*flush_output)(void);
	void (*purge_output)(void);
} PlayMode;

extern PlayMode *play_mode_list[], *play_mode;

#endif
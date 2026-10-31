/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_sound_stub.c - sound entry points until the mixer exists (M8).
   Replaced in all.c by the real module. */

void rgd3_sound_begin_level_load(void)
{
	rgd3_hooks_trace("sound_begin_level_load");
}

void rgd3_sound_end_level_load(void)
{
	rgd3_hooks_trace("sound_end_level_load");
}

void rgd3_sound_preload(const Rgd3PreloadManifest* manifest)
{
	(void)manifest;
	rgd3_hooks_trace("sound_preload");
}

void rgd3_sound_set_paused(int paused)
{
	(void)paused; /* called every frame: not traced */
}

void rgd3_sound_stop_all(void)
{
	rgd3_hooks_trace("sound_stop_all");
}

void rgd3_sound_render(void)
{
	/* called every frame: not traced */
}

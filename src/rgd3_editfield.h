/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_editfield.h - single-line text editing with Tab completion
   (idEditField, neo/framework/EditField.cpp/.h). Used by the console input
   line; later by GUI edit windows.

   Completion asks the command and cvar systems for names
   (rgd3_cmd_command_completion / rgd3_cvar_command_completion), then for
   argument values (rgd3_cmd_arg_completion / rgd3_cvar_arg_completion), as
   the original does. A second Tab with several matches cycles through
   them. */
#ifndef RGD3_EDITFIELD_H
#define RGD3_EDITFIELD_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_MAX_EDIT_LINE 256

typedef struct Rgd3AutoComplete
{
	int valid;
	int length;
	char completion_string[RGD3_MAX_EDIT_LINE];
	char current_match[RGD3_MAX_EDIT_LINE];
	int match_count;
	int match_index;
	int find_match_index;
} Rgd3AutoComplete;

typedef struct Rgd3EditField
{
	int cursor;
	int scroll;
	int width_in_chars;
	char buffer[RGD3_MAX_EDIT_LINE];
	Rgd3AutoComplete auto_complete;
} Rgd3EditField;

void rgd3_editfield_clear(Rgd3EditField* f);
void rgd3_editfield_set_width_in_chars(Rgd3EditField* f, int width);
void rgd3_editfield_set_cursor(Rgd3EditField* f, int cursor);
int rgd3_editfield_cursor(const Rgd3EditField* f);
void rgd3_editfield_clear_auto_complete(Rgd3EditField* f);
int rgd3_editfield_auto_complete_length(const Rgd3EditField* f);
void rgd3_editfield_auto_complete(Rgd3EditField* f);
void rgd3_editfield_char_event(Rgd3EditField* f, int ch);
void rgd3_editfield_key_down_event(Rgd3EditField* f, int key);
void rgd3_editfield_paste(Rgd3EditField* f);
const char* rgd3_editfield_buffer(const Rgd3EditField* f);
void rgd3_editfield_set_buffer(Rgd3EditField* f, const char* text);

/* the visible slice for a field drawn width_in_chars wide (idEditField::Draw
   without the drawing): copies it to out and returns the cursor column in
   it, -1 when the cursor is not visible */
int rgd3_editfield_visible(Rgd3EditField* f, char* out, int out_size);

/* idKeyInput::GetOverstrikeMode / SetOverstrikeMode (Insert toggles it) */
int rgd3_editfield_overstrike(void);
void rgd3_editfield_set_overstrike(int on);

#endif

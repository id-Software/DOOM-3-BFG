/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_editfield.c - structure and behaviour follow
   neo/framework/EditField.cpp: FindMatches / FindIndexMatch / PrintMatches /
   PrintCvarMatches callbacks over a file-static completion state (the
   callbacks carry no user pointer), AutoComplete's two passes (names, then
   arguments when there is one match or an argument was typed), cycling on
   repeated Tab, CharEvent control codes, KeyDownEvent cursor moves.

   Deviations: SDL text input never delivers control characters, so the
   console maps Backspace and Ctrl+letter keys onto the CharEvent codes;
   characters above 126 are ignored (the buffer is ASCII); the cursor column
   skips colour escapes between the scroll point and the cursor (the
   original's loop indexed the visible copy from the buffer start). */

static Rgd3AutoComplete rgd3_global_auto_complete;
static int rgd3_editfield_overstrike_mode;

int rgd3_editfield_overstrike(void)
{
	return rgd3_editfield_overstrike_mode;
}

void rgd3_editfield_set_overstrike(int on)
{
	rgd3_editfield_overstrike_mode = on != 0;
}

/* ---------------------------------------------------------------------------
   Completion callbacks
--------------------------------------------------------------------------- */

static int rgd3_editfield_lower(char c)
{
	return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : (unsigned char)c;
}

static void rgd3_editfield_find_matches(const char* s)
{
	Rgd3AutoComplete* ac = &rgd3_global_auto_complete;
	int i;
	if (rgd3_str_icmpn(s, ac->completion_string, strlen(ac->completion_string)) != 0)
		return;
	ac->match_count++;
	if (ac->match_count == 1)
	{
		rgd3_str_copynz(ac->current_match, s, sizeof(ac->current_match));
		return;
	}
	/* cut current_match to the part common with s */
	for (i = 0; s[i]; i++)
	{
		if (rgd3_editfield_lower(ac->current_match[i]) != rgd3_editfield_lower(s[i]))
		{
			ac->current_match[i] = 0;
			break;
		}
	}
	ac->current_match[i] = 0;
}

static void rgd3_editfield_find_index_match(const char* s)
{
	Rgd3AutoComplete* ac = &rgd3_global_auto_complete;
	if (rgd3_str_icmpn(s, ac->completion_string, strlen(ac->completion_string)) != 0)
		return;
	if (ac->find_match_index == ac->match_index)
		rgd3_str_copynz(ac->current_match, s, sizeof(ac->current_match));
	ac->find_match_index++;
}

static void rgd3_editfield_print_matches(const char* s)
{
	const char* match = rgd3_global_auto_complete.current_match;
	if (rgd3_str_icmpn(s, match, strlen(match)) == 0)
		rgd3_printf("    %s\n", s);
}

static void rgd3_editfield_print_cvar_matches(const char* s)
{
	const char* match = rgd3_global_auto_complete.current_match;
	if (rgd3_str_icmpn(s, match, strlen(match)) == 0)
		rgd3_printf("    %s^7 = \"%s\"\n", s, rgd3_cvar_get_string(s));
}

/* ---------------------------------------------------------------------------
   Field
--------------------------------------------------------------------------- */

void rgd3_editfield_clear(Rgd3EditField* f)
{
	f->buffer[0] = 0;
	f->cursor = 0;
	f->scroll = 0;
	f->auto_complete.length = 0;
	f->auto_complete.valid = 0;
}

void rgd3_editfield_set_width_in_chars(Rgd3EditField* f, int width)
{
	RG_ASSERT(width <= RGD3_MAX_EDIT_LINE);
	f->width_in_chars = width;
}

void rgd3_editfield_set_cursor(Rgd3EditField* f, int cursor)
{
	RG_ASSERT(cursor <= RGD3_MAX_EDIT_LINE);
	f->cursor = cursor;
}

int rgd3_editfield_cursor(const Rgd3EditField* f)
{
	return f->cursor;
}

const char* rgd3_editfield_buffer(const Rgd3EditField* f)
{
	return f->buffer;
}

void rgd3_editfield_set_buffer(Rgd3EditField* f, const char* text)
{
	rgd3_editfield_clear(f);
	rgd3_str_copynz(f->buffer, text, sizeof(f->buffer));
	rgd3_editfield_set_cursor(f, (int)strlen(f->buffer));
}

void rgd3_editfield_clear_auto_complete(Rgd3EditField* f)
{
	Rgd3AutoComplete* ac = &f->auto_complete;
	if (ac->length > 0 && ac->length <= (int)strlen(f->buffer))
	{
		f->buffer[ac->length] = 0;
		if (f->cursor > ac->length)
			f->cursor = ac->length;
	}
	ac->length = 0;
	ac->valid = 0;
}

int rgd3_editfield_auto_complete_length(const Rgd3EditField* f)
{
	return f->auto_complete.length;
}

void rgd3_editfield_auto_complete(Rgd3EditField* f)
{
	char completion_arg_string[RGD3_MAX_EDIT_LINE];
	Rgd3AutoComplete* ac = &f->auto_complete;
	Rgd3CmdArgs args;

	if (!ac->valid)
	{
		rgd3_cmdargs_tokenize(&args, f->buffer, 0);
		rgd3_str_copynz(ac->completion_string, rgd3_cmdargs_argv(&args, 0), sizeof(ac->completion_string));
		rgd3_str_copynz(completion_arg_string, rgd3_cmdargs_args(&args, 1, -1, 0), sizeof(completion_arg_string));
		ac->match_count = 0;
		ac->match_index = 0;
		ac->current_match[0] = 0;

		if (!ac->completion_string[0])
			return;

		rgd3_global_auto_complete = *ac;
		rgd3_cmd_command_completion(rgd3_editfield_find_matches);
		rgd3_cvar_command_completion(rgd3_editfield_find_matches);
		*ac = rgd3_global_auto_complete;

		if (ac->match_count == 0)
			return; /* no matches */

		/* one match, or an argument was typed: try completing arguments */
		if (ac->match_count == 1 || completion_arg_string[0])
		{
			rgd3_str_append(ac->completion_string, sizeof(ac->completion_string), " ");
			rgd3_str_append(ac->completion_string, sizeof(ac->completion_string), completion_arg_string);
			ac->match_count = 0;

			rgd3_global_auto_complete = *ac;
			rgd3_cmd_arg_completion(ac->completion_string, rgd3_editfield_find_matches);
			rgd3_cvar_arg_completion(ac->completion_string, rgd3_editfield_find_matches);
			*ac = rgd3_global_auto_complete;

			rg_snprintf(f->buffer, sizeof(f->buffer), "%s", ac->current_match);

			if (ac->match_count == 0)
			{
				/* no argument matches */
				rgd3_str_append(f->buffer, sizeof(f->buffer), " ");
				rgd3_str_append(f->buffer, sizeof(f->buffer), completion_arg_string);
				rgd3_editfield_set_cursor(f, (int)strlen(f->buffer));
				return;
			}
		}
		else
		{
			/* several matches: complete to the shortest common prefix */
			rg_snprintf(f->buffer, sizeof(f->buffer), "%s", ac->current_match);
			if (completion_arg_string[0])
			{
				rgd3_str_append(f->buffer, sizeof(f->buffer), " ");
				rgd3_str_append(f->buffer, sizeof(f->buffer), completion_arg_string);
			}
		}

		ac->length = (int)strlen(f->buffer);
		ac->valid = ac->match_count != 1;
		rgd3_editfield_set_cursor(f, ac->length);

		rgd3_printf("]%s\n", f->buffer);

		/* run through again, printing the matches */
		rgd3_global_auto_complete = *ac;
		rgd3_cmd_command_completion(rgd3_editfield_print_matches);
		rgd3_cmd_arg_completion(ac->completion_string, rgd3_editfield_print_matches);
		rgd3_cvar_command_completion(rgd3_editfield_print_cvar_matches);
		rgd3_cvar_arg_completion(ac->completion_string, rgd3_editfield_print_matches);
	}
	else if (ac->match_count != 1)
	{
		/* show the next match instead */
		ac->match_index++;
		if (ac->match_index == ac->match_count)
			ac->match_index = 0;
		ac->find_match_index = 0;

		rgd3_global_auto_complete = *ac;
		rgd3_cmd_command_completion(rgd3_editfield_find_index_match);
		rgd3_cmd_arg_completion(ac->completion_string, rgd3_editfield_find_index_match);
		rgd3_cvar_command_completion(rgd3_editfield_find_index_match);
		rgd3_cvar_arg_completion(ac->completion_string, rgd3_editfield_find_index_match);
		*ac = rgd3_global_auto_complete;

		rg_snprintf(f->buffer, sizeof(f->buffer), "%s", ac->current_match);
		if (ac->length > (int)strlen(f->buffer))
			ac->length = (int)strlen(f->buffer);
		rgd3_editfield_set_cursor(f, ac->length);
	}
}

#define RGD3_CTRL(c) ((c) - 'a' + 1)

void rgd3_editfield_char_event(Rgd3EditField* f, int ch)
{
	int len;

	if (ch == RGD3_CTRL('v'))
	{
		rgd3_editfield_paste(f);
		return;
	}
	if (ch == RGD3_CTRL('c'))
	{
		rgd3_editfield_clear(f);
		return;
	}

	len = (int)strlen(f->buffer);

	if (ch == RGD3_CTRL('h') || ch == RGD3_K_BACKSPACE)
	{
		if (f->cursor > 0)
		{
			memmove(f->buffer + f->cursor - 1, f->buffer + f->cursor, (size_t)(len + 1 - f->cursor));
			f->cursor--;
			if (f->cursor < f->scroll)
				f->scroll--;
		}
		return;
	}
	if (ch == RGD3_CTRL('a'))
	{
		f->cursor = 0;
		f->scroll = 0;
		return;
	}
	if (ch == RGD3_CTRL('e'))
	{
		f->cursor = len;
		f->scroll = f->cursor - f->width_in_chars;
		return;
	}

	/* ignore any other non-printable character */
	if (ch < 32 || ch > 126)
		return;

	if (rgd3_editfield_overstrike_mode)
	{
		if (f->cursor == RGD3_MAX_EDIT_LINE - 1)
			return;
		f->buffer[f->cursor] = (char)ch;
		f->cursor++;
	}
	else
	{
		if (len == RGD3_MAX_EDIT_LINE - 1)
			return; /* full */
		memmove(f->buffer + f->cursor + 1, f->buffer + f->cursor, (size_t)(len + 1 - f->cursor));
		f->buffer[f->cursor] = (char)ch;
		f->cursor++;
	}

	if (f->cursor >= f->width_in_chars)
		f->scroll++;
	if (f->cursor == len + 1)
		f->buffer[f->cursor] = 0;
}

static int rgd3_editfield_ctrl_down(void)
{
	return rgd3_keys_is_down(RGD3_K_LCTRL) || rgd3_keys_is_down(RGD3_K_RCTRL);
}

void rgd3_editfield_key_down_event(Rgd3EditField* f, int key)
{
	int len;

	/* shift-insert is paste */
	if ((key == RGD3_K_INS || key == RGD3_K_KP_0) && (rgd3_keys_is_down(RGD3_K_LSHIFT) || rgd3_keys_is_down(RGD3_K_RSHIFT)))
	{
		rgd3_editfield_clear_auto_complete(f);
		rgd3_editfield_paste(f);
		return;
	}

	len = (int)strlen(f->buffer);

	if (key == RGD3_K_DEL)
	{
		if (f->auto_complete.length)
			rgd3_editfield_clear_auto_complete(f);
		else if (f->cursor < len)
			memmove(f->buffer + f->cursor, f->buffer + f->cursor + 1, (size_t)(len - f->cursor));
		return;
	}

	if (key == RGD3_K_RIGHTARROW)
	{
		if (rgd3_editfield_ctrl_down())
		{
			/* skip to the next word */
			while (f->cursor < len && f->buffer[f->cursor] != ' ')
				f->cursor++;
			while (f->cursor < len && f->buffer[f->cursor] == ' ')
				f->cursor++;
		}
		else
		{
			f->cursor++;
		}
		if (f->cursor > len)
			f->cursor = len;
		if (f->cursor >= f->scroll + f->width_in_chars)
			f->scroll = f->cursor - f->width_in_chars + 1;
		if (f->auto_complete.length > 0)
			f->auto_complete.length = f->cursor;
		return;
	}

	if (key == RGD3_K_LEFTARROW)
	{
		if (rgd3_editfield_ctrl_down())
		{
			/* skip to the previous word */
			while (f->cursor > 0 && f->buffer[f->cursor - 1] == ' ')
				f->cursor--;
			while (f->cursor > 0 && f->buffer[f->cursor - 1] != ' ')
				f->cursor--;
		}
		else
		{
			f->cursor--;
		}
		if (f->cursor < 0)
			f->cursor = 0;
		if (f->cursor < f->scroll)
			f->scroll = f->cursor;
		if (f->auto_complete.length)
			f->auto_complete.length = f->cursor;
		return;
	}

	if (key == RGD3_K_HOME || (key == RGD3_K_A && rgd3_editfield_ctrl_down()))
	{
		f->cursor = 0;
		f->scroll = 0;
		if (f->auto_complete.length)
		{
			f->auto_complete.length = f->cursor;
			f->auto_complete.valid = 0;
		}
		return;
	}

	if (key == RGD3_K_END || (key == RGD3_K_E && rgd3_editfield_ctrl_down()))
	{
		f->cursor = len;
		if (f->cursor >= f->scroll + f->width_in_chars)
			f->scroll = f->cursor - f->width_in_chars + 1;
		if (f->auto_complete.length)
		{
			f->auto_complete.length = f->cursor;
			f->auto_complete.valid = 0;
		}
		return;
	}

	if (key == RGD3_K_INS)
	{
		rgd3_editfield_set_overstrike(!rgd3_editfield_overstrike_mode);
		return;
	}

	/* any other key clears the completion */
	if (key != RGD3_K_CAPSLOCK && key != RGD3_K_LALT && key != RGD3_K_LCTRL && key != RGD3_K_LSHIFT &&
		key != RGD3_K_RALT && key != RGD3_K_RCTRL && key != RGD3_K_RSHIFT)
		rgd3_editfield_clear_auto_complete(f);
}

/* Sys_GetClipboardData through SDL; sent as typed so insert/overstrike apply. */
void rgd3_editfield_paste(Rgd3EditField* f)
{
	char* cbd = SDL_GetClipboardText();
	const char* p;
	if (!cbd)
		return;
	for (p = cbd; *p; ++p)
		rgd3_editfield_char_event(f, (unsigned char)*p);
	SDL_free(cbd);
}

int rgd3_editfield_visible(Rgd3EditField* f, char* out, int out_size)
{
	int draw_len = f->width_in_chars;
	int len = (int)strlen(f->buffer) + 1;
	int prestep, i, column;

	/* guarantee that the cursor is visible: the incremental scroll updates
	   assume a fixed width, so a resized console or a recalled long line
	   can leave the cursor outside [scroll, scroll + draw_len) */
	if (len <= draw_len || draw_len <= 0)
	{
		prestep = 0;
	}
	else
	{
		if (f->cursor >= f->scroll + draw_len)
			f->scroll = f->cursor - draw_len + 1;
		if (f->cursor < f->scroll)
			f->scroll = f->cursor;
		if (f->scroll + draw_len > len)
			f->scroll = len - draw_len;
		if (f->scroll < 0)
			f->scroll = 0;
		prestep = f->scroll;
		/* skip a colour code, unless that would hide the cursor */
		if (rgd3_str_is_color(f->buffer + prestep) && prestep + 2 <= f->cursor)
			prestep += 2;
		if (prestep > 0 && prestep < f->cursor && rgd3_str_is_color(f->buffer + prestep - 1))
			prestep++;
	}
	if (prestep > len - 1)
		prestep = len - 1;
	if (prestep + draw_len > len)
		draw_len = len - prestep;
	if (draw_len >= out_size)
		draw_len = out_size - 1;
	if (draw_len < 0)
		draw_len = 0;
	memcpy(out, f->buffer + prestep, (size_t)draw_len);
	out[draw_len] = 0;

	if (f->cursor < prestep)
		return -1;
	column = f->cursor - prestep;
	for (i = prestep; i < f->cursor && f->buffer[i]; i++)
	{
		if (rgd3_str_is_color(f->buffer + i))
		{
			i++;
			column -= 2;
		}
	}
	return column < 0 ? 0 : column;
}

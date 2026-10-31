/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_str.c - see rgd3_str.h. */

static int rgd3_str_lower_char(int c)
{
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
}

int rgd3_str_icmp(const char* a, const char* b)
{
	for (;;)
	{
		int ca = rgd3_str_lower_char((unsigned char)*a++);
		int cb = rgd3_str_lower_char((unsigned char)*b++);
		if (ca != cb)
			return ca < cb ? -1 : 1;
		if (!ca)
			return 0;
	}
}

int rgd3_str_icmpn(const char* a, const char* b, size_t n)
{
	while (n--)
	{
		int ca = rgd3_str_lower_char((unsigned char)*a++);
		int cb = rgd3_str_lower_char((unsigned char)*b++);
		if (ca != cb)
			return ca < cb ? -1 : 1;
		if (!ca)
			return 0;
	}
	return 0;
}

int rgd3_str_icmp_prefix(const char* text, const char* prefix)
{
	return rgd3_str_icmpn(text, prefix, strlen(prefix)) == 0;
}

int rgd3_str_iends_with(const char* text, const char* suffix)
{
	size_t tl = strlen(text);
	size_t sl = strlen(suffix);
	return sl <= tl && rgd3_str_icmp(text + tl - sl, suffix) == 0;
}

void rgd3_str_copynz(char* dst, const char* src, size_t dst_size)
{
	size_t len;
	if (!dst || !dst_size)
		return;
	if (!src)
	{
		dst[0] = 0;
		return;
	}
	len = strlen(src);
	if (len >= dst_size)
		len = dst_size - 1;
	memcpy(dst, src, len);
	dst[len] = 0;
}

void rgd3_str_append(char* dst, size_t dst_size, const char* src)
{
	size_t len = strlen(dst);
	if (len < dst_size)
		rgd3_str_copynz(dst + len, src, dst_size - len);
}

char* rgd3_str_to_lower(char* text)
{
	char* p;
	for (p = text; *p; ++p)
		*p = (char)rgd3_str_lower_char((unsigned char)*p);
	return text;
}

void rgd3_str_backslashes_to_slashes(char* text)
{
	for (; *text; ++text)
		if (*text == '\\')
			*text = '/';
}

void rgd3_str_strip_trailing_slash(char* text)
{
	size_t len = strlen(text);
	while (len > 1 && (text[len - 1] == '/' || text[len - 1] == '\\'))
		text[--len] = 0;
}

void rgd3_str_strip_trailing_whitespace(char* text)
{
	size_t len = strlen(text);
	while (len && (unsigned char)text[len - 1] <= ' ')
		text[--len] = 0;
}

const char* rgd3_str_file_extension(const char* path)
{
	const char* p = path + strlen(path);
	while (p > path)
	{
		--p;
		if (*p == '.')
			return p;
		if (*p == '/' || *p == '\\')
			break;
	}
	return "";
}

void rgd3_str_default_file_extension(char* path, size_t size, const char* ext)
{
	if (rgd3_str_file_extension(path)[0])
		return;
	rgd3_str_append(path, size, ext);
}

const char* rgd3_str_file_name(const char* path)
{
	const char* p = path + strlen(path);
	while (p > path && p[-1] != '/' && p[-1] != '\\')
		--p;
	return p;
}

int rgd3_str_is_color(const char* text)
{
	return text[0] == RGD3_COLOR_ESCAPE && text[1] && text[1] != ' ';
}

void rgd3_str_remove_colors(char* text)
{
	char* d = text;
	const char* s = text;
	while (*s)
	{
		if (rgd3_str_is_color(s))
		{
			s += 2;
			continue;
		}
		*d++ = *s++;
	}
	*d = 0;
}

static int rgd3_str_char_eq(int a, int b, int case_sensitive)
{
	if (!case_sensitive)
	{
		a = rgd3_str_lower_char(a);
		b = rgd3_str_lower_char(b);
	}
	return a == b;
}

/* Wildcard match: '*' any run, '?' any single character. */
int rgd3_str_filter(const char* filter, const char* name, int case_sensitive)
{
	const char* star_filter = NULL;
	const char* star_name = NULL;
	while (*name)
	{
		if (*filter == '*')
		{
			star_filter = ++filter;
			star_name = name;
		}
		else if (*filter == '?' || (*filter && rgd3_str_char_eq((unsigned char)*filter, (unsigned char)*name, case_sensitive)))
		{
			++filter;
			++name;
		}
		else if (star_filter)
		{
			filter = star_filter;
			name = ++star_name;
		}
		else
		{
			return 0;
		}
	}
	while (*filter == '*')
		++filter;
	return *filter == 0;
}

int rgd3_str_is_numeric(const char* text)
{
	int dot = 0;
	if (*text == '-')
		++text;
	if (!*text)
		return 0;
	for (; *text; ++text)
	{
		if (*text == '.' && !dot)
		{
			dot = 1;
			continue;
		}
		if (*text < '0' || *text > '9')
			return 0;
	}
	return 1;
}

u32 rgd3_str_ihash(const char* text)
{
	u32 h = 2166136261u;
	for (; *text; ++text)
	{
		int c = rgd3_str_lower_char((unsigned char)*text);
		if (c == '\\')
			c = '/';
		h = (h ^ (u32)c) * 16777619u;
	}
	return h;
}

/* Matches idStr(float): fixed notation with trailing zeros and dot removed. */
void rgd3_str_float_to_string(float value, char* out, size_t size)
{
	size_t len;
	rg_snprintf(out, size, "%f", value);
	len = strlen(out);
	while (len > 1 && out[len - 1] == '0')
		out[--len] = 0;
	if (len > 1 && out[len - 1] == '.')
		out[--len] = 0;
}

/* Ring of four buffers like the original va(); main thread only. */
const char* rgd3_va(const char* fmt, ...)
{
	static char buffers[4][16384];
	static int index;
	char* buf = buffers[index];
	va_list args;
	index = (index + 1) & 3;
	va_start(args, fmt);
	rg_vsnprintf(buf, sizeof(buffers[0]), fmt, args);
	va_end(args);
	return buf;
}

/* ---- Rgd3StrBuf ---- */

void rgd3_strbuf_reserve(Rgd3StrBuf* s, int size)
{
	int cap;
	if (size <= s->alloced)
		return;
	cap = s->alloced ? s->alloced : 32;
	while (cap < size)
		cap *= 2;
	s->data = (char*)rgd3_heap_realloc(s->data, (size_t)cap, RGD3_TAG_STRING);
	if (!s->alloced)
		s->data[0] = 0;
	s->alloced = cap;
}

void rgd3_strbuf_free(Rgd3StrBuf* s)
{
	if (s->data)
		rgd3_heap_free(s->data);
	s->data = NULL;
	s->len = 0;
	s->alloced = 0;
}

void rgd3_strbuf_clear(Rgd3StrBuf* s)
{
	s->len = 0;
	if (s->data)
		s->data[0] = 0;
}

void rgd3_strbuf_set(Rgd3StrBuf* s, const char* text)
{
	rgd3_strbuf_clear(s);
	rgd3_strbuf_append(s, text);
}

void rgd3_strbuf_appendn(Rgd3StrBuf* s, const char* text, int n)
{
	if (n <= 0)
	{
		rgd3_strbuf_reserve(s, s->len + 1);
		return;
	}
	rgd3_strbuf_reserve(s, s->len + n + 1);
	memcpy(s->data + s->len, text, (size_t)n);
	s->len += n;
	s->data[s->len] = 0;
}

void rgd3_strbuf_append(Rgd3StrBuf* s, const char* text)
{
	rgd3_strbuf_appendn(s, text, text ? (int)strlen(text) : 0);
}

void rgd3_strbuf_appendc(Rgd3StrBuf* s, char c)
{
	rgd3_strbuf_reserve(s, s->len + 2);
	s->data[s->len++] = c;
	s->data[s->len] = 0;
}

void rgd3_strbuf_printf(Rgd3StrBuf* s, const char* fmt, ...)
{
	char stack[1024];
	va_list args;
	int n;
	va_start(args, fmt);
	n = rg_vsnprintf(stack, sizeof(stack), fmt, args);
	va_end(args);
	if (n < (int)sizeof(stack))
	{
		rgd3_strbuf_appendn(s, stack, n);
		return;
	}
	rgd3_strbuf_reserve(s, s->len + n + 1);
	va_start(args, fmt);
	rg_vsnprintf(s->data + s->len, (size_t)n + 1, fmt, args);
	va_end(args);
	s->len += n;
}

/* idStr::Strip(c): remove leading and trailing runs of c. */
void rgd3_strbuf_strip(Rgd3StrBuf* s, char c)
{
	int start = 0;
	if (!s->data)
		return;
	while (start < s->len && s->data[start] == c)
		start++;
	if (start)
	{
		memmove(s->data, s->data + start, (size_t)(s->len - start + 1));
		s->len -= start;
	}
	rgd3_strbuf_strip_trailing(s, c);
}

void rgd3_strbuf_strip_trailing(Rgd3StrBuf* s, char c)
{
	if (!s->data)
		return;
	while (s->len > 0 && s->data[s->len - 1] == c)
		s->data[--s->len] = 0;
}

const char* rgd3_strbuf_cstr(const Rgd3StrBuf* s)
{
	return s->data ? s->data : "";
}

/* ---- idStr hashes, FloatArrayToString, CRC32 ---- */

/* idStr::Hash: sum of (signed) char * (i + 119). Hash index keys use it. */
int rgd3_str_id_hash(const char* text)
{
	int i, hash = 0;
	for (i = 0; *text; i++)
		hash += (int)(signed char)*text++ * (i + 119);
	return hash;
}

int rgd3_str_id_ihash(const char* text)
{
	int i, hash = 0;
	for (i = 0; *text; i++)
	{
		signed char c = (signed char)*text++;
		if (c >= 'A' && c <= 'Z')
			c = (signed char)(c + ('a' - 'A'));
		hash += (int)c * (i + 119);
	}
	return hash;
}

/* idStr::FloatArrayToString: "%.<precision>f" per value, space separated,
   trailing zeros and a trailing dot trimmed when precision > 0. */
const char* rgd3_str_float_array_to_string(const float* array, int length, int precision)
{
	static char str[4][16384];
	static int index;
	char format[16];
	char* s = str[index];
	int i, n;

	index = (index + 1) & 3;
	rg_snprintf(format, sizeof(format), "%%.%df", precision);
	n = rg_snprintf(s, sizeof(str[0]), format, array[0]);
	if (precision > 0)
	{
		while (n > 0 && s[n - 1] == '0')
			s[--n] = 0;
		while (n > 0 && s[n - 1] == '.')
			s[--n] = 0;
	}
	rg_snprintf(format, sizeof(format), " %%.%df", precision);
	for (i = 1; i < length; i++)
	{
		n += rg_snprintf(s + n, sizeof(str[0]) - (size_t)n, format, array[i]);
		if (precision > 0)
		{
			while (n > 0 && s[n - 1] == '0')
				s[--n] = 0;
			while (n > 0 && s[n - 1] == '.')
				s[--n] = 0;
		}
	}
	return s;
}

/* CRC-32, polynomial 0xEDB88320 (neo/idlib/hashing/CRC32.cpp) */
static u32 rgd3_crc32_table[256];
static int rgd3_crc32_ready;

static void rgd3_crc32_build(void)
{
	u32 i, j, c;
	for (i = 0; i < 256; i++)
	{
		c = i;
		for (j = 0; j < 8; j++)
			c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
		rgd3_crc32_table[i] = c;
	}
	rgd3_crc32_ready = 1;
}

void rgd3_crc32_init(u32* crc)
{
	if (!rgd3_crc32_ready)
		rgd3_crc32_build();
	*crc = 0xffffffffu;
}

void rgd3_crc32_update(u32* crc, const void* data, int length)
{
	const u8* buf = (const u8*)data;
	u32 c = *crc;
	if (!rgd3_crc32_ready)
		rgd3_crc32_build();
	while (length-- > 0)
		c = rgd3_crc32_table[(c ^ *buf++) & 0xff] ^ (c >> 8);
	*crc = c;
}

void rgd3_crc32_finish(u32* crc)
{
	*crc ^= 0xffffffffu;
}

u32 rgd3_crc32_block(const void* data, int length)
{
	u32 crc;
	rgd3_crc32_init(&crc);
	rgd3_crc32_update(&crc, data, length);
	rgd3_crc32_finish(&crc);
	return crc;
}

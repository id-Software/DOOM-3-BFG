/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_str.h - the idStr behaviours the engine relies on, on plain char*.
   Behaviour lifted from neo/idlib/Str.h/.cpp (case-insensitive compares,
   path and extension helpers, colour escapes, wildcard filter, va()). */
#ifndef RGD3_STR_H
#define RGD3_STR_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

#define RGD3_COLOR_ESCAPE '^'

int rgd3_str_icmp(const char* a, const char* b);
int rgd3_str_icmpn(const char* a, const char* b, size_t n);
int rgd3_str_icmp_prefix(const char* text, const char* prefix);
int rgd3_str_iends_with(const char* text, const char* suffix);
void rgd3_str_copynz(char* dst, const char* src, size_t dst_size);
void rgd3_str_append(char* dst, size_t dst_size, const char* src);
char* rgd3_str_to_lower(char* text);
void rgd3_str_backslashes_to_slashes(char* text);
void rgd3_str_strip_trailing_slash(char* text);
void rgd3_str_strip_trailing_whitespace(char* text);
const char* rgd3_str_file_extension(const char* path);
void rgd3_str_default_file_extension(char* path, size_t size, const char* ext);
const char* rgd3_str_file_name(const char* path);
int rgd3_str_is_color(const char* text);
void rgd3_str_remove_colors(char* text);
int rgd3_str_filter(const char* filter, const char* name, int case_sensitive);
int rgd3_str_is_numeric(const char* text);
u32 rgd3_str_ihash(const char* text);
void rgd3_str_float_to_string(float value, char* out, size_t size);
const char* rgd3_va(const char* fmt, ...);
int rgd3_str_id_hash(const char* text);  /* idStr::Hash */
int rgd3_str_id_ihash(const char* text); /* idStr::IHash */
const char* rgd3_str_float_array_to_string(const float* array, int length, int precision);

/* CRC-32 as neo/idlib/hashing/CRC32.cpp */
void rgd3_crc32_init(u32* crc);
void rgd3_crc32_update(u32* crc, const void* data, int length);
void rgd3_crc32_finish(u32* crc);
u32 rgd3_crc32_block(const void* data, int length);

/* Growable heap string (RGD3_TAG_STRING), the idStr out-parameter of the
   lexer/parser API. Zero-initialise; data is NULL until the first write and
   rgd3_strbuf_cstr() returns "" for it. Free with rgd3_strbuf_free(). */
typedef struct Rgd3StrBuf
{
	char* data;
	int len;
	int alloced;
} Rgd3StrBuf;

void rgd3_strbuf_reserve(Rgd3StrBuf* s, int size);
void rgd3_strbuf_free(Rgd3StrBuf* s);
void rgd3_strbuf_clear(Rgd3StrBuf* s);
void rgd3_strbuf_set(Rgd3StrBuf* s, const char* text);
void rgd3_strbuf_append(Rgd3StrBuf* s, const char* text);
void rgd3_strbuf_appendn(Rgd3StrBuf* s, const char* text, int n);
void rgd3_strbuf_appendc(Rgd3StrBuf* s, char c);
void rgd3_strbuf_printf(Rgd3StrBuf* s, const char* fmt, ...);
void rgd3_strbuf_strip(Rgd3StrBuf* s, char c);
void rgd3_strbuf_strip_trailing(Rgd3StrBuf* s, char c);
const char* rgd3_strbuf_cstr(const Rgd3StrBuf* s);

#endif

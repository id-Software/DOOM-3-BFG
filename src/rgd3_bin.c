/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_bin.c - see rgd3_bin.h (neo/framework/File.cpp ReadBig, ReadInt,
   ReadString and their Write counterparts). */

void rgd3_bin_reader_init(Rgd3BinReader* r, const void* data, size_t size)
{
	r->data = (const u8*)data;
	r->size = size;
	r->pos = 0;
	r->error = 0;
}

size_t rgd3_bin_remaining(const Rgd3BinReader* r)
{
	return r->size - r->pos;
}

const u8* rgd3_bin_read_bytes(Rgd3BinReader* r, size_t n)
{
	const u8* p;
	if (r->error || n > r->size - r->pos)
	{
		r->error = 1;
		r->pos = r->size;
		return NULL;
	}
	p = r->data + r->pos;
	r->pos += n;
	return p;
}

int rgd3_bin_read_into(Rgd3BinReader* r, void* dst, size_t n)
{
	const u8* p = rgd3_bin_read_bytes(r, n);
	if (!p)
	{
		memset(dst, 0, n);
		return 0;
	}
	memcpy(dst, p, n);
	return 1;
}

void rgd3_bin_skip(Rgd3BinReader* r, size_t n)
{
	rgd3_bin_read_bytes(r, n);
}

u8 rgd3_bin_u8(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 1);
	return p ? p[0] : 0;
}

u16 rgd3_bin_u16_be(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 2);
	return p ? rg_bin_load_u16_be(p) : 0;
}

u16 rgd3_bin_u16_le(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 2);
	return p ? rg_bin_load_u16_le(p) : 0;
}

i16 rgd3_bin_i16_be(Rgd3BinReader* r)
{
	return (i16)rgd3_bin_u16_be(r);
}

u32 rgd3_bin_u32_be(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 4);
	return p ? rg_bin_load_u32_be(p) : 0;
}

u32 rgd3_bin_u32_le(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 4);
	return p ? rg_bin_load_u32_le(p) : 0;
}

i32 rgd3_bin_i32_be(Rgd3BinReader* r)
{
	return (i32)rgd3_bin_u32_be(r);
}

i32 rgd3_bin_i32_le(Rgd3BinReader* r)
{
	return (i32)rgd3_bin_u32_le(r);
}

i64 rgd3_bin_i64_be(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 8);
	return p ? (i64)rg_bin_load_u64_be(p) : 0;
}

f32 rgd3_bin_f32_be(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 4);
	return p ? rg_bin_load_f32_be(p) : 0.0f;
}

f32 rgd3_bin_f32_le(Rgd3BinReader* r)
{
	const u8* p = rgd3_bin_read_bytes(r, 4);
	return p ? rg_bin_load_f32_le(p) : 0.0f;
}

void rgd3_bin_f32_be_array(Rgd3BinReader* r, float* out, int count)
{
	int i;
	for (i = 0; i < count; i++)
		out[i] = rgd3_bin_f32_be(r);
}

int rgd3_bin_string(Rgd3BinReader* r, Rgd3StrBuf* out)
{
	i32 len = rgd3_bin_i32_le(r);
	const u8* p;
	rgd3_strbuf_clear(out);
	if (len <= 0)
	{
		rgd3_strbuf_reserve(out, 1);
		return 0;
	}
	p = rgd3_bin_read_bytes(r, (size_t)len);
	if (!p)
		return 0;
	rgd3_strbuf_appendn(out, (const char*)p, len);
	return len;
}

int rgd3_bin_string_fixed(Rgd3BinReader* r, char* out, size_t size)
{
	i32 len = rgd3_bin_i32_le(r);
	const u8* p;
	size_t n;
	out[0] = 0;
	if (len <= 0)
		return 0;
	p = rgd3_bin_read_bytes(r, (size_t)len);
	if (!p)
		return 0;
	n = (size_t)len < size - 1 ? (size_t)len : size - 1;
	memcpy(out, p, n);
	out[n] = 0;
	return len;
}

/* ---- writer ---- */

void rgd3_bin_writer_free(Rgd3BinWriter* w)
{
	if (w->data)
		rgd3_heap_free(w->data);
	w->data = NULL;
	w->size = 0;
	w->capacity = 0;
}

static u8* rgd3_bin_writer_grow(Rgd3BinWriter* w, size_t n)
{
	u8* p;
	if (w->size + n > w->capacity)
	{
		size_t cap = w->capacity ? w->capacity : 4096;
		while (cap < w->size + n)
			cap *= 2;
		w->data = (u8*)rgd3_heap_realloc(w->data, cap, RGD3_TAG_FILE);
		w->capacity = cap;
	}
	p = w->data + w->size;
	w->size += n;
	return p;
}

void rgd3_bin_write_bytes(Rgd3BinWriter* w, const void* src, size_t n)
{
	if (n)
		memcpy(rgd3_bin_writer_grow(w, n), src, n);
}

void rgd3_bin_write_u8(Rgd3BinWriter* w, u8 v)
{
	*rgd3_bin_writer_grow(w, 1) = v;
}

void rgd3_bin_write_u16_be(Rgd3BinWriter* w, u16 v)
{
	rg_bin_store_u16_be(rgd3_bin_writer_grow(w, 2), v);
}

void rgd3_bin_write_u32_be(Rgd3BinWriter* w, u32 v)
{
	rg_bin_store_u32_be(rgd3_bin_writer_grow(w, 4), v);
}

void rgd3_bin_write_u32_le(Rgd3BinWriter* w, u32 v)
{
	rg_bin_store_u32_le(rgd3_bin_writer_grow(w, 4), v);
}

void rgd3_bin_write_i32_be(Rgd3BinWriter* w, i32 v)
{
	rgd3_bin_write_u32_be(w, (u32)v);
}

void rgd3_bin_write_i64_be(Rgd3BinWriter* w, i64 v)
{
	rg_bin_store_u64_be(rgd3_bin_writer_grow(w, 8), (u64)v);
}

void rgd3_bin_write_f32_be(Rgd3BinWriter* w, f32 v)
{
	rg_bin_store_f32_be(rgd3_bin_writer_grow(w, 4), v);
}

void rgd3_bin_write_string(Rgd3BinWriter* w, const char* s)
{
	size_t len = s ? strlen(s) : 0;
	rgd3_bin_write_u32_le(w, (u32)len);
	rgd3_bin_write_bytes(w, s, len);
}

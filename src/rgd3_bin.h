/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_bin.h - reading and writing the original binary layouts in memory
   (idFile ReadBig/ReadInt/ReadString, neo/framework/File.cpp, and
   neo/idlib/Swap.h). The generated BFG files are big-endian ("Big") except
   string lengths, which idFile::ReadString reads with ReadInt (little-
   endian). Reads past the end return zeros and set `error`; check it once
   after a block instead of after every call. Wraps rg_bin load/store. */
#ifndef RGD3_BIN_H
#define RGD3_BIN_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3BinReader
{
	const u8* data;
	size_t size;
	size_t pos;
	int error;
} Rgd3BinReader;

void rgd3_bin_reader_init(Rgd3BinReader* r, const void* data, size_t size);
size_t rgd3_bin_remaining(const Rgd3BinReader* r);
const u8* rgd3_bin_read_bytes(Rgd3BinReader* r, size_t n); /* NULL on overrun */
int rgd3_bin_read_into(Rgd3BinReader* r, void* dst, size_t n);
void rgd3_bin_skip(Rgd3BinReader* r, size_t n);
u8 rgd3_bin_u8(Rgd3BinReader* r);
u16 rgd3_bin_u16_be(Rgd3BinReader* r);
u16 rgd3_bin_u16_le(Rgd3BinReader* r);
i16 rgd3_bin_i16_be(Rgd3BinReader* r);
u32 rgd3_bin_u32_be(Rgd3BinReader* r);
u32 rgd3_bin_u32_le(Rgd3BinReader* r);
i32 rgd3_bin_i32_be(Rgd3BinReader* r);
i32 rgd3_bin_i32_le(Rgd3BinReader* r);
i64 rgd3_bin_i64_be(Rgd3BinReader* r);
f32 rgd3_bin_f32_be(Rgd3BinReader* r);
f32 rgd3_bin_f32_le(Rgd3BinReader* r);
void rgd3_bin_f32_be_array(Rgd3BinReader* r, float* out, int count);
/* idFile::ReadString: LE int length, then that many bytes (no NUL) */
int rgd3_bin_string(Rgd3BinReader* r, Rgd3StrBuf* out);
int rgd3_bin_string_fixed(Rgd3BinReader* r, char* out, size_t size); /* truncates */

/* Growable writer (RGD3_TAG_FILE). Zero-initialise, free with
   rgd3_bin_writer_free(). */
typedef struct Rgd3BinWriter
{
	u8* data;
	size_t size;
	size_t capacity;
} Rgd3BinWriter;

void rgd3_bin_writer_free(Rgd3BinWriter* w);
void rgd3_bin_write_bytes(Rgd3BinWriter* w, const void* src, size_t n);
void rgd3_bin_write_u8(Rgd3BinWriter* w, u8 v);
void rgd3_bin_write_u16_be(Rgd3BinWriter* w, u16 v);
void rgd3_bin_write_u32_be(Rgd3BinWriter* w, u32 v);
void rgd3_bin_write_u32_le(Rgd3BinWriter* w, u32 v);
void rgd3_bin_write_i32_be(Rgd3BinWriter* w, i32 v);
void rgd3_bin_write_i64_be(Rgd3BinWriter* w, i64 v);
void rgd3_bin_write_f32_be(Rgd3BinWriter* w, f32 v);
void rgd3_bin_write_string(Rgd3BinWriter* w, const char* s); /* LE length + bytes */

#endif

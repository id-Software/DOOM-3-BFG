/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_bitmsg.h - bit-level message reader/writer (idBitMsg,
   neo/idlib/BitMsg.h/.cpp). The bit layout is the original's: values are
   packed least significant bit first, starting at bit 0 of each byte, and
   multi-byte values are little-endian bit streams. Negative bit counts
   (WriteBits(v, -n)) store an n-bit two's complement value.

   A message works on a caller buffer that it never frees. Writing past the
   end is fatal unless allow_overflow is set, in which case the message is
   reset, `overflowed` is set and the write is dropped (as the original).
   Reading past the end returns -1 from rgd3_bitmsg_read_bits (and the
   typed readers built on it); strings and data stop at the end.

   Templates become runtime arguments: WriteQuantizedFloat<max, bits>(v) is
   rgd3_bitmsg_write_quantized_float(msg, v, max, bits) with the same
   integer arithmetic, and the vector helpers take a float pointer and a
   dimension. Network-address (netadr_t) reads and writes are not ported.

   Main thread or any single thread: a message has no locking. */
#ifndef RGD3_BITMSG_H
#define RGD3_BITMSG_H
#ifndef RGD3_ALL_C
#error "rgd3 headers are included through src/all.c only"
#endif

typedef struct Rgd3BitMsg
{
	u8* write_data;       /* NULL for read-only messages */
	const u8* read_data;
	int max_size;         /* bytes */
	int cur_size;         /* bytes completely or partially written */
	int write_bit;        /* bits written into the last byte (0..7) */
	int read_count;       /* bytes read so far */
	int read_bit;         /* bits read from the last read byte (0..7) */
	int allow_overflow;
	int overflowed;
	int has_changed;
	u64 temp_value;       /* bits not yet flushed to a whole byte */
} Rgd3BitMsg;

/* setup */
void rgd3_bitmsg_init_write(Rgd3BitMsg* msg, u8* data, int length); /* read and write */
void rgd3_bitmsg_init_read(Rgd3BitMsg* msg, const u8* data, int length);
u8* rgd3_bitmsg_get_write_data(Rgd3BitMsg* msg);
const u8* rgd3_bitmsg_get_read_data(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_max_size(const Rgd3BitMsg* msg);
void rgd3_bitmsg_set_allow_overflow(Rgd3BitMsg* msg, int set);
int rgd3_bitmsg_is_overflowed(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_size(const Rgd3BitMsg* msg);
void rgd3_bitmsg_set_size(Rgd3BitMsg* msg, int size);
int rgd3_bitmsg_get_write_bit(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_num_bits_written(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_remaining_space(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_remaining_write_bits(const Rgd3BitMsg* msg);
void rgd3_bitmsg_save_write_state(const Rgd3BitMsg* msg, int* s, int* b, u64* t);
void rgd3_bitmsg_restore_write_state(Rgd3BitMsg* msg, int s, int b, u64 t);
void rgd3_bitmsg_set_has_changed(Rgd3BitMsg* msg, int changed);
int rgd3_bitmsg_has_changed(const Rgd3BitMsg* msg);

/* read state */
int rgd3_bitmsg_get_read_count(const Rgd3BitMsg* msg);
void rgd3_bitmsg_set_read_count(Rgd3BitMsg* msg, int bytes);
int rgd3_bitmsg_get_read_bit(const Rgd3BitMsg* msg);
void rgd3_bitmsg_set_read_bit(Rgd3BitMsg* msg, int bit);
int rgd3_bitmsg_get_num_bits_read(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_remaining_data(const Rgd3BitMsg* msg);
int rgd3_bitmsg_get_remaining_read_bits(const Rgd3BitMsg* msg);
void rgd3_bitmsg_save_read_state(const Rgd3BitMsg* msg, int* c, int* b);
void rgd3_bitmsg_restore_read_state(Rgd3BitMsg* msg, int c, int b);

/* writing */
void rgd3_bitmsg_begin_writing(Rgd3BitMsg* msg);
void rgd3_bitmsg_write_byte_align(Rgd3BitMsg* msg);
void rgd3_bitmsg_write_bits(Rgd3BitMsg* msg, int value, int num_bits);
void rgd3_bitmsg_write_bool(Rgd3BitMsg* msg, int c);
void rgd3_bitmsg_write_char(Rgd3BitMsg* msg, i8 c);
void rgd3_bitmsg_write_byte(Rgd3BitMsg* msg, u8 c);
void rgd3_bitmsg_write_short(Rgd3BitMsg* msg, i16 c);
void rgd3_bitmsg_write_ushort(Rgd3BitMsg* msg, u16 c);
void rgd3_bitmsg_write_long(Rgd3BitMsg* msg, i32 c);
void rgd3_bitmsg_write_long_long(Rgd3BitMsg* msg, i64 c);
void rgd3_bitmsg_write_float(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_float_bits(Rgd3BitMsg* msg, float f, int exponent_bits, int mantissa_bits);
void rgd3_bitmsg_write_angle8(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_angle16(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_dir(Rgd3BitMsg* msg, Rgd3Vec3 dir, int num_bits);
/* max_length < 0 = unlimited; make_7bit replaces bytes > 127 with '.' */
void rgd3_bitmsg_write_string(Rgd3BitMsg* msg, const char* s, int max_length, int make_7bit);
void rgd3_bitmsg_write_data(Rgd3BitMsg* msg, const void* data, int length);
void rgd3_bitmsg_write_unorm8(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_unorm16(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_norm16(Rgd3BitMsg* msg, float f);
void rgd3_bitmsg_write_delta_char(Rgd3BitMsg* msg, i8 old_value, i8 new_value);
void rgd3_bitmsg_write_delta_byte(Rgd3BitMsg* msg, u8 old_value, u8 new_value);
void rgd3_bitmsg_write_delta_short(Rgd3BitMsg* msg, i16 old_value, i16 new_value);
void rgd3_bitmsg_write_delta_ushort(Rgd3BitMsg* msg, u16 old_value, u16 new_value);
void rgd3_bitmsg_write_delta_long(Rgd3BitMsg* msg, i32 old_value, i32 new_value);
void rgd3_bitmsg_write_delta_float(Rgd3BitMsg* msg, float old_value, float new_value);
void rgd3_bitmsg_write_delta_float_bits(Rgd3BitMsg* msg, float old_value, float new_value, int exponent_bits, int mantissa_bits);
int rgd3_bitmsg_write_delta_dict(Rgd3BitMsg* msg, const Rgd3Dict* dict, const Rgd3Dict* base); /* returns changed */
void rgd3_bitmsg_write_quantized_float(Rgd3BitMsg* msg, float value, int max, int num_bits);
void rgd3_bitmsg_write_quantized_ufloat(Rgd3BitMsg* msg, float value, int max, int num_bits);
/* WriteVectorFloat / WriteFloatArray, WriteVectorUNorm8/UNorm16/Norm16,
   WriteQuantizedVector and WriteDeltaFloatArray over `dim` floats */
void rgd3_bitmsg_write_vector_float(Rgd3BitMsg* msg, const float* v, int dim);
void rgd3_bitmsg_write_vector_unorm8(Rgd3BitMsg* msg, const float* v, int dim);
void rgd3_bitmsg_write_vector_unorm16(Rgd3BitMsg* msg, const float* v, int dim);
void rgd3_bitmsg_write_vector_norm16(Rgd3BitMsg* msg, const float* v, int dim);
void rgd3_bitmsg_write_quantized_vector(Rgd3BitMsg* msg, const float* v, int dim, int max, int num_bits);
void rgd3_bitmsg_write_delta_float_array(Rgd3BitMsg* msg, const float* old_values, const float* new_values, int dim);

/* reading */
void rgd3_bitmsg_begin_reading(Rgd3BitMsg* msg);
void rgd3_bitmsg_read_byte_align(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_bits(Rgd3BitMsg* msg, int num_bits);
int rgd3_bitmsg_read_bool(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_char(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_byte(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_short(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_ushort(Rgd3BitMsg* msg);
int rgd3_bitmsg_read_long(Rgd3BitMsg* msg);
i64 rgd3_bitmsg_read_long_long(Rgd3BitMsg* msg);
float rgd3_bitmsg_read_float(Rgd3BitMsg* msg);
float rgd3_bitmsg_read_float_bits(Rgd3BitMsg* msg, int exponent_bits, int mantissa_bits);
float rgd3_bitmsg_read_angle8(Rgd3BitMsg* msg);
float rgd3_bitmsg_read_angle16(Rgd3BitMsg* msg);
Rgd3Vec3 rgd3_bitmsg_read_dir(Rgd3BitMsg* msg, int num_bits);
/* ReadString( char *, int ): '%' becomes '.', excess is skipped; returns length */
int rgd3_bitmsg_read_string(Rgd3BitMsg* msg, char* buffer, int buffer_size);
/* ReadString( idStr & ) */
int rgd3_bitmsg_read_string_buf(Rgd3BitMsg* msg, Rgd3StrBuf* out);
int rgd3_bitmsg_read_data(Rgd3BitMsg* msg, void* data, int length); /* data may be NULL to skip */
float rgd3_bitmsg_read_unorm8(Rgd3BitMsg* msg);
float rgd3_bitmsg_read_unorm16(Rgd3BitMsg* msg);
float rgd3_bitmsg_read_norm16(Rgd3BitMsg* msg);
i8 rgd3_bitmsg_read_delta_char(Rgd3BitMsg* msg, i8 old_value);
u8 rgd3_bitmsg_read_delta_byte(Rgd3BitMsg* msg, u8 old_value);
i16 rgd3_bitmsg_read_delta_short(Rgd3BitMsg* msg, i16 old_value);
u16 rgd3_bitmsg_read_delta_ushort(Rgd3BitMsg* msg, u16 old_value);
i32 rgd3_bitmsg_read_delta_long(Rgd3BitMsg* msg, i32 old_value);
float rgd3_bitmsg_read_delta_float(Rgd3BitMsg* msg, float old_value);
float rgd3_bitmsg_read_delta_float_bits(Rgd3BitMsg* msg, float old_value, int exponent_bits, int mantissa_bits);
int rgd3_bitmsg_read_delta_dict(Rgd3BitMsg* msg, Rgd3Dict* dict, const Rgd3Dict* base); /* returns changed */
float rgd3_bitmsg_read_quantized_float(Rgd3BitMsg* msg, int max, int num_bits);
float rgd3_bitmsg_read_quantized_ufloat(Rgd3BitMsg* msg, int max, int num_bits);
void rgd3_bitmsg_read_vector_float(Rgd3BitMsg* msg, float* v, int dim);
void rgd3_bitmsg_read_vector_unorm8(Rgd3BitMsg* msg, float* v, int dim);
void rgd3_bitmsg_read_vector_unorm16(Rgd3BitMsg* msg, float* v, int dim);
void rgd3_bitmsg_read_vector_norm16(Rgd3BitMsg* msg, float* v, int dim);
void rgd3_bitmsg_read_quantized_vector(Rgd3BitMsg* msg, float* v, int dim, int max, int num_bits);
void rgd3_bitmsg_read_delta_float_array(Rgd3BitMsg* msg, const float* old_values, float* out, int dim);

int rgd3_bitmsg_dir_to_bits(Rgd3Vec3 dir, int num_bits);
Rgd3Vec3 rgd3_bitmsg_bits_to_dir(int bits, int num_bits);

int rgd3_bitmsg_self_test(void); /* 1 on pass */

#endif

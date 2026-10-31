/* SPDX-License-Identifier: GPL-3.0-or-later
   rgd3_bitmsg.c - see rgd3_bitmsg.h. Ported from neo/idlib/BitMsg.h and
   neo/idlib/BitMsg.cpp (idBitMsg): WriteBits/ReadBits bit order and the
   64-bit staging value, overflow policy, the 7-bit string and '%' rules,
   DirToBits/BitsToDir, the quantised float arithmetic and the delta
   helpers. Deviations:
	- SetWriteBit (asserted unused in the original) and the netadr_t
	  functions are not ported;
	- ReadString( idStr & ) stops at the end of the message instead of
	  moving the read count past it, so a missing terminator cannot make
	  GetRemainingData negative (and a later ReadData copy a negative size);
	- signed arithmetic that overflows in C (delta longs, sign extension)
	  is done in unsigned types with the same two's complement result. */

#define RGD3_BITMSG_NBM(x) (u64)((1ull << (x)) - 1)

static const u64 rgd3_bitmsg_mask[33] = {
	RGD3_BITMSG_NBM(0x00), RGD3_BITMSG_NBM(0x01), RGD3_BITMSG_NBM(0x02), RGD3_BITMSG_NBM(0x03),
	RGD3_BITMSG_NBM(0x04), RGD3_BITMSG_NBM(0x05), RGD3_BITMSG_NBM(0x06), RGD3_BITMSG_NBM(0x07),
	RGD3_BITMSG_NBM(0x08), RGD3_BITMSG_NBM(0x09), RGD3_BITMSG_NBM(0x0A), RGD3_BITMSG_NBM(0x0B),
	RGD3_BITMSG_NBM(0x0C), RGD3_BITMSG_NBM(0x0D), RGD3_BITMSG_NBM(0x0E), RGD3_BITMSG_NBM(0x0F),
	RGD3_BITMSG_NBM(0x10), RGD3_BITMSG_NBM(0x11), RGD3_BITMSG_NBM(0x12), RGD3_BITMSG_NBM(0x13),
	RGD3_BITMSG_NBM(0x14), RGD3_BITMSG_NBM(0x15), RGD3_BITMSG_NBM(0x16), RGD3_BITMSG_NBM(0x17),
	RGD3_BITMSG_NBM(0x18), RGD3_BITMSG_NBM(0x19), RGD3_BITMSG_NBM(0x1A), RGD3_BITMSG_NBM(0x1B),
	RGD3_BITMSG_NBM(0x1C), RGD3_BITMSG_NBM(0x1D), RGD3_BITMSG_NBM(0x1E), RGD3_BITMSG_NBM(0x1F),
	0xFFFFFFFFull
};

static u32 rgd3_bitmsg_float_as_bits(float f)
{
	u32 b;
	memcpy(&b, &f, sizeof(b));
	return b;
}

static float rgd3_bitmsg_bits_as_float(u32 b)
{
	float f;
	memcpy(&f, &b, sizeof(f));
	return f;
}

/* ---- setup ---- */

void rgd3_bitmsg_init_write(Rgd3BitMsg* msg, u8* data, int length)
{
	msg->write_data = data;
	msg->read_data = data;
	msg->max_size = length;
	msg->cur_size = 0;
	msg->write_bit = 0;
	msg->read_count = 0;
	msg->read_bit = 0;
	msg->allow_overflow = 0;
	msg->overflowed = 0;
	msg->has_changed = 0;
	msg->temp_value = 0;
}

void rgd3_bitmsg_init_read(Rgd3BitMsg* msg, const u8* data, int length)
{
	msg->write_data = NULL;
	msg->read_data = data;
	msg->max_size = length;
	msg->cur_size = length;
	msg->write_bit = 0;
	msg->read_count = 0;
	msg->read_bit = 0;
	msg->allow_overflow = 0;
	msg->overflowed = 0;
	msg->has_changed = 0;
	msg->temp_value = 0;
}

u8* rgd3_bitmsg_get_write_data(Rgd3BitMsg* msg)
{
	return msg->write_data;
}

const u8* rgd3_bitmsg_get_read_data(const Rgd3BitMsg* msg)
{
	return msg->read_data;
}

int rgd3_bitmsg_get_max_size(const Rgd3BitMsg* msg)
{
	return msg->max_size;
}

void rgd3_bitmsg_set_allow_overflow(Rgd3BitMsg* msg, int set)
{
	msg->allow_overflow = set ? 1 : 0;
}

int rgd3_bitmsg_is_overflowed(const Rgd3BitMsg* msg)
{
	return msg->overflowed;
}

int rgd3_bitmsg_get_size(const Rgd3BitMsg* msg)
{
	return msg->cur_size + (msg->write_bit != 0);
}

void rgd3_bitmsg_set_size(Rgd3BitMsg* msg, int size)
{
	RG_ASSERT(msg->write_bit == 0);
	msg->cur_size = size > msg->max_size ? msg->max_size : size;
}

int rgd3_bitmsg_get_write_bit(const Rgd3BitMsg* msg)
{
	return msg->write_bit;
}

int rgd3_bitmsg_get_num_bits_written(const Rgd3BitMsg* msg)
{
	return (msg->cur_size << 3) + msg->write_bit;
}

int rgd3_bitmsg_get_remaining_space(const Rgd3BitMsg* msg)
{
	return msg->max_size - rgd3_bitmsg_get_size(msg);
}

int rgd3_bitmsg_get_remaining_write_bits(const Rgd3BitMsg* msg)
{
	return (msg->max_size << 3) - rgd3_bitmsg_get_num_bits_written(msg);
}

void rgd3_bitmsg_save_write_state(const Rgd3BitMsg* msg, int* s, int* b, u64* t)
{
	*s = msg->cur_size;
	*b = msg->write_bit;
	*t = msg->temp_value;
}

void rgd3_bitmsg_restore_write_state(Rgd3BitMsg* msg, int s, int b, u64 t)
{
	msg->cur_size = s;
	msg->write_bit = b & 7;
	if (msg->write_bit)
		msg->write_data[msg->cur_size] &= (u8)((1 << msg->write_bit) - 1);
	msg->temp_value = t;
}

void rgd3_bitmsg_set_has_changed(Rgd3BitMsg* msg, int changed)
{
	msg->has_changed = changed ? 1 : 0;
}

int rgd3_bitmsg_has_changed(const Rgd3BitMsg* msg)
{
	return msg->has_changed;
}

/* ---- read state ---- */

int rgd3_bitmsg_get_read_count(const Rgd3BitMsg* msg)
{
	return msg->read_count;
}

void rgd3_bitmsg_set_read_count(Rgd3BitMsg* msg, int bytes)
{
	msg->read_count = bytes;
}

int rgd3_bitmsg_get_read_bit(const Rgd3BitMsg* msg)
{
	return msg->read_bit;
}

void rgd3_bitmsg_set_read_bit(Rgd3BitMsg* msg, int bit)
{
	msg->read_bit = bit & 7;
}

int rgd3_bitmsg_get_num_bits_read(const Rgd3BitMsg* msg)
{
	return (msg->read_count << 3) - ((8 - msg->read_bit) & 7);
}

int rgd3_bitmsg_get_remaining_data(const Rgd3BitMsg* msg)
{
	RG_ASSERT(msg->write_bit == 0);
	return msg->cur_size - msg->read_count;
}

int rgd3_bitmsg_get_remaining_read_bits(const Rgd3BitMsg* msg)
{
	RG_ASSERT(msg->write_bit == 0);
	return (msg->cur_size << 3) - rgd3_bitmsg_get_num_bits_read(msg);
}

void rgd3_bitmsg_save_read_state(const Rgd3BitMsg* msg, int* c, int* b)
{
	RG_ASSERT(msg->write_bit == 0);
	*c = msg->read_count;
	*b = msg->read_bit;
}

void rgd3_bitmsg_restore_read_state(Rgd3BitMsg* msg, int c, int b)
{
	RG_ASSERT(msg->write_bit == 0);
	msg->read_count = c;
	msg->read_bit = b & 7;
}

/* ---- writing ---- */

void rgd3_bitmsg_begin_writing(Rgd3BitMsg* msg)
{
	msg->cur_size = 0;
	msg->overflowed = 0;
	msg->write_bit = 0;
	msg->temp_value = 0;
}

/* WriteBits zeroes the unused high bits of a partial byte, so aligning
   never lets uninitialised bits into the stream (snapshots memcmp it). */
void rgd3_bitmsg_write_byte_align(Rgd3BitMsg* msg)
{
	msg->cur_size += msg->write_bit != 0;
	RG_ASSERT(msg->write_bit == 0 || (msg->write_data[msg->cur_size - 1] >> msg->write_bit) == 0);
	msg->write_bit = 0;
	msg->temp_value = 0;
}

static int rgd3_bitmsg_check_overflow(Rgd3BitMsg* msg, int num_bits)
{
	if (num_bits > rgd3_bitmsg_get_remaining_write_bits(msg))
	{
		if (!msg->allow_overflow)
		{
			rgd3_error_fatal("rgd3_bitmsg: overflow without allowOverflow set; maxsize=%d size=%d numBits=%d numRemainingWriteBits=%d",
				rgd3_bitmsg_get_max_size(msg), rgd3_bitmsg_get_size(msg), num_bits, rgd3_bitmsg_get_remaining_write_bits(msg));
		}
		if (num_bits > (msg->max_size << 3))
			rgd3_error_fatal("rgd3_bitmsg: %d bits is > full message size", num_bits);
		rgd3_printf("rgd3_bitmsg: overflow\n");
		rgd3_bitmsg_begin_writing(msg);
		msg->overflowed = 1;
		return 1;
	}
	return 0;
}

static u8* rgd3_bitmsg_get_byte_space(Rgd3BitMsg* msg, int length)
{
	u8* ptr;
	if (!msg->write_data)
		rgd3_error_fatal("rgd3_bitmsg_get_byte_space: cannot write to message");
	/* round up to the next byte */
	rgd3_bitmsg_write_byte_align(msg);
	/* on overflow the message restarts at 0 (allow_overflow) */
	rgd3_bitmsg_check_overflow(msg, length << 3);
	ptr = msg->write_data + msg->cur_size;
	msg->cur_size += length;
	return ptr;
}

/* A negative num_bits stores a signed value of -num_bits bits. */
void rgd3_bitmsg_write_bits(Rgd3BitMsg* msg, int value, int num_bits)
{
	if (!msg->write_data)
		rgd3_error_fatal("rgd3_bitmsg_write_bits: cannot write to message");
	if (num_bits == 0 || num_bits < -31 || num_bits > 32)
		rgd3_error_fatal("rgd3_bitmsg_write_bits: bad numBits %d", num_bits);
	if (num_bits != 32)
	{
		if (num_bits > 0)
		{
			if ((i64)value > ((i64)1 << num_bits) - 1 || value < 0)
				rgd3_error_fatal("rgd3_bitmsg_write_bits: value overflow %d %d", value, num_bits);
		}
		else
		{
			i64 r = (i64)1 << (-1 - num_bits);
			if ((i64)value > r - 1 || (i64)value < -r)
				rgd3_error_fatal("rgd3_bitmsg_write_bits: value overflow %d %d", value, num_bits);
		}
	}
	if (num_bits < 0)
		num_bits = -num_bits;
	if (rgd3_bitmsg_check_overflow(msg, num_bits))
		return;

	/* merge the value with the leftover bits, flush whole bytes */
	msg->temp_value |= ((u64)(i64)value & rgd3_bitmsg_mask[num_bits]) << msg->write_bit;
	msg->write_bit += num_bits;
	while (msg->write_bit >= 8)
	{
		msg->write_data[msg->cur_size++] = (u8)(msg->temp_value & 255);
		msg->temp_value >>= 8;
		msg->write_bit -= 8;
	}
	/* write the leftover now, in case this is the last call */
	if (msg->write_bit > 0)
		msg->write_data[msg->cur_size] = (u8)(msg->temp_value & 255);
}

void rgd3_bitmsg_write_bool(Rgd3BitMsg* msg, int c)
{
	rgd3_bitmsg_write_bits(msg, c ? 1 : 0, 1);
}

void rgd3_bitmsg_write_char(Rgd3BitMsg* msg, i8 c)
{
	rgd3_bitmsg_write_bits(msg, c, -8);
}

void rgd3_bitmsg_write_byte(Rgd3BitMsg* msg, u8 c)
{
	rgd3_bitmsg_write_bits(msg, c, 8);
}

void rgd3_bitmsg_write_short(Rgd3BitMsg* msg, i16 c)
{
	rgd3_bitmsg_write_bits(msg, c, -16);
}

void rgd3_bitmsg_write_ushort(Rgd3BitMsg* msg, u16 c)
{
	rgd3_bitmsg_write_bits(msg, c, 16);
}

void rgd3_bitmsg_write_long(Rgd3BitMsg* msg, i32 c)
{
	rgd3_bitmsg_write_bits(msg, c, 32);
}

void rgd3_bitmsg_write_long_long(Rgd3BitMsg* msg, i64 c)
{
	int a = (int)(u32)((u64)c & 0xFFFFFFFFull);
	int b = (int)(u32)((u64)c >> 32);
	rgd3_bitmsg_write_bits(msg, a, 32);
	rgd3_bitmsg_write_bits(msg, b, 32);
}

void rgd3_bitmsg_write_float(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_bits(msg, (int)rgd3_bitmsg_float_as_bits(f), 32);
}

void rgd3_bitmsg_write_float_bits(Rgd3BitMsg* msg, float f, int exponent_bits, int mantissa_bits)
{
	int bits = rgd3_math_float_to_bits(f, exponent_bits, mantissa_bits);
	rgd3_bitmsg_write_bits(msg, bits, 1 + exponent_bits + mantissa_bits);
}

void rgd3_bitmsg_write_angle8(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_byte(msg, (u8)RGD3_ANGLE2BYTE(f));
}

void rgd3_bitmsg_write_angle16(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_short(msg, (i16)(u16)RGD3_ANGLE2SHORT(f));
}

void rgd3_bitmsg_write_dir(Rgd3BitMsg* msg, Rgd3Vec3 dir, int num_bits)
{
	rgd3_bitmsg_write_bits(msg, rgd3_bitmsg_dir_to_bits(dir, num_bits), num_bits);
}

void rgd3_bitmsg_write_string(Rgd3BitMsg* msg, const char* s, int max_length, int make_7bit)
{
	if (!s)
	{
		rgd3_bitmsg_write_data(msg, "", 1);
	}
	else
	{
		int i, l = (int)strlen(s);
		u8* data_ptr;
		const u8* byte_ptr = (const u8*)s;
		if (max_length >= 0 && l >= max_length)
			l = max_length - 1;
		data_ptr = rgd3_bitmsg_get_byte_space(msg, l + 1);
		for (i = 0; i < l; i++)
			data_ptr[i] = (make_7bit && byte_ptr[i] > 127) ? (u8)'.' : byte_ptr[i];
		data_ptr[i] = 0;
	}
}

void rgd3_bitmsg_write_data(Rgd3BitMsg* msg, const void* data, int length)
{
	memcpy(rgd3_bitmsg_get_byte_space(msg, length), data, (size_t)length);
}

void rgd3_bitmsg_write_unorm8(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_byte(msg, rgd3_math_ftob(f * 255.0f));
}

void rgd3_bitmsg_write_unorm16(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_ushort(msg, (u16)rgd3_math_ftoi(f * 65535.0f));
}

void rgd3_bitmsg_write_norm16(Rgd3BitMsg* msg, float f)
{
	rgd3_bitmsg_write_short(msg, (i16)(u16)rgd3_math_ftoi(f * 32767.0f));
}

void rgd3_bitmsg_write_delta_char(Rgd3BitMsg* msg, i8 old_value, i8 new_value)
{
	rgd3_bitmsg_write_byte(msg, (u8)(new_value - old_value));
}

void rgd3_bitmsg_write_delta_byte(Rgd3BitMsg* msg, u8 old_value, u8 new_value)
{
	rgd3_bitmsg_write_byte(msg, (u8)(new_value - old_value));
}

void rgd3_bitmsg_write_delta_short(Rgd3BitMsg* msg, i16 old_value, i16 new_value)
{
	rgd3_bitmsg_write_ushort(msg, (u16)(new_value - old_value));
}

void rgd3_bitmsg_write_delta_ushort(Rgd3BitMsg* msg, u16 old_value, u16 new_value)
{
	rgd3_bitmsg_write_ushort(msg, (u16)(new_value - old_value));
}

void rgd3_bitmsg_write_delta_long(Rgd3BitMsg* msg, i32 old_value, i32 new_value)
{
	rgd3_bitmsg_write_long(msg, (i32)((u32)new_value - (u32)old_value));
}

void rgd3_bitmsg_write_delta_float(Rgd3BitMsg* msg, float old_value, float new_value)
{
	rgd3_bitmsg_write_float(msg, new_value - old_value);
}

void rgd3_bitmsg_write_delta_float_bits(Rgd3BitMsg* msg, float old_value, float new_value, int exponent_bits, int mantissa_bits)
{
	rgd3_bitmsg_write_float_bits(msg, new_value - old_value, exponent_bits, mantissa_bits);
}

/* changed and new keys, "", deleted keys, "" (values compared ignoring case) */
int rgd3_bitmsg_write_delta_dict(Rgd3BitMsg* msg, const Rgd3Dict* dict, const Rgd3Dict* base)
{
	int i, changed = 0;
	if (base)
	{
		for (i = 0; i < rgd3_dict_num(dict); i++)
		{
			const Rgd3KeyValue* kv = rgd3_dict_get_kv(dict, i);
			const Rgd3KeyValue* basekv = rgd3_dict_find_key(base, rgd3_kv_key(kv));
			if (!basekv || rgd3_str_icmp(rgd3_kv_value(basekv), rgd3_kv_value(kv)) != 0)
			{
				rgd3_bitmsg_write_string(msg, rgd3_kv_key(kv), -1, 1);
				rgd3_bitmsg_write_string(msg, rgd3_kv_value(kv), -1, 1);
				changed = 1;
			}
		}
		rgd3_bitmsg_write_string(msg, "", -1, 1);
		for (i = 0; i < rgd3_dict_num(base); i++)
		{
			const Rgd3KeyValue* basekv = rgd3_dict_get_kv(base, i);
			if (!rgd3_dict_find_key(dict, rgd3_kv_key(basekv)))
			{
				rgd3_bitmsg_write_string(msg, rgd3_kv_key(basekv), -1, 1);
				changed = 1;
			}
		}
		rgd3_bitmsg_write_string(msg, "", -1, 1);
	}
	else
	{
		for (i = 0; i < rgd3_dict_num(dict); i++)
		{
			const Rgd3KeyValue* kv = rgd3_dict_get_kv(dict, i);
			rgd3_bitmsg_write_string(msg, rgd3_kv_key(kv), -1, 1);
			rgd3_bitmsg_write_string(msg, rgd3_kv_value(kv), -1, 1);
			changed = 1;
		}
		rgd3_bitmsg_write_string(msg, "", -1, 1);
		rgd3_bitmsg_write_string(msg, "", -1, 1);
	}
	return changed;
}

/* signed: -num_bits bits, |value| quantised to max; whole numbers survive
   when max <= storeMax (integer scale) */
void rgd3_bitmsg_write_quantized_float(Rgd3BitMsg* msg, float value, int max, int num_bits)
{
	int store_max = (1 << (num_bits - 1)) - 1;
	if (max > store_max)
	{
		/* scaling down (scale < 1) */
		float scale = (float)store_max / (float)max;
		rgd3_bitmsg_write_bits(msg, rgd3_math_clamp_int(-store_max, store_max, rgd3_math_ftoi(value * scale)), -num_bits);
	}
	else
	{
		/* scaling up by a whole number */
		int scale = store_max / max;
		rgd3_bitmsg_write_bits(msg, rgd3_math_clamp_int(-store_max, store_max, rgd3_math_ftoi(value * (float)scale)), -num_bits);
	}
}

void rgd3_bitmsg_write_quantized_ufloat(Rgd3BitMsg* msg, float value, int max, int num_bits)
{
	int store_max = (int)(((u32)1 << num_bits) - 1);
	if (max > store_max)
	{
		float scale = (float)store_max / (float)max;
		rgd3_bitmsg_write_bits(msg, rgd3_math_clamp_int(0, store_max, rgd3_math_ftoi(value * scale)), num_bits);
	}
	else
	{
		int scale = store_max / max;
		rgd3_bitmsg_write_bits(msg, rgd3_math_clamp_int(0, store_max, rgd3_math_ftoi(value * (float)scale)), num_bits);
	}
}

void rgd3_bitmsg_write_vector_float(Rgd3BitMsg* msg, const float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_float(msg, v[i]);
}

void rgd3_bitmsg_write_vector_unorm8(Rgd3BitMsg* msg, const float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_unorm8(msg, v[i]);
}

void rgd3_bitmsg_write_vector_unorm16(Rgd3BitMsg* msg, const float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_unorm16(msg, v[i]);
}

void rgd3_bitmsg_write_vector_norm16(Rgd3BitMsg* msg, const float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_norm16(msg, v[i]);
}

void rgd3_bitmsg_write_quantized_vector(Rgd3BitMsg* msg, const float* v, int dim, int max, int num_bits)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_quantized_float(msg, v[i], max, num_bits);
}

void rgd3_bitmsg_write_delta_float_array(Rgd3BitMsg* msg, const float* old_values, const float* new_values, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		rgd3_bitmsg_write_delta_float(msg, old_values[i], new_values[i]);
}

/* ---- reading ---- */

void rgd3_bitmsg_begin_reading(Rgd3BitMsg* msg)
{
	msg->read_count = 0;
	msg->read_bit = 0;
	msg->write_bit = 0;
	msg->temp_value = 0;
}

void rgd3_bitmsg_read_byte_align(Rgd3BitMsg* msg)
{
	msg->read_bit = 0;
}

/* A negative num_bits reads a signed value. Returns -1 when fewer than
   |num_bits| bits remain. */
int rgd3_bitmsg_read_bits(Rgd3BitMsg* msg, int num_bits)
{
	u32 value = 0;
	int value_bits = 0;
	int sgn;
	if (!msg->read_data)
		rgd3_error_fatal("rgd3_bitmsg_read_bits: cannot read from message");
	if (num_bits == 0 || num_bits < -31 || num_bits > 32)
		rgd3_error_fatal("rgd3_bitmsg_read_bits: bad numBits %d", num_bits);
	sgn = num_bits < 0;
	if (sgn)
		num_bits = -num_bits;
	if (num_bits > rgd3_bitmsg_get_remaining_read_bits(msg))
		return -1;
	while (value_bits < num_bits)
	{
		int get;
		u32 fraction;
		if (msg->read_bit == 0)
			msg->read_count++;
		get = 8 - msg->read_bit;
		if (get > num_bits - value_bits)
			get = num_bits - value_bits;
		fraction = msg->read_data[msg->read_count - 1];
		fraction >>= msg->read_bit;
		fraction &= (1u << get) - 1;
		value |= fraction << value_bits;
		value_bits += get;
		msg->read_bit = (msg->read_bit + get) & 7;
	}
	if (sgn && (value & (1u << (num_bits - 1))))
		value |= ~((1u << num_bits) - 1);
	return (int)value;
}

int rgd3_bitmsg_read_bool(Rgd3BitMsg* msg)
{
	return rgd3_bitmsg_read_bits(msg, 1) == 1;
}

int rgd3_bitmsg_read_char(Rgd3BitMsg* msg)
{
	return (i8)(u8)rgd3_bitmsg_read_bits(msg, -8);
}

int rgd3_bitmsg_read_byte(Rgd3BitMsg* msg)
{
	return (u8)rgd3_bitmsg_read_bits(msg, 8);
}

int rgd3_bitmsg_read_short(Rgd3BitMsg* msg)
{
	return (i16)(u16)rgd3_bitmsg_read_bits(msg, -16);
}

int rgd3_bitmsg_read_ushort(Rgd3BitMsg* msg)
{
	return (u16)rgd3_bitmsg_read_bits(msg, 16);
}

int rgd3_bitmsg_read_long(Rgd3BitMsg* msg)
{
	return rgd3_bitmsg_read_bits(msg, 32);
}

i64 rgd3_bitmsg_read_long_long(Rgd3BitMsg* msg)
{
	u64 a = (u64)(i64)rgd3_bitmsg_read_bits(msg, 32);
	u64 b = (u64)(i64)rgd3_bitmsg_read_bits(msg, 32);
	return (i64)((a & 0xFFFFFFFFull) | (b << 32));
}

float rgd3_bitmsg_read_float(Rgd3BitMsg* msg)
{
	return rgd3_bitmsg_bits_as_float((u32)rgd3_bitmsg_read_bits(msg, 32));
}

float rgd3_bitmsg_read_float_bits(Rgd3BitMsg* msg, int exponent_bits, int mantissa_bits)
{
	int bits = rgd3_bitmsg_read_bits(msg, 1 + exponent_bits + mantissa_bits);
	return rgd3_math_bits_to_float(bits, exponent_bits, mantissa_bits);
}

float rgd3_bitmsg_read_angle8(Rgd3BitMsg* msg)
{
	return (float)rgd3_bitmsg_read_byte(msg) * (360.0f / 256.0f);
}

float rgd3_bitmsg_read_angle16(Rgd3BitMsg* msg)
{
	return (float)rgd3_bitmsg_read_short(msg) * (360.0f / 65536.0f);
}

Rgd3Vec3 rgd3_bitmsg_read_dir(Rgd3BitMsg* msg, int num_bits)
{
	return rgd3_bitmsg_bits_to_dir(rgd3_bitmsg_read_bits(msg, num_bits), num_bits);
}

int rgd3_bitmsg_read_string(Rgd3BitMsg* msg, char* buffer, int buffer_size)
{
	int l = 0, c;
	rgd3_bitmsg_read_byte_align(msg);
	for (;;)
	{
		c = rgd3_bitmsg_read_byte(msg);
		if (c <= 0 || c >= 255)
			break;
		/* translate all fmt spec to avoid crash bugs in string routines */
		if (c == '%')
			c = '.';
		/* read past excessively long strings so following data stays readable */
		if (l < buffer_size - 1)
			buffer[l++] = (char)c;
	}
	buffer[l] = 0;
	return l;
}

int rgd3_bitmsg_read_string_buf(Rgd3BitMsg* msg, Rgd3StrBuf* out)
{
	int i, cnt = 0, end;
	rgd3_bitmsg_read_byte_align(msg);
	for (i = msg->read_count; i < msg->cur_size; i++)
	{
		if (msg->read_data[i] == 0)
			break;
		cnt++;
	}
	rgd3_strbuf_clear(out);
	if (cnt)
		rgd3_strbuf_appendn(out, (const char*)msg->read_data + msg->read_count, cnt);
	end = msg->read_count + cnt + 1;
	msg->read_count = end > msg->cur_size ? msg->cur_size : end;
	return out->len;
}

int rgd3_bitmsg_read_data(Rgd3BitMsg* msg, void* data, int length)
{
	int cnt;
	rgd3_bitmsg_read_byte_align(msg);
	cnt = msg->read_count;
	if (msg->read_count + length > msg->cur_size)
	{
		int remaining = rgd3_bitmsg_get_remaining_data(msg);
		if (data && remaining > 0)
			memcpy(data, msg->read_data + msg->read_count, (size_t)remaining);
		msg->read_count = msg->cur_size;
	}
	else
	{
		if (data)
			memcpy(data, msg->read_data + msg->read_count, (size_t)length);
		msg->read_count += length;
	}
	return msg->read_count - cnt;
}

float rgd3_bitmsg_read_unorm8(Rgd3BitMsg* msg)
{
	return (float)rgd3_bitmsg_read_byte(msg) / 255.0f;
}

float rgd3_bitmsg_read_unorm16(Rgd3BitMsg* msg)
{
	return (float)rgd3_bitmsg_read_ushort(msg) / 65535.0f;
}

float rgd3_bitmsg_read_norm16(Rgd3BitMsg* msg)
{
	return (float)rgd3_bitmsg_read_short(msg) / 32767.0f;
}

i8 rgd3_bitmsg_read_delta_char(Rgd3BitMsg* msg, i8 old_value)
{
	return (i8)(u8)(old_value + rgd3_bitmsg_read_byte(msg));
}

u8 rgd3_bitmsg_read_delta_byte(Rgd3BitMsg* msg, u8 old_value)
{
	return (u8)(old_value + rgd3_bitmsg_read_byte(msg));
}

i16 rgd3_bitmsg_read_delta_short(Rgd3BitMsg* msg, i16 old_value)
{
	return (i16)(u16)(old_value + rgd3_bitmsg_read_ushort(msg));
}

u16 rgd3_bitmsg_read_delta_ushort(Rgd3BitMsg* msg, u16 old_value)
{
	return (u16)(old_value + rgd3_bitmsg_read_ushort(msg));
}

i32 rgd3_bitmsg_read_delta_long(Rgd3BitMsg* msg, i32 old_value)
{
	return (i32)((u32)old_value + (u32)rgd3_bitmsg_read_long(msg));
}

float rgd3_bitmsg_read_delta_float(Rgd3BitMsg* msg, float old_value)
{
	return old_value + rgd3_bitmsg_read_float(msg);
}

float rgd3_bitmsg_read_delta_float_bits(Rgd3BitMsg* msg, float old_value, int exponent_bits, int mantissa_bits)
{
	return old_value + rgd3_bitmsg_read_float_bits(msg, exponent_bits, mantissa_bits);
}

int rgd3_bitmsg_read_delta_dict(Rgd3BitMsg* msg, Rgd3Dict* dict, const Rgd3Dict* base)
{
	char key[RGD3_MAX_STRING_CHARS];
	char value[RGD3_MAX_STRING_CHARS];
	int changed = 0;
	if (base)
		rgd3_dict_assign(dict, base);
	else
		rgd3_dict_clear(dict);
	while (rgd3_bitmsg_read_string(msg, key, (int)sizeof(key)) != 0)
	{
		rgd3_bitmsg_read_string(msg, value, (int)sizeof(value));
		rgd3_dict_set(dict, key, value);
		changed = 1;
	}
	while (rgd3_bitmsg_read_string(msg, key, (int)sizeof(key)) != 0)
	{
		rgd3_dict_delete(dict, key);
		changed = 1;
	}
	return changed;
}

float rgd3_bitmsg_read_quantized_float(Rgd3BitMsg* msg, int max, int num_bits)
{
	int store_max = (1 << (num_bits - 1)) - 1;
	if (max > store_max)
	{
		float inv_scale = (float)max / (float)store_max;
		return (float)rgd3_bitmsg_read_bits(msg, -num_bits) * inv_scale;
	}
	else
	{
		/* the scale is the whole number the writer used */
		float scale = (float)(store_max / max);
		float inv_scale = 1.0f / scale;
		return (float)rgd3_bitmsg_read_bits(msg, -num_bits) * inv_scale;
	}
}

float rgd3_bitmsg_read_quantized_ufloat(Rgd3BitMsg* msg, int max, int num_bits)
{
	int store_max = (int)(((u32)1 << num_bits) - 1);
	if (max > store_max)
	{
		float inv_scale = (float)max / (float)store_max;
		return (float)rgd3_bitmsg_read_bits(msg, num_bits) * inv_scale;
	}
	else
	{
		float scale = (float)(store_max / max);
		float inv_scale = 1.0f / scale;
		return (float)rgd3_bitmsg_read_bits(msg, num_bits) * inv_scale;
	}
}

void rgd3_bitmsg_read_vector_float(Rgd3BitMsg* msg, float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		v[i] = rgd3_bitmsg_read_float(msg);
}

void rgd3_bitmsg_read_vector_unorm8(Rgd3BitMsg* msg, float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		v[i] = rgd3_bitmsg_read_unorm8(msg);
}

void rgd3_bitmsg_read_vector_unorm16(Rgd3BitMsg* msg, float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		v[i] = rgd3_bitmsg_read_unorm16(msg);
}

void rgd3_bitmsg_read_vector_norm16(Rgd3BitMsg* msg, float* v, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		v[i] = rgd3_bitmsg_read_norm16(msg);
}

void rgd3_bitmsg_read_quantized_vector(Rgd3BitMsg* msg, float* v, int dim, int max, int num_bits)
{
	int i;
	for (i = 0; i < dim; i++)
		v[i] = rgd3_bitmsg_read_quantized_float(msg, max, num_bits);
}

void rgd3_bitmsg_read_delta_float_array(Rgd3BitMsg* msg, const float* old_values, float* out, int dim)
{
	int i;
	for (i = 0; i < dim; i++)
		out[i] = rgd3_bitmsg_read_delta_float(msg, old_values[i]);
}

/* ---- direction packing: per axis a sign bit and (numBits/3 - 1) bits of
   magnitude, x in the high bits ---- */

int rgd3_bitmsg_dir_to_bits(Rgd3Vec3 dir, int num_bits)
{
	int max;
	float bias;
	u32 bits;
	RG_ASSERT(num_bits >= 6 && num_bits <= 32);
	num_bits /= 3;
	max = (1 << (num_bits - 1)) - 1;
	bias = 0.5f / (float)max;
	bits = (rgd3_bitmsg_float_as_bits(dir.x) >> 31) << (num_bits * 3 - 1);
	bits |= (u32)rgd3_math_ftoi((fabsf(dir.x) + bias) * (float)max) << (num_bits * 2);
	bits |= (rgd3_bitmsg_float_as_bits(dir.y) >> 31) << (num_bits * 2 - 1);
	bits |= (u32)rgd3_math_ftoi((fabsf(dir.y) + bias) * (float)max) << (num_bits * 1);
	bits |= (rgd3_bitmsg_float_as_bits(dir.z) >> 31) << (num_bits * 1 - 1);
	bits |= (u32)rgd3_math_ftoi((fabsf(dir.z) + bias) * (float)max) << (num_bits * 0);
	return (int)bits;
}

Rgd3Vec3 rgd3_bitmsg_bits_to_dir(int bits, int num_bits)
{
	static const float sign[2] = { 1.0f, -1.0f };
	Rgd3Vec3 dir;
	u32 b = (u32)bits;
	u32 max;
	float inv_max;
	RG_ASSERT(num_bits >= 6 && num_bits <= 32);
	num_bits /= 3;
	max = (1u << (num_bits - 1)) - 1;
	inv_max = 1.0f / (float)max;
	dir.x = sign[(b >> (num_bits * 3 - 1)) & 1] * (float)((b >> (num_bits * 2)) & max) * inv_max;
	dir.y = sign[(b >> (num_bits * 2 - 1)) & 1] * (float)((b >> (num_bits * 1)) & max) * inv_max;
	dir.z = sign[(b >> (num_bits * 1 - 1)) & 1] * (float)((b >> (num_bits * 0)) & max) * inv_max;
	rgd3_vec3_normalize_fast(&dir);
	return dir;
}

/* ---------------------------------------------------------------------------
   Self-test
--------------------------------------------------------------------------- */

static int rgd3_bitmsg_test_checks;
static int rgd3_bitmsg_test_failures;

#define RGD3_BITMSG_CHECK(cond) rgd3_bitmsg_test_check((cond) != 0, #cond, __LINE__)

static void rgd3_bitmsg_test_check(int ok, const char* what, int line)
{
	rgd3_bitmsg_test_checks++;
	if (!ok)
	{
		rgd3_bitmsg_test_failures++;
		rgd3_printf("^1FAILED^7 %s (rgd3_bitmsg.c:%d)\n", what, line);
	}
}

static int rgd3_bitmsg_near(float a, float b, float eps)
{
	return fabsf(a - b) <= eps;
}

static void rgd3_bitmsg_test_layout(void)
{
	u8 buf[64];
	Rgd3BitMsg w, r;
	int s, b, c, rb;
	u64 t;

	/* bit order: LSB first within each byte */
	memset(buf, 0xCC, sizeof(buf));
	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_write_bits(&w, 1, 1);
	rgd3_bitmsg_write_bits(&w, 5, 3);
	rgd3_bitmsg_write_bits(&w, 0xAB, 8);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_size(&w) == 2);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_num_bits_written(&w) == 12);
	RGD3_BITMSG_CHECK(buf[0] == 0xBB && buf[1] == 0x0A);
	/* a long at bit offset 12 */
	rgd3_bitmsg_write_long(&w, 0x12345678);
	RGD3_BITMSG_CHECK(buf[1] == 0x8A && buf[2] == 0x67 && buf[3] == 0x45 && buf[4] == 0x23 && buf[5] == 0x01);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_write_bits(&w) == 64 * 8 - 44);

	/* write state save/restore truncates the partial byte */
	rgd3_bitmsg_save_write_state(&w, &s, &b, &t);
	rgd3_bitmsg_write_bits(&w, 0xF, 4);
	rgd3_bitmsg_restore_write_state(&w, s, b, t);
	rgd3_bitmsg_write_bits(&w, 0x3, 4);
	RGD3_BITMSG_CHECK(buf[5] == 0x31);

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 1) == 1);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 3) == 5);
	rgd3_bitmsg_save_read_state(&r, &c, &rb);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_byte(&r) == 0xAB);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_num_bits_read(&r) == 12);
	rgd3_bitmsg_restore_read_state(&r, c, rb);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_byte(&r) == 0xAB);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_long(&r) == 0x12345678);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 4) == 3);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_read_bits(&r) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 1) == -1);
}

static void rgd3_bitmsg_test_values(void)
{
	u8 buf[256];
	Rgd3BitMsg w, r;
	const i64 big = (i64)0x123456789ABCDEF0ull;

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_write_bool(&w, 1);         /* everything below at odd offsets */
	rgd3_bitmsg_write_bits(&w, -16, -5);
	rgd3_bitmsg_write_bits(&w, 15, -5);
	rgd3_bitmsg_write_bits(&w, 0x7FFFFFFF, 31);
	rgd3_bitmsg_write_bits(&w, -1, 32);
	rgd3_bitmsg_write_bits(&w, -1073741824, -31);
	rgd3_bitmsg_write_char(&w, -128);
	rgd3_bitmsg_write_char(&w, 127);
	rgd3_bitmsg_write_byte(&w, 255);
	rgd3_bitmsg_write_short(&w, -32768);
	rgd3_bitmsg_write_short(&w, 32767);
	rgd3_bitmsg_write_ushort(&w, 65535);
	rgd3_bitmsg_write_long(&w, (i32)0x80000000u);
	rgd3_bitmsg_write_long_long(&w, big);
	rgd3_bitmsg_write_long_long(&w, -big);
	rgd3_bitmsg_write_float(&w, -1.25e-3f);
	rgd3_bitmsg_write_float_bits(&w, 1.5f, 5, 10);
	rgd3_bitmsg_write_float_bits(&w, -0.375f, 4, 6);
	RGD3_BITMSG_CHECK(!rgd3_bitmsg_is_overflowed(&w));

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bool(&r) == 1);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, -5) == -16);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, -5) == 15);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 31) == 0x7FFFFFFF);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 32) == -1);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, -31) == -1073741824);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_char(&r) == -128);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_char(&r) == 127);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_byte(&r) == 255);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_short(&r) == -32768);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_short(&r) == 32767);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_ushort(&r) == 65535);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_long(&r) == (i32)0x80000000u);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_long_long(&r) == big);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_long_long(&r) == -big);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_float(&r) == -1.25e-3f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_float_bits(&r, 5, 10) == 1.5f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_float_bits(&r, 4, 6) == -0.375f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_read_bits(&r) < 8);
}

static void rgd3_bitmsg_test_delta(void)
{
	u8 buf[128];
	Rgd3BitMsg w, r;
	const float olds[3] = { 1.0f, 2.0f, 3.0f };
	const float news[3] = { 1.5f, -2.0f, 3.0f };
	float got[3];

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_write_bool(&w, 0);
	rgd3_bitmsg_write_delta_byte(&w, 250, 5);
	rgd3_bitmsg_write_delta_char(&w, -100, 100);
	rgd3_bitmsg_write_delta_short(&w, -30000, 30000);
	rgd3_bitmsg_write_delta_ushort(&w, 65000, 3);
	rgd3_bitmsg_write_delta_long(&w, (i32)0x80000000u, 0x7FFFFFFF);
	rgd3_bitmsg_write_delta_float(&w, 10.0f, 12.5f);
	rgd3_bitmsg_write_delta_float_bits(&w, 1.0f, 3.0f, 5, 10);
	rgd3_bitmsg_write_delta_float_array(&w, olds, news, 3);
	RGD3_BITMSG_CHECK(buf[0] == (u8)(((u8)(5 - 250)) << 1)); /* 11 shifted past the bool */

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bool(&r) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_byte(&r, 250) == 5);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_char(&r, -100) == 100);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_short(&r, -30000) == 30000);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_ushort(&r, 65000) == 3);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_long(&r, (i32)0x80000000u) == 0x7FFFFFFF);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_float(&r, 10.0f) == 12.5f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_float_bits(&r, 1.0f, 5, 10) == 3.0f);
	rgd3_bitmsg_read_delta_float_array(&r, olds, got, 3);
	RGD3_BITMSG_CHECK(got[0] == 1.5f && got[1] == -2.0f && got[2] == 3.0f);
}

static void rgd3_bitmsg_test_quantize(void)
{
	u8 buf[128];
	Rgd3BitMsg w, r;
	Rgd3Vec3 d1, d2, d3, g;
	const float v3[3] = { 0.25f, -1.0f, 0.5f };
	const float u3[3] = { 0.0f, 0.5f, 1.0f };
	float out[3];

	d1.x = 0.0f; d1.y = 0.0f; d1.z = 1.0f;
	d2.x = 0.6f; d2.y = -0.8f; d2.z = 0.0f;
	d3.x = -0.57735f; d3.y = -0.57735f; d3.z = 0.57735f;

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_write_bits(&w, 3, 3);
	rgd3_bitmsg_write_quantized_float(&w, 12.5f, 64, 16);     /* scale up: integer scale 511 */
	rgd3_bitmsg_write_quantized_float(&w, 100.0f, 64, 16);    /* clamps to +storeMax */
	rgd3_bitmsg_write_quantized_float(&w, 50000.0f, 100000, 8); /* scale down */
	rgd3_bitmsg_write_quantized_float(&w, -3.0f, 4, 8);
	rgd3_bitmsg_write_quantized_ufloat(&w, 0.5f, 1, 8);
	rgd3_bitmsg_write_quantized_ufloat(&w, -1.0f, 1, 8);      /* clamps to 0 */
	rgd3_bitmsg_write_quantized_ufloat(&w, 700.0f, 1000, 6);
	rgd3_bitmsg_write_angle8(&w, 90.0f);
	rgd3_bitmsg_write_angle16(&w, -90.0f);
	rgd3_bitmsg_write_angle16(&w, 359.0f);
	rgd3_bitmsg_write_dir(&w, d1, 24);
	rgd3_bitmsg_write_dir(&w, d2, 24);
	rgd3_bitmsg_write_dir(&w, d3, 9);
	rgd3_bitmsg_write_unorm8(&w, 0.5f);
	rgd3_bitmsg_write_unorm16(&w, 1.0f);
	rgd3_bitmsg_write_norm16(&w, -1.0f);
	rgd3_bitmsg_write_quantized_vector(&w, v3, 3, 2, 12);
	rgd3_bitmsg_write_vector_unorm8(&w, u3, 3);
	rgd3_bitmsg_write_vector_float(&w, v3, 3);

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 3) == 3);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_float(&r, 64, 16), 12.5f, 1.0f / 511.0f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_float(&r, 64, 16), 32767.0f / 511.0f, 1e-4f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_float(&r, 100000, 8), 50000.0f, 100000.0f / 127.0f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_float(&r, 4, 8), -3.0f, 1e-5f)); /* whole numbers survive */
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_ufloat(&r, 1, 8), 127.0f / 255.0f, 1e-6f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_quantized_ufloat(&r, 1, 8) == 0.0f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_quantized_ufloat(&r, 1000, 6), 700.0f, 1000.0f / 63.0f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_angle8(&r) == 90.0f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_angle16(&r) == -90.0f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(rgd3_bitmsg_read_angle16(&r), -1.0f, 360.0f / 65536.0f));
	g = rgd3_bitmsg_read_dir(&r, 24);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(g.x, 0.0f, 1e-3f) && rgd3_bitmsg_near(g.y, 0.0f, 1e-3f) && rgd3_bitmsg_near(g.z, 1.0f, 1e-3f));
	g = rgd3_bitmsg_read_dir(&r, 24);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(g.x, 0.6f, 0.02f) && rgd3_bitmsg_near(g.y, -0.8f, 0.02f) && rgd3_bitmsg_near(g.z, 0.0f, 0.02f));
	g = rgd3_bitmsg_read_dir(&r, 9);
	RGD3_BITMSG_CHECK(g.x < 0.0f && g.y < 0.0f && g.z > 0.0f && rgd3_bitmsg_near(g.x, g.y, 1e-6f));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_unorm8(&r) == 127.0f / 255.0f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_unorm16(&r) == 1.0f);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_norm16(&r) == -1.0f);
	rgd3_bitmsg_read_quantized_vector(&r, out, 3, 2, 12);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_near(out[0], 0.25f, 1.0f / 1023.0f) && rgd3_bitmsg_near(out[1], -1.0f, 1e-5f) && rgd3_bitmsg_near(out[2], 0.5f, 1.0f / 1023.0f));
	rgd3_bitmsg_read_vector_unorm8(&r, out, 3);
	RGD3_BITMSG_CHECK(out[0] == 0.0f && out[1] == 127.0f / 255.0f && out[2] == 1.0f);
	rgd3_bitmsg_read_vector_float(&r, out, 3);
	RGD3_BITMSG_CHECK(out[0] == v3[0] && out[1] == v3[1] && out[2] == v3[2]);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_dir_to_bits(d1, 24) == 0x7F); /* x 0, y 0, z +127 */
}

static void rgd3_bitmsg_test_strings(void)
{
	u8 buf[128];
	u8 blob[5] = { 1, 2, 3, 4, 5 };
	u8 got[8];
	char small[4];
	char line[64];
	Rgd3BitMsg w, r;
	Rgd3StrBuf sb = { 0 };

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_write_bits(&w, 1, 3);          /* strings align to the next byte */
	rgd3_bitmsg_write_string(&w, "hello%", -1, 1);
	rgd3_bitmsg_write_string(&w, "longword", -1, 1);
	rgd3_bitmsg_write_string(&w, "\xE9t\xE9", -1, 1);
	rgd3_bitmsg_write_string(&w, "\xE9t", -1, 0);
	rgd3_bitmsg_write_string(&w, "abcdef", 3, 1);
	rgd3_bitmsg_write_string(&w, NULL, -1, 1);
	rgd3_bitmsg_write_string(&w, "buf", -1, 1);
	rgd3_bitmsg_write_bits(&w, 1, 1);
	rgd3_bitmsg_write_data(&w, blob, 5);
	RGD3_BITMSG_CHECK(buf[1] == 'h' && buf[7] == 0);

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 3) == 1);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 6 && strcmp(line, "hello.") == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, small, sizeof(small)) == 3 && strcmp(small, "lon") == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 3 && strcmp(line, ".t.") == 0);
	/* kept raw when not made 7-bit */
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 2 && (u8)line[0] == 0xE9);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 2 && strcmp(line, "ab") == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string_buf(&r, &sb) == 3 && strcmp(rgd3_strbuf_cstr(&sb), "buf") == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_bits(&r, 1) == 1);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_data(&r, got, 5) == 5 && memcmp(got, blob, 5) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_data(&r) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_data(&r, got, 4) == 0);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string(&r, line, sizeof(line)) == 0);
	rgd3_strbuf_free(&sb);

	/* an unterminated string at the end stays inside the message */
	{
		const u8 raw[3] = { 'a', 'b', 'c' };
		Rgd3BitMsg u;
		rgd3_bitmsg_init_read(&u, raw, 3);
		RGD3_BITMSG_CHECK(rgd3_bitmsg_read_string_buf(&u, &sb) == 3);
		RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_data(&u) == 0);
		RGD3_BITMSG_CHECK(rgd3_bitmsg_read_data(&u, got, 2) == 0);
		rgd3_strbuf_free(&sb);
	}
}

static void rgd3_bitmsg_test_overflow(void)
{
	u8 buf[4];
	Rgd3BitMsg w;

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	rgd3_bitmsg_set_allow_overflow(&w, 1);
	rgd3_bitmsg_write_long(&w, 7);
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_remaining_space(&w) == 0 && !rgd3_bitmsg_is_overflowed(&w));
	rgd3_bitmsg_write_bits(&w, 1, 1); /* resets the message and drops the write */
	RGD3_BITMSG_CHECK(rgd3_bitmsg_is_overflowed(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_get_size(&w) == 0);
	rgd3_bitmsg_begin_writing(&w);
	RGD3_BITMSG_CHECK(!rgd3_bitmsg_is_overflowed(&w));
	rgd3_bitmsg_write_byte(&w, 1);
	rgd3_bitmsg_write_string(&w, "abc", -1, 1); /* 32 bits > 24 left: byte space overflow */
	RGD3_BITMSG_CHECK(rgd3_bitmsg_is_overflowed(&w));
	rgd3_bitmsg_begin_writing(&w);
	rgd3_bitmsg_write_bits(&w, 1, 7);
	rgd3_bitmsg_write_bits(&w, 1, 25);
	RGD3_BITMSG_CHECK(!rgd3_bitmsg_is_overflowed(&w) && rgd3_bitmsg_get_remaining_write_bits(&w) == 0);
}

static void rgd3_bitmsg_test_dict(void)
{
	u8 buf[256];
	Rgd3BitMsg w, r;
	Rgd3Dict base = { 0 }, cur = { 0 }, got = { 0 }, full = { 0 };

	rgd3_dict_set(&base, "a", "1");
	rgd3_dict_set(&base, "b", "2");
	rgd3_dict_set(&base, "c", "3");
	rgd3_dict_set(&cur, "a", "1");
	rgd3_dict_set(&cur, "b", "5");
	rgd3_dict_set(&cur, "d", "4");

	rgd3_bitmsg_init_write(&w, buf, sizeof(buf));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_write_delta_dict(&w, &cur, &base));
	RGD3_BITMSG_CHECK(!rgd3_bitmsg_write_delta_dict(&w, &base, &base));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_write_delta_dict(&w, &cur, NULL));

	rgd3_bitmsg_init_read(&r, buf, rgd3_bitmsg_get_size(&w));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_dict(&r, &got, &base));
	RGD3_BITMSG_CHECK(rgd3_dict_checksum(&got) == rgd3_dict_checksum(&cur));
	RGD3_BITMSG_CHECK(!rgd3_dict_find_key(&got, "c"));
	RGD3_BITMSG_CHECK(!rgd3_bitmsg_read_delta_dict(&r, &got, &base));
	RGD3_BITMSG_CHECK(rgd3_dict_checksum(&got) == rgd3_dict_checksum(&base));
	RGD3_BITMSG_CHECK(rgd3_bitmsg_read_delta_dict(&r, &full, NULL));
	RGD3_BITMSG_CHECK(rgd3_dict_checksum(&full) == rgd3_dict_checksum(&cur));
	rgd3_dict_clear(&base);
	rgd3_dict_clear(&cur);
	rgd3_dict_clear(&got);
	rgd3_dict_clear(&full);
}

int rgd3_bitmsg_self_test(void)
{
	rgd3_bitmsg_test_checks = 0;
	rgd3_bitmsg_test_failures = 0;
	rgd3_bitmsg_test_layout();
	rgd3_bitmsg_test_values();
	rgd3_bitmsg_test_delta();
	rgd3_bitmsg_test_quantize();
	rgd3_bitmsg_test_strings();
	rgd3_bitmsg_test_overflow();
	rgd3_bitmsg_test_dict();
	rgd3_printf("bitmsg: %d checks, %d failures\n", rgd3_bitmsg_test_checks, rgd3_bitmsg_test_failures);
	return rgd3_bitmsg_test_failures == 0;
}

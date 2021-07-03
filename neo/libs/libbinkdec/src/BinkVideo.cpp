/*
 * libbinkdec - Bink video decoder
 * Copyright (C) 2011 Barry Duncan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* This code is based on the Bink decoder from the FFmpeg project which can be obtained from http://www.ffmpeg.org/
 * below is the license from FFmpeg
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Bink video decoder
 * Copyright (c) 2009 Konstantin Shishkov
 * Copyright (C) 2011 Peter Ross <pross@xvid.org>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "BinkDecoder.h"
#include "Util.h"
#include "LogError.h"
#include "binkdata.h"
#include <algorithm> // DG: for std::min/max

static const uint8_t bink_rlelens[4] = { 4, 8, 12, 32 };

static void bink_idct_add_c(uint8_t *dest, int linesize, int32_t *block);
static void bink_idct_put_c(uint8_t *dest, int linesize, int32_t *block);
static void scale_block_c(const uint8_t src[64], uint8_t *dst, int linesize);

void BinkDecoder::VideoPacket(uint32_t packetSize)
{
	// create a temp bit reader for this packet
	BinkCommon::BitReader bits(file, packetSize);

	DecodeFrame(bits);
}

/**
 * Initialize length length in all bundles.
 *
 * @param c     decoder context
 * @param width plane width
 * @param bw    plane width in 8x8 blocks
 */
void BinkDecoder::InitLengths(int width, int bw)
{
	bundle[BINK_SRC_BLOCK_TYPES].len = av_log2_c((width >> 3) + 511) + 1;

	bundle[BINK_SRC_SUB_BLOCK_TYPES].len = av_log2_c((width >> 4) + 511) + 1;

	bundle[BINK_SRC_COLORS].len = av_log2_c(bw*64 + 511) + 1;

	bundle[BINK_SRC_INTRA_DC].len =
	bundle[BINK_SRC_INTER_DC].len =
	bundle[BINK_SRC_X_OFF].len =
	bundle[BINK_SRC_Y_OFF].len = av_log2_c((width >> 3) + 511) + 1;

	bundle[BINK_SRC_PATTERN].len = av_log2_c((bw << 3) + 511) + 1;

	bundle[BINK_SRC_RUN].len = av_log2_c(bw*48 + 511) + 1;
}

// Allocate memory for bundles.
void BinkDecoder::InitBundles()
{
	int bw = (frameWidth  + 7) >> 3;
	int bh = (frameHeight + 7) >> 3;
	int blocks = bw * bh;

	for (int i = 0; i < BINK_NB_SRC; i++)
	{
		bundle[i].data = new uint8_t[blocks * 64];
		bundle[i].data_end = bundle[i].data + blocks * 64;
	}
}

void BinkDecoder::FreeBundles()
{
	for (int i = 0; i < BINK_NB_SRC; i++)
		delete[] bundle[i].data;
}

uint8_t BinkDecoder::GetHuffSymbol(BinkCommon::BitReader &bits, Tree &tree)
{
	return tree.symbols[VLC_GetCodeBits(bits, bink_trees[tree.vlc_num])];
}

// as per http://michael.dipperstein.com/huffman/#decode - ".. However I have also read that it is faster to store the 
// codes for each symbol in an array sorted by code length and search for a match every time a bit is read in." 
void BinkDecoder::InitTrees()
{
	for (uint32_t i = 0; i < kNumTrees; i++)
	{
		// get max code length
		const int maxBits = bink_tree_lens[i][15];

		VLC_InitTable(bink_trees[i], maxBits, 16, &bink_tree_lens[i][0], &bink_tree_bits[i][0]);
	}
}

/**
 * Merge two consequent lists of equal size depending on bits read.
 *
 * @param gb   context for reading bits
 * @param dst  buffer where merged list will be written to
 * @param src  pointer to the head of the first list (the second lists starts at src+size)
 * @param size input lists size
 */
void BinkDecoder::Merge(BinkCommon::BitReader &bits, uint8_t *dst, uint8_t *src, int size)
{
    uint8_t *src2 = src + size;
    int size2 = size;

    do {
        if (!bits.GetBit()) {
            *dst++ = *src++;
            size--;
        } else {
            *dst++ = *src2++;
            size2--;
        }
    } while (size && size2);

    while (size--)
        *dst++ = *src++;
    while (size2--)
        *dst++ = *src2++;
}

/**
 * Read information about Huffman tree used to decode data.
 *
 * @param gb   context for reading bits
 * @param tree pointer for storing tree data
 */
void BinkDecoder::ReadTree(BinkCommon::BitReader &bits, Tree *tree)
{
    uint8_t tmp1[16], tmp2[16], *in = tmp1, *out = tmp2;
    int i, t, len;

    tree->vlc_num = bits.GetBits(4);
    if (!tree->vlc_num) {
        for (i = 0; i < 16; i++)
            tree->symbols[i] = i;
        return;
    }
    if (bits.GetBit()) {
        len = bits.GetBits(3);
        memset(tmp1, 0, sizeof(tmp1));
        for (i = 0; i <= len; i++) {
            tree->symbols[i] = bits.GetBits(4);
            tmp1[tree->symbols[i]] = 1;
        }
        for (i = 0; i < 16 && len < 16 - 1; i++)
            if (!tmp1[i])
                tree->symbols[++len] = i;
    } else {
        len = bits.GetBits(2);
        for (i = 0; i < 16; i++)
            in[i] = i;
        for (i = 0; i <= len; i++) {
            int size = 1 << i;
            for (t = 0; t < 16; t += size << 1)
                Merge(bits, out + t, in + t, size);
			std::swap(in, out);
        }
        memcpy(tree->symbols, in, 16);
    }
}

/**
 * Prepare bundle for decoding data.
 *
 * @param gb          context for reading bits
 * @param c           decoder context
 * @param bundle_num  number of the bundle to initialize
 */
void BinkDecoder::ReadBundle(BinkCommon::BitReader &bits, int bundle_num)
{
    if (bundle_num == BINK_SRC_COLORS) {
        for (int i = 0; i < 16; i++)
            ReadTree(bits, &col_high[i]);
        col_lastval = 0;
    }
    if (bundle_num != BINK_SRC_INTRA_DC && bundle_num != BINK_SRC_INTER_DC)
        ReadTree(bits, &bundle[bundle_num].tree);
    bundle[bundle_num].cur_dec =
	bundle[bundle_num].cur_ptr = bundle[bundle_num].data;
}

/**
 * common check before starting decoding bundle data
 *
 * @param gb context for reading bits
 * @param b  bundle
 * @param t  variable where number of elements to decode will be stored
 */

int CheckReadVal(BinkCommon::BitReader &bits, Bundle *b, int &t)
{
	if (!b->cur_dec || (b->cur_dec > b->cur_ptr))
        return 0;
	
	t = bits.GetBits(b->len);

	if (!t) {
        b->cur_dec = NULL;
        return 0;
    }

	return 1;
}

int BinkDecoder::ReadRuns(BinkCommon::BitReader &bits, Bundle *b)
{
    int t, v;
    const uint8_t *dec_end;

    if (CheckReadVal(bits, b, t) == 0)
	{
		return 0;
	}

    dec_end = b->cur_dec + t;
    if (dec_end > b->data_end) {
		BinkCommon::LogError("Run value went out of bounds");
        return -1;
    }
    if (bits.GetBit()) {
        v = bits.GetBits(4);
        memset(b->cur_dec, v, t);
        b->cur_dec += t;
    } else {
        while (b->cur_dec < dec_end)
            *b->cur_dec++ = GetHuffSymbol(bits, b->tree);
    }
    return 0;
}

int BinkDecoder::ReadMotionValues(BinkCommon::BitReader &bits, Bundle *b)
{
    int t, sign, v;
    const uint8_t *dec_end;

    if (CheckReadVal(bits, b, t) == 0)
	{
		return 0;
	}

    dec_end = b->cur_dec + t;
    if (dec_end > b->data_end) {
		BinkCommon::LogError("Too many motion values");
        return -1;
    }
    if (bits.GetBit()) {
        v = bits.GetBits(4);
        if (v) {
            sign = -(int)bits.GetBit();
            v = (v ^ sign) - sign;
        }
        memset(b->cur_dec, v, t);
        b->cur_dec += t;
    } else {
        while (b->cur_dec < dec_end) {
            v = GetHuffSymbol(bits, b->tree);
            if (v) {
                sign = -(int)bits.GetBit();
                v = (v ^ sign) - sign;
            }
            *b->cur_dec++ = v;
        }
    }
    return 0;
}

int BinkDecoder::ReadBlockTypes(BinkCommon::BitReader &bits, Bundle *b)
{
    int t, v;
    int last = 0;
    const uint8_t *dec_end;

    if (CheckReadVal(bits, b, t) == 0)
	{
		return 0;
	}

    dec_end = b->cur_dec + t;
    if (dec_end > b->data_end) {
		BinkCommon::LogError("Too many block type values");
        return -1;
    }
    if (bits.GetBit()) {
        v = bits.GetBits(4);
        memset(b->cur_dec, v, t);
        b->cur_dec += t;
    } else {
        while (b->cur_dec < dec_end) {
            v = GetHuffSymbol(bits, b->tree);
            if (v < 12) {
                last = v;
                *b->cur_dec++ = v;
            } else {
                int run = bink_rlelens[v - 12];

                if (dec_end - b->cur_dec < run)
                    return -1;

                memset(b->cur_dec, last, run);
                b->cur_dec += run;
            }
        }
    }
    return 0;
}

int BinkDecoder::ReadPatterns(BinkCommon::BitReader &bits, Bundle *b)
{
    int t, v;
    const uint8_t *dec_end;

    if (CheckReadVal(bits, b, t) == 0)
	{
		return 0;
	}

    dec_end = b->cur_dec + t;
    if (dec_end > b->data_end) {
		BinkCommon::LogError("Too many pattern values");
        return -1;
    }
    while (b->cur_dec < dec_end) {
        v  = GetHuffSymbol(bits, b->tree);
        v |= GetHuffSymbol(bits, b->tree) << 4;
        *b->cur_dec++ = v;
    }

    return 0;
}

int BinkDecoder::ReadColors(BinkCommon::BitReader &bits, Bundle *b)
{
    int t, sign, v;
    const uint8_t *dec_end;

    if (CheckReadVal(bits, b, t) == 0)
	{
		return 0;
	}

    dec_end = b->cur_dec + t;

    if (dec_end > b->data_end) {
		BinkCommon::LogError("Too many color values");
        return -1;
    }
    if (bits.GetBit()) 
	{
        col_lastval = GetHuffSymbol(bits, col_high[col_lastval]);
        v = GetHuffSymbol(bits, b->tree);
        v = (col_lastval << 4) | v;
        if (signature < kBIKiID)
		{
            sign = ((int8_t) v) >> 7;
            v = ((v & 0x7F) ^ sign) - sign;
            v += 0x80;
        }
        memset(b->cur_dec, v, t);
        b->cur_dec += t;
    }
	else 
	{
        while (b->cur_dec < dec_end)
		{
            col_lastval = GetHuffSymbol(bits, col_high[col_lastval]);
            v = GetHuffSymbol(bits, b->tree);
            v = (col_lastval << 4) | v;

			if (signature < kBIKiID)
			{
                sign = ((int8_t) v) >> 7;
                v = ((v & 0x7F) ^ sign) - sign;
                v += 0x80;
            }
            *b->cur_dec++ = v;
        }
    }
    return 0;
}

int BinkDecoder::ReadDCs(BinkCommon::BitReader &bits, Bundle *b, int start_bits, int has_sign)
{
    int i, j, len, len2, bsize, sign, v, v2;
    int16_t *dst     = (int16_t*)b->cur_dec;
    int16_t *dst_end = (int16_t*)b->data_end;

    if (CheckReadVal(bits, b, len) == 0)
	{
		return 0;
	}

    v = bits.GetBits(start_bits - has_sign);
    if (v && has_sign) {
        sign = -(int)bits.GetBit();
        v = (v ^ sign) - sign;
    }

    if (dst_end - dst < 1)
        return -1;

    *dst++ = v;
    len--;
    for (i = 0; i < len; i += 8) {
        len2 = std::min(len - i, (int)8);
        if (dst_end - dst < len2)
            return -1;
        bsize = bits.GetBits(4);
        if (bsize) {
            for (j = 0; j < len2; j++) {
                v2 = bits.GetBits(bsize);
                if (v2) {
                    sign = -(int)bits.GetBit();
                    v2 = (v2 ^ sign) - sign;
                }
                v += v2;
                *dst++ = v;
                if (v < -32768 || v > 32767) {
					BinkCommon::LogError("DC value went out of bounds" + v);
                    return -1;
                }
            }
        } else {
            for (j = 0; j < len2; j++)
                *dst++ = v;
        }
    }

    b->cur_dec = (uint8_t*)dst;
    return 0;
}

/**
 * Retrieve next value from bundle.
 *
 * @param c      decoder context
 * @param bundle bundle number
 */
int BinkDecoder::GetValue(int bundle)
{
	// TODO - checkme
    int ret;

    if (bundle < BINK_SRC_X_OFF || bundle == BINK_SRC_RUN)
        return *this->bundle[bundle].cur_ptr++;
    if (bundle == BINK_SRC_X_OFF || bundle == BINK_SRC_Y_OFF)
        return (int8_t)*this->bundle[bundle].cur_ptr++;

    ret = *(int16_t*)this->bundle[bundle].cur_ptr;
    this->bundle[bundle].cur_ptr += 2;
    return ret;
}

/**
 * Read 8x8 block of DCT coefficients.
 *
 * @param gb       context for reading bits
 * @param block    place for storing coefficients
 * @param scan     scan order table
 * @param quant_matrices quantization matrices
 * @return 0 for success, negative value in other cases
 */
int BinkDecoder::ReadDCTcoeffs(BinkCommon::BitReader &bits, int32_t block[64], const uint8_t *scan,
                           const int32_t quant_matrices[16][64], int q)
{
    int coef_list[128];
    int mode_list[128];
    int i, t, mask, nBits, ccoef, mode, sign;
    int list_start = 64, list_end = 64, list_pos;
    int coef_count = 0;
    int coef_idx[64];
    int quant_idx;
    const int32_t *quant;

    coef_list[list_end] = 4;  mode_list[list_end++] = 0;
    coef_list[list_end] = 24; mode_list[list_end++] = 0;
    coef_list[list_end] = 44; mode_list[list_end++] = 0;
    coef_list[list_end] = 1;  mode_list[list_end++] = 3;
    coef_list[list_end] = 2;  mode_list[list_end++] = 3;
    coef_list[list_end] = 3;  mode_list[list_end++] = 3;

    nBits = bits.GetBits(4) - 1;

    for (mask = 1 << nBits; nBits >= 0; mask >>= 1, nBits--) {
        list_pos = list_start;
        while (list_pos < list_end) {
            if (!(mode_list[list_pos] | coef_list[list_pos]) || !bits.GetBit()) {
                list_pos++;
                continue;
            }
            ccoef = coef_list[list_pos];
            mode  = mode_list[list_pos];
            switch (mode) {
            case 0:
                coef_list[list_pos] = ccoef + 4;
                mode_list[list_pos] = 1;
            case 2:
                if (mode == 2) {
                    coef_list[list_pos]   = 0;
                    mode_list[list_pos++] = 0;
                }
                for (i = 0; i < 4; i++, ccoef++) {
                    if (bits.GetBit()) {
                        coef_list[--list_start] = ccoef;
                        mode_list[  list_start] = 3;
                    } else {
                        if (!nBits) {
                            t = 1 - (bits.GetBit() << 1);
                        } else {
                            t = bits.GetBits(nBits) | mask;
                            sign = -(int)bits.GetBit();
                            t = (t ^ sign) - sign;
                        }
                        block[scan[ccoef]] = t;
                        coef_idx[coef_count++] = ccoef;
                    }
                }
                break;
            case 1:
                mode_list[list_pos] = 2;
                for (i = 0; i < 3; i++) {
                    ccoef += 4;
                    coef_list[list_end]   = ccoef;
                    mode_list[list_end++] = 2;
                }
                break;
            case 3:
                if (!nBits) {
                    t = 1 - (bits.GetBit() << 1);
                } else {
                    t = bits.GetBits(nBits) | mask;
                    sign = -(int)bits.GetBit();
                    t = (t ^ sign) - sign;
                }
                block[scan[ccoef]] = t;
                coef_idx[coef_count++] = ccoef;
                coef_list[list_pos]   = 0;
                mode_list[list_pos++] = 0;
                break;
            }
        }
    }

    if (q == -1) {
        quant_idx = bits.GetBits(4);
    } else {
        quant_idx = q;
    }

    quant = quant_matrices[quant_idx];

    block[0] = (block[0] * quant[0]) >> 11;
    for (i = 0; i < coef_count; i++) {
        int idx = coef_idx[i];
        block[scan[idx]] = (block[scan[idx]] * quant[idx]) >> 11;
    }

    return 0;
}

/**
 * Read 8x8 block with residue after motion compensation.
 *
 * @param gb          context for reading bits
 * @param block       place to store read data
 * @param masks_count number of masks to decode
 * @return 0 on success, negative value in other cases
 */
int BinkDecoder::ReadResidue(BinkCommon::BitReader &bits, DCTELEM block[64], int masks_count)
{
    int coef_list[128];
    int mode_list[128];
    int i, sign, mask, ccoef, mode;
    int list_start = 64, list_end = 64, list_pos;
    int nz_coeff[64];
    int nz_coeff_count = 0;

    coef_list[list_end] =  4; mode_list[list_end++] = 0;
    coef_list[list_end] = 24; mode_list[list_end++] = 0;
    coef_list[list_end] = 44; mode_list[list_end++] = 0;
    coef_list[list_end] =  0; mode_list[list_end++] = 2;

    for (mask = 1 << bits.GetBits(3); mask; mask >>= 1) {
        for (i = 0; i < nz_coeff_count; i++) {
            if (!bits.GetBit())
                continue;
            if (block[nz_coeff[i]] < 0)
                block[nz_coeff[i]] -= mask;
            else
                block[nz_coeff[i]] += mask;
            masks_count--;
            if (masks_count < 0)
                return 0;
        }
        list_pos = list_start;
        while (list_pos < list_end) {
            if (!(coef_list[list_pos] | mode_list[list_pos]) || !bits.GetBit()) {
                list_pos++;
                continue;
            }
            ccoef = coef_list[list_pos];
            mode  = mode_list[list_pos];
            switch (mode) {
            case 0:
                coef_list[list_pos] = ccoef + 4;
                mode_list[list_pos] = 1;
            case 2:
                if (mode == 2) {
                    coef_list[list_pos]   = 0;
                    mode_list[list_pos++] = 0;
                }
                for (i = 0; i < 4; i++, ccoef++) {
                    if (bits.GetBit()) {
                        coef_list[--list_start] = ccoef;
                        mode_list[  list_start] = 3;
                    } else {
                        nz_coeff[nz_coeff_count++] = bink_scan[ccoef];
                        sign = -(int)bits.GetBit();
                        block[bink_scan[ccoef]] = (mask ^ sign) - sign;
                        masks_count--;
                        if (masks_count < 0)
                            return 0;
                    }
                }
                break;
            case 1:
                mode_list[list_pos] = 2;
                for (i = 0; i < 3; i++) {
                    ccoef += 4;
                    coef_list[list_end]   = ccoef;
                    mode_list[list_end++] = 2;
                }
                break;
            case 3:
                nz_coeff[nz_coeff_count++] = bink_scan[ccoef];
                sign = -(int)bits.GetBit();
                block[bink_scan[ccoef]] = (mask ^ sign) - sign;
                coef_list[list_pos]   = 0;
                mode_list[list_pos++] = 0;
                masks_count--;
                if (masks_count < 0)
                    return 0;
                break;
            }
        }
    }

    return 0;
}

/**
 * Copy 8x8 block from source to destination, where src and dst may be overlapped
 */
void BinkDecoder::PutPixels8x8Overlapped(uint8_t *dst, uint8_t *src, int stride)
{
    uint8_t tmp[64];
    for (int i = 0; i < 8; i++)
        memcpy(tmp + i*8, src + i*stride, 8);
    for (int i = 0; i < 8; i++)
        memcpy(dst + i*stride, tmp + i*8, 8);
}

// copy size x size block from source to destination
void BinkDecoder::PutPixelsTab(uint8_t *dest, uint8_t *prev, uint32_t pitch, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++)
	{
		memcpy(dest, prev, size);

		dest += pitch;
		prev += pitch;
	}
}

// fill size x size block with value
void BinkDecoder::FillBlockTab(uint8_t *dest, uint32_t value, uint32_t pitch, uint32_t size)
{
	for (uint32_t i = 0; i < size; i++)
	{
		memset(dest, value, size);
		dest += pitch;
	}
}

void BinkDecoder::AddPixels8(uint8_t *dest, DCTELEM *block, uint32_t stride)
{
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			dest[j] += block[j];
		}

		dest  += stride;
		block += 8;
	}
}

void BinkDecoder::ClearBlock(DCTELEM *block)
{
	memset(block, 0, 64 * sizeof(DCTELEM));
}

int BinkDecoder::DecodePlane(BinkCommon::BitReader &bits, int plane_idx, int is_chroma)
{
	int blk;
	int bx, by;
	uint8_t *dst, *prev, *ref, *ref_start, *ref_end;
	int v, col[2];
	const uint8_t *scan;
	int xoff, yoff;

	DCTELEM block[64];
	uint8_t ublock[64];
	int32_t dctblock[64];
	int coordmap[64];

	int bw = is_chroma ? (frameWidth  + 15) >> 4 : (frameWidth  + 7) >> 3;
	int bh = is_chroma ? (frameHeight + 15) >> 4 : (frameHeight + 7) >> 3;
	int width = planes[plane_idx].width;
	int height = planes[plane_idx].height;
	int stride = planes[plane_idx].pitch;

	InitLengths(std::max(width, (int)8), bw);

	for (int i = 0; i < BINK_NB_SRC; i++)
		ReadBundle(bits, i);

	
    ref_start = planes[plane_idx].last ? planes[plane_idx].last
                                       : planes[plane_idx].current;

    ref_end   = ref_start + (bw - 1 + planes[plane_idx].pitch * (bh - 1)) * 8;

    for (int i = 0; i < 64; i++)
        coordmap[i] = (i & 7) + (i >> 3) * stride;

    for (by = 0; by < bh; by++) 
	{
        if (ReadBlockTypes(bits, &bundle[BINK_SRC_BLOCK_TYPES]) < 0)
            return -1;
        if (ReadBlockTypes(bits, &bundle[BINK_SRC_SUB_BLOCK_TYPES]) < 0)
            return -1;
        if (ReadColors(bits, &bundle[BINK_SRC_COLORS]) < 0)
            return -1;
        if (ReadPatterns(bits, &bundle[BINK_SRC_PATTERN]) < 0)
            return -1;
        if (ReadMotionValues(bits, &bundle[BINK_SRC_X_OFF]) < 0)
            return -1;
        if (ReadMotionValues(bits, &bundle[BINK_SRC_Y_OFF]) < 0)
            return -1;
        if (ReadDCs(bits, &bundle[BINK_SRC_INTRA_DC], kDCstartBits, 0) < 0)
            return -1;
        if (ReadDCs(bits, &bundle[BINK_SRC_INTER_DC], kDCstartBits, 1) < 0)
            return -1;
        if (ReadRuns(bits, &bundle[BINK_SRC_RUN]) < 0)
            return -1;

        if (by == bh)
            break;

		dst  = planes[plane_idx].current + 8 * by * planes[plane_idx].pitch;
        prev = (planes[plane_idx].last ? planes[plane_idx].last
                                       : planes[plane_idx].current) + 8*by*stride;

        for (bx = 0; bx < bw; bx++, dst += 8, prev += 8)
		{
            blk = GetValue(BINK_SRC_BLOCK_TYPES);

            // 16x16 block type on odd line means part of the already decoded block, so skip it
            if ((by & 1) && blk == SCALED_BLOCK) {
                bx++;
                dst  += 8;
                prev += 8;
                continue;
            }

            switch (blk) 
			{
				case SKIP_BLOCK:
				{
					PutPixelsTab(dst, prev, stride, 8);
					break;
				}
				case SCALED_BLOCK:
				{
					// This is 16x16 block which has its own subtype, which corresponds to other block types (range 3-9). Blocks should be decoded in the 
					// same way and then scaled twice.
					blk = GetValue(BINK_SRC_SUB_BLOCK_TYPES);

					switch (blk)
					{
						case RUN_BLOCK:
						{
							scan = bink_patterns[bits.GetBits(4)];
							int i = 0;
							do {
								int run = GetValue(BINK_SRC_RUN) + 1;

								i += run;
								if (i > 64) {
									BinkCommon::LogError("Run went out of bounds");
									return -1;
								}
								if (bits.GetBit()) {
									v = GetValue(BINK_SRC_COLORS);
									for (int j = 0; j < run; j++)
										ublock[*scan++] = v;
								} else {
									for (int j = 0; j < run; j++)
										ublock[*scan++] = GetValue(BINK_SRC_COLORS);
								}
							} while (i < 63);
							if (i == 63)
								ublock[*scan++] = GetValue(BINK_SRC_COLORS);
							break;
						}
						case INTRA_BLOCK:
						{
							memset(dctblock, 0, sizeof(*dctblock) * 64);
							dctblock[0] = GetValue(BINK_SRC_INTRA_DC);
							ReadDCTcoeffs(bits, dctblock, bink_scan, bink_intra_quant, -1);
							bink_idct_put_c(ublock, 8, dctblock);
							break;
						}
						case FILL_BLOCK:
						{
							v = GetValue(BINK_SRC_COLORS);

							FillBlockTab(dst, v, stride, 16);
							break;
						}
						case PATTERN_BLOCK:
						{
							for (int i = 0; i < 2; i++)
								col[i] = GetValue(BINK_SRC_COLORS);

							for (int j = 0; j < 8; j++) {
								v = GetValue(BINK_SRC_PATTERN);
								for (int k = 0; k < 8; k++, v >>= 1)
									ublock[k + j*8] = col[v & 1];
							}
							break;
						}
						case RAW_BLOCK:
						{
							for (int j = 0; j < 8; j++)
								for (int i = 0; i < 8; i++)
									ublock[i + j*8] = GetValue(BINK_SRC_COLORS);
							break;
						}
						default:
							BinkCommon::LogError("Incorrect 16x16 block type");
							return -1;
					}

					if (blk != FILL_BLOCK)
					{
						scale_block_c(ublock, dst, stride);
					}
					bx++;
					dst  += 8;
					prev += 8;
					break;
				}
				case MOTION_BLOCK:
				{
					// Copy 8x8 block from previous frame with some offset (x and y values decoded at the beginning of the block)
					xoff = GetValue(BINK_SRC_X_OFF);
					yoff = GetValue(BINK_SRC_Y_OFF);

					ref = prev + xoff + yoff * stride;
					if (ref < ref_start || ref > ref_end) {
						BinkCommon::LogError("MOTION_BLOCK - Copy out of bounds");
						return -1;
					}

					PutPixelsTab(dst, ref, stride, 8);
					break;
				}
				case RUN_BLOCK:
				{
					scan = bink_patterns[bits.GetBits(4)];
					int i = 0;
					do {
						int run = GetValue(BINK_SRC_RUN) + 1;

						i += run;
						if (i > 64) {
							BinkCommon::LogError("Run went out of bounds");
							return -1;
						}
						if (bits.GetBit()) {
							v = GetValue(BINK_SRC_COLORS);
							for (int j = 0; j < run; j++)
								dst[coordmap[*scan++]] = v;
						} else {
							for (int j = 0; j < run; j++)
								dst[coordmap[*scan++]] = GetValue(BINK_SRC_COLORS);
						}
					} while (i < 63);
					if (i == 63)
						dst[coordmap[*scan++]] = GetValue(BINK_SRC_COLORS);
					break;
				}
				case RESIDUE_BLOCK:
				{
					// motion and residue
					xoff = GetValue(BINK_SRC_X_OFF);
					yoff = GetValue(BINK_SRC_Y_OFF);

					ref = prev + xoff + yoff * stride;
					if (ref < ref_start || ref > ref_end) {
						BinkCommon::LogError("Copy out of bounds");
						return -1;
					}

					PutPixelsTab(dst, ref, stride, 8);

					ClearBlock(block);

					// number of masks to decode
					v = bits.GetBits(7);
					ReadResidue(bits, block, v);

					AddPixels8(dst, block, stride);
					break;
				}
				case INTRA_BLOCK:
				{
					memset(dctblock, 0, sizeof(*dctblock) * 64);
					dctblock[0] = GetValue(BINK_SRC_INTRA_DC);
					ReadDCTcoeffs(bits, dctblock, bink_scan, bink_intra_quant, -1);
					bink_idct_put_c(dst, stride, dctblock);
					break;
				}
				case FILL_BLOCK:
				{
					// Fill 8x8 block with one value from Colors
					v = GetValue(BINK_SRC_COLORS);

					FillBlockTab(dst, v, stride, 8);
					break;
				}
				case INTER_BLOCK:
				{
					xoff = GetValue(BINK_SRC_X_OFF);
					yoff = GetValue(BINK_SRC_Y_OFF);

					ref = prev + xoff + yoff * stride;
					if (ref < ref_start || ref > ref_end) {
						BinkCommon::LogError("Copy out of bounds");
						return -1;
					}

					PutPixelsTab(dst, ref, stride, 8);

					memset(dctblock, 0, sizeof(*dctblock) * 64);
					dctblock[0] = GetValue(BINK_SRC_INTER_DC);

					ReadDCTcoeffs(bits, dctblock, bink_scan, bink_inter_quant, -1);

					bink_idct_add_c(dst, stride, dctblock);
					break;
				}
				case PATTERN_BLOCK:
				{
					// Fill block with two colours taken from Colors values using 8 values from Patterns
					for (int i = 0; i < 2; i++)
						col[i] = GetValue(BINK_SRC_COLORS);

					for (int i = 0; i < 8; i++) {
						v = GetValue(BINK_SRC_PATTERN);
						for (int j = 0; j < 8; j++, v >>= 1)
							dst[i*stride + j] = col[v & 1];
					}
					break;
				}
				case RAW_BLOCK:
				{
					for (int i = 0; i < 8; i++)
						memcpy(dst + i*stride, bundle[BINK_SRC_COLORS].cur_ptr + i*8, 8);
					bundle[BINK_SRC_COLORS].cur_ptr += 64;
					break;
				}
				default:
				{
					BinkCommon::LogError("Unknown block type");
					return -1;
				}
			}
		}
	}

	if (bits.GetPosition() & 0x1F) //next plane data starts at 32-bit boundary
		bits.SkipBits(32 - (bits.GetPosition() & 0x1F));

	return 0;
}

int BinkDecoder::DecodeFrame(BinkCommon::BitReader &bits)
{
    if (hasAlpha) {
		if (signature >= kBIKiID)
            bits.SkipBits(32); // skip alpha plane data size

        if (DecodePlane(bits, 3, 0) < 0)
            return -1;
    }

    if (signature >= kBIKiID)
        bits.SkipBits(32);

    for (int plane = 0; plane < 3; plane++)
	{
        int plane_idx = (!plane || !swapPlanes) ? plane : (plane ^ 3);

		if (signature > kBIKbID) {
            if (DecodePlane(bits, plane_idx, !!plane) < 0)
                return -1;
        } else {
            if (DecodePlane(bits, plane_idx, !!plane) < 0)
                return -1;
        }
        if (bits.GetPosition() >= bits.GetSize())
            break;
    }

	if (signature > kBIKbID) 
	{
		for (uint32_t i = 0; i < planes.size(); i++)
			planes[i].Swap();
	}

	return 1;
}

/*
 * Bink DSP routines
 */

#define A1  2896 /* (1/sqrt(2))<<12 */
#define A2  2217
#define A3  3784
#define A4 -5352

#define IDCT_TRANSFORM(dest,s0,s1,s2,s3,s4,s5,s6,s7,d0,d1,d2,d3,d4,d5,d6,d7,munge,src) {\
    const int a0 = (src)[s0] + (src)[s4]; \
    const int a1 = (src)[s0] - (src)[s4]; \
    const int a2 = (src)[s2] + (src)[s6]; \
    const int a3 = (A1*((src)[s2] - (src)[s6])) >> 11; \
    const int a4 = (src)[s5] + (src)[s3]; \
    const int a5 = (src)[s5] - (src)[s3]; \
    const int a6 = (src)[s1] + (src)[s7]; \
    const int a7 = (src)[s1] - (src)[s7]; \
    const int b0 = a4 + a6; \
    const int b1 = (A3*(a5 + a7)) >> 11; \
    const int b2 = ((A4*a5) >> 11) - b0 + b1; \
    const int b3 = (A1*(a6 - a4) >> 11) - b2; \
    const int b4 = ((A2*a7) >> 11) + b3 - b1; \
    (dest)[d0] = munge(a0+a2   +b0); \
    (dest)[d1] = munge(a1+a3-a2+b2); \
    (dest)[d2] = munge(a1-a3+a2+b3); \
    (dest)[d3] = munge(a0-a2   -b4); \
    (dest)[d4] = munge(a0-a2   +b4); \
    (dest)[d5] = munge(a1-a3+a2-b3); \
    (dest)[d6] = munge(a1+a3-a2-b2); \
    (dest)[d7] = munge(a0+a2   -b0); \
}
/* end IDCT_TRANSFORM macro */

#define MUNGE_NONE(x) (x)
#define IDCT_COL(dest,src) IDCT_TRANSFORM(dest,0,8,16,24,32,40,48,56,0,8,16,24,32,40,48,56,MUNGE_NONE,src)

#define MUNGE_ROW(x) (((x) + 0x7F)>>8)
#define IDCT_ROW(dest,src) IDCT_TRANSFORM(dest,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,MUNGE_ROW,src)

static inline void bink_idct_col(int *dest, const int32_t *src)
{
    if ((src[8]|src[16]|src[24]|src[32]|src[40]|src[48]|src[56])==0) {
        dest[0]  =
        dest[8]  =
        dest[16] =
        dest[24] =
        dest[32] =
        dest[40] =
        dest[48] =
        dest[56] = src[0];
    } else {
        IDCT_COL(dest, src);
    }
}

static void bink_idct_c(int32_t *block)
{
    int temp[64];

    for (int i = 0; i < 8; i++)
        bink_idct_col(&temp[i], &block[i]);
    for (int i = 0; i < 8; i++) {
        IDCT_ROW( (&block[8*i]), (&temp[8*i]) );
    }
}

static void bink_idct_add_c(uint8_t *dest, int linesize, int32_t *block)
{
    bink_idct_c(block);
    for (int i = 0; i < 8; i++, dest += linesize, block += 8)
        for (int j = 0; j < 8; j++)
             dest[j] += block[j];
}

static void bink_idct_put_c(uint8_t *dest, int linesize, int32_t *block)
{
    int temp[64];
    for (int i = 0; i < 8; i++)
        bink_idct_col(&temp[i], &block[i]);
    for (int i = 0; i < 8; i++) {
        IDCT_ROW( (&dest[i*linesize]), (&temp[8*i]) );
    }
}

static void scale_block_c(const uint8_t src[64], uint8_t *dst, int linesize)
{
    int i, j;
    uint16_t *dst1 = (uint16_t *) dst;
    uint16_t *dst2 = (uint16_t *)(dst + linesize);

    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            dst1[i] = dst2[i] = src[i] * 0x0101;
        }
        src  += 8;
        dst1 += linesize;
        dst2 += linesize;
    }
}

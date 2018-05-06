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

#ifndef _BinkVideo_h_
#define _BinkVideo_h_

#include <stdint.h>
#include "HuffmanVLC.h"

/** number of bits used to store first DC value in bundle */
const int kDCstartBits = 11;

/**
 * IDs for different data types used in Bink video codec
 */
enum Sources {
    BINK_SRC_BLOCK_TYPES = 0, ///< 8x8 block types
    BINK_SRC_SUB_BLOCK_TYPES, ///< 16x16 block types (a subset of 8x8 block types)
    BINK_SRC_COLORS,          ///< pixel values used for different block types
    BINK_SRC_PATTERN,         ///< 8-bit values for 2-colour pattern fill
    BINK_SRC_X_OFF,           ///< X components of motion value
    BINK_SRC_Y_OFF,           ///< Y components of motion value
    BINK_SRC_INTRA_DC,        ///< DC values for intrablocks with DCT
    BINK_SRC_INTER_DC,        ///< DC values for interblocks with DCT
    BINK_SRC_RUN,             ///< run lengths for special fill block

    BINK_NB_SRC
};

static BinkCommon::VLCtable bink_trees[16];

/**
 * data needed to decode 4-bit Huffman-coded value
 */
typedef struct Tree 
{
    int     vlc_num;       ///< tree number (in bink_trees[])
    uint8_t symbols[16];   ///< leaf value to symbol mapping
} Tree;

/**
 * data structure used for decoding single Bink data type
 */
typedef struct Bundle
{
    int     len;       ///< length of number of entries to decode (in bits)
    Tree    tree;      ///< Huffman tree-related data
    uint8_t *data;     ///< buffer for decoded symbols
    uint8_t *data_end; ///< buffer end
    uint8_t *cur_dec;  ///< pointer to the not yet decoded part of the buffer
    uint8_t *cur_ptr;  ///< pointer to the data that is not read from buffer yet
} Bundle;

/**
 * Bink video block types
 */
enum BlockTypes {
    SKIP_BLOCK = 0, ///< skipped block
    SCALED_BLOCK,   ///< block has size 16x16
    MOTION_BLOCK,   ///< block is copied from previous frame with some offset
    RUN_BLOCK,      ///< block is composed from runs of colours with custom scan order
    RESIDUE_BLOCK,  ///< motion block with some difference added
    INTRA_BLOCK,    ///< intra DCT block
    FILL_BLOCK,     ///< block is filled with single colour
    INTER_BLOCK,    ///< motion block with DCT applied to the difference
    PATTERN_BLOCK,  ///< block is filled with two colours following custom pattern
    RAW_BLOCK,      ///< uncoded 8x8 block
};

struct VideoFrame
{
	uint32_t offset;
	uint32_t keyFrame;
	uint32_t size;
};

#endif
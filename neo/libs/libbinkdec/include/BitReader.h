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

#ifndef _BinkBitReader_h_
#define _BinkBitReader_h_

#include <stdint.h>
#include "FileStream.h"

namespace BinkCommon {

class BitReader
{
	public:
		BitReader(BinkCommon::FileStream &file, uint32_t size);
		~BitReader();
		uint32_t GetBit();
		uint32_t GetBits(uint32_t n);
		void SkipBits(uint32_t n);

		uint32_t GetSize();
		uint32_t GetPosition();

	private:
		uint32_t totalSize;
		uint32_t currentOffset;
		uint32_t bytesRead;

		BinkCommon::FileStream *file;

		uint8_t cache;
		uint32_t nCachedBits;

		void FillCache();
};

} // close namespace BinkCommon

#endif
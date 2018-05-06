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

#include <HuffmanVLC.h>

namespace BinkCommon {

uint8_t VLC_GetCodeBits(BitReader &bits, VLCtable &table)
{
	uint8_t codeBits = 0;

	// search each length array
	for (uint32_t i = 0; i < table.size(); i++)
	{
		// get and add a new bit to codeBits
		uint8_t theBit = bits.GetBit() << i;
		codeBits |= theBit;

		// search for a code match
		for (uint32_t j = 0; j < table[i].size(); j++)
		{
			if (codeBits == table[i][j].code)
			{
				return table[i][j].symbol;
			}
		}
	}

	// shouldn't get here..
	return 0;
}

void VLC_InitTable(VLCtable &table, uint32_t maxLength, uint32_t size, const uint8_t *lengths, const uint8_t *bits)
{
	table.resize(maxLength);

	for (uint32_t i = 0; i < size; i++)
	{
		VLC newCode;
		newCode.symbol = i;
		newCode.code   = bits[i];

		uint8_t codeLength = lengths[i];

		// add the code to the array corresponding to the length
		table[codeLength - 1].push_back(newCode);
	}
}

uint32_t VLC_GetSize(VLCtable &table)
{
	return table.size();
}

} // close namespace BinkCommon

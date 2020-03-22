/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2015 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __SWF_TYPES1_H__
#define __SWF_TYPES1_H__

ID_INLINE float SWFTWIP( int twip )
{
	return twip * ( 1.0f / 20.0f );
}
ID_INLINE float SWFFIXED16( int fixed )
{
	return fixed * ( 1.0f / 65536.0f );
}
ID_INLINE float SWFFIXED8( int fixed )
{
	return fixed * ( 1.0f / 256.0f );
}

// RB begin
ID_INLINE int FLOAT2SWFTWIP( float value )
{
	return ( int )( value * 20 );
}

ID_INLINE int FLOAT2SWFFIXED16( float value )
{
	return ( int )( value * 65536 );
}

ID_INLINE int FLOAT2SWFFIXED8( float value )
{
	return ( int )( value * 256 );
}
// RB end

struct swfHeader_t
{
	byte compression;
	byte W;
	byte S;
	byte version;
	uint32 fileLength;
};
struct swfRect_t
{
	swfRect_t();
	idVec2 tl;
	idVec2 br;

	// RB: helpers
	swfRect_t( float x, float y, float w, float h )
	{
		tl.x = x;
		tl.y = y;
		br.x = x + w;
		br.y = y + h;
	}

	float x() const
	{
		return tl.x;
	}

	float y() const
	{
		return tl.y;
	}

	float w() const
	{
		return fabs( br.x - tl.x );
	}

	float h() const
	{
		return fabs( br.y - tl.y );
	}

	float Bottom() const
	{
		return br.y;
	}
	// RB end
};
struct swfMatrix_t
{
	swfMatrix_t();
	float xx, yy;
	float xy, yx;
	float tx, ty;
	idVec2		Scale( const idVec2& in ) const;
	idVec2		Transform( const idVec2& in ) const;
	swfMatrix_t Multiply( const swfMatrix_t& a ) const;
	swfMatrix_t	Inverse() const;
	swfMatrix_t& operator=( const swfMatrix_t& a )
	{
		xx = a.xx;
		yy = a.yy;
		xy = a.xy;
		yx = a.yx;
		tx = a.tx;
		ty = a.ty;
		return *this;
	}

	// RB begin
	bool operator==( const swfMatrix_t& a )
	{
		return ( xx == a.xx && yy == a.yy && xy == a.xy && yx == a.yx && tx == a.tx && ty == a.ty );

	}

	bool operator!=( const swfMatrix_t& a )
	{
		return !( xx == a.xx && yy == a.yy && xy == a.xy && yx == a.yx && tx == a.tx && ty == a.ty );

	}
	// RB end
};
struct swfColorRGB_t
{
	swfColorRGB_t();
	idVec4 ToVec4() const;
	uint8 r, g, b;
};
struct swfColorRGBA_t : public swfColorRGB_t
{
	swfColorRGBA_t();
	idVec4 ToVec4() const;
	uint8 a;
};
struct swfLineStyle_t
{
	swfLineStyle_t();
	uint16 startWidth;
	uint16 endWidth;
	swfColorRGBA_t startColor;
	swfColorRGBA_t endColor;
};
struct swfGradientRecord_t
{
	swfGradientRecord_t();
	uint8 startRatio;
	uint8 endRatio;
	swfColorRGBA_t startColor;
	swfColorRGBA_t endColor;
};
struct swfGradient_t
{
	swfGradient_t();
	uint8 numGradients;
	swfGradientRecord_t gradientRecords[ 16 ];
};
struct swfFillStyle_t
{
	swfFillStyle_t();
	uint8 type;			// 0 = solid, 1 = gradient, 4 = bitmap
	uint8 subType;		// 0 = linear, 2 = radial, 3 = focal; 0 = repeat, 1 = clamp, 2 = near repeat, 3 = near clamp
	swfColorRGBA_t startColor;	// if type = 0
	swfColorRGBA_t endColor;	// if type = 0
	swfMatrix_t startMatrix;	// if type > 0
	swfMatrix_t endMatrix;		// if type > 0
	swfGradient_t gradient;	// if type = 1
	float focalPoint;		// if type = 1 and subType = 3
	uint16 bitmapID;		// if type = 4
};
class idSWFShapeDrawFill
{
public:
	swfFillStyle_t style;
	idList< idVec2, TAG_SWF > startVerts;
	idList< idVec2, TAG_SWF > endVerts;
	idList< uint16, TAG_SWF > indices;
};
class idSWFShapeDrawLine
{
public:
	swfLineStyle_t style;
	idList< idVec2, TAG_SWF > startVerts;
	idList< idVec2, TAG_SWF > endVerts;
	idList< uint16, TAG_SWF > indices;
};
class idSWFShape
{
public:
	~idSWFShape()
	{
		fillDraws.Clear();
		lineDraws.Clear();
	}
	swfRect_t startBounds;
	swfRect_t endBounds;
	idList< idSWFShapeDrawFill, TAG_SWF > fillDraws;
	idList< idSWFShapeDrawLine, TAG_SWF > lineDraws;
};
class idSWFFontGlyph
{
public:
	idSWFFontGlyph();
	uint16 code;
	int16 advance;
	// RB: verts and indices are not used by the renderer
	idList< idVec2, TAG_SWF > verts;
	idList< uint16, TAG_SWF > indices;
};
class idSWFFont
{
public:
	idSWFFont();
	class idFont* fontID;
	int16 ascent;
	int16 descent;
	int16 leading;
	idList< idSWFFontGlyph, TAG_SWF > glyphs;
};
class idSWFTextRecord
{
public:
	idSWFTextRecord();
	uint16 fontID;
	swfColorRGBA_t color;
	int16 xOffset;
	int16 yOffset;
	uint16 textHeight;
	uint16 firstGlyph;
	uint8 numGlyphs;
};
struct swfGlyphEntry_t
{
	swfGlyphEntry_t();
	uint32 index;
	int32 advance;
};
class idSWFText
{
public:
	swfRect_t bounds;
	swfMatrix_t matrix;
	idList< idSWFTextRecord, TAG_SWF > textRecords;
	idList< swfGlyphEntry_t, TAG_SWF > glyphs;
};
enum swfEditTextFlags_t
{
	SWF_ET_NONE = 0,
	SWF_ET_WORDWRAP = BIT( 0 ),
	SWF_ET_MULTILINE = BIT( 1 ),
	SWF_ET_PASSWORD = BIT( 2 ),
	SWF_ET_READONLY = BIT( 3 ),
	SWF_ET_AUTOSIZE = BIT( 4 ),
	SWF_ET_BORDER = BIT( 5 ),
};
enum swfEditTextAlign_t
{
	SWF_ET_ALIGN_LEFT,
	SWF_ET_ALIGN_RIGHT,
	SWF_ET_ALIGN_CENTER,
	SWF_ET_ALIGN_JUSTIFY
};
enum swfTextRenderMode_t
{
	SWF_TEXT_RENDER_NORMAL = 0,
	SWF_TEXT_RENDER_RANDOM_APPEAR,
	SWF_TEXT_RENDER_RANDOM_APPEAR_CAPS,
	SWF_TEXT_RENDER_PARAGRAPH,
	SWF_TEXT_RENDER_AUTOSCROLL,
	SWF_TEXT_RENDER_MODE_COUNT,
};

class idSWFEditText
{
public:
	idSWFEditText();
	swfRect_t bounds;
	uint32 flags;
	uint16 fontID;
	uint16 fontHeight;
	swfColorRGBA_t color;
	uint16 maxLength;
	swfEditTextAlign_t align;
	uint16 leftMargin;
	uint16 rightMargin;
	uint16 indent;
	int16 leading;
	idStr variable;
	idStr initialText;
};
struct swfColorXform_t
{
	swfColorXform_t();
	idVec4 mul;
	idVec4 add;
	swfColorXform_t Multiply( const swfColorXform_t& a ) const;
	swfColorXform_t& operator=( const swfColorXform_t& a )
	{
		mul = a.mul;
		add = a.add;
		return *this;
	}
};
struct swfDisplayEntry_t
{
	swfDisplayEntry_t();
	uint16 characterID;
	uint16 depth;
	uint16 clipDepth;
	uint16 blendMode;
	swfMatrix_t matrix;
	swfColorXform_t cxf;
	float ratio;
	// if this entry is a sprite, then this will point to the specific instance of that sprite
	class idSWFSpriteInstance* spriteInstance;
	// if this entry is text, then this will point to the specific instance of the text
	class idSWFTextInstance* textInstance;
};
struct swfRenderState_t
{
	swfRenderState_t();
	swfMatrix_t matrix;
	swfColorXform_t cxf;
	const idMaterial* material;
	int materialWidth;
	int materialHeight;
	int activeMasks;
	uint8 blendMode;
	float ratio;
	stereoDepthType_t stereoDepth;
};

ID_INLINE swfRect_t::swfRect_t() :
	tl( 0.0f, 0.0f ),
	br( 0.0f, 0.0f )
{
}

ID_INLINE swfMatrix_t::swfMatrix_t() :
	xx( 1.0f ), yy( 1.0f ),
	xy( 0.0f ), yx( 0.0f ),
	tx( 0.0f ), ty( 0.0f )
{
}

ID_INLINE idVec2 swfMatrix_t::Scale( const idVec2& in ) const
{
	return idVec2( ( in.x * xx ) + ( in.y * xy ),
				   ( in.y * yy ) + ( in.x * yx ) );
}

ID_INLINE idVec2 swfMatrix_t::Transform( const idVec2& in ) const
{
	return idVec2( ( in.x * xx ) + ( in.y * xy ) + tx,
				   ( in.y * yy ) + ( in.x * yx ) + ty );
}

ID_INLINE swfMatrix_t swfMatrix_t::Inverse() const
{
	swfMatrix_t inverse;
	float det = ( ( xx * yy ) - ( yx * xy ) );
	if( idMath::Fabs( det ) < idMath::FLT_SMALLEST_NON_DENORMAL )
	{
		return *this;
	}
	float invDet = 1.0f / det;
	inverse.xx = invDet *  yy;
	inverse.yx = invDet * -yx;
	inverse.xy = invDet * -xy;
	inverse.yy = invDet *  xx;
	//inverse.tx = invDet * ( xy * ty ) - ( yy * tx );
	//inverse.ty = invDet * ( yx * tx ) - ( xx * ty );
	return inverse;
}

ID_INLINE swfMatrix_t swfMatrix_t::Multiply( const swfMatrix_t& a ) const
{
	swfMatrix_t result;
	result.xx = xx * a.xx + yx * a.xy;
	result.yx = xx * a.yx + yx * a.yy;
	result.xy = xy * a.xx + yy * a.xy;
	result.yy = xy * a.yx + yy * a.yy;
	result.tx = tx * a.xx + ty * a.xy + a.tx;
	result.ty = tx * a.yx + ty * a.yy + a.ty;
	return result;
}

ID_INLINE swfColorRGB_t::swfColorRGB_t() :
	r( 255 ), g( 255 ), b( 255 )
{
}

ID_INLINE idVec4 swfColorRGB_t::ToVec4() const
{
	return idVec4( r * ( 1.0f / 255.0f ), g * ( 1.0f / 255.0f ), b * ( 1.0f / 255.0f ), 1.0f );
}

ID_INLINE swfColorRGBA_t::swfColorRGBA_t() :
	a( 255 )
{
}

ID_INLINE idVec4 swfColorRGBA_t::ToVec4() const
{
	return idVec4( r * ( 1.0f / 255.0f ), g * ( 1.0f / 255.0f ), b * ( 1.0f / 255.0f ), a * ( 1.0f / 255.0f ) );
}

ID_INLINE swfLineStyle_t::swfLineStyle_t() :
	startWidth( 20 ),
	endWidth( 20 )
{
}

ID_INLINE swfGradientRecord_t::swfGradientRecord_t() :
	startRatio( 0 ),
	endRatio( 0 )
{
}

ID_INLINE swfGradient_t::swfGradient_t() :
	numGradients( 0 )
{
}

ID_INLINE swfFillStyle_t::swfFillStyle_t() :
	type( 0 ),
	subType( 0 ),
	focalPoint( 0.0f ),
	bitmapID( 0 )
{
}

ID_INLINE swfColorXform_t::swfColorXform_t() :
	mul( 1.0f, 1.0f, 1.0f, 1.0f ),
	add( 0.0f, 0.0f, 0.0f, 0.0f )
{
}

ID_INLINE swfColorXform_t swfColorXform_t::Multiply( const swfColorXform_t& a ) const
{
	swfColorXform_t result;
	result.mul = mul.Multiply( a.mul );
	result.add = ( add.Multiply( a.mul ) ) + a.add;
	return result;
}

ID_INLINE swfDisplayEntry_t::swfDisplayEntry_t() :
	characterID( 0 ),
	depth( 0 ),
	clipDepth( 0 ),
	blendMode( 0 ),
	ratio( 0.0f ),
	spriteInstance( NULL ),
	textInstance( NULL )
{
}

ID_INLINE swfRenderState_t::swfRenderState_t() :
	material( NULL ),
	materialWidth( 0 ),
	materialHeight( 0 ),
	activeMasks( 0 ),
	blendMode( 0 ),
	ratio( 0.0f ),
	stereoDepth( STEREO_DEPTH_TYPE_NONE )
{
}

ID_INLINE idSWFFontGlyph::idSWFFontGlyph() :
	code( 0 ),
	advance( 0 )
{
}

ID_INLINE idSWFFont::idSWFFont() :
	fontID( 0 ),
	ascent( 0 ),
	descent( 0 ),
	leading( 0 )
{
}

ID_INLINE idSWFTextRecord::idSWFTextRecord() :
	fontID( 0 ),
	xOffset( 0 ),
	yOffset( 0 ),
	textHeight( 0 ),
	firstGlyph( 0 ),
	numGlyphs( 0 )
{
}

ID_INLINE idSWFEditText::idSWFEditText() :
	flags( SWF_ET_NONE ),
	fontID( 0 ),
	fontHeight( 24 ),
	maxLength( 0xFFFF ),
	align( SWF_ET_ALIGN_LEFT ),
	leftMargin( 0 ),
	rightMargin( 0 ),
	indent( 0 ),
	leading( 0 )
{
}

ID_INLINE swfGlyphEntry_t::swfGlyphEntry_t() :
	index( 0 ),
	advance( 0 )
{
}

#endif // !__SWF_TYPES1_H__

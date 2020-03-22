/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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
#ifndef IDRECTANGLE_H_
#define IDRECTANGLE_H_
//
// simple rectangle
//
extern void RotateVector( idVec3& v, idVec3 origin, float a, float c, float s );
class idRectangle
{
public:
	float x;    // horiz position
	float y;    // vert position
	float w;    // width
	float h;    // height;
	idRectangle()
	{
		x = y = w = h = 0.0;
	}
	idRectangle( float ix, float iy, float iw, float ih )
	{
		x = ix;
		y = iy;
		w = iw;
		h = ih;
	}
	float Bottom() const
	{
		return y + h;
	}
	float Right() const
	{
		return x + w;
	}
	void Offset( float x, float y )
	{
		this->x += x;
		this->y += y;
	}
	bool Contains( float xt, float yt )
	{
		if( w == 0.0 && h == 0.0 )
		{
			return false;
		}
		if( xt >= x && xt <= Right() && yt >= y && yt <= Bottom() )
		{
			return true;
		}
		return false;
	}
	void Empty()
	{
		x = y = w = h = 0.0;
	};

	void ClipAgainst( idRectangle r, bool sizeOnly )
	{
		if( !sizeOnly )
		{
			if( x < r.x )
			{
				w -= r.x - x;
				x = r.x;
			}
			if( y < r.y )
			{
				h -= r.y - y;
				y = r.y;
			}
		}
		if( x + w > r.x + r.w )
		{
			w = ( r.x + r.w ) - x;
		}
		if( y + h > r.y + r.h )
		{
			h = ( r.y + r.h ) - y;
		}
	}



	void Rotate( float a, idRectangle& out )
	{
		idVec3 p1, p2, p3, p4, p5;
		float c, s;
		idVec3 center;
		center.Set( ( x + w ) / 2.0, ( y + h ) / 2.0, 0 );
		p1.Set( x, y, 0 );
		p2.Set( Right(), y, 0 );
		p4.Set( x, Bottom(), 0 );
		if( a )
		{
			s = sin( DEG2RAD( a ) );
			c = cos( DEG2RAD( a ) );
		}
		else
		{
			s = c = 0;
		}
		RotateVector( p1, center, a, c, s );
		RotateVector( p2, center, a, c, s );
		RotateVector( p4, center, a, c, s );
		out.x = p1.x;
		out.y = p1.y;
		out.w = ( p2 - p1 ).Length();
		out.h = ( p4 - p1 ).Length();
	}

	idRectangle& operator+=( const idRectangle& a );
	idRectangle& operator-=( const idRectangle& a );
	idRectangle& operator/=( const idRectangle& a );
	idRectangle& operator/=( const float a );
	idRectangle& operator*=( const float a );
	idRectangle& operator=( const idVec4 v );
	int operator==( const idRectangle& a ) const;
	float& 	operator[]( const int index );
	char* String() const;
	const idVec4& ToVec4() const;

};

ID_INLINE const idVec4& idRectangle::ToVec4() const
{
	return *reinterpret_cast<const idVec4*>( &x );
}


ID_INLINE idRectangle& idRectangle::operator+=( const idRectangle& a )
{
	x += a.x;
	y += a.y;
	w += a.w;
	h += a.h;

	return *this;
}

ID_INLINE idRectangle& idRectangle::operator/=( const idRectangle& a )
{
	x /= a.x;
	y /= a.y;
	w /= a.w;
	h /= a.h;

	return *this;
}

ID_INLINE idRectangle& idRectangle::operator/=( const float a )
{
	float inva = 1.0f / a;
	x *= inva;
	y *= inva;
	w *= inva;
	h *= inva;

	return *this;
}

ID_INLINE idRectangle& idRectangle::operator-=( const idRectangle& a )
{
	x -= a.x;
	y -= a.y;
	w -= a.w;
	h -= a.h;

	return *this;
}

ID_INLINE idRectangle& idRectangle::operator*=( const float a )
{
	x *= a;
	y *= a;
	w *= a;
	h *= a;

	return *this;
}


ID_INLINE idRectangle& idRectangle::operator=( const idVec4 v )
{
	x = v.x;
	y = v.y;
	w = v.z;
	h = v.w;
	return *this;
}

ID_INLINE int idRectangle::operator==( const idRectangle& a ) const
{
	return ( x == a.x && y == a.y && w == a.w && a.h );
}

ID_INLINE float& idRectangle::operator[]( int index )
{
	return ( &x )[ index ];
}

class idRegion
{
public:
	idRegion() { };

	void Empty()
	{
		rects.Clear();
	}

	bool Contains( float xt, float yt )
	{
		int c = rects.Num();
		for( int i = 0; i < c; i++ )
		{
			if( rects[i].Contains( xt, yt ) )
			{
				return true;
			}
		}
		return false;
	}

	void AddRect( float x, float y, float w, float h )
	{
		rects.Append( idRectangle( x, y, w, h ) );
	}

	int GetRectCount()
	{
		return rects.Num();
	}

	idRectangle* GetRect( int index )
	{
		if( index >= 0 && index < rects.Num() )
		{
			return &rects[index];
		}
		return NULL;
	}

protected:

	idList<idRectangle, TAG_OLD_UI> rects;
};


#endif

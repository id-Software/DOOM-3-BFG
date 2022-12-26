
/*
Copyright (c) 2013, christopher stones < chris.stones@zoho.com >
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



/**
 * BinPack2D is a 2 dimensional, multi-bin, bin-packer. ( Texture Atlas Array! )
 * It supports an arbitrary number of bins, at arbitrary sizes.
 * rectangles can be added one at a time, chunks at a time, or all at once.
 * rectangles that dont fit are reported back.
 * Data can be associated to rectangles before processing via a template, and recalled after processing.
 *
 * There is no documentation, See ExampleProgram() below for a taste.
 *
 * Instead of tracking 'free rectangles' like other solutions I've found online,
 * this algorithm tracks free 'top lefts', keeps them sorted by closest to origin, and puts new rectangles into
 * the first free top left that doesnt collide. Consuming a top left creates 2 new top lefts (x+w,y) and (x,y+h).
 * If a rectangle doesnt fit into a bin, before condisering the next bin, the current bin is re-tried with the rectangle rotated.
 * This SOMTIMES helps... but not always.. i might disable this in future !?
 *
 * This Header was origonally part of my rh_texture_packer program.
 * A program I wrote to take advantage of my nexus-7's GL_EXT_texture_array extension.
 * I wanted to be able to render out whole scenes with a single glDraw*
 * blah blah blah...
 */


/** ***** EXAMPLE CODE **************************************

 // Your data - whatever you want to associate with 'rectangle'
 class MyContent {
  public:
  std::string str;
  MyContent() : str("default string") {}
  MyContent(const std::string &str) : str(str) {}
  };

 int ExampleProgram() {

  srandom(0x69);

  // Create some 'content' to work on.
  BinPack2D::ContentAccumulator<MyContent> inputContent;

  for(int i=0;i<20;i++) {

    // random size for this content
    int width  = ((random() % 32)+1) * ((random() % 10)+1);
    int height = ((random() % 32)+1) * ((random() % 10)+1);

    // whatever data you want to associate with this content
    std::stringstream ss;
    ss << "box " << i;
    MyContent mycontent( ss.str().c_str() );

    // Add it
    inputContent += BinPack2D::Content<MyContent>(mycontent, BinPack2D::Coord(), BinPack2D::Size(width, height), false );
  }

  // Sort the input content by size... usually packs better.
  inputContent.Sort();

  // Create some bins! ( 2 bins, 128x128 in this example )
  BinPack2D::CanvasArray<MyContent> canvasArray =
    BinPack2D::UniformCanvasArrayBuilder<MyContent>(128,128,2).Build();

  // A place to store content that didnt fit into the canvas array.
  BinPack2D::ContentAccumulator<MyContent> remainder;

  // try to pack content into the bins.
  bool success = canvasArray.Place( inputContent, remainder );

  // A place to store packed content.
  BinPack2D::ContentAccumulator<MyContent> outputContent;

  // Read all placed content.
  canvasArray.CollectContent( outputContent );

  // parse output.
  typedef BinPack2D::Content<MyContent>::Vector::iterator binpack2d_iterator;
  printf("PLACED:\n");
  for( binpack2d_iterator itor = outputContent.Get().begin(); itor != outputContent.Get().end(); itor++ ) {

    const BinPack2D::Content<MyContent> &content = *itor;

    // retreive your data.
    const MyContent &myContent = content.content;

    printf("\t%9s of size %3dx%3d at position %3d,%3d,%2d rotated=%s\n",
	   myContent.str.c_str(),
	   content.size.w,
	   content.size.h,
	   content.coord.x,
	   content.coord.y,
	   content.coord.z,
	   (content.rotated ? "yes":" no"));
  }

  printf("NOT PLACED:\n");
  for( binpack2d_iterator itor = remainder.Get().begin(); itor != remainder.Get().end(); itor++ ) {

    const BinPack2D::Content<MyContent> &content = *itor;

    const MyContent &myContent = content.content;

    printf("\t%9s of size %3dx%3d\n",
	   myContent.str.c_str(),
	   content.size.w,
	   content.size.h);
  }

  exit(0);
}
*/



#pragma once

#include<vector>
#include<map>
#include<list>
#include<algorithm>
#include<math.h>
#include<sstream>

namespace BinPack2D
{

class Size
{

public:

	/*const*/
	int w;
	/*const*/ int h;

	Size( int w, int h )
		: w( w ),
		  h( h )
	{}

	bool operator < ( const Size& that ) const
	{
		if( this->w != that.w )
		{
			return this->w < that.w;
		}
		if( this->h != that.h )
		{
			return this->h < that.h;
		}
		return false;
	}
};

class Coord
{

public:

	typedef std::vector<Coord> Vector;
	typedef std::list<Coord>   List;

	/*const*/ int x;
	/*const*/ int y;
	/*const*/ int z;

	Coord()
		: x( 0 ),
		  y( 0 ),
		  z( 0 )
	{}

	Coord( int x, int y )
		: x( x ),
		  y( y ),
		  z( 0 )
	{}

	Coord( int x, int y, int z )
		: x( x ),
		  y( y ),
		  z( z )
	{}

	bool operator < ( const Coord& that ) const
	{
		if( this->x != that.x )
		{
			return this->x < that.x;
		}
		if( this->y != that.y )
		{
			return this->y < that.y;
		}
		if( this->z != that.z )
		{
			return this->z < that.z;
		}
		return false;
	}
};

template<typename _T> class Content
{

public:

	typedef std::vector<Content<_T> > Vector;

	/*const*/ bool rotated;
	/*const*/ Coord coord;
	/*const*/ Size  size;
	/*const*/ _T content;

	Content( const Content<_T>& src )
		: rotated( src.rotated ),
		  coord( src.coord ),
		  size( src.size ),
		  content( src.content )
	{}

	Content( const _T& content, const Coord& coord, const Size& size, bool rotated )
		:
		content( content ),
		coord( coord ),
		size( size ),
		rotated( rotated )
	{}

	void Rotate()
	{

		rotated = !rotated;
		size = Size( size.h, size.w );
	}

	bool intersects( const Content<_T>& that ) const
	{
		if( this->coord.x >= ( that.coord.x + that.size.w ) )
		{
			return false;
		}

		if( this->coord.y >= ( that.coord.y + that.size.h ) )
		{
			return false;
		}

		if( that.coord.x >= ( this->coord.x + this->size.w ) )
		{
			return false;
		}

		if( that.coord.y >= ( this->coord.y + this->size.h ) )
		{
			return false;
		}

		return true;
	}
};

template<typename _T> class Canvas
{

	Coord::List topLefts;
	typename Content<_T>::Vector contentVector;

	bool needToSort;

public:

	typedef Canvas<_T> CanvasT;
	typedef typename std::vector<CanvasT> Vector;

	static bool Place( Vector& canvasVector, const typename Content<_T>::Vector& contentVector, typename Content<_T>::Vector& remainder )
	{

		typename Content<_T>::Vector todo = contentVector;

		for( typename Vector::iterator itor = canvasVector.begin(); itor != canvasVector.end(); itor++ )
		{

			Canvas <_T>& canvas = *itor;

			remainder.clear();
			canvas.Place( todo, remainder );
			todo = remainder;
		}

		if( remainder.size() == 0 )
		{
			return true;
		}

		return false;
	}

	static bool Place( Vector& canvasVector, const typename Content<_T>::Vector& contentVector )
	{

		typename Content<_T>::Vector remainder;

		return Place( canvasVector, contentVector, remainder );
	}

	static bool Place( Vector& canvasVector, const Content<_T>& content )
	{

		typename Content<_T>::Vector contentVector( 1, content );

		return Place( canvasVector, contentVector );
	}

	const int w;
	const int h;

	Canvas( int w, int h )
		: needToSort( false ),
		  w( w ),
		  h( h )
	{
		topLefts.push_back( Coord( 0, 0 ) );
	}

	bool HasContent() const
	{

		return ( contentVector.size() > 0 ) ;
	}

	const typename Content<_T>::Vector& GetContents() const
	{

		return contentVector;
	}

	bool operator < ( const Canvas& that ) const
	{

		if( this->w != that.w )
		{
			return this->w < that.w;
		}
		if( this->h != that.h )
		{
			return this->h < that.h;
		}
		return false;
	}

	bool Place( const typename Content<_T>::Vector& contentVector, typename Content<_T>::Vector& remainder )
	{

		bool placedAll = true;

		for( typename Content<_T>::Vector::const_iterator itor = contentVector.begin(); itor != contentVector.end(); itor++ )
		{

			const Content<_T>& content = *itor;

			if( Place( content ) == false )
			{

				placedAll = false;
				remainder.push_back( content );
			}
		}

		return placedAll;
	}

	bool Place( Content<_T> content )
	{
		Sort();

		for( Coord::List::iterator itor = topLefts.begin(); itor != topLefts.end(); itor++ )
		{

			content.coord = *itor;

			if( Fits( content ) )
			{

				Use( content );
				topLefts.erase( itor );
				return true;
			}
		}

		// EXPERIMENTAL - TRY ROTATED?
		content.Rotate();
		for( Coord::List::iterator itor = topLefts.begin(); itor != topLefts.end(); itor++ )
		{

			content.coord = *itor;

			if( Fits( content ) )
			{

				Use( content );
				topLefts.erase( itor );
				return true;
			}
		}
		////////////////////////////////


		return false;
	}

private:

	bool Fits( const Content<_T>& content ) const
	{
		if( ( content.coord.x + content.size.w ) > w )
		{
			return false;
		}

		if( ( content.coord.y + content.size.h ) > h )
		{
			return false;
		}

		for( typename Content<_T>::Vector::const_iterator itor = contentVector.begin(); itor != contentVector.end(); itor++ )
			if( content.intersects( *itor ) )
			{
				return false;
			}

		return true;
	}

	bool Use( const Content<_T>& content )
	{
		const Size&  size = content.size;
		const Coord& coord = content.coord;

		topLefts.push_front( Coord( coord.x + size.w, coord.y ) );
		topLefts.push_back( Coord( coord.x         , coord.y + size.h ) );

		contentVector.push_back( content );

		needToSort = true;

		return true;
	}

private:

	struct TopToBottomLeftToRightSort
	{

		bool operator()( const Coord& a, const Coord& b ) const
		{

			return ( a.x * a.x + a.y * a.y ) < ( b.x * b.x + b.y * b.y );
		}
	};

public:

	void Sort()
	{

		if( !needToSort )
		{
			return;
		}

		topLefts.sort( TopToBottomLeftToRightSort() );

		needToSort = false;
	}
};

template <typename _T> class ContentAccumulator
{
	typename Content<_T>::Vector contentVector;

public:

	ContentAccumulator()
	{}

	const typename Content<_T>::Vector& Get() const
	{

		return contentVector;
	}

	typename Content<_T>::Vector& Get()
	{

		return contentVector;
	}

	ContentAccumulator<_T>& operator += ( const Content<_T>& content )
	{

		contentVector.push_back( content );


		return *this;
	}

	ContentAccumulator<_T>& operator += ( const typename Content<_T>::Vector& content )
	{

		contentVector.insert( contentVector.end(), content.begin(), content.end() );


		return *this;
	}

	ContentAccumulator<_T> operator + ( const Content<_T>& content )
	{

		ContentAccumulator<_T> temp = *this;

		temp += content;

		return temp;
	}

	ContentAccumulator<_T> operator + ( const typename Content<_T>::Vector& content )
	{

		ContentAccumulator<_T> temp = *this;

		temp += content;

		return temp;
	}



private:

	struct GreatestWidthThenGreatestHeightSort
	{
		bool operator()( const Content<_T>& a, const Content<_T>& b ) const
		{

			const Size& sa = a.size;
			const Size& sb = b.size;

//      return( sa.w * sa.h > sb.w * sb.h );

			if( sa.w != sb.w )
			{
				return sa.w > sb.w;
			}
			return sa.h > sb.h;
		}
	};

	struct MakeHorizontal
	{

		Content<_T> operator()( const Content<_T>& elem )
		{

			if( elem.size.h > elem.size.w )
			{
				Content<_T> r = elem;

				r.size.w = elem.size.h;
				r.size.h = elem.size.w;
				r.rotated = !elem.rotated;

				return r;
			}

			return elem;
		}
	};

public:

	void Sort()
	{

//  if(allow_rotation)
//    std::transform(contentVector.begin(), contentVector.end(), contentVector.begin(), MakeHorizontal());

		std::sort( contentVector.begin(), contentVector.end(), GreatestWidthThenGreatestHeightSort() );
	}
};

template <typename _T> class UniformCanvasArrayBuilder
{
	int w;
	int h;
	int d;

public:

	UniformCanvasArrayBuilder( int w, int h, int d )
		: w( w ),
		  h( h ),
		  d( d )
	{}

	typename Canvas<_T>::Vector Build()
	{

		return typename Canvas<_T>::Vector( d, Canvas<_T>( w, h ) );
	}
};

template<typename _T> class CanvasArray
{
	typename Canvas<_T>::Vector canvasArray;

public:

	CanvasArray( const typename Canvas<_T>::Vector& canvasArray )
		: canvasArray( canvasArray )
	{}

	bool Place( const typename Content<_T>::Vector& contentVector, typename Content<_T>::Vector& remainder )
	{

		return Canvas<_T>::Place( canvasArray, contentVector, remainder );
	}

	bool Place( const ContentAccumulator<_T>& content, ContentAccumulator<_T>& remainder )
	{

		return Place( content.Get(), remainder.Get() );
	}

	bool Place( const typename Content<_T>::Vector& contentVector )
	{

		return Canvas<_T>::Place( canvasArray, contentVector );
	}

	bool Place( const ContentAccumulator<_T>& content )
	{

		return Place( content.Get() );
	}

	bool CollectContent( typename Content<_T>::Vector& contentVector ) const
	{

		int z = 0;

		for( typename Canvas<_T>::Vector::const_iterator itor = canvasArray.begin(); itor != canvasArray.end(); itor++ )
		{

			const typename Content<_T>::Vector& contents = itor->GetContents();

			for( typename Content<_T>::Vector::const_iterator itor = contents.begin(); itor != contents.end(); itor++ )
			{

				Content<_T> content = *itor;

				content.coord.z = z;

				contentVector.push_back( content );
			}
			z++;
		}
		return true;
	}

	bool CollectContent( ContentAccumulator<_T>& content ) const
	{

		return CollectContent( content.Get() );
	}
};

} /*** BinPack2D ***/

// Copyright (C) 2002-2005 Nikolaus Gebhardt
// Copyright (C) 2012 Robert Beckebans (id Tech 4 integration)
// This file is part of the "Irrlicht Engine" and the "irrXML" project.
// For conditions of distribution and use, see copyright notice in irrlicht.h and/or irrXML.h

#include "precompiled.h"
#pragma hdrstop

#include "irrXML.h"
#include "irrString.h"
#include "irrArray.h"
#include "fast_atof.h"
#include "CXMLReaderImpl.h"

namespace irr
{
namespace io
{

// RB: changed CFileReadCallback to use the idlib filesystem
class CFileReadCallBack : public IFileReadCallBack
{
public:

	//! construct from filename
	CFileReadCallBack(const char* filename)
		: _file(0), _size(0), _close(true)
	{
		// open file
		_file = idLib::fileSystem->OpenFileRead( filename );
		if( _file != NULL )
		{
			getFileSize();
		}
	}

	//! construct from FILE pointer
	CFileReadCallBack(idFile* file)
		: _file(file), _size(0), _close(false)
	{
		if( _file != NULL )
		{
			getFileSize();
		}
	}

	//! destructor
	virtual ~CFileReadCallBack()
	{
		if( _close && _file )
		{
			idLib::fileSystem->CloseFile( _file );
		}
	}

	//! Reads an amount of bytes from the file.
	virtual int read(void* buffer, int sizeToRead)
	{
		if( _file == NULL )
		{
			return 0;
		}

		return _file->Read( buffer, sizeToRead );
	}

	//! Returns size of file in bytes
	virtual int getSize()
	{
		return _size;
	}

	virtual ID_TIME_T getTimestamp() const
	{
		if( _file == NULL )
		{
			return FILE_NOT_FOUND_TIMESTAMP;
		}

		return _file->Timestamp();
	}

private:

	//! retrieves the file size of the open file
	void getFileSize()
	{
		_size = _file->Length();
	}

	idFile* _file;
	int _size;
	bool _close;
};
// RB end


// FACTORY FUNCTIONS:


//! Creates an instance of an UFT-8 or ASCII character xml parser. 
IrrXMLReader* createIrrXMLReader(const char* filename)
{
	return new CXMLReaderImpl<char, IXMLBase>(new CFileReadCallBack(filename)); 
}


//! Creates an instance of an UFT-8 or ASCII character xml parser. 
IrrXMLReader* createIrrXMLReader(idFile* file)
{
	return new CXMLReaderImpl<char, IXMLBase>(new CFileReadCallBack(file)); 
}


//! Creates an instance of an UFT-8 or ASCII character xml parser. 
IrrXMLReader* createIrrXMLReader(IFileReadCallBack* callback)
{
	return new CXMLReaderImpl<char, IXMLBase>(callback, false); 
}


//! Creates an instance of an UTF-16 xml parser. 
IrrXMLReaderUTF16* createIrrXMLReaderUTF16(const char* filename)
{
	return new CXMLReaderImpl<char16, IXMLBase>(new CFileReadCallBack(filename)); 
}


//! Creates an instance of an UTF-16 xml parser. 
IrrXMLReaderUTF16* createIrrXMLReaderUTF16(idFile* file)
{
	return new CXMLReaderImpl<char16, IXMLBase>(new CFileReadCallBack(file)); 
}


//! Creates an instance of an UTF-16 xml parser. 
IrrXMLReaderUTF16* createIrrXMLReaderUTF16(IFileReadCallBack* callback)
{
	return new CXMLReaderImpl<char16, IXMLBase>(callback, false); 
}


//! Creates an instance of an UTF-32 xml parser. 
IrrXMLReaderUTF32* createIrrXMLReaderUTF32(const char* filename)
{
	return new CXMLReaderImpl<char32, IXMLBase>(new CFileReadCallBack(filename)); 
}


//! Creates an instance of an UTF-32 xml parser. 
IrrXMLReaderUTF32* createIrrXMLReaderUTF32(idFile* file)
{
	return new CXMLReaderImpl<char32, IXMLBase>(new CFileReadCallBack(file)); 
}


//! Creates an instance of an UTF-32 xml parser. 
IrrXMLReaderUTF32* createIrrXMLReaderUTF32(IFileReadCallBack* callback)
{
	return new CXMLReaderImpl<char32, IXMLBase>(callback, false); 
}


} // end namespace io
} // end namespace irr

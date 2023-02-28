// The MIT License(MIT)
//
// Copyright(c) 2019 Vadim Slyusarev
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "optick_serialization.h"

#if USE_OPTICK

#include "optick_common.h"

namespace Optick
{
	string OutputDataStream::GetData()
	{
		flush();
		return str();
	}

	OutputDataStream & OutputDataStream::Write(const char * buffer, size_t size)
	{
		write(buffer, size);
		return *this;
	}

	OutputDataStream &operator << ( OutputDataStream &stream, const char* val )
	{
		uint32 length = val == nullptr ? 0 : (uint32)strlen(val);
		stream << length;

		if (length > 0)
		{
			stream.write( val, length );
		}
		return stream;
	}

	OutputDataStream &operator << ( OutputDataStream &stream, int val )
	{
		stream.write( (char*)&val, sizeof(int) );
		return stream;
	}

	OutputDataStream &operator << ( OutputDataStream &stream, int64 val )
	{
		stream.write( (char*)&val, sizeof(int64) );
		return stream;
	}

	OutputDataStream &operator << ( OutputDataStream &stream, char val )
	{
		stream.write( (char*)&val, sizeof(char) );
		return stream;
	}

	OutputDataStream &operator << (OutputDataStream &stream, int8 val)
	{
		stream.write((char*)&val, sizeof(val));
		return stream;
	}

	OutputDataStream &operator << ( OutputDataStream &stream, byte val )
	{
		stream.write( (char*)&val, sizeof(byte) );
		return stream;
	}
	
	OutputDataStream & operator<<(OutputDataStream &stream, uint64 val)
	{
		stream.write( (char*)&val, sizeof(uint64) );
		return stream;
	}

	OutputDataStream & operator<<(OutputDataStream &stream, uint32 val)
	{
		stream.write( (char*)&val, sizeof(uint32) );
		return stream;
	}

	OutputDataStream & operator<<(OutputDataStream &stream, float val)
	{
		stream.write((char*)&val, sizeof(float));
		return stream;
	}

	OutputDataStream & operator<<(OutputDataStream &stream, const string& val)
	{
		stream << (uint32)val.length();
		if (!val.empty())
			stream.write(&val[0], sizeof(val[0]) * val.length());
		return stream;
	}

	OutputDataStream & operator<<(OutputDataStream &stream, const wstring& val)
	{
		size_t count = val.length() * sizeof(wchar_t);
		stream << (uint32)count;
		if (!val.empty())
			stream.write((char*)(&val[0]), count);
		return stream;
	}

	InputDataStream &operator >> (InputDataStream &stream, int16 &val)
	{
		stream.read((char*)&val, sizeof(int16));
		return stream;
	}

	InputDataStream &operator >> ( InputDataStream &stream, int32 &val )
	{
		stream.read( (char*)&val, sizeof(int) );
		return stream;
	}

	InputDataStream &operator >> ( InputDataStream &stream, int64 &val )
	{
		stream.read( (char*)&val, sizeof(int64) );
		return stream;
	}

	InputDataStream & operator>>( InputDataStream &stream, byte &val )
	{
		stream.read( (char*)&val, sizeof(byte) );
		return stream;
	}

	InputDataStream & operator >> (InputDataStream &stream, uint16 &val)
	{
		stream.read((char*)&val, sizeof(uint16));
		return stream;
	}

	InputDataStream & operator>>( InputDataStream &stream, uint32 &val )
	{
		stream.read( (char*)&val, sizeof(uint32) );
		return stream;
	}

	InputDataStream & operator>>( InputDataStream &stream, uint64 &val )
	{
		stream.read( (char*)&val, sizeof(uint64) );
		return stream;
	}

	InputDataStream & operator >> ( InputDataStream &stream, string &val)
	{
		int32 length = 0;
		stream >> length;
		val.resize(length + 1);
		stream.read( (char*)&val[0], length);
		return stream;
	}

	InputDataStream::InputDataStream() :
		stringstream( ios_base::in | ios_base::out )
	{
	}

	void InputDataStream::Append(const char *buffer, size_t length)
	{
		write( buffer, length );
	}

	size_t InputDataStream::Length()
	{
		return (size_t)(tellp() - tellg());
	}

	bool InputDataStream::Skip(size_t length)
	{
		bool result = Length() <= length;
		seekg(length, ios_base::cur);
		return result;
	}



}

#endif //USE_OPTICK
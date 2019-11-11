#ifndef __WIN_NANOAFX_HPP__
#define __WIN_NANOAFX_HPP__

#define	_T(n)	L##n

class CComBSTR : public _bstr_t
{
public:
	inline CComBSTR( const wchar_t* str ) : _bstr_t( str )
	{
	}

	inline CComBSTR( const char* str ) : _bstr_t( str )
	{
	}
};

class CComVariant : public tagVARIANT
{
};

template<class T>
class CComPtr
{
private:
	T* _ptr;

public:
	inline CComPtr()
	{
		_ptr = NULL;
	}

	inline CComPtr( T* ptr )
	{
		if( ptr )
		{
			ptr->AddRef();
			_ptr = ptr;
		}
	}

	inline ~CComPtr()
	{
		if( _ptr )
		{
			_ptr->Release();
		}
		_ptr = NULL;
	}

	inline CComPtr& operator = ( T* ptr )
	{
		if( ptr )
		{
			ptr->AddRef();
			_ptr = ptr;
		}
	}

	inline bool operator == ( T* ptr )
	{
		return _ptr == ptr;
	}

	inline T* operator -> ()
	{
		return _ptr;
	}

	inline T** operator & ()
	{
		return &_ptr;
	}

	inline operator T* ()
	{
		return _ptr;
	}
};

#endif

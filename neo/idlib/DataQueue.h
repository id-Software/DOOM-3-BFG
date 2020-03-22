#ifndef DATAQUEUE_H
#define DATAQUEUE_H

template< int maxItems, int maxBuffer >
class idDataQueue
{
public:
	idDataQueue()
	{
		dataLength = 0;
	}
	bool Append( int sequence, const byte* b1, int b1Len, const byte* b2 = NULL, int b2Len = 0 );
	void RemoveOlderThan( int sequence );

	int GetDataLength() const
	{
		return dataLength;
	}

	int Num() const
	{
		return items.Num();
	}
	int ItemSequence( int i ) const
	{
		return items[i].sequence;
	}
	int ItemLength( int i ) const
	{
		return items[i].length;
	}
	const byte* ItemData( int i ) const
	{
		return &data[items[i].dataOffset];
	}

	void Clear()
	{
		dataLength = 0;
		items.Clear();
		memset( data, 0, sizeof( data ) );
	}

private:
	struct msgItem_t
	{
		int		sequence;
		int		length;
		int		dataOffset;
	};
	idStaticList<msgItem_t, maxItems > items;
	int		dataLength;
	byte	data[ maxBuffer ];
};

/*
========================
idDataQueue::RemoveOlderThan
========================
*/
template< int maxItems, int maxBuffer >
void idDataQueue< maxItems, maxBuffer >::RemoveOlderThan( int sequence )
{
	int length = 0;
	while( items.Num() > 0 && items[0].sequence < sequence )
	{
		length += items[0].length;
		items.RemoveIndex( 0 );
	}
	if( length >= dataLength )
	{
		assert( items.Num() == 0 );
		assert( dataLength == length );
		dataLength = 0;
	}
	else if( length > 0 )
	{
		memmove( data, data + length, dataLength - length );
		dataLength -= length;
	}
	length = 0;
	for( int i = 0; i < items.Num(); i++ )
	{
		items[i].dataOffset = length;
		length += items[i].length;
	}
	assert( length == dataLength );
}

/*
========================
idDataQueue::Append
========================
*/
template< int maxItems, int maxBuffer >
bool idDataQueue< maxItems, maxBuffer >::Append( int sequence, const byte* b1, int b1Len, const byte* b2, int b2Len )
{
	if( items.Num() == items.Max() )
	{
		return false;
	}
	if( dataLength + b1Len + b2Len >= maxBuffer )
	{
		return false;
	}
	msgItem_t& item = *items.Alloc();
	item.length = b1Len + b2Len;
	item.sequence = sequence;
	item.dataOffset = dataLength;
	memcpy( data + dataLength, b1, b1Len );
	dataLength += b1Len;
	memcpy( data + dataLength, b2, b2Len );
	dataLength += b2Len;
	return true;
}

#endif // DATAQUEUE_H

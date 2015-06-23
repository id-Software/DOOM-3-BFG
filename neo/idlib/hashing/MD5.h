#ifndef __MD5_H__
#define __MD5_H__

/*
===============================================================================

	Calculates a checksum for a block of data
	using the MD5 message-digest algorithm.

===============================================================================
*/
struct MD5_CTX
{
	unsigned int	state[4];
	unsigned int	bits[2];
	unsigned char	in[64];
};

void MD5_Init( MD5_CTX* ctx );
void MD5_Update( MD5_CTX* context, unsigned char const* input, size_t inputLen );
void MD5_Final( MD5_CTX* context, unsigned char digest[16] );

unsigned int MD5_BlockChecksum( const void* data, size_t length );

#endif /* !__MD5_H__ */

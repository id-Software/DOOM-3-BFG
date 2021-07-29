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

#include "precompiled.h"
#pragma hdrstop


#include "Model_lwo.h"

/*
======================================================================

	Converted from lwobject sample prog from LW 6.5 SDK.

======================================================================
*/

/*
======================================================================
lwFreeClip()

Free memory used by an lwClip.
====================================================================== */

void lwFreeClip( lwClip* clip )
{
	if( clip )
	{
		lwListFree( clip->ifilter, ( void ( __cdecl* )( void* ) )lwFreePlugin );
		lwListFree( clip->pfilter, ( void ( __cdecl* )( void* ) )lwFreePlugin );
		switch( clip->type )
		{
			case ID_STIL:
			{
				if( clip->source.still.name )
				{
					Mem_Free( clip->source.still.name );
				}
				break;
			}
			case ID_ISEQ:
			{
				if( clip->source.seq.suffix )
				{
					Mem_Free( clip->source.seq.suffix );
				}
				if( clip->source.seq.prefix )
				{
					Mem_Free( clip->source.seq.prefix );
				}
				break;
			}
			case ID_ANIM:
			{
				if( clip->source.anim.server )
				{
					Mem_Free( clip->source.anim.server );
				}
				if( clip->source.anim.name )
				{
					Mem_Free( clip->source.anim.name );
				}
				break;
			}
			case ID_XREF:
			{
				if( clip->source.xref.string )
				{
					Mem_Free( clip->source.xref.string );
				}
				break;
			}
			case ID_STCC:
			{
				if( clip->source.cycle.name )
				{
					Mem_Free( clip->source.cycle.name );
				}
				break;
			}
		}
		Mem_Free( clip );
	}
}


/*
======================================================================
lwGetClip()

Read image references from a CLIP chunk in an LWO2 file.
====================================================================== */

lwClip* lwGetClip( idFile* fp, int cksize )
{
	lwClip* clip;
	lwPlugin* filt;
	unsigned int id;
	unsigned short sz;
	int pos, rlen;


	/* allocate the Clip structure */

	clip = ( lwClip* )Mem_ClearedAlloc( sizeof( lwClip ), TAG_MODEL );
	if( !clip )
	{
		goto Fail;
	}

	clip->contrast.val = 1.0f;
	clip->brightness.val = 1.0f;
	clip->saturation.val = 1.0f;
	clip->gamma.val = 1.0f;

	/* remember where we started */

	set_flen( 0 );
	pos = fp->Tell();

	/* index */

	clip->index = getI4( fp );

	/* first subchunk header */

	clip->type = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	sz += sz & 1;
	set_flen( 0 );

	switch( clip->type )
	{
		case ID_STIL:
			clip->source.still.name = getS0( fp );
			break;

		case ID_ISEQ:
			clip->source.seq.digits  = getU1( fp );
			clip->source.seq.flags   = getU1( fp );
			clip->source.seq.offset  = getI2( fp );
			clip->source.seq.start   = getI2( fp );
			clip->source.seq.end     = getI2( fp );
			clip->source.seq.prefix  = getS0( fp );
			clip->source.seq.suffix  = getS0( fp );
			break;

		case ID_ANIM:
			clip->source.anim.name   = getS0( fp );
			clip->source.anim.server = getS0( fp );
			rlen = get_flen();
			clip->source.anim.data   = getbytes( fp, sz - rlen );
			break;

		case ID_XREF:
			clip->source.xref.index  = getI4( fp );
			clip->source.xref.string = getS0( fp );
			break;

		case ID_STCC:
			clip->source.cycle.lo   = getI2( fp );
			clip->source.cycle.hi   = getI2( fp );
			clip->source.cycle.name = getS0( fp );
			break;

		default:
			break;
	}

	/* error while reading current subchunk? */

	rlen = get_flen();
	if( rlen < 0 || rlen > sz )
	{
		goto Fail;
	}

	/* skip unread parts of the current subchunk */

	if( rlen < sz )
	{
		fp->Seek( sz - rlen, FS_SEEK_CUR );
	}

	/* end of the CLIP chunk? */

	rlen = fp->Tell() - pos;
	if( cksize < rlen )
	{
		goto Fail;
	}
	if( cksize == rlen )
	{
		return clip;
	}

	/* process subchunks as they're encountered */

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_TIME:
				clip->start_time = getF4( fp );
				clip->duration = getF4( fp );
				clip->frame_rate = getF4( fp );
				break;

			case ID_CONT:
				clip->contrast.val = getF4( fp );
				clip->contrast.eindex = getVX( fp );
				break;

			case ID_BRIT:
				clip->brightness.val = getF4( fp );
				clip->brightness.eindex = getVX( fp );
				break;

			case ID_SATR:
				clip->saturation.val = getF4( fp );
				clip->saturation.eindex = getVX( fp );
				break;

			case ID_HUE:
				clip->hue.val = getF4( fp );
				clip->hue.eindex = getVX( fp );
				break;

			case ID_GAMM:
				clip->gamma.val = getF4( fp );
				clip->gamma.eindex = getVX( fp );
				break;

			case ID_NEGA:
				clip->negative = getU2( fp );
				break;

			case ID_IFLT:
			case ID_PFLT:
				filt = ( lwPlugin* )Mem_ClearedAlloc( sizeof( lwPlugin ), TAG_MODEL );
				if( !filt )
				{
					goto Fail;
				}

				filt->name = getS0( fp );
				filt->flags = getU2( fp );
				rlen = get_flen();
				filt->data = getbytes( fp, sz - rlen );

				if( id == ID_IFLT )
				{
					lwListAdd( ( void** )&clip->ifilter, filt );
					clip->nifilters++;
				}
				else
				{
					lwListAdd( ( void** )&clip->pfilter, filt );
					clip->npfilters++;
				}
				break;

			default:
				break;
		}

		/* error while reading current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			goto Fail;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the CLIP chunk? */

		rlen = fp->Tell() - pos;
		if( cksize < rlen )
		{
			goto Fail;
		}
		if( cksize == rlen )
		{
			break;
		}

		/* get the next chunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			goto Fail;
		}
	}

	return clip;

Fail:
	lwFreeClip( clip );
	return NULL;
}


/*
======================================================================
lwFindClip()

Returns an lwClip pointer, given a clip index.
====================================================================== */

lwClip* lwFindClip( lwClip* list, int index )
{
	lwClip* clip;

	clip = list;
	while( clip )
	{
		if( clip->index == index )
		{
			break;
		}
		clip = clip->next;
	}
	return clip;
}


/*
======================================================================
lwFreeEnvelope()

Free the memory used by an lwEnvelope.
====================================================================== */

void lwFree( void* ptr )
{
	Mem_Free( ptr );
}

void lwFreeEnvelope( lwEnvelope* env )
{
	if( env )
	{
		if( env->name )
		{
			Mem_Free( env->name );
		}
		lwListFree( env->key, lwFree );
		lwListFree( env->cfilter, ( void ( __cdecl* )( void* ) )lwFreePlugin );
		Mem_Free( env );
	}
}


static int compare_keys( lwKey* k1, lwKey* k2 )
{
	return k1->time > k2->time ? 1 : k1->time < k2->time ? -1 : 0;
}


/*
======================================================================
lwGetEnvelope()

Read an ENVL chunk from an LWO2 file.
====================================================================== */

lwEnvelope* lwGetEnvelope( idFile* fp, int cksize )
{
	lwEnvelope* env = NULL;
	lwKey* key = NULL;
	lwPlugin* plug = NULL;
	unsigned int id;
	unsigned short sz;
	float f[ 4 ];
	int i, nparams, pos, rlen;


	/* allocate the Envelope structure */

	env = ( lwEnvelope* )Mem_ClearedAlloc( sizeof( lwEnvelope ), TAG_MODEL );
	if( !env )
	{
		goto Fail;
	}

	/* remember where we started */

	set_flen( 0 );
	pos = fp->Tell();

	/* index */

	env->index = getVX( fp );

	/* first subchunk header */

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	/* process subchunks as they're encountered */

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_TYPE:
				env->type = getU2( fp );
				break;

			case ID_NAME:
				env->name = getS0( fp );
				break;

			case ID_PRE:
				env->behavior[ 0 ] = getU2( fp );
				break;

			case ID_POST:
				env->behavior[ 1 ] = getU2( fp );
				break;

			case ID_KEY:
				key = ( lwKey* )Mem_ClearedAlloc( sizeof( lwKey ), TAG_MODEL );
				if( !key )
				{
					goto Fail;
				}
				key->time = getF4( fp );
				key->value = getF4( fp );
				lwListInsert( ( void** )&env->key, key, ( int ( __cdecl* )( void*, void* ) )compare_keys );
				env->nkeys++;
				break;

			case ID_SPAN:
				if( key == NULL )
				{
					goto Fail;
				}
				key->shape = getU4( fp );

				nparams = ( sz - 4 ) / 4;
				if( nparams > 4 )
				{
					nparams = 4;
				}
				for( i = 0; i < nparams; i++ )
				{
					f[ i ] = getF4( fp );
				}

				switch( key->shape )
				{
					case ID_TCB:
						key->tension = f[ 0 ];
						key->continuity = f[ 1 ];
						key->bias = f[ 2 ];
						break;

					case ID_BEZI:
					case ID_HERM:
					case ID_BEZ2:
						for( i = 0; i < nparams; i++ )
						{
							key->param[ i ] = f[ i ];
						}
						break;
				}
				break;

			case ID_CHAN:
				plug = ( lwPlugin* )Mem_ClearedAlloc( sizeof( lwPlugin ), TAG_MODEL );
				if( !plug )
				{
					goto Fail;
				}

				plug->name = getS0( fp );
				plug->flags = getU2( fp );
				plug->data = getbytes( fp, sz - get_flen() );

				lwListAdd( ( void** )&env->cfilter, plug );
				env->ncfilters++;
				break;

			default:
				break;
		}

		/* error while reading current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			goto Fail;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the ENVL chunk? */

		rlen = fp->Tell() - pos;
		if( cksize < rlen )
		{
			goto Fail;
		}
		if( cksize == rlen )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			goto Fail;
		}
	}

	return env;

Fail:
	lwFreeEnvelope( env );
	return NULL;
}


/*
======================================================================
lwFindEnvelope()

Returns an lwEnvelope pointer, given an envelope index.
====================================================================== */

lwEnvelope* lwFindEnvelope( lwEnvelope* list, int index )
{
	lwEnvelope* env;

	env = list;
	while( env )
	{
		if( env->index == index )
		{
			break;
		}
		env = env->next;
	}
	return env;
}


/*
======================================================================
range()

Given the value v of a periodic function, returns the equivalent value
v2 in the principal interval [lo, hi].  If i isn't NULL, it receives
the number of wavelengths between v and v2.

   v2 = v - i * (hi - lo)

For example, range( 3 pi, 0, 2 pi, i ) returns pi, with i = 1.
====================================================================== */

static float range( float v, float lo, float hi, int* i )
{
	float v2, r = hi - lo;

	if( r == 0.0 )
	{
		if( i )
		{
			*i = 0;
		}
		return lo;
	}

	v2 = lo + v - r * ( float ) floor( ( double ) v / r );
	if( i )
	{
		*i = -( int )( ( v2 - v ) / r + ( v2 > v ? 0.5 : -0.5 ) );
	}

	return v2;
}


/*
======================================================================
hermite()

Calculate the Hermite coefficients.
====================================================================== */

static void hermite( float t, float* h1, float* h2, float* h3, float* h4 )
{
	float t2, t3;

	t2 = t * t;
	t3 = t * t2;

	*h2 = 3.0f * t2 - t3 - t3;
	*h1 = 1.0f - *h2;
	*h4 = t3 - t2;
	*h3 = *h4 - t2 + t;
}


/*
======================================================================
bezier()

Interpolate the value of a 1D Bezier curve.
====================================================================== */

static float bezier( float x0, float x1, float x2, float x3, float t )
{
	float a, b, c, t2, t3;

	t2 = t * t;
	t3 = t2 * t;

	c = 3.0f * ( x1 - x0 );
	b = 3.0f * ( x2 - x1 ) - c;
	a = x3 - x0 - c - b;

	return a * t3 + b * t2 + c * t + x0;
}


/*
======================================================================
bez2_time()

Find the t for which bezier() returns the input time.  The handle
endpoints of a BEZ2 curve represent the control points, and these have
(time, value) coordinates, so time is used as both a coordinate and a
parameter for this curve type.
====================================================================== */

static float bez2_time( float x0, float x1, float x2, float x3, float time,
						float* t0, float* t1 )
{
	float v, t;

	t = *t0 + ( *t1 - *t0 ) * 0.5f;
	v = bezier( x0, x1, x2, x3, t );
	if( idMath::Fabs( time - v ) > .0001f )
	{
		if( v > time )
		{
			*t1 = t;
		}
		else
		{
			*t0 = t;
		}
		return bez2_time( x0, x1, x2, x3, time, t0, t1 );
	}
	else
	{
		return t;
	}
}


/*
======================================================================
bez2()

Interpolate the value of a BEZ2 curve.
====================================================================== */

static float bez2( lwKey* key0, lwKey* key1, float time )
{
	float x, y, t, t0 = 0.0f, t1 = 1.0f;

	if( key0->shape == ID_BEZ2 )
	{
		x = key0->time + key0->param[ 2 ];
	}
	else
	{
		x = key0->time + ( key1->time - key0->time ) / 3.0f;
	}

	t = bez2_time( key0->time, x, key1->time + key1->param[ 0 ], key1->time,
				   time, &t0, &t1 );

	if( key0->shape == ID_BEZ2 )
	{
		y = key0->value + key0->param[ 3 ];
	}
	else
	{
		y = key0->value + key0->param[ 1 ] / 3.0f;
	}

	return bezier( key0->value, y, key1->param[ 1 ] + key1->value, key1->value, t );
}


/*
======================================================================
outgoing()

Return the outgoing tangent to the curve at key0.  The value returned
for the BEZ2 case is used when extrapolating a linear pre behavior and
when interpolating a non-BEZ2 span.
====================================================================== */

static float outgoing( lwKey* key0, lwKey* key1 )
{
	float a, b, d, t, out;

	switch( key0->shape )
	{
		case ID_TCB:
			a = ( 1.0f - key0->tension )
				* ( 1.0f + key0->continuity )
				* ( 1.0f + key0->bias );
			b = ( 1.0f - key0->tension )
				* ( 1.0f - key0->continuity )
				* ( 1.0f - key0->bias );
			d = key1->value - key0->value;

			if( key0->prev )
			{
				t = ( key1->time - key0->time ) / ( key1->time - key0->prev->time );
				out = t * ( a * ( key0->value - key0->prev->value ) + b * d );
			}
			else
			{
				out = b * d;
			}
			break;

		case ID_LINE:
			d = key1->value - key0->value;
			if( key0->prev )
			{
				t = ( key1->time - key0->time ) / ( key1->time - key0->prev->time );
				out = t * ( key0->value - key0->prev->value + d );
			}
			else
			{
				out = d;
			}
			break;

		case ID_BEZI:
		case ID_HERM:
			out = key0->param[ 1 ];
			if( key0->prev )
			{
				out *= ( key1->time - key0->time ) / ( key1->time - key0->prev->time );
			}
			break;

		case ID_BEZ2:
			out = key0->param[ 3 ] * ( key1->time - key0->time );
			if( idMath::Fabs( key0->param[ 2 ] ) > 1e-5f )
			{
				out /= key0->param[ 2 ];
			}
			else
			{
				out *= 1e5f;
			}
			break;

		case ID_STEP:
		default:
			out = 0.0f;
			break;
	}

	return out;
}


/*
======================================================================
incoming()

Return the incoming tangent to the curve at key1.  The value returned
for the BEZ2 case is used when extrapolating a linear post behavior.
====================================================================== */

static float incoming( lwKey* key0, lwKey* key1 )
{
	float a, b, d, t, in;

	switch( key1->shape )
	{
		case ID_LINE:
			d = key1->value - key0->value;
			if( key1->next )
			{
				t = ( key1->time - key0->time ) / ( key1->next->time - key0->time );
				in = t * ( key1->next->value - key1->value + d );
			}
			else
			{
				in = d;
			}
			break;

		case ID_TCB:
			a = ( 1.0f - key1->tension )
				* ( 1.0f - key1->continuity )
				* ( 1.0f + key1->bias );
			b = ( 1.0f - key1->tension )
				* ( 1.0f + key1->continuity )
				* ( 1.0f - key1->bias );
			d = key1->value - key0->value;

			if( key1->next )
			{
				t = ( key1->time - key0->time ) / ( key1->next->time - key0->time );
				in = t * ( b * ( key1->next->value - key1->value ) + a * d );
			}
			else
			{
				in = a * d;
			}
			break;

		case ID_BEZI:
		case ID_HERM:
			in = key1->param[ 0 ];
			if( key1->next )
			{
				in *= ( key1->time - key0->time ) / ( key1->next->time - key0->time );
			}
			break;

		case ID_BEZ2:
			in = key1->param[ 1 ] * ( key1->time - key0->time );
			if( idMath::Fabs( key1->param[ 0 ] ) > 1e-5f )
			{
				in /= key1->param[ 0 ];
			}
			else
			{
				in *= 1e5f;
			}
			break;

		case ID_STEP:
		default:
			in = 0.0f;
			break;
	}

	return in;
}


/*
======================================================================
evalEnvelope()

Given a list of keys and a time, returns the interpolated value of the
envelope at that time.
====================================================================== */

float evalEnvelope( lwEnvelope* env, float time )
{
	lwKey* key0, *key1, *skey, *ekey;
	float t, h1, h2, h3, h4, in, out, offset = 0.0f;
	int noff;

	// Start key
	skey = ekey = env->key;

	/* if there's no key, the value is 0 */
	if( env->nkeys == 0 || skey == NULL )
	{
		return 0.0f;
	}

	/* if there's only one key, the value is constant */
	if( env->nkeys == 1 )
	{
		return env->key->value;
	}

	/* find the last keys */
	while( ekey->next != NULL )
	{
		ekey = ekey->next;
	}

	/* use pre-behavior if time is before first key time */
	if( time < skey->time )
	{
		switch( env->behavior[ 0 ] )
		{
			case BEH_RESET:
				return 0.0f;

			case BEH_CONSTANT:
				return skey->value;

			case BEH_REPEAT:
				time = range( time, skey->time, ekey->time, NULL );
				break;

			case BEH_OSCILLATE:
				time = range( time, skey->time, ekey->time, &noff );
				if( noff % 2 )
				{
					time = ekey->time - skey->time - time;
				}
				break;

			case BEH_OFFSET:
				time = range( time, skey->time, ekey->time, &noff );
				offset = noff * ( ekey->value - skey->value );
				break;

			case BEH_LINEAR:
				if( skey->next != NULL )
				{
					out = outgoing( skey, skey->next )
						  / ( skey->next->time - skey->time );
					return out * ( time - skey->time ) + skey->value;
				}
				else
				{
					return 0.0f;
				}
		}
	}

	/* use post-behavior if time is after last key time */

	else if( time > ekey->time )
	{
		switch( env->behavior[ 1 ] )
		{
			case BEH_RESET:
				return 0.0f;

			case BEH_CONSTANT:
				return ekey->value;

			case BEH_REPEAT:
				time = range( time, skey->time, ekey->time, NULL );
				break;

			case BEH_OSCILLATE:
				time = range( time, skey->time, ekey->time, &noff );
				if( noff % 2 )
				{
					time = ekey->time - skey->time - time;
				}
				break;

			case BEH_OFFSET:
				time = range( time, skey->time, ekey->time, &noff );
				offset = noff * ( ekey->value - skey->value );
				break;

			case BEH_LINEAR:
				in = incoming( ekey->prev, ekey )
					 / ( ekey->time - ekey->prev->time );
				return in * ( time - ekey->time ) + ekey->value;
		}
	}

	/* get the endpoints of the interval being evaluated */

	key0 = env->key;
	if( key0 == NULL || key0->next == NULL )
	{
		return 0.0f;
	}
	while( time > key0->next->time )
	{
		key0 = key0->next;
	}
	key1 = key0->next;
	if( key1 == NULL )
	{
		return 0.0f;
	}

	/* check for singularities first */

	if( time == key0->time )
	{
		return key0->value + offset;
	}
	else if( time == key1->time )
	{
		return key1->value + offset;
	}

	/* get interval length, time in [0, 1] */

	t = ( time - key0->time ) / ( key1->time - key0->time );

	/* interpolate */

	switch( key1->shape )
	{
		case ID_TCB:
		case ID_BEZI:
		case ID_HERM:
			out = outgoing( key0, key1 );
			in = incoming( key0, key1 );
			hermite( t, &h1, &h2, &h3, &h4 );
			return h1 * key0->value + h2 * key1->value + h3 * out + h4 * in + offset;

		case ID_BEZ2:
			return bez2( key0, key1, time ) + offset;

		case ID_LINE:
			return key0->value + t * ( key1->value - key0->value ) + offset;

		case ID_STEP:
			return key0->value + offset;

		default:
			return offset;
	}
}



/*
======================================================================
lwListFree()

Free the items in a list.
====================================================================== */

void lwListFree( void* list, void ( *freeNode )( void* ) )
{
	lwNode* node, *next;

	node = ( lwNode* ) list;
	while( node )
	{
		next = node->next;
		freeNode( node );
		node = next;
	}
}


/*
======================================================================
lwListAdd()

Append a node to a list.
====================================================================== */

void lwListAdd( void** list, void* node )
{
	lwNode* head = NULL, *tail = NULL;

	head = *( ( lwNode** ) list );
	if( head == NULL )
	{
		*list = node;
		return;
	}
	while( head )
	{
		tail = head;
		head = head->next;
	}
	tail->next = ( lwNode* ) node;
	( ( lwNode* ) node )->prev = tail;
}


/*
======================================================================
lwListInsert()

Insert a node into a list in sorted order.
====================================================================== */

void lwListInsert( void** vlist, void* vitem, int ( *compare )( void*, void* ) )
{
	lwNode** list, *item, *node, *prev;

	if( !*vlist )
	{
		*vlist = vitem;
		return;
	}

	list = ( lwNode** ) vlist;
	item = ( lwNode* ) vitem;
	node = *list;
	prev = NULL;

	while( node )
	{
		if( 0 < compare( node, item ) )
		{
			break;
		}
		prev = node;
		node = node->next;
	}

	if( !prev )
	{
		*list = item;
		node->prev = item;
		item->next = node;
	}
	else if( !node )
	{
		prev->next = item;
		item->prev = prev;
	}
	else
	{
		item->next = node;
		item->prev = prev;
		prev->next = item;
		node->prev = item;
	}
}

/*
======================================================================
flen

This accumulates a count of the number of bytes read.  Callers can set
it at the beginning of a sequence of reads and then retrieve it to get
the number of bytes actually read.  If one of the I/O functions fails,
flen is set to an error code, after which the I/O functions ignore
read requests until flen is reset.
====================================================================== */

#define FLEN_ERROR -9999

static int flen;

void set_flen( int i )
{
	flen = i;
}

int get_flen()
{
	return flen;
}

void* getbytes( idFile* fp, int size )
{
	void* data;

	if( flen == FLEN_ERROR )
	{
		return NULL;
	}
	if( size < 0 )
	{
		flen = FLEN_ERROR;
		return NULL;
	}
	data = Mem_ClearedAlloc( size, TAG_MODEL );
	if( !data )
	{
		flen = FLEN_ERROR;
		return NULL;
	}
	if( size != fp->Read( data, size ) )
	{
		flen = FLEN_ERROR;
		Mem_Free( data );
		return NULL;
	}

	flen += size;
	return data;
}


void skipbytes( idFile* fp, int n )
{
	if( flen == FLEN_ERROR )
	{
		return;
	}
	if( fp->Seek( n, FS_SEEK_CUR ) )
	{
		flen = FLEN_ERROR;
	}
	else
	{
		flen += n;
	}
}


int getI1( idFile* fp )
{
	int i, c;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	c = 0;
	i = fp->Read( &c, 1 );
	if( i < 0 )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	if( c > 127 )
	{
		c -= 256;
	}
	flen += 1;
	return c;
}


short getI2( idFile* fp )
{
	short i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	if( 2 != fp->Read( &i, 2 ) )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	BigRevBytes( &i, 2, 1 );
	flen += 2;
	return i;
}


int getI4( idFile* fp )
{
	int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	if( 4 != fp->Read( &i, 4 ) )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	BigRevBytes( &i, 4, 1 );
	flen += 4;
	return i;
}


unsigned char getU1( idFile* fp )
{
	int i, c;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	c = 0;
	i = fp->Read( &c, 1 );
	if( i < 0 )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	flen += 1;
	return c;
}


unsigned short getU2( idFile* fp )
{
	unsigned short i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	if( 2 != fp->Read( &i, 2 ) )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	BigRevBytes( &i, 2, 1 );
	flen += 2;
	return i;
}


unsigned int getU4( idFile* fp )
{
	unsigned int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	if( 4 != fp->Read( &i, 4 ) )
	{
		flen = FLEN_ERROR;
		return 0;
	}
	BigRevBytes( &i, 4, 1 );
	flen += 4;
	return i;
}


int getVX( idFile* fp )
{
	byte c;
	int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}

	c = 0;
	if( fp->Read( &c, 1 ) == -1 )
	{
		return 0;
	}

	if( c != 0xFF )
	{
		i = c << 8;
		c = 0;
		if( fp->Read( &c, 1 ) == -1 )
		{
			return 0;
		}
		i |= c;
		flen += 2;
	}
	else
	{
		c = 0;
		if( fp->Read( &c, 1 ) == -1 )
		{
			return 0;
		}
		i = c << 16;
		c = 0;
		if( fp->Read( &c, 1 ) == -1 )
		{
			return 0;
		}
		i |= c << 8;
		c = 0;
		if( fp->Read( &c, 1 ) == -1 )
		{
			return 0;
		}
		i |= c;
		flen += 4;
	}

	return i;
}


float getF4( idFile* fp )
{
	float f;

	if( flen == FLEN_ERROR )
	{
		return 0.0f;
	}
	if( 4 != fp->Read( &f, 4 ) )
	{
		flen = FLEN_ERROR;
		return 0.0f;
	}
	BigRevBytes( &f, 4, 1 );
	flen += 4;

	if( IEEE_FLT_IS_DENORMAL( f ) )
	{
		f = 0.0f;
	}
	return f;
}


char* getS0( idFile* fp )
{
	char* s;
	int i, c, len, pos;

	if( flen == FLEN_ERROR )
	{
		return NULL;
	}

	pos = fp->Tell();
	for( i = 1; ; i++ )
	{
		c = 0;
		if( fp->Read( &c, 1 ) == -1 )
		{
			flen = FLEN_ERROR;
			return NULL;
		}
		if( c == 0 )
		{
			break;
		}
	}

	if( i == 1 )
	{
		if( fp->Seek( pos + 2, FS_SEEK_SET ) )
		{
			flen = FLEN_ERROR;
		}
		else
		{
			flen += 2;
		}
		return NULL;
	}

	len = i + ( i & 1 );
	s = ( char* )Mem_ClearedAlloc( len, TAG_MODEL );
	if( !s )
	{
		flen = FLEN_ERROR;
		return NULL;
	}

	if( fp->Seek( pos, FS_SEEK_SET ) )
	{
		flen = FLEN_ERROR;
		return NULL;
	}
	if( len != fp->Read( s, len ) )
	{
		flen = FLEN_ERROR;
		return NULL;
	}

	flen += len;
	return s;
}


int sgetI1( unsigned char** bp )
{
	assert( bp != NULL && *bp != NULL ); // remove compiler warning
	int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	i = **bp;
	if( i > 127 )
	{
		i -= 256;
	}
	flen += 1;
	( *bp )++;
	return i;
}


short sgetI2( unsigned char** bp )
{
	assert( bp != NULL && *bp != NULL ); // remove compiler warning
	short i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	memcpy( &i, *bp, 2 );
	BigRevBytes( &i, 2, 1 );
	flen += 2;
	( *bp ) += 2;
	return i;
}


int sgetI4( unsigned char** bp )
{
	assert( bp != NULL && *bp != NULL ); // remove compiler warning
	short i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	memcpy( &i, *bp, sizeof( i ) );
	BigRevBytes( &i, 4, 1 );
	flen += 4;
	( *bp ) += 4;
	return i;
}


unsigned char sgetU1( unsigned char** bp )
{
	unsigned char c;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	c = **bp;
	flen += 1;
	( *bp )++;
	return c;
}


unsigned short sgetU2( unsigned char** bp )
{
	unsigned char* buf = *bp;
	unsigned short i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	i = ( buf[ 0 ] << 8 ) | buf[ 1 ];
	flen += 2;
	( *bp ) += 2;
	return i;
}


unsigned int sgetU4( unsigned char** bp )
{
	unsigned int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}
	memcpy( &i, *bp, 4 );
	BigRevBytes( &i, 4, 1 );
	flen += 4;
	( *bp ) += 4;
	return i;
}


int sgetVX( unsigned char** bp )
{
	unsigned char* buf = *bp;
	int i;

	if( flen == FLEN_ERROR )
	{
		return 0;
	}

	if( buf[ 0 ] != 0xFF )
	{
		i = buf[ 0 ] << 8 | buf[ 1 ];
		flen += 2;
		( *bp ) += 2;
	}
	else
	{
		i = ( buf[ 1 ] << 16 ) | ( buf[ 2 ] << 8 ) | buf[ 3 ];
		flen += 4;
		( *bp ) += 4;
	}
	return i;
}


float sgetF4( unsigned char** bp )
{
	float f;

	if( flen == FLEN_ERROR )
	{
		return 0.0f;
	}
	memcpy( &f, *bp, 4 );
	BigRevBytes( &f, 4, 1 );
	flen += 4;
	( *bp ) += 4;

	if( IEEE_FLT_IS_DENORMAL( f ) )
	{
		f = 0.0f;
	}
	return f;
}


char* sgetS0( unsigned char** bp )
{
	char* s;
	unsigned char* buf = *bp;
	int len;

	if( flen == FLEN_ERROR )
	{
		return NULL;
	}

	len = strlen( ( const char* )buf ) + 1;
	if( len == 1 )
	{
		flen += 2;
		( *bp ) += 2;
		return NULL;
	}
	len += len & 1;
	s = ( char* )Mem_ClearedAlloc( len, TAG_MODEL );
	if( !s )
	{
		flen = FLEN_ERROR;
		return NULL;
	}

	memcpy( s, buf, len );
	flen += len;
	( *bp ) += len;
	return s;
}

/*
======================================================================
lwFreeLayer()

Free memory used by an lwLayer.
====================================================================== */

void lwFreeLayer( lwLayer* layer )
{
	if( layer )
	{
		if( layer->name )
		{
			Mem_Free( layer->name );
		}
		lwFreePoints( &layer->point );
		lwFreePolygons( &layer->polygon );
		lwListFree( layer->vmap, ( void ( __cdecl* )( void* ) )lwFreeVMap );
		Mem_Free( layer );
	}
}


/*
======================================================================
lwFreeObject()

Free memory used by an lwObject.
====================================================================== */

void lwFreeObject( lwObject* object )
{
	if( object )
	{
		lwListFree( object->layer, ( void ( __cdecl* )( void* ) )lwFreeLayer );
		lwListFree( object->env, ( void ( __cdecl* )( void* ) )lwFreeEnvelope );
		lwListFree( object->clip, ( void ( __cdecl* )( void* ) )lwFreeClip );
		lwListFree( object->surf, ( void ( __cdecl* )( void* ) )lwFreeSurface );
		lwFreeTags( &object->taglist );
		Mem_Free( object );
	}
}


/*
======================================================================
lwGetObject()

Returns the contents of a LightWave object, given its filename, or
NULL if the file couldn't be loaded.  On failure, failID and failpos
can be used to diagnose the cause.

1.  If the file isn't an LWO2 or an LWOB, failpos will contain 12 and
    failID will be unchanged.

2.  If an error occurs while reading, failID will contain the most
    recently read IFF chunk ID, and failpos will contain the value
    returned by fp->Tell() at the time of the failure.

3.  If the file couldn't be opened, or an error occurs while reading
    the first 12 bytes, both failID and failpos will be unchanged.

If you don't need this information, failID and failpos can be NULL.
====================================================================== */

lwObject* lwGetObject( const char* filename, unsigned int* failID, int* failpos )
{
	idFile* fp = NULL;
	lwObject* object;
	lwLayer* layer;
	lwNode* node;
	int id, formsize, type, cksize;
	int i, rlen;

	fp = fileSystem->OpenFileRead( filename );
	if( !fp )
	{
		return NULL;
	}

	/* read the first 12 bytes */

	set_flen( 0 );
	id       = getU4( fp );
	formsize = getU4( fp );
	type     = getU4( fp );
	if( 12 != get_flen() )
	{
		fileSystem->CloseFile( fp );
		return NULL;
	}

	/* is this a LW object? */

	if( id != ID_FORM )
	{
		fileSystem->CloseFile( fp );
		if( failpos )
		{
			*failpos = 12;
		}
		return NULL;
	}

	if( type != ID_LWO2 )
	{
		fileSystem->CloseFile( fp );
		if( type == ID_LWOB )
		{
			return lwGetObject5( filename, failID, failpos );
		}
		else
		{
			if( failpos )
			{
				*failpos = 12;
			}
			return NULL;
		}
	}

	/* allocate an object and a default layer */

	object = ( lwObject* )Mem_ClearedAlloc( sizeof( lwObject ), TAG_MODEL );
	if( !object )
	{
		goto Fail;
	}

	layer = ( lwLayer* )Mem_ClearedAlloc( sizeof( lwLayer ), TAG_MODEL );
	if( !layer )
	{
		goto Fail;
	}
	object->layer = layer;

	object->timeStamp = fp->Timestamp();

	/* get the first chunk header */

	id = getU4( fp );
	cksize = getU4( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	/* process chunks as they're encountered */

	while( 1 )
	{
		cksize += cksize & 1;

		switch( id )
		{
			case ID_LAYR:
				if( object->nlayers > 0 )
				{
					layer = ( lwLayer* )Mem_ClearedAlloc( sizeof( lwLayer ), TAG_MODEL );
					if( !layer )
					{
						goto Fail;
					}
					lwListAdd( ( void** )&object->layer, layer );
				}
				object->nlayers++;

				set_flen( 0 );
				layer->index = getU2( fp );
				layer->flags = getU2( fp );
				layer->pivot[ 0 ] = getF4( fp );
				layer->pivot[ 1 ] = getF4( fp );
				layer->pivot[ 2 ] = getF4( fp );
				layer->name = getS0( fp );

				rlen = get_flen();
				if( rlen < 0 || rlen > cksize )
				{
					goto Fail;
				}
				if( rlen <= cksize - 2 )
				{
					layer->parent = getU2( fp );
				}
				rlen = get_flen();
				if( rlen < cksize )
				{
					fp->Seek( cksize - rlen, FS_SEEK_CUR );
				}
				break;

			case ID_PNTS:
				if( !lwGetPoints( fp, cksize, &layer->point ) )
				{
					goto Fail;
				}
				break;

			case ID_POLS:
				if( !lwGetPolygons( fp, cksize, &layer->polygon,
									layer->point.offset ) )
				{
					goto Fail;
				}
				break;

			case ID_VMAP:
			case ID_VMAD:
				node = ( lwNode* ) lwGetVMap( fp, cksize, layer->point.offset,
											  layer->polygon.offset, id == ID_VMAD );
				if( !node )
				{
					goto Fail;
				}
				lwListAdd( ( void** )&layer->vmap, node );
				layer->nvmaps++;
				break;

			case ID_PTAG:
				if( !lwGetPolygonTags( fp, cksize, &object->taglist,
									   &layer->polygon ) )
				{
					goto Fail;
				}
				break;

			case ID_BBOX:
				set_flen( 0 );
				for( i = 0; i < 6; i++ )
				{
					layer->bbox[ i ] = getF4( fp );
				}
				rlen = get_flen();
				if( rlen < 0 || rlen > cksize )
				{
					goto Fail;
				}
				if( rlen < cksize )
				{
					fp->Seek( cksize - rlen, FS_SEEK_CUR );
				}
				break;

			case ID_TAGS:
				if( !lwGetTags( fp, cksize, &object->taglist ) )
				{
					goto Fail;
				}
				break;

			case ID_ENVL:
				node = ( lwNode* ) lwGetEnvelope( fp, cksize );
				if( !node )
				{
					goto Fail;
				}
				lwListAdd( ( void** )&object->env, node );
				object->nenvs++;
				break;

			case ID_CLIP:
				node = ( lwNode* ) lwGetClip( fp, cksize );
				if( !node )
				{
					goto Fail;
				}
				lwListAdd( ( void** )&object->clip, node );
				object->nclips++;
				break;

			case ID_SURF:
				node = ( lwNode* ) lwGetSurface( fp, cksize );
				if( !node )
				{
					goto Fail;
				}
				lwListAdd( ( void** )&object->surf, node );
				object->nsurfs++;
				break;

			case ID_DESC:
			case ID_TEXT:
			case ID_ICON:
			default:
				fp->Seek( cksize, FS_SEEK_CUR );
				break;
		}

		/* end of the file? */

		if( formsize <= fp->Tell() - 8 )
		{
			break;
		}

		/* get the next chunk header */

		set_flen( 0 );
		id = getU4( fp );
		cksize = getU4( fp );
		if( 8 != get_flen() )
		{
			goto Fail;
		}
	}

	fileSystem->CloseFile( fp );
	fp = NULL;

	if( object->nlayers == 0 )
	{
		object->nlayers = 1;
	}

	layer = object->layer;
	while( layer )
	{
		lwGetBoundingBox( &layer->point, layer->bbox );
		lwGetPolyNormals( &layer->point, &layer->polygon );
		if( !lwGetPointPolygons( &layer->point, &layer->polygon ) )
		{
			goto Fail;
		}
		if( !lwResolvePolySurfaces( &layer->polygon, &object->taglist,
									&object->surf, &object->nsurfs ) )
		{
			goto Fail;
		}
		lwGetVertNormals( &layer->point, &layer->polygon );
		if( !lwGetPointVMaps( &layer->point, layer->vmap ) )
		{
			goto Fail;
		}
		if( !lwGetPolyVMaps( &layer->polygon, layer->vmap ) )
		{
			goto Fail;
		}
		layer = layer->next;
	}

	return object;

Fail:
	if( failID )
	{
		*failID = id;
	}
	if( fp )
	{
		if( failpos )
		{
			*failpos = fp->Tell();
		}
		fileSystem->CloseFile( fp );
	}
	lwFreeObject( object );
	return NULL;
}




/* IDs specific to LWOB */

#define ID_SRFS  LWID_('S','R','F','S')
#define ID_FLAG  LWID_('F','L','A','G')
#define ID_VLUM  LWID_('V','L','U','M')
#define ID_VDIF  LWID_('V','D','I','F')
#define ID_VSPC  LWID_('V','S','P','C')
#define ID_RFLT  LWID_('R','F','L','T')
#define ID_BTEX  LWID_('B','T','E','X')
#define ID_CTEX  LWID_('C','T','E','X')
#define ID_DTEX  LWID_('D','T','E','X')
#define ID_LTEX  LWID_('L','T','E','X')
#define ID_RTEX  LWID_('R','T','E','X')
#define ID_STEX  LWID_('S','T','E','X')
#define ID_TTEX  LWID_('T','T','E','X')
#define ID_TFLG  LWID_('T','F','L','G')
#define ID_TSIZ  LWID_('T','S','I','Z')
#define ID_TCTR  LWID_('T','C','T','R')
#define ID_TFAL  LWID_('T','F','A','L')
#define ID_TVEL  LWID_('T','V','E','L')
#define ID_TCLR  LWID_('T','C','L','R')
#define ID_TVAL  LWID_('T','V','A','L')
#define ID_TAMP  LWID_('T','A','M','P')
#define ID_TIMG  LWID_('T','I','M','G')
#define ID_TAAS  LWID_('T','A','A','S')
#define ID_TREF  LWID_('T','R','E','F')
#define ID_TOPC  LWID_('T','O','P','C')
#define ID_SDAT  LWID_('S','D','A','T')
#define ID_TFP0  LWID_('T','F','P','0')
#define ID_TFP1  LWID_('T','F','P','1')


/*
======================================================================
add_clip()

Add a clip to the clip list.  Used to store the contents of an RIMG or
TIMG surface subchunk.
====================================================================== */

static int add_clip( char* s, lwClip** clist, int* nclips )
{
	lwClip* clip;
	char* p;

	clip = ( lwClip* )Mem_ClearedAlloc( sizeof( lwClip ), TAG_MODEL );
	if( clip == NULL )
	{
		return 0;
	}

	clip->contrast.val = 1.0f;
	clip->brightness.val = 1.0f;
	clip->saturation.val = 1.0f;
	clip->gamma.val = 1.0f;

	if( ( p = strstr( s, "(sequence)" ) ) != NULL )
	{
		p[ -1 ] = 0;
		clip->type = ID_ISEQ;
		clip->source.seq.prefix = s;
		clip->source.seq.digits = 3;
	}
	else
	{
		clip->type = ID_STIL;
		clip->source.still.name = s;
	}

	( *nclips )++;
	clip->index = *nclips;

	lwListAdd( ( void** )clist, clip );

	return clip->index;
}


/*
======================================================================
add_tvel()

Add a triple of envelopes to simulate the old texture velocity
parameters.
====================================================================== */

static int add_tvel( float pos[], float vel[], lwEnvelope** elist, int* nenvs )
{
	lwEnvelope* env = NULL;
	lwKey* key0 = NULL, *key1 = NULL;
	int i;

	for( i = 0; i < 3; i++ )
	{
		env = ( lwEnvelope* )Mem_ClearedAlloc( sizeof( lwEnvelope ), TAG_MODEL );
		key0 = ( lwKey* )Mem_ClearedAlloc( sizeof( lwKey ), TAG_MODEL );
		key1 = ( lwKey* )Mem_ClearedAlloc( sizeof( lwKey ), TAG_MODEL );
		if( !env || !key0 || !key1 )
		{
			return 0;
		}

		key0->next = key1;
		key0->value = pos[ i ];
		key0->time = 0.0f;
		key1->prev = key0;
		key1->value = pos[ i ] + vel[ i ] * 30.0f;
		key1->time = 1.0f;
		key0->shape = key1->shape = ID_LINE;

		env->index = *nenvs + i + 1;
		env->type = 0x0301 + i;
		env->name = ( char* )Mem_ClearedAlloc( 11, TAG_MODEL );
		if( env->name )
		{
			strcpy( env->name, "Position.X" );
			env->name[ 9 ] += i;
		}
		env->key = key0;
		env->nkeys = 2;
		env->behavior[ 0 ] = BEH_LINEAR;
		env->behavior[ 1 ] = BEH_LINEAR;

		lwListAdd( ( void** )elist, env );
	}
	assert( env != NULL );

	*nenvs += 3;
	return env->index - 2;
}


/*
======================================================================
get_texture()

Create a new texture for BTEX, CTEX, etc. subchunks.
====================================================================== */

static lwTexture* get_texture( char* s )
{
	lwTexture* tex;

	tex = ( lwTexture* )Mem_ClearedAlloc( sizeof( lwTexture ), TAG_MODEL );
	if( !tex )
	{
		return NULL;
	}

	tex->tmap.size.val[ 0 ] =
		tex->tmap.size.val[ 1 ] =
			tex->tmap.size.val[ 2 ] = 1.0f;
	tex->opacity.val = 1.0f;
	tex->enabled = 1;

	if( strstr( s, "Image Map" ) )
	{
		tex->type = ID_IMAP;
		if( strstr( s, "Planar" ) )
		{
			tex->param.imap.projection = 0;
		}
		else if( strstr( s, "Cylindrical" ) )
		{
			tex->param.imap.projection = 1;
		}
		else if( strstr( s, "Spherical" ) )
		{
			tex->param.imap.projection = 2;
		}
		else if( strstr( s, "Cubic" ) )
		{
			tex->param.imap.projection = 3;
		}
		else if( strstr( s, "Front" ) )
		{
			tex->param.imap.projection = 4;
		}
		tex->param.imap.aa_strength = 1.0f;
		tex->param.imap.amplitude.val = 1.0f;
		Mem_Free( s );
	}
	else
	{
		tex->type = ID_PROC;
		tex->param.proc.name = s;
	}

	return tex;
}


/*
======================================================================
lwGetSurface5()

Read an lwSurface from an LWOB file.
====================================================================== */

lwSurface* lwGetSurface5( idFile* fp, int cksize, lwObject* obj )
{
	lwSurface* surf = NULL;
	lwTexture* tex = NULL;
	lwPlugin* shdr = NULL;
	char* s = NULL;
	float v[ 3 ];
	unsigned int id, flags;
	unsigned short sz;
	int pos, rlen, i = 0;


	/* allocate the Surface structure */

	surf = ( lwSurface* )Mem_ClearedAlloc( sizeof( lwSurface ), TAG_MODEL );
	if( !surf )
	{
		goto Fail;
	}

	/* non-zero defaults */

	surf->color.rgb[ 0 ] = 0.78431f;
	surf->color.rgb[ 1 ] = 0.78431f;
	surf->color.rgb[ 2 ] = 0.78431f;
	surf->diffuse.val    = 1.0f;
	surf->glossiness.val = 0.4f;
	surf->bump.val       = 1.0f;
	surf->eta.val        = 1.0f;
	surf->sideflags      = 1;

	/* remember where we started */

	set_flen( 0 );
	pos = fp->Tell();

	/* name */

	surf->name = getS0( fp );

	/* first subchunk header */

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	/* process subchunks as they're encountered */

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_COLR:
				surf->color.rgb[ 0 ] = getU1( fp ) / 255.0f;
				surf->color.rgb[ 1 ] = getU1( fp ) / 255.0f;
				surf->color.rgb[ 2 ] = getU1( fp ) / 255.0f;
				break;

			case ID_FLAG:
				flags = getU2( fp );
				if( flags &   4 )
				{
					surf->smooth = 1.56207f;
				}
				if( flags &   8 )
				{
					surf->color_hilite.val = 1.0f;
				}
				if( flags &  16 )
				{
					surf->color_filter.val = 1.0f;
				}
				if( flags & 128 )
				{
					surf->dif_sharp.val = 0.5f;
				}
				if( flags & 256 )
				{
					surf->sideflags = 3;
				}
				if( flags & 512 )
				{
					surf->add_trans.val = 1.0f;
				}
				break;

			case ID_LUMI:
				surf->luminosity.val = getI2( fp ) / 256.0f;
				break;

			case ID_VLUM:
				surf->luminosity.val = getF4( fp );
				break;

			case ID_DIFF:
				surf->diffuse.val = getI2( fp ) / 256.0f;
				break;

			case ID_VDIF:
				surf->diffuse.val = getF4( fp );
				break;

			case ID_SPEC:
				surf->specularity.val = getI2( fp ) / 256.0f;
				break;

			case ID_VSPC:
				surf->specularity.val = getF4( fp );
				break;

			case ID_GLOS:
				surf->glossiness.val = ( float ) logf( ( float ) getU2( fp ) ) / 20.7944f;
				break;

			case ID_SMAN:
				surf->smooth = getF4( fp );
				break;

			case ID_REFL:
				surf->reflection.val.val = getI2( fp ) / 256.0f;
				break;

			case ID_RFLT:
				surf->reflection.options = getU2( fp );
				break;

			case ID_RIMG:
				s = getS0( fp );
				surf->reflection.cindex = add_clip( s, &obj->clip, &obj->nclips );
				surf->reflection.options = 3;
				break;

			case ID_RSAN:
				surf->reflection.seam_angle = getF4( fp );
				break;

			case ID_TRAN:
				surf->transparency.val.val = getI2( fp ) / 256.0f;
				break;

			case ID_RIND:
				surf->eta.val = getF4( fp );
				break;

			case ID_BTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->bump.tex, tex );
				break;

			case ID_CTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->color.tex, tex );
				break;

			case ID_DTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->diffuse.tex, tex );
				break;

			case ID_LTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->luminosity.tex, tex );
				break;

			case ID_RTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->reflection.val.tex, tex );
				break;

			case ID_STEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->specularity.tex, tex );
				break;

			case ID_TTEX:
				s = ( char* )getbytes( fp, sz );
				tex = get_texture( s );
				lwListAdd( ( void** )&surf->transparency.val.tex, tex );
				break;

			case ID_TFLG:
				assert( tex != NULL );
				flags = getU2( fp );

				if( flags & 1 )
				{
					i = 0;
				}
				if( flags & 2 )
				{
					i = 1;
				}
				if( flags & 4 )
				{
					i = 2;
				}
				tex->axis = i;
				if( tex->type == ID_IMAP )
				{
					tex->param.imap.axis = i;
				}
				else
				{
					tex->param.proc.axis = i;
				}

				if( flags &  8 )
				{
					tex->tmap.coord_sys = 1;
				}
				if( flags & 16 )
				{
					tex->negative = 1;
				}
				if( flags & 32 )
				{
					tex->param.imap.pblend = 1;
				}
				if( flags & 64 )
				{
					tex->param.imap.aa_strength = 1.0f;
					tex->param.imap.aas_flags = 1;
				}
				break;

			case ID_TSIZ:
				assert( tex != NULL );
				for( i = 0; i < 3; i++ )
				{
					tex->tmap.size.val[ i ] = getF4( fp );
				}
				break;

			case ID_TCTR:
				assert( tex != NULL );
				for( i = 0; i < 3; i++ )
				{
					tex->tmap.center.val[ i ] = getF4( fp );
				}
				break;

			case ID_TFAL:
				assert( tex != NULL );
				for( i = 0; i < 3; i++ )
				{
					tex->tmap.falloff.val[ i ] = getF4( fp );
				}
				break;

			case ID_TVEL:
				assert( tex != NULL );
				for( i = 0; i < 3; i++ )
				{
					v[ i ] = getF4( fp );
				}
				tex->tmap.center.eindex = add_tvel( tex->tmap.center.val, v,
													&obj->env, &obj->nenvs );
				break;

			case ID_TCLR:
				assert( tex != NULL );
				if( tex->type == ID_PROC )
					for( i = 0; i < 3; i++ )
					{
						tex->param.proc.value[ i ] = getU1( fp ) / 255.0f;
					}
				break;

			case ID_TVAL:
				assert( tex != NULL );
				tex->param.proc.value[ 0 ] = getI2( fp ) / 256.0f;
				break;

			case ID_TAMP:
				assert( tex != NULL );
				if( tex->type == ID_IMAP )
				{
					tex->param.imap.amplitude.val = getF4( fp );
				}
				break;

			case ID_TIMG:
				assert( tex != NULL );
				s = getS0( fp );
				tex->param.imap.cindex = add_clip( s, &obj->clip, &obj->nclips );
				break;

			case ID_TAAS:
				assert( tex != NULL );
				tex->param.imap.aa_strength = getF4( fp );
				tex->param.imap.aas_flags = 1;
				break;

			case ID_TREF:
				assert( tex != NULL );
				tex->tmap.ref_object = ( char* )getbytes( fp, sz );
				break;

			case ID_TOPC:
				assert( tex != NULL );
				tex->opacity.val = getF4( fp );
				break;

			case ID_TFP0:
				assert( tex != NULL );
				if( tex->type == ID_IMAP )
				{
					tex->param.imap.wrapw.val = getF4( fp );
				}
				break;

			case ID_TFP1:
				assert( tex != NULL );
				if( tex->type == ID_IMAP )
				{
					tex->param.imap.wraph.val = getF4( fp );
				}
				break;

			case ID_SHDR:
				shdr = ( lwPlugin* )Mem_ClearedAlloc( sizeof( lwPlugin ), TAG_MODEL );
				if( !shdr )
				{
					goto Fail;
				}
				shdr->name = ( char* )getbytes( fp, sz );
				lwListAdd( ( void** )&surf->shader, shdr );
				surf->nshaders++;
				break;

			case ID_SDAT:
				assert( shdr != NULL );
				shdr->data = getbytes( fp, sz );
				break;

			default:
				break;
		}

		/* error while reading current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			goto Fail;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the SURF chunk? */

		if( cksize <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			goto Fail;
		}
	}

	return surf;

Fail:
	if( surf )
	{
		lwFreeSurface( surf );
	}
	return NULL;
}


/*
======================================================================
lwGetPolygons5()

Read polygon records from a POLS chunk in an LWOB file.  The polygons
are added to the array in the lwPolygonList.
====================================================================== */

int lwGetPolygons5( idFile* fp, int cksize, lwPolygonList* plist, int ptoffset )
{
	lwPolygon* pp;
	lwPolVert* pv;
	unsigned char* buf, *bp;
	int i, j, nv, nverts, npols;


	if( cksize == 0 )
	{
		return 1;
	}

	/* read the whole chunk */

	set_flen( 0 );
	buf = ( unsigned char* )getbytes( fp, cksize );
	if( !buf )
	{
		goto Fail;
	}

	/* count the polygons and vertices */

	nverts = 0;
	npols = 0;
	bp = buf;

	while( bp < buf + cksize )
	{
		nv = sgetU2( &bp );
		nverts += nv;
		npols++;
		bp += 2 * nv;
		i = sgetI2( &bp );
		if( i < 0 )
		{
			bp += 2;    /* detail polygons */
		}
	}

	if( !lwAllocPolygons( plist, npols, nverts ) )
	{
		goto Fail;
	}

	/* fill in the new polygons */

	bp = buf;
	pp = plist->pol + plist->offset;
	pv = plist->pol[ 0 ].v + plist->voffset;

	for( i = 0; i < npols; i++ )
	{
		nv = sgetU2( &bp );

		pp->nverts = nv;
		pp->type = ID_FACE;
		if( !pp->v )
		{
			pp->v = pv;
		}
		for( j = 0; j < nv; j++ )
		{
			pv[ j ].index = sgetU2( &bp ) + ptoffset;
		}
		j = sgetI2( &bp );
		if( j < 0 )
		{
			j = -j;
			bp += 2;
		}
		j -= 1;
		pp->surf.index = j;

		pp++;
		pv += nv;
	}

	Mem_Free( buf );
	return 1;

Fail:
	if( buf )
	{
		Mem_Free( buf );
	}
	lwFreePolygons( plist );
	return 0;
}


/*
======================================================================
getLWObject5()

Returns the contents of an LWOB, given its filename, or NULL if the
file couldn't be loaded.  On failure, failID and failpos can be used
to diagnose the cause.

1.  If the file isn't an LWOB, failpos will contain 12 and failID will
    be unchanged.

2.  If an error occurs while reading an LWOB, failID will contain the
    most recently read IFF chunk ID, and failpos will contain the
    value returned by fp->Tell() at the time of the failure.

3.  If the file couldn't be opened, or an error occurs while reading
    the first 12 bytes, both failID and failpos will be unchanged.

If you don't need this information, failID and failpos can be NULL.
====================================================================== */

lwObject* lwGetObject5( const char* filename, unsigned int* failID, int* failpos )
{
	idFile* fp = NULL;
	lwObject* object;
	lwLayer* layer;
	lwNode* node;
	int id, formsize, type, cksize;


	/* open the file */

	//fp = fopen( filename, "rb" );
	//if ( !fp ) return NULL;

	/* read the first 12 bytes */
	fp = fileSystem->OpenFileRead( filename );
	if( !fp )
	{
		return NULL;
	}

	set_flen( 0 );
	id       = getU4( fp );
	formsize = getU4( fp );
	type     = getU4( fp );
	if( 12 != get_flen() )
	{
		fileSystem->CloseFile( fp );
		return NULL;
	}

	/* LWOB? */

	if( id != ID_FORM || type != ID_LWOB )
	{
		fileSystem->CloseFile( fp );
		if( failpos )
		{
			*failpos = 12;
		}
		return NULL;
	}

	/* allocate an object and a default layer */

	object = ( lwObject* )Mem_ClearedAlloc( sizeof( lwObject ), TAG_MODEL );
	if( !object )
	{
		goto Fail2;
	}

	layer = ( lwLayer* )Mem_ClearedAlloc( sizeof( lwLayer ), TAG_MODEL );
	if( !layer )
	{
		goto Fail2;
	}
	object->layer = layer;
	object->nlayers = 1;

	/* get the first chunk header */

	id = getU4( fp );
	cksize = getU4( fp );
	if( 0 > get_flen() )
	{
		goto Fail2;
	}

	/* process chunks as they're encountered */

	while( 1 )
	{
		cksize += cksize & 1;

		switch( id )
		{
			case ID_PNTS:
				if( !lwGetPoints( fp, cksize, &layer->point ) )
				{
					goto Fail2;
				}
				break;

			case ID_POLS:
				if( !lwGetPolygons5( fp, cksize, &layer->polygon,
									 layer->point.offset ) )
				{
					goto Fail2;
				}
				break;

			case ID_SRFS:
				if( !lwGetTags( fp, cksize, &object->taglist ) )
				{
					goto Fail2;
				}
				break;

			case ID_SURF:
				node = ( lwNode* ) lwGetSurface5( fp, cksize, object );
				if( !node )
				{
					goto Fail2;
				}
				lwListAdd( ( void** )&object->surf, node );
				object->nsurfs++;
				break;

			default:
				fp->Seek( cksize, FS_SEEK_CUR );
				break;
		}

		/* end of the file? */

		if( formsize <= fp->Tell() - 8 )
		{
			break;
		}

		/* get the next chunk header */

		set_flen( 0 );
		id = getU4( fp );
		cksize = getU4( fp );
		if( 8 != get_flen() )
		{
			goto Fail2;
		}
	}

	fileSystem->CloseFile( fp );
	fp = NULL;

	lwGetBoundingBox( &layer->point, layer->bbox );
	lwGetPolyNormals( &layer->point, &layer->polygon );
	if( !lwGetPointPolygons( &layer->point, &layer->polygon ) )
	{
		goto Fail2;
	}
	if( !lwResolvePolySurfaces( &layer->polygon, &object->taglist,
								&object->surf, &object->nsurfs ) )
	{
		goto Fail2;
	}
	lwGetVertNormals( &layer->point, &layer->polygon );

	return object;

Fail2:
	if( failID )
	{
		*failID = id;
	}
	if( fp )
	{
		if( failpos )
		{
			*failpos = fp->Tell();
		}
		fileSystem->CloseFile( fp );
	}
	lwFreeObject( object );
	return NULL;
}

/*
======================================================================
lwFreePoints()

Free the memory used by an lwPointList.
====================================================================== */

void lwFreePoints( lwPointList* point )
{
	int i;

	if( point )
	{
		if( point->pt )
		{
			for( i = 0; i < point->count; i++ )
			{
				if( point->pt[ i ].pol )
				{
					Mem_Free( point->pt[ i ].pol );
				}
				if( point->pt[ i ].vm )
				{
					Mem_Free( point->pt[ i ].vm );
				}
			}
			Mem_Free( point->pt );
		}
		memset( point, 0, sizeof( lwPointList ) );
	}
}


/*
======================================================================
lwFreePolygons()

Free the memory used by an lwPolygonList.
====================================================================== */

void lwFreePolygons( lwPolygonList* plist )
{
	int i, j;

	if( plist )
	{
		if( plist->pol )
		{
			for( i = 0; i < plist->count; i++ )
			{
				if( plist->pol[ i ].v )
				{
					for( j = 0; j < plist->pol[ i ].nverts; j++ )
						if( plist->pol[ i ].v[ j ].vm )
						{
							Mem_Free( plist->pol[ i ].v[ j ].vm );
						}
				}
			}
			if( plist->pol[ 0 ].v )
			{
				Mem_Free( plist->pol[ 0 ].v );
			}
			Mem_Free( plist->pol );
		}
		memset( plist, 0, sizeof( lwPolygonList ) );
	}
}


/*
======================================================================
lwGetPoints()

Read point records from a PNTS chunk in an LWO2 file.  The points are
added to the array in the lwPointList.
====================================================================== */

int lwGetPoints( idFile* fp, int cksize, lwPointList* point )
{
	float* f;
	int np, i, j;

	if( cksize == 1 )
	{
		return 1;
	}

	/* extend the point array to hold the new points */

	np = cksize / 12;
	point->offset = point->count;
	point->count += np;
	lwPoint* oldpt = point->pt;
	point->pt = ( lwPoint* )Mem_Alloc( point->count * sizeof( lwPoint ), TAG_MODEL );
	if( !point->pt )
	{
		return 0;
	}
	if( oldpt )
	{
		memcpy( point->pt, oldpt, point->offset * sizeof( lwPoint ) );
		Mem_Free( oldpt );
	}
	memset( &point->pt[ point->offset ], 0, np * sizeof( lwPoint ) );

	/* read the whole chunk */

	f = ( float* ) getbytes( fp, cksize );
	if( !f )
	{
		return 0;
	}
	BigRevBytes( f, 4, np * 3 );

	/* assign position values */

	for( i = 0, j = 0; i < np; i++, j += 3 )
	{
		point->pt[ i ].pos[ 0 ] = f[ j ];
		point->pt[ i ].pos[ 1 ] = f[ j + 1 ];
		point->pt[ i ].pos[ 2 ] = f[ j + 2 ];
	}

	Mem_Free( f );
	return 1;
}


/*
======================================================================
lwGetBoundingBox()

Calculate the bounding box for a point list, but only if the bounding
box hasn't already been initialized.
====================================================================== */

void lwGetBoundingBox( lwPointList* point, float bbox[] )
{
	int i, j;

	if( point->count == 0 )
	{
		return;
	}

	for( i = 0; i < 6; i++ )
		if( bbox[ i ] != 0.0f )
		{
			return;
		}

	bbox[ 0 ] = bbox[ 1 ] = bbox[ 2 ] = 1e20f;
	bbox[ 3 ] = bbox[ 4 ] = bbox[ 5 ] = -1e20f;
	for( i = 0; i < point->count; i++ )
	{
		for( j = 0; j < 3; j++ )
		{
			if( bbox[ j ] > point->pt[ i ].pos[ j ] )
			{
				bbox[ j ] = point->pt[ i ].pos[ j ];
			}
			if( bbox[ j + 3 ] < point->pt[ i ].pos[ j ] )
			{
				bbox[ j + 3 ] = point->pt[ i ].pos[ j ];
			}
		}
	}
}


/*
======================================================================
lwAllocPolygons()

Allocate or extend the polygon arrays to hold new records.
====================================================================== */

int lwAllocPolygons( lwPolygonList* plist, int npols, int nverts )
{
	int i;

	plist->offset = plist->count;
	plist->count += npols;
	lwPolygon* oldpol = plist->pol;
	plist->pol = ( lwPolygon* )Mem_Alloc( plist->count * sizeof( lwPolygon ), TAG_MODEL );
	if( !plist->pol )
	{
		return 0;
	}
	if( oldpol )
	{
		memcpy( plist->pol, oldpol, plist->offset * sizeof( lwPolygon ) );
		Mem_Free( oldpol );
	}
	memset( plist->pol + plist->offset, 0, npols * sizeof( lwPolygon ) );

	plist->voffset = plist->vcount;
	plist->vcount += nverts;
	lwPolVert* oldpolv = plist->pol[0].v;
	plist->pol[0].v = ( lwPolVert* )Mem_Alloc( plist->vcount * sizeof( lwPolVert ), TAG_MODEL );
	if( !plist->pol[ 0 ].v )
	{
		return 0;
	}
	if( oldpolv )
	{
		memcpy( plist->pol[0].v, oldpolv, plist->voffset * sizeof( lwPolVert ) );
		Mem_Free( oldpolv );
	}
	memset( plist->pol[ 0 ].v + plist->voffset, 0, nverts * sizeof( lwPolVert ) );

	/* fix up the old vertex pointers */

	for( i = 1; i < plist->offset; i++ )
	{
		plist->pol[ i ].v = plist->pol[ i - 1 ].v + plist->pol[ i - 1 ].nverts;
	}

	return 1;
}


/*
======================================================================
lwGetPolygons()

Read polygon records from a POLS chunk in an LWO2 file.  The polygons
are added to the array in the lwPolygonList.
====================================================================== */

int lwGetPolygons( idFile* fp, int cksize, lwPolygonList* plist, int ptoffset )
{
	lwPolygon* pp;
	lwPolVert* pv;
	unsigned char* buf, *bp;
	int i, j, flags, nv, nverts, npols;
	unsigned int type;


	if( cksize == 0 )
	{
		return 1;
	}

	/* read the whole chunk */

	set_flen( 0 );
	type = getU4( fp );
	buf = ( unsigned char* )getbytes( fp, cksize - 4 );
	if( cksize != get_flen() )
	{
		goto Fail;
	}

	/* count the polygons and vertices */

	nverts = 0;
	npols = 0;
	bp = buf;

	while( bp < buf + cksize - 4 )
	{
		nv = sgetU2( &bp );
		nv &= 0x03FF;
		nverts += nv;
		npols++;
		for( i = 0; i < nv; i++ )
		{
			j = sgetVX( &bp );
		}
	}

	if( !lwAllocPolygons( plist, npols, nverts ) )
	{
		goto Fail;
	}

	/* fill in the new polygons */

	bp = buf;
	pp = plist->pol + plist->offset;
	pv = plist->pol[ 0 ].v + plist->voffset;

	for( i = 0; i < npols; i++ )
	{
		nv = sgetU2( &bp );
		flags = nv & 0xFC00;
		nv &= 0x03FF;

		pp->nverts = nv;
		pp->flags = flags;
		pp->type = type;
		if( !pp->v )
		{
			pp->v = pv;
		}
		for( j = 0; j < nv; j++ )
		{
			pp->v[ j ].index = sgetVX( &bp ) + ptoffset;
		}

		pp++;
		pv += nv;
	}

	Mem_Free( buf );
	return 1;

Fail:
	if( buf )
	{
		Mem_Free( buf );
	}
	lwFreePolygons( plist );
	return 0;
}


/*
======================================================================
lwGetPolyNormals()

Calculate the polygon normals.  By convention, LW's polygon normals
are found as the cross product of the first and last edges.  It's
undefined for one- and two-point polygons.
====================================================================== */

void lwGetPolyNormals( lwPointList* point, lwPolygonList* polygon )
{
	int i, j;
	float p1[ 3 ], p2[ 3 ], pn[ 3 ], v1[ 3 ], v2[ 3 ];

	for( i = 0; i < polygon->count; i++ )
	{
		if( polygon->pol[ i ].nverts < 3 )
		{
			continue;
		}
		for( j = 0; j < 3; j++ )
		{

			// FIXME: track down why indexes are way out of range
			p1[ j ] = point->pt[ polygon->pol[ i ].v[ 0 ].index ].pos[ j ];
			p2[ j ] = point->pt[ polygon->pol[ i ].v[ 1 ].index ].pos[ j ];
			pn[ j ] = point->pt[ polygon->pol[ i ].v[ polygon->pol[ i ].nverts - 1 ].index ].pos[ j ];
		}

		for( j = 0; j < 3; j++ )
		{
			v1[ j ] = p2[ j ] - p1[ j ];
			v2[ j ] = pn[ j ] - p1[ j ];
		}

		cross( v1, v2, polygon->pol[ i ].norm );
		normalize( polygon->pol[ i ].norm );
	}
}


/*
======================================================================
lwGetPointPolygons()

For each point, fill in the indexes of the polygons that share the
point.  Returns 0 if any of the memory allocations fail, otherwise
returns 1.
====================================================================== */

int lwGetPointPolygons( lwPointList* point, lwPolygonList* polygon )
{
	int i, j, k;

	/* count the number of polygons per point */

	for( i = 0; i < polygon->count; i++ )
		for( j = 0; j < polygon->pol[ i ].nverts; j++ )
		{
			++point->pt[ polygon->pol[ i ].v[ j ].index ].npols;
		}

	/* alloc per-point polygon arrays */

	for( i = 0; i < point->count; i++ )
	{
		if( point->pt[ i ].npols == 0 )
		{
			continue;
		}
		point->pt[ i ].pol = ( int* )Mem_ClearedAlloc( point->pt[ i ].npols * sizeof( int ), TAG_MODEL );
		if( !point->pt[ i ].pol )
		{
			return 0;
		}
		point->pt[ i ].npols = 0;
	}

	/* fill in polygon array for each point */

	for( i = 0; i < polygon->count; i++ )
	{
		for( j = 0; j < polygon->pol[ i ].nverts; j++ )
		{
			k = polygon->pol[ i ].v[ j ].index;
			point->pt[ k ].pol[ point->pt[ k ].npols ] = i;
			++point->pt[ k ].npols;
		}
	}

	return 1;
}


/*
======================================================================
lwResolvePolySurfaces()

Convert tag indexes into actual lwSurface pointers.  If any polygons
point to tags for which no corresponding surface can be found, a
default surface is created.
====================================================================== */

int lwResolvePolySurfaces( lwPolygonList* polygon, lwTagList* tlist,
						   lwSurface** surf, int* nsurfs )
{
	lwSurface** s, *st;
	int i;

	if( tlist->count == 0 )
	{
		return 1;
	}

	s = ( lwSurface** )Mem_ClearedAlloc( tlist->count * sizeof( lwSurface* ), TAG_MODEL );
	if( !s )
	{
		return 0;
	}

	for( i = 0; i < tlist->count; i++ )
	{
		st = *surf;
		while( st )
		{
			if( !strcmp( st->name, tlist->tag[ i ] ) )
			{
				s[ i ] = st;
				break;
			}
			st = st->next;
		}
	}

	intptr_t index;
	for( i = 0; i < polygon->count; i++ )
	{
		index = polygon->pol[ i ].surf.index;

		if( index < 0 || index > tlist->count )
		{
			return 0;
		}
		if( !s[ index ] )
		{
			s[ index ] = lwDefaultSurface();
			if( !s[ index ] )
			{
				return 0;
			}
			s[ index ]->name = ( char* )Mem_ClearedAlloc( strlen( tlist->tag[ index ] ) + 1, TAG_MODEL );
			if( !s[ index ]->name )
			{
				return 0;
			}
			strcpy( s[ index ]->name, tlist->tag[ index ] );
			lwListAdd( ( void** )surf, s[ index ] );
			*nsurfs = *nsurfs + 1;
		}
		polygon->pol[ i ].surf.ptr = s[ index ];
	}

	Mem_Free( s );
	return 1;
}


/*
======================================================================
lwGetVertNormals()

Calculate the vertex normals.  For each polygon vertex, sum the
normals of the polygons that share the point.  If the normals of the
current and adjacent polygons form an angle greater than the max
smoothing angle for the current polygon's surface, the normal of the
adjacent polygon is excluded from the sum.  It's also excluded if the
polygons aren't in the same smoothing group.

Assumes that lwGetPointPolygons(), lwGetPolyNormals() and
lwResolvePolySurfaces() have already been called.
====================================================================== */

void lwGetVertNormals( lwPointList* point, lwPolygonList* polygon )
{
	int j, k, n, g, h, p;
	float a;

	for( j = 0; j < polygon->count; j++ )
	{
		for( n = 0; n < polygon->pol[ j ].nverts; n++ )
		{
			for( k = 0; k < 3; k++ )
			{
				polygon->pol[ j ].v[ n ].norm[ k ] = polygon->pol[ j ].norm[ k ];
			}

			if( polygon->pol[ j ].surf.ptr->smooth <= 0 )
			{
				continue;
			}

			p = polygon->pol[ j ].v[ n ].index;

			for( g = 0; g < point->pt[ p ].npols; g++ )
			{
				h = point->pt[ p ].pol[ g ];
				if( h == j )
				{
					continue;
				}

				if( polygon->pol[ j ].smoothgrp != polygon->pol[ h ].smoothgrp )
				{
					continue;
				}
				a = vecangle( polygon->pol[ j ].norm, polygon->pol[ h ].norm );
				if( a > polygon->pol[ j ].surf.ptr->smooth )
				{
					continue;
				}

				for( k = 0; k < 3; k++ )
				{
					polygon->pol[ j ].v[ n ].norm[ k ] += polygon->pol[ h ].norm[ k ];
				}
			}

			normalize( polygon->pol[ j ].v[ n ].norm );
		}
	}
}


/*
======================================================================
lwFreeTags()

Free memory used by an lwTagList.
====================================================================== */

void lwFreeTags( lwTagList* tlist )
{
	int i;

	if( tlist )
	{
		if( tlist->tag )
		{
			for( i = 0; i < tlist->count; i++ )
				if( tlist->tag[ i ] )
				{
					Mem_Free( tlist->tag[ i ] );
				}
			Mem_Free( tlist->tag );
		}
		memset( tlist, 0, sizeof( lwTagList ) );
	}
}


/*
======================================================================
lwGetTags()

Read tag strings from a TAGS chunk in an LWO2 file.  The tags are
added to the lwTagList array.
====================================================================== */

int lwGetTags( idFile* fp, int cksize, lwTagList* tlist )
{
	char* buf, *bp;
	int i, len, ntags;

	if( cksize == 0 )
	{
		return 1;
	}

	/* read the whole chunk */

	set_flen( 0 );
	buf = ( char* )getbytes( fp, cksize );
	if( !buf )
	{
		return 0;
	}

	/* count the strings */

	ntags = 0;
	bp = buf;
	while( bp < buf + cksize )
	{
		len = strlen( bp ) + 1;
		len += len & 1;
		bp += len;
		++ntags;
	}

	/* expand the string array to hold the new tags */

	tlist->offset = tlist->count;
	tlist->count += ntags;
	char** oldtag = tlist->tag;
	tlist->tag = ( char** )Mem_Alloc( tlist->count * sizeof( char* ), TAG_MODEL );
	if( !tlist->tag )
	{
		goto Fail;
	}
	if( oldtag )
	{
		memcpy( tlist->tag, oldtag, tlist->offset * sizeof( char* ) );
		Mem_Free( oldtag );
	}
	memset( &tlist->tag[ tlist->offset ], 0, ntags * sizeof( char* ) );

	/* copy the new tags to the tag array */

	bp = buf;
	for( i = 0; i < ntags; i++ )
	{
		tlist->tag[ i + tlist->offset ] = sgetS0( ( unsigned char** )&bp );
	}

	Mem_Free( buf );
	return 1;

Fail:
	if( buf )
	{
		Mem_Free( buf );
	}
	return 0;
}


/*
======================================================================
lwGetPolygonTags()

Read polygon tags from a PTAG chunk in an LWO2 file.
====================================================================== */

int lwGetPolygonTags( idFile* fp, int cksize, lwTagList* tlist, lwPolygonList* plist )
{
	unsigned int type;
	int rlen = 0, i, j;

	set_flen( 0 );
	type = getU4( fp );
	rlen = get_flen();
	if( rlen < 0 )
	{
		return 0;
	}

	if( type != ID_SURF && type != ID_PART && type != ID_SMGP )
	{
		fp->Seek( cksize - 4, FS_SEEK_CUR );
		return 1;
	}

	while( rlen < cksize )
	{
		i = getVX( fp ) + plist->offset;
		j = getVX( fp ) + tlist->offset;
		rlen = get_flen();
		if( rlen < 0 || rlen > cksize )
		{
			return 0;
		}

		switch( type )
		{
			case ID_SURF:
				plist->pol[ i ].surf.index = j;
				break;
			case ID_PART:
				plist->pol[ i ].part = j;
				break;
			case ID_SMGP:
				plist->pol[ i ].smoothgrp = j;
				break;
		}
	}

	return 1;
}


/*
======================================================================
lwFreePlugin()

Free the memory used by an lwPlugin.
====================================================================== */

void lwFreePlugin( lwPlugin* p )
{
	if( p )
	{
		if( p->ord )
		{
			Mem_Free( p->ord );
		}
		if( p->name )
		{
			Mem_Free( p->name );
		}
		if( p->data )
		{
			Mem_Free( p->data );
		}
		Mem_Free( p );
	}
}


/*
======================================================================
lwFreeTexture()

Free the memory used by an lwTexture.
====================================================================== */

void lwFreeTexture( lwTexture* t )
{
	if( t )
	{
		if( t->ord )
		{
			Mem_Free( t->ord );
		}
		switch( t->type )
		{
			case ID_IMAP:
				if( t->param.imap.vmap_name )
				{
					Mem_Free( t->param.imap.vmap_name );
				}
				break;
			case ID_PROC:
				if( t->param.proc.name )
				{
					Mem_Free( t->param.proc.name );
				}
				if( t->param.proc.data )
				{
					Mem_Free( t->param.proc.data );
				}
				break;
			case ID_GRAD:
				if( t->param.grad.key )
				{
					Mem_Free( t->param.grad.key );
				}
				if( t->param.grad.ikey )
				{
					Mem_Free( t->param.grad.ikey );
				}
				break;
		}
		if( t->tmap.ref_object )
		{
			Mem_Free( t->tmap.ref_object );
		}
		Mem_Free( t );
	}
}


/*
======================================================================
lwFreeSurface()

Free the memory used by an lwSurface.
====================================================================== */

void lwFreeSurface( lwSurface* surf )
{
	if( surf )
	{
		if( surf->name )
		{
			Mem_Free( surf->name );
		}
		if( surf->srcname )
		{
			Mem_Free( surf->srcname );
		}

		lwListFree( surf->shader, ( void ( __cdecl* )( void* ) )lwFreePlugin );

		lwListFree( surf->color.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->luminosity.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->diffuse.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->specularity.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->glossiness.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->reflection.val.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->transparency.val.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->eta.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->translucency.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );
		lwListFree( surf->bump.tex, ( void ( __cdecl* )( void* ) )lwFreeTexture );

		Mem_Free( surf );
	}
}


/*
======================================================================
lwGetTHeader()

Read a texture map header from a SURF.BLOK in an LWO2 file.  This is
the first subchunk in a BLOK, and its contents are common to all three
texture types.
====================================================================== */

int lwGetTHeader( idFile* fp, int hsz, lwTexture* tex )
{
	unsigned int id;
	unsigned short sz;
	int pos, rlen;


	/* remember where we started */

	set_flen( 0 );
	pos = fp->Tell();

	/* ordinal string */

	tex->ord = getS0( fp );

	/* first subchunk header */

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		return 0;
	}

	/* process subchunks as they're encountered */

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_CHAN:
				tex->chan = getU4( fp );
				break;

			case ID_OPAC:
				tex->opac_type = getU2( fp );
				tex->opacity.val = getF4( fp );
				tex->opacity.eindex = getVX( fp );
				break;

			case ID_ENAB:
				tex->enabled = getU2( fp );
				break;

			case ID_NEGA:
				tex->negative = getU2( fp );
				break;

			case ID_AXIS:
				tex->axis = getU2( fp );
				break;

			default:
				break;
		}

		/* error while reading current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			return 0;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the texture header subchunk? */

		if( hsz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			return 0;
		}
	}

	set_flen( fp->Tell() - pos );
	return 1;
}


/*
======================================================================
lwGetTMap()

Read a texture map from a SURF.BLOK in an LWO2 file.  The TMAP
defines the mapping from texture to world or object coordinates.
====================================================================== */

int lwGetTMap( idFile* fp, int tmapsz, lwTMap* tmap )
{
	unsigned int id;
	unsigned short sz;
	int rlen, pos, i;

	pos = fp->Tell();
	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		return 0;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_SIZE:
				for( i = 0; i < 3; i++ )
				{
					tmap->size.val[ i ] = getF4( fp );
				}
				tmap->size.eindex = getVX( fp );
				break;

			case ID_CNTR:
				for( i = 0; i < 3; i++ )
				{
					tmap->center.val[ i ] = getF4( fp );
				}
				tmap->center.eindex = getVX( fp );
				break;

			case ID_ROTA:
				for( i = 0; i < 3; i++ )
				{
					tmap->rotate.val[ i ] = getF4( fp );
				}
				tmap->rotate.eindex = getVX( fp );
				break;

			case ID_FALL:
				tmap->fall_type = getU2( fp );
				for( i = 0; i < 3; i++ )
				{
					tmap->falloff.val[ i ] = getF4( fp );
				}
				tmap->falloff.eindex = getVX( fp );
				break;

			case ID_OREF:
				tmap->ref_object = getS0( fp );
				break;

			case ID_CSYS:
				tmap->coord_sys = getU2( fp );
				break;

			default:
				break;
		}

		/* error while reading the current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			return 0;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the TMAP subchunk? */

		if( tmapsz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			return 0;
		}
	}

	set_flen( fp->Tell() - pos );
	return 1;
}


/*
======================================================================
lwGetImageMap()

Read an lwImageMap from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetImageMap( idFile* fp, int rsz, lwTexture* tex )
{
	unsigned int id;
	unsigned short sz;
	int rlen, pos;

	pos = fp->Tell();
	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		return 0;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_TMAP:
				if( !lwGetTMap( fp, sz, &tex->tmap ) )
				{
					return 0;
				}
				break;

			case ID_PROJ:
				tex->param.imap.projection = getU2( fp );
				break;

			case ID_VMAP:
				tex->param.imap.vmap_name = getS0( fp );
				break;

			case ID_AXIS:
				tex->param.imap.axis = getU2( fp );
				break;

			case ID_IMAG:
				tex->param.imap.cindex = getVX( fp );
				break;

			case ID_WRAP:
				tex->param.imap.wrapw_type = getU2( fp );
				tex->param.imap.wraph_type = getU2( fp );
				break;

			case ID_WRPW:
				tex->param.imap.wrapw.val = getF4( fp );
				tex->param.imap.wrapw.eindex = getVX( fp );
				break;

			case ID_WRPH:
				tex->param.imap.wraph.val = getF4( fp );
				tex->param.imap.wraph.eindex = getVX( fp );
				break;

			case ID_AAST:
				tex->param.imap.aas_flags = getU2( fp );
				tex->param.imap.aa_strength = getF4( fp );
				break;

			case ID_PIXB:
				tex->param.imap.pblend = getU2( fp );
				break;

			case ID_STCK:
				tex->param.imap.stck.val = getF4( fp );
				tex->param.imap.stck.eindex = getVX( fp );
				break;

			case ID_TAMP:
				tex->param.imap.amplitude.val = getF4( fp );
				tex->param.imap.amplitude.eindex = getVX( fp );
				break;

			default:
				break;
		}

		/* error while reading the current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			return 0;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the image map? */

		if( rsz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			return 0;
		}
	}

	set_flen( fp->Tell() - pos );
	return 1;
}


/*
======================================================================
lwGetProcedural()

Read an lwProcedural from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetProcedural( idFile* fp, int rsz, lwTexture* tex )
{
	unsigned int id;
	unsigned short sz;
	int rlen, pos;

	pos = fp->Tell();
	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		return 0;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_TMAP:
				if( !lwGetTMap( fp, sz, &tex->tmap ) )
				{
					return 0;
				}
				break;

			case ID_AXIS:
				tex->param.proc.axis = getU2( fp );
				break;

			case ID_VALU:
				tex->param.proc.value[ 0 ] = getF4( fp );
				if( sz >= 8 )
				{
					tex->param.proc.value[ 1 ] = getF4( fp );
				}
				if( sz >= 12 )
				{
					tex->param.proc.value[ 2 ] = getF4( fp );
				}
				break;

			case ID_FUNC:
				tex->param.proc.name = getS0( fp );
				rlen = get_flen();
				tex->param.proc.data = getbytes( fp, sz - rlen );
				break;

			default:
				break;
		}

		/* error while reading the current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			return 0;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the procedural block? */

		if( rsz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			return 0;
		}
	}

	set_flen( fp->Tell() - pos );
	return 1;
}


/*
======================================================================
lwGetGradient()

Read an lwGradient from a SURF.BLOK in an LWO2 file.
====================================================================== */

int lwGetGradient( idFile* fp, int rsz, lwTexture* tex )
{
	unsigned int id;
	unsigned short sz;
	int rlen, pos, i, j, nkeys;

	pos = fp->Tell();
	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		return 0;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_TMAP:
				if( !lwGetTMap( fp, sz, &tex->tmap ) )
				{
					return 0;
				}
				break;

			case ID_PNAM:
				tex->param.grad.paramname = getS0( fp );
				break;

			case ID_INAM:
				tex->param.grad.itemname = getS0( fp );
				break;

			case ID_GRST:
				tex->param.grad.start = getF4( fp );
				break;

			case ID_GREN:
				tex->param.grad.end = getF4( fp );
				break;

			case ID_GRPT:
				tex->param.grad.repeat = getU2( fp );
				break;

			case ID_FKEY:
				nkeys = sz / sizeof( lwGradKey );
				tex->param.grad.key = ( lwGradKey* )Mem_ClearedAlloc( nkeys * sizeof( lwGradKey ), TAG_MODEL );
				if( !tex->param.grad.key )
				{
					return 0;
				}
				for( i = 0; i < nkeys; i++ )
				{
					tex->param.grad.key[ i ].value = getF4( fp );
					for( j = 0; j < 4; j++ )
					{
						tex->param.grad.key[ i ].rgba[ j ] = getF4( fp );
					}
				}
				break;

			case ID_IKEY:
				nkeys = sz / 2;
				tex->param.grad.ikey = ( short* )Mem_ClearedAlloc( nkeys * sizeof( short ), TAG_MODEL );
				if( !tex->param.grad.ikey )
				{
					return 0;
				}
				for( i = 0; i < nkeys; i++ )
				{
					tex->param.grad.ikey[ i ] = getU2( fp );
				}
				break;

			default:
				break;
		}

		/* error while reading the current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			return 0;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the gradient? */

		if( rsz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			return 0;
		}
	}

	set_flen( fp->Tell() - pos );
	return 1;
}


/*
======================================================================
lwGetTexture()

Read an lwTexture from a SURF.BLOK in an LWO2 file.
====================================================================== */

lwTexture* lwGetTexture( idFile* fp, int bloksz, unsigned int type )
{
	lwTexture* tex;
	unsigned short sz;
	int ok;

	tex = ( lwTexture* )Mem_ClearedAlloc( sizeof( lwTexture ), TAG_MODEL );
	if( !tex )
	{
		return NULL;
	}

	tex->type = type;
	tex->tmap.size.val[ 0 ] =
		tex->tmap.size.val[ 1 ] =
			tex->tmap.size.val[ 2 ] = 1.0f;
	tex->opacity.val = 1.0f;
	tex->enabled = 1;

	sz = getU2( fp );
	if( !lwGetTHeader( fp, sz, tex ) )
	{
		Mem_Free( tex );
		return NULL;
	}

	sz = bloksz - sz - 6;
	switch( type )
	{
		case ID_IMAP:
			ok = lwGetImageMap( fp, sz, tex );
			break;
		case ID_PROC:
			ok = lwGetProcedural( fp, sz, tex );
			break;
		case ID_GRAD:
			ok = lwGetGradient( fp, sz, tex );
			break;
		default:
			ok = !fp->Seek( sz, FS_SEEK_CUR );
	}

	if( !ok )
	{
		lwFreeTexture( tex );
		return NULL;
	}

	set_flen( bloksz );
	return tex;
}


/*
======================================================================
lwGetShader()

Read a shader record from a SURF.BLOK in an LWO2 file.
====================================================================== */

lwPlugin* lwGetShader( idFile* fp, int bloksz )
{
	lwPlugin* shdr;
	unsigned int id;
	unsigned short sz;
	int hsz, rlen, pos;

	shdr = ( lwPlugin* )Mem_ClearedAlloc( sizeof( lwPlugin ), TAG_MODEL );
	if( !shdr )
	{
		return NULL;
	}

	pos = fp->Tell();
	set_flen( 0 );
	hsz = getU2( fp );
	shdr->ord = getS0( fp );
	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	while( hsz > 0 )
	{
		sz += sz & 1;
		hsz -= sz;
		if( id == ID_ENAB )
		{
			shdr->flags = getU2( fp );
			break;
		}
		else
		{
			fp->Seek( sz, FS_SEEK_CUR );
			id = getU4( fp );
			sz = getU2( fp );
		}
	}

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_FUNC:
				shdr->name = getS0( fp );
				rlen = get_flen();
				shdr->data = getbytes( fp, sz - rlen );
				break;

			default:
				break;
		}

		/* error while reading the current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			goto Fail;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the shader block? */

		if( bloksz <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			goto Fail;
		}
	}

	set_flen( fp->Tell() - pos );
	return shdr;

Fail:
	lwFreePlugin( shdr );
	return NULL;
}


/*
======================================================================
compare_textures()
compare_shaders()

Callbacks for the lwListInsert() function, which is called to add
textures to surface channels and shaders to surfaces.
====================================================================== */

static int compare_textures( lwTexture* a, lwTexture* b )
{
	return strcmp( a->ord, b->ord );
}


static int compare_shaders( lwPlugin* a, lwPlugin* b )
{
	return strcmp( a->ord, b->ord );
}


/*
======================================================================
add_texture()

Finds the surface channel (lwTParam or lwCParam) to which a texture is
applied, then calls lwListInsert().
====================================================================== */

static int add_texture( lwSurface* surf, lwTexture* tex )
{
	lwTexture** list;

	switch( tex->chan )
	{
		case ID_COLR:
			list = &surf->color.tex;
			break;
		case ID_LUMI:
			list = &surf->luminosity.tex;
			break;
		case ID_DIFF:
			list = &surf->diffuse.tex;
			break;
		case ID_SPEC:
			list = &surf->specularity.tex;
			break;
		case ID_GLOS:
			list = &surf->glossiness.tex;
			break;
		case ID_REFL:
			list = &surf->reflection.val.tex;
			break;
		case ID_TRAN:
			list = &surf->transparency.val.tex;
			break;
		case ID_RIND:
			list = &surf->eta.tex;
			break;
		case ID_TRNL:
			list = &surf->translucency.tex;
			break;
		case ID_BUMP:
			list = &surf->bump.tex;
			break;
		default:
			return 0;
	}

	lwListInsert( ( void** )list, tex, ( int ( __cdecl* )( void*, void* ) )compare_textures );
	return 1;
}


/*
======================================================================
lwDefaultSurface()

Allocate and initialize a surface.
====================================================================== */

lwSurface* lwDefaultSurface()
{
	lwSurface* surf;

	surf = ( lwSurface* )Mem_ClearedAlloc( sizeof( lwSurface ), TAG_MODEL );
	if( !surf )
	{
		return NULL;
	}

	surf->color.rgb[ 0 ] = 0.78431f;
	surf->color.rgb[ 1 ] = 0.78431f;
	surf->color.rgb[ 2 ] = 0.78431f;
	surf->diffuse.val    = 1.0f;
	surf->glossiness.val = 0.4f;
	surf->bump.val       = 1.0f;
	surf->eta.val        = 1.0f;
	surf->sideflags      = 1;

	return surf;
}


/*
======================================================================
lwGetSurface()

Read an lwSurface from an LWO2 file.
====================================================================== */

lwSurface* lwGetSurface( idFile* fp, int cksize )
{
	lwSurface* surf;
	lwTexture* tex;
	lwPlugin* shdr;
	unsigned int id, type;
	unsigned short sz;
	int pos, rlen;


	/* allocate the Surface structure */

	surf = ( lwSurface* )Mem_ClearedAlloc( sizeof( lwSurface ), TAG_MODEL );
	if( !surf )
	{
		goto Fail;
	}

	/* non-zero defaults */

	surf->color.rgb[ 0 ] = 0.78431f;
	surf->color.rgb[ 1 ] = 0.78431f;
	surf->color.rgb[ 2 ] = 0.78431f;
	surf->diffuse.val    = 1.0f;
	surf->glossiness.val = 0.4f;
	surf->bump.val       = 1.0f;
	surf->eta.val        = 1.0f;
	surf->sideflags      = 1;

	/* remember where we started */

	set_flen( 0 );
	pos = fp->Tell();

	/* names */

	surf->name = getS0( fp );
	surf->srcname = getS0( fp );

	/* first subchunk header */

	id = getU4( fp );
	sz = getU2( fp );
	if( 0 > get_flen() )
	{
		goto Fail;
	}

	/* process subchunks as they're encountered */

	while( 1 )
	{
		sz += sz & 1;
		set_flen( 0 );

		switch( id )
		{
			case ID_COLR:
				surf->color.rgb[ 0 ] = getF4( fp );
				surf->color.rgb[ 1 ] = getF4( fp );
				surf->color.rgb[ 2 ] = getF4( fp );
				surf->color.eindex = getVX( fp );
				break;

			case ID_LUMI:
				surf->luminosity.val = getF4( fp );
				surf->luminosity.eindex = getVX( fp );
				break;

			case ID_DIFF:
				surf->diffuse.val = getF4( fp );
				surf->diffuse.eindex = getVX( fp );
				break;

			case ID_SPEC:
				surf->specularity.val = getF4( fp );
				surf->specularity.eindex = getVX( fp );
				break;

			case ID_GLOS:
				surf->glossiness.val = getF4( fp );
				surf->glossiness.eindex = getVX( fp );
				break;

			case ID_REFL:
				surf->reflection.val.val = getF4( fp );
				surf->reflection.val.eindex = getVX( fp );
				break;

			case ID_RFOP:
				surf->reflection.options = getU2( fp );
				break;

			case ID_RIMG:
				surf->reflection.cindex = getVX( fp );
				break;

			case ID_RSAN:
				surf->reflection.seam_angle = getF4( fp );
				break;

			case ID_TRAN:
				surf->transparency.val.val = getF4( fp );
				surf->transparency.val.eindex = getVX( fp );
				break;

			case ID_TROP:
				surf->transparency.options = getU2( fp );
				break;

			case ID_TIMG:
				surf->transparency.cindex = getVX( fp );
				break;

			case ID_RIND:
				surf->eta.val = getF4( fp );
				surf->eta.eindex = getVX( fp );
				break;

			case ID_TRNL:
				surf->translucency.val = getF4( fp );
				surf->translucency.eindex = getVX( fp );
				break;

			case ID_BUMP:
				surf->bump.val = getF4( fp );
				surf->bump.eindex = getVX( fp );
				break;

			case ID_SMAN:
				surf->smooth = getF4( fp );
				break;

			case ID_SIDE:
				surf->sideflags = getU2( fp );
				break;

			case ID_CLRH:
				surf->color_hilite.val = getF4( fp );
				surf->color_hilite.eindex = getVX( fp );
				break;

			case ID_CLRF:
				surf->color_filter.val = getF4( fp );
				surf->color_filter.eindex = getVX( fp );
				break;

			case ID_ADTR:
				surf->add_trans.val = getF4( fp );
				surf->add_trans.eindex = getVX( fp );
				break;

			case ID_SHRP:
				surf->dif_sharp.val = getF4( fp );
				surf->dif_sharp.eindex = getVX( fp );
				break;

			case ID_GVAL:
				surf->glow.val = getF4( fp );
				surf->glow.eindex = getVX( fp );
				break;

			case ID_LINE:
				surf->line.enabled = 1;
				if( sz >= 2 )
				{
					surf->line.flags = getU2( fp );
				}
				if( sz >= 6 )
				{
					surf->line.size.val = getF4( fp );
				}
				if( sz >= 8 )
				{
					surf->line.size.eindex = getVX( fp );
				}
				break;

			case ID_ALPH:
				surf->alpha_mode = getU2( fp );
				surf->alpha = getF4( fp );
				break;

			case ID_AVAL:
				surf->alpha = getF4( fp );
				break;

			case ID_BLOK:
				type = getU4( fp );

				switch( type )
				{
					case ID_IMAP:
					case ID_PROC:
					case ID_GRAD:
						tex = lwGetTexture( fp, sz - 4, type );
						if( !tex )
						{
							goto Fail;
						}
						if( !add_texture( surf, tex ) )
						{
							lwFreeTexture( tex );
						}
						set_flen( 4 + get_flen() );
						break;
					case ID_SHDR:
						shdr = lwGetShader( fp, sz - 4 );
						if( !shdr )
						{
							goto Fail;
						}
						lwListInsert( ( void** )&surf->shader, shdr, ( int ( __cdecl* )( void*, void* ) )compare_shaders );
						++surf->nshaders;
						set_flen( 4 + get_flen() );
						break;
				}
				break;

			default:
				break;
		}

		/* error while reading current subchunk? */

		rlen = get_flen();
		if( rlen < 0 || rlen > sz )
		{
			goto Fail;
		}

		/* skip unread parts of the current subchunk */

		if( rlen < sz )
		{
			fp->Seek( sz - rlen, FS_SEEK_CUR );
		}

		/* end of the SURF chunk? */

		if( cksize <= fp->Tell() - pos )
		{
			break;
		}

		/* get the next subchunk header */

		set_flen( 0 );
		id = getU4( fp );
		sz = getU2( fp );
		if( 6 != get_flen() )
		{
			goto Fail;
		}
	}

	return surf;

Fail:
	if( surf )
	{
		lwFreeSurface( surf );
	}
	return NULL;
}


float dot( float a[], float b[] )
{
	return a[ 0 ] * b[ 0 ] + a[ 1 ] * b[ 1 ] + a[ 2 ] * b[ 2 ];
}


void cross( float a[], float b[], float c[] )
{
	c[ 0 ] = a[ 1 ] * b[ 2 ] - a[ 2 ] * b[ 1 ];
	c[ 1 ] = a[ 2 ] * b[ 0 ] - a[ 0 ] * b[ 2 ];
	c[ 2 ] = a[ 0 ] * b[ 1 ] - a[ 1 ] * b[ 0 ];
}


void normalize( float v[] )
{
	float r;

	r = ( float ) idMath::Sqrt( dot( v, v ) );
	if( r > 0 )
	{
		v[ 0 ] /= r;
		v[ 1 ] /= r;
		v[ 2 ] /= r;
	}
}

/*
======================================================================
lwFreeVMap()

Free memory used by an lwVMap.
====================================================================== */

void lwFreeVMap( lwVMap* vmap )
{
	if( vmap )
	{
		if( vmap->name )
		{
			Mem_Free( vmap->name );
		}
		if( vmap->vindex )
		{
			Mem_Free( vmap->vindex );
		}
		if( vmap->pindex )
		{
			Mem_Free( vmap->pindex );
		}
		if( vmap->val )
		{
			if( vmap->val[ 0 ] )
			{
				Mem_Free( vmap->val[ 0 ] );
			}
			Mem_Free( vmap->val );
		}
		Mem_Free( vmap );
	}
}


/*
======================================================================
lwGetVMap()

Read an lwVMap from a VMAP or VMAD chunk in an LWO2.
====================================================================== */

lwVMap* lwGetVMap( idFile* fp, int cksize, int ptoffset, int poloffset,
				   int perpoly )
{
	unsigned char* buf, *bp;
	lwVMap* vmap;
	float* f;
	int i, j, npts, rlen;


	/* read the whole chunk */

	set_flen( 0 );
	buf = ( unsigned char* )getbytes( fp, cksize );
	if( !buf )
	{
		return NULL;
	}

	vmap = ( lwVMap* )Mem_ClearedAlloc( sizeof( lwVMap ), TAG_MODEL );
	if( !vmap )
	{
		Mem_Free( buf );
		return NULL;
	}

	/* initialize the vmap */

	vmap->perpoly = perpoly;

	bp = buf;
	set_flen( 0 );
	vmap->type = sgetU4( &bp );
	vmap->dim  = sgetU2( &bp );
	vmap->name = sgetS0( &bp );
	rlen = get_flen();

	/* count the vmap records */

	npts = 0;
	while( bp < buf + cksize )
	{
		i = sgetVX( &bp );
		if( perpoly )
		{
			i = sgetVX( &bp );
		}
		bp += vmap->dim * sizeof( float );
		++npts;
	}

	/* allocate the vmap */

	vmap->nverts = npts;
	vmap->vindex = ( int* )Mem_ClearedAlloc( npts * sizeof( int ), TAG_MODEL );
	if( !vmap->vindex )
	{
		goto Fail;
	}
	if( perpoly )
	{
		vmap->pindex = ( int* )Mem_ClearedAlloc( npts * sizeof( int ), TAG_MODEL );
		if( !vmap->pindex )
		{
			goto Fail;
		}
	}

	if( vmap->dim > 0 )
	{
		vmap->val = ( float** )Mem_ClearedAlloc( npts * sizeof( float* ), TAG_MODEL );
		if( !vmap->val )
		{
			goto Fail;
		}
		f = ( float* )Mem_ClearedAlloc( npts * vmap->dim * sizeof( float ), TAG_MODEL );
		if( !f )
		{
			goto Fail;
		}
		for( i = 0; i < npts; i++ )
		{
			vmap->val[ i ] = f + i * vmap->dim;
		}
	}

	/* fill in the vmap values */

	bp = buf + rlen;
	for( i = 0; i < npts; i++ )
	{
		vmap->vindex[ i ] = sgetVX( &bp );
		if( perpoly )
		{
			vmap->pindex[ i ] = sgetVX( &bp );
		}
		for( j = 0; j < vmap->dim; j++ )
		{
			vmap->val[ i ][ j ] = sgetF4( &bp );
		}
	}

	Mem_Free( buf );
	return vmap;

Fail:
	if( buf )
	{
		Mem_Free( buf );
	}
	lwFreeVMap( vmap );
	return NULL;
}


/*
======================================================================
lwGetPointVMaps()

Fill in the lwVMapPt structure for each point.
====================================================================== */

int lwGetPointVMaps( lwPointList* point, lwVMap* vmap )
{
	lwVMap* vm;
	int i, j, n;

	/* count the number of vmap values for each point */

	vm = vmap;
	while( vm )
	{
		if( !vm->perpoly )
			for( i = 0; i < vm->nverts; i++ )
			{
				++point->pt[ vm->vindex[ i ]].nvmaps;
			}
		vm = vm->next;
	}

	/* allocate vmap references for each mapped point */

	for( i = 0; i < point->count; i++ )
	{
		if( point->pt[ i ].nvmaps )
		{
			point->pt[ i ].vm = ( lwVMapPt* )Mem_ClearedAlloc( point->pt[ i ].nvmaps * sizeof( lwVMapPt ), TAG_MODEL );
			if( !point->pt[ i ].vm )
			{
				return 0;
			}
			point->pt[ i ].nvmaps = 0;
		}
	}

	/* fill in vmap references for each mapped point */

	vm = vmap;
	while( vm )
	{
		if( !vm->perpoly )
		{
			for( i = 0; i < vm->nverts; i++ )
			{
				j = vm->vindex[ i ];
				n = point->pt[ j ].nvmaps;
				point->pt[ j ].vm[ n ].vmap = vm;
				point->pt[ j ].vm[ n ].index = i;
				++point->pt[ j ].nvmaps;
			}
		}
		vm = vm->next;
	}

	return 1;
}


/*
======================================================================
lwGetPolyVMaps()

Fill in the lwVMapPt structure for each polygon vertex.
====================================================================== */

int lwGetPolyVMaps( lwPolygonList* polygon, lwVMap* vmap )
{
	lwVMap* vm;
	lwPolVert* pv;
	int i, j;

	/* count the number of vmap values for each polygon vertex */

	vm = vmap;
	while( vm )
	{
		if( vm->perpoly )
		{
			for( i = 0; i < vm->nverts; i++ )
			{
				for( j = 0; j < polygon->pol[ vm->pindex[ i ]].nverts; j++ )
				{
					pv = &polygon->pol[ vm->pindex[ i ]].v[ j ];
					if( vm->vindex[ i ] == pv->index )
					{
						++pv->nvmaps;
						break;
					}
				}
			}
		}
		vm = vm->next;
	}

	/* allocate vmap references for each mapped vertex */

	for( i = 0; i < polygon->count; i++ )
	{
		for( j = 0; j < polygon->pol[ i ].nverts; j++ )
		{
			pv = &polygon->pol[ i ].v[ j ];
			if( pv->nvmaps )
			{
				pv->vm = ( lwVMapPt* )Mem_ClearedAlloc( pv->nvmaps * sizeof( lwVMapPt ), TAG_MODEL );
				if( !pv->vm )
				{
					return 0;
				}
				pv->nvmaps = 0;
			}
		}
	}

	/* fill in vmap references for each mapped point */

	vm = vmap;
	while( vm )
	{
		if( vm->perpoly )
		{
			for( i = 0; i < vm->nverts; i++ )
			{
				for( j = 0; j < polygon->pol[ vm->pindex[ i ]].nverts; j++ )
				{
					pv = &polygon->pol[ vm->pindex[ i ]].v[ j ];
					if( vm->vindex[ i ] == pv->index )
					{
						pv->vm[ pv->nvmaps ].vmap = vm;
						pv->vm[ pv->nvmaps ].index = i;
						++pv->nvmaps;
						break;
					}
				}
			}
		}
		vm = vm->next;
	}

	return 1;
}

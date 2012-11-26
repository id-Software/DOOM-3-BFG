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

#include "../idlib/precompiled.h"
#pragma hdrstop

/*
================================================================================================
Contains external code for building ZipFiles.
================================================================================================
*/

#include "Zip.h"
#include "Unzip.h"

#undef STDC
#include "zlib/zutil.h"

/* zip.c -- IO on .zip files using zlib
   Version 1.01e, February 12th, 2005

   27 Dec 2004 Rolf Kalbermatter
   Modification to zipOpen2 to support globalComment retrieval.

   Copyright (C) 1998-2005 Gilles Vollant

   This unzip package allow creates .ZIP file, compatible with PKZip 2.04g
   WinZip, InfoZip tools and compatible.
   Multi volume ZipFile (span) are not supported.
   Encryption compatible with pkzip 2.04g only supported
   Old compressions used by old PKZip 1.x are not supported

   For uncompress .zip file, look at unzip.h

   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.html for evolution

   Condition of use and distribution are the same than zlib :

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   For more info about .ZIP format, see
   http://www.info-zip.org/pub/infozip/doc/appnote-981119-iz.zip
   http://www.info-zip.org/pub/infozip/doc/
   PkWare has also a specification at :
   ftp://ftp.pkware.com/probdesc.zip

*/

#ifndef Z_MAXFILENAMEINZIP
#define Z_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
#define ALLOC( size ) ( Mem_Alloc( size, TAG_ZIP ) )
#endif
#ifndef TRYFREE
#define TRYFREE( p ) { if (p) { Mem_Free(p); } }
#endif

/*
#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)
*/

const char zip_copyright[] =
   " zip 1.01 Copyright 1998-2004 Gilles Vollant - http://www.winimage.com/zLibDll";

#define LOCALHEADERMAGIC		(0x04034b50)
#define CENTRALHEADERMAGIC		(0x02014b50)
#define ENDHEADERMAGIC			(0x06054b50)

#define FLAG_LOCALHEADER_OFFSET (0x06)
#define CRC_LOCALHEADER_OFFSET  (0x0e)

#define SIZECENTRALHEADER		(0x2e)	/* 46 */

#define	DEFAULT_COMPRESSION_LEVEL	(5)	/* 1 == Compress faster, 9 == Compress better */
#define DEFAULT_WRITEBUFFERSIZE (16384)

#ifndef NOCRYPT
#define INCLUDECRYPTINGCODE_IFCRYPTALLOWED
#include "crypt.h"
#endif

idCVar zip_verbosity( "zip_verbosity", "0", CVAR_BOOL, "1 = verbose logging when building zip files" );

/*
========================
allocate_new_datablock
========================
*/
linkedlist_datablock_internal* allocate_new_datablock() {
    linkedlist_datablock_internal* ldi = NULL;
    ldi = (linkedlist_datablock_internal*) ALLOC( sizeof( linkedlist_datablock_internal ) );
    if ( ldi != NULL ) {
        ldi->next_datablock = NULL;
        ldi->filled_in_this_block = 0;
        ldi->avail_in_this_block = SIZEDATA_INDATABLOCK;
    }
    return ldi;
}

/*
========================
free_datablock
========================
*/
void free_datablock( linkedlist_datablock_internal* ldi ) {
    while ( ldi != NULL ) {
        linkedlist_datablock_internal* ldinext = ldi->next_datablock;
        TRYFREE( ldi );
        ldi = ldinext;
    }
}

/*
========================
init_linkedlist
========================
*/
void init_linkedlist( linkedlist_data* ll ) {
    ll->first_block = ll->last_block = NULL;
}

/*
========================
free_linkedlist
========================
*/
void free_linkedlist( linkedlist_data* ll ) {
    free_datablock( ll->first_block );
    ll->first_block = ll->last_block = NULL;
}

/*
========================
add_data_in_datablock
========================
*/
int add_data_in_datablock( linkedlist_data* ll, const void* buf, unsigned long len ) {
    linkedlist_datablock_internal* ldi;
    const unsigned char* from_copy;

	if ( ll == NULL ) {
        return ZIP_INTERNALERROR;
	}

    if ( ll->last_block == NULL ) {
        ll->first_block = ll->last_block = allocate_new_datablock();
		if ( ll->first_block == NULL ) {
            return ZIP_INTERNALERROR;
		}
    }

    ldi = ll->last_block;
    from_copy = (unsigned char*)buf;

    while ( len > 0 ) {
        unsigned int copy_this;
        unsigned char* to_copy;

        if ( ldi->avail_in_this_block == 0 ) {
            ldi->next_datablock = allocate_new_datablock();
			if ( ldi->next_datablock == NULL ) {
                return ZIP_INTERNALERROR;
			}
            ldi = ldi->next_datablock;
            ll->last_block = ldi;
        }

		if ( ldi->avail_in_this_block < len ) {
            copy_this = (unsigned int)ldi->avail_in_this_block;
		} else {
            copy_this = (unsigned int)len;
		}

        to_copy = &( ldi->data[ ldi->filled_in_this_block ] );

		for ( unsigned int i = 0; i < copy_this; i++ ) {
            *( to_copy + i ) =* ( from_copy + i );
		}

        ldi->filled_in_this_block += copy_this;
        ldi->avail_in_this_block -= copy_this;
        from_copy += copy_this;
        len -= copy_this;
    }

    return ZIP_OK;
}

#ifndef NO_ADDFILEINEXISTINGZIP

/*
========================
ziplocal_putValue

Inputs a long in LSB order to the given file
nbByte == 1, 2 or 4 (byte, short or long)
========================
*/
int ziplocal_putValue( idFile* filestream, unsigned long x, int nbByte ) {
    unsigned char buf[4];
    for ( int n = 0; n < nbByte; n++ ) {
        buf[n] = (unsigned char)( x & 0xff );
        x >>= 8;
    }
    if ( x != 0 ) {     
		/* data overflow - hack for ZIP64 (X Roche) */
		for ( int n = 0; n < nbByte; n++ ) {
          buf[n] = 0xff;
        }
    }
	if ( filestream->Write( buf, nbByte ) != nbByte ) {
		return ZIP_ERRNO;
	} else {
		return ZIP_OK;
	}
}

/*
========================
ziplocal_putValue_inmemory
========================
*/
void ziplocal_putValue_inmemory( void* dest, unsigned long x, int nbByte ){
    unsigned char* buf = (unsigned char*)dest;
    for ( int n = 0; n < nbByte; n++ ) {
        buf[n] = (unsigned char)( x & 0xff );
        x >>= 8;
    }

    if ( x != 0 ) {     
		/* data overflow - hack for ZIP64 */
		for ( int n = 0; n < nbByte; n++ ) {
          buf[n] = 0xff;
       }
    }
}

/*
========================
ziplocal_TmzDateToDosDate
========================
*/
unsigned long ziplocal_TmzDateToDosDate( const tm_zip* ptm, unsigned long dosDate ) {
    unsigned long year = (unsigned long)ptm->tm_year;
	if ( year > 1980 ) {
        year-=1980;
	} else if ( year > 80 ) {
        year -= 80;
	}
    return (unsigned long)( ( ( ptm->tm_mday ) + ( 32 * ( ptm->tm_mon + 1 ) ) + ( 512 * year ) ) << 16 ) |
			( ( ptm->tm_sec / 2 ) + ( 32 * ptm->tm_min ) + ( 2048 * (unsigned long)ptm->tm_hour ) );
}

/*
========================
ziplocal_getByte
========================
*/
int ziplocal_getByte( idFile* filestream, int *pi ) {
	unsigned char c;
	int err = (int)filestream->Read( &c, 1 );
	if ( err == 1 )	{
		*pi = (int)c;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
/*
    unsigned char c;
    int err = (int)ZREAD( filestream, &c, 1 );

    if ( err == 1 )	{
        *pi = (int)c;
        return ZIP_OK;
    } else {
		if ( ZERROR( filestream ) ) {
			return ZIP_ERRNO;
		} else {
            return ZIP_EOF;
		}
    }
*/
}

/*
========================
ziplocal_getShort

Reads a long in LSB order from the given gz_stream. Sets
========================
*/
int ziplocal_getShort( idFile* filestream, unsigned long *pX ) {
	short v;
	if ( filestream->Read( &v, sizeof( v ) ) == sizeof( v ) ) {
		idSwap::Little( v );
		*pX = v;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
/*
    unsigned long x ;
    int i;
    int err;

    err = ziplocal_getByte( filestream, &i );
    x = (unsigned long)i;

	if ( err == ZIP_OK ) {
        err = ziplocal_getByte( filestream, &i );
	}
    x += ( (unsigned long)i ) << 8;

	if ( err == ZIP_OK ) {
        *pX = x;
	} else {
        *pX = 0;
	}
    return err;
*/
}

/*
========================
ziplocal_getLong
========================
*/
int ziplocal_getLong( idFile* filestream, unsigned long *pX ) {
	int v;
	if ( filestream->Read( &v, sizeof( v ) ) == sizeof( v ) ) {
		idSwap::Little( v );
		*pX = v;
		return ZIP_OK;
	} else {
		return ZIP_ERRNO;
	}
/*
    unsigned long x ;
    int i;
    int err;

    err = ziplocal_getByte( filestream, &i );
    x = (unsigned long)i;

	if ( err == ZIP_OK ) {
        err = ziplocal_getByte( filestream, &i );
	}
    x += ( (unsigned long)i ) << 8;

	if ( err == ZIP_OK ) {
        err = ziplocal_getByte( filestream, &i );
	}
    x += ( (unsigned long)i) << 16;

	if ( err == ZIP_OK ) {
        err = ziplocal_getByte( filestream, &i );
	}
    x += ( (unsigned long)i ) << 24;

	if ( err == ZIP_OK ) {
        *pX = x;
	} else {
        *pX = 0;
	}
    return err;
*/
}

#ifndef BUFREADCOMMENT
#define BUFREADCOMMENT (0x400)
#endif

/*
========================
ziplocal_SearchCentralDir

Locate the Central directory of a zipfile (at the end, just before
the global comment)
========================
*/
unsigned long ziplocal_SearchCentralDir( idFile* filestream ) {
    unsigned char* buf;
    unsigned long uSizeFile;
    unsigned long uBackRead;
    unsigned long uMaxBack = 0xffff; /* maximum size of global comment */
    unsigned long uPosFound = 0;

	if ( filestream->Seek( 0, FS_SEEK_END ) != 0 ) {
        return 0;
	}

    uSizeFile = (unsigned long)filestream->Tell();

	if ( uMaxBack > uSizeFile ) {
        uMaxBack = uSizeFile;
	}

    buf = (unsigned char*)ALLOC( BUFREADCOMMENT + 4 );
	if ( buf == NULL ) {
        return 0;
	}

    uBackRead = 4;
    while ( uBackRead < uMaxBack ) {
        unsigned long uReadSize,uReadPos;
		if ( uBackRead + BUFREADCOMMENT > uMaxBack ) {
            uBackRead = uMaxBack;
		} else {
            uBackRead += BUFREADCOMMENT;
		}
        uReadPos = uSizeFile - uBackRead ;

        uReadSize = ( ( BUFREADCOMMENT + 4 ) < ( uSizeFile - uReadPos ) ) ? ( BUFREADCOMMENT + 4 ) : ( uSizeFile - uReadPos );
		if ( filestream->Seek( uReadPos, FS_SEEK_SET ) != 0 ) {
            break;
		}

		if ( filestream->Read( buf, uReadSize ) != ( int )uReadSize ) {
            break;
		}

		for ( int i = (int)uReadSize - 3; ( i -- ) > 0; ) {
            if ( ( ( *( buf + i ) ) == 0x50 ) && ( ( *( buf + i + 1 ) ) == 0x4b ) && ( ( *( buf + i + 2 ) ) == 0x05 ) && ( ( *( buf + i + 3 ) ) == 0x06 ) ) {
                uPosFound = uReadPos + i;
                break;
            }
		}

		if ( uPosFound != 0 ) {
            break;
		}
    }
    TRYFREE( buf );
    return uPosFound;
}
#endif /* !NO_ADDFILEINEXISTINGZIP*/

/*
========================
zipOpen2
========================
*/
zipFile zipOpen2( const char *pathname, int append, char* globalcomment ) {
    zip_internal ziinit;
    zip_internal* zi;
    int err = ZIP_OK;

	ziinit.filestream = fileSystem->OpenExplicitFileWrite( pathname );
/*
	ziinit.filestream = ZOPEN( pathname, ( append == APPEND_STATUS_CREATE ) ?
								( ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_CREATE ) :
								( ZLIB_FILEFUNC_MODE_READ | ZLIB_FILEFUNC_MODE_WRITE | ZLIB_FILEFUNC_MODE_EXISTING ) );
*/
	if ( ziinit.filestream == NULL ) {
        return NULL;
	}
	ziinit.begin_pos = (unsigned long)ziinit.filestream->Tell();
    ziinit.in_opened_file_inzip = 0;
    ziinit.ci.stream_initialised = 0;
    ziinit.number_entry = 0;
    ziinit.add_position_when_writting_offset = 0;
    init_linkedlist( &(ziinit.central_dir) );

    zi = (zip_internal*)ALLOC( sizeof( zip_internal ) );
    if ( zi == NULL ) {
		delete ziinit.filestream;
		ziinit.filestream = NULL;
        return NULL;
    }

    /* now we add file in a zipfile */
#ifndef NO_ADDFILEINEXISTINGZIP
    ziinit.globalcomment = NULL;
    if ( append == APPEND_STATUS_ADDINZIP ) {
        unsigned long byte_before_the_zipfile;	// byte before the zipfile, ( > 0 for sfx )
        unsigned long size_central_dir;			// size of the central directory
        unsigned long offset_central_dir;		// offset of start of central directory
        unsigned long central_pos,uL;
        unsigned long number_disk;				// number of the current dist, used for spaning ZIP, unsupported, always 0
        unsigned long number_disk_with_CD;		// number the the disk with central dir, used for spaning ZIP, unsupported, always 0
        unsigned long number_entry;
        unsigned long number_entry_CD;			// total number of entries in the central dir ( same than number_entry on nospan )
        unsigned long size_comment;

        central_pos = ziplocal_SearchCentralDir( ziinit.filestream );
		if ( central_pos == 0 ) {
            err = ZIP_ERRNO;
		}

		if ( ziinit.filestream->Seek( central_pos, FS_SEEK_SET ) != 0 ) {
			err = ZIP_ERRNO;
		}

		/* the signature, already checked */
		if ( ziplocal_getLong( ziinit.filestream, &uL ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* number of this disk */
		if ( ziplocal_getShort( ziinit.filestream, &number_disk ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* number of the disk with the start of the central directory */
		if ( ziplocal_getShort( ziinit.filestream, &number_disk_with_CD ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* total number of entries in the central dir on this disk */
		if ( ziplocal_getShort( ziinit.filestream, &number_entry ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* total number of entries in the central dir */
		if ( ziplocal_getShort( ziinit.filestream, &number_entry_CD ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

		if ( ( number_entry_CD != number_entry ) || ( number_disk_with_CD != 0 ) || ( number_disk != 0 ) ) {
            err = ZIP_BADZIPFILE;
		}

        /* size of the central directory */
		if ( ziplocal_getLong( ziinit.filestream, &size_central_dir ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* offset of start of central directory with respect to the	starting disk number */
		if ( ziplocal_getLong( ziinit.filestream, &offset_central_dir ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

        /* zipfile global comment length */
		if ( ziplocal_getShort( ziinit.filestream, &size_comment ) != ZIP_OK ) {
            err = ZIP_ERRNO;
		}

		if ( ( central_pos < ( offset_central_dir + size_central_dir ) ) && ( err == ZIP_OK ) ) {
            err = ZIP_BADZIPFILE;
		}

        if ( err != ZIP_OK ) {
			delete ziinit.filestream;
            ziinit.filestream = NULL;
            return NULL;
        }

        if ( size_comment > 0 ) {
            ziinit.globalcomment = (char*)ALLOC( size_comment + 1 );
            if ( ziinit.globalcomment ) {
               size_comment = (unsigned long)ziinit.filestream->Read( ziinit.globalcomment, size_comment );
               ziinit.globalcomment[size_comment] = 0;
            }
        }

        byte_before_the_zipfile = central_pos -	( offset_central_dir + size_central_dir );
        ziinit.add_position_when_writting_offset = byte_before_the_zipfile;
        {
            unsigned long size_central_dir_to_read = size_central_dir;
            size_t buf_size = SIZEDATA_INDATABLOCK;
            void* buf_read = (void*)ALLOC( buf_size );
			if ( ziinit.filestream->Seek( offset_central_dir + byte_before_the_zipfile, FS_SEEK_SET ) != 0 ) {
				err = ZIP_ERRNO;
			}

            while ( ( size_central_dir_to_read > 0 ) && ( err == ZIP_OK ) ) {
                unsigned long read_this = SIZEDATA_INDATABLOCK;
				if ( read_this > size_central_dir_to_read ) {
                    read_this = size_central_dir_to_read;
				}
				if ( ziinit.filestream->Read( buf_read, read_this ) != ( int )read_this ) {
                    err = ZIP_ERRNO;
				}
				if ( err == ZIP_OK ) {
                    err = add_data_in_datablock( &ziinit.central_dir, buf_read, (unsigned long)read_this );
				}
                size_central_dir_to_read -= read_this;
            }
            TRYFREE( buf_read );
        }
        ziinit.begin_pos = byte_before_the_zipfile;
        ziinit.number_entry = number_entry_CD;

		if ( ziinit.filestream->Seek( offset_central_dir + byte_before_the_zipfile, FS_SEEK_SET ) != 0 ) {
            err = ZIP_ERRNO;
		}
    }

    if ( globalcomment ) {
		/// ??
		globalcomment = ziinit.globalcomment;
    }
#endif /* !NO_ADDFILEINEXISTINGZIP*/

    if ( err != ZIP_OK ) {
#ifndef NO_ADDFILEINEXISTINGZIP
        TRYFREE( ziinit.globalcomment );
#endif /* !NO_ADDFILEINEXISTINGZIP*/
        TRYFREE( zi );
        return NULL;
    } else {
        *zi = ziinit;
        return (zipFile)zi;
    }
}

/*
========================
zipOpen
========================
*/
zipFile zipOpen( const char *pathname, int append ) {
    return zipOpen2( pathname, append, NULL );
}

/*
========================
zipOpenNewFileInZip3
========================
*/
int zipOpenNewFileInZip3( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local, const void* extrafield_global,
									unsigned int size_extrafield_global, const char* comment, int method, int level, int raw, int windowBits, int memLevel, int strategy, const char* password, unsigned long crcForCrypting ) {
    unsigned int size_filename;
    unsigned int size_comment;
    int err = ZIP_OK;

#ifdef NOCRYPT
	if ( password != NULL ) {
        return ZIP_PARAMERROR;
	}
#endif

	if ( file == NULL ) {
        return ZIP_PARAMERROR;
	}
	if ( ( method != 0 ) && ( method != Z_DEFLATED ) ) {
        return ZIP_PARAMERROR;
	}

    zip_internal* zi = (zip_internal*)file;

    if ( zi->in_opened_file_inzip == 1 ) {
        err = zipCloseFileInZip( file );
		if ( err != ZIP_OK ) {
            return err;
		}
    }

	if ( filename == NULL ) {
        filename = "-";
	}

	if ( comment == NULL ) {
        size_comment = 0;
	} else {
        size_comment = (unsigned int)idStr::Length( comment );
	}

    size_filename = (unsigned int)idStr::Length( filename );

	if ( zipfi == NULL ) {
        zi->ci.dosDate = 0;
	} else {
		if ( zipfi->dosDate != 0 ) {
            zi->ci.dosDate = zipfi->dosDate;
		} else {
			zi->ci.dosDate = ziplocal_TmzDateToDosDate( &zipfi->tmz_date, zipfi->dosDate );
		}
    }

    zi->ci.flag = 0;
	if ( ( level == 8 ) || ( level == 9 ) ) {
      zi->ci.flag |= 2;
	}
	if ( ( level == 2 ) ) {
      zi->ci.flag |= 4;
	}
	if ( ( level == 1 ) ) {
      zi->ci.flag |= 6;
	}
	if ( password != NULL ) {
      zi->ci.flag |= 1;
	}

    zi->ci.crc32 = 0;
    zi->ci.method = method;
    zi->ci.encrypt = 0;
    zi->ci.stream_initialised = 0;
    zi->ci.pos_in_buffered_data = 0;
    zi->ci.raw = raw;
    zi->ci.pos_local_header = (unsigned long)zi->filestream->Tell();
    zi->ci.size_centralheader = SIZECENTRALHEADER + size_filename +	size_extrafield_global + size_comment;
    zi->ci.central_header = (char*)ALLOC( (unsigned int)zi->ci.size_centralheader );

    ziplocal_putValue_inmemory( zi->ci.central_header, (unsigned long)CENTRALHEADERMAGIC, 4 );
    /* version info */
    ziplocal_putValue_inmemory( zi->ci.central_header + 4, (unsigned long)0, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 6, (unsigned long)20, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 8, (unsigned long)zi->ci.flag, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 10, (unsigned long)zi->ci.method, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 12, (unsigned long)zi->ci.dosDate, 4 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 16, (unsigned long)0, 4 ); /*crc*/
    ziplocal_putValue_inmemory( zi->ci.central_header + 20, (unsigned long)0, 4 ); /*compr size*/
    ziplocal_putValue_inmemory( zi->ci.central_header + 24, (unsigned long)0, 4 ); /*uncompr size*/
    ziplocal_putValue_inmemory( zi->ci.central_header + 28, (unsigned long)size_filename, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 30, (unsigned long)size_extrafield_global, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 32, (unsigned long)size_comment, 2 );
    ziplocal_putValue_inmemory( zi->ci.central_header + 34, (unsigned long)0, 2 ); /*disk nm start*/

	if ( zipfi == NULL ) {
        ziplocal_putValue_inmemory( zi->ci.central_header + 36, (unsigned long)0, 2 );
	} else {
        ziplocal_putValue_inmemory( zi->ci.central_header + 36, (unsigned long)zipfi->internal_fa, 2 );
	}

	if ( zipfi == NULL ) {
        ziplocal_putValue_inmemory( zi->ci.central_header + 38,(unsigned long)0, 4);
	} else {
        ziplocal_putValue_inmemory( zi->ci.central_header + 38,(unsigned long)zipfi->external_fa, 4);
	}

    ziplocal_putValue_inmemory( zi->ci.central_header + 42, (unsigned long)zi->ci.pos_local_header - zi->add_position_when_writting_offset, 4 );

	for ( unsigned int i = 0; i < size_filename; i++ ) {
        *( zi->ci.central_header + SIZECENTRALHEADER + i ) = *( filename + i );
	}

	for ( unsigned int i = 0; i < size_extrafield_global; i++ ) {
        *( zi->ci.central_header + SIZECENTRALHEADER + size_filename + i ) = *( ( ( const char* )extrafield_global ) + i );
	}

	for ( unsigned int i = 0; i < size_comment; i++ ) {
        *( zi->ci.central_header + SIZECENTRALHEADER + size_filename + size_extrafield_global + i ) = *( comment + i );
	}

	if ( zi->ci.central_header == NULL ) {
        return ZIP_INTERNALERROR;
	}

    /* write the local header */
    err = ziplocal_putValue( zi->filestream, (unsigned long)LOCALHEADERMAGIC, 4 );

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)20, 2 ); /* version needed to extract */
	}
	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)zi->ci.flag, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)zi->ci.method, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)zi->ci.dosDate, 4 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)0, 4 ); /* crc 32, unknown */
	}
	
	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)0, 4 ); /* compressed size, unknown */
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)0, 4 ); /* uncompressed size, unknown */
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)size_filename, 2 );
	}

	if ( err == ZIP_OK ) {
        err = ziplocal_putValue( zi->filestream, (unsigned long)size_extrafield_local, 2 );
	}

	if ( ( err == ZIP_OK ) && ( size_filename > 0 ) ) {
		if ( zi->filestream->Write( filename, size_filename ) != ( int )size_filename ) {
			err = ZIP_ERRNO;
		}
	}

	if ( ( err == ZIP_OK ) && ( size_extrafield_local > 0 ) ) {
		if ( zi->filestream->Write( extrafield_local, size_extrafield_local ) != ( int )size_extrafield_local ) {
			err = ZIP_ERRNO;
		}
	}

    zi->ci.stream.avail_in = (unsigned int)0;
    zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
    zi->ci.stream.next_out = zi->ci.buffered_data;
    zi->ci.stream.total_in = 0;
    zi->ci.stream.total_out = 0;

    if ( ( err == ZIP_OK ) && ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) ) {
        zi->ci.stream.zalloc = (alloc_func)0;
        zi->ci.stream.zfree = (free_func)0;
        zi->ci.stream.opaque = (voidpf)0;

		if ( windowBits > 0 ) {
            windowBits = -windowBits;
		}

        err = deflateInit2( &zi->ci.stream, level, Z_DEFLATED, windowBits, memLevel, strategy );

		if ( err == Z_OK ) {
            zi->ci.stream_initialised = 1;
		}
    }
#ifndef NOCRYPT
    zi->ci.crypt_header_size = 0;
    if ( ( err == Z_OK ) && ( password != NULL ) ) {
        unsigned char bufHead[RAND_HEAD_LEN];
        unsigned int sizeHead;
        zi->ci.encrypt = 1;
        zi->ci.pcrc_32_tab = get_crc_table();
        /*init_keys( password, zi->ci.keys, zi->ci.pcrc_32_tab );*/

        sizeHead=crypthead( password, bufHead, RAND_HEAD_LEN, zi->ci.keys, zi->ci.pcrc_32_tab, crcForCrypting );
        zi->ci.crypt_header_size = sizeHead;
		if ( ZWRITE( zi->z_filefunc, zi->filestream, bufHead, sizeHead ) != sizeHead ) {
			err = ZIP_ERRNO;
		}
    }
#endif

	if ( err == Z_OK ) {
		zi->in_opened_file_inzip = 1;
	}
    return err;
}

/*
========================
zipOpenNewFileInZip2
========================
*/
int zipOpenNewFileInZip2( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local,
									const void* extrafield_global, unsigned int size_extrafield_global, const char* comment, int method, int level, int raw ) {
    return zipOpenNewFileInZip3( file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global, size_extrafield_global,
                                 comment, method, level, raw, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY, NULL, 0 );
}

/*
========================
zipOpenNewFileInZip
========================
*/
int zipOpenNewFileInZip( zipFile file, const char* filename, const zip_fileinfo* zipfi, const void* extrafield_local, unsigned int size_extrafield_local, const void* extrafield_global,
								unsigned int size_extrafield_global, const char* comment, int method, int level ) {
    return zipOpenNewFileInZip2( file, filename, zipfi, extrafield_local, size_extrafield_local, extrafield_global, size_extrafield_global, comment, method, level, 0 );
}

/*
========================
zipFlushWriteBuffer
========================
*/
int zipFlushWriteBuffer( zip_internal* zi ) {
    int err = ZIP_OK;
    if ( zi->ci.encrypt != 0 ) {
#ifndef NOCRYPT
        int t;
		for ( int i = 0; i < zi->ci.pos_in_buffered_data; i++ ) {
            zi->ci.buffered_data[i] = zencode( zi->ci.keys, zi->ci.pcrc_32_tab, zi->ci.buffered_data[i], t );
		}
#endif
    }
	if ( zi->filestream->Write( zi->ci.buffered_data, zi->ci.pos_in_buffered_data ) != (int)zi->ci.pos_in_buffered_data ) {
		err = ZIP_ERRNO;
	}
    zi->ci.pos_in_buffered_data = 0;
    return err;
}

/*
========================
zipWriteInFileInZip
========================
*/
int zipWriteInFileInZip( zipFile file, const void* buf, unsigned int len ) {
    zip_internal* zi;
    int err = ZIP_OK;

	if ( file == NULL ) {
        return ZIP_PARAMERROR;
	}
    zi = (zip_internal*)file;

	if ( zi->in_opened_file_inzip == 0 ) {
        return ZIP_PARAMERROR;
	}
    zi->ci.stream.next_in = (Bytef*)buf;
    zi->ci.stream.avail_in = len;
    zi->ci.crc32 = crc32( zi->ci.crc32, (byte*)buf, len );

    while ( ( err == ZIP_OK ) && ( zi->ci.stream.avail_in > 0 ) ) {
        if ( zi->ci.stream.avail_out == 0 ) {
			if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
                err = ZIP_ERRNO;
			}
            zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
            zi->ci.stream.next_out = zi->ci.buffered_data;
        }

		if ( err != ZIP_OK ) {
            break;
		}

        if ( ( zi->ci.method == Z_DEFLATED ) && ( !zi->ci.raw ) ) {
            unsigned long uTotalOutBefore = zi->ci.stream.total_out;
            err = deflate( &zi->ci.stream,  Z_NO_FLUSH );
            zi->ci.pos_in_buffered_data += (unsigned int)( zi->ci.stream.total_out - uTotalOutBefore );
        } else {
            unsigned int copy_this;
			if ( zi->ci.stream.avail_in < zi->ci.stream.avail_out ) {
				copy_this = zi->ci.stream.avail_in;
			} else {
				copy_this = zi->ci.stream.avail_out;
			}
			for ( unsigned int i = 0; i < copy_this; i++ ) {
				*( ( (char*)zi->ci.stream.next_out ) + i ) = *( ( (const char*)zi->ci.stream.next_in ) + i );
			}

			zi->ci.stream.avail_in -= copy_this;
            zi->ci.stream.avail_out-= copy_this;
            zi->ci.stream.next_in+= copy_this;
            zi->ci.stream.next_out+= copy_this;
            zi->ci.stream.total_in+= copy_this;
            zi->ci.stream.total_out+= copy_this;
            zi->ci.pos_in_buffered_data += copy_this;
        }
    }

    return err;
}

/*
========================
zipCloseFileInZipRaw
========================
*/
int zipCloseFileInZipRaw( zipFile file, unsigned long uncompressed_size, unsigned long crc32 ) {
    zip_internal* zi;
    unsigned long compressed_size;
    int err = ZIP_OK;

	if ( file == NULL ) {
        return ZIP_PARAMERROR;
	}
    zi = (zip_internal*)file;

	if ( zi->in_opened_file_inzip == 0 ) {
        return ZIP_PARAMERROR;
	}
    zi->ci.stream.avail_in = 0;

	if ( ( zi->ci.method == Z_DEFLATED ) && !zi->ci.raw ) {
        while ( err == ZIP_OK ) {
			unsigned long uTotalOutBefore;
			if ( zi->ci.stream.avail_out == 0 ) {
				if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
					err = ZIP_ERRNO;
				}
				zi->ci.stream.avail_out = (unsigned int)Z_BUFSIZE;
				zi->ci.stream.next_out = zi->ci.buffered_data;
			}
			uTotalOutBefore = zi->ci.stream.total_out;
			err = deflate( &zi->ci.stream, Z_FINISH );
			zi->ci.pos_in_buffered_data += (unsigned int)( zi->ci.stream.total_out - uTotalOutBefore );
		}
	}

	if ( err == Z_STREAM_END ) {
        err = ZIP_OK; /* this is normal */
	}

	if ( ( zi->ci.pos_in_buffered_data > 0 ) && ( err == ZIP_OK ) ) {
		if ( zipFlushWriteBuffer( zi ) == ZIP_ERRNO ) {
            err = ZIP_ERRNO;
		}
	}

    if ( ( zi->ci.method == Z_DEFLATED ) && !zi->ci.raw ) {
        err = deflateEnd( &zi->ci.stream );
        zi->ci.stream_initialised = 0;
    }

    if ( !zi->ci.raw ) {
        crc32 = (unsigned long)zi->ci.crc32;
        uncompressed_size = (unsigned long)zi->ci.stream.total_in;
    }
    compressed_size = (unsigned long)zi->ci.stream.total_out;
#ifndef NOCRYPT
    compressed_size += zi->ci.crypt_header_size;
#endif

    ziplocal_putValue_inmemory( zi->ci.central_header + 16, crc32, 4); /*crc*/
    ziplocal_putValue_inmemory( zi->ci.central_header + 20, compressed_size, 4 ); /*compr size*/
	if ( zi->ci.stream.data_type == Z_ASCII ) {
        ziplocal_putValue_inmemory( zi->ci.central_header + 36, (unsigned long)Z_ASCII, 2 );
	}
    ziplocal_putValue_inmemory( zi->ci.central_header + 24, uncompressed_size, 4 ); /*uncompr size*/

	if ( err == ZIP_OK ) {
        err = add_data_in_datablock( &zi->central_dir, zi->ci.central_header, (unsigned long)zi->ci.size_centralheader );
	}
    TRYFREE( zi->ci.central_header );

    if ( err == ZIP_OK ) {
        long cur_pos_inzip = (long)zi->filestream->Tell();
		if ( zi->filestream->Seek( zi->ci.pos_local_header + 14, FS_SEEK_SET ) != 0 ) {
            err = ZIP_ERRNO;
		}

		if ( err == ZIP_OK ) {
            err = ziplocal_putValue( zi->filestream, crc32, 4 ); /* crc 32, unknown */
		}

		if ( err == ZIP_OK ) { /* compressed size, unknown */
            err = ziplocal_putValue( zi->filestream, compressed_size, 4 );
		}

		if ( err == ZIP_OK ) { /* uncompressed size, unknown */
            err = ziplocal_putValue( zi->filestream, uncompressed_size, 4 );
		}

		if ( zi->filestream->Seek( cur_pos_inzip, FS_SEEK_SET ) != 0 ) {
			err = ZIP_ERRNO;
		}
    }
    zi->number_entry++;
    zi->in_opened_file_inzip = 0;

    return err;
}

/*
========================
zipCloseFileInZip
========================
*/
int zipCloseFileInZip( zipFile file ) {
    return zipCloseFileInZipRaw( file, 0, 0 );
}

/*
========================
zipClose
========================
*/
int zipClose( zipFile file, const char* global_comment ) {
    zip_internal* zi;
    int err = 0;
    unsigned long size_centraldir = 0;
    unsigned long centraldir_pos_inzip;
    unsigned int size_global_comment;
	if ( file == NULL ) {
        return ZIP_PARAMERROR;
	}
    zi = (zip_internal*)file;

    if ( zi->in_opened_file_inzip == 1 ) {
        err = zipCloseFileInZip( file );
    }

#ifndef NO_ADDFILEINEXISTINGZIP
	if ( global_comment == NULL ) {
        global_comment = zi->globalcomment;
	}
#endif
	if ( global_comment == NULL ) {
        size_global_comment = 0;
	} else {
        size_global_comment = (unsigned int)idStr::Length( global_comment );
	}

    centraldir_pos_inzip = (unsigned long)zi->filestream->Tell();
	if ( err == ZIP_OK ) {
        linkedlist_datablock_internal* ldi = zi->central_dir.first_block;
        while ( ldi != NULL ) {
			if ( ( err == ZIP_OK ) && ( ldi->filled_in_this_block > 0 ) ) {
				if ( zi->filestream->Write( ldi->data, ldi->filled_in_this_block ) != (int)ldi->filled_in_this_block ) {
					err = ZIP_ERRNO;
				}
			}
            size_centraldir += ldi->filled_in_this_block;
            ldi = ldi->next_datablock;
        }
    }
    free_datablock( zi->central_dir.first_block );

	if ( err == ZIP_OK ) { /* Magic End */
        err = ziplocal_putValue( zi->filestream, (unsigned long)ENDHEADERMAGIC, 4 );
	}

	if ( err == ZIP_OK ) { /* number of this disk */
		err = ziplocal_putValue( zi->filestream, (unsigned long)0, 2 );
	}

	if ( err == ZIP_OK ) { /* number of the disk with the start of the central directory */
        err = ziplocal_putValue( zi->filestream, (unsigned long)0, 2 );
	}

	if ( err == ZIP_OK ) { /* total number of entries in the central dir on this disk */
        err = ziplocal_putValue( zi->filestream, (unsigned long)zi->number_entry, 2 );
	}

	if ( err == ZIP_OK ) { /* total number of entries in the central dir */
        err = ziplocal_putValue( zi->filestream, (unsigned long)zi->number_entry, 2 );
	}

	if ( err == ZIP_OK ) { /* size of the central directory */
        err = ziplocal_putValue( zi->filestream, (unsigned long)size_centraldir, 4 );
	}

	if ( err == ZIP_OK ) { /* offset of start of central directory with respect to the starting disk number */
        err = ziplocal_putValue( zi->filestream, (unsigned long)( centraldir_pos_inzip - zi->add_position_when_writting_offset ), 4 );
	}

	if ( err == ZIP_OK ) { /* zipfile comment length */
        err = ziplocal_putValue( zi->filestream, (unsigned long)size_global_comment, 2 );
	}

	if ( ( err == ZIP_OK ) && ( size_global_comment > 0 ) ) {
		if ( zi->filestream->Write( global_comment, size_global_comment ) != (int)size_global_comment ) {
			err = ZIP_ERRNO;
		}
	}

	delete zi->filestream;
	zi->filestream = NULL;

#ifndef NO_ADDFILEINEXISTINGZIP
    TRYFREE( zi->globalcomment );
#endif
    TRYFREE( zi );

    return err;
}

/*
========================
idZipBuilder::AddFileFilters
========================
*/
void idZipBuilder::AddFileFilters( const char *filters ) {
#if 0
	idStrList exts;
	idStrListBreakupString( exts, filters, "|" );
	if ( ( exts.Num() > 0 ) && ( exts[ exts.Num() - 1 ] == "" ) ) {
		exts.RemoveIndex( exts.Num() - 1 );
	}
	filterExts.Append( exts );
#endif
}

/*
========================
idZipBuilder::AddUncompressedFileFilters
========================
*/
void idZipBuilder::AddUncompressedFileFilters( const char *filters ) {
#if 0
	idStrList exts;
	idStrListBreakupString( exts, filters, "|" );
	if ( ( exts.Num() > 0 ) && ( exts[ exts.Num() - 1 ] == "" ) ) {
		exts.RemoveIndex( exts.Num() - 1 );
	}
	uncompressedFilterExts.Append( exts );
#endif
}

/*
========================
idZipBuilder::Build

builds a zip file of all the files in the specified folder, overwriting if necessary
========================
*/
bool idZipBuilder::Build( const char* zipPath, const char *folder, bool cleanFolder ) {
	zipFileName = zipPath;
	sourceFolderName = folder;

	if ( !CreateZipFile( false ) ) {
		// don't clean the folder if the zip fails
		return false;
	}

	if ( cleanFolder ) {
		CleanSourceFolder();
	}
	return true;
}

/*
========================
idZipBuilder::Update

updates a zip file with the files in the specified folder
========================
*/
bool idZipBuilder::Update( const char* zipPath, const char *folder, bool cleanFolder ) {
	// if this file doesn't exist, just build it
	if ( fileSystem->GetTimestamp( zipPath ) == FILE_NOT_FOUND_TIMESTAMP ) {
		return Build( zipPath, folder, cleanFolder );
	}
	zipFileName = zipPath;
	sourceFolderName = folder;

	if ( !CreateZipFile( true ) ) {
		// don't clean the folder if the zip fails
		return false;
	}

	if ( cleanFolder ) {
		CleanSourceFolder();
	}
	return true;
}

/*
========================
idZipBuilder::GetFileTime
========================
*/
bool idZipBuilder::GetFileTime( const idStr &filename, unsigned long *dostime ) const {
	{
		FILETIME filetime;
		WIN32_FIND_DATA fileData;
		HANDLE			findHandle = FindFirstFile( filename.c_str(), &fileData );
		if ( findHandle != INVALID_HANDLE_VALUE ) { 
			FileTimeToLocalFileTime( &(fileData.ftLastWriteTime), &filetime );
			FileTimeToDosDateTime( &filetime, ((LPWORD)dostime) + 1, ((LPWORD)dostime) + 0 );
			FindClose( findHandle );
			return true;
		}
		FindClose( findHandle );
	}
	return false;
}

/*
========================
idZipBuilder::IsFiltered
========================
*/
bool idZipBuilder::IsFiltered( const idStr &filename ) const {
	if ( filterExts.Num() == 0 && uncompressedFilterExts.Num() == 0 ) {
		return false;
	}
	for ( int j = 0; j < filterExts.Num(); j++ ) {
		idStr fileExt = idStr( "." + filterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return false;
		}
	}
	for ( int j = 0; j < uncompressedFilterExts.Num(); j++ ) {
		idStr fileExt = idStr( "." + uncompressedFilterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return false;
		}
	}
	return true;
}

/*
========================
idZipBuilder::IsUncompressed
========================
*/
bool idZipBuilder::IsUncompressed( const idStr &filename ) const {
	if ( uncompressedFilterExts.Num() == 0 ) {
		return false;
	}
	for ( int j = 0; j < uncompressedFilterExts.Num(); j++ ) {
		idStr fileExt = idStr( "." + uncompressedFilterExts[j] );
		if ( filename.Right( fileExt.Length() ).Icmp( fileExt ) == 0 ) {
			return true;
		}
	}
	return false;
}

/*
========================
idZipBuilder::CreateZipFile
========================
*/
bool idZipBuilder::CreateZipFile( bool appendFiles ) {
#if 0
//#ifdef ID_PC
	if ( zipFileName.IsEmpty() || sourceFolderName.IsEmpty() ) {
		idLib::Warning( "[%s] - invalid parameters!", __FUNCTION__ );
		return false;
	}

	// need to clear the filesystem's zip cache before we can open and write
	//fileSystem->ClearZipCache();

	idLib::Printf( "Building zip file: '%s'\n", zipFileName.c_str() );

	sourceFolderName.StripTrailing( "\\" );
	sourceFolderName.StripTrailing( "/" );

#if 0
	// attempt to check the file out
	if ( !Sys_IsFileWritable( zipFileName ) ) {
		if ( ( idLib::sourceControl == NULL ) || !idLib::sourceControl->CheckOut( zipFileName ) ) {
			idLib::Warning( "READONLY zip file couldn't be checked out: %s", zipFileName.c_str() );
		} else {
			idLib::Printf( "Checked out: %s\n", zipFileName.c_str() );
		}
	}
#endif

	// if not appending, set the file size to zero to "create it from scratch"
	if ( !appendFiles ) {
		idLib::PrintfIf( zip_verbosity.GetBool(), "Overwriting zip file: '%s'\n", zipFileName.c_str() );
		idFile *zipFile = fileSystem->OpenExplicitFileWrite( zipFileName );
		if ( zipFile != NULL ) {
			delete zipFile;
			zipFile = NULL;
		}
	} else {
		idLib::PrintfIf( zip_verbosity.GetBool(), "Appending to zip file: '%s'\n", zipFileName.c_str() );
	}

	// enumerate the files to zip up in the source folder
	idStrStatic< MAX_OSPATH > relPath;
	relPath = 
	fileSystem->OSPathToRelativePath( sourceFolderName );
	idFileList *files = fileSystem->ListFilesTree( relPath, "*.*" );

	// check to make sure that at least one file will be added to the package
	int atLeastOneFilteredFile = false;
	for ( int i = 0; i < files->GetNumFiles(); i++ ) {
		idStr filename = files->GetFile( i );

		if ( !IsFiltered( filename ) ) {
			atLeastOneFilteredFile = true;
			break;
		}
	}
	if ( !atLeastOneFilteredFile ) {
		// although we didn't actually update/create a zip file, it's because no files would be added anyway, which would result in a corrupted zip
		idLib::Printf( "Skipping zip creation/modification, no additional changes need to be made...\n" );
		return true;
	}

	// open the zip file
	zipFile zf = zipOpen( zipFileName, appendFiles ? APPEND_STATUS_ADDINZIP : 0 );
	if ( zf == NULL ) {
		idLib::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	// add the files to the zip file
	for ( int i = 0; i < files->GetNumFiles(); i++ ) {

		// add each file to the zip file
		zip_fileinfo zi;
		memset( &zi, 0, sizeof( zip_fileinfo ) );
		
		idStr filename = files->GetFile( i );

		if ( IsFiltered( filename ) ) {
			idLib::PrintfIf( zip_verbosity.GetBool(), "...Skipping: '%s'\n", filename.c_str() );
			continue;				
		}
		
		idStr filenameInZip = filename;
		filenameInZip.Strip( relPath );
		filenameInZip.StripLeading( "/" );

		idStrStatic< MAX_OSPATH > ospath;
		ospath = fileSystem->RelativePathToOSPath( filename );
		GetFileTime( ospath, &zi.dosDate );

		idLib::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", filenameInZip.c_str() );

		int compressionMethod = Z_DEFLATED;
		if ( IsUncompressed( filenameInZip ) ) {
			compressionMethod = 0;
		}

		int errcode = zipOpenNewFileInZip3( zf, filenameInZip, &zi, NULL, 0, NULL, 0, NULL /* comment*/,
											compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL, 
											Z_DEFAULT_STRATEGY, NULL /*password*/, 0 /*fileCRC*/ );

		if ( errcode != ZIP_OK ) {
			idLib::Warning( "Error opening file in zipfile!" );
			continue;
		} else {
			// open the source file
			idFile_Permanent src( filename, ospath, FS_READ );
			if ( !src.IsOpen() ) {
				idLib::Warning( "Error opening source file!" );
				continue;
			}

			// copy the file data into the zip file
			idTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
			size_t total = 0;
			while ( size_t bytesRead = src.Read( buffer.Ptr(), buffer.Size() ) ) {
				if ( bytesRead > 0 ) {
					errcode = zipWriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
					if ( errcode != ZIP_OK ) {
						idLib::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
						continue;
					}
				}
				total += bytesRead;
			}
			assert( total == (size_t)src.Length() );
		}

		errcode = zipCloseFileInZip( zf );
		if ( errcode != ZIP_OK ) {
			idLib::Warning( "Error zipping source file!" );
			continue;
		}
		idLib::PrintfIf( zip_verbosity.GetBool(), "\n" );
	}

	// free the file list
	if ( files != NULL ) {
		fileSystem->FreeFileList( files );
	}

	// close the zip file
	int closeError = zipClose( zf, NULL );
	if ( closeError != ZIP_OK ) {
		idLib::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	idLib::Printf( "Done.\n" );

	return true;
#else

	return false;
#endif

}

/*
========================
idZipBuilder::CreateZipFileFromFileList
========================
*/
bool idZipBuilder::CreateZipFileFromFileList( const char *name, const idList< idFile_Memory * > & srcFiles ) {
	zipFileName = name;
	return CreateZipFileFromFiles( srcFiles );
}
/*
========================
idZipBuilder::CreateZipFileFromFiles
========================
*/
bool idZipBuilder::CreateZipFileFromFiles( const idList< idFile_Memory * > & srcFiles ) {
	if ( zipFileName.IsEmpty() ) {
		idLib::Warning( "[%s] - invalid parameters!", __FUNCTION__ );
		return false;
	}

	// need to clear the filesystem's zip cache before we can open and write
	//fileSystem->ClearZipCache();

	idLib::Printf( "Building zip file: '%s'\n", zipFileName.c_str() );

	// do not allow overwrite as this should be a tempfile attempt to check the file out
	if ( !Sys_IsFileWritable( zipFileName ) ) {
		idLib::PrintfIf( zip_verbosity.GetBool(), "File %s not writeable, cannot proceed.\n", zipFileName.c_str() );
		return false;
	}

	// open the zip file
	zipFile zf = zipOpen( zipFileName, 0 );
	if ( zf == NULL ) {
		idLib::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	// add the files to the zip file
	for ( int i = 0; i < srcFiles.Num(); i++ ) {

		// add each file to the zip file
		zip_fileinfo zi;
		memset( &zi, 0, sizeof( zip_fileinfo ) );

		idFile_Memory * src = srcFiles[i];
		src->MakeReadOnly();

		idLib::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", src->GetName() );

		int compressionMethod = Z_DEFLATED;
		if ( IsUncompressed( src->GetName() ) ) {
			compressionMethod = 0;
		}

		int errcode = zipOpenNewFileInZip3( zf, src->GetName(), &zi, NULL, 0, NULL, 0, NULL /* comment*/,
			compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL, 
			Z_DEFAULT_STRATEGY, NULL /*password*/, 0 /*fileCRC*/ );

		if ( errcode != ZIP_OK ) {
			idLib::Warning( "Error opening file in zipfile!" );
			continue;
		} else {
			// copy the file data into the zip file
			idTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
			size_t total = 0;
			while ( size_t bytesRead = src->Read( buffer.Ptr(), buffer.Size() ) ) {
				if ( bytesRead > 0 ) {
					errcode = zipWriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
					if ( errcode != ZIP_OK ) {
						idLib::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
						continue;
					}
				}
				total += bytesRead;
			}
			assert( total == (size_t)src->Length() );
		}

		errcode = zipCloseFileInZip( zf );
		if ( errcode != ZIP_OK ) {
			idLib::Warning( "Error zipping source file!" );
			continue;
		}
		idLib::PrintfIf( zip_verbosity.GetBool(), "\n" );
	}

	// close the zip file
	int closeError = zipClose( zf, zipFileName );
	if ( closeError != ZIP_OK ) {
		idLib::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
		return false;
	}

	idLib::PrintfIf( zip_verbosity.GetBool(), "Done.\n" );

	return true;
}

/*
========================
idZipBuilder::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
zipFile idZipBuilder::CreateZipFile( const char *name ) {
	idLib::Printf( "Creating zip file: '%s'\n", name );

	// do not allow overwrite as this should be a tempfile attempt to check the file out
	if ( !Sys_IsFileWritable( name ) ) {
		idLib::PrintfIf( zip_verbosity.GetBool(), "File %s not writeable, cannot proceed.\n", name );
		return NULL;
	}

	// open the zip file
	zipFile zf = zipOpen( name, 0 );
	if ( zf == NULL ) {
		idLib::Warning( "[%s] - error opening file '%s'!", __FUNCTION__, name );
	}
	return zf;
}

/*
========================
idZipBuilder::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
bool idZipBuilder::AddFile( zipFile zf, idFile_Memory *src, bool deleteFile ) {
	// add each file to the zip file
	zip_fileinfo zi;
	memset( &zi, 0, sizeof( zip_fileinfo ) );


	src->MakeReadOnly();

	idLib::PrintfIf( zip_verbosity.GetBool(), "...Adding: '%s' ", src->GetName() );

	int compressionMethod = Z_DEFLATED;
	if ( IsUncompressed( src->GetName() ) ) {
		compressionMethod = Z_NO_COMPRESSION;
	}

	int errcode = zipOpenNewFileInZip3( zf, src->GetName(), &zi, NULL, 0, NULL, 0, NULL /* comment*/,
		compressionMethod,	DEFAULT_COMPRESSION_LEVEL, 0, -MAX_WBITS, DEF_MEM_LEVEL, 
		Z_DEFAULT_STRATEGY, NULL /*password*/, 0 /*fileCRC*/ );

	if ( errcode != ZIP_OK ) {
		idLib::Warning( "Error opening file in zipfile!" );
		if ( deleteFile ) {
			src->Clear( true );
			delete src;
		}
		return false;
	} else {
		// copy the file data into the zip file
		idTempArray<byte> buffer( DEFAULT_WRITEBUFFERSIZE );
		size_t total = 0;
		while ( size_t bytesRead = src->Read( buffer.Ptr(), buffer.Size() ) ) {
			if ( bytesRead > 0 ) {
				errcode = zipWriteInFileInZip( zf, buffer.Ptr(), (unsigned int)bytesRead );
				if ( errcode != ZIP_OK ) {
					idLib::Warning( "Error writing to zipfile (%i bytes)!", bytesRead );
					continue;
				}
			}
			total += bytesRead;
		}
		assert( total == (size_t)src->Length() );
	}

	errcode = zipCloseFileInZip( zf );
	if ( errcode != ZIP_OK ) {
		idLib::Warning( "Error zipping source file!" );
		if ( deleteFile ) {
			src->Clear( true );
			delete src;
		}
		return false;
	}
	idLib::PrintfIf( zip_verbosity.GetBool(), "\n" );
	if ( deleteFile ) {
		src->Clear( true );
		delete src;
	}
	return true;
}

/*
========================
idZipBuilder::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
void idZipBuilder::CloseZipFile( zipFile zf ) {
	// close the zip file
	int closeError = zipClose( zf, zipFileName );
	if ( closeError != ZIP_OK ) {
		idLib::Warning( "[%s] - error closing file '%s'!", __FUNCTION__, zipFileName.c_str() );
	}
	idLib::PrintfIf( zip_verbosity.GetBool(), "Done.\n" );
}
/*
========================
idZipBuilder::CleanSourceFolder

this folder is assumed to be a path under FSPATH_BASE
========================
*/
void idZipBuilder::CleanSourceFolder() {
#if 0
//#ifdef ID_PC_WIN
	idStrList deletedFiles;

	// make sure this is a valid path, we don't want to go nuking
	// some user path  or something else unintentionally
	idStr ospath = sourceFolderName;
	ospath.SlashesToBackSlashes();
	ospath.ToLower();

	char relPath[MAX_OSPATH];
	fileSystem->OSPathToRelativePath( ospath, relPath, MAX_OSPATH );

	// get the game's base path
	idStr basePath = fileSystem->GetBasePathStr( FSPATH_BASE );
	basePath.AppendPath( BASE_GAMEDIR );
	basePath.AppendPath( "maps" );
	basePath.SlashesToBackSlashes();
	basePath.ToLower();
	// path must be off of our base path, ospath can't have .map on the end, and
	// do some additional sanity checks
	if ( ( ospath.Find( basePath ) == 0 ) && ( ospath.Right( 4 ) != ".map" ) && 
		( ospath != "c:\\" ) && ( ospath.Length() > basePath.Length() ) ) {
			// get the files in the current directory
			idFileList *files = fileSystem->ListFilesTree( relPath, "*.*" );
			if ( files->GetNumFiles() && zip_verbosity.GetBool() ) {
				idLib::Printf( "Deleting files in '%s'...\n", relPath );
			}
			for ( int i = 0; i < files->GetNumFiles(); i++ ) {
				if ( IsFiltered( files->GetFile( i ) ) ) {
					continue;
				}
				// nuke 'em
				if ( zip_verbosity.GetBool() ) {
					idLib::Printf( "\t...%s\n", files->GetFile( i ) );
				}
				fileSystem->RemoveFile( files->GetFile( i ) );

				char ospath2[MAX_OSPATH];
				fileSystem->RelativePathToOSPath( files->GetFile( i ), ospath2, MAX_OSPATH );
				deletedFiles.Append( ospath2 );
			}
			fileSystem->FreeFileList( files );
			fileSystem->RemoveDir( relPath );
	} else {
		idLib::Printf( "Warning: idZipBuilder::CleanSourceFolder - Non-standard path: '%s'!\n", ospath.c_str() );
		return;
	}

	// figure out which deleted files need to be removed from source control, and then remove those files
	idStrList filesToRemoveFromSourceControl;
	for ( int i = 0; i < deletedFiles.Num(); i++ ) {
		scFileStatus_t fileStatus = idLib::sourceControl->GetFileStatus( deletedFiles[ i ] );
		if ( SCF_IS_IN_SOURCE_CONTROL( fileStatus ) ) {
			filesToRemoveFromSourceControl.Append( deletedFiles[ i ] );
		}
	}
	if ( filesToRemoveFromSourceControl.Num() > 0 ) {
		idLib::sourceControl->Delete( filesToRemoveFromSourceControl );
	}

#endif
}

/*
========================
idZipBuilder::BuildMapFolderZip
========================
*/
const char *ZIP_FILE_EXTENSION = "pk4";
bool idZipBuilder::BuildMapFolderZip( const char *mapFileName ) {
	idStr zipFileName = mapFileName;
	zipFileName.SetFileExtension( ZIP_FILE_EXTENSION );
	idStr pathToZip = mapFileName;
	pathToZip.StripFileExtension();
	idZipBuilder zip;
	zip.AddFileFilters( "bcm|bmodel|proc|" );
	zip.AddUncompressedFileFilters( "genmodel|sbcm|tbcm|" );
	bool success = zip.Build( zipFileName, pathToZip, true );
	// even if the zip build failed we want to clear the source folder so no contributing files are left around
	if ( !success ) {
		zip.CleanSourceFolder();
	}
	return success;
}

/*
========================
idZipBuilder::UpdateMapFolderZip
========================
*/
bool idZipBuilder::UpdateMapFolderZip( const char *mapFileName ) {
	idStr zipFileName = mapFileName;
	zipFileName.SetFileExtension( ZIP_FILE_EXTENSION );
	idStr pathToZip = mapFileName;
	pathToZip.StripFileExtension();
	idZipBuilder zip;
	zip.AddFileFilters( "bcm|bmodel|proc|" );
	zip.AddUncompressedFileFilters( "genmodel|sbcm|tbcm|" );
	bool success = zip.Update( zipFileName, pathToZip, true );
	// even if the zip build failed we want to clear the source folder so no contributing files are left around
	if ( !success ) {
		zip.CleanSourceFolder();
	}
	return success;
}

/*
========================
idZipBuilder::CombineFiles
========================
*/
idFile_Memory * idZipBuilder::CombineFiles( const idList< idFile_Memory * > & srcFiles ) {
	idFile_Memory * destFile = NULL;

#if 0
//#ifdef ID_PC

	// create a new temp file so we can zip into it without refactoring the zip routines
	char ospath[MAX_OSPATH];
	const char * tempName = "temp.tmp";
	fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );
	fileSystem->RemoveFile( ospath );

	// combine src files into dest filename just specified
	idZipBuilder zip;
	zip.zipFileName = ospath;
	bool ret = zip.CreateZipFileFromFiles( srcFiles );

	// read the temp file created into a memory file to return
	if ( ret ) {
		destFile = new idFile_Memory();

		if ( !destFile->Load( tempName, ospath ) ) {
			assert( false && "couldn't read the combined file" );
			delete destFile;
			destFile = NULL;
		}

		// delete the temp file
		fileSystem->RemoveFile( ospath );

		// make the new file readable
		destFile->MakeReadOnly();
	}

#endif

	return destFile;
}

CONSOLE_COMMAND( testZipBuilderCombineFiles, "test routine for memory zip file building", 0 ) {
#if 0
	idList< idFile_Memory * > list;
	const char *	testString = "test";
	int				numFiles = 2;

	if ( args.Argc() > 2 ) {
		idLib::Printf( "usage: testZipBuilderExtractFiles [numFiles]\n" );
		return;
	} 

	for ( int arg = 1; arg < args.Argc(); arg++ ) {
		numFiles = atoi( args.Argv( arg ) );
	}

	// allocate all the test files
	for ( int i = 0; i < numFiles; i++ ) {
		idFile_Memory * file = new idFile_Memory( va( "%s%d.txt", testString, i + 1 ) );
		file->MakeWritable();
		idStr str = va( "%s%d", testString, i + 1 );
		file->WriteString( str );
		list.Append( file );
	}

	// combine the files into a single memory file
	idZipBuilder zip;
	idFile_Memory * file = zip.CombineFiles( list );
	if ( file != NULL ) {
		file->MakeReadOnly();

		char ospath[MAX_OSPATH];
		const char * tempName = "temp.zip";
		fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );

		// remove previous file if it exists
		fileSystem->RemoveFile( ospath );

		if ( file->Save( tempName, ospath ) ) {
			idLib::PrintfIf( zip_verbosity.GetBool(), va( "File written: %s.\n", ospath ) );
		} else {
			idLib::Error( "Could not save the file." );
		}

		delete file;
	}

	list.DeleteContents();
#endif
	// Now look at the temp.zip, unzip it to see if it works
}

/*
========================
idZipBuilder::ExtractFiles
========================
*/
bool idZipBuilder::ExtractFiles( idFile_Memory * & srcFile, idList< idFile_Memory * > & destFiles ) {
	bool ret = false;

#if 0
//#ifdef ID_PC

	destFiles.Clear();

	// write the memory file to temp storage so we can unzip it without refactoring the unzip routines
	char ospath[MAX_OSPATH];
	const char * tempName = "temp.tmp";
	fileSystem->RelativePathToOSPath( tempName, ospath, MAX_OSPATH, FSPATH_SAVE );
	ret = srcFile->Save( tempName, ospath );
	assert( ret && "couldn't create temp file" );

	if ( ret ) {

		idLib::PrintfIf( zip_verbosity.GetBool(), "Opening archive %s:\n", ospath );
		unzFile zip = unzOpen( ospath );

		int numFiles = 0;
		int result = unzGoToFirstFile( zip );
		while( result == UNZ_OK ) {
			numFiles++;
			unz_file_info curFileInfo;
			char fileName[MAX_OSPATH];
			unzGetCurrentFileInfo( zip, &curFileInfo, fileName, MAX_OSPATH, NULL, 0, NULL, 0 );

			idLib::PrintfIf( zip_verbosity.GetBool(), "%d: %s, size: %d \\ %d\n", numFiles, fileName, curFileInfo.compressed_size, curFileInfo.uncompressed_size );

			// create a buffer big enough to hold the entire uncompressed file
			void * buff = Mem_Alloc( curFileInfo.uncompressed_size, TAG_TEMP );
			result = unzOpenCurrentFile( zip );
			if ( result == UNZ_OK ) {
				result = unzReadCurrentFile( zip, buff, curFileInfo.uncompressed_size );
				unzCloseCurrentFile( zip );
			}

			// create the new memory file
			idFile_Memory * outFile = new idFile_Memory( fileName );
			outFile->SetReadOnlyData( (const char *)buff, curFileInfo.uncompressed_size );

			destFiles.Append( outFile );

			result = unzGoToNextFile( zip );
		}

		// close it so we can delete the zip file and create a new one
		unzClose( zip );

		// delete the temp zipfile
		fileSystem->RemoveFile( ospath );
	}

#endif

	return ret;
}

CONSOLE_COMMAND( testZipBuilderExtractFiles, "test routine for memory zip file extraction", 0 ) {
#if 0
	idList< idFile_Memory * > list;
	idFile_Memory * zipfile;
	const char *	testString = "test";
	int				numFiles = 2;
	bool			overallSuccess = true;
	bool			success = true;

	if ( args.Argc() > 2 ) {
		idLib::Printf( "usage: testZipBuilderExtractFiles [numFiles]\n" );
		return;
	} 

	for ( int arg = 1; arg < args.Argc(); arg++ ) {
		numFiles = atoi( args.Argv( arg ) );
	}

	// create a temp.zip file with string files
	{
		// allocate all the test files
		for ( int i = 0; i < numFiles; i++ ) {
			idFile_Memory * file = new idFile_Memory( va( "%s%d.txt", testString, i + 1 ) );
			file->MakeWritable();
			idStr str = va( "%s%d", testString, i + 1 );
			file->WriteString( str );
			list.Append( file );
		}

		// combine the files into a single memory file
		idZipBuilder zip;
		zipfile = zip.CombineFiles( list );
		
		success = ( zipfile != NULL );
		overallSuccess &= success;
		idLib::Printf( "Zip file created: %s\n", success ? "^2PASS" : "^1FAIL" );

		// destroy all the test files
		list.DeleteContents();
	}

	// unzip the file into separate memory files
	if ( overallSuccess ) {

		// extract all the test files using the single zip file from above
		idZipBuilder zip;
		if ( !zip.ExtractFiles( zipfile, list ) ) {
			idLib::Error( "Could not extract files." );
		}

		success = ( list.Num() == numFiles );
		overallSuccess &= success;
		idLib::Printf( "Number of files: %s\n", success ? "^2PASS" : "^1FAIL" );

		for ( int i = 0; i < list.Num(); i++ ) {
			idStr str;
			idFile_Memory * file = list[i];
			file->MakeReadOnly();
			file->ReadString( str );

			idStr filename = va( "%s%d.txt", testString, i + 1 );
			idStr contents = va( "%s%d", testString, i + 1 );

			// test the filename
			bool nameSuccess = ( file->GetName() == filename );
			overallSuccess &= nameSuccess;

			// test the string 
			bool contentSuccess = ( str == contents );
			overallSuccess &= contentSuccess;

			idLib::Printf( "Extraction of file, %s: %s^0, contents check: %s\n", filename.c_str(), nameSuccess ? "^2PASS" : "^1FAIL", contentSuccess ? "^2PASS" : "^1FAIL" );
		}

		list.DeleteContents();
	}

	if ( zipfile != NULL ) {
		delete zipfile;
	}

	idLib::Printf( "[%s] overall tests: %s\n", __FUNCTION__, overallSuccess ? "^2PASS" : "^1FAIL" );
#endif
}

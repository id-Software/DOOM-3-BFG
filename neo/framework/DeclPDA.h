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

#ifndef __DECLPDA_H__
#define __DECLPDA_H__

/*
===============================================================================

	idDeclPDA

===============================================================================
*/


class idDeclEmail : public idDecl
{
public:
	idDeclEmail() {}

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char* 			GetFrom() const
	{
		return from;
	}
	const char* 			GetBody() const
	{
		return text;
	}
	const char* 			GetSubject() const
	{
		return subject;
	}
	const char* 			GetDate() const
	{
		return date;
	}
	const char* 			GetTo() const
	{
		return to;
	}

private:
	idStr					text;
	idStr					subject;
	idStr					date;
	idStr					to;
	idStr					from;
};


class idDeclVideo : public idDecl
{
public:
	idDeclVideo() : preview( NULL ), video( NULL ), audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const idMaterial* 		GetRoq() const
	{
		return video;
	}
	const idSoundShader* 	GetWave() const
	{
		return audio;
	}
	const char* 			GetVideoName() const
	{
		return videoName;
	}
	const char* 			GetInfo() const
	{
		return info;
	}
	const idMaterial* 		GetPreview() const
	{
		return preview;
	}

private:
	const idMaterial* 		preview;
	const idMaterial* 		video;
	idStr					videoName;
	idStr					info;
	const idSoundShader* 	audio;
};


class idDeclAudio : public idDecl
{
public:
	idDeclAudio() : audio( NULL ) {};

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	const char* 			GetAudioName() const
	{
		return audioName;
	}
	const idSoundShader* 	GetWave() const
	{
		return audio;
	}
	const char* 			GetInfo() const
	{
		return info;
	}

private:
	const idSoundShader* 	audio;
	idStr					audioName;
	idStr					info;
};

class idDeclPDA : public idDecl
{
public:
	idDeclPDA()
	{
		originalEmails = originalVideos = 0;
	};

	virtual size_t			Size() const;
	virtual const char* 	DefaultDefinition() const;
	virtual bool			Parse( const char* text, const int textLength, bool allowBinaryVersion );
	virtual void			FreeData();
	virtual void			Print() const;
	virtual void			List() const;

	virtual void			AddVideo( const idDeclVideo* video, bool unique = true ) const
	{
		if( unique )
		{
			videos.AddUnique( video );
		}
		else
		{
			videos.Append( video );
		}
	}
	virtual void			AddAudio( const idDeclAudio* audio, bool unique = true ) const
	{
		if( unique )
		{
			audios.AddUnique( audio );
		}
		else
		{
			audios.Append( audio );
		}
	}
	virtual void			AddEmail( const idDeclEmail* email, bool unique = true ) const
	{
		if( unique )
		{
			emails.AddUnique( email );
		}
		else
		{
			emails.Append( email );
		}
	}
	virtual void			RemoveAddedEmailsAndVideos() const;

	virtual const int		GetNumVideos() const
	{
		return videos.Num();
	}
	virtual const int		GetNumAudios() const
	{
		return audios.Num();
	}
	virtual const int		GetNumEmails() const
	{
		return emails.Num();
	}
	virtual const idDeclVideo* GetVideoByIndex( int index ) const
	{
		return ( index < 0 || index > videos.Num() ? NULL : videos[index] );
	}
	virtual const idDeclAudio* GetAudioByIndex( int index ) const
	{
		return ( index < 0 || index > audios.Num() ? NULL : audios[index] );
	}
	virtual const idDeclEmail* GetEmailByIndex( int index ) const
	{
		return ( index < 0 || index > emails.Num() ? NULL : emails[index] );
	}

	virtual void			SetSecurity( const char* sec ) const;

	const char* 			GetPdaName() const
	{
		return pdaName;
	}
	const char* 			GetSecurity() const
	{
		return security;
	}
	const char* 			GetFullName() const
	{
		return fullName;
	}
	const char* 			GetIcon() const
	{
		return icon;
	}
	const char* 			GetPost() const
	{
		return post;
	}
	const char* 			GetID() const
	{
		return id;
	}
	const char* 			GetTitle() const
	{
		return title;
	}

private:
	mutable idList<const idDeclVideo*>	videos;
	mutable idList<const idDeclAudio*>	audios;
	mutable idList<const idDeclEmail*>	emails;
	idStr					pdaName;
	idStr					fullName;
	idStr					icon;
	idStr					id;
	idStr					post;
	idStr					title;
	mutable idStr			security;
	mutable	int				originalEmails;
	mutable int				originalVideos;
};

#endif /* !__DECLPDA_H__ */
